#pragma once

using namespace DirectX::SimpleMath;

#define MAX_BUTTONS 13

// Define button
// Add button to array
// Define button tag
// Add functionality in Render()
// Add functionality in Main()

const static Vector2 OptionsButton[] =
{
	{ 280.0f + 50.0f, 330.0f - 65.0f },
	{ 280.0f + 150.0f, 330.0f },
	{ 280.0f - 120.0f, 330.0f },
	{ 280.0f - 120.0f, 330.0f - 65.0f }
};

const static Vector2 StartButton[] =
{
	{ 590.0f, 330.0f - 65.0f },
	{ 590.0f, 330.0f },
	{ 590.0f - 150.0f, 330.0f },
	{ 590.0f - 250.0f, 330.0f - 65.0f }
};

const static Vector2 CrazyButton[] =
{
	{ 720.0f + 120.0f, 330.0f - 65.0f },
	{ 720.0f + 120.0f, 330.0f },
	{ 720.0f - 120.0f, 330.0f },
	{ 720.0f - 120.0f, 330.0f - 65.0f }
};

const static Vector2 SoundButton[] =
{
	{ 10.0f + 10.0f, 20.0f - 20.0f },
	{ 10.0f + 10.0f, 20.0f },
	{ 10.0f - 10.0f, 20.0f },
	{ 10.0f - 10.0f, 20.0f - 20.0f }
};

const static Vector2 ShapeSizeUpButton[] =
{
	{ 500.0f + 10.0f, 400.0f - 20.0f },
	{ 500.0f + 10.0f, 400.0f },
	{ 500.0f - 10.0f, 400.0f },
	{ 500.0f - 10.0f, 400.0f - 20.0f }
};

const static Vector2 ShapeSizeDownButton[] =
{
	{ 550.0f + 10.0f, 400.0f - 20.0f },
	{ 550.0f + 10.0f, 400.0f },
	{ 550.0f - 10.0f, 400.0f },
	{ 550.0f - 10.0f, 400.0f - 20.0f }
};

const static Vector2 GameTimeUpButton[] =
{
	{ 500.0f + 10.0f, 360.0f - 20.0f },
	{ 500.0f + 10.0f, 360.0f },
	{ 500.0f - 10.0f, 360.0f },
	{ 500.0f - 10.0f, 360.0f - 20.0f }
};

const static Vector2 GameTimeDownButton[] =
{
	{ 550.0f + 10.0f, 360.0f - 20.0f },
	{ 550.0f + 10.0f, 360.0f },
	{ 550.0f - 10.0f, 360.0f },
	{ 550.0f - 10.0f, 360.0f - 20.0f }
};

const static Vector2 EditorButton[] =
{
	{ 720.0f + 120.0f, 405.0f - 65.0f },
	{ 720.0f + 120.0f, 405.0f },
	{ 720.0f - 120.0f, 405.0f },
	{ 720.0f - 120.0f, 405.0f - 65.0f }
};

const static Vector2 UseOwnShapeButton[] =
{
	{ 720.0f + 120.0f, 405.0f - 65.0f },
	{ 720.0f + 120.0f, 405.0f },
	{ 720.0f - 120.0f, 405.0f },
	{ 720.0f - 120.0f, 405.0f - 65.0f }
};

const static Vector2 UseGravityButton[] =
{
	{ 720.0f + 120.0f, 480.0f - 65.0f },
	{ 720.0f + 120.0f, 480.0f },
	{ 720.0f - 120.0f, 480.0f },
	{ 720.0f - 120.0f, 480.0f - 65.0f }
};

const static Vector2 EpilepticModeButton[] =
{
	{ 280.0f + 120.0f, 405.0f - 65.0f },
	{ 280.0f + 120.0f, 405.0f },
	{ 280.0f - 120.0f, 405.0f },
	{ 280.0f - 120.0f, 405.0f - 65.0f }
};

/*
0 = Null
1 = OptionsButton
2 = startButton
3 = CrazyButton
4 = SoundButton
5 = ShapeSizeUpButton
6 = ShapeSizeDownButton
7 = GameTimeUpButton
8 = GameTimeDown
9 = EditorButton
10 = UseOwnShapeButton
11 = UseGravityButton
12 = EpilepticModeButton
13 = Max
*/

const static Vector2 ButtonArray[MAX_BUTTONS][4] =
{
	{ Vector2(NULL, NULL), Vector2(NULL, NULL), Vector2(NULL, NULL), Vector2(NULL, NULL) },
	{ OptionsButton[0], OptionsButton[1], OptionsButton[2], OptionsButton[3] },
	{ StartButton[0], StartButton[1], StartButton[2], StartButton[3] },
	{ CrazyButton[0], CrazyButton[1], CrazyButton[2], CrazyButton[3] },
	{ SoundButton[0], SoundButton[1], SoundButton[2], SoundButton[3] },
	{ ShapeSizeUpButton[0], ShapeSizeUpButton[1], ShapeSizeUpButton[2], ShapeSizeUpButton[3] },
	{ ShapeSizeDownButton[0], ShapeSizeDownButton[1], ShapeSizeDownButton[2], ShapeSizeDownButton[3] },
	{ GameTimeUpButton[0], GameTimeUpButton[1], GameTimeUpButton[2], GameTimeUpButton[3] },
	{ GameTimeDownButton[0], GameTimeDownButton[1], GameTimeDownButton[2], GameTimeDownButton[3] },
	{ EditorButton[0], EditorButton[1], EditorButton[2], EditorButton[3] },
	{ UseOwnShapeButton[0], UseOwnShapeButton[1], UseOwnShapeButton[2], UseOwnShapeButton[3] },
	{ UseGravityButton[0], UseGravityButton[1], UseGravityButton[2], UseGravityButton[3] },
	{ EpilepticModeButton[0], EpilepticModeButton[1], EpilepticModeButton[2], EpilepticModeButton[3] },
};