#pragma once

#include <HD/hd.h>
#include <HDU/hduVector.h>
#include <HDU/hduMatrix.h>

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

	//현재 오브젝트 변환
	hduMatrix objectTransform;

	//grab 시작 순간 저장
	hduMatrix objectStartTransform;
	hduMatrix proxyStartTransfrom;

	hduVector3Dd proxyStartPosition;
};

extern GrabObjectState gGrabObj;
extern bool gUseVirtualCoupling;