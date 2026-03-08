#pragma once
#include <fabgl.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define PLAYER_SPEED 10
#define BULLET_SPEED 18
#define ENEMY_SPEED 2

#define MAX_ENEMIES 32

struct Enemy {
  int x;
  int y;
  bool alive;
};

class Game {

public:
  void begin(fabgl::Canvas & canvas);
  void update();
  void render(fabgl::Canvas & canvas);

private:

  int playerX;
  int playerY;

  int bulletX;
  int bulletY;
  bool bulletActive;

  Enemy enemies[MAX_ENEMIES];

  int enemyDirection;

  void spawnEnemies();
};