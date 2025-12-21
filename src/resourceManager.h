#pragma once
#include <string>
#include <unordered_map>
#include "raylib.h"
#include <stdexcept>
#include <memory>



class ResourceManager {
public:
    static ResourceManager& Get();
    
    // Texture
    Texture2D& LoadTexture(const std::string& name, const std::string& path);
    //Texture2D& GetTexture(const std::string& name) const;
    Texture2D&  GetTexture(const std::string& name);     
    // Model
    Model&      LoadModel(const std::string& name, const std::string& path);
    Model&      LoadModelFromMesh(const std::string& name, const Mesh& mesh);
    Model&      GetModel(const std::string& name) const;

    // Shader
    Shader&     LoadShader(const std::string& name, const std::string& vsPath, const std::string& fsPath);
    Shader&     GetShader(const std::string& name) const;


    // RenderTexture
    RenderTexture2D& LoadRenderTexture(const std::string& name, int w, int h);
    RenderTexture2D& GetRenderTexture(const std::string& name);             // non-const
    const RenderTexture2D& GetRenderTexture(const std::string& name) const; // const

    // RenderTexture2D& LoadRenderTexture(const std::string& name, int width, int height);
    // RenderTexture2D& GetRenderTexture(const std::string& name) const;

    Font& LoadFont(const std::string& name,
                   const std::string& path,
                   int baseSize = 128,
                   int filter = TEXTURE_FILTER_BILINEAR);

    // Optional: access by name without loading
    Font& GetFont(const std::string& name);

    // Optional: cleanup helpers
    void UnloadAllFonts();
    void LoadAllResources();
    void UpdateShaders(Camera& camera);
    void SetBloomShaderValues();
    void SetLavaShaderValues();
    void SetLightingShaderValues();
    void SetTerrainShaderValues();
    void SetPortalShaderValues();
    void SetWaterShaderValues(Camera& camera);
    void SetShaderValues();
    // void EnsureScreenSizedRTs();


    // Clean-up
    void UnloadAll();
    ~ResourceManager();

private:
    ResourceManager() = default;
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    void UnloadRenderTextures();
    template<typename T, typename UnloadFn>
    void UnloadContainer(std::unordered_map<std::string, T>& container, UnloadFn unloadFn) {
        for (auto& kv : container) unloadFn(kv.second);
        container.clear();
    }

    // Storage maps
    std::unordered_map<std::string, Texture2D>      _textures;
    std::unordered_map<std::string, Model>          _models;
    std::unordered_map<std::string, Shader>         _shaders;
    std::unordered_map<std::string, std::unique_ptr<RenderTexture2D>> _renderTextures; //render textures need to be resized on fullscreen. use safer method
    std::unordered_map<std::string, Font> _fonts;

    // Fallback texture (not in the map); 'mutable' so const GetTexture can lazy-init it
    mutable Texture2D _fallbackTex{};
    mutable bool      _fallbackReady = false;
    void ensureFallback_() const;   // creates procedural checker if needed

    static ResourceManager* _instance;
};

inline ResourceManager& R = ResourceManager::Get();
