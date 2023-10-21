/*******************************************************************
** This code is part of Breakout.
**
** Breakout is free software: you can redistribute it and/or modify
** it under the terms of the CC BY 4.0 license as published by
** Creative Commons, either version 4 of the License, or (at your
** option) any later version.
******************************************************************/
#ifndef GAMELEVEL
#define GAMELEVEL

#include <vector>

#include "game_object.h"

class GameLevel {
public:
	// level state
	std::vector<GameObject> Bricks;
	// contructor
	GameLevel(){}
	// loads level from file
	void Load(const char* file, unsigned int levelWidth, unsigned int levelHeight);
	// render level
	void Draw(SpriteRenderer& renderer);
	// check if the level is completed (all non-solid tiles are destroyed
	bool IsCompleted();
private:
	// initialize level from tile data
	void init(std::vector<std::vector<unsigned int>> tileData, unsigned int levelWidth, unsigned int levelHeight);
};

#endif // !GAMELEVEL
