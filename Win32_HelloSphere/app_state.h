#pragma once

#include <HD/hd.h>
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
extern HLuint gSphereShapeId;

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