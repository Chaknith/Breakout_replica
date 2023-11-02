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
#include "particle_generator.h"
#include "post_processor.h"
#include "text_renderer.h"

#include <iostream>
#include <sstream>
#include <filesystem>
#include <irrKlang.h>

using namespace irrklang;
ISoundEngine* SoundEngine = createIrrKlangDevice();

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

struct InitialValue {
	glm::vec2 playerSize = PLAYER_SIZE;
	glm::vec2 ballVelocity = INITIAL_BALL_VELOCITY;
};

struct InitialValue initialValue;
// Ball
BallObject* Ball;
PostProcessor* Effects;
TextRenderer* Text;
ParticleGenerator* Particles;
float ShakeTime = 0.0f;

Game::Game(unsigned int width, unsigned int height)
	: State(GAME_MENU), Keys(), Width(width), Height(height) {
}

Game::~Game() {
	delete Renderer;
	delete Player;
	delete Ball;
	delete Particles;
	delete Effects;
}

void Game::Init() {
	// load shaders
	ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.frag", nullptr, "sprite");
	ResourceManager::LoadShader("shaders/particle.vs", "shaders/particle.frag", nullptr, "particle");
	ResourceManager::LoadShader("shaders/post_processing.vs", "shaders/post_processing.frag", nullptr, "postprocessing");
	// configure shaders
	glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(this->Width), static_cast<float>(this->Height), 0.0f, -1.0f, 1.0f);
	ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
	ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);
	ResourceManager::GetShader("particle").Use().SetInteger("sprite", 0);
	ResourceManager::GetShader("particle").SetMatrix4("projection", projection);
	// set render-specific controls
	Shader spriteShader = ResourceManager::GetShader("sprite");
	Renderer = new SpriteRenderer(spriteShader);
	Effects = new PostProcessor(ResourceManager::GetShader("postprocessing"), this->Width, this->Height);
	// load textures
	ResourceManager::LoadTexture("resources/textures/background.jpg", false, "background");
	ResourceManager::LoadTexture("resources/textures/paddle.png", true, "paddle");
	ResourceManager::LoadTexture("resources/textures/block.png", false, "block");
	ResourceManager::LoadTexture("resources/textures/block_solid.png", false, "block_solid");
	ResourceManager::LoadTexture("resources/textures/awesomeface.png", true, "ball");
	ResourceManager::LoadTexture("resources/textures/particle.png", true, "particle");
	ResourceManager::LoadTexture("resources/textures/powerup_speed.png", true, "powerup_speed");
	ResourceManager::LoadTexture("resources/textures/powerup_chaos.png", true, "powerup_chaos");
	ResourceManager::LoadTexture("resources/textures/powerup_confuse.png", true, "powerup_confuse");
	ResourceManager::LoadTexture("resources/textures/powerup_increase.png", true, "powerup_increase");
	ResourceManager::LoadTexture("resources/textures/powerup_passthrough.png", true, "powerup_passthrough");
	ResourceManager::LoadTexture("resources/textures/powerup_sticky.png", true, "powerup_sticky");
	// load levels
	for (const auto& entry : std::filesystem::directory_iterator("levels")) {
		GameLevel level; level.Load(entry.path().string().c_str(), this->Width, this->Height / 2);
		this->Levels.push_back(level);
	}
	this->Level = 0;
	// load player
	glm::vec2 playerPos = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
	Player = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("paddle"));
	// load ball 
	glm::vec2 ballPos = glm::vec2(this->Width / 2.0f - BALL_RADIUS, this->Height - PLAYER_SIZE.y - BALL_RADIUS * 2);
	Ball = new BallObject(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY, ResourceManager::GetTexture("ball"));
	Ball->Sticky = false;
	Ball->PassThrough = false;
	this->Lives = 3;
	// initialize particles
	Particles = new ParticleGenerator(ResourceManager::GetShader("particle"), ResourceManager::GetTexture("particle"), 500);
	// load background sound
	SoundEngine->play2D("resources/audios/breakout.mp3", true);
	// load font
	Text = new TextRenderer(this->Width, this->Height);
	Text->Load("resources/fonts/ocraext.TTF", 24);
}

void Game::Update(float dt) {
	if (this->State == GAME_ACTIVE) {
		// update objects
		Ball->Move(dt, this->Width);
		// check for collisions
		this->DoCollisions();
		// ball hit the bottom edge
		if (Ball->Position.y >= this->Height) {
			--this->Lives;
			// did the player lose all his lives? : Game over
			if (this->Lives == 0)
			{
				this->Lives = 3;
				this->ResetLevel();
				this->State = GAME_MENU;
				Particles->Reset();
			}
			this->ResetPlayer();
		}
		// update particle
		Particles->Update(dt, *Ball, 2, glm::vec2(Ball->Radius / 2.0f));
		// update PowerUps
		this->UpdatePowerUps(dt);
		if (ShakeTime > 0.0f)
		{
			ShakeTime -= dt;
			if (ShakeTime <= 0.0f)
				Effects->Shake = false;
		}

		if (this->Levels[this->Level].IsCompleted())
		{
			Effects->Chaos = true;
			this->State = GAME_WIN;
			this->Lives = 3;
			Player->Size = initialValue.playerSize;
			Player->Velocity = initialValue.ballVelocity;
		}
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
		if (this->Keys[GLFW_KEY_M]) {
			this->State = GAME_MENU;
		}
	}
	else if (this->State == GAME_MENU) {
		SoundEngine->setSoundVolume(0.5f);
		if (this->Keys[GLFW_KEY_ENTER] && !this->KeysProcessed[GLFW_KEY_ENTER]) {
			this->KeysProcessed[GLFW_KEY_ENTER] = true;
			this->State = GAME_ACTIVE;
		}
		if (this->Keys[GLFW_KEY_W] && !this->KeysProcessed[GLFW_KEY_W]) {
			this->KeysProcessed[GLFW_KEY_W] = true;
			this->ResetPlayer();
			Particles->Reset();
			this->Lives = 3;
			this->ResetPowerUp();
			this->Level += 1;
			this->Level = this->Level % this->Levels.size();
		}
		if (this->Keys[GLFW_KEY_S] && !this->KeysProcessed[GLFW_KEY_S]) {
			this->KeysProcessed[GLFW_KEY_S] = true;
			this->ResetPlayer();
			Particles->Reset();
			this->Lives = 3;
			this->ResetPowerUp();
			if(this->Level > 0)
				this->Level -= 1;
			else
				this->Level = this->Levels.size() - 1;
		}
	}
	else if (this->State == GAME_WIN)
	{
		if (this->Keys[GLFW_KEY_ENTER])
		{
			this->ResetPlayer();
			this->ResetLevel();
			this->ResetPowerUp();
			Particles->Reset();
			this->KeysProcessed[GLFW_KEY_ENTER] = true;
			Effects->Chaos = false;
			this->State = GAME_MENU;
		}
	}
}

void Game::Render() {
	Effects->BeginRender();
	// draw background
	Texture2D background = ResourceManager::GetTexture("background");
	Renderer->DrawSprite(background, glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);
	// draw level
	this->Levels[this->Level].Draw(*Renderer);
	// draw player
	Player->Draw(*Renderer);
	// draw PowerUps
	for (PowerUp& powerUp : this->PowerUps)
		if (!powerUp.Destroyed)
			powerUp.Draw(*Renderer);
	// draw particles
	Particles->Draw();
	// draw ball
	Ball->Draw(*Renderer);
	Effects->EndRender();
	Effects->Render(glfwGetTime());
	std::stringstream ss;
	ss << this->Lives;
	Text->RenderText("Lives:" + ss.str(), 5.0f, 5.0f, 1.0f);
	switch (this->State) {
	case GAME_ACTIVE:
		Text->RenderText("Press m for menu", Width - 250, 5.0f, 1.0f);
		break;
	case GAME_MENU:
		Text->RenderText("Press ENTER to start", 250.0f, Height / 2, 1.0f);
		Text->RenderText("Press W or S to select level", 245.0f, Height / 2 + 20.0f, 0.75f);
		break;
	case GAME_WIN:
		Text->RenderText("You WON!!!", 260.0, Height / 2 - 40.0, 2.0, glm::vec3(0.0, 1.0, 0.0));
		Text->RenderText("Press ENTER to retry or ESC to quit", 130.0, Height / 2, 1.0, glm::vec3(1.0, 1.0, 0.0));
		break;
	}
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
	// also disable all active powerups
	Effects->Chaos = Effects->Confuse = false;
	Ball->PassThrough = Ball->Sticky = false;
	Player->Color = glm::vec3(1.0f);
	Ball->Color = glm::vec3(1.0f);
}

void ActivatePowerUp(PowerUp& powerUp)
{
	if (powerUp.Type == "speed")
	{
		Ball->Velocity *= 1.2;
	}
	else if (powerUp.Type == "sticky")
	{
		Ball->Sticky = true;
		Player->Color = glm::vec3(1.0f, 0.5f, 1.0f);
	}
	else if (powerUp.Type == "pass-through")
	{
		Ball->PassThrough = true;
		Ball->Color = glm::vec3(1.0f, 0.5f, 0.5f);
	}
	else if (powerUp.Type == "pad-size-increase")
	{
		Player->Size.x += 50;
	}
	else if (powerUp.Type == "confuse")
	{
		if (!Effects->Chaos)
			Effects->Confuse = true; // only activate if chaos wasn't already active
	}
	else if (powerUp.Type == "chaos")
	{
		if (!Effects->Confuse)
			Effects->Chaos = true;
	}
}

// collision detection
bool CheckCollision(GameObject& one, GameObject& two);
Collision CheckCollision(BallObject& one, GameObject& two);
Direction VectorDirection(glm::vec2 closest);

void Game::DoCollisions() {
	// ball collides with brick
	for (GameObject& tile : this->Levels[this->Level].Bricks) {
		if (!tile.Destroyed)
		{
			Collision collision = CheckCollision(*Ball, tile);
			if (!tile.Destroyed) {
				if (collision.collided) {
					// destroy block if not solid
					if (!tile.IsSolid) {
						tile.Destroyed = true;
						this->SpawnPowerUps(tile);
						SoundEngine->play2D("resources/audios/destroy.wav", false);
					}
					else
					{   // if block is solid, enable shake effect
						ShakeTime = 0.05f;
						Effects->Shake = true;
						SoundEngine->play2D("resources/audios/solid.wav", false);
					}
					if (!(Ball->PassThrough && !tile.IsSolid)) // don't do collision resolution on non-solid bricks if pass-through is activated
					{
						if (collision.direction == LEFT || collision.direction == RIGHT) {
							// change the ball direction
							Ball->Velocity.x *= -1;
							// reposition the ball
							float penetrationValue = Ball->Radius - std::abs(collision.vector.x);
							if (collision.direction == LEFT)
								Ball->Position.x += penetrationValue;
							else
								Ball->Position.x -= penetrationValue;
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
					}
				}
			}
		}
	}
	
	// ball collides with player
	Collision collision = CheckCollision(*Ball, *Player);
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

		// if Sticky powerup is activated, also stick ball to paddle once new velocity vectors were calculated
		Ball->Stuck = Ball->Sticky;
		SoundEngine->play2D("resources/audios/rebounce.wav", false);
	}

	for (PowerUp& powerUp : this->PowerUps)
	{
		if (!powerUp.Destroyed)
		{
			if (powerUp.Position.y >= this->Height)
				powerUp.Destroyed = true;
			if (CheckCollision(*Player, powerUp))
			{	// collided with player, now activate powerup
				ActivatePowerUp(powerUp);
				powerUp.Destroyed = true;
				powerUp.Activated = true;
				SoundEngine->play2D("resources/audios/powerup.wav", false);
			}
		}
	}
}

bool CheckCollision(GameObject& one, GameObject& two) // AABB - AABB collision
{
	// collision x-axis?
	bool collisionX = one.Position.x + one.Size.x >= two.Position.x &&
		two.Position.x + two.Size.x >= one.Position.x;
	// collision y-axis?
	bool collisionY = one.Position.y + one.Size.y >= two.Position.y &&
		two.Position.y + two.Size.y >= one.Position.y;
	// collision only if on both axes
	return collisionX && collisionY;
}

Collision CheckCollision(BallObject &ball, GameObject &brick) {
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

Direction VectorDirection(glm::vec2 target){
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

bool ShouldSpawn(unsigned int chance)
{
	unsigned int random = rand() % chance;
	return random == 0;
}
void Game::SpawnPowerUps(GameObject& block)
{
	if (ShouldSpawn(75)) // 1 in 75 chance
		this->PowerUps.push_back(
			PowerUp("speed", glm::vec3(0.5f, 0.5f, 1.0f), 0.0f, block.Position, ResourceManager::GetTexture("powerup_speed")));
	if (ShouldSpawn(75))
		this->PowerUps.push_back(
			PowerUp("sticky", glm::vec3(1.0f, 0.5f, 1.0f), 20.0f, block.Position, ResourceManager::GetTexture("powerup_sticky")));
	if (ShouldSpawn(75))
		this->PowerUps.push_back(
			PowerUp("pass-through", glm::vec3(0.5f, 1.0f, 0.5f), 10.0f, block.Position, ResourceManager::GetTexture("powerup_passthrough")));
	if (ShouldSpawn(75))
		this->PowerUps.push_back(
			PowerUp("pad-size-increase", glm::vec3(1.0f, 0.6f, 0.4), 0.0f, block.Position, ResourceManager::GetTexture("powerup_increase")));
	if (ShouldSpawn(15)) // negative powerups should spawn more often
		this->PowerUps.push_back(
			PowerUp("confuse", glm::vec3(1.0f, 0.3f, 0.3f), 15.0f, block.Position, ResourceManager::GetTexture("powerup_confuse")));
	if (ShouldSpawn(15))
		this->PowerUps.push_back(
			PowerUp("chaos", glm::vec3(0.9f, 0.25f, 0.25f), 15.0f, block.Position, ResourceManager::GetTexture("powerup_chaos")));
}

bool IsOtherPowerUpActive(std::vector<PowerUp>& powerUps, std::string type)
{
	for (const PowerUp& powerUp : powerUps)
	{
		if (powerUp.Activated)
			if (powerUp.Type == type)
				return true;
	}
	return false;
}

void Game::UpdatePowerUps(float dt)
{
	for (PowerUp& powerUp : this->PowerUps)
	{
		powerUp.Position += powerUp.Velocity * dt;
		if (powerUp.Activated)
		{
			powerUp.Duration -= dt;

			if (powerUp.Duration <= 0.0f)
			{
				// remove powerup from list (will later be removed)
				powerUp.Activated = false;
				// deactivate effects
				if (powerUp.Type == "sticky")
				{
					if (!IsOtherPowerUpActive(this->PowerUps, "sticky"))
					{	// only reset if no other PowerUp of type sticky is active
						Ball->Sticky = false;
						Player->Color = glm::vec3(1.0f);
					}
				}
				else if (powerUp.Type == "pass-through")
				{
					if (!IsOtherPowerUpActive(this->PowerUps, "pass-through"))
					{	// only reset if no other PowerUp of type pass-through is active
						Ball->PassThrough = false;
						Ball->Color = glm::vec3(1.0f);
					}
				}
				else if (powerUp.Type == "confuse")
				{
					if (!IsOtherPowerUpActive(this->PowerUps, "confuse"))
					{	// only reset if no other PowerUp of type confuse is active
						Effects->Confuse = false;
					}
				}
				else if (powerUp.Type == "chaos")
				{
					if (!IsOtherPowerUpActive(this->PowerUps, "chaos"))
					{	// only reset if no other PowerUp of type chaos is active
						Effects->Chaos = false;
					}
				}
			}
		}
	}
	this->PowerUps.erase(std::remove_if(this->PowerUps.begin(), this->PowerUps.end(),
		[](const PowerUp& powerUp) { return powerUp.Destroyed && !powerUp.Activated; }
	), this->PowerUps.end());
}

void Game::ResetPowerUp() {
	this->PowerUps.clear();
}