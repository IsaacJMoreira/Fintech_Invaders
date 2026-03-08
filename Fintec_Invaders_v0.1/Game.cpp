#include "Game.h"

void Game::begin(fabgl::Canvas & canvas)
{
  playerX = SCREEN_WIDTH / 2;
  playerY = SCREEN_HEIGHT - 100;

  bulletActive = false;

  enemyDirection = 1;

  spawnEnemies();
}

void Game::spawnEnemies()
{
  int index = 0;

  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 8; x++) {

      enemies[index].x = 200 + x * 100;
      enemies[index].y = 100 + y * 80;
      enemies[index].alive = true;

      index++;
    }
  }
}

void Game::update()
{
  // mover inimigos
  for (int i = 0; i < MAX_ENEMIES; i++) {

    if (!enemies[i].alive)
      continue;

    enemies[i].x += enemyDirection * ENEMY_SPEED;

    if (enemies[i].x > SCREEN_WIDTH - 100 || enemies[i].x < 100) {

      enemyDirection *= -1;

      for (int j = 0; j < MAX_ENEMIES; j++)
        enemies[j].y += 20;

      break;
    }
  }

  // bala
  if (bulletActive) {

    bulletY -= BULLET_SPEED;

    if (bulletY < 0)
      bulletActive = false;
  }

  // colisão
  if (bulletActive) {

    for (int i = 0; i < MAX_ENEMIES; i++) {

      if (!enemies[i].alive)
        continue;

      if (bulletX > enemies[i].x &&
          bulletX < enemies[i].x + 64 &&
          bulletY > enemies[i].y &&
          bulletY < enemies[i].y + 64) {

        enemies[i].alive = false;
        bulletActive = false;
      }
    }
  }
}

void Game::render(fabgl::Canvas & canvas)
{
  canvas.setBrushColor(fabgl::Color::Black);
  canvas.clear();

  // draw player
  canvas.setBrushColor(fabgl::Color::Green);
  canvas.fillRectangle(playerX, playerY, playerX + 64, playerY + 32);

  // bullet
  if (bulletActive) {
    canvas.setBrushColor(fabgl::Color::Yellow);
    canvas.fillRectangle(bulletX, bulletY, bulletX + 6, bulletY + 20);
  }

  // enemies
  canvas.setBrushColor(fabgl::Color::Red);

  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].alive)
      continue;

    canvas.fillRectangle(
      enemies[i].x,
      enemies[i].y,
      enemies[i].x + 64,
      enemies[i].y + 32
    );
  }
}