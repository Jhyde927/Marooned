#pragma once

#include "raylib.h"
#include "raymath.h"
#include <string>
#include "player.h"

class Kraken
{
public:
    enum class State
    {
        Hidden,     // Fully submerged / inactive
        Rising,     // Moving up toward exposed height
        Exposed,    // Visible, bobbing, targetable
        Sinking     // Going back down
    };

    Kraken() = default;

    // Load the model and set initial transform/state
    void Init(Vector3 spawnPosition,
              float waterY,
              float scale = 1.0f);

    void Unload();

    void Update(float dt, Player& player);
    void Draw() const;

    // Movement/state control
    void Rise();
    void Sink();
    void HideImmediately();
    void ShowImmediately();

    // Teleport while submerged to a new side of the ship
    void TeleportTo(Vector3 newBasePosition);

    // Convenience: sink, teleport, then rise somewhere else
    void Reposition(Vector3 newBasePosition);

    // Basic transform controls
    void SetBasePosition(Vector3 newBasePosition);
    Vector3 GetBasePosition() const;

    Vector3 GetDrawPosition() const;
    float GetYawDegrees() const;

    void SetYawDegrees(float yawDeg);
    void AddYawDegrees(float deltaYawDeg);

    void SetScale(float newScale);
    float GetScale() const;

    void SetWaterY(float newWaterY);
    float GetWaterY() const;

    void SetBobEnabled(bool enabled);
    bool IsBobEnabled() const;

    void SetVisible(bool visible);
    bool IsVisible() const;

    State GetState() const;

private:
    void UpdateState(float dt, Player& player);
    void UpdateIdleMotion(float dt, Player& player);
    void UpdateTransform();

private:
    Model model{};
    bool modelLoaded{false};

    State state{State::Hidden};
    bool visible{true};
    bool bobEnabled{true};

    // Logical anchor position in the world
    Vector3 basePosition{0.0f, 0.0f, 0.0f};

    // Final position used for drawing after bob/offset motion
    Vector3 drawPosition{0.0f, 0.0f, 0.0f};

    float waterY{0.0f};
    float scale{1.0f};

    // Rotation
    float baseYawDeg{0.0f};      // Main facing rotation
    float currentYawDeg{0.0f};   // Final yaw after idle rocking
    float idleTiltDeg{0.0f};     // Optional subtle extra motion

    // Vertical movement
    float currentHeightOffset{-350.0f}; // Starts submerged
    float targetHeightOffset{-350.0f};

    float hiddenOffset{-350.0f};   // Below water
    float exposedOffset{0.0f};   // Head poking out of water
    float riseSpeed{220.0f};       // Units per second
    float sinkSpeed{260.0f};

    // Bobbing / idle animation
    float bobTime{0.0f};
    float bobAmplitude{20.0f};
    float bobSpeed{1.6f};

    float rockTime{0.0f};
    float rockAmplitudeDeg{10.0f};
    float rockSpeed{0.9f};

    // Random little wobble timer
    float characterTime{0.0f};
};