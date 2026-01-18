#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "hintManager.h"
#include "raylib.h"



class DialogManager
{
public:
    DialogManager();

    // ---- setup ----
    void SetFont(Font f, float size, float spacing);
    void SetHintManager(HintManager* hm);

    // ---- dialog data ----
    void AddDialog(const std::string& dialogId,
                   const std::vector<std::string>& lines);

    // ---- runtime control ----
    void StartDialog(const std::string& dialogId);
    void Advance();              // called on E
    void EndDialog();

    bool IsActive() const;

    // ---- update / draw ----
    void Update(float dt);
    void Draw() const;

private:
    // dialog definitions
    std::unordered_map<std::string, std::vector<std::string>> dialogs;

    // runtime state
    bool        isActive = false;
    std::string activeDialogId;
    int         currentLineIndex = 0;

    // UI config
    Font  font = {};
    float fontSize = 20.0f;
    float spacing  = 2.0f;

    HintManager* hintMgr = nullptr;

private:
    const std::string& CurrentLine() const;
};
