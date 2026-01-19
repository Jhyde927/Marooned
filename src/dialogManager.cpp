#include "dialogManager.h"
#include "hintManager.h" // for MeasureMultiline / DrawMultilineText
#include "iostream"
#include <algorithm>

DialogManager::DialogManager() {}

void DialogManager::SetFont(Font f, float size, float sp)
{
    font = f;
    fontSize = size;
    spacing  = sp;
}

void DialogManager::SetHintManager(HintManager* hm)
{
    hintMgr = hm;
}

void DialogManager::AddDialog(const std::string& dialogId,
                              const std::vector<std::string>& lines)
{
    dialogs[dialogId] = lines;
}

void DialogManager::StartDialog(const std::string& dialogId)
{
    auto it = dialogs.find(dialogId);
    if (it == dialogs.end()) return;
    
    activeDialogId   = dialogId;
    currentLineIndex = 0;
    isActive         = true;
}

void DialogManager::Advance()
{
    if (!isActive) return;

    currentLineIndex++;

    const auto& lines = dialogs[activeDialogId];

    if (currentLineIndex >= (int)lines.size())
    {
        EndDialog();
    }
}

void DialogManager::EndDialog()
{
    isActive = false;
    activeDialogId.clear();
    currentLineIndex = 0;
}

bool DialogManager::IsActive() const
{
    return isActive;
}

void DialogManager::SetActive(bool active){
    isActive = active;
}

int DialogManager::GetCurrentLineIndex(){
    return currentLineIndex;
}

const std::string& DialogManager::GetCurrentLineText() const
{
    static const std::string empty = "";

    if (!isActive) return empty;

    auto it = dialogs.find(activeDialogId);
    if (it == dialogs.end()) return empty;

    const auto& lines = it->second;
    if (currentLineIndex < 0 || currentLineIndex >= (int)lines.size())
        return empty;

    return lines[currentLineIndex];
}


void DialogManager::Update(float /*dt*/)
{
    // Reserved for:
    // - text reveal effects
    // - sound timing
    // - animation
}

const std::string& DialogManager::CurrentLine() const
{
    static std::string empty;
    if (!isActive) return empty;

    const auto& lines = dialogs.at(activeDialogId);
    if (currentLineIndex < 0 || currentLineIndex >= (int)lines.size())
        return empty;

    return lines[currentLineIndex];
}

void DialogManager::Draw() const
{
    if (!isActive || !hintMgr) return;

    const std::string& text = CurrentLine();
    if (text.empty()) return;

    // ---- layout ----
    const float padding = 20.0f;

    Vector2 textSize =
        hintMgr->MeasureMultiline(text, font, fontSize, spacing);

    Rectangle panel = {
        (GetScreenWidth()  - textSize.x) * 0.5f - padding,
        GetScreenHeight() - textSize.y   - 120.0f,
        textSize.x + padding * 2.0f,
        textSize.y + padding * 2.0f
    };

    // ---- draw panel ----
    DrawRectangleRec(panel, { 0, 0, 0, 200 });
    //DrawRectangleLinesEx(panel, 2.0f, WHITE);

    // ---- draw text ----
    Vector2 textPos = {
        panel.x + padding,
        panel.y + padding
    };

    hintMgr->DrawMultilineText(
        text,
        font,
        fontSize,
        spacing,
        textPos,
        WHITE,
        1.0f
    );
}
