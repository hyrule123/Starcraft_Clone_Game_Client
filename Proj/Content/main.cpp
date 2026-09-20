// Content.cpp : Defines the entry point for the application.
//
#include "Content/pch.h"

#include <Content/framework.h>

#include <Engine/Core/EngineMain.h>

#include <Engine/Manager/SceneManager.h>
#include <Engine/Manager/ResourceManager.h>

#include <Content/Scene/MainGameScene.h>

#include <SCSimulator/Include/SCSimulatorAPI.h>


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    using namespace engine;
    EngineMain::GetInst().Init(hInstance);

    SceneManager::GetInst().ChangeScene(EntityManager::CreateEntity<MainGameScene>());

	const auto& res_dir = ResourceManager::GetInst().GetResourceDir();
    stdfs::path  mpq_dir = res_dir / "SCSim" / "mpq";
	
    SCSimContext* ctx = SCSim_Create(mpq_dir.string().c_str());

    if (ctx == nullptr)
    {
        const char* err = SCSim_GetLastError();
        return 0;
    }
    
    stdfs::path map_dir = res_dir / "SCMap" / "(4)Fighting Spirit 1.4.scx";
	bool result = SCSim_LoadMap(ctx, map_dir.string().c_str(), true);
    if (!result)
    {
        const char* err = SCSim_GetLastError();
        return 0;
    }

	uint32_t unit_id = SCSim_CreateUnit(ctx, 0, 0, 100, 100);

    for (int i = 0; i < 10; ++i)
    {
        SCSim_NextFrame(ctx);
    }

	int32_t cur_frame = SCSim_GetCurrentFrame(ctx);

    return EngineMain::GetInst().Run();
}
