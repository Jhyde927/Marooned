#include "journalUI.h"
#include "resourceManager.h"
#include <iostream>
#include "journalData.h"
#include <string>
#include "game_settings.h"
#include "shaderSetup.h"
#include "camera_system.h"
#include "JournalBook.h"

void JournalUI::Init()
{
    if (initialized)
        return;

    // bookTexture = &ResourceManager::Get().LoadRenderTexture(
    //     "journalBook",
    //     GetScreenWidth(),
    //     GetScreenHeight()
    // );

    InitJournalBook();

    //unlock washed ashore immediately.
    JournalData::Progress::DiscoverJournalEntry(
        JournalData::JournalEntryID::WashedAshore
    );

    currentJournalPage = 1;

    //ShowCreatureEntry(JournalData::CreatureEntryID::Raptor);
    currentCreaturePage = 0;

    grayShader = R.GetShader("grayscale");
    initialized = true;

}

static inline void DrawCarvedText(Font font, const char* text, Rectangle r, float fontSize, float spacing, bool title = false, bool selected = false)
{
    // dark “burnt” letter color
    Color ink = { 100, 70, 40, 255 };
    Color inkSelected = { 120, 90, 60, 255 };

    if (title) ink = {60, 40, 20, 255};

    // subtle top-left highlight (makes it look carved)
    Color hi  = { 255, 255, 255, 85 };

    Vector2 sz = MeasureTextEx(font, text, fontSize, spacing);
    Vector2 p  = { r.x + (r.width - sz.x)*0.5f,
                   r.y + (r.height - sz.y)*0.5f };

    // Highlight first (top-left)
    DrawTextEx(font, text, { p.x - 1, p.y - 1 }, fontSize, spacing, hi);

    // Main “engraved” text (slightly down-right)
    DrawTextEx(font, text, { p.x + 1, p.y + 1 }, fontSize, spacing, (selected ? inkSelected : ink));

    // Optional: reinforce center
    DrawTextEx(font, text, p, fontSize, spacing, (selected ? inkSelected : ink));
}

void JournalUI::OpenJournalSection()
{
    std::vector<const JournalData::JournalEntry*> journalEntries =
        JournalData::GetDiscoveredJournalEntries();

    currentJournalPage = static_cast<int>(journalEntries.size());

    if (journalEntries.empty())
    {
        currentJournalEntry = nullptr;
    }
    else
    {
        currentJournalEntry = journalEntries.back();
    }
}

void JournalUI::OpenCreatureSection()
{
    std::vector<const JournalData::CreatureEntry*> entries =
        JournalData::GetDiscoveredCreatureEntries();

    currentCreatureEntry =
        JournalData::Progress::GetLastDiscoveredCreature();

    currentCreaturePage = 0;

    if (currentCreatureEntry == nullptr)
    {
        return;
    }

    for (int i = 0; i < static_cast<int>(entries.size()); i++)
    {
        if (entries[i]->id == currentCreatureEntry->id)
        {
            currentCreaturePage = i + 1;
            currentCreatureEntry = entries[i];
            return;
        }
    }
}



void JournalUI::PreviousJournalPage()
{
    std::vector<const JournalData::JournalEntry*> entries =
        JournalData::GetDiscoveredJournalEntries();

    if (currentJournalPage > 0)
    {
        currentJournalPage--;
    }

    if (currentJournalPage == 0 || entries.empty())
    {
        currentJournalPage = 0;
        currentJournalEntry = nullptr;
        return;
    }

    currentJournalEntry = entries[currentJournalPage - 1];
}

void JournalUI::NextJournalPage()
{
    std::vector<const JournalData::JournalEntry*> entries =
        JournalData::GetDiscoveredJournalEntries();

    int lastPage = static_cast<int>(entries.size());

    if (currentJournalPage < lastPage)
    {
        currentJournalPage++;
    }

    if (currentJournalPage == 0 || entries.empty())
    {
        currentJournalPage = 0;
        currentJournalEntry = nullptr;
        return;
    }

    currentJournalEntry = entries[currentJournalPage - 1];
}

void JournalUI::PreviousCreaturePage()
{
    std::vector<const JournalData::CreatureEntry*> entries =
        JournalData::GetDiscoveredCreatureEntries();

    if (currentCreaturePage > 0)
    {
        currentCreaturePage--;
    }

    if (currentCreaturePage == 0)
    {
        currentCreatureEntry = nullptr;
        return;
    }

    currentCreatureEntry = entries[currentCreaturePage - 1];
}

void JournalUI::NextCreaturePage()
{
    std::vector<const JournalData::CreatureEntry*> entries =
        JournalData::GetDiscoveredCreatureEntries();

    int lastPage = static_cast<int>(entries.size());

    if (currentCreaturePage < lastPage)
    {
        currentCreaturePage++;
    }

    if (currentCreaturePage == 0)
    {
        currentCreatureEntry = nullptr;
        return;
    }

    currentCreatureEntry = entries[currentCreaturePage - 1];
}



void JournalUI::Update(float deltaTime)
{
    if (!initialized){
        return;
    }


    if (IsKeyPressed(KEY_J)){
        Toggle();
    }


    if (open && IsKeyPressed(KEY_ESCAPE)){
        Close();
    }

    if (open)
    {
        if (IsKeyPressed(KEY_Q))
        {
            PreviousJournalPage();
        }

        if (IsKeyPressed(KEY_E))
        {
            NextJournalPage();
        }

        if (IsKeyPressed(KEY_LEFT))
        {
            PreviousCreaturePage();
        }

        if (IsKeyPressed(KEY_RIGHT))
        {
            NextCreaturePage();
        }
    }


    float target = open ? 1.0f : 0.0f;

    if (openAmount < target)
    {
        openAmount += openSpeed * deltaTime;

        if (openAmount > target)
            openAmount = target;
    }
    else if (openAmount > target)
    {
        openAmount -= openSpeed * deltaTime;

        if (openAmount < target)
            openAmount = target;
    }
}



void JournalUI::DrawJournalPrompt()
{
    if (CameraSystem::Get().GetMode() != CamMode::Player) return;
    Font pieces = R.GetFont("Pieces");

    
    const bool hasNewEntry =
        JournalData::Progress::HasNewEntry();

    const float size = 42.0f;
    const float x = GetScreenWidth() - 64.0f;
    const float y = GetScreenHeight() - size - 24.0f;

    Color background = hasNewEntry ? GOLD : WHITE;
    Color foreground = BLACK;

    DrawRectangle(
        static_cast<int>(x),
        static_cast<int>(y),
        static_cast<int>(size),
        static_cast<int>(size),
        background
    );

    DrawRectangleLinesEx(
        Rectangle{x, y, size, size},
        2.0f,
        foreground
    );

    const char* keyText = "J";
    const float keyFontSize = 26.0f;
    const float keySpacing = 1.0f;

    Vector2 keyTextSize = MeasureTextEx(
        pieces,
        keyText,
        keyFontSize,
        keySpacing
    );

    DrawTextEx(
        pieces,
        keyText,
        Vector2{
            x + (size - keyTextSize.x) / 2.0f,
            y + (size - keyTextSize.y) / 2.0f
        },
        keyFontSize,
        keySpacing,
        foreground
    );

    const char* promptText =
        hasNewEntry ? "New journal entry" : "Journal";

    Vector2 promptSize = MeasureTextEx(pieces, promptText, 20.0f, 1.0f);

    Vector2 promptPosition{
        x - promptSize.x - 10.0f,
        y + (size - promptSize.y) / 2.0f
    };

    DrawTextEx(
        pieces,
        promptText,
        promptPosition,
        20.0f,
        1.0f,
        hasNewEntry ? GOLD : WHITE
    );
}

void JournalUI::Draw()
{
    if (!initialized || openAmount <= 0.0f)
    {
        return;
    }

    JournalBookLayout book{};
    if (!DrawJournalBook(book))
    {
        return;
    }

    const Rectangle& leftPageScreen = book.leftPageScreen;
    const Rectangle& rightPageScreen = book.rightPageScreen;
    const float scaleX = book.scaleX;
    const float scaleY = book.scaleY;

    Font& pieces = R.GetFont("Pieces");

    Color inkColor = Color{55, 34, 25, 255};

    // Keep the original code from headingSize through the end of Draw().

    //HERE

    float headingSize = 60.0f * scaleY;
    float entryTitleSize = 40.0f * scaleY;
    float bodySize = 32.0f * scaleY;
    float bodySpacing = 2.0f * scaleY;
    float lineSpacing = 8.0f * scaleY;



    auto DrawCenteredPageText =
        [&](const char* text, const Rectangle& page, float fontSize, float y)
    {
        Vector2 textSize = MeasureTextEx(
            pieces,
            text,
            fontSize,
            bodySpacing
        );

        Vector2 position{
            page.x + (page.width - textSize.x) / 2.0f,
            y
        };

        Rectangle destRect = {position.x, position.y, textSize.x, textSize.y};

        DrawCarvedText(pieces, text, destRect, fontSize, bodySpacing, false, false);
        // DrawTextEx(
        //     pieces,
        //     text,
        //     position,
        //     fontSize,
        //     bodySpacing,
        //     inkColor
        // );
    };

    auto DrawWrappedText =
        [&](const std::string& text,
            Vector2 position,
            float maxWidth,
            float fontSize,
            Font& font)
    {
        std::string line;
        std::string word;
        float currentY = position.y;
        float lineHeight = fontSize + lineSpacing;


        auto DrawLine = [&]()
        {
            if (!line.empty())
            {
                
                DrawTextEx(
                    font,
                    line.c_str(),
                    Vector2{position.x + 1.0f, currentY + 1.0f},
                    fontSize,
                    bodySpacing,
                    inkColor
                );

                DrawTextEx(
                    font,
                    line.c_str(),
                    Vector2{position.x, currentY},
                    fontSize,
                    bodySpacing,
                    inkColor
                );

                currentY += lineHeight;
                line.clear();
            }
        };

        for (size_t i = 0; i <= text.size(); i++)
        {
            bool endOfText = i == text.size();
            bool endOfWord =
                endOfText ||
                text[i] == ' ' ||
                text[i] == '\n';

            if (!endOfWord)
            {
                word += text[i];
                continue;
            }

            if (!word.empty())
            {
                std::string testLine = line;

                if (!testLine.empty())
                {
                    testLine += " ";
                }

                testLine += word;

                float testWidth = MeasureTextEx(
                    font,
                    testLine.c_str(),
                    fontSize,
                    bodySpacing
                ).x;

                if (testWidth > maxWidth && !line.empty())
                {
                    DrawLine();
                    line = word;
                }
                else
                {
                    line = testLine;
                }

                word.clear();
            }

            if (!endOfText && text[i] == '\n')
            {
                DrawLine();
                currentY += lineSpacing;
            }
        }

        DrawLine();
    };

    // Permanent page headings.
    DrawCenteredPageText(
        "Journal",
        leftPageScreen,
        headingSize,
        leftPageScreen.y + 45.0f * scaleY
    );

    DrawCenteredPageText(
        "Creatures",
        rightPageScreen,
        headingSize,
        rightPageScreen.y + 45.0f * scaleY
    );

    if (currentCreatureEntry != nullptr)
    {
        Texture2D& creatureTexture =
            R.GetTexture(currentCreatureEntry->textureName);

        float creatureTitleSize = 38.0f * scaleY;
        float creatureSpacing = 2.0f * scaleY;

        Vector2 titleSize = MeasureTextEx(
            pieces,
            currentCreatureEntry->name.c_str(),
            creatureTitleSize,
            creatureSpacing
        );

        Vector2 titlePosition{
            rightPageScreen.x +
                (rightPageScreen.width - titleSize.x) / 2.0f,
            rightPageScreen.y + 135.0f * scaleY
        };

        Rectangle destRect = {titlePosition.x, titlePosition.y, titleSize.x, titleSize.y};

        DrawCarvedText(pieces, currentCreatureEntry->name.c_str(), destRect, creatureTitleSize, creatureSpacing, false, false);

        // DrawTextEx(
        //     pieces,
        //     currentCreatureEntry->name.c_str(),
        //     titlePosition,
        //     creatureTitleSize,
        //     creatureSpacing,
        //     inkColor
        // );

        Rectangle creatureSource{
            0.0f,
            0.0f,
            static_cast<float>(currentCreatureEntry->frameWidth),
            static_cast<float>(currentCreatureEntry->frameHeight)
        };

        float imageSize = 230.0f * scaleY * currentCreatureEntry->imageScale;

        

        Rectangle creatureDestination{
            rightPageScreen.x +
                (rightPageScreen.width - imageSize) / 2.0f,
            rightPageScreen.y + 190.0f * scaleY,
            imageSize,
            imageSize
        };

        BeginShaderMode(grayShader);
        {
            DrawTexturePro(
                creatureTexture,
                creatureSource,
                creatureDestination,
                Vector2{0.0f, 0.0f},
                0.0f,
                WHITE
            );
        }
        EndShaderMode();

        // Description below the creature image.
        float descriptionMargin = 65.0f * scaleX;

        Vector2 descriptionPosition{
            rightPageScreen.x + descriptionMargin,
            creatureDestination.y + creatureDestination.height + 10.0f * scaleY
        };

        float descriptionWidth =
            rightPageScreen.width - descriptionMargin * 2.0f;

        DrawWrappedText(
            currentCreatureEntry->description,
            descriptionPosition,
            descriptionWidth,
            bodySize,
            pieces
        );

        //behavior below description
        float behaviorMargin = 65.0f * scaleX;

        Vector2 behaviorPosition{
            rightPageScreen.x + behaviorMargin,
            creatureDestination.y + creatureDestination.height + 100.0f * scaleY
        };

        float behaviorWidth =
            rightPageScreen.width - behaviorMargin * 2.0f;

        DrawWrappedText(
            currentCreatureEntry->behavior,
            behaviorPosition,
            behaviorWidth,
            bodySize,
            pieces
        );
    }

    if (currentJournalEntry != nullptr)
    {
        float entryTitleY = leftPageScreen.y + 145.0f * scaleY;

        DrawCenteredPageText(
            currentJournalEntry->title.c_str(),
            leftPageScreen,
            entryTitleSize,
            entryTitleY
        );

        float horizontalMargin = 65.0f * scaleX;

        Vector2 bodyPosition{
            leftPageScreen.x + horizontalMargin + 30.0f * scaleX,
            leftPageScreen.y + 220.0f * scaleY
        };

        float bodyWidth =
            leftPageScreen.width - horizontalMargin * 2.0f;

        DrawWrappedText(
            currentJournalEntry->body,
            bodyPosition,
            bodyWidth,
            bodySize,
            pieces
        );
    }

    DrawTextEx(
        pieces,
        "<- Q                  E ->",
        Vector2{
            leftPageScreen.x + 65.0f * scaleX,
            leftPageScreen.y + leftPageScreen.height - 55.0f * scaleY
        },
        40.0f * scaleY,
        1.0f * scaleY,
        inkColor
    );

    DrawTextEx(
        pieces,
        "{<-}                     {->}",
        Vector2{
            rightPageScreen.x + 65.0f * scaleX,
            rightPageScreen.y + rightPageScreen.height - 55.0f * scaleY
        },
        40.0f * scaleY,
        1.0f * scaleY,
        inkColor
    );
}


void JournalUI::Open()
{
    open = true;
    GameSettings::showJournal = open;
    JournalData::Progress::MarkEntriesSeen();
    OpenCreatureSection();
    OpenJournalSection();

}

void JournalUI::Close()
{
    open = false;
    GameSettings::showJournal = open;   
}

void JournalUI::Toggle()
{
    open = !open;
    GameSettings::showJournal = open;
    JournalData::Progress::MarkEntriesSeen();
    OpenCreatureSection();
    OpenJournalSection();

}

bool JournalUI::IsOpen() const
{
    return open;
}