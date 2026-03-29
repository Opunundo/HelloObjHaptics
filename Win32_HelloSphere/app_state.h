#pragma once

#include <HD/hd.h>
#include <HDU/hduVector.h>
#include <HL/hl.h>
#include <GL/glut.h>


struct UIButton
{
	float x, y, w, h;
	const char* label;
	float* targetValue;
	float delta;
};

extern HHD ghHD;
extern HHLRC ghHLRC;
extern HLuint gShapeId;

#define CURSOR_SIZE_PIXELS 40

extern double gCursorScale;
extern GLuint gCursorDisplayList;

extern float gStiffness;
extern float gDamping;
extern float gStaticFriction;
extern float gDynamicFriction;

extern int gWindowWidth;
extern int gWindowHeight;

extern UIButton gButtons[];
extern const int gButtonCount;


// 그랩 인터렉션 전용 상태변수 //

struct GrabObjectState
{
	bool isGrabbed = false;
	bool isTouched = false;

	hduVector3Dd position{ 0.0, 0.0, 0.0 }; //실제 렌더링/햅틱에 쓰는 위치
	hduVector3Dd velocity{ 0.0, 0.0, 0.0 }; // 추후 coupling 전환 시 사용
	hduVector3Dd targetPosition{ 0.0, 0.0, 0.0 }; // 프록시가 원하는 목표 위치
	hduVector3Dd grabOffset{ 0.0, 0.0, 0.0 }; // 잡을 때 proxy와 물체 사이 오프셋

	double mass = 1.0;
	double springK = 18.0; // coupling용
	double dampingC = 7.0; // coupling용

	//회전 상태
	hduVector3Dd rotationEuler{ 0.0, 0.0, 0.0 };
	hduVector3Dd targetRotationEuler{ 0.0, 0.0, 0.0 };
	hduVector3Dd angularVelocity{ 0.0, 0.0, 0.0 };

	//잡은 점 상태
	hduVector3Dd grabLocalPoint{ 0.0, 0.0, 0.0 };
	hduVector3Dd lastProxyPos{ 0.0, 0.0, 0.0 };

	double rotationSensitivity = 120.0;
};

extern GrabObjectState gGrabObj;
extern bool gUseVirtualCoupling;