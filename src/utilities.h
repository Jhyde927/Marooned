// utilities.h
#pragma once

#include "raylib.h"
#include "raymath.h"
#include <string>


bool InBounds(int x, int y, int w, int h);
float RandomFloat(float min, float max);
float Clamp01(float x);
float LerpExp(float current, float target, float lambda, float dt); 
// Vectors
Vector2 LerpV2(Vector2 a, Vector2 b, float t);
Vector3 LerpVec3(Vector3 a, Vector3 b, float t);
void DebugPrintVector(const Vector3& v);
Vector3 ColorToV3(Color c);

//color
Color V3ToColor(Vector3 v, float a=1.0f);
Color ColorLerpFast(Color a, Color b, float t);
Color LerpColor(Color a, Color b, float t);
Vector3 Limit(Vector3 v, float maxLen);

Vector3 AddLightHeadroom(Vector3 base, Vector3 lightColor, float intensity); //stops oversaturating model.tint

Vector3 NormalizeXZ(Vector3 v);
Vector3 RandomPointOnRingXZ(const Vector3& center, float minR, float maxR);
float DistXZ(const Vector3& a, const Vector3& b);
inline float Rand01() { return (float)GetRandomValue(0, 1000) / 1000.0f; }
Vector3 DirFromYawDeg(float yawDeg);

//launchers
bool IsDirPixel(Color c);
bool IsTimingPixel(Color c);
float TimingFromPixel(Color c);

Rectangle FitTextureDest(const Texture2D& tex, int screenW, int screenH, bool cover);