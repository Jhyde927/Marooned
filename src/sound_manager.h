#pragma once

#include "raylib.h"
#include <map>
#include <string>
#include <vector>

class SoundManager {
public:
    static SoundManager& GetInstance(); // Singleton
    void PlaySoundAtPosition(const std::string& soundName, const Vector3& soundPos, const Vector3& listenerPos, float listenerYaw, float maxDistance);
    void LoadMusic(const std::string& name, const std::string& filePath);
    void PlayMusic(const std::string& name, float volume = 1.0f);
    void Update(float dt); // Call this every frame
    Music& GetMusic(const std::string& name);
    void LoadSound(const std::string& name, const std::string& filePath);
    Sound GetSound(const std::string& name);
    void Play(const std::string& name);
    void Stop(const std::string& name);
    bool IsPlaying(const std::string& name) const;
    void LoadSounds();
    void UnloadAll();

    // --- NEW: NPC speech chaining ---
    void RegisterSpeechBank(const std::string& bankName, const std::vector<std::string>& keys);
    void StartSpeech(int npcId, const std::string& bankName, float durationSec, bool extend = true);
    void StopSpeech(int npcId);
    bool IsSpeaking(int npcId) const;

private:
    std::map<std::string, Sound> sounds;
    std::map<std::string, Music> musicTracks;

    // bankName -> sound keys
    std::map<std::string, std::vector<std::string>> speechBanks;

    struct SpeechState {
        bool active = false;
        std::string bankName;
        float timeLeft = 0.0f;
        float pauseTimer = 0.0f;
        std::string currentKey; // last played clip key
        int lastIndex = -1;
    };

    // npcId -> speech state
    std::map<int, SpeechState> npcSpeech;

    void UpdateSpeech(float dt);
    int  PickRandomIndexNoRepeat(int count, int lastIndex);

    SoundManager() = default;
    ~SoundManager();
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;
};
