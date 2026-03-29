#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(_WIN32) || defined(linux)
#include <GL/glut.h>
#elif defined(__APPLE__)
#include <GLUT/glut.h>
#endif

#include <HD/hd.h>
#include <HDU/hduMatrix.h>
#include <HDU/hduError.h>

#include <HL/hl.h>
#include <HLU/hlu.h>

#include "app_state.h"
#include "scene.h"
#include "ui.h"
#include "obj_loader.h"


/* Haptic device and rendering context handles. */
HHD ghHD = HD_INVALID_HANDLE; //햅틱 디바이스 핸들. 초기에는 invalid로 설정
HHLRC ghHLRC = 0; //햅틱 디바이스 컨텍스트

/* Shape id for shape we will render haptically. */
HLuint gShapeId;

/*cursor*/
double gCursorScale = 1.0;
GLuint gCursorDisplayList = 0;

/* material properties */
float gStiffness = 0.7;
float gDamping = 0.1f;
float gStaticFriction = 0.2f;
float gDynamicFriction = 0.3f;

/* Window size */
int gWindowWidth = 1280;
int gWindowHeight = 1080;

/* 그랩 인터랙션 */
GrabObjectState gGrabObj =
{
	false, false,
	hduVector3Dd(0.0, 0.0, 0.0), // position
	hduVector3Dd(0.0, 0.0, 0.0), // velocity
	hduVector3Dd(0.0, 0.0, 0.0), // targetPosition
	hduVector3Dd(0.0, 0.0, 0.0), // grabOffset
	1.0,   // mass
	18.0,  // springK
	7.0    // dampingC
};

bool gUseVirtualCoupling = false;

/* Propety Control Buttons*/
UIButton gButtons[] =
{
	{0.25f, 0.93f, 0.05f, 0.04f, "-", &gStiffness, -0.05f},
	{0.32f, 0.93f, 0.05f, 0.04f, "+", &gStiffness, +0.05f},
	{0.25f, 0.88f, 0.05f, 0.04f, "-", &gDamping, -0.05f},
	{0.32f, 0.88f, 0.05f, 0.04f, "+", &gDamping, +0.05f},
	{0.25f, 0.83f, 0.05f, 0.04f, "-", &gStaticFriction, -0.05f},
	{0.32f, 0.83f, 0.05f, 0.04f, "+", &gStaticFriction, +0.05f},
	{0.25f, 0.78f, 0.05f, 0.04f, "-", &gDynamicFriction, -0.05f},
	{0.32f, 0.78f, 0.05f, 0.04f, "+", &gDynamicFriction, +0.05f}
};

const int gButtonCount = sizeof(gButtons) / sizeof(gButtons[0]);


/* 그랩 인터랙션 헬퍼 함수 */
static hduVector3Dd getProxyPosition()
{
	HLdouble p[3];
	hlGetDoublev(HL_PROXY_POSITION, p);
	return hduVector3Dd(p[0], p[1], p[2]);
}

static double degToRad(double deg)
{
	return deg * 3.1415926 / 180.0;
}

static hduVector3Dd rotateVEctorEulerXYZ(const hduVector3Dd& v, const hduVector3Dd& eulerDeg)
{
	double rx = degToRad(eulerDeg[0]);
	double ry = degToRad(eulerDeg[1]);
	double rz = degToRad(eulerDeg[2]);

	double cx = cos(rx), sx = sin(rx);
	hduVector3Dd r1(v[0], cx * v[1] - sx * v[2], sx * v[1] + cx * v[2]);

	double cy = cos(ry), sy = sin(ry);
	hduVector3Dd r2(cy * r1[0] + sy * r1[2], r1[1], -sy * r1[0] + cy * r1[2]);

	double cz = cos(rz), sz = sin(rz);
	hduVector3Dd r3(cz * r2[0] - sz * r2[1], sz * r2[0] + cz * r2[1], r2[2]);

	return r3;

}

static void beginGrab(const hduVector3Dd& proxyPos)
{
	if (!gGrabObj.isTouched)
		return;

	gGrabObj.isGrabbed = true;

	// 위치
	gGrabObj.grabOffset = gGrabObj.position - proxyPos;
	gGrabObj.targetPosition = gGrabObj.position;

	// 회전
	gGrabObj.lastProxyPos = proxyPos;
	gGrabObj.grabLocalPoint = proxyPos - gGrabObj.position;
	gGrabObj.targetRotationEuler = gGrabObj.rotationEuler;
}

static void endGrab()
{
	gGrabObj.isGrabbed = false;
}

static void updateGrabTarget(const hduVector3Dd& proxyPos)
{
	if (!gGrabObj.isGrabbed)
		return;

	gGrabObj.targetPosition = proxyPos + gGrabObj.grabOffset;
}

static void updateGrabRotationTarget(const hduVector3Dd& proxyPos)
{
	if (!gGrabObj.isGrabbed)
		return;

	hduVector3Dd delta = proxyPos - gGrabObj.lastProxyPos;
	gGrabObj.lastProxyPos = proxyPos;

	//좌우 이동 -> y축 회전
	//상하 이동 -> x축 회전
	gGrabObj.targetRotationEuler[1] += delta[0] * gGrabObj.rotationSensitivity;
	gGrabObj.targetRotationEuler[0] += -delta[1] * gGrabObj.rotationSensitivity;
}

static void updateObjectMotion(double dt)
{
	if (!gGrabObj.isGrabbed)
		return;

	if (!gUseVirtualCoupling)
	{
		//위치
		gGrabObj.position = gGrabObj.targetPosition;
		gGrabObj.velocity.set(0.0, 0.0, 0.0);

		//회전
		gGrabObj.rotationEuler = gGrabObj.targetRotationEuler;
		gGrabObj.angularVelocity.set(0.0, 0.0, 0.0);
	}
	else
	{
		//위치 커플링
		hduVector3Dd x = gGrabObj.position;
		hduVector3Dd v = gGrabObj.velocity;
		hduVector3Dd xTarget = gGrabObj.targetPosition;

		hduVector3Dd force = gGrabObj.springK * (xTarget - x) - gGrabObj.dampingC * v;

		hduVector3Dd accel = force / gGrabObj.mass;
		gGrabObj.velocity += accel * dt;
		gGrabObj.position += gGrabObj.velocity * dt;

		//회전 커플링
		hduVector3Dd r = gGrabObj.rotationEuler;
		hduVector3Dd w = gGrabObj.angularVelocity;
		hduVector3Dd rTarget = gGrabObj.targetRotationEuler;

		hduVector3Dd torque = gGrabObj.springK * (rTarget - r) - gGrabObj.dampingC * w;

		hduVector3Dd angAccel = torque / gGrabObj.mass;
		gGrabObj.angularVelocity += angAccel * dt;
		gGrabObj.rotationEuler += gGrabObj.angularVelocity * dt;
	}

	//회전된 잡은 점이 게속 proxy 위치에 오도록 중심 재계산
	hduVector3Dd proxyPos = getProxyPosition();
	hduVector3Dd rotatedLocalGrabPoint = rotateVEctorEulerXYZ(gGrabObj.grabLocalPoint, gGrabObj.rotationEuler);

	gGrabObj.position = proxyPos - rotatedLocalGrabPoint;
}

static void drawSharedObjectGeometry()
{
	if (hasObjModel())
		drawObjModel();
	else
		glutSolidSphere(0.5, 32, 32);
}

static void applyObjectTransform()
{
	glTranslated(gGrabObj.position[0], gGrabObj.position[1], gGrabObj.position[2]);

	glRotated(gGrabObj.rotationEuler[0], 1.0, 0.0, 0.0);
	glRotated(gGrabObj.rotationEuler[1], 0.0, 1.0, 0.0);
	glRotated(gGrabObj.rotationEuler[2], 0.0, 0.0, 1.0);
}

/* 인터랙션 콜백 */
void HLCALLBACK buttonDownCB(HLenum event, HLuint object, HLenum thread, HLcache* cache, void* userdata)
{
	hduVector3Dd proxyPos = getProxyPosition();
	beginGrab(proxyPos);
}

void HLCALLBACK buttonUpCB(HLenum event, HLuint object, HLenum thread, HLcache* cache, void* userdata)
{
	endGrab();
}

void HLCALLBACK touchCB(HLenum event, HLuint object, HLenum thread, HLcache* cache, void* userdata)
{
	gGrabObj.isTouched = true;
}

void HLCALLBACK untouchCB(HLenum event, HLuint object, HLenum thread, HLcache* cache, void* userdata)
{
	gGrabObj.isTouched = false;
}

/* 씬 초기화 */
void initScene()
{
	initGL();
	initHL();

	if (!loadObjModel("Suzanne.obj"))
	{
		printf_s("Failed to load OBJ: Suzanne.obj\n");
	}
}

/* OpenGL 렌더링 셋업 */
void initGL()
{
	static const GLfloat light_model_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	static const GLfloat light0_diffuse[] = { 0.9f, 0.9f, 0.9f, 0.9f };
	static const GLfloat light0_direction[] = { 0.0f, -0.4f, 1.0f, 0.0f };

	//depth buffer 활성화. 은면 제거용
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	// Cull back faces.
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	// 그 외 나머지 세팅
	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);
	glShadeModel(GL_SMOOTH);

	// 조명 모델 세팅
	//전역 조명 규칙 설정(glLightModel*) → 개별 광원 속성 설정(glLightfv) → 광원 켜기(glEnable(GL_LIGHT0))
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light_model_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, light0_direction);
	glEnable(GL_LIGHT0);
}

/* OpenGL의 뷰 변환을 햅틱 디바이스의 workspace에 매핑 업데이트 */
void updateWorkspace()
{
	GLdouble modelview[16];
	GLdouble projection[16];
	GLint viewport[4];

	//glGet...(a, b); a의 값을 가져와서 b에 저장할거임.
	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);
	glGetIntegerv(GL_VIEWPORT, viewport);

	hlMatrixMode(HL_TOUCHWORKSPACE);
	hlLoadIdentity();

	//워크스페이스를 뷰 볼륨에 맞추기
	hluFitWorkspace(projection);

	//커서 스케일 계산
	gCursorScale = hluScreenToModelScale(modelview, projection, viewport);
	gCursorScale *= CURSOR_SIZE_PIXELS;
}

/* HDAPI 초기화. 디바이스 초기화, 힘, 장치 스레드 스케줄링 활성화 등이 포함됨 */
void initHL()
{
	HDErrorInfo error;

	ghHD = hdInitDevice(HD_DEFAULT_DEVICE);
	if (HD_DEVICE_ERROR(error = hdGetError()))
	{
		hduPrintError(stderr, &error, "Failed to initialize haptic device");
		fprintf(stderr, "Press any key to exit");
		getchar();
		exit(-1);
	}

	ghHLRC = hlCreateContext(ghHD);
	hlMakeCurrent(ghHLRC);

	//OpenHaptics 지오메트리 렌더링 시 뷰 파라미터 최적화 활성화
	hlEnable(HL_HAPTIC_CAMERA_VIEW);

	// 기하에 대한 id 생성
	gShapeId = hlGenShapes(1);

	hlTouchableFace(HL_FRONT); //이거 잘 하면 슬라임 같은 느낌 낼 수도 있을 듯? HL_BACK으로 하면?

	//콜백 호출
	hlAddEventCallback(HL_EVENT_1BUTTONDOWN, gShapeId, HL_CLIENT_THREAD, buttonDownCB, NULL);
	hlAddEventCallback(HL_EVENT_1BUTTONUP, HL_OBJECT_ANY, HL_CLIENT_THREAD, buttonUpCB, NULL);
	hlAddEventCallback(HL_EVENT_TOUCH, gShapeId, HL_CLIENT_THREAD, touchCB, NULL);
	hlAddEventCallback(HL_EVENT_UNTOUCH, gShapeId, HL_CLIENT_THREAD, untouchCB, NULL);
}

/*햅틱 렌더링 매인 루프 */
void drawSceneHaptics()
{
	//햅틱 프레임 시작(필수)
	hlBeginFrame();

	// 재질 속성 세팅
	hlMaterialf(HL_FRONT_AND_BACK, HL_STIFFNESS, gStiffness);
	hlMaterialf(HL_FRONT_AND_BACK, HL_DAMPING, gDamping);
	hlMaterialf(HL_FRONT_AND_BACK, HL_STATIC_FRICTION, gStaticFriction);
	hlMaterialf(HL_FRONT_AND_BACK, HL_DYNAMIC_FRICTION, gDynamicFriction);

	// OpenGL 기하를 가져와서 햅틱 렌더링에 쓸 수 있도록 함.
	hlBeginShape(HL_SHAPE_FEEDBACK_BUFFER, gShapeId);

	glPushMatrix();

	applyObjectTransform();

	drawSharedObjectGeometry();

	glPopMatrix();

	// End the shape
	hlEndShape();

	// End the haptic frame
	hlEndFrame();
}

/* 로컬 변환, 작업 공간 -> 월드 변환, 화면 좌표 스케일을 이용하여 햅틱 디바이스용 3d커서 그리기*/
void drawCursor()
{
	static const double kCursorRadius = 0.5;
	static const double kCursorHeight = 1.5;
	static const int kCursorTess = 15;
	HLdouble proxyxform[16];

	// GLUquadricObj 타입의 객체를 가리키는 포인터 생성
	// C/C++에서는 포인터에 0을 넣으면 null로 취급
	// 요즘 스타일은 아님. 요즘엔 0 대신 nullptr 많이 사용함.
	GLUquadricObj* qobj = 0;

	//glPushAttrib: 현재 OpenGL 상태(속성: Attrib) 일부를 스택에 저장(Push) 
	//GL_CURRENT_BIT: 색, 노멀, 텍스쳐 좌표
	//GL_ENABLE_BIT: 각종 활성화 상태
	//GL_LIGHTING_BIT: 조명 관련 상태
	//이 세 가지를 한 번에 저장하라는 뜻
	glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT);
	//glPushMatrix도 마찬가지
	glPushMatrix();

	if (!gCursorDisplayList)
	{
		//gCursorDisplayList -> glList 식별자라고 봐도 무방
		//glGenLists(1) -> display list용 번호 한 개 발급
		//하지만 glGenList가 반환하는 값은 1이 아닐 수도 있음. 한 개를 발급한다는 거지 1이라는 값을 반환한다는 건 아님. 실제 리스트 번호는 4가 될 수도, 57이 될 수도 있음.
		//반환값은 숫자 하나지만, 내부적으로는 파라미터만큼 예약을 해 둠.
		//e.g. glGenLists(3)이라고 한다면 내부적으로는 45, 46, 47이라는 번호를 가진 리스트를 생성하고 gCursorDisplayList에는 가장 빠른 숫자 45만 반환하는 거임.
		//실제로 쓸 때는 다음과 같이 쓸 수 있는거임
		/*
		glNewList(gCursorDisplayList, GL_COMPILE);
		...
			glEndList();

		glNewList(gCursorDisplayList + 1, GL_COMPILE);
		...
			glEndList();

		glNewList(gCursorDisplayList + 2, GL_COMPILE);
		...
		*/
		//관련된 세트의 도형을 한 번에 처리할 때 용이.
		//어색하고 불편해 보이지만 옛날엔 이렇게 썼다.
		gCursorDisplayList = glGenLists(1);
		//번호에 해당하는 리스트에 내용 기록 시작
		glNewList(gCursorDisplayList, GL_COMPILE);

		//gluNewQuadric();은 객체의 주소를 반환하는 함수
		//qobj에는 gluNewQuadric 객체가 있는 곳을 가리키게 됨
		qobj = gluNewQuadric();

		//원래는 원기둥인데 바닥 반지름이 0.0이므로 사실상 원뿔임.
		//Tess는 테셀레이션. 이건 뭔지 알지?
		gluCylinder(qobj, 0.0, kCursorRadius, kCursorHeight, kCursorTess, kCursorTess);
		glTranslated(0.0, 0.0, kCursorHeight);
		gluCylinder(qobj, kCursorRadius, 0.0, kCursorHeight / 5.0, kCursorTess, kCursorTess);

		//포인터 반환
		gluDeleteQuadric(qobj);
		//내용 기록 끝.
		glEndList();
	}

	// 월드 좌표 내에서 프록시 변환 얻기
	hlGetDoublev(HL_PROXY_TRANSFORM, proxyxform);
	// 현재 형렬에 proxy변환행렬 곱한다는 뜻
	glMultMatrixd(proxyxform);

	// 로컬 커서 스케일 팩터 적용
	// 현재 행렬에 스케일 변환 추가
	glScaled(gCursorScale, gCursorScale, gCursorScale);
	// 그러면 지금 행렬은 기존행렬 * 프록시 * 스케일 상태인 거임


	//상태를 건드리는 코드
	//enable 상태 바꿈
	glEnable(GL_COLOR_MATERIAL);
	//현재 색 상태 바꿈
	glColor3f(0.0, 0.5, 1.0);

	//드로우콜: 리스트에 있는 내용을 드로우
	//그러면 현재 상태가 리스트 기하에 적용됨.
	glCallList(gCursorDisplayList);


	//다시 기존행렬 상태로 돌아가기 위해 앞서 push해둔 행렬을 꺼냄.
	//Matrix와 Attrib의 스택은 독립적이지만, 관습 상 역순으로 pop 함.
	glPopMatrix();
	glPopAttrib();
}

/*그래픽 렌더링 매인 루프. 햅틱 스레드 최신값을 가져와 3d커서 그림.*/
void drawSceneGraphics()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glColor3f(0.8f, 0.8f, 0.8f);

	applyObjectTransform();
	drawSharedObjectGeometry();

	glPopMatrix();

	//햅틱 디바이스 포지션으로부터 3d 커서 렌더링
	drawCursor();

	//속성 텍스트 그리기
	drawMaterialUI();
}

/* 앱 종료 시 호출되는 핸들러. 메모리 해제 */
void exitHandler()
{
	// 쉐이프 id 할당 해제
	hlDeleteShapes(gShapeId, 1); // ram 블록 해제

	// 햅틱 렌더링 컨텍스트 할당 해제
	hlMakeCurrent(NULL);
	if (ghHLRC != NULL)
	{
		hlDeleteContext(ghHLRC);
	}

	//햅틱 디바이스 할당 해제
	if (ghHD != HD_INVALID_HANDLE)
	{
		hdDisableDevice(ghHD);
	}
}

/* 인터랙션 업데이트 함수 */
void updateInteraction()
{
	static int lastTime = glutGet(GLUT_ELAPSED_TIME);
	int currentTime = glutGet(GLUT_ELAPSED_TIME);
	double dt = (currentTime - lastTime) / 1000.0;
	lastTime = currentTime;

	if (dt <= 0.0)
		dt = 0.001;

	hduVector3Dd proxyPos = getProxyPosition();

	if (gGrabObj.isGrabbed)
	{
		updateGrabTarget(proxyPos);
		updateGrabRotationTarget(proxyPos);
	}

	updateObjectMotion(dt);

	hlCheckEvents();
}