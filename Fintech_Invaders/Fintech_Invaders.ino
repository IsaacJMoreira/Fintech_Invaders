#include "fabgl.h"
#include "sprites.h"

using fabgl::iclamp;

#define PLAY_AREA_LEFT 79
#define PLAY_AREA_RIGHT 239
#define PLAYER_AREA_LANES 5

#define STAR0_COUNT 10
#define STAR1_COUNT 7
#define STAR2_COUNT 3

#define ASTEROID_COUNT 5
#define EXPLOSION_FRAMES 4

float     STAR_LAYER3_SPEED  = 0;
#define STAR_LAYER0_SPEED (STAR_LAYER1_SPEED * 2)
#define STAR_LAYER1_SPEED (STAR_LAYER2_SPEED * 2)
#define STAR_LAYER2_SPEED (STAR_LAYER3_SPEED * 2)


#define PLAYER_FIRE_SPEED 4

enum GameState {
  INTRO_SCREEN,
  IN_GAME,
  GAME_OVER,
  INPUT_NAME_HI_SCORE
};

GameState gameState = INTRO_SCREEN;

fabgl::VGAController DisplayController;
fabgl::PS2Controller PS2Controller;
fabgl::Canvas canvas(&DisplayController);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// INTRO SCENE //////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct IntroScene : public Scene {

  IntroScene()
    : Scene(0, 20, 320, 200) {}

  void init() override {

    canvas.clear();
    canvas.drawBitmap(0, 0, &FINTECH_INVADERS);
  }

  void update(int updateCount) override {

    auto keyboard = PS2Controller.keyboard();

    if (updateCount < 20)
      return;

    if (keyboard && keyboard->isVKDown(fabgl::VK_SPACE)) {

      //canvas.setPenColor(Color::Red);
      //canvas.drawText(110, 180, "ESPACO PRESSIONADO");
      //DisplayController.removeSprites();   // <-- REQUIRED
      gameState = IN_GAME;

      stop();
    }
  }

  void collisionDetected(Sprite*, Sprite*, Point) override {}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// GAME SCENE ///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum SpriteType {
  TYPE_NONE = 0,
  TYPE_PLAYER,
  TYPE_PLAYER_FIRE,
  TYPE_ASTEROID
};

struct GameScene : public Scene {

  static const int SPRITESCOUNT = 35;

  fabgl::Sprite sprites[SPRITESCOUNT];
  SpriteType spriteType[SPRITESCOUNT];

  fabgl::Sprite* player = &sprites[20];
  fabgl::Sprite* afterburner = &sprites[21];
  fabgl::Sprite* playerFire = &sprites[22];

  fabgl::Sprite* asteroids[ASTEROID_COUNT] = {
    &sprites[23],
    &sprites[24],
    &sprites[25],
    &sprites[26],
    &sprites[27]
  };

  bool asteroidActive[ASTEROID_COUNT];
  int asteroidSpeed[ASTEROID_COUNT];

  bool asteroidExploding[ASTEROID_COUNT];
  int asteroidExplosionFrame[ASTEROID_COUNT];
  int asteroidExplosionTimer[ASTEROID_COUNT];

  bool playerExploding = false;
  int playerExplosionFrame = 0;
  int playerExplosionTimer = 0;

  int currentUpdateCount = 0;

  float starY[STAR0_COUNT + STAR1_COUNT + STAR2_COUNT];

  int playerVelX = 0;

  bool leftHeld = false;
  bool rightHeld = false;

  int lastAnimUpdate = 0;

  bool playerFired = false;
  int playerFireSpeed = 0;
  int playerFire_x = 0;
  int playerFire_y = 0;

  int gameOverTime = 0;
  bool gameOver = false;

  GameScene()
    : Scene(SPRITESCOUNT, 20, 320, 200) {}

  //////////////////////////////////////////////////////////////////////
  ////////////////////////// STARFIELD //////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  void generateFarStars() {

    int width = PLAY_AREA_RIGHT - PLAY_AREA_LEFT - 1;
    int height = DisplayController.getViewPortHeight();

    int dots = width * height * 0.05;

    canvas.drawBitmap(PLAY_AREA_LEFT + 1, 0, &STARFIELD);

    for (int i = 0; i < dots; i++) {

      int color = random(0, 3);

      canvas.setPenColor(color ? (color == 1 ? Color::White : Color::BrightBlack) : Color::BrightBlack);

      int x = random(PLAY_AREA_LEFT + 1, PLAY_AREA_RIGHT - 1);
      int y = random(0, height);

      canvas.setPixel(x, y);
    }
  }

  void initStars() {

    int index = 0;

    for (int i = 0; i < STAR0_COUNT; i++) {

      sprites[index].addBitmap(&starBMP0);

      sprites[index].moveTo(random(PLAY_AREA_LEFT + 1, PLAY_AREA_RIGHT - 3),
                            random(0, DisplayController.getViewPortHeight()));

      starY[index] = sprites[index].y;

      addSprite(&sprites[index]);

      index++;
    }

    for (int i = 0; i < STAR1_COUNT; i++) {

      sprites[index].addBitmap(&starBMP1);

      sprites[index].moveTo(random(PLAY_AREA_LEFT + 1, PLAY_AREA_RIGHT - 5),
                            random(0, DisplayController.getViewPortHeight()));

      starY[index] = sprites[index].y;

      addSprite(&sprites[index]);

      index++;
    }

    for (int i = 0; i < STAR2_COUNT; i++) {

      sprites[index].addBitmap(&starBMP2);

      sprites[index].moveTo(random(PLAY_AREA_LEFT + 1, PLAY_AREA_RIGHT - 7),
                            random(0, DisplayController.getViewPortHeight()));

      starY[index] = sprites[index].y;

      addSprite(&sprites[index]);

      index++;
    }
    DisplayController.refreshSprites();
  }

  void updateStars() {

    int totalStars = STAR0_COUNT + STAR1_COUNT + STAR2_COUNT;

    for (int i = 0; i < totalStars; i++) {

      float speed;

      if (i < 5) speed = STAR_LAYER3_SPEED;
      else if (i < 10) speed = STAR_LAYER2_SPEED;
      else if (i < 15) speed = STAR_LAYER1_SPEED;
      else speed = STAR_LAYER0_SPEED;

      starY[i] += speed;

      sprites[i].y = (int)starY[i];

      if (sprites[i].y >= DisplayController.getViewPortHeight()) {

        starY[i] = 0;

        sprites[i].y = 0;

        sprites[i].x = random(PLAY_AREA_LEFT + 1,
                              PLAY_AREA_RIGHT - sprites[i].getWidth());
      }

      updateSprite(&sprites[i]);
    }
  }

  //////////////////////////////////////////////////////////////////////
  ////////////////////////// GAME INIT //////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  void init() override {

    STAR_LAYER3_SPEED = 1;

    for (int i = 0; i < SPRITESCOUNT; i++)
      spriteType[i] = TYPE_NONE;

    canvas.clear();

    for (int i = 0; i < SPRITESCOUNT; i++) {
      sprites[i].clearBitmaps();
      sprites[i].visible = true;
    }

    spriteType[20] = TYPE_PLAYER;
    spriteType[22] = TYPE_PLAYER_FIRE;

    player->addBitmap(&bitmap1);
    player->addBitmap(&bitmap2);
    player->addBitmap(&bitmap3);
    player->addBitmap(&bitmap4);
    player->addBitmap(&bitmap5);

    player->addBitmap(&EXPLOSION_0);
    player->addBitmap(&EXPLOSION_1);
    player->addBitmap(&EXPLOSION_2);
    player->addBitmap(&EXPLOSION_3);

    afterburner->addBitmap(&afterburner_0);
    afterburner->addBitmap(&afterburner_1);
    afterburner->addBitmap(&afterburner_2);
    afterburner->addBitmap(&afterburner_3);

    playerFire->addBitmap(&PLAYER_FIRE);
    playerFire->visible = false;

    for (int i = 0; i < ASTEROID_COUNT; i++) {

      asteroids[i]->addBitmap(&ASTEROID);
      asteroids[i]->addBitmap(&EXPLOSION_0);
      asteroids[i]->addBitmap(&EXPLOSION_1);
      asteroids[i]->addBitmap(&EXPLOSION_2);
      asteroids[i]->addBitmap(&EXPLOSION_3);

      asteroids[i]->moveTo(-100, -100);

      asteroidActive[i] = false;

      asteroidExploding[i] = false;

      asteroidExplosionFrame[i] = 0;

      asteroidSpeed[i] = random(1, 3);

      addSprite(asteroids[i]);

      spriteType[23 + i] = TYPE_ASTEROID;
    }

    canvas.setBrushColor(Color::White);

    canvas.fillRectangle(0, 0, PLAY_AREA_LEFT, DisplayController.getViewPortHeight());

    canvas.fillRectangle(PLAY_AREA_RIGHT, 0,
                         DisplayController.getViewPortWidth(),
                         DisplayController.getViewPortHeight());

    canvas.drawBitmap(1, 1, &CAIXA);

    generateFarStars();

    player->moveTo(152, 170);
    afterburner->moveTo(156, 184);
    playerFire->moveTo(154, 167);

    initStars();

    addSprite(player);
    addSprite(afterburner);
    addSprite(playerFire);

    DisplayController.setSprites(sprites, SPRITESCOUNT);

    player->setFrame(2);
  }

  //////////////////////////////////////////////////////////////////////
  ////////////////////////// UPDATE /////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  void update(int updateCount) override {

    currentUpdateCount = updateCount;

    auto keyboard = PS2Controller.keyboard();

    if (gameOver) {

      canvas.setPenColor(Color::Red);
      canvas.drawText(124, 100, "GAME OVER");

      if (keyboard && keyboard->isKeyboardAvailable() && keyboard->isVKDown(fabgl::VK_SPACE)) {

        DisplayController.removeSprites();  // IMPORTANT

        gameState = INTRO_SCREEN;

        stop();
      }

      return;
    }

    ////////////////////////////////////////////////////////
    ////////////////// PLAYER INPUT ////////////////////////

    playerVelX = 0;

    bool leftPressed = false;
    bool rightPressed = false;

    if (keyboard && keyboard->isKeyboardAvailable()) {

      if (keyboard->isVKDown(fabgl::VK_LEFT)) {

        playerVelX = -2;
        leftPressed = true;
      } else if (keyboard->isVKDown(fabgl::VK_RIGHT)) {

        playerVelX = 2;
        rightPressed = true;
      }

      if (keyboard->isVKDown(fabgl::VK_SPACE) && !playerFired) {

        playerFire->visible = true;

        playerFired = true;

        playerFireSpeed = PLAYER_FIRE_SPEED;

        playerFire_x = player->x + 2;

        playerFire_y = player->y - 3;

        playerFire->moveTo(playerFire_x, playerFire_y);
      }
    }

    // ---------- PLAYER EXPLOSION ----------
    if (playerExploding) {

      afterburner->visible = false;

      if (currentUpdateCount - playerExplosionTimer > 2) {

        playerExplosionTimer = currentUpdateCount;

        playerExplosionFrame++;

        if (playerExplosionFrame < EXPLOSION_FRAMES) {

          player->setFrame(playerExplosionFrame + 5);

        } else {

          playerExploding = false;

          player->visible = false;

          gameOver = true;

          canvas.setPenColor(Color::White);
          canvas.drawText(124, 100, "GAME OVER");
        }
      }
    }

    player->x += playerVelX;

    player->x = iclamp(player->x,
                       PLAY_AREA_LEFT + 1,
                       PLAY_AREA_RIGHT - player->getWidth());

    // ---------- PLAYER ANIMATION ----------
    if (rightPressed) {
      if (!rightHeld) {
        rightHeld = true;
        lastAnimUpdate = updateCount;
      }

      if (updateCount - lastAnimUpdate >= 4) {
        lastAnimUpdate = updateCount;

        int frame = player->getFrameIndex();
        if (frame < 4)
          player->setFrame(frame + 1);
      }

    } else rightHeld = false;

    if (leftPressed) {
      if (!leftHeld) {
        leftHeld = true;
        lastAnimUpdate = updateCount;
      }

      if (updateCount - lastAnimUpdate >= 4) {
        lastAnimUpdate = updateCount;

        int frame = player->getFrameIndex();
        if (frame > 0)
          player->setFrame(frame - 1);
      }

    } else leftHeld = false;

    if (!leftPressed && !rightPressed) {

      if (updateCount - lastAnimUpdate >= 4) {

        lastAnimUpdate = updateCount;

        int frame = player->getFrameIndex();

        if (frame < 2) player->setFrame(frame + 1);
        if (frame > 2) player->setFrame(frame - 1);
      }
    }

    // ---------- AFTERBURNER ----------
    if (updateCount % 4 == 0) {

      static int lastFrame = 0;

      int newFrame = random(0, 4);

      lastFrame = (newFrame == lastFrame) ? random(0, 4) : newFrame;

      afterburner->setFrame(lastFrame);
    }

    // follow player
    afterburner->x = player->x + 6;
    afterburner->y = player->y + 14;

    ////////////////////////////////////////////////////////
    ////////////////// FIRE /////////////////////////////////

    if (playerFired) {

      playerFire->y -= playerFireSpeed;

      if (playerFire->y <= 0) {

        playerFired = false;

        playerFire->visible = false;
      }
    }

    ////////////////////////////////////////////////////////
    ////////////////// ASTEROIDS ///////////////////////////

    // ---------- ASTEROID EXPLOSIONS ----------
    for (int i = 0; i < ASTEROID_COUNT; i++) {

      if (asteroidExploding[i]) {

        if (updateCount - asteroidExplosionTimer[i] > 2) {

          asteroidExplosionTimer[i] = updateCount;

          asteroidExplosionFrame[i]++;

          if (asteroidExplosionFrame[i] < EXPLOSION_FRAMES) {

            asteroids[i]->setFrame(asteroidExplosionFrame[i]);

          } else {

            asteroidExploding[i] = false;
            asteroidExplosionFrame[i] = 0;
            asteroidActive[i] = false;

            asteroids[i]->moveTo(-100, -100);
          }
        }
      }
    }

    if (updateCount % 40 == 0)
      spawnAsteroid();

    for (int i = 0; i < ASTEROID_COUNT; i++) {

      if (asteroidActive[i] && !asteroidExploding[i]) {

        asteroids[i]->y += asteroidSpeed[i];

        if (asteroids[i]->y > DisplayController.getViewPortHeight()) {

          asteroidActive[i] = false;

          asteroids[i]->moveTo(-100, -100);
        }
      }
    }

    ////////////////////////////////////////////////////////

    updateStars();

    updateSprite(player);
    updateSprite(afterburner);
    updateSpriteAndDetectCollisions(playerFire);

    for (int i = 0; i < ASTEROID_COUNT; i++)
      updateSpriteAndDetectCollisions(asteroids[i]);

    DisplayController.refreshSprites();
  }

  //////////////////////////////////////////////////////////////////////
  ////////////////////////// COLLISIONS ////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  void collisionDetected(Sprite* spriteA, Sprite* spriteB, Point) override {

    int indexA = spriteA - sprites;
    int indexB = spriteB - sprites;

    SpriteType typeA = spriteType[indexA];
    SpriteType typeB = spriteType[indexB];

    if ((typeA == TYPE_PLAYER && typeB == TYPE_ASTEROID) || (typeB == TYPE_PLAYER && typeA == TYPE_ASTEROID)) {

      Sprite* asteroid = (typeA == TYPE_ASTEROID) ? spriteA : spriteB;

      STAR_LAYER3_SPEED = 0;
      // trigger asteroid explosion
      for (int i = 0; i < ASTEROID_COUNT; i++) {
        asteroidSpeed[i] = 0;

        if (!asteroidExploding[i]) {

          asteroidExploding[i] = true;
          asteroidExplosionFrame[i] = 1;
          asteroidExplosionTimer[i] = currentUpdateCount;

          asteroids[i]->setFrame(1);

          break;
        }
      }

      // trigger player explosion
      playerExploding = true;
      playerExplosionFrame = 1;
      playerExplosionTimer = currentUpdateCount;
    }

    if ((typeA == TYPE_PLAYER_FIRE && typeB == TYPE_ASTEROID) || (typeB == TYPE_PLAYER_FIRE && typeA == TYPE_ASTEROID)) {

      Sprite* asteroid = (typeA == TYPE_ASTEROID) ? spriteA : spriteB;

      for (int i = 0; i < ASTEROID_COUNT; i++) {

        if (asteroids[i] == asteroid && !asteroidExploding[i]) {

          asteroidExploding[i] = true;
          asteroidExplosionFrame[i] = 1;
          asteroidExplosionTimer[i] = currentUpdateCount;

          asteroids[i]->setFrame(1);

          playerFire->visible = false;
          playerFired = false;

          break;
        }
      }
    }
  }

  //////////////////////////////////////////////////////////////////////
  ////////////////////////// ASTEROIDS //////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  void spawnAsteroid() {

    for (int i = 0; i < ASTEROID_COUNT; i++) {

      if (!asteroidActive[i] && !asteroidExploding[i]) {

        int lane = random(0, PLAYER_AREA_LANES);

        asteroids[i]->moveTo(lane * 32 + PLAY_AREA_LEFT, -16);

        asteroids[i]->setFrame(0);

        asteroidActive[i] = true;

        asteroidSpeed[i] = random(1, 3);

        break;
      }
    }
  }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {

  PS2Controller.begin(PS2Preset::KeyboardPort0, KbdMode::GenerateVirtualKeys);

  DisplayController.begin();
  DisplayController.setResolution(VGA_320x200_75Hz);

  DisplayController.moveScreen(21, 0);
}

void loop() {
  if (gameState == INTRO_SCREEN) {
    IntroScene intro;
    intro.start();
  }

  if (gameState == IN_GAME) {
    GameScene game;
    game.start();
  }
}