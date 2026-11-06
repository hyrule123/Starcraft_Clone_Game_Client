// Content.cpp : Defines the entry point for the application.
//
#include "Content/pch.h"

#include <Content/framework.h>

#include <Engine/Core/EngineMain.h>

#include <Engine/Manager/SceneManager.h>

#include <Content/SCData/SCResourceManager.h>

#include <Content/Scene/LoadingScene.h>
#include <Content/Scene/MainGameScene.h>

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    engine::EngineMain::GetInst().Init(hInstance);

    sc::SCResourceManager::GetInst().Init();

    std::unique_ptr<sc::LoadingScene> loading_scene = sc::LoadingScene::CreateByName(engine::HashedStringView("MainGameScene"));
    engine::SceneManager::GetInst().ChangeScene(std::move(loading_scene));

    return engine::EngineMain::GetInst().Run();
}
