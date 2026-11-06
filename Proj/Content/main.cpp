// Content.cpp : Defines the entry point for the application.
//
#include "Content/pch.h"

#include <Content/framework.h>

#include <Engine/Core/EngineMain.h>

#include <Engine/Manager/SceneManager.h>

#include <Content/SCData/SCResourceManager.h>

#include <Content/Scene/MainGameScene.h>
#include <thread>

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    engine::EngineMain::GetInst().Init(hInstance);

    sc::SCResourceManager::GetInst().Init();

    std::unique_ptr<sc::MainGameScene> scene;

    {
        std::jthread worker([&scene]()
            {
                scene = engine::EntityManager::CreateEntity<sc::MainGameScene>();
            });
    }	// 스코프 종료 시 join

    engine::SceneManager::GetInst().ChangeScene(std::move(scene));

    //engine::SceneManager::GetInst().ChangeScene("MainGameScene");

    return engine::EngineMain::GetInst().Run();
}
