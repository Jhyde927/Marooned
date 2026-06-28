// world_update.h
#pragma once
#include "raylib.h"
#include "player.h"

void UpdatePlayingFrame(Camera3D& camera, Player& player, float dt, float elapsedTime);
void UpdateMenuState(Camera3D& camera, float deltaTime, float elapsedTime);
bool HandleFadeLevelSwap(Camera3D& camera);
void UpdateStateTransitions();