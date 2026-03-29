#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(_WIN32) || defined(linux)
#include <GL/glut.h>
#elif defined(__APPLE__)
#include <GLUT/glut.h>
#endif

#include "app_state.h"
#include "ui.h"

float clamp01(float v)
{
	if (v < 0.0f) return 0.0f;
	if (v > 1.0f) return 1.0f;
	return v;
}

bool isInsideButton(const UIButton& b, float mx, float my)
{
	return (mx >= b.x && mx <= b.x + b.w && my >= b.y && my <= b.y + b.h);
}

void drawText(float x, float y, const char* text)
{
	//x, y 래스터 위치 지정(픽셀 기반 그리기용)
	glRasterPos2f(x, y);

	while (*text)
	{
		//글자 하나를 그리는 명령
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *text);
		++text;
	}
}

void drawScaledText(float x, float y, float scale, const char* text)
{
	glPushMatrix();

	glTranslatef(x, y, 0.0f);
	glScalef(scale, scale, scale);

	while (*text)
	{
		glutStrokeCharacter(GLUT_STROKE_ROMAN, *text);
		++text;
	}

	glPopMatrix();
}

void drawButton(const UIButton& b)
{
	glColor3f(0.3f, 0.3f, 0.3f);
	glBegin(GL_QUADS);
	glVertex2f(b.x, b.y);
	glVertex2f(b.x + b.w, b.y);
	glVertex2f(b.x + b.w, b.y + b.h);
	glVertex2f(b.x, b.y + b.h);
	glEnd();

	glColor3f(1.0f, 1.0f, 1.0f);
	drawText(b.x + 0.021f, b.y + 0.015f, b.label);
}

void drawMaterialUI()
{
	char buf[128];

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0, 1.0, 0.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

	glColor3f(1.0f, 1.0f, 1.0f);

	//sprintf_s: 문자열을 만드는 함수. buf에 gStiffness 값을 문자열 형식에 맞게 저장
	sprintf_s(buf, "Stiffness: %.2f", gStiffness);
	drawText(0.05f, 0.95f, buf);

	sprintf_s(buf, "Damping: %.2f", gDamping);
	drawText(0.05f, 0.90f, buf);

	sprintf_s(buf, "Static Friction: %.2f", gStaticFriction);
	drawText(0.05f, 0.85f, buf);

	sprintf_s(buf, "Dynamic Friction: %.2f", gDynamicFriction);
	drawText(0.05f, 0.80f, buf);

	for (int i = 0; i < gButtonCount; ++i)
	{
		drawButton(gButtons[i]);
	}

	glPopAttrib();

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

void glutKeyboard(unsigned char key, int x, int y)
{
	const float step = 0.05f;

	switch (key)
	{
	case '1': gStiffness = clamp01(gStiffness - step); break;
	case '2': gStiffness = clamp01(gStiffness + step); break;

	case 'q': gDamping = clamp01(gDamping - step); break;
	case 'w': gDamping = clamp01(gDamping + step); break;

	case 'a': gStaticFriction = clamp01(gStaticFriction - step); break;
	case 's': gStaticFriction = clamp01(gStaticFriction + step); break;

	case 'z': gDynamicFriction = clamp01(gDynamicFriction - step); break;
	case 'x': gDynamicFriction = clamp01(gDynamicFriction + step); break;
	}
}

void glutMouse(int button, int state, int x, int y)
{
	if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN)
		return;

	float mx = (float)x / (float)gWindowWidth;
	float my = 1.0f - ((float)y / (float)gWindowHeight);

	for (int i = 0; i < gButtonCount; ++i)
	{
		if (isInsideButton(gButtons[i], mx, my))
		{
			*(gButtons[i].targetValue) = clamp01(*(gButtons[i].targetValue) + gButtons[i].delta);

			glutPostRedisplay();
			return;
		}
	}
}