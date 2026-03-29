// PracticeOpenHaptics.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(_WIN32) || defined(linux)
#include <GL/glut.h>
#elif defined(__APPLE__)
#include <GLUT/glut.h>
#endif

#include <HL/hl.h>
#include <HDU/hduError.h>

#include "app_state.h"
#include "scene.h"
#include "ui.h"

/* Funtion protoypes */
void glutDisplay(void);
void glutReshape(int width, int height);
void glutIdle(void);
void glutMenu(int value);

/* Initialize GLUT for displaying a simple haptic scene */
int main(int argc, char* argv[])
{
	glutInit(&argc, argv);

	// GLUT_BOUBLE: 더블 버퍼링을 쓰겠다.
	// 더블 버퍼링이란, 하나는 보여주고 있는 화면, 다른 하나는 새로 그리는 화면
	// glutSwapBuffers();를 사용하면 버퍼가 바뀌면서 업데이트 된 화면을 보여줌.
	// 이걸 쓰는 애유는 깜빡임, 지우고 다시 그리는 과정 노출 등을 줄이기 위함.
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

	glutInitWindowSize(gWindowWidth, gWindowHeight);
	glutCreateWindow("HelloSphere Example");

	//Set glut callback funtions.
	glutDisplayFunc(glutDisplay);
	glutReshapeFunc(glutReshape);
	glutIdleFunc(glutIdle);
	glutKeyboardFunc(glutKeyboard);
	glutMouseFunc(glutMouse);

	glutCreateMenu(glutMenu);
	glutAddMenuEntry("Quit", 0);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	//Provide a cleanup routine for handiling application exit.
	atexit(exitHandler);

	initScene();

	glutMainLoop();

	return 0;
}

/* GLUT redraw 콜백 */
void glutDisplay()
{
	updateInteraction();

	drawSceneHaptics();
	drawSceneGraphics();

	glutSwapBuffers();
}

/* GLUT callback for reshaping the window. This is the main place where the viewing and workspace transforms get initialized. */
void glutReshape(int width, int height)
{
	gWindowWidth = width;
	gWindowHeight = height;

	//k 를 앞에 붙이는 건 상수로 쓸 거라는 뜻. 그냥 관습적 네이밍 규칙임.
	static const double kPI = 3.1415926535897932384626433832795;
	static const double kFovY = 40;

	double nearDist, farDist, aspect;

	glViewport(0, 0, width, height);

	//Compute the viewing parameters based on a fixed fov and viewing a canonical nox centered at the origin.

	nearDist = 1.0 / tan((kFovY / 2.0) * kPI / 180.0); //컴퓨터그래픽스에서 nearplane 거리 구했던 거 기억나지? 1/tan(theta/2) 했던거? 여기는 라디안 값만 받아서 뒤에 kPI/180.0 곱해주는 거.
	farDist = nearDist + 2.0;
	aspect = (double)width / height;

	//클립공간 설정
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(kFovY, aspect, nearDist, farDist);

	//Place the camera down the Z axis looking at the origin.
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, 0, nearDist + 1.0,
		0, 0, 0,
		0, 1, 0);

	updateWorkspace();

}

/* idle 상태가 되었을 때 glut가 호출하는 콜백. 이 상태일 때 redraw요청과 HLAPI에러도 점검*/
void glutIdle()
{
	HLerror error;

	while (HL_ERROR(error = hlGetError()))
	{
		fprintf(stderr, "HLError: %s\n", error.errorCode);

		if (error.errorCode == HL_DEVICE_ERROR)
		{
			hduPrintError(stderr, &error.errorInfo, "Error during haptic rendering\n");
		}
	}
	glutPostRedisplay();
}

/* 팝업메뉴 핸들러 */
void glutMenu(int ID)
{
	switch (ID) {
	case 0:
		exit(0);
		break;
	}
}

