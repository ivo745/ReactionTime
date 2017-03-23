#include "pch.h"
#include "Game.h"
#include <string>       // std::string
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream
#include <numeric>      // std::accumulate
#include <algorithm>    // std::min_element
#include <time.h>       // time
#include "FileHandler.h"
#include "ShapeColors.h"
#include "D3D9.h"
#include <iomanip>
#include "Wincodec.h"
#include <thread>
#include <functional>
#include "Buttons.h"

#define GRID_RESOLUTION 20.0f

using namespace Microsoft::WRL;
using Microsoft::WRL::ComPtr;

Game::Game() : m_window(0), m_featureLevel(D3D_FEATURE_LEVEL_11_1) { }
Game::~Game()
{
	if (m_audEngine)
		m_audEngine->Suspend();

	m_music_i.reset();
	m_tapped_i.reset();
	m_missed_i.reset();
}

void Game::Initialize(HWND window)
{
	m_window = window;

	CreateDevice();
	CreateResources();

	m_timer.SetFixedTimeStep(true);
	m_timer.SetTargetElapsedSeconds(0.001);

	srand(static_cast<unsigned>(time(0)));

	AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
	eflags = eflags | AudioEngine_Debug;
#endif
	m_audEngine.reset(new AudioEngine(eflags));
	m_retryAudio = false;

	m_countdown.reset(new SoundEffect(m_audEngine.get(), L"Media/beep-07.wav"));
	m_music.reset(new SoundEffect(m_audEngine.get(), L"Media/Reaction_Time_Song.wav"));
	m_right.reset(new SoundEffect(m_audEngine.get(), L"Media/button-11.wav"));
	m_wrong.reset(new SoundEffect(m_audEngine.get(), L"Media/button-10.wav"));

	m_music_i = m_music->CreateInstance();
	m_music_i->Play(true);

	if (!fH->FileExists(CONFIG_FILE))
		fH->WriteConfig(0, DEFAULT_SHAPE_SIZE, DEFAULT_GAME_TIME);
	SetGameState(state_null);
}

void Game::Tick()
{
	m_timer.Tick([&]()
	{
		Update(m_timer);
	});

	Render();
}

void Game::ControlSound()
{
	if (m_music_i->GetState() == PLAYING)
		m_music_i->Pause();
	else
		m_music_i->Resume();
}

void Game::StartScreen()
{

}

void Game::StartTimer()
{
	startTimer = std::chrono::high_resolution_clock::now();
}

double Game::GetTime()
{
	endTimer = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed_seconds = endTimer - startTimer;
	return elapsed_seconds.count();
}

double Game::GetFastestReactionTime()
{
	if ((rtv.size()) == NULL)
		return 0;

	return *std::min_element(rtv.begin(), rtv.end());
}

double Game::GetSlowestReactionTime()
{
	if ((rtv.size()) == NULL)
		return 0;

	return *std::max_element(rtv.begin(), rtv.end());
}

double Game::GetAverageReactionTime()
{
	if ((rtv.size()) == NULL)
		return 0;

	double sum = std::accumulate(rtv.begin(), rtv.end(), 0.0);
	return sum / rtv.size();
}

std::string getDate()
{
	char date[30];
	date[0] = '\0';
	time_t now = time(NULL);
	strftime(date, 30, "%Y-%m-%d_%H.%M.%S", gmtime(&now));

	return date;
}

void Game::Screenshot()
{
	ComPtr<ID3D11Texture2D> backBufferTex;
	HRESULT hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferTex);
	if (SUCCEEDED(hr))
	{
		std::string result;
		std::stringstream ss;

		ss << "Screenshots/Screenshot_" << getDate() << ".png";
		result = ss.str();
		std::wstring widestr = std::wstring(result.begin(), result.end());
		LPCWSTR widecstr = widestr.c_str();
		hr = SaveWICTextureToFile(m_d3dContext.Get(), backBufferTex.Get(), GUID_ContainerFormatPng, widecstr);
	}
}

float Game::RandomFloat(float min, float max)
{
	assert(max >= min);
	float random = ((float)rand()) / (float)RAND_MAX;

	float range = max - min;
	return (random * range) + min;
}

void Game::ShowText(const wchar_t* widecstr, float x, float y, FXMVECTOR color, float rotation, float scale)
{
	Vector2 origin = m_font->MeasureString(widecstr) / 2.0f;
	m_spriteBatch->Begin();
	m_font->DrawString(m_spriteBatch.get(), widecstr, XMFLOAT2(x, y), color, rotation, origin, scale);
	m_spriteBatch->End();
}

void Game::ShowTime(std::string text, double value, std::streamsize decimals, float x, float y, FXMVECTOR color, float rotation, float scale)
{
	std::string result;
	std::stringstream ss;
	
	ss << std::fixed << std::setprecision(decimals) << text << value;
	result = ss.str();
	std::wstring widestr = std::wstring(result.begin(), result.end());
	const wchar_t* widecstr = widestr.c_str();
	Vector2 origin = m_font->MeasureString(widecstr) / 2.0f;

	m_spriteBatch->Begin();
	m_font->DrawString(m_spriteBatch.get(), widecstr, XMFLOAT2(x, y), color, rotation, origin, scale);
	m_spriteBatch->End();
}

void Game::StartCountdown()
{
	m_music_i->SetVolume(0.5f);
	countdownTime = 3;
	SetGameState(state_countdown);
}

void Game::StartGame()
{
	// Clear time vector before we start
	rtv.clear();
	calculateRandomColors();
	GenerateShape();
	gameTime = fH->ReadGameTimeFromFile();
	if (!crazyGame)
		SetGameState(state_play);
	else
		SetGameState(state_playcrazy);
}

double fRand(double fMin, double fMax)
{
	assert(fMax >= fMin);
	double f = (double)rand() / RAND_MAX;
	return fMin + f * (fMax - fMin);
}

void Game::CreateTriangle()
{
	XMVECTORF32 randomColor = { ColorList[randColor].r, ColorList[randColor].b, ColorList[randColor].g, ColorList[randColor].a };

	m_effect->Apply(m_d3dContext.Get());
	m_d3dContext->IASetInputLayout(m_inputLayout.Get());
	VertexPositionColor v1(Vector2(t.x + t.r, t.y - ShapeSize() + t.r), randomColor);
	VertexPositionColor v2(Vector2(t.x, t.y + t.r), randomColor);
	VertexPositionColor v3(Vector2(t.x - ShapeSize() + t.r, t.y - t.r), randomColor);
	m_batch->Begin();
	m_batch->DrawTriangle(v1, v2, v3);
	m_batch->End();
}

void Game::CreateRectangle()
{
	XMVECTORF32 randomColor = { ColorList[randColor].r, ColorList[randColor].b, ColorList[randColor].g, ColorList[randColor].a };

	m_effect->Apply(m_d3dContext.Get());
	m_d3dContext->IASetInputLayout(m_inputLayout.Get());
	VertexPositionColor v1(Vector2(r.x, r.y - ShapeSize() + r.r), randomColor);
	VertexPositionColor v2(Vector2(r.x - r.r, r.y), randomColor);
	VertexPositionColor v3(Vector2(r.x - ShapeSize(), r.y - r.r), randomColor);
	VertexPositionColor v4(Vector2(r.x - ShapeSize() + r.r, r.y - ShapeSize()), randomColor);
	m_batch->Begin();
	m_batch->DrawQuad(v1, v2, v3, v4);
	m_batch->End();
}

bool Game::checkIfOwnShapeInWindow()
{
	ownShape.x = RandomFloat(-GAME_WIDTH, GAME_WIDTH);
	ownShape.y = RandomFloat(-GAME_HEIGHT, GAME_HEIGHT);
	for (int i = 0; i < ownButtonShape; i++)
	{
		if (MousePoint[i].x + ownShape.x > GAME_WIDTH ||
			(MousePoint[i].x + ownShape.x) < 0.0f ||
			(MousePoint[i].y + ownShape.y) > GAME_HEIGHT ||
			(MousePoint[i].y + ownShape.y) < 0.0f)
			return false;
	}
	return true;
}

void Game::GenerateShape()
{
	if (!EpilepticMode)
		randColor = rand() % 20;
	if (!useOwnShape)
	{
		descending = true;
		ascending = false;
		forceRight = false;
		forceLeft = false;
		deltaForce = 0.0f;
		deltaGravity = 0.0f;
		dropCount = 1;
		randShape = rand() % shape_max;
		switch (randShape)
		{
		    case shape_triangle:
		    {
				t.r = RandomFloat(0.0f, (float)ShapeSize() / 2);
				t.x = RandomFloat((float)ShapeSize() - t.r, GAME_WIDTH - t.r);
				t.y = RandomFloat((float)ShapeSize() - t.r, GAME_HEIGHT - t.r);
		    	break;
		    }
		    case shape_rectangle:
		    {
				r.r = RandomFloat(0, (float)ShapeSize() / 2);
				r.x = RandomFloat((float)ShapeSize(), GAME_WIDTH);
				r.y = RandomFloat((float)ShapeSize(), GAME_HEIGHT);
		    	break;
		    }
		}
	}
	else
	{
		if (drawShape)
		{
			if (ownButtonShape > 2 && ownButtonShape < maxLines)
			{
				if (!checkIfOwnShapeInWindow())
				{
					GenerateShape();
					return;
				}
			}
		}
	}
	StartTimer();
	ClickedOnce = false;
}

void Game::ShapeTapped()
{
	if (ClickedOnce)
		return;
	
	ClickedOnce = true;
	shape = true;
	tapped = true;
	shapePos = 0.0f;
	shapeTimer = 0.0;
	alpha = 1.0f;
	fadeTimer = 0.0;
	rtv.insert(rtv.end(), GetTime());
	m_tapped_i = m_right->CreateInstance();
	m_tapped_i->SetVolume(0.6f);
	m_tapped_i->Play();
	if (GetGameState(state_play))
		GenerateShape();
}

void Game::ShapeMissed()
{
	gameTime -= 1.0;
	m_missed_i = m_wrong->CreateInstance();
	m_missed_i->SetVolume(0.3f);
	m_missed_i->Play();
	missed = true;
	missPos = 0.0f;
	missTimer = 0.0;
}

int Game::ShapeSize()
{
	return fH->ReadShapeSizeFromFile();
}

int Game::GameTime()
{
	return fH->ReadGameTimeFromFile();
}

int Game::Credits()
{
	return fH->ReadCreditsFromFile();
}

void Game::CreateButton(UINT8 buttonTag, XMVECTOR color)
{
	assert(buttonTag < MAX_BUTTONS);
	m_effect->Apply(m_d3dContext.Get());
	m_d3dContext->IASetInputLayout(m_inputLayout.Get());
	m_batch->Begin();
	VertexPositionColor v1(Vector2(ButtonArray[buttonTag][0].x, ButtonArray[buttonTag][0].y), color);
	VertexPositionColor v2(Vector2(ButtonArray[buttonTag][1].x, ButtonArray[buttonTag][1].y), color);
	VertexPositionColor v3(Vector2(ButtonArray[buttonTag][2].x, ButtonArray[buttonTag][2].y), color);
	VertexPositionColor v4(Vector2(ButtonArray[buttonTag][3].x, ButtonArray[buttonTag][3].y), color);
	m_batch->DrawQuad(v1, v2, v3, v4);
	m_batch->End();
}

bool Game::calculateRandomColors()
{
	randColorEpileptic = rand() % 20;
	randColor = rand() % 20;
	if (randColorEpileptic == randColor || randColor == randColorEpileptic)
	{
		calculateRandomColors();
		return false;
	}
	return true;
}

void Game::EndGame()
{
	m_music_i->SetVolume(1.0f);

	if (Credits() + rtv.size() <= INT_MAX)
		fH->WriteConfig(Credits() + rtv.size(), ShapeSize(), GameTime());

	missed = false;
	tapped = false;
	shape = false;
	shapePos = 0.0f;
	shapeTimer = 0.0;
	unlock = 160.0f;
	missPos = 0.0f;
	missTimer = 0.0;
	fadeTimer = 0.0;
	epilepticTimer = 0.0;
	ownShape.x = 0.0f;
	ownShape.y = 0.0f;
	SetGameState(state_endmenu);
}

void Game::SetGameState(GameState state)
{
	// don't wanna loop through same state
	if (state != oldState)
	   oldState = gameState;
	gameState = state;
}

bool Game::GetGameState(GameState state, bool last)
{
	switch (last)
	{
	case true:
		if (state == oldState)
			return true;
		break;
	case false:
		if (state == gameState)
			return true;
		break;
	}
	return false;
}

void Game::CursorClipCheck()
{
	switch (gameState)
	{
	case state_null:
	case state_startmenu:
	case state_countdown:
	case state_play:
	case state_playcrazy:
	case state_optionsmenu:
	case state_endmenu:
	case state_editor:
		if (ClipCursor(NULL))
		{
			GetWindowRect(m_window, &rc);
			ClipCursor(&rc);
		}
		break;
	case state_suspended:
		if (ClipCursor(&rc))
			ClipCursor(NULL);
		break;
	}
}

void Game::CreateOwnShape()
{
	if (ownButtonShape > 2 && ownButtonShape < maxLines)
	{
		XMVECTORF32 randomColor = { ColorList[randColor].r, ColorList[randColor].b, ColorList[randColor].g, ColorList[randColor].a };
		
		// outside edges
		for (int i = 0; i < ownButtonShape - 1; i++)
		{
			VertexPositionColor v1(Vector2(MousePoint[i].x + ownShape.x, MousePoint[i].y + ownShape.y), randomColor);
			VertexPositionColor v2(Vector2(MousePoint[i + 1].x + ownShape.x, MousePoint[i + 1].y + ownShape.y), randomColor);
			VertexPositionColor v3(Vector2(MousePoint[i + 2].x + ownShape.x, MousePoint[i + 2].y + ownShape.y), randomColor);
		    m_batch->DrawTriangle(v1, v2, v3);
	    }
		// inside
		for (int i = 2; i < ownButtonShape - 2; i += 2)
		{
			VertexPositionColor v1(Vector2(MousePoint[i].x + ownShape.x, MousePoint[i].y + ownShape.y), randomColor);
			VertexPositionColor v2(Vector2(MousePoint[i + 2].x + ownShape.x, MousePoint[i + 2].y + ownShape.y), randomColor);
			VertexPositionColor v3(Vector2(MousePoint[0].x + ownShape.x, MousePoint[0].y + ownShape.y), randomColor);
			m_batch->DrawTriangle(v1, v2, v3);
		}
	}
}

void Game::CountDown(double time)
{
	Clear();
	ShowTime("", time, 0, GAME_WIDTH / 2, 300.0f, Colors::LawnGreen, 0.0f, 1.0f);
	m_countdown->Play();
}

void Game::Update(DX::StepTimer const& timer)
{
	if (m_timer.GetFrameCount() == 0)
		return;

	CursorClipCheck();
	switch (gameState)
	{
	case state_null:
		splashScreenTimer -= 0.001;
		alphaSplash -= 0.00024f;
		if (splashScreenTimer <= 0)
			SetGameState(state_startmenu);
		break;
	case state_countdown:
		if (countdownTime == 3 || countdownTime < 2.001 && countdownTime > 2 || countdownTime < 1.001 && countdownTime > 1.0)
			CountDown(countdownTime);
		else if (countdownTime < 0.001)
			StartGame();
		countdownTime -= 0.002;
		break;
	case state_play:
	case state_playcrazy:
		if (EpilepticMode)
		{
			epilepticTimer += 0.001;
			if (epilepticTimer >= 0.25)
			{
				if (calculateRandomColors())
					epilepticTimer = 0.0;
			}
		}
		if (useGravity)
		{
			if (descending)
			{
				// while descending gravity increases, increasing the speed of the descend
				deltaGravity += 0.0025f;
				// max gravity
				if (deltaGravity < 2.5)
				{
					switch (randShape)
					{
					case shape_triangle:
						// increase y to make it drop to the floor and check if it's not outside game boundary
						if (t.y < GAME_HEIGHT && t.y >= 0.0f)
							t.y = t.y + deltaGravity;
						if (t.y + t.r >= GAME_HEIGHT)
						{
							// check if shape is not outside game boundary
							if (t.x - ShapeSize() + t.r >= 0 || t.x + t.r >= GAME_WIDTH)
							{
								// check if shape is rotated to the left
								if (t.r < ShapeSize() / 2)
								{
									t.r -= deltaGravity;
								}
								// check if shape is rotated to the right
								else if (t.r > ShapeSize() / 2)
								{
									t.r += deltaGravity;
								}
							}
							// check if x is not outside left game boundary, otherwise apply right force
							if (t.x - ShapeSize() + t.r <= 0)
							{
								ascending = true;
								descending = false;
								forceRight = true;
								forceLeft = false;
								dropCount = dropCount + 0.4f;
							}
							// check if x is not outside right game boundary, otherwise apply left force
							else if (t.x - t.r >= GAME_WIDTH)
							{
								ascending = true;
								descending = false;
								forceLeft = true;
								forceRight = false;
								dropCount = dropCount + 0.4f;
							}
							// check if the rotation is not outside game boundary otherwise make it bounce back
							if (t.r < 0)
							{
								t.r = 0;
								ascending = true;
								descending = false;
								forceRight = true;
								forceLeft = false;
								dropCount = dropCount + 0.4f;
							}
							else if (t.r > ShapeSize() / 1)
							{
								t.r = (float)ShapeSize() / 1;
								ascending = true;
								descending = false;
								forceLeft = true;
								forceRight = false;
								dropCount = dropCount + 0.4f;
							}
						}
						break;
					case shape_rectangle:
						// increase y to make it drop to the floor and check if it's not outside game boundary
						if (r.y < GAME_HEIGHT && r.y > 0.0f)
							r.y = r.y + deltaGravity;
						if (r.y >= GAME_HEIGHT)
						{
							if (r.x - ShapeSize() + r.r >= 0 || r.x + r.r >= GAME_WIDTH)
							{
								// check if shape is rotated to the left
								if (r.r < ShapeSize() / 2 && r.x - ShapeSize() >= 0.0f)
								{
									r.x -= deltaGravity / dropCount;
									r.y -= deltaGravity / dropCount;
									r.r -= deltaGravity / dropCount;
								}
								// check if shape is rotated to the right
								else if (r.r > ShapeSize() / 2)
								{
									r.r += deltaGravity / dropCount;
									r.x -= deltaGravity / dropCount;
									r.y -= deltaGravity / dropCount;
								}
							}
							// check if x is not outside left game boundary, otherwise apply right force
							if (r.x - ShapeSize() <= 0)
							{
								ascending = true;
								descending = false;
								forceRight = true;
								forceLeft = false;
								dropCount = dropCount + 0.4f;
							}
							// check if x is not outside right game boundary, otherwise apply left force
							else if (r.x - r.r >= GAME_WIDTH)
							{
								ascending = true;
								descending = false;
								forceLeft = true;
								forceRight = false;
								dropCount = dropCount + 0.4f;
							}
							// check if the rotation is not outside game boundary otherwise make it bounce back
							if (r.r < 0)
							{
								r.r = 0;
								ascending = true;
								descending = false;
								forceRight = true;
								forceLeft = false;
								dropCount = dropCount + 0.4f;
							}
							else if (r.r > ShapeSize() / 1)
							{
								r.r = (float)ShapeSize() / 1;
								ascending = true;
								descending = false;
								forceLeft = true;
								forceRight = false;
								dropCount = dropCount + 0.4f;
							}
						}
						break;
					}
				}
				else
				{
					descending = false;
					ascending = true;
				}
			}
			else if (ascending)
			{
				// while ascending gravity decreases, decreasing the speed of the ascend
				if (deltaGravity > 0.0f)
				{
					switch (randShape)
					{
					case shape_triangle:
						// decrease y to make it ascend to the ceiling and check if it's not outside game boundary
						if (t.y < GAME_HEIGHT - t.r && t.y > 0.0f)
							deltaGravity -= 0.0025f * dropCount;
						t.y = t.y - deltaGravity;
						if (t.y <= 0.0f + ShapeSize())
						{
							ascending = false;
							descending = true;
							deltaGravity = 0.0;
						}
						break;
					case shape_rectangle:
						// decrease y to make it ascend to the ceiling and check if it's not outside game boundary
						if (r.y < GAME_HEIGHT - r.r && r.y > 0.0f)
							deltaGravity -= 0.0025f  * dropCount;
						r.y = r.y - deltaGravity;
						if (r.y <= 0.0f + ShapeSize())
						{
							ascending = false;
							descending = true;
							deltaGravity = 0.0;
						}
						break;
					}
				}
				else
				{
					descending = true;
					ascending = false;
					deltaGravity = 0.0;
				}
			}
			if (forceRight)
			{
				// max force
				switch (randShape)
				{
				case shape_triangle:
					deltaForce += ((t.r / 1000) / 100) * deltaGravity;
					if (t.x < GAME_WIDTH && t.x > 0.0f)
						if ((deltaForce / dropCount) - 0.025f > 0.0f)
							t.x = t.x + (deltaForce / dropCount) - 0.025f;
					break;
				case shape_rectangle:
					deltaForce += ((r.r / 1000) / 100) * deltaGravity;
					if (r.x < GAME_WIDTH && r.x > 0.0f)
						if ((deltaForce / dropCount) - 0.025f > 0.0f)
							r.x = r.x + (deltaForce / dropCount) - 0.025f;
					break;
				}
			}
			else if (forceLeft)
			{
				// max force
				switch (randShape)
				{
				case shape_triangle:
					deltaForce -= ((t.r / 1000) / 100) * deltaGravity;
					t.x = t.x - (deltaForce / dropCount) - 0.02f;
					break;
				case shape_rectangle:
					deltaForce -= ((r.r / 1000) / 100) * deltaGravity;
					if (r.x < GAME_WIDTH && r.x > 0.0f)
						if ((deltaForce / dropCount) + 0.025f > 0.0f)
							r.x = r.x + (deltaForce / dropCount) + 0.025f;
					break;
				}
			}
		}

		if (missed)
		{
			missTimer += 0.001;
			if (missTimer < 1)
				missPos += 0.02f;
			else
			{
				missed = false;
				missPos = 0.0f;
				missTimer = 0.0;
			}
		}
		if (tapped)
		{
			fadeTimer += 0.001;
			if (fadeTimer > 0.4)
			{
				alpha -= 0.0015f;
				if (alpha <= 0.0f)
				{
					tapped = false;
					alpha = 1.0f;
					fadeTimer = 0.0;
				}
			}
		}
		if (shape)
		{
			shapeTimer += 0.001;
			if (shapeTimer < 1)
				shapePos += 0.02f;
			else
			{
				shape = false;
				shapePos = 0.0f;
				shapeTimer = 0.0;
			}
		}
		if (GetGameState(state_playcrazy) && GetTime() >= crazyTimer)
		{
			GenerateShape();
			crazyTimer = fRand(0.3, 0.6);
		}
		else if (gameTime <= 0.0)
			EndGame();
		gameTime -= 0.001;
		break;
	case state_endmenu:
		if (buttonDown)
		{
			if (mPoint().x >= 185.0f && mPoint().x <= 425.0f && mPoint().y <= 385.0f && mPoint().y >= 335.0f)
				unlock = mPoint().x - 25.0f;
		}
		break;
	default:
		break;
	}

	if (m_retryAudio)
	{
		m_retryAudio = false;
		if (m_audEngine->Reset())
		{
			if (m_music_i)
				m_music_i->Play(true);
		}
	}
	else if (!m_audEngine->Update())
	{
		if (m_audEngine->IsCriticalError())
			m_retryAudio = true;
	}
}

// Draws the scene
void Game::Render()
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
		return;

	if (!GetGameState(state_countdown))
	    Clear();

	switch (gameState)
	{
	    case state_null:
	    {
			XMVECTORF32 green = { 0.000000000f, 0.501960814f, 0.000000000f, alphaSplash };
	    	ShowText(L"Directx 11", GAME_WIDTH / 2, GAME_HEIGHT / 2- 20.0f, green, 0.0f, 0.6f);
			if (splashScreenTimer < 5)
			{
				XMVECTORF32 green = { 0.000000000f, 0.501960814f, 0.000000000f, alphaSplash + 0.35f };
				ShowText(L"the way shapes are meant to be made", GAME_WIDTH / 2, 320.0f, green, 0.0f, 0.6f);
			}
	    	break;
	    }
	    case state_suspended:
			break;
		case state_editor:
		{
			m_effect->Apply(m_d3dContext.Get());
			m_d3dContext->IASetInputLayout(m_inputLayout.Get());
			XMVECTORF32 Red = { 0.000000000f, 0.000000000f, 0.000000000f, 0.500000000f };

			m_batch->Begin();
			//vertical
			for (uint8_t i = 0; i < GAME_WIDTH / GRID_RESOLUTION; i++)
			{
				//vertical
				VertexPositionColor v17(Vector2(GRID_RESOLUTION*i, 0.0f), Red);
				VertexPositionColor v18(Vector2(GRID_RESOLUTION*i, GAME_HEIGHT), Red);
				m_batch->DrawLine(v17, v18);
				//horizontal
				VertexPositionColor v19(Vector2(0.0f, GRID_RESOLUTION*i), Red);
				VertexPositionColor v20(Vector2(GAME_WIDTH, GRID_RESOLUTION*i), Red);
				m_batch->DrawLine(v19, v20);
			}

			if (drawShape)
			{
				CreateOwnShape();
				m_batch->DrawLine(vertexXM[ownButtonShape], vertexXM[ownButtonShape - 1]);
			}
			if (ownButtonShape > 1)
			{
				for (int i = 0; i < ownButtonShape-1; i++)
				{
					m_batch->DrawLine(vertexXM[i], vertexXM[i+1]);
				}
			}

			m_batch->End();
			IsCursorInsideButton(button_options) ? CreateButton(button_options, Colors::GreenYellow) : CreateButton(button_options, Colors::Red);
			ShowText(L"Back", OptionsButton[0].x - 70.0f, OptionsButton[0].y + 35.0f, Colors::Black, 0.0f, 1.0f);
			ShowTime("ownButtonShape: ", ownButtonShape, 0, GAME_WIDTH / 4, 110.0f, Colors::Crimson, 0.0f, 0.6f);
			break;
		}
		case state_startmenu:
		{
			IsCursorInsideButton(button_options) ? CreateButton(button_options, Colors::GreenYellow) : CreateButton(button_options, Colors::Red);
			IsCursorInsideButton(button_start) ? CreateButton(button_start, Colors::GreenYellow) : CreateButton(button_start, Colors::Red);
			IsCursorInsideButton(button_crazy) ? CreateButton(button_crazy, Colors::GreenYellow) : CreateButton(button_crazy, Colors::Red);
			IsCursorInsideButton(button_sound) ? CreateButton(button_sound, Colors::GreenYellow) : CreateButton(button_sound, Colors::Red);
			IsCursorInsideButton(button_editor) ? CreateButton(button_editor, Colors::GreenYellow) : CreateButton(button_editor, Colors::Red);
			ShowText(L"Options", OptionsButton[0].x - 70.0f, OptionsButton[0].y + 35.0f, Colors::Black, 0.0f, 1.0f);
			ShowText(L"Start", StartButton[0].x - 90.0f, StartButton[0].y + 35.0f, Colors::Black, 0.0f, 1.0f);
			ShowText(L"Crazy", CrazyButton[0].x - 150.0f, CrazyButton[0].y + 35.0f, Colors::Black, 0.0f, 1.0f);
			ShowText(L"Editor", EditorButton[0].x - 150.0f, EditorButton[0].y + 35.0f, Colors::Black, 0.0f, 1.0f);
			break;
		}
		case state_countdown:
			break;
	    case state_play:
	    case state_playcrazy:
	    {
			if (gameTime > 0)
			{
				m_effect->Apply(m_d3dContext.Get());
				m_d3dContext->IASetInputLayout(m_inputLayout.Get());
				VertexPositionColor v1(Vector2(150.0f + 75.0f, 590.0f - 45.0f), Colors::WhiteSmoke);
				VertexPositionColor v2(Vector2(150.0f + 75.0f, 590.0f), Colors::WhiteSmoke);
				VertexPositionColor v3(Vector2(0.0f, 590.0f), Colors::WhiteSmoke);
				VertexPositionColor v4(Vector2(0.0f, 590.0f - 45.0f), Colors::WhiteSmoke);
				m_batch->Begin();
				m_batch->DrawQuad(v1, v2, v3, v4);
				VertexPositionColor v5(Vector2(525.0f + 75.0f, 0.0f), Colors::WhiteSmoke);
				VertexPositionColor v6(Vector2(525.0f + 75.0f, 50.0f), Colors::WhiteSmoke);
				VertexPositionColor v7(Vector2(415.0f, 50.0f), Colors::WhiteSmoke);
				VertexPositionColor v8(Vector2(415.0f, 0.0f), Colors::WhiteSmoke);
				m_batch->DrawQuad(v5, v6, v7, v8);
				VertexPositionColor v9(Vector2(1000.0f, 590 - 45.0f), Colors::WhiteSmoke);
				VertexPositionColor v10(Vector2(1000.0f, 590.0f), Colors::WhiteSmoke);
				VertexPositionColor v11(Vector2(850.0f - 75.0f, 590.0f), Colors::WhiteSmoke);
				VertexPositionColor v12(Vector2(850.0f - 75.0f, 590.0f - 45.0f), Colors::WhiteSmoke);
				m_batch->DrawQuad(v9, v10, v11, v12);
				m_batch->End();
				ShowTime("Time:", gameTime, 2, 110.0f, 570.0f, Colors::Black, 0.0f, 0.8f);
			}
			if (missed)
			    ShowText(L"-1", 105.0f, 520.0f + missPos, Colors::Red, 0.0f, 0.6f);
			if (tapped)
			{
				XMVECTORF32 Red = { 0.392156899f, 0.584313750f, 0.929411829f, alpha };
				if (GetReactionTime() < 0.1)
					ShowText(L"Hacker!", GAME_WIDTH / 2, 65.0f, Red, 0.0f, 0.75f);
				else if (GetReactionTime() < 0.2)
					ShowText(L"Unreal!", GAME_WIDTH / 2, 65.0f, Red, 0.0f, 0.75f);
				else if (GetReactionTime() < 0.3)
					ShowText(L"Amazing!", GAME_WIDTH / 2, 65.0f, Red, 0.0f, 0.75f);
				else if (GetReactionTime() < 0.4)
					ShowText(L"Great!", GAME_WIDTH / 2, 65.0f, Red, 0.0f, 0.75f);
				else
					ShowText(L"Slow!", GAME_WIDTH / 2, 65.0f, Red, 0.0f, 0.75f);
			}
			if (shape)
				ShowText(L"+1", 885.0f, 520.0f + shapePos, Colors::GreenYellow, 0.0f, 0.6f);
			if (!rtv.empty())
			{
				ShowTime("", GetReactionTime(), TimeDecimals, GAME_WIDTH / 2, 26.0f, Colors::Black, 0.0f, 1.0f);
				ShowTime("Shapes:", rtv.size(), 0, 885.0f, 570.0f, Colors::Black, 0.0f, 0.8f);
			}
			if (!useOwnShape)
			{
				switch (randShape)
				{
				case shape_triangle:
					CreateTriangle();
					break;
				case shape_rectangle:
					CreateRectangle();
					break;
				}
			}
			else
			{
				m_effect->Apply(m_d3dContext.Get());
				m_d3dContext->IASetInputLayout(m_inputLayout.Get());
				m_batch->Begin();
				if (drawShape)
				{
					if (ownButtonShape > 2)
					    CreateOwnShape();
				}
				m_batch->End();
			}
#ifdef _DEBUG
			ShowTime("deltaGravity: ", deltaGravity, 5, GAME_WIDTH / 4, 110.0f, Colors::Crimson, 0.0f, 0.6f);
			ShowTime("dropCount: ", dropCount, 5, GAME_WIDTH / 4, 140.0f, Colors::Crimson, 0.0f, 0.6f);
			ShowTime("t.y: ", t.y, 5, GAME_WIDTH / 4, 170.0f, Colors::Crimson, 0.0f, 0.6f);
			ShowTime("t.r: ", t.r, 5, GAME_WIDTH / 4, 200.0f, Colors::Crimson, 0.0f, 0.6f);
			ShowTime("t.x: ", t.x, 5, GAME_WIDTH / 4, 230.0f, Colors::Crimson, 0.0f, 0.6f);
			ShowTime("deltaForce: ", deltaForce, 5, GAME_WIDTH / 4, 260.0f, Colors::Crimson, 0.0f, 0.6f);
#endif // DEBUG
			break;
	    }
		case state_optionsmenu:
			IsCursorInsideButton(button_options) ? CreateButton(button_options, Colors::GreenYellow) : CreateButton(button_options, Colors::Red);
			IsCursorInsideButton(button_start) ? CreateButton(button_start, Colors::GreenYellow) : CreateButton(button_start, Colors::Red);
			IsCursorInsideButton(button_crazy) ? CreateButton(button_crazy, Colors::GreenYellow) : CreateButton(button_crazy, Colors::Red);
			IsCursorInsideButton(button_shapeSizeDown) ? CreateButton(button_shapeSizeDown, Colors::GreenYellow) : CreateButton(button_shapeSizeDown, Colors::Red);
			IsCursorInsideButton(button_shapeSizeUp) ? CreateButton(button_shapeSizeUp, Colors::GreenYellow) : CreateButton(button_shapeSizeUp, Colors::Red);
			IsCursorInsideButton(button_gameTimeUp) ? CreateButton(button_gameTimeUp, Colors::GreenYellow) : CreateButton(button_gameTimeUp, Colors::Red);
			IsCursorInsideButton(button_gameTimeDown) ? CreateButton(button_gameTimeDown, Colors::GreenYellow) : CreateButton(button_gameTimeDown, Colors::Red);
			IsCursorInsideButton(button_sound) ? CreateButton(button_sound, Colors::GreenYellow) : CreateButton(button_sound, Colors::Red);
			EpilepticMode ? CreateButton(button_epileptic, Colors::GreenYellow) : CreateButton(button_epileptic, Colors::Red);
			useGravity ? CreateButton(button_useGravity, Colors::GreenYellow) : CreateButton(button_useGravity, Colors::Red);
			useOwnShape ? CreateButton(button_useOwnShape, Colors::GreenYellow) : CreateButton(button_useOwnShape, Colors::Red);
			ShowText(L"Epileptic", EpilepticModeButton[0].x - 70.0f, EpilepticModeButton[0].y + 35.0f, Colors::Black, 0.0f, 1.0f);
			ShowText(L"Use own shape", UseOwnShapeButton[0].x - 70.0f, UseOwnShapeButton[0].y + 35.0f, Colors::Black, 0.0f, 1.0f);
			ShowText(L"Use gravity", UseGravityButton[0].x - 70.0f, UseGravityButton[0].y + 35.0f, Colors::Black, 0.0f, 1.0f);
			ShowText(L"Back", OptionsButton[0].x - 70.0f, OptionsButton[0].y + 35.0f, Colors::Black, 0.0f, 1.0f);
			ShowText(L"-", ShapeSizeDownButton[0].x - 13.0f, ShapeSizeDownButton[0].y + 11.0f, Colors::Black, 0.0f, 0.7f);
			ShowText(L"+", ShapeSizeUpButton[0].x - 13.0f, ShapeSizeUpButton[0].y + 11.0f, Colors::Black, 0.0f, 0.7f);
			ShowText(L"+", GameTimeUpButton[0].x - 13.0f, GameTimeUpButton[0].y + 11.0f, Colors::Black, 0.0f, 0.7f);
			ShowText(L"-", GameTimeDownButton[0].x - 13.0f, GameTimeDownButton[0].y + 11.0f, Colors::Black, 0.0f, 0.7f);
			ShowTime("Game Time: ", GameTime(), 0, GAME_WIDTH / 2, 80.0f, Colors::Red, 0.0f, 0.5f);
			ShowTime("Shape Size: ", ShapeSize(), 0, GAME_WIDTH / 2, 100.0f, Colors::Red, 0.0f, 0.5f);
			ShowTime("Credits: ", Credits(), 0, GAME_WIDTH / 2, 120.0f, Colors::Red, 0.0f, 0.5f);
			break;
		case state_endmenu:
		{
			IsCursorInsideButton(button_options) ? CreateButton(button_options, Colors::GreenYellow) : CreateButton(button_options, Colors::Red);
			IsCursorInsideButton(button_start) ? CreateButton(button_start, Colors::GreenYellow) : CreateButton(button_start, Colors::Red);
			IsCursorInsideButton(button_crazy) ? CreateButton(button_crazy, Colors::GreenYellow) : CreateButton(button_crazy, Colors::Red);
			IsCursorInsideButton(button_editor) ? CreateButton(button_editor, Colors::GreenYellow) : CreateButton(button_editor, Colors::Red);
			ShowText(L"Editor", EditorButton[0].x - 150.0f, EditorButton[0].y + 35.0f, Colors::Black, 0.0f, 1.0f);
			ShowText(L"Options", OptionsButton[0].x - 70.0f, OptionsButton[0].y + 35.0f, Colors::Black, 0.0f, 1.0f);
			ShowText(L"Start", StartButton[0].x - 90.0f, StartButton[0].y + 35.0f, Colors::Black, 0.0f, 1.0f);
			ShowText(L"Crazy", CrazyButton[0].x - 150.0f, CrazyButton[0].y + 35.0f, Colors::Black, 0.0f, 1.0f);
			ShowTime("Fastest reaction time: ", GetFastestReactionTime(), TimeDecimals, GAME_WIDTH / 2, 30.0f, Colors::Coral, 0.0f, 1.0f);
			ShowTime("Slowest reaction time: ", GetSlowestReactionTime(), TimeDecimals, GAME_WIDTH / 2, 30.0f*2.5, Colors::Crimson, 0.0f, 1.0f);
			ShowTime("Avarage reaction time: ", GetAverageReactionTime(), TimeDecimals, GAME_WIDTH / 2, 120.0f, Colors::Magenta, 0.0f, 1.0f);
			ShowTime("Shapes tapped: ", rtv.size(), 0, GAME_WIDTH / 2, 165.0f, Colors::DeepSkyBlue, 0.0f, 1.0f);
			ShowTime("Credits: ", Credits(), 0, GAME_WIDTH / 2, 210.0f, Colors::Crimson, 0.0f, 1.0f);
			m_effect->Apply(m_d3dContext.Get());
			m_d3dContext->IASetInputLayout(m_inputLayout.Get());
			if (unlock <= 370.0f)
			{
					VertexPositionColor v1(Vector2(unlock + 50.0f, 385.0f - 50.0f), Colors::CornflowerBlue);
					VertexPositionColor v2(Vector2(unlock, 385.0f), Colors::CornflowerBlue);
					VertexPositionColor v3(Vector2(160.0f, 385.0f), Colors::CornflowerBlue);
					VertexPositionColor v4(Vector2(160.0f, 385.0f - 50.0f), Colors::CornflowerBlue);
					m_batch->Begin();
					m_batch->DrawQuad(v1, v2, v3, v4);
					m_batch->End();
					if (isCursorInsideUnlock())
						ShowText(L"Drag to unlock", 275.0f, 360.0f, Colors::Black, 0.0f, 0.6f);
			}
			else if (unlock > 370.0f)
			{
				VertexPositionColor v1(Vector2(305.0f + 125.0f, 385.0f - 50.0f), Colors::Red);
				VertexPositionColor v2(Vector2(305.0f + 75.0f, 385.0f), Colors::Red);
				VertexPositionColor v3(Vector2(160.0f, 385.0f), Colors::Red);
				VertexPositionColor v4(Vector2(160.0f, 385.0f - 50.0f), Colors::Red);
				m_batch->Begin();
				m_batch->DrawQuad(v1, v2, v3, v4);
				m_batch->End();
				unlock = 380.0f;
				if (isCursorInsideUnlock())
					ShowText(L"Drag to lock", 275.0f, 360.0f, Colors::Black, 0.0f, 0.6f);
			}
			break;
		}
		default:
			break;
	}

	Present();
}

// Helper method to clear the backbuffers
void Game::Clear()
{
	// Clear the views
	if (EpilepticMode && GetGameState(state_play) || EpilepticMode && GetGameState(state_playcrazy))
	{
		XMVECTORF32 randomColor = { ColorList[randColorEpileptic].r, ColorList[randColorEpileptic].b, ColorList[randColorEpileptic].g, ColorList[randColorEpileptic].a };
		m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), randomColor);
	}
	else if (GetGameState(state_null))
		m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::DarkGray);
	else
	    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::White);
	m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
}

// Presents the backbuffer contents to the screen
void Game::Present()
{
	// The first argument instructs DXGI to block until VSync, putting the application
	// to sleep until the next VSync. This ensures we don't waste any cycles rendering
	// frames that will never be displayed to the screen.
	HRESULT hr = m_swapChain->Present(0, 0);

	// If the device was reset we must completely reinitialize the renderer.
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		OnDeviceLost();
	else
		DX::ThrowIfFailed(hr);
}

// Message handlers
void Game::OnActivated()
{
	m_audEngine->Resume();
	SetGameState(oldState);
}

void Game::OnDeactivated()
{
	m_audEngine->Suspend();
	SetGameState(state_suspended);
}

void Game::OnSuspending()
{
}

void Game::OnResuming()
{
	m_audEngine->Resume();
	SetGameState(oldState);
}

void Game::OnWindowSizeChanged()
{
}

// Properties
void Game::GetDefaultSize(size_t& width, size_t& height) const
{
	width = GAME_WIDTH;
	height = GAME_HEIGHT;
}

// These are the resources that depend on the device.
void Game::CreateDevice()
{
	// This flag adds support for surfaces with a different color channel ordering than the API default.
	UINT creationFlags = 0;

#ifdef _DEBUG
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	static const D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};

	// Create the DX11 API device object, and get a corresponding context.
	HRESULT hr = D3D11CreateDevice(
		nullptr,                                // specify null to use the default adapter
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,                                // leave as nullptr unless software device
		creationFlags,                          // optionally set debug and Direct2D compatibility flags
		featureLevels,                          // list of feature levels this app can support
		_countof(featureLevels),                // number of entries in above list
		D3D11_SDK_VERSION,                      // always set this to D3D11_SDK_VERSION
		m_d3dDevice.ReleaseAndGetAddressOf(),   // returns the Direct3D device created
		&m_featureLevel,                        // returns feature level of device created
		m_d3dContext.ReleaseAndGetAddressOf()   // returns the device immediate context
		);

	if (hr == E_INVALIDARG)
	{
		// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
		hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
			creationFlags, &featureLevels[1], _countof(featureLevels) - 1,
			D3D11_SDK_VERSION, m_d3dDevice.ReleaseAndGetAddressOf(),
			&m_featureLevel, m_d3dContext.ReleaseAndGetAddressOf());
	}

	DX::ThrowIfFailed(hr);

#ifndef NDEBUG
	ComPtr<ID3D11Debug> d3dDebug;
	hr = m_d3dDevice.As(&d3dDebug);
	if (SUCCEEDED(hr))
	{
		ComPtr<ID3D11InfoQueue> d3dInfoQueue;
		hr = d3dDebug.As(&d3dInfoQueue);
		if (SUCCEEDED(hr))
		{
#ifdef _DEBUG
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
			D3D11_MESSAGE_ID hide[] =
			{
				D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
				// TODO: Add more message IDs here as needed 
			};
			D3D11_INFO_QUEUE_FILTER filter;
			memset(&filter, 0, sizeof(filter));
			filter.DenyList.NumIDs = _countof(hide);
			filter.DenyList.pIDList = hide;
			d3dInfoQueue->AddStorageFilterEntries(&filter);
		}
	}
#endif

	// TODO: Initialize device dependent objects here (independent of window size)
	m_effect.reset(new BasicEffect(m_d3dDevice.Get()));
	m_effect->SetVertexColorEnabled(true);

	void const* shaderByteCode;
	size_t byteCodeLength;

	m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

	DX::ThrowIfFailed(
		m_d3dDevice->CreateInputLayout(VertexPositionColor::InputElements,
		VertexPositionColor::InputElementCount,
		shaderByteCode, byteCodeLength,
		m_inputLayout.ReleaseAndGetAddressOf()));

	m_batch.reset(new PrimitiveBatch<VertexPositionColor>(m_d3dContext.Get()));
	m_font.reset(new SpriteFont(m_d3dDevice.Get(), L"Media/myfile.spritefont"));
	m_spriteBatch.reset(new SpriteBatch(m_d3dContext.Get()));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateResources()
{
	// Clear the previous window size specific context.
	ID3D11RenderTargetView* nullViews[] = { nullptr };
	m_d3dContext->OMSetRenderTargets(_countof(nullViews), nullViews, nullptr);
	m_renderTargetView.Reset();
	m_depthStencilView.Reset();
	m_d3dContext->Flush();

	RECT rc;
	GetWindowRect(m_window, &rc);

	UINT backBufferWidth = std::max<UINT>(rc.right - rc.left, 1);
	UINT backBufferHeight = std::max<UINT>(rc.bottom - rc.top, 1);
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
	DXGI_FORMAT depthBufferFormat = (m_featureLevel >= D3D_FEATURE_LEVEL_10_0) ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_D16_UNORM;

	// If the swap chain already exists, resize it, otherwise create one.
	if (m_swapChain)
	{
		HRESULT hr = m_swapChain->ResizeBuffers(2, backBufferWidth, backBufferHeight, backBufferFormat, 0);

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			// If the device was removed for any reason, a new device and swap chain will need to be created.
			OnDeviceLost();

			// Everything is set up now. Do not continue execution of this method. OnDeviceLost will reenter this method 
			// and correctly set up the new device.
			return;
		}
		else
			DX::ThrowIfFailed(hr);
	}
	else
	{
		// First, retrieve the underlying DXGI Device from the D3D Device
		ComPtr<IDXGIDevice1> dxgiDevice;
		DX::ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));

		// Identify the physical adapter (GPU or card) this device is running on.
		ComPtr<IDXGIAdapter> dxgiAdapter;
		DX::ThrowIfFailed(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

		// And obtain the factory object that created it.
		ComPtr<IDXGIFactory1> dxgiFactory;
		DX::ThrowIfFailed(dxgiAdapter->GetParent(__uuidof(IDXGIFactory1), &dxgiFactory));

		ComPtr<IDXGIFactory2> dxgiFactory2;
		HRESULT hr = dxgiFactory.As(&dxgiFactory2);
		if (SUCCEEDED(hr))
		{
			// DirectX 11.1 or later
			m_d3dDevice.As(&m_d3dDevice1);
			m_d3dContext.As(&m_d3dContext1);

			// Create a descriptor for the swap chain.
			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
			swapChainDesc.Width = backBufferWidth;
			swapChainDesc.Height = backBufferHeight;
			swapChainDesc.Format = backBufferFormat;
			swapChainDesc.SampleDesc.Count = 8;
			swapChainDesc.SampleDesc.Quality = 0;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.BufferCount = 2;

			DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = { 0 };
			fsSwapChainDesc.Windowed = TRUE;

			// Create a SwapChain from a CoreWindow.
			DX::ThrowIfFailed(dxgiFactory2->CreateSwapChainForHwnd(
				m_d3dDevice.Get(), m_window, &swapChainDesc,
				&fsSwapChainDesc,
				nullptr, m_swapChain1.ReleaseAndGetAddressOf()));

			m_swapChain1.As(&m_swapChain);
		}
		else
		{
			DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
			swapChainDesc.BufferCount = 2;
			swapChainDesc.BufferDesc.Width = backBufferWidth;
			swapChainDesc.BufferDesc.Height = backBufferHeight;
			swapChainDesc.BufferDesc.Format = backBufferFormat;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.OutputWindow = m_window;
			swapChainDesc.SampleDesc.Count = 8;
			swapChainDesc.SampleDesc.Quality = 0;
			swapChainDesc.Windowed = TRUE;

			DX::ThrowIfFailed(dxgiFactory->CreateSwapChain(m_d3dDevice.Get(), &swapChainDesc, m_swapChain.ReleaseAndGetAddressOf()));
		}

		// This template does not support 'full-screen' mode and prevents the ALT+ENTER shortcut from working
		dxgiFactory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER);
	}

	// Allocate a 2-D surface as the depth/stencil buffer and
	// create a DepthStencil view on this surface to use on bind.
	CD3D11_TEXTURE2D_DESC depthStencilDesc(depthBufferFormat, backBufferWidth, backBufferHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL);
	depthStencilDesc.Width = backBufferWidth;
	depthStencilDesc.Height = backBufferHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = depthBufferFormat;
	depthStencilDesc.SampleDesc.Count = 8;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	// Obtain the backbuffer for this window which will be the final 3D rendertarget.
	ComPtr<ID3D11Texture2D> backBuffer;
	DX::ThrowIfFailed(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer));

	// Create a view interface on the rendertarget to use on bind.
	DX::ThrowIfFailed(m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.ReleaseAndGetAddressOf()));

	ComPtr<ID3D11Texture2D> depthStencil;
	DX::ThrowIfFailed(m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, depthStencil.GetAddressOf()));

	CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2DMS);
	DX::ThrowIfFailed(m_d3dDevice->CreateDepthStencilView(depthStencil.Get(), &depthStencilViewDesc, m_depthStencilView.ReleaseAndGetAddressOf()));

	// Create a viewport descriptor of the full window size.
	CD3D11_VIEWPORT viewPort(0.0f, 0.0f, static_cast<float>(backBufferWidth), static_cast<float>(backBufferHeight));

	// Set the current viewport using the descriptor.
	m_d3dContext->RSSetViewports(1, &viewPort);

	// TODO: Initialize windows-size dependent objects here
	Matrix proj = Matrix::CreateScale(2.0f / viewPort.Width, -2.0f / viewPort.Height, 1.0f)
		* Matrix::CreateTranslation(-1.0f, 1.0f, 0.0f);

	m_effect->SetProjection(proj);
	m_fontPos.x = backBufferWidth / 2.0f;
	m_fontPos.y = backBufferHeight / 2.0f;
}

void Game::OnDeviceLost()
{
	m_depthStencil.Reset();
	m_depthStencilView.Reset();
	m_renderTargetView.Reset();
	m_swapChain1.Reset();
	m_swapChain.Reset();
	m_d3dContext1.Reset();
	m_d3dContext.Reset();
	m_d3dDevice1.Reset();
	m_d3dDevice.Reset();
	m_effect.reset();
	m_batch.reset();
	m_inputLayout.Reset();
	m_font.reset();
	m_spriteBatch.reset();

	CreateDevice();
	CreateResources();
}