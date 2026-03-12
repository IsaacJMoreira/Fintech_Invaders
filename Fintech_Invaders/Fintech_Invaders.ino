#include "fabgl.h"
#include "sprites.h"

#define PLAY_AREA_LEFT   59
#define PLAY_AREA_RIGHT  259

#define STAR0_COUNT 10
#define STAR1_COUNT 7
#define STAR2_COUNT 3
#define STAR_SPEED 8  // at the future, must be relative to spaceship speed.

using fabgl::iclamp;

fabgl::VGAController DisplayController;
fabgl::PS2Controller PS2Controller;
fabgl::Canvas canvas(&DisplayController);


struct GameScene : public Scene {

  static const int SPRITESCOUNT = 22;
  fabgl::Sprite sprites[SPRITESCOUNT];

  fabgl::Sprite* player = &sprites[20];
  fabgl::Sprite* afterburner = &sprites[21];

  int playerVelX = 0;
  int lastAnimUpdate = 0;
  bool leftHeld = false;
  bool rightHeld = false;

  GameScene()
    : Scene(SPRITESCOUNT, 20, DisplayController.getViewPortWidth(), DisplayController.getViewPortHeight()) {}



  // ----------- STAR INITIALIZATION -----------
  void initStars() {

    int index = 0;

    // small stars
    for (int i = 0; i < STAR0_COUNT; i++) {
      sprites[index].addBitmap(&starBMP0);
      sprites[index].moveTo(
        random(PLAY_AREA_LEFT + 1, PLAY_AREA_RIGHT - 3),
        random(0, DisplayController.getViewPortHeight())
      );
      addSprite(&sprites[index]);
      index++;
    }

    // medium stars
    for (int i = 0; i < STAR1_COUNT; i++) {
      sprites[index].addBitmap(&starBMP1);
      sprites[index].moveTo(
        random(PLAY_AREA_LEFT + 1, PLAY_AREA_RIGHT - 5),
        random(0, DisplayController.getViewPortHeight())
      );
      addSprite(&sprites[index]);
      index++;
    }

    // large stars
    for (int i = 0; i < STAR2_COUNT; i++) {
      sprites[index].addBitmap(&starBMP2);
      sprites[index].moveTo(
        random(PLAY_AREA_LEFT + 1, PLAY_AREA_RIGHT - 7),
        random(0, DisplayController.getViewPortHeight())
      );
      addSprite(&sprites[index]);
      index++;
    }
  }

  // ----------- STAR MOVEMENT -----------
  void updateStars() {

    int start = 0;
    int end = STAR0_COUNT + STAR1_COUNT + STAR2_COUNT;

    for (int i = start; i < end; i++) {

      sprites[i].y += STAR_SPEED;

      if (sprites[i].y >= DisplayController.getViewPortHeight()) {

        sprites[i].y = 0;

        sprites[i].x = random(
          PLAY_AREA_LEFT + 1,
          PLAY_AREA_RIGHT - sprites[i].getWidth()
        );
      }

      updateSprite(&sprites[i]);
    }
  }



  void init() {

    canvas.clear();

    // PLAYER BITMAPS
    player->addBitmap(&bitmap1);
    player->addBitmap(&bitmap2);
    player->addBitmap(&bitmap3);
    player->addBitmap(&bitmap4);
    player->addBitmap(&bitmap5);

    // AFTERBURNER
    afterburner->addBitmap(&afterburner_0);
    afterburner->addBitmap(&afterburner_1);
    afterburner->addBitmap(&afterburner_2);
    afterburner->addBitmap(&afterburner_3);

    player->moveTo(152,170);
    afterburner->moveTo(156,173);

    // STARFIELD
    initStars();

    // PLAYER + ENGINE
    addSprite(player);
    addSprite(afterburner);

    DisplayController.setSprites(sprites, SPRITESCOUNT);

    player->setFrame(2);
    afterburner->setFrame(0);


    // SIDE PANELS
    canvas.setBrushColor(Color::White);

    canvas.fillRectangle(
      0,0,
      PLAY_AREA_LEFT,
      DisplayController.getViewPortHeight() - 1
    );

    canvas.fillRectangle(
      PLAY_AREA_RIGHT,
      0,
      DisplayController.getViewPortWidth() - 1,
      DisplayController.getViewPortHeight() - 1
    );

    // STARFIELD BACKGROUND
    canvas.setBrushColor(Color::Black);

    canvas.fillRectangle(
      PLAY_AREA_LEFT + 1,
      0,
      PLAY_AREA_RIGHT - 1,
      DisplayController.getViewPortHeight() - 1
    );
  }



  void update(int updateCount) override {

    auto keyboard = PS2Controller.keyboard();
    playerVelX = 0;

    bool leftPressed = false;
    bool rightPressed = false;

    if (keyboard && keyboard->isKeyboardAvailable()) {

      if (keyboard->isVKDown(fabgl::VK_LEFT)) {
        playerVelX = -2;
        leftPressed = true;
      }

      else if (keyboard->isVKDown(fabgl::VK_RIGHT)) {
        playerVelX = 2;
        rightPressed = true;
      }
    }


    // PLAYER MOVEMENT
    player->x += playerVelX;

    player->x = iclamp(
      player->x,
      PLAY_AREA_LEFT + 1,
      PLAY_AREA_RIGHT - player->getWidth()
    );


    // PLAYER ANIMATION
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



    // RETURN TO CENTER
    if (!leftPressed && !rightPressed) {

      if (updateCount - lastAnimUpdate >= 4) {

        lastAnimUpdate = updateCount;

        int frame = player->getFrameIndex();

        if (frame < 2) player->setFrame(frame + 1);
        if (frame > 2) player->setFrame(frame - 1);
      }
    }



    // AFTERBURNER ANIMATION
    if (updateCount % 4 == 0) {

      static int lastFrame = 0;

      int newFrame = random(0,4);

      lastFrame = (newFrame == lastFrame) ? random(0,4) : newFrame;

      afterburner->setFrame(lastFrame);
    }


    // ATTACH AFTERBURNER
    afterburner->x = player->x + 6;
    afterburner->y = player->y + (16 - 3);


    // UPDATE STARFIELD
    updateStars();


    // UPDATE PLAYER
    updateSprite(player);
    updateSprite(afterburner);

    DisplayController.refreshSprites();
  }



  void collisionDetected(Sprite* spriteA, Sprite* spriteB, Point collisionPoint) override {
  }
};



GameScene* gameScene;



void setup() {

  PS2Controller.begin(
    PS2Preset::KeyboardPort0,
    KbdMode::GenerateVirtualKeys
  );

  DisplayController.begin();
  DisplayController.setResolution(VGA_320x200_75Hz);

  DisplayController.moveScreen(21,-1);

  gameScene = new GameScene();
  gameScene->start();
}



void loop() {
}