#include "debug_console.h"

#include <sstream>
#include <algorithm>
#include "world.h"
#include "sound_manager.h"
#include "debug_overlay.h"
#include "shaderSetup.h"
#include "pathfinding.h"
#include <iomanip>
#include "vegetation_instanced.h"
#include "dungeon_props.h"
#include <iostream>

namespace DebugConsole
{

    Font gConsoleFont;
    static bool gIsOpen = false;

    // For later if you want the boot-up screen.
    static bool gHasBooted = false;

    static std::string gInput;
    static std::vector<std::string> gLog;

    static float gConsoleHeight = 0.0f;
    static float gTargetHeight = 0.0f;

    static constexpr float OPEN_HEIGHT = 300.0f;
    static constexpr int MAX_LOG_LINES = 12;

    
    static int gIgnoreTextInputFrames = 0;

    Color consoleLogColor = { 180, 125, 70, 255 }; // less saturated, more tan
    Color consoleInputColor = { 255, 238, 205, 255 };

    // ------------------------------------------------------------
    // Helpers
    // ------------------------------------------------------------

    static void ClearTextInputQueue()
    {
        int key = GetCharPressed();

        while (key > 0)
        {
            key = GetCharPressed();
        }
    }

    static std::string PadRight(const std::string& text, int width)
    {
        if ((int)text.length() >= width)
        {
            return text;
        }

        return text + std::string(width - text.length(), ' ');
    }

    static void LogCommandRow(
        const std::string& a,
        const std::string& b,
        const std::string& c,
        const std::string& d,
        const std::string& e,
        const std::string& f
    )
    {
        std::string line;

        line += PadRight(a, 16);
        line += PadRight(b, 16);
        line += PadRight(c, 16);
        line += PadRight(d, 16);
        line += PadRight(e, 16);
        line += PadRight(f, 16);
        Log(line);
    }



    static std::string FloatToCleanString(float value, int maxDecimals = 2)
    {
        std::ostringstream stream;

        stream << std::fixed << std::setprecision(maxDecimals) << value;

        std::string result = stream.str();

        // Remove trailing zeros after the decimal point
        result.erase(result.find_last_not_of('0') + 1);

        // Remove trailing decimal point if there are no decimals left
        if (!result.empty() && result.back() == '.')
        {
            result.pop_back();
        }

        return result;
    }

    static void FlushCharPressedQueue()
    {
        while (GetCharPressed() > 0)
        {
            // eat all pending typed characters
        }
    }

    static std::vector<std::string> SplitWords(const std::string& text)
    {
        std::vector<std::string> words;
        std::stringstream ss(text);
        std::string word;

        while (ss >> word)
        {
            words.push_back(word);
        }

        return words;
    }

    static std::string ToLower(std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });

        return text;
    }

    static bool TryParseInt(const std::string& text, int& outValue)
    {
        try
        {
            size_t charsRead = 0;
            int value = std::stoi(text, &charsRead);

            // Make sure the whole string was a number.
            // This rejects things like "5abc".
            if (charsRead != text.size())
            {
                return false;
            }

            outValue = value;
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    static void ExecuteCommand(const std::string& text)
    {
        std::vector<std::string> words = SplitWords(text);

        if (words.empty())
        {
            return;
        }

        std::string command = ToLower(words[0]);



        if (command == "sky")
        {
            int duration = 15; // default duration in seconds

            if (words.size() >= 2)
            {
                if (!TryParseInt(words[1], duration))
                {
                    Log("Invalid duration: " + words[1]);
                    return;
                }
            }

            CommandSky(duration);
        }
        else if (command == "health")
        {
            int amount = 1; // default amount

            if (words.size() >= 2)
            {
                if (!TryParseInt(words[1], amount))
                {
                    Log("Invalid amount: " + words[1]);
                    return;
                }
            }

            CommandHealth(amount);
        }
        else if (command == "mana")
        {
            int amount = 1; // default amount

            if (words.size() >= 2)
            {
                if (!TryParseInt(words[1], amount))
                {
                    Log("Invalid amount: " + words[1]);
                    return;
                }
            }

            CommandMana(amount);
        }
        else if (command == "position")
        {
            CommandPosition();
        }
        else if (command == "vegetation")
        {
            CommandVegetation();
        }
        else if (command == "doors")
        {
            CommandDoors();
        }
        else if (command == "quad")
        {
            CommandQuadDamage();
        }
        else if (command == "overhealth")
        {
            CommandOverHealth();
        }
        else if (command == "haste"){
            CommandHaste();
        }
        else if (command == "stamina")
        {
            CommandStamina();
        }
        else if (command == "enemies")
        {
            CommandEnemies();
        }
        else if (command == "start")
        {
            CommandStart();
        }
        else if (command == "end"){
            CommandEnd();
        }
        else if (command == "god"){
            CommandGod();
        }
        else if (command == "keys")
        {
            CommandKeys();
        }
        else if (command == "kill")
        {
            CommandKill();
        }
        else if (command == "stats")
        {
            CommandStats();
        }
        else if (command == "props"){
            CommandProps();
        }
        else if (command == "freecam")
        {
            CommandFreecam();
        }
        else if (command == "thirdperson")
        {
            CommandThirdPerson();
        }
        else if (command == "doubleshot")
        {
            CommandDoubleShot();
        }
        else if (command == "ceiling")
        {
            CommandCeiling();
        }
        else if (command == "weapons")
        {
            CommandWeapons();
        }else if (command == "fog"){
            CommandFog();
        }
        else if (command == "clear")
        {
            CommandClear();
        }
        else if (command == "exit")
        {
            CommandExit();
        }
        else if (command == "help")
        {
            Log("Commands:");
            LogCommandRow("Freecam",    "Health [amount]", "Mana [amount]", "Sky [duration]",      "Props",       "Exit");
            LogCommandRow("Vegetation", "Position",        "Keys",          "Stamina",             "Fog",         "");
            LogCommandRow("Enemies",    "Start",           "End",           "Kill",                "ThirdPerson", "");
            LogCommandRow("God",        "Doors",           "Stats",         "Ceiling",             "DoubleShot",  "");
            LogCommandRow("Weapons",    "Quad",            "Haste",         "Overhealth",          "Clear",       "");

        }
        else
        {
            Log("Unknown command: " + words[0]);
        }
    }

    // ------------------------------------------------------------
    // Public API
    // ------------------------------------------------------------

    void Init()
    {
        gConsoleFont = R.GetFont("terminal");
        SetTextureFilter(gConsoleFont.texture, TEXTURE_FILTER_POINT);

        gIsOpen = false;
        gHasBooted = false;
        gInput.clear();
        gLog.clear();

        gConsoleHeight = 0.0f;
        gTargetHeight = 0.0f;

        Log("Debug console initialized.");
    }

    void Toggle()
    {
        if (gIsOpen)
        {
            Close();
        }
        else
        {
            Open();
        }
    }

    void Open()
    {
        gIsOpen = true;
        gTargetHeight = OPEN_HEIGHT;

        gInput.clear();
        FlushCharPressedQueue();

        // Later this could trigger the boot-up screen once.
        if (!gHasBooted)
        {
            gHasBooted = true;
            Log("Marooned debug console online. Type help for command list");
        }
    }

    void Close()
    {
        gIsOpen = false;
        gTargetHeight = 0.0f;

        gInput.clear();
        FlushCharPressedQueue();


    }

    bool IsOpen()
    {
        return gIsOpen;
    }

    void Log(const std::string& message)
    {
        gLog.push_back(message);

        if ((int)gLog.size() > 100)
        {
            gLog.erase(gLog.begin());
        }
    }

    void Update(float dt)
    {
        // Toggle with ~ / `
        if (IsKeyPressed(KEY_GRAVE))
        {
            Toggle();

            // Linux/VMs may queue the ` character one or more frames later.
            gIgnoreTextInputFrames = 3;
            ClearTextInputQueue();

            return;
        }

        // Smooth slide open / closed
        gConsoleHeight += (gTargetHeight - gConsoleHeight) * 12.0f * dt;

        if (!gIsOpen)
        {
            return;
        }

        // Escape closes console
        if (IsKeyPressed(KEY_ESCAPE))
        {
            Close();
            return;
        }

        // Ignore text input briefly after opening console.
        if (gIgnoreTextInputFrames > 0)
        {
            gIgnoreTextInputFrames--;
            ClearTextInputQueue();
            return;
        }

        // Text input
        int key = GetCharPressed();

        while (key > 0)
        {
            // Printable ASCII characters
            if (key >= 32 && key <= 126 && key != '`' && key != '~') //make damn sure we are not printing ` on console open.
            {
                gInput.push_back(static_cast<char>(key));
            }

            key = GetCharPressed();
        }

        // Backspace
        if (IsKeyPressed(KEY_BACKSPACE) && !gInput.empty())
        {
            gInput.pop_back();
        }

        // Enter executes command
        if (IsKeyPressed(KEY_ENTER))
        {
            if (!gInput.empty())
            {
                Log("> " + gInput);
                ExecuteCommand(gInput);
                gInput.clear();
            }
        }
    }

    void Draw()
    {
        if (gConsoleHeight <= 1.0f)
        {
            return;
        }

        int screenWidth = GetScreenWidth();

        Rectangle panel = {
            0.0f,
            0.0f,
            static_cast<float>(screenWidth),
            gConsoleHeight
        };

        DrawRectangleRec(panel, Fade(BLACK, 0.85f));

        // Clip all text/details to the visible console rectangle.
        BeginScissorMode(
            0,
            0,
            screenWidth,
            static_cast<int>(gConsoleHeight)
        );

        // This makes the text move upward as the console closes.
        // When fully open: 0
        // When closing: negative
        float slideOffset = gConsoleHeight - OPEN_HEIGHT;

        int fontSize = 30;
        int lineHeight = 22;
        int x = 12;
        int y = 12 + static_cast<int>(slideOffset);

        int startLine = 0;

        if ((int)gLog.size() > MAX_LOG_LINES)
        {
            startLine = (int)gLog.size() - MAX_LOG_LINES;
        }

        for (int i = startLine; i < (int)gLog.size(); i++)
        {
            DrawTextEx(
                gConsoleFont,
                gLog[i].c_str(),
                { (float)x, (float)y },
                fontSize,
                1.0f,
                consoleLogColor
            );
            y += lineHeight;
        }

        // Only draw input line when console is actually open.
        // Otherwise the old input doesn't hang around during close.
        if (gIsOpen)
        {
            std::string inputLine = "> " + gInput + "_";

            DrawTextEx(
                gConsoleFont,
                inputLine.c_str(),
                { (float)x, (float)y },
                fontSize,
                1.0f,
                consoleInputColor
            );
        }

        EndScissorMode();

        // Trim line should draw after scissor so it is always visible at the edge.
        DrawRectangle(
            0,
            static_cast<int>(gConsoleHeight) - 3,
            screenWidth,
            3,
            Fade(GOLD, 0.85f)
        );

        // Scanlines can also be clipped if you want.
        for (int yLine = 0; yLine < static_cast<int>(gConsoleHeight); yLine += 4)
        {
            DrawRectangle(0, yLine, screenWidth, 1, Fade(BLACK, 0.25f));
        }
    }

    // ------------------------------------------------------------
    // Placeholder command functions
    // Replace these with real Marooned logic later.
    // ------------------------------------------------------------
    void CommandFog(){
        Log("Toggle Fog");
        GameSettings::useFog = !GameSettings::useFog;
    }


    void CommandSky(float duration){
        Log("Toggling Sky Transition. Durration: " + FloatToCleanString(duration));
        //toggle sky
        ShaderSetup::ToggleSkyTransition(duration);
    }

    void CommandVegetation(){
        Log("Toggle showVeg");
        VegetationInstanced::showVeg = !VegetationInstanced::showVeg;
    }

    void CommandDoors(){
        Log("Unlocking all doors");
        UnlockAllDoors();

    }

    void CommandEnemies(){
        Log("Killing all enemies");
        KillEnemies();
    }

    void CommandStart(){
        Log("Moving to start position");
        player.position = player.startPosition;
    }

    void CommandGod(){
        Log("Toggle God Mode");
        player.godMode = !player.godMode;
    }

    void CommandQuadDamage(){
        Log("Powerup Quad Damage");
        player.currentPowerUp = PowerUpType::QuadDamage;
    }

    void CommandHaste(){
        Log("Powerup Haste");
        player.currentPowerUp = PowerUpType::Haste;
    }

    void CommandOverHealth(){
        Log("Powerup Over Health");
        player.currentPowerUp = PowerUpType::OverHealth;
    }

    void CommandDoubleShot(){
        Log("Powerup double shot");
        hasDoubleShot = true;
    }

    void CommandStamina(){
        Log("Toggle infinite Stamina");
        player.infiniteStam = !player.infiniteStam;

    }

    void CommandEnd(){
        Log("Teleporting to exit door");
        TeleportPlayerToEnd();
    }

    void CommandPosition(){
        Vector2 playerTile = WorldToImageCoords(player.position);
        Log("Player Position: " + FloatToCleanString(player.position.x) + ", " + FloatToCleanString(player.position.y) + ", " + FloatToCleanString(player.position.z));
        Log("Player Tile: " + FloatToCleanString(playerTile.x) + ", " + FloatToCleanString(playerTile.y));

        DebugPrintVector(player.position);
        std::cout << "Player Tile: " << playerTile.x << ", " << playerTile.y << "\n";
    }

    void CommandKill(){
        Log("Killing Player");
        player.TakeDamage(101);

    }

    void CommandKeys(){
        Log("Give Keys.");
        GiveKeys();

    }

    void CommandHealth(int amount)
    {
        Log("Health command amount: " + std::to_string(amount));
    
        player.inventory.AddItemAmount("HealthPotion", amount);
        SoundManager::GetInstance().Play("clink");

    }

    void CommandMana(int amount)
    {
        Log("Mana command amount: " + std::to_string(amount));

        player.inventory.AddItemAmount("ManaPotion", amount);
        SoundManager::GetInstance().Play("clink");

    }

    void CommandThirdPerson(){
        Log("Toggle Third Person.");
        ToggleThirdPerson();

    }

    void CommandStats()
    {
        Log("Toggle Stats.");

        showStats = !showStats;
        
    }

    void CommandProps()
    {
        Log("Regenerate Corner Props");
        GenerateProps(floorHeight + 20);
    }

    void CommandFreecam()
    {
        Log("Toggle Freecam.");
        ToggleFreeCam();

    }

    void CommandCeiling()
    {
        Log("Toggle Ceiling");
        drawCeiling = !drawCeiling;

    }

    void CommandWeapons()
    {
        Log("Give All Weapons.");
        GiveWeapons();

    }

    void CommandClear(){
        Log("Clearing Log");
        gLog.clear();
    }

    void CommandExit()
    {
        Log("Exiting");
        Close();
    }
}