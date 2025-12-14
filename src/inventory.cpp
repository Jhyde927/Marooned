#include "inventory.h"
#include "raylib.h" // Only needed for DrawText
#include <vector>
#include "resourceManager.h"


std::vector<std::string> slotOrder = {
    "HealthPotion",
    "ManaPotion",
    "GoldKey",
    "PlaceHolder",
    
};

std::map<std::string, Texture2D> itemTextures;

void Inventory::SetupItemTextures() {
    itemTextures["HealthPotion"] = R.GetTexture("healthPotTexture");
    itemTextures["GoldKey"] = R.GetTexture("keyTexture");
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


void Inventory::DrawInventoryUIWithIcons(const std::map<std::string, Texture2D>& itemTextures, const std::vector<std::string>& slotOrder, int x, int y, int slotSize) const {
    int spacing = slotSize + 10;

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
                DrawText(TextFormat("x%d", count), (int)slotRect.x + 4, (int)slotRect.y + slotSize - 20, 16, WHITE);
            }
        }
    }
}
