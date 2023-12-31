/*******************************************************************
** This code is part of Breakout.
**
** Breakout is free software: you can redistribute it and/or modify
** it under the terms of the CC BY 4.0 license as published by
** Creative Commons, either version 4 of the License, or (at your
** option) any later version.
******************************************************************/
#ifndef GAME_H
#define GAME_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <vector>

#include "game_level.h"
#include "ball_object.h"
#include "power_up.h"

// Represents the current state of the game
enum GameState {
	GAME_ACTIVE,
	GAME_MENU,
	GAME_WIN
};

enum Direction {
	UP,     // 0 
	RIGHT,  // 1
	DOWN,   // 2
	LEFT    // 3
};

struct Collision {
	bool collided;
	Direction direction;
	glm::vec2 vector;
};

// Game holds all game-related state and funtionality;
// combines all game-related data into a single class for
// easy access to each of the components and manageability.
class Game {
public:
	unsigned int Lives;
	std::vector<PowerUp> PowerUps;
	// game levels
	std::vector<GameLevel> Levels;
	unsigned int Level;
	// game state
	GameState State;
	bool Keys[1024];
	bool KeysProcessed[1024];
	unsigned int Width, Height;
	// constructor/destructor
	Game(unsigned int width, unsigned int height);
	~Game();
	// initialize game state (load all shaders/textures/levels)
	void Init();
	// game loop
	void ProcessInput(float dt);
	void Update(float dt);
	void Render();
	// check collisions
	void DoCollisions();
	void SpawnPowerUps(GameObject& block);
	void UpdatePowerUps(float dt);
private:
	void ResetLevel();
	void ResetPlayer();
	void ResetPowerUp();
};

#endif // !GAME_H
