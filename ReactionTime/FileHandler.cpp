#include "pch.h"
#include "FileHandler.h"
#include <iostream>
#include <fstream>

bool FileHandler::FileExists(const std::string& filename)
{
	std::ifstream file(filename.c_str());
	return file.good();
}

void FileHandler::WriteConfig(int credits, int shapeSize, int gameTime)
{
	std::ofstream myfile(CONFIG_FILE);
	myfile << credits << "\n" << shapeSize << "\n" << gameTime;
}

int FileHandler::ReadCreditsFromFile()
{
	std::fstream myfile(CONFIG_FILE);
	GotoLine(myfile, 1);
	int64_t credits;
	myfile >> credits;
	if (credits > INT_MAX)
		return INT_MAX;
	else if (credits < 0)
		return 0;

	return (int)credits;
}

int FileHandler::ReadShapeSizeFromFile()
{
	std::fstream myfile(CONFIG_FILE);
	GotoLine(myfile, 2);
	int shapeSize;
	myfile >> shapeSize;
	if (shapeSize > SHAPE_SIZE_MAX)
		return SHAPE_SIZE_MAX;
	else if (shapeSize < 1)
		return DEFAULT_SHAPE_SIZE;

	return shapeSize;
}

int FileHandler::ReadGameTimeFromFile()
{
	std::fstream myfile(CONFIG_FILE);
	GotoLine(myfile, 3);
	int gameTime;
	myfile >> gameTime;
	if (gameTime > GAME_TIME_MAX)
		return GAME_TIME_MAX;
	else if (gameTime < 1)
		return DEFAULT_GAME_TIME;

	return gameTime;
}

std::fstream& FileHandler::GotoLine(std::fstream& file, unsigned int num)
{
	file.seekg(std::ios::beg);
	for (UINT8 i = 0; i < num - 1; ++i)
	{
		file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	return file;
}