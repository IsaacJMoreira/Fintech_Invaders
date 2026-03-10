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

  GameScene()
    : Scene(SPRITESCOUNT, 20, DisplayController.getViewPortWidth(), DisplayController.getViewPortHeight()) {
  }


  void init() {

    canvas.clear();

    player->addBitmap(&bitmap1);  // frame 0
    player->addBitmap(&bitmap2);  // frame 1
    player->addBitmap(&bitmap3);  // frame 2

    player->moveTo(152, 170);

    addSprite(player);

    DisplayController.setSprites(sprites, SPRITESCOUNT);
  }


  void update(int updateCount) {

    auto keyboard = PS2Controller.keyboard();

    playerVelX = 0;
    bool leftPressed = false;

    if (keyboard && keyboard->isKeyboardAvailable()) {

      if (keyboard->isVKDown(fabgl::VK_LEFT)) {
        playerVelX = -2;
        leftPressed = true;
      }

      else if (keyboard->isVKDown(fabgl::VK_RIGHT)) {
        playerVelX = 2;
      }
    }


    player->x += playerVelX;
    player->x = iclamp(player->x, 0, getWidth() - player->getWidth());


    // LEFT tilt animation
    if (leftPressed) {

      if (!leftHeld) {
        // key was just pressed
        leftHeld = true;
        lastAnimUpdate = updateCount;
      }

      if (updateCount - lastAnimUpdate >= 4) {

        lastAnimUpdate = updateCount;

        int frame = player->getFrameIndex();

        if (frame < 2) {     // advance until frame 2
          player->setFrame(frame + 1);
        }
      }

    } else {

      // key released → return to neutral
      leftHeld = false;
      player->setFrame(0);
    }


    updateSprite(player);

    DisplayController.refreshSprites();
  }


  void collisionDetected(Sprite* spriteA, Sprite* spriteB, Point collisionPoint) override {
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