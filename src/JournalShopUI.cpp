#include "JournalShopUI.h"
#include "resourceManager.h"

#include <algorithm>
#include <sstream>
#include <utility>
#include "JournalBook.h"
#include "world.h"

namespace
{


    constexpr Color Ink{55, 34, 25, 255};

    void DrawCenteredText(Font font, const std::string& text,
                          Rectangle page, float y, float size, float spacing)
    {
        const float width = MeasureTextEx(font, text.c_str(), size, spacing).x;
        const Vector2 position{page.x + (page.width - width) / 2.0f, y};

        DrawTextEx(font, text.c_str(), position, size, spacing, Ink);
    }

    void DrawDescription(Font font, const std::string& description,
                         Vector2 position, float maxWidth, float scale)
    {
        const float size = 32.0f * scale;
        const float spacing = 2.0f * scale;
        const float lineHeight = 40.0f * scale;

        std::istringstream words(description);
        std::string word;
        std::string line;

        while (words >> word)
        {
            const std::string nextLine = line.empty() ? word : line + " " + word;

            if (!line.empty() &&
                MeasureTextEx(font, nextLine.c_str(), size, spacing).x > maxWidth)
            {
                DrawTextEx(font, line.c_str(), position, size, spacing, Ink);
                position.y += lineHeight;
                line = word;
            }
            else
            {
                line = nextLine;
            }
        }

        if (!line.empty())
        {
            DrawTextEx(font, line.c_str(), position, size, spacing, Ink);
        }
    }
}




void JournalShopUI::Init()
{
    SetWares({
        {ShopUpgrade::Harpoon, "Harpoon", "Upgrades your crossbow with a grappling harpoon. Pull yourself across gaps or drag enemies toward you.", "", 30},
        {ShopUpgrade::DoubleLoad, "Double Load", "Load a second charge into your blunderbuss for a devastating double shot.", "", 40},
        {ShopUpgrade::Fireball, "Fireball", "Launch an explosive ball of fire that scorches enemies caught in the blast.", "", 50},
        {ShopUpgrade::Iceball, "Iceball", "Launch a ball of ice that freezes enemies caught in the blast.", "", 60},
        {ShopUpgrade::MagicMissile, "Magic Missile", "Release three homing missiles that seek out nearby enemies.", "", 100},
        {ShopUpgrade::ChainLightning, "Chain Lightning", "Unleash a bolt of lightning that leaps between enemies, losing damage with each jump.", "", 150}
    });
}

void JournalShopUI::SetWares(std::vector<ShopWare> newWares)
{
    wares = std::move(newWares);
    selectedIndex = 0;

    if (wares.empty())
    {
        Close();
    }
}

void JournalShopUI::Open()
{
    open = !wares.empty();
}

void JournalShopUI::Close()
{
    open = false;
}

bool JournalShopUI::IsOpen() const
{
    return open;
}

void JournalShopUI::PurchaseSelectedWare()
{
    if (selectedIndex < 0 ||
        selectedIndex >= static_cast<int>(wares.size()))
    {
        return;
    }

    const ShopWare& ware = wares[selectedIndex];
    bool* unlocked = nullptr;

    switch (ware.upgrade)
    {
        case ShopUpgrade::Harpoon:
            unlocked = &hasHarpoon;
            break;

        case ShopUpgrade::DoubleLoad:
            unlocked = &hasDoubleShot;
            break;

        case ShopUpgrade::Fireball:
            unlocked = &hasStaff;
            break;

        case ShopUpgrade::Iceball:
            unlocked = &hasIce;
            break;

        case ShopUpgrade::MagicMissile:
            unlocked = &hasMissile;
            break;

        case ShopUpgrade::ChainLightning:
            unlocked = &hasLightning;
            break;
    }

    if (unlocked == nullptr || *unlocked)
    {
        return;
    }

    if (ware.price < 0 || player.gold < ware.price)
    {
        return;
    }

    *unlocked = true;
    player.gold -= ware.price;
}


void JournalShopUI::Update()
{
    if (!open)
    {
        return;
    }

    if (IsKeyPressed(KEY_ENTER))
    {
        // Get the currently selected ware.
        int selected = selectedIndex;
        if (wares[selectedIndex].price < player.gold){
            
        }
        // If the player has enough gold, call the purchase function.
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        Close();
        return;
    }

    if (IsKeyPressed(KEY_Q) && selectedIndex > 0)
    {
        --selectedIndex;
    }
    else if (IsKeyPressed(KEY_E) && selectedIndex + 1 < wares.size())
    {
        ++selectedIndex;
    }
}

void JournalShopUI::Draw(int playerGold) const
{
    if (!open)
    {
        return;
    }

    JournalBookLayout book{};
    if (!DrawJournalBook(book))
    {
        return;
    }

    const ShopWare& ware = wares[selectedIndex];
    const Font font = R.GetFont("Pieces");

    const Rectangle& leftPage = book.leftPageScreen;
    const Rectangle& rightPage = book.rightPageScreen;
    const float scale = book.scaleY;

    const float headingSize = 60.0f * scale;
    const float titleSize = 40.0f * scale;
    const float bodySize = 32.0f * scale;
    const float spacing = 2.0f * scale;
    const float top = leftPage.y;
    const float headingY = top + 38.0f * scale;

    DrawCenteredText(font, "Wares", leftPage,
                    headingY, headingSize, spacing);

    DrawCenteredText(font, ware.name, leftPage,
                     top + 110.0f * scale, titleSize, spacing);

    if (!ware.textureKey.empty())
    {
        const Texture2D icon = R.GetTexture(ware.textureKey);
        const float iconWidth = static_cast<float>(icon.width);
        const float iconHeight = static_cast<float>(icon.height);
        const float iconScale = std::min(
            220.0f * scale / iconWidth,
            180.0f * scale / iconHeight
        );

        const Rectangle iconSource{0.0f, 0.0f, iconWidth, iconHeight};
        const Rectangle iconDestination{
            leftPage.x + (leftPage.width - iconWidth * iconScale) / 2.0f,
            top + 260.0f * scale - iconHeight * iconScale / 2.0f,
            iconWidth * iconScale,
            iconHeight * iconScale
        };

        DrawTexturePro(icon, iconSource, iconDestination, {}, 0.0f, WHITE);
    }

    const float margin = 52.0f * scale;
    DrawDescription(font, ware.description,
                    {leftPage.x + margin, top + 380.0f * scale},
                    leftPage.width - 2.0f * margin, scale);

    const std::string pageLabel =
        "Q   " + std::to_string(selectedIndex + 1) +
        " / " + std::to_string(wares.size()) + "   E";

    DrawCenteredText(font, pageLabel, leftPage,
                     top + 590.0f * scale, bodySize, spacing);

    DrawCenteredText(font, "Purchase", rightPage,
                     headingY, headingSize, spacing);
    DrawCenteredText(font, "Price " + std::to_string(ware.price) + " gold",
                     rightPage, top + 190.0f * scale, titleSize, spacing);
    DrawCenteredText(font, "Your purse " + std::to_string(playerGold) + " gold",
                     rightPage, top + 255.0f * scale, bodySize, spacing);

    if (playerGold < ware.price)
    {
        const std::string shortfall =
            std::to_string(ware.price - playerGold) + " more gold needed";

        DrawCenteredText(font, shortfall, rightPage,
                         top + 330.0f * scale, bodySize, spacing);
    }

    DrawCenteredText(font, "Esc   Close", rightPage,
                     top + 590.0f * scale, bodySize, spacing);
}