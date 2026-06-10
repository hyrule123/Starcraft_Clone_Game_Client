// Content.cpp : Defines the entry point for the application.
//
#include "Content/pch.h"

#include <Content/framework.h>

#include <Engine/Core/EngineMain.h>

#include <Engine/Manager/SceneManager.h>
#include <Content/TestScene.h>

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    using namespace engine;
    EngineMain::GetInst().Init(hInstance);

    SceneManager::GetInst().ChangeScene(std::make_shared<TestScene>());

    return EngineMain::GetInst().Run();
}
