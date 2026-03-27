#pragma once

#include "app_state.h"

void drawText(float x, float y, const char* text);
void drawScaledText(float x, float y, float scale, const char* text);
void drawMaterialUI();

void drawButton(const UIButton& b);
bool isInsideButton(const UIButton& b, float mx, float my);
float clamp01(float v);

void glutKeyboard(unsigned char key, int x, int y);
void glutMouse(int button, int state, int x, int y);