#include "pch.h"
#include "Game.h"
#include <Dbt.h>
#include <vector>
#include "FileHandler.h"
#include <windows.h>

using namespace DirectX;

namespace
{
	std::unique_ptr<Game> g_game;
};

namespace
{
	std::unique_ptr<FileHandler> fileHandler;
};

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void ClientResize(HWND hWnd, int nWidth, int nHeight)
{
	RECT rcClient, rcWind;
	POINT ptDiff;
	GetClientRect(hWnd, &rcClient);
	GetWindowRect(hWnd, &rcWind);
	ptDiff.x = (rcWind.right - rcWind.left) - rcClient.right;
	ptDiff.y = (rcWind.bottom - rcWind.top) - rcClient.bottom;
	MoveWindow(hWnd, rcWind.left, rcWind.top, nWidth + ptDiff.x, nHeight + ptDiff.y, TRUE);
}

// Entry point
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
	if (FAILED(hr))
		return 1;

	g_game.reset(new Game());

	// Register class and create window
	HDEVNOTIFY hNewAudio = nullptr;
	{
		// Register class
		WNDCLASSEX wcex;
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = LoadIcon(hInstance, L"IDI_ICON");
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = L"ReactionTimeWindowClass";
		wcex.hIconSm = LoadIcon(wcex.hInstance, L"IDI_ICON");
		if (!RegisterClassEx(&wcex))
			return 1;

		// Create window
		size_t w, h;
		g_game->GetDefaultSize(w, h);

		RECT rc;
		rc.top = 0;
		rc.left = 0;
		rc.right = static_cast<LONG>(w);
		rc.bottom = static_cast<LONG>(h);

		HWND hwnd = CreateWindow(L"ReactionTimeWindowClass", L"Reaction Time", WS_MINIMIZEBOX | WS_SYSMENU,
			CW_USEDEFAULT, CW_USEDEFAULT, rc.right, rc.bottom, nullptr, nullptr, hInstance,
			nullptr);
		if (!hwnd)
			return 1;

		RegisterTouchWindow(hwnd, 0);
		ShowWindow(hwnd, nCmdShow);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(g_game.get()));

		g_game->Initialize(hwnd);
		ClientResize(hwnd, rc.right, rc.bottom);

		// Listen for new audio devices
		DEV_BROADCAST_DEVICEINTERFACE filter = { 0 };
		filter.dbcc_size = sizeof(filter);
		filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
		filter.dbcc_classguid = KSCATEGORY_AUDIO;

		hNewAudio = RegisterDeviceNotification(hwnd, &filter,
			DEVICE_NOTIFY_WINDOW_HANDLE);
	}

	// Main message loop
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			if (g_game->GetGameState(g_game->state_suspended))
				Sleep(1);
			else
				g_game->Tick();
		}
	}

	g_game.reset();

	if (hNewAudio)
		UnregisterDeviceNotification(hNewAudio);

	CoUninitialize();

	return (int)msg.wParam;
}

// Windows procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	static bool s_in_sizemove = false;
	static bool s_in_suspend = false;
	static bool s_minimized = false;

	auto game = reinterpret_cast<Game*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (message)
	{
	case WM_TOUCH:
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	case WM_KEYUP:
		if (wParam == VK_SNAPSHOT)
			game->Screenshot();
		break;
	case WM_LBUTTONUP:
		game->buttonDown = false;
		// Sound button
		if (game->GetGameState(game->state_optionsmenu) || game->GetGameState(game->state_startmenu))
		{
			if (game->IsCursorInsideButton(game->button_sound))
				game->ControlSound();
		}
		// Options Menu: shape size up and down, game time down and up
		if (game->GetGameState(game->state_optionsmenu))
		{
			if (game->IsCursorInsideButton(game->button_useOwnShape))
				game->useOwnShape = !game->useOwnShape;
			else if (game->IsCursorInsideButton(game->button_epileptic))
				game->EpilepticMode = !game->EpilepticMode;
			else if (game->IsCursorInsideButton(game->button_useGravity))
				game->useGravity = !game->useGravity;
			else if (game->IsCursorInsideButton(game->button_shapeSizeUp))
			{
				if (GetKeyState(VK_SHIFT) & 0x8000)
				{
					if (game->ShapeSize() + 10 <= SHAPE_SIZE_MAX && game->Credits() - 10 >= 0)
						fileHandler->WriteConfig(game->Credits() - 10, game->ShapeSize() + 10, game->GameTime());
				}
				else
				{
					if (game->ShapeSize() + 1 <= SHAPE_SIZE_MAX && game->Credits() - 1 >= 0)
						fileHandler->WriteConfig(game->Credits() - 1, game->ShapeSize() + 1, game->GameTime());
				}
			}
			else if (game->IsCursorInsideButton(game->button_shapeSizeDown))
			{
				if (GetKeyState(VK_SHIFT) & 0x8000)
				{
					if (game->ShapeSize() - 10 > 0 && game->Credits() - 10 >= 0)
						fileHandler->WriteConfig(game->Credits() - 10, game->ShapeSize() - 10, game->GameTime());
				}
				else
				{
					if (game->ShapeSize() - 1 > 0 && game->Credits() - 1 >= 0)
						fileHandler->WriteConfig(game->Credits() - 1, game->ShapeSize() - 1, game->GameTime());
				}
			}
			else if (game->IsCursorInsideButton(game->button_gameTimeUp))
			{
				if (game->GameTime() + 1 <= GAME_TIME_MAX && game->Credits() - 1 >= 0)
					fileHandler->WriteConfig(game->Credits() - 1, game->ShapeSize(), game->GameTime() + 1);
			}
			else if (game->IsCursorInsideButton(game->button_gameTimeDown))
			{
				if (game->GameTime() - 1 > 0 && game->Credits() - 1 >= 0)
					fileHandler->WriteConfig(game->Credits() - 1, game->ShapeSize(), game->GameTime() - 1);
			}
		}
		break;
	case WM_LBUTTONDOWN:
		game->buttonDown = true;
		if (game->GetGameState(game->state_endmenu) && game->unlock < 380.0f)
			return 0;
		if (game->GetGameState(game->state_play) || game->GetGameState(game->state_playcrazy))
		{
			if (game->IsCursorInsideShape())
				game->ShapeTapped();
			else
				game->ShapeMissed();
		}
		else if (game->GetGameState(game->state_editor))
		{
			if (game->IsCursorInsideButton(game->button_options))
			{
				game->SetGameState(game->state_optionsmenu);
				return 0;
			}
			if (game->drawShape)
			{
				std::vector<VertexPositionColor>().swap(game->vertexXM);
				for (int i = 0; i < maxLines; i++)
				{
					game->MousePoint[i] = Vector2(0.0f, 0.0f);
				}
				game->ownButtonShape = 0;
				game->drawShape = false;
				return 0;
			}
			else if (!game->drawShape && game->ownButtonShape > 2 && game->ownButtonShape < 100)
			{
				if (game->mPoint().x < game->MousePoint[0].x + 10.0f && game->mPoint().x > game->MousePoint[0].x - 10.0f &&
					game->mPoint().y < game->MousePoint[0].y + 10.0f && game->mPoint().y > game->MousePoint[0].y - 10.0f)
				{
					game->MousePoint[game->ownButtonShape] = game->MousePoint[0];
					for (int i = 0; i < maxLines; i++)
					{
						game->vertexXM.push_back(VertexPositionColor(Vector2(game->MousePoint[i].x, game->MousePoint[i].y), Colors::Red));
					}
					game->drawShape = true;
					return 0;
				}
				game->MousePoint[game->ownButtonShape] = game->mPoint();
				game->vertexXM.push_back(VertexPositionColor(Vector2(game->MousePoint[game->ownButtonShape].x, game->MousePoint[game->ownButtonShape].y), Colors::Red));
				game->ownButtonShape++;
			}
			// last mouse
			//  [0] [0]
			switch (game->ownButtonShape)
			{
			case 0: // first point
				game->MousePoint[0] = game->mPoint();
				game->vertexXM.push_back(VertexPositionColor(Vector2(game->MousePoint[0].x, game->MousePoint[0].y), Colors::Red));
				game->ownButtonShape++;
				break;
			case 1: // second point
				game->MousePoint[1] = game->mPoint();
				game->vertexXM.push_back(VertexPositionColor(Vector2(game->MousePoint[1].x, game->MousePoint[1].y), Colors::Red));
				game->ownButtonShape++;
				break;
			case 2: // third point
				game->MousePoint[2] = game->mPoint();
				game->vertexXM.push_back(VertexPositionColor(Vector2(game->MousePoint[2].x, game->MousePoint[2].y), Colors::Red));
				game->ownButtonShape++;
				break;
			case 99: // Max point
				game->MousePoint[game->ownButtonShape] = game->MousePoint[0];
				game->vertexXM.push_back(VertexPositionColor(Vector2(game->MousePoint[game->ownButtonShape].x, game->MousePoint[game->ownButtonShape].y), Colors::Red));
				game->drawShape = true;
				return 0;
				break;
			}
		}
		// Start Menu or End Menu: start, crazy or options
		else if (game->GetGameState(game->state_startmenu) || game->GetGameState(game->state_endmenu))
		{
			if (game->IsCursorInsideButton(game->button_start))
			{
				game->StartCountdown();
				game->crazyGame = false;
			}
			else if (game->IsCursorInsideButton(game->button_crazy))
			{
				game->StartCountdown();
				game->crazyGame = true;
			}
			else if (game->IsCursorInsideButton(game->button_options))
				game->SetGameState(game->state_optionsmenu);
			if (game->IsCursorInsideButton(game->button_editor))
				game->SetGameState(game->state_editor);
		}
		// End Menu: options
		else if (game->GetGameState(game->state_endmenu, true))
		{
			if (game->IsCursorInsideButton(game->button_options))
				game->SetGameState(game->state_endmenu);
		}
		// Options Menu: back (to end menu)
		else if (game->GetGameState(game->state_optionsmenu))
		{
			if (game->IsCursorInsideButton(game->button_options))
				game->SetGameState(game->state_startmenu);
		}
		else if (game->GetGameState(game->state_null))
			game->SetGameState(game->state_startmenu);
		break;
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
		{
			if (!s_minimized)
			{
				s_minimized = true;
				if (!s_in_suspend && game)
					game->OnSuspending();
				s_in_suspend = true;
			}
		}
		else if (s_minimized)
		{
			s_minimized = false;
			if (s_in_suspend && game)
				game->OnResuming();
			s_in_suspend = false;
		}
		else if (!s_in_sizemove && game)
			game->OnWindowSizeChanged();
		break;
	case WM_ENTERSIZEMOVE:
		s_in_sizemove = true;
		break;
	case WM_EXITSIZEMOVE:
		s_in_sizemove = false;
		if (game)
			game->OnWindowSizeChanged();
		break;
	case WM_GETMINMAXINFO:
	{
		auto info = reinterpret_cast<MINMAXINFO*>(lParam);
		info->ptMinTrackSize.x = 320;
		info->ptMinTrackSize.y = 200;
	}
	break;
	case WM_ACTIVATEAPP:
		if (game)
		{
			if (wParam)
				game->OnActivated();
			else
				game->OnDeactivated();
		}
		break;
	case WM_POWERBROADCAST:
		switch (wParam)
		{
		case PBT_APMQUERYSUSPEND:
			if (!s_in_suspend && game)
				game->OnSuspending();
			s_in_suspend = true;
			return true;
		case PBT_APMRESUMESUSPEND:
			if (!s_minimized)
			{
				if (s_in_suspend && game)
					game->OnResuming();
				s_in_suspend = false;
			}
			return true;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_DEVICECHANGE:
		if (wParam == DBT_DEVICEARRIVAL)
		{
			auto pDev = reinterpret_cast<PDEV_BROADCAST_HDR>(lParam);
			if (pDev)
			{
				if (pDev->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
				{
					auto pInter = reinterpret_cast<const PDEV_BROADCAST_DEVICEINTERFACE>
						(pDev);
					if (pInter->dbcc_classguid == KSCATEGORY_AUDIO)
					{
						if (game)
							game->OnNewAudioDevice();
					}
				}
			}
		}
		return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}