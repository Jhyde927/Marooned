// render_pipeline.h
#pragma once
#include "raylib.h"
#include "player.h"
void RenderFrame(Camera3D& camera,Player& player, float deltaTime);
void RenderMenuFrame(Camera3D& camera, Player& player, float dt);