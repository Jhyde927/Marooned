#pragma once

#include "raylib.h"
#include "JournalData.h"

class JournalUI
{
public:
    void Init();
    void Update(float deltaTime);
    void Draw();
    void DrawJournalPrompt();
    void OpenCreatureSection();
    void OpenJournalSection();
    void ShowJournalEntry(JournalData::JournalEntryID id);
    void ShowCreatureEntry(JournalData::CreatureEntryID id);

    void PreviousJournalPage();
    void NextJournalPage();

    void PreviousCreaturePage();
    void NextCreaturePage();

    void Open();
    void Close();
    void Toggle();

    bool IsOpen() const;

private:
    bool initialized = false;
    bool open = false;

    float openAmount = 0.0f;
    float openSpeed = 4.0f;
    Shader grayShader{};
    RenderTexture2D* bookTexture = nullptr;

    int currentJournalPage = 0;
    int currentCreaturePage = 0;
    const JournalData::JournalEntry* currentJournalEntry = nullptr;
    const JournalData::CreatureEntry* currentCreatureEntry = nullptr;
};