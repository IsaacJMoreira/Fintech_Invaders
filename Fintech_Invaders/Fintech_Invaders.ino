#include "fabgl.h"
#include "sprites.h"

using fabgl::iclamp;

fabgl::VGAController DisplayController;
fabgl::PS2Controller PS2Controller;
fabgl::Canvas canvas(&DisplayController);


struct GameScene : public Scene {

  static const int SPRITESCOUNT = 1;
  fabgl::Sprite sprites[SPRITESCOUNT];
  fabgl::Sprite* player = &sprites[0];

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
    player->addBitmap(&bitmap3);
    player->addBitmap(&bitmap4);
    player->addBitmap(&bitmap5);

    player->moveTo(152, 170);
    addSprite(player);
    DisplayController.setSprites(sprites, SPRITESCOUNT);
    player->setFrame(2);
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

    // update sprite
    updateSprite(player);
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