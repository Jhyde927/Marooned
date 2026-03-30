#include "inventory.h"
#include "raylib.h" // Only needed for DrawText
#include <vector>
#include "resourceManager.h"
#include "world.h"
#include "powerUps.h"


std::vector<std::string> slotOrder = {
    "HealthPotion",
    "ManaPotion",
    "PlaceHolder",
    "PlaceHolder2",
    "PlaceHolder3",
    "PlaceHolder4"
    
};

std::map<std::string, Texture2D> itemTextures;

void Inventory::SetupItemTextures() {
    itemTextures["HealthPotion"] = R.GetTexture("healthPotTexture");
    itemTextures["ManaPotion" ] = R.GetTexture("manaPotion");
}


void Inventory::AddItem(const std::string& itemId) {
    items[itemId]++;
}

bool Inventory::UseItem(const std::string& itemId) {
    auto it = items.find(itemId);
    if (it != items.end() && it->second > 0) {
        it->second--;
        return true;
    }
    return false;
}

bool Inventory::HasItem(const std::string& itemId) const {
    auto it = items.find(itemId);
    return (it != items.end() && it->second > 0);
}

int Inventory::GetItemCount(const std::string& itemId) const {
    auto it = items.find(itemId);
    return (it != items.end()) ? it->second : 0;
}

void DrawPowerUpTimer(float powerUpTimer, int slotX, int slotY, int slotWidth, int slotHeight)
{
    if (powerUpTimer <= 0.0f) return;

    int displaySeconds = (int)ceilf(powerUpTimer);
    if (displaySeconds < 0) displaySeconds = 0;
    if (displaySeconds > 99) displaySeconds = 99;

    char timerText[3];
    snprintf(timerText, sizeof(timerText), "%02d", displaySeconds);

    int fontSize = 30;
    int textWidth = MeasureText(timerText, fontSize);

    int textX = slotX + (slotWidth - textWidth) / 2;
    int textY = slotY + (slotHeight - fontSize) / 2;

    Color textColor = WHITE;
    if (player.powerUpTimer < 10.0f){
        textColor = RED;
    }

    DrawText(timerText, textX, textY, fontSize, textColor);
}


void Inventory::DrawInventoryUIWithIcons(const std::map<std::string, Texture2D>& itemTextures, const std::vector<std::string>& slotOrder, int x, int y, int slotSize, bool hasGoldKey, bool hasSilverKey, bool hasSkeletonKey, PowerUpType& type) const {
    int spacing = slotSize + 10;
    auto &font = R.GetFont("Pieces");
    
    for (int i = 0; i < slotOrder.size(); ++i) {
        const std::string& itemId = slotOrder[i];

        Rectangle slotRect = { (float)(x + i * spacing), (float)y, (float)slotSize, (float)slotSize };

        // Draw slot outline
        DrawRectangleRec(slotRect, (Color){20,20,20,200});
        DrawRectangleLinesEx(slotRect, 2, WHITE);

        // If we have the item and a texture for it
        if (HasItem(itemId) && itemTextures.count(itemId) > 0) {
            const Texture2D& tex = itemTextures.at(itemId);

            DrawTexturePro(
                tex,
                { 0, 0, (float)tex.width, (float)tex.height },
                { slotRect.x, slotRect.y, slotRect.width, slotRect.height },
                { 0, 0 },
                0.0f,
                WHITE
            );

            int count = GetItemCount(itemId);
            if (count > 1) {
                DrawTextEx(font, TextFormat("x%d", count), {slotRect.x + slotSize - 30, slotRect.y + slotSize - 25}, 24, 1, WHITE);
            }

            const char *key = (itemId == "HealthPotion" ? "F" : "G");
            DrawTextEx(font, key, {slotRect.x + slotSize - 15, slotRect.y}, 24, 1, WHITE);
        }
    }
        // Only show key icon if you have it
    if (hasGoldKey) {
        Texture2D keyIcon = ResourceManager::Get().GetTexture("keyTexture"); 
        DrawTexturePro(
            keyIcon,
            Rectangle{0,0,(float)keyIcon.width,(float)keyIcon.height},
            Rectangle{(float)(x + 2 * spacing), (float)y, (float)slotSize, (float)slotSize},
            Vector2{0,0},
            0.0f,
            WHITE
        );
    }

    // Only show key icon if you have it
    if (hasSilverKey) {
        Texture2D keyIcon = ResourceManager::Get().GetTexture("silverKey"); 
        DrawTexturePro(
            keyIcon,
            Rectangle{0,0,(float)keyIcon.width,(float)keyIcon.height},
            Rectangle{(float)(x + 3 * spacing), (float)y, (float)slotSize, (float)slotSize},
            Vector2{0,0},
            0.0f,
            WHITE
        );
    }

    if (hasSkeletonKey) {
        Texture2D keyIcon = ResourceManager::Get().GetTexture("skeletonKey"); 
        DrawTexturePro(
            keyIcon,
            Rectangle{0,0,(float)keyIcon.width,(float)keyIcon.height},
            Rectangle{(float)(x + 4 * spacing), (float)y, (float)slotSize, (float)slotSize},
            Vector2{0,0},
            0.0f,
            WHITE
        );
    }

    if (player.powerUpTimer > 0.0f){ //display count down timer while power up is active. 
        DrawPowerUpTimer(player.powerUpTimer, (x + 5 * spacing), y, slotSize, slotSize);
    }

    if (player.currentPowerUp == PowerUpType::QuadDamage){
        Texture2D upIcon = R.GetTexture("quadDamage");
        DrawTexturePro(
            upIcon,
            Rectangle{0,0,(float)upIcon.width,(float)upIcon.height},
            Rectangle{(float)(x + 5 * spacing), (float)y, (float)slotSize, (float)slotSize},
            Vector2{0,0},
            0.0f,
            WHITE
        );      
    }
    
    if (player.currentPowerUp == PowerUpType::Haste){
        Texture2D upIcon = R.GetTexture("haste");
        DrawTexturePro(
            upIcon,
            Rectangle{0,0,(float)upIcon.width,(float)upIcon.height},
            Rectangle{(float)(x + 5 * spacing), (float)y, (float)slotSize, (float)slotSize},
            Vector2{0,0},
            0.0f,
            WHITE
        );   

    }

    if (player.currentPowerUp == PowerUpType::OverHealth){
        Texture2D upIcon = R.GetTexture("overHealth");
        DrawTexturePro(
            upIcon,
            Rectangle{0,0,(float)upIcon.width,(float)upIcon.height},
            Rectangle{(float)(x + 5 * spacing), (float)y, (float)slotSize, (float)slotSize},
            Vector2{0,0},
            0.0f,
            WHITE
        );   

    }


}
