#include "sound_manager.h"
#include <iostream>
#include "raymath.h"

int SoundManager::PickRandomIndexNoRepeat(int count, int lastIndex)
{
    if (count <= 1) return 0;
    int idx = GetRandomValue(0, count - 1);
    if (idx == lastIndex) {
        idx = (idx + 1) % count; // cheap no-repeat
    }
    return idx;
}

void SoundManager::RegisterSpeechBank(const std::string& bankName, const std::vector<std::string>& keys)
{
    speechBanks[bankName] = keys;
}

void SoundManager::StartSpeech(int npcId, const std::string& bankName, float durationSec, bool extend)
{
    auto bankIt = speechBanks.find(bankName);
    if (bankIt == speechBanks.end() || bankIt->second.empty()) {
        std::cerr << "Speech bank missing/empty: " << bankName << "\n";
        return;
    }

    SpeechState& st = npcSpeech[npcId];
    st.active = true;
    st.bankName = bankName;

    if (extend) st.timeLeft += durationSec;
    else        st.timeLeft  = durationSec;

    // optional: clamp so repeated clicks don't stack forever
    if (st.timeLeft > 6.0f) st.timeLeft = 6.0f;

    // If nothing is currently playing, kick it immediately (no pause)
    if (st.currentKey.empty() || !IsSoundPlaying(sounds[st.currentKey])) {
        st.pauseTimer = 0.0f;
    }
}

void SoundManager::StopSpeech(int npcId)
{
    auto it = npcSpeech.find(npcId);
    if (it == npcSpeech.end()) return;

    // Optional: stop the currently playing clip (not required)
    if (!it->second.currentKey.empty()) {
        auto sIt = sounds.find(it->second.currentKey);
        if (sIt != sounds.end()) StopSound(sIt->second);
    }

    npcSpeech.erase(it);
}

bool SoundManager::IsSpeaking(int npcId) const
{
    auto it = npcSpeech.find(npcId);
    if (it == npcSpeech.end()) return false;
    return it->second.active;
}

void SoundManager::UpdateSpeech(float dt)
{
    for (auto it = npcSpeech.begin(); it != npcSpeech.end(); )
    {
        SpeechState& st = it->second;

        if (!st.active) { it = npcSpeech.erase(it); continue; }

        // bank lookup
        auto bankIt = speechBanks.find(st.bankName);
        if (bankIt == speechBanks.end() || bankIt->second.empty()) {
            it = npcSpeech.erase(it);
            continue;
        }
        const auto& keys = bankIt->second;

        // If current clip still playing, wait
        if (!st.currentKey.empty()) {
            auto sIt = sounds.find(st.currentKey);
            if (sIt != sounds.end() && IsSoundPlaying(sIt->second)) {
                ++it;
                continue;
            }
        }

        // Pause between clips
        if (st.pauseTimer > 0.0f) {
            st.pauseTimer -= dt;
            ++it;
            continue;
        }

        // Done?
        if (st.timeLeft <= 0.0f) {
            it = npcSpeech.erase(it);
            continue;
        }

        // Pick + play next clip
        int idx = PickRandomIndexNoRepeat((int)keys.size(), st.lastIndex);
        st.lastIndex = idx;
        st.currentKey = keys[idx];

        auto sIt = sounds.find(st.currentKey);
        if (sIt != sounds.end()) {
            // fixed volume; adjust if you want
            SetSoundVolume(sIt->second, 1.0f);
            PlaySound(sIt->second);
        }

        // Estimate: your clips are ~1–2 seconds; we just budget ~1.0f each,
        // and the IsSoundPlaying() check prevents overlap anyway.
        st.timeLeft -= 1.0f;

        // Small natural gap between syllables
        st.pauseTimer = (float)GetRandomValue(6, 18) / 100.0f; // 0.06..0.18

        ++it;
    }
}

std::vector<std::string> footstepKeys;
SoundManager& SoundManager::GetInstance() {
    static SoundManager instance;
    return instance;
}

void SoundManager::LoadSound(const std::string& name, const std::string& filePath) {
    Sound sound = ::LoadSound(filePath.c_str());
    sounds[name] = sound;
}

void SoundManager::LoadMusic(const std::string& name, const std::string& filePath) {
    Music music = LoadMusicStream(filePath.c_str());
    musicTracks[name] = music;
}

Sound SoundManager::GetSound(const std::string& name) {
    if (sounds.count(name)) {
        return sounds[name];
    }
    std::cerr << "Sound not found: " << name << std::endl;
    return {0}; // Empty sound
}

bool SoundManager::IsPlaying(const std::string& name) const {
    auto it = sounds.find(name);
    if (it == sounds.end()) return false;
    return IsSoundPlaying(it->second);
}

void SoundManager::Play(const std::string& name) {
    auto it = sounds.find(name);
    if (it == sounds.end()) return;

    if (!IsSoundPlaying(it->second)) {
        PlaySound(it->second);
    }
}

void SoundManager::Stop(const std::string& name) {
    if (sounds.count(name)) {
        StopSound(sounds[name]);
    }
}



void SoundManager::PlayMusic(const std::string& name, float volume) {
    if (musicTracks.find(name) == musicTracks.end()) {
        std::cerr << "Music not found: " << name << std::endl;
        return;
    }

    Music& music = musicTracks[name];
    StopMusicStream(music); // Ensure only one plays at a time
    SetMusicVolume(music, volume);
    PlayMusicStream(music);
}

void SoundManager::Update(float dt) {
    // for (auto& [name, music] : musicTracks) {
    //     UpdateMusicStream(music);
    // }

    UpdateSpeech(dt);


}

Music& SoundManager::GetMusic(const std::string& name) {
    return musicTracks[name]; 
}



void SoundManager::PlaySoundAtPosition(const std::string& soundName, const Vector3& soundPos, const Vector3& listenerPos, float listenerYaw, float maxDistance) {
    if (sounds.find(soundName) == sounds.end()) {
        std::cerr << "Sound not found: " << soundName << std::endl;
        return;
    }

    Vector3 delta = Vector3Subtract(soundPos, listenerPos);
    float distance = Vector3Length(delta);
    if (distance > maxDistance) return;

    float volume = Clamp(1.0f - (distance / maxDistance), 0.0f, 1.0f);


    Sound sound = sounds[soundName];
    // StopSound(sound);
    SetSoundVolume(sound, volume);
    // SetSoundPan(sound, finalPan);
    PlaySound(sound);


}



SoundManager::~SoundManager() {
    UnloadAll();
}

void SoundManager::UnloadAll() {
    for (auto& [name, sound] : sounds) {
        ::UnloadSound(sound);
    }
    sounds.clear();
    for (auto& [name, music] : musicTracks){
        ::UnloadMusicStream(music);
    }
    musicTracks.clear();

}

void SoundManager::LoadSounds() {
    //Sounds
    SoundManager::GetInstance().LoadSound("dinoHit", "assets/sounds/dinoHit.ogg");
    SoundManager::GetInstance().LoadSound("dinoDeath", "assets/sounds/dinoDeath.ogg");
    SoundManager::GetInstance().LoadSound("dinoTweet", "assets/sounds/dino1.ogg");
    SoundManager::GetInstance().LoadSound("dinoTarget", "assets/sounds/dino2.ogg");
    SoundManager::GetInstance().LoadSound("dinoTweet2", "assets/sounds/dino3.ogg");
    SoundManager::GetInstance().LoadSound("dinoBite", "assets/sounds/bite.ogg");
    SoundManager::GetInstance().LoadSound("reload", "assets/sounds/reload.ogg");
    SoundManager::GetInstance().LoadSound("shotgun", "assets/sounds/shotgun.ogg");

    SoundManager::GetInstance().LoadSound("step1", "assets/sounds/step1.ogg");
    SoundManager::GetInstance().LoadSound("step2", "assets/sounds/step2.ogg");
    SoundManager::GetInstance().LoadSound("step3", "assets/sounds/step3.ogg");
    SoundManager::GetInstance().LoadSound("step4", "assets/sounds/step4.ogg");

    SoundManager::GetInstance().LoadSound("phit1", "assets/sounds/PlayerHit1.ogg");
    SoundManager::GetInstance().LoadSound("phit2", "assets/sounds/PlayerHit2.ogg");

    SoundManager::GetInstance().LoadSound("doorOpen", "assets/sounds/doorOpen.ogg");
    SoundManager::GetInstance().LoadSound("doorClose", "assets/sounds/doorCLose.ogg");
    SoundManager::GetInstance().LoadSound("swipe1", "assets/sounds/swipe1.ogg");
    SoundManager::GetInstance().LoadSound("swipe2", "assets/sounds/swipe2.ogg");
    SoundManager::GetInstance().LoadSound("swipe3", "assets/sounds/swipe3.ogg");
    SoundManager::GetInstance().LoadSound("swordHit", "assets/sounds/swordHit.ogg");
    SoundManager::GetInstance().LoadSound("swordBlock", "assets/sounds/swordBlock.ogg");
    SoundManager::GetInstance().LoadSound("swordBlock2", "assets/sounds/swordBlock2.ogg");
    SoundManager::GetInstance().LoadSound("slice", "assets/sounds/slice.ogg");
    SoundManager::GetInstance().LoadSound("bones", "assets/sounds/bones.ogg");
    SoundManager::GetInstance().LoadSound("bones2", "assets/sounds/bones2.ogg");
    SoundManager::GetInstance().LoadSound("gulp", "assets/sounds/gulp.ogg");
    SoundManager::GetInstance().LoadSound("clink", "assets/sounds/clink.ogg");
    SoundManager::GetInstance().LoadSound("lockedDoor", "assets/sounds/lockedDoor.ogg");
    SoundManager::GetInstance().LoadSound("unlock", "assets/sounds/unlock.ogg");
    SoundManager::GetInstance().LoadSound("key", "assets/sounds/KeyGet.ogg");
    SoundManager::GetInstance().LoadSound("barrelBreak", "assets/sounds/barrelBreak.ogg");
    SoundManager::GetInstance().LoadSound("musket", "assets/sounds/musket.ogg");
    SoundManager::GetInstance().LoadSound("chestOpen", "assets/sounds/chestOpen.ogg");
    SoundManager::GetInstance().LoadSound("spiderBite1", "assets/sounds/spiderBite1.ogg");
    SoundManager::GetInstance().LoadSound("spiderBite2", "assets/sounds/spiderBite2.ogg");
    SoundManager::GetInstance().LoadSound("spiderDeath", "assets/sounds/spiderDeath.ogg");
    SoundManager::GetInstance().LoadSound("flame1", "assets/sounds/flame1.ogg");
    SoundManager::GetInstance().LoadSound("flame2", "assets/sounds/flame2.ogg");
    SoundManager::GetInstance().LoadSound("explosion", "assets/sounds/explosion.ogg");
    SoundManager::GetInstance().LoadSound("staffHit", "assets/sounds/staffHit.ogg");
    SoundManager::GetInstance().LoadSound("iceMagic", "assets/sounds/iceMagic.ogg");
    SoundManager::GetInstance().LoadSound("jump", "assets/sounds/jump.ogg");
    SoundManager::GetInstance().LoadSound("harpoon", "assets/sounds/harpoon.ogg");
    SoundManager::GetInstance().LoadSound("ratchet", "assets/sounds/ratchet.ogg");
    SoundManager::GetInstance().LoadSound("giantSpiderBite", "assets/sounds/monsterBite.ogg");
    SoundManager::GetInstance().LoadSound("pirateDeath", "assets/sounds/pirateDeath.ogg");
    SoundManager::GetInstance().LoadSound("deathScream", "assets/sounds/deathScream.ogg");
    SoundManager::GetInstance().LoadSound("batDamage", "assets/sounds/batDamage.ogg");
    SoundManager::GetInstance().LoadSound("portal", "assets/sounds/portal.ogg");
    SoundManager::GetInstance().LoadSound("portal2", "assets/sounds/portal2.ogg");

    SoundManager::GetInstance().LoadSound("swim1", "assets/sounds/swim1.ogg");
    SoundManager::GetInstance().LoadSound("swim2", "assets/sounds/swim2.ogg");
    SoundManager::GetInstance().LoadSound("swim3", "assets/sounds/swim3.ogg");
    SoundManager::GetInstance().LoadSound("swim4", "assets/sounds/swim4.ogg");

    SoundManager::GetInstance().LoadSound("TrexRoar",       "assets/sounds/TrexRoar.ogg");
    SoundManager::GetInstance().LoadSound("TrexRoar2",      "assets/sounds/TrexRoar2.ogg");
    SoundManager::GetInstance().LoadSound("TrexBite",       "assets/sounds/TrexBite.ogg");
    SoundManager::GetInstance().LoadSound("TrexBite2",      "assets/sounds/TrexBite2.ogg");
    SoundManager::GetInstance().LoadSound("TrexHurt",       "assets/sounds/TrexHurt.ogg");
    SoundManager::GetInstance().LoadSound("TrexHurt2",      "assets/sounds/TrexHurt2.ogg");
    SoundManager::GetInstance().LoadSound("TrexStep",       "assets/sounds/TrexStep.ogg");
    SoundManager::GetInstance().LoadSound("eggHatch",       "assets/sounds/eggHatch.ogg");
    SoundManager::GetInstance().LoadSound("squish",         "assets/sounds/squish.ogg");
    SoundManager::GetInstance().LoadSound("crossbowFire",   "assets/sounds/crossbowFire.ogg");
    SoundManager::GetInstance().LoadSound("crossbowReload", "assets/sounds/crossbowReload.ogg");


    //speech
    SoundManager::GetInstance().LoadSound("hermitTalk1", "assets/sounds/AlienVoice1.ogg");
    SoundManager::GetInstance().LoadSound("hermitTalk2", "assets/sounds/AlienVoice2.ogg");
    SoundManager::GetInstance().LoadSound("hermitTalk3", "assets/sounds/AlienVoice3.ogg");
    SoundManager::GetInstance().LoadSound("hermitTalk4", "assets/sounds/AlienVoice4.ogg");
    SoundManager::GetInstance().LoadSound("hermitTalk5", "assets/sounds/AlienVoice5.ogg");
    SoundManager::GetInstance().LoadSound("hermitTalk6", "assets/sounds/AlienVoice6.ogg");

    SoundManager::GetInstance().RegisterSpeechBank("hermitSpeech", {
        "hermitTalk1","hermitTalk2","hermitTalk3",
        "hermitTalk4","hermitTalk5","hermitTalk6"
    });

    
    //music (ambience)
    SoundManager::GetInstance().LoadMusic("dungeonAir", "assets/sounds/dungeonAir.ogg");
    SoundManager::GetInstance().LoadMusic("jungleAmbience", "assets/sounds/jungleSounds.ogg");
}
