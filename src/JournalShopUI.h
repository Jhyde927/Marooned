#pragma once

#include "raylib.h"

#include <cstddef>
#include <string>
#include <vector>


enum class ShopUpgrade
{
    Harpoon,
    DoubleLoad,
    Fireball,
    Iceball,
    MagicMissile,
    ChainLightning
};

struct ShopWare
{
    ShopUpgrade upgrade;
    std::string name;
    std::string description;
    std::string textureKey;
    int price = 0;

};



class JournalShopUI
{
public:
    void SetWares(std::vector<ShopWare> newWares);
    void Init();
    void Open();
    void Close();
    bool IsOpen() const;

    void PurchaseSelectedWare();


    void Update();
    void Draw(int playerGold) const;

private:
    std::vector<ShopWare> wares;
    std::size_t selectedIndex = 0;
    bool open = false;
};