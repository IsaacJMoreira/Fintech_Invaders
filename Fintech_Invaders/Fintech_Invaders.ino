#include "fabgl.h"
#include "sprites.h"

using fabgl::iclamp;

fabgl::VGAController DisplayController;
fabgl::PS2Controller PS2Controller;
fabgl::Canvas canvas(&DisplayController);


struct GameScene : public Scene {

  static const int SPRITESCOUNT = 9;
  fabgl::Sprite sprites[SPRITESCOUNT];
  fabgl::Sprite* player = &sprites[0];       //player sprite group
  fabgl::Sprite* afterburner = &sprites[5];  //afterburner sprites

  int playerVelX = 0;
  int lastAnimUpdate = 0;
  bool leftHeld = false;
  bool rightHeld = false;

  GameScene()
    : Scene(SPRITESCOUNT, 20, DisplayController.getViewPortWidth(), DisplayController.getViewPortHeight()) {}

  void init() {

    canvas.clear();

    player->addBitmap(&bitmap1);
    player->addBitmap(&bitmap2);
    player->addBitmap(&bitmap3);  //Main sprite for player
    player->addBitmap(&bitmap4);
    player->addBitmap(&bitmap5);

    //sprites for afterburner
    afterburner->addBitmap(&afterburner_0);
    afterburner->addBitmap(&afterburner_1);
    afterburner->addBitmap(&afterburner_2);
    afterburner->addBitmap(&afterburner_3);

    afterburner->moveTo(152 + 4, 170 + 4 - 1);  //test

    player->moveTo(152, 170);  //x,y
    addSprite(player);
    addSprite(afterburner);
    DisplayController.setSprites(sprites, SPRITESCOUNT);
    player->setFrame(2);
    afterburner->setFrame(0);
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
      } else if (keyboard->isVKDown(fabgl::VK_RIGHT)) {
        playerVelX = 2;
        rightPressed = true;
      }
    }

    // move player
    player->x += playerVelX;
    player->x = iclamp(player->x, 0, getWidth() - player->getWidth());

    // animation
    if (rightPressed) {
      if (!rightHeld) {
        rightHeld = true;
        lastAnimUpdate = updateCount;
      }
      if (updateCount - lastAnimUpdate >= 4) {
        lastAnimUpdate = updateCount;
        int frame = player->getFrameIndex();
        if (frame < 4) player->setFrame(frame + 1);  // advance to left tilt
      }
    } else {
      rightHeld = false;
    }

    if (leftPressed) {
      if (!leftHeld) {
        leftHeld = true;
        lastAnimUpdate = updateCount;
      }
      if (updateCount - lastAnimUpdate >= 4) {
        lastAnimUpdate = updateCount;
        int frame = player->getFrameIndex();
        if (frame > 0) player->setFrame(frame - 1);  // advance to right tilt
      }
    } else {
      leftHeld = false;
    }

    // if no keys, return to neutral
    if (!leftPressed && !rightPressed) {

      if (updateCount - lastAnimUpdate >= 4) {
        lastAnimUpdate = updateCount;
        int frame = player->getFrameIndex();
        if (frame < 2) player->setFrame(frame + 1);  // advance to right tilt
        if (frame > 2) player->setFrame(frame - 1);  // advance to right tilt
      }
      //player->setFrame(2);
    }

    // animate afterburner
if (updateCount % 4 == 0) {

  static int lastFrame = 0;

  int newFrame = random(0, 4);

  lastFrame = (newFrame == lastFrame) ? random(0, 4) : newFrame;

  afterburner->setFrame(lastFrame);
}
    // keep it attached to player
    afterburner->x = player->x + 6;
    afterburner->y = player->y + (16 - 3);
    // update sprites
    updateSprite(player);
    updateSprite(afterburner);
    DisplayController.refreshSprites();
  }

  void collisionDetected(Sprite* spriteA, Sprite* spriteB, Point collisionPoint) override {
    // handle collisions here
  }
};






GameScene* gameScene;


void setup() {

  PS2Controller.begin(PS2Preset::KeyboardPort0, KbdMode::GenerateVirtualKeys);

  DisplayController.begin();
  DisplayController.setResolution(VGA_320x200_75Hz);
  DisplayController.moveScreen(20, -2);

  gameScene = new GameScene();
  gameScene->start();
}


void loop() {
}