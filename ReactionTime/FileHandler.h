#pragma once

#include <fstream>
#include <limits>

#define GAME_WIDTH 1000
#define GAME_HEIGHT 600
#define CONFIG_FILE "config.txt"
#define GAME_TIME_MAX 30
#define SHAPE_SIZE_MAX GAME_HEIGHT
#define DEFAULT_GAME_TIME 5
#define DEFAULT_SHAPE_SIZE 200

class FileHandler
{
public:
	bool FileExists(const std::string& filename);
	void WriteConfig(int credits, int shapeSize, int gameTime);
	int ReadCreditsFromFile();
	int ReadShapeSizeFromFile();
	int ReadGameTimeFromFile();
	std::fstream& GotoLine(std::fstream& file, unsigned int num);
};