#include "fabgl.h"
#include "sprites.h"

using fabgl::iclamp;

fabgl::VGAController DisplayController;
fabgl::PS2Controller PS2Controller;
fabgl::Canvas canvas(&DisplayController);

struct GameScene : public Scene {

  static const int SPRITESCOUNT = 1;

  fabgl::Sprite sprites[SPRITESCOUNT];
  fabgl::Sprite * player = &sprites[0];

  int playerVelX = 0;

  GameScene()
    : Scene(SPRITESCOUNT, 20, DisplayController.getViewPortWidth(), DisplayController.getViewPortHeight())
  {
  }

  void init() {

    canvas.clear();

    player->addBitmap(&bitmap1);
    player->moveTo(152, 170);

    addSprite(player);

    DisplayController.setSprites(sprites, SPRITESCOUNT);
  }

  void update(int updateCount) {

    auto keyboard = PS2Controller.keyboard();

    playerVelX = 0;

    if (keyboard && keyboard->isKeyboardAvailable()) {

      if (keyboard->isVKDown(fabgl::VK_LEFT))
        playerVelX = -2;
      else if (keyboard->isVKDown(fabgl::VK_RIGHT))
        playerVelX = 2;
    }

    player->x += playerVelX;
    player->x = iclamp(player->x, 0, getWidth() - player->getWidth());

    updateSprite(player);

    DisplayController.refreshSprites();
  }

  void collisionDetected(Sprite * spriteA, Sprite * spriteB, Point collisionPoint) override {
  }
};

GameScene * gameScene;

void setup() {

  PS2Controller.begin(PS2Preset::KeyboardPort0, KbdMode::GenerateVirtualKeys);

  DisplayController.begin();
  DisplayController.setResolution(VGA_320x200_75Hz);
  DisplayController.moveScreen(20, -2);

  gameScene = new GameScene();   // create AFTER VGA init
  gameScene->start();
}

void loop() {
}


