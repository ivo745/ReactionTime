#pragma once

#include "StepTimer.h"
#include <CommonStates.h>
#include <SimpleMath.h>
#include <vector>
#include "audio.h"
#include <ctime>
#include <chrono>

#define TimeDecimals 3
#define maxLines 100

using namespace DirectX;
using namespace DirectX::SimpleMath;

class FileHandler;

class Game
{
public:
	Game();
	~Game();
	FileHandler* fH;
	// Initialization and management
	void Initialize(HWND window);
	// Basic game loop
	void Tick();
	void Render();
	// Rendering helpers
	void Clear();
	void Present();
	// Messages
	void OnActivated();
	void OnDeactivated();
	void OnSuspending();
	void OnResuming();
	void OnWindowSizeChanged();
	void GetDefaultSize(size_t& width, size_t& height) const;
	void StartScreen();
	void StartCountdown();
	void StartGame();
	void StartTimer();
	void GenerateShape();
	void CreateRectangle();
	void CreateTriangle();
	bool IsCursorInsideShape();
	bool OnButtonClick();
	void ShapeTapped();
	void ShapeMissed();
	void EndGame();
	int ShapeSize();
	int Credits();
	int GameTime();
	double GetTime();
	double GetFastestReactionTime();
	double GetSlowestReactionTime();
	double GetAverageReactionTime();
	double GetReactionTime() { return rtv.back(); }
	void ShowTime(std::string text, double value, std::streamsize decimals, float x, float y, FXMVECTOR color, float rotation, float scale);
	void ShowText(const wchar_t* widecstr, float x, float y, FXMVECTOR color, float rotation, float scale);
	enum GameState { state_null, state_suspended, state_startmenu, state_countdown, state_play, state_playcrazy, state_optionsmenu, state_endmenu, state_editor, state_max };
	enum ShapeTag { shape_triangle, shape_rectangle, shape_max };
	enum ButtonTag { button_null, button_options, button_start, button_crazy, button_sound, button_shapeSizeUp, button_shapeSizeDown, button_gameTimeUp, button_gameTimeDown, button_editor, button_useOwnShape, button_useGravity, button_epileptic, button_max };
	bool GetGameState(GameState state, bool last = false);
	void SetGameState(GameState state);
	void CreateButton(UINT8 button, XMVECTOR color);
	bool IsCursorInsideButton(ButtonTag tag);
	void ControlSound();
	bool useOwnShape = false;
	void Screenshot();
	void OnNewAudioDevice() { m_retryAudio = true; }
	bool crazyGame = false;
	bool isCursorInsideUnlock();
	bool missed = false;
	bool shape = false;
	double missTimer = 0.0;
	double splashScreenTimer = 7.0;
	float alphaSplash = 1.0f;
	float missPos = 0.0f;
	double shapeTimer = 0.0;
	float shapePos = 0.0f;
	bool tapped = false;
	double fadeTimer = 0.0;
	float alpha = 1.0f;
	float unlock = 0.0f;
	bool buttonDown = false;
	int ownButtonShape = 0;
	bool drawShape = false;
	void CreateOwnShape();
	Vector2 MousePoint[maxLines];
	std::vector<VertexPositionColor> vertexXM;
	Vector2 mPoint();
	bool checkIfOwnShapeInWindow();
	int randShape = 0;
	struct Shape { float r = 0.0f; float x = 0.0f; float y = 0.0f; } t, r;
	float deltaGravity = 0.0f;
	float dropCount = 1.0f;
	float deltaForce = 0.0f;
	bool useGravity = false;
	bool descending = false;
	bool ascending = false;
	bool forceRight = false;
	bool forceLeft = false;
	float deltaBounce = 0.0f;
	int randColorEpileptic = 0;
	bool EpilepticMode = false;
	double epilepticTimer = 0.0;
	bool calculateRandomColors();
	struct OwnShape { float r = 0.0f; float x = 0.0f; float y = 0.0f; } ownShape;
private:
	void Update(DX::StepTimer const& timer);
	bool m_retryAudio;
	void CreateDevice();
	void CreateResources();
	void OnDeviceLost();
	// Application state
	HWND m_window;
	RECT rc;
	// Direct3D Objects
	DirectX::SimpleMath::Matrix m_world;
	D3D_FEATURE_LEVEL m_featureLevel;
	Microsoft::WRL::ComPtr<ID3D11Device> m_d3dDevice;
	Microsoft::WRL::ComPtr<ID3D11Device1> m_d3dDevice1;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_d3dContext;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext1> m_d3dContext1;

	// Rendering resources
	Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain1;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_depthStencil;

	// Game state
	DX::StepTimer m_timer;
	std::unique_ptr<DirectX::BasicEffect> m_effect;
	std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_batch;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
	std::unique_ptr<DirectX::SpriteFont> m_font;
	DirectX::SimpleMath::Vector2 m_fontPos;
	std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
	std::unique_ptr<DirectX::AudioEngine> m_audEngine;
	std::unique_ptr<DirectX::SoundEffect> m_music;
	std::unique_ptr<DirectX::SoundEffect> m_right;
	std::unique_ptr<DirectX::SoundEffect> m_countdown;
	std::unique_ptr<DirectX::SoundEffect> m_wrong;
	std::unique_ptr<DirectX::SoundEffectInstance> m_tapped_i;
	std::unique_ptr<DirectX::SoundEffectInstance> m_music_i;
	std::unique_ptr<DirectX::SoundEffectInstance> m_missed_i;
	std::chrono::time_point<std::chrono::high_resolution_clock> startTimer, endTimer;
	std::vector<double> rtv;
	void CountDown(double time);
	void CursorClipCheck();
	float RandomFloat(float min, float max);
	int randColor = 0;
	double countdownTime = 0.0;
	double gameTime = 0.0;
	double crazyTimer = 0.0;
	bool ClickedOnce = false;
	GameState gameState = state_null;
	GameState oldState = state_null;
};