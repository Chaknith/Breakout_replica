/*******************************************************************
** This code is part of Breakout.
**
** Breakout is free software: you can redistribute it and/or modify
** it under the terms of the CC BY 4.0 license as published by
** Creative Commons, either version 4 of the License, or (at your
** option) any later version.
******************************************************************/
#ifndef BALLOBJECT_H
#define BALLOBJECT_H

#include "game_object.h"

class BallObject : public GameObject {
public:
	// ball state
	float Radius;
	bool Stuck;

	BallObject();
	BallObject(glm::vec2 pos, float radius, glm::vec2 velocity, Texture2D sprite);
	glm::vec2 Move(float dt, unsigned int window_width);
	void Reset(glm::vec2 position, glm::vec2 velocity);
};

#endif // !BALLOBJECT_H
