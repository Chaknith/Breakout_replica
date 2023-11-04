# Breakout_replica

The breakout game has been done by following the [learnopengl.com](https://learnopengl.com/In-Practice/2D-Game/Breakout)

## Screenshots of the game:
<img width="800" alt="2023-11-04_15-19" src="https://github.com/Chaknith/Breakout_replica/assets/101816109/0b03116d-a648-4d9e-ab0d-a8f2ed3a711d">

## Customize Level:
Inside the 'levels' folder, you can edit or add new text files consisting of numbers. Here's what each number represents:
* 0: Empty space
* 1: Solid block
* 2, 3, 4, 5: Destroyable blocks

## Special Feature:
I have implemented a special feature that allows the power-up that extends the player's pad to remain activated when the player loses. This ensures that the player can eventually win, even if the level is super hard. The power-up will only reset when the player wins or changes levels.

## Disclaimer:
There is a bug with my collision system. If the ball hits the corner of the block at a specific angle, the rebound angle of the ball is slightly off. I'm not sure how to solve it yet. :D
