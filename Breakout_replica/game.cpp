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
#include "ball_object.h"
#include <iostream>

SpriteRenderer* Renderer;

// Initial size of the player paddle
const glm::vec2 PLAYER_SIZE(100.0f, 20.0f);
// Initial velocity of the player paddle
const float PLAYER_VELOCITY(500.0f);
// player paddle
GameObject* Player;

// Initial velocity of the Ball
const glm::vec2 INITIAL_BALL_VELOCITY(100.0f, -350.0f);
// Radius of the ball object
const float BALL_RADIUS = 12.5f;
// Ball
BallObject* Ball;

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
	ResourceManager::LoadTexture("resources/textures/awesomeface.png", true, "ball");
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
	// load ball 
	glm::vec2 ballPos = glm::vec2(this->Width / 2.0f - BALL_RADIUS, this->Height - PLAYER_SIZE.y - BALL_RADIUS * 2);
	Ball = new BallObject(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY, ResourceManager::GetTexture("ball"));
}

void Game::Update(float dt) {
	// update objects
	Ball->Move(dt, this->Width);
	// check for collisions
	this->DoCollisions();
	// ball hit the bottom edge
	if(Ball->Position.y >= this->Height){
		this->ResetLevel();
		this->ResetPlayer();
	}
}

void Game::ProcessInput(float dt) {
	if (this->State == GAME_ACTIVE) {
		float velocity = PLAYER_VELOCITY * dt;
		// move playerboard
		if (this->Keys[GLFW_KEY_A]) {
			if (Player->Position.x > 0.0f) {
				Player->Position.x -= velocity;
				if (Ball->Stuck)
					Ball->Position.x -= velocity;
			}
		}
		if (this->Keys[GLFW_KEY_D]) {
			if (Player->Position.x < this->Width - Player->Size.x) {
				Player->Position.x += velocity;
				if (Ball->Stuck)
					Ball->Position.x += velocity;
			}
		}
		if (this->Keys[GLFW_KEY_SPACE]) {
			Ball->Stuck = false;
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
		// draw ball
		Ball->Draw(*Renderer);
	}
}

void Game::DoCollisions() {
	// ball collides with brick
	for (GameObject& tile : this->Levels[this->Level].Bricks) {
		if (!tile.Destroyed) {
			Collision collision = checkCollision(*Ball, tile);
			if (collision.collided) {
				if (collision.direction == LEFT || collision.direction == RIGHT) {
					// change the ball direction
					Ball->Velocity.x *= -1;
					// reposition the ball
					float penetrationValue = Ball->Radius - std::abs(collision.vector.x);
					if (collision.direction == LEFT)
						Ball->Position.x -= penetrationValue;
					else
						Ball->Position.x += penetrationValue;
				}
				else if (collision.direction == UP || collision.direction == DOWN) {
					// change the ball direction
					Ball->Velocity.y *= -1;
					// reposition the ball
					float penetrationValue = Ball->Radius - std::abs(collision.vector.y);
					if (collision.direction == UP)
						Ball->Position.y -= penetrationValue;
					else
						Ball->Position.y += penetrationValue;
				}

				if (!tile.IsSolid)
					tile.Destroyed = true;
			}
		}
	}
	
	// ball collides with player
	Collision collision = checkCollision(*Ball, *Player);
	if (!Ball->Stuck && collision.collided) {
		// reposition the ball
		float penetrationValue = Ball->Radius - std::abs(collision.vector.y);
		Ball->Position.y -= penetrationValue;
		// redirect the ball
		float playCenter = Player->Position.x + Player->Size.x / 2;
		float ballCenter = Ball->Position.x + Ball->Radius;
		// how far the ball from the center of the player
		float distance = ballCenter - playCenter;

		float percentage = distance / (Player->Size.x / 2.0f);
		
		// then move accordingly
		float strength = 2.0f;
		glm::vec2 oldVelocity = Ball->Velocity;
		Ball->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
		Ball->Velocity.y *= -1;
		Ball->Velocity = glm::normalize(Ball->Velocity) * glm::length(oldVelocity);

	}
}

Collision Game::checkCollision(BallObject &ball, GameObject &brick) {
	Collision collision;
	
	// get ball's center
	glm::vec2 ballCenter(ball.Position + ball.Radius);
	// get brick's center
	glm::vec2 aabb_half_extents(brick.Size.x / 2.0f, brick.Size.y / 2.0f);
	glm::vec2 brickCenter(brick.Position + aabb_half_extents);

	// vector pointing from brick's center to ball's center
	glm::vec2 vector = ballCenter - brickCenter;
	// closest point to the ball
	glm::vec2 closestPoint = brickCenter + glm::clamp(vector, -aabb_half_extents, aabb_half_extents);
	
	// distance from the closest point to the ball's center
	float distance = glm::distance(closestPoint, ballCenter);

	collision.collided = distance <= ball.Size.x / 2.0f;

	// calculate collision direction
	collision.direction = VectorDirection(closestPoint - ballCenter);
	collision.vector = closestPoint - ballCenter;

	return collision;
}

Direction Game::VectorDirection(glm::vec2 target){
	glm::vec2 compass[] = {
		glm::vec2(0.0f, 1.0f),	// up
		glm::vec2(1.0f, 0.0f),	// right
		glm::vec2(0.0f, -1.0f),	// down
		glm::vec2(-1.0f, 0.0f)	// left
	};

	float highestValue = 0.0f;
	int highestIndex = 0;

	for (int i = 0; i < 4; i++) {
		float newDotProduct = glm::dot(glm::normalize(target), compass[i]);
		if (newDotProduct > highestValue) {
			highestValue = newDotProduct;
			highestIndex = i;
		}
	}

	return (Direction)highestIndex;
}

void Game::ResetLevel() {
	// redraw the level
	for (GameObject& tile : this->Levels[this->Level].Bricks) {
		tile.Destroyed = false;
	}
}

void Game::ResetPlayer() {
	// reset the ball
	Ball->Stuck = true;
	Ball->Position = glm::vec2(this->Width / 2.0f - BALL_RADIUS, this->Height - PLAYER_SIZE.y - BALL_RADIUS * 2);
	Ball->Velocity = glm::vec2(100.0f, -350.0f);
	// reset the player
	Player->Position = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
}