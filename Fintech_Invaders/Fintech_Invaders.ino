#include "fabgl.h"
#include "sprites.h"

#define PLAY_AREA_LEFT   59
#define PLAY_AREA_RIGHT  259

using fabgl::iclamp;

fabgl::VGAController DisplayController;
fabgl::PS2Controller PS2Controller;
fabgl::Canvas canvas(&DisplayController);


struct GameScene : public Scene {

  static const int SPRITESCOUNT = 12;
  fabgl::Sprite sprites[SPRITESCOUNT];
  fabgl::Sprite* player = &sprites[0];       //player sprite group
  fabgl::Sprite* afterburner = &sprites[5];  //afterburner sprites
  fabgl::Sprite* star0 = &sprites[9];        //star1 sprites
  fabgl::Sprite* star1 = &sprites[10];        //star2 sprites
  fabgl::Sprite* star2 = &sprites[11];        //star3 sprites

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

    //sprites for Starts

    star0->addBitmap(&starBMP0);
    star1->addBitmap(&starBMP1);
    star2->addBitmap(&starBMP2);

    star0->moveTo((320-3)/2,100);
    star1->moveTo((320-5)/2,110);
    star2->moveTo((320-7)/2,120);

    afterburner->moveTo(152 + 4, 170 + 4 - 1);  //test

    player->moveTo(152, 170);  //x,y


    addSprite(player);
    addSprite(afterburner);
    addSprite(star0);
    addSprite(star1);
    addSprite(star2);
    DisplayController.setSprites(sprites, SPRITESCOUNT);
    player->setFrame(2);
    afterburner->setFrame(0);

  //DRAW THE RIGHT AND LEFT MENUS
  canvas.setBrushColor(Color::White);
  //canvas.setBrushColor(3, 2, 0);
  canvas.fillRectangle(0,0, PLAY_AREA_LEFT,DisplayController.getViewPortHeight() - 1 );
  canvas.fillRectangle( PLAY_AREA_RIGHT, 0, DisplayController.getViewPortWidth() - 1, DisplayController.getViewPortHeight() - 1); 
  //DRAW THE STAR FIELD
  canvas.setBrushColor(Color::Black);
  canvas.fillRectangle(PLAY_AREA_LEFT + 1, 0, PLAY_AREA_RIGHT -1 , DisplayController.getViewPortHeight() - 1);
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
    player->x = iclamp(player->x,PLAY_AREA_LEFT + 1, PLAY_AREA_RIGHT - player->getWidth());

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
    updateSprite(star0);
    updateSprite(star1);
    updateSprite(star2);
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
  DisplayController.moveScreen(21, -1);

  gameScene = new GameScene();
  gameScene->start();
}


void loop() {
}