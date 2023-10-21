/*******************************************************************
** This code is part of Breakout.
**
** Breakout is free software: you can redistribute it and/or modify
** it under the terms of the CC BY 4.0 license as published by
** Creative Commons, either version 4 of the License, or (at your
** option) any later version.
******************************************************************/
#include "game.h"
#include "resource_manager.h"
#include "sprite_renderer.h"

SpriteRenderer* Renderer;

// Initial size of the player paddle
const glm::vec2 PLAYER_SIZE(100.0f, 20.0f);
// Initial velocity of the player paddle
const float PLAYER_VELOCITY(500.0f);

Game::Game(unsigned int width, unsigned int height)
	: State(GAME_ACTIVE), Keys(), Width(width), Height(height) {

}

Game::~Game() {

}

void Game::Init() {
	// load shaders
	ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.frag", nullptr, "sprite");
	// configure shaders
	glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(this->Width), static_cast<float>(this->Height), 0.0f, -1.0f, 1.0f);
	ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
	ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);
	// set render-specific controls
	Shader spriteShader = ResourceManager::GetShader("sprite");
	Renderer = new SpriteRenderer(spriteShader);
	// load textures
	ResourceManager::LoadTexture("resources/textures/background.jpg", false, "background");
	ResourceManager::LoadTexture("resources/textures/paddle.png", true, "paddle");
	ResourceManager::LoadTexture("resources/textures/block.png", false, "block");
	ResourceManager::LoadTexture("resources/textures/block_solid.png", false, "block_solid");
	// load levels
	GameLevel one; one.Load("levels/one.txt", this->Width, this->Height / 2);
	GameLevel two; two.Load("levels/two.txt", this->Width, this->Height / 2);
	GameLevel three; three.Load("levels/three.txt", this->Width, this->Height / 2);
	GameLevel four; four.Load("levels/four.txt", this->Width, this->Height / 2);
	this->Levels.push_back(one);
	this->Levels.push_back(two);
	this->Levels.push_back(three);
	this->Levels.push_back(four);
	this->Level = 0;
	// load player
	glm::vec2 playerPos = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
	Player = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("paddle"));
}

void Game::Update(float dt) {

}

void Game::ProcessInput(float dt) {
	if (this->State == GAME_ACTIVE) {
		float velocity = PLAYER_VELOCITY * dt;
		// move playerboard
		if (this->Keys[GLFW_KEY_A]) {
			if(Player->Position.x > 0.0f)
				Player->Position.x -= velocity;
		}
		if (this->Keys[GLFW_KEY_D]) {
			if (Player->Position.x < this->Width - Player->Size.x)
				Player->Position.x += velocity;
		}
	}
}

void Game::Render() {
	if (this->State == GAME_ACTIVE) {
		// draw background
		Texture2D background = ResourceManager::GetTexture("background");
		Renderer->DrawSprite(background, glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);
		// draw level
		this->Levels[this->Level].Draw(*Renderer);
		// draw player
		Player->Draw(*Renderer);
	}
}