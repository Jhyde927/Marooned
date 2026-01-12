#pragma once

#include <string>
#include <map>
#include "raylib.h"
#include <vector>

extern std::vector<std::string> slotOrder;
extern std::map<std::string, Texture2D> itemTextures;

class Inventory {
public:
    void AddItem(const std::string& itemId);
    bool UseItem(const std::string& itemId); // Returns true if successful
    bool HasItem(const std::string& itemId) const;
    int GetItemCount(const std::string& itemId) const;
    void SetupItemTextures();

    //void DrawInventoryUI(int x = 20, int y = 800) const;
    void DrawInventoryUIWithIcons(const std::map<std::string, Texture2D>& itemTextures, const std::vector<std::string>& slotOrder, int x, int y, int slotSize, bool hasGoldKey, bool hasSilverKey, bool hasSkeletonKey) const;

private:
    std::map<std::string, int> items;
};
