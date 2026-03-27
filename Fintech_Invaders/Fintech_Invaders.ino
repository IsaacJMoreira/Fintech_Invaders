#include "fabgl.h"
#include "sprites.h"
#include <EEPROM.h>

#define EEPROM_SIZE 64
#define HIGHSCORE_ADDR 0
#define PLAYERNAME_ADDR 4  // after int (4 bytes)
#define MAX_NAME_LENGTH 15

int HIGH_SCORE = 0;
char HIGH_SCORE_NAME[MAX_NAME_LENGTH + 1] = "PLAYER";

using fabgl::iclamp;

int SCORE = 0;
float SCREEN_SPEED = 0;
int MISSION_TIME = 0;

#define PLAY_AREA_LEFT 79
#define PLAY_AREA_RIGHT 239
#define PLAYER_AREA_LANES 5

#define STAR0_COUNT 10
#define STAR1_COUNT 7
#define STAR2_COUNT 3

float ASTEROID_SPEED = 2;
#define ASTEROID_COUNT 5
#define EXPLOSION_FRAMES 4

float STAR_LAYER3_SPEED = 0.1;
float STAR_LAYER2_SPEED = (STAR_LAYER3_SPEED * 2);
float STAR_LAYER1_SPEED = (STAR_LAYER2_SPEED * 2);
float STAR_LAYER0_SPEED = (STAR_LAYER1_SPEED * 2);

float PLAYER_FIRE_SPEED = 1;
#define PLAYER_FIRE_MAX_SPEED 7
int PLAYER_AMO_COUNT = 10;
#define PLAYER_MAX_AMO_AUTO 10       // FOR AUTO
#define PLAYER_MAX_AMO_ABSOLUTE 500  //FOR PHASE 2
int playerLifeCount = 2;

#define PEGUE_PAG_RESPAWN_INTERVAL 200
#define PEGUE_PAGUE_KILL_HITS 5
#define FEIRAPAGA_RESPAWN_INTERVAL 100
#define FEIRAPAGA_KILL_HITS 7
#define OUTER_KILL_HITS 1
#define OUTER_RESPAWN_INTERVAL 300

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

void loadHighScore() {
  EEPROM.begin(EEPROM_SIZE);

  EEPROM.get(HIGHSCORE_ADDR, HIGH_SCORE);

  for (int i = 0; i < MAX_NAME_LENGTH; i++) {
    HIGH_SCORE_NAME[i] = EEPROM.read(PLAYERNAME_ADDR + i);
  }

  HIGH_SCORE_NAME[MAX_NAME_LENGTH] = '\0';

  // safety fallback
  if (HIGH_SCORE < 0 || HIGH_SCORE > 999999) {
    HIGH_SCORE = 0;
    strcpy(HIGH_SCORE_NAME, "COLEGA");
  }
}

void saveHighScore() {

  EEPROM.put(HIGHSCORE_ADDR, HIGH_SCORE);

  for (int i = 0; i < MAX_NAME_LENGTH; i++) {
    EEPROM.write(PLAYERNAME_ADDR + i,
                 i < strlen(HIGH_SCORE_NAME) ? HIGH_SCORE_NAME[i] : 0);
  }

  EEPROM.commit();
}

/////////// HI SCORE SCENE ///////////////////////

struct InputNameScene : public Scene {

  char name[MAX_NAME_LENGTH + 1] = "";
  int length = 0;

  InputNameScene()
    : Scene(0, 20, 320, 200) {}

  void init() override {
    auto keyboard = PS2Controller.keyboard();
    if (keyboard) {
      keyboard->setLayout(&fabgl::USLayout);
      keyboard->enableVirtualKeys(true, true);
    }
    // 🔥 DRAW EVERYTHING EVERY FRAME (NO onPaint)
    canvas.clear();
    canvas.drawBitmap(0, 0, &ASTRONAUT);  // 🔥 FIRST STORY SCREEN
    canvas.setBrushColor(Color::Black);
    canvas.fillRectangle(187, 93, 318, 115);
    canvas.setPenColor(Color::BrightYellow);
    canvas.selectFont(&fabgl::FONT_8x14);
    canvas.drawText(190, 94, "FINTECH INVADERS");
    canvas.selectFont(&fabgl::FONT_4x6);
    //canvas.setPenColor(Color::BrightGreen);
    canvas.drawText(249, 108, "POR ISAAC MOREIRA");
    canvas.drawText(249, 194, "V1.0 (01/04/2026)");
    //canvas.drawText(1, 7, "'THOSE WHO CAN IMAGINE ANYTHING,");
    //canvas.drawText(1, 7, "CAN CREATE THE IMPOSSIBLE' - ALAN TURING");
  }

  void update(int updateCount) override {

    auto keyboard = PS2Controller.keyboard();

    if (!keyboard || !keyboard->isKeyboardAvailable())
      return;

    fabgl::VirtualKeyItem vk;

    while (keyboard->virtualKeyAvailable()) {

      keyboard->getNextVirtualKey(&vk);

      if (!vk.down)
        continue;

      if (vk.vk == fabgl::VK_RETURN) {

        if (length == 0)
          strcpy(name, "PLAYER");

        strcpy(HIGH_SCORE_NAME, name);
        saveHighScore();

        auto keyboard = PS2Controller.keyboard();
        if (keyboard) {
          keyboard->emptyVirtualKeyQueue();
          keyboard->enableVirtualKeys(true, false);  // 🔥 disable VK system
        }

        gameState = INTRO_SCREEN;
        stop();
        return;
      }

      else if (vk.vk == fabgl::VK_BACKSPACE) {

        if (length > 0) {
          length--;
          name[length] = '\0';
        }
      }

      else if (vk.ASCII >= 32 && vk.ASCII <= 126) {

        if (length < MAX_NAME_LENGTH) {

          char c = (char)vk.ASCII;

          if (c >= 'a' && c <= 'z')
            c -= 32;

          name[length++] = c;
          name[length] = '\0';
        }
      }
    }



    canvas.setBrushColor(Color::Blue);
    canvas.fillRectangle(0, 172, 177, 200);
    canvas.selectFont(&fabgl::FONT_4x6);
    canvas.setPenColor(Color::BrightCyan);
    ///////////////////////////////////////////////////////////////////<<<<
    canvas.drawTextFmt(2, 174, "VOCE CHEGOU LONGE! SEU SCORE: %05d", SCORE);

    canvas.setPenColor(Color::BrightYellow);
    canvas.drawText(2, 181, "DIGITE SEU NOME + [ENTER] PARA CONFIRMAR:");

    canvas.setPenColor(Color::BrightCyan);
    canvas.selectFont(&fabgl::FONT_8x8);

    canvas.drawText(2, 189, name);

    // blinking cursor
    if ((millis() / 300) % 2 == 0 && length < MAX_NAME_LENGTH) {
      canvas.drawText(2 + (length * 8), 189, "_");
    }
  }

  void collisionDetected(Sprite*, Sprite*, Point) override {}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// INTRO SCENE //////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct IntroScene : public Scene {

  enum IntroPhase {
    INTRO_IDLE,
    INTRO_INSTRUCTIONS,
    INTRO_INVITE,
    INTRO_ATTACK
  };

  IntroPhase phase = INTRO_IDLE;
  unsigned long phaseStartTime = 0;



  IntroScene()
    : Scene(0, 20, 320, 200) {}

  void init() override {
    SCORE = 0;
    SCREEN_SPEED = 0;
    MISSION_TIME = 0;
    PLAYER_FIRE_SPEED = 1;
    PLAYER_AMO_COUNT = 10;
    ASTEROID_SPEED = 1;
    playerLifeCount = 2;

    canvas.clear();
    canvas.drawBitmap(0, 0, &FINTECH_INVADERS);

    // 🔥 DRAW HIGH SCORE
    canvas.setBrushColor(Color::Blue);
    canvas.fillRectangle(0, 172, 120, 200);
    canvas.selectFont(&fabgl::FONT_4x6);
    canvas.setPenColor(Color::BrightCyan);

    canvas.drawText(2, 174, "MELHOR SCORE");

    canvas.setPenColor(Color::BrightYellow);
    canvas.drawTextFmt(2, 183, "%s | %05d", HIGH_SCORE_NAME, HIGH_SCORE);

    canvas.setPenColor(Color::BrightRed);

    canvas.drawText(2, 192, "PRESSIONE [ESPACO] PARA JOGAR");
    phase = INTRO_IDLE;
    phaseStartTime = 0;
  }

  void update(int updateCount) override {

    if (updateCount < 20)
      return;

    auto keyboard = PS2Controller.keyboard();

    // =========================================================
    // PHASE 1: WAIT FOR SPACE (NORMAL INTRO SCREEN)
    // =========================================================
    if (phase == INTRO_IDLE) {

      if (keyboard && keyboard->isVKDown(fabgl::VK_SPACE)) {

        phase = INTRO_ATTACK;
        phaseStartTime = millis();

        canvas.clear();
        canvas.drawBitmap(0, 0, &ATTACK);  // 🔥 FIRST STORY SCREEN

        canvas.setBrushColor(Color::Blue);
        canvas.fillRectangle(0, 120, 140, 200);
        canvas.selectFont(&fabgl::FONT_4x6);
        canvas.setPenColor(Color::BrightCyan);

        canvas.drawText(1, 122, "DIARIO DE BORDO - ANO 2018");
        ///////////////////////////////////////////////////////////////<<<<<<
        canvas.drawText(1, 134, "O MERCADO MUDOU MAIS RAPIDO DO QUE");
        canvas.drawText(1, 141, "QUALQUER PREVISAO. NOVAS FORCAS");
        canvas.drawText(1, 148, "SURGIRAM. SEM AGENCIAS, SEM FILAS,");
        canvas.drawText(1, 155, "APENAS CODIGO E VELOCIDADE.");
        canvas.drawText(1, 162, "AS FINTECHS AVANCAM EM ONDAS,");
        canvas.drawText(1, 169, "CONQUISTANDO CLIENTES E REDUZINDO");
        canvas.drawText(1, 176, "NOSSO TERRITORIO. SE NAO REAGIRMOS");
        canvas.drawText(1, 183, "AGORA PERDEREMOS O CONTROLE!");

        canvas.setPenColor(Color::BrightRed);

        canvas.drawText(12, 192, "PRESSIONE [P] PARA PULAR INTRO");
      }

      return;
    }

    // =========================================================
    // SKIP WITH [P] (WORKS IN ANY STORY PHASE)
    // =========================================================
    if (keyboard && (keyboard->isVKDown(fabgl::VK_p) || keyboard->isVKDown(fabgl::VK_P))) {
      gameState = IN_GAME;
      stop();
    }

    unsigned long elapsed = millis() - phaseStartTime;

    // =========================================================
    // PHASE 2: INVITE → after 5s go to ATTACK
    // =========================================================
    if (phase == INTRO_ATTACK) {

      if (elapsed >= 2000) {

        phase = INTRO_INVITE;
        phaseStartTime = millis();

        canvas.clear();
        canvas.drawBitmap(0, 0, &INVITE);  // 🔥 SECOND STORY SCREEN

        canvas.setBrushColor(Color::Blue);
        canvas.fillRectangle(180, 120, 320, 200);
        canvas.selectFont(&fabgl::FONT_4x6);
        canvas.setPenColor(Color::BrightCyan);

        ///////////////////////////////////////////////////////////////<<<<<<

        canvas.drawText(181, 122, "PRECISAMOS DE VOCE NESSA MISSAO!");
        canvas.drawText(181, 129, "ASSUMA O CONTROLE DA NAVE CAIXA E");
        canvas.drawText(181, 136, "GARANTA O FUTURO DA INSTITUICAO!");
        //canvas.drawText(181, 143, "");
        canvas.drawText(181, 150, "NAO VAI SER FACIL. HAVERA PEDRAS");
        canvas.drawText(181, 157, "NO SEU CAMINHO E O INIMIGO E VELOZ.");
        canvas.drawText(181, 164, "MAS USANDO BEM OS RECURSOS E");
        canvas.drawText(181, 171, "POUPANDO, VOCE NOS LEVARA LONGE!");
        canvas.drawText(181, 178, "CONTAMOS COM VOCE!");

        canvas.setPenColor(Color::BrightRed);

        canvas.drawText(192, 192, "PRESSIONE [P] PARA PULAR INTRO");
      }

      return;
    }


    // =========================================================
    // PHASE 2: INVITE → after 5s go to ATTACK
    // =========================================================
    if (phase == INTRO_INVITE) {

      if (elapsed >= 2000) {

        phase = INTRO_INSTRUCTIONS;
        phaseStartTime = millis();

        canvas.clear();
        canvas.drawBitmap(0, 0, &INSTRUCTIONS);  // 🔥 SECOND STORY SCREEN


        canvas.setPenColor(Color::BrightYellow);

        ///////////////////////////////////////////////////////////////<<<<<<
        canvas.selectFont(&fabgl::FONT_8x8);
        canvas.drawText(70, 40, "CONTROLES");
        //canvas.drawText(181, 129, "ASSUMA O CONTROLE DA NAVE CAIXA E");
        canvas.drawText(70, 50, "MOVER: [<-] e [->]");
        //canvas.drawText(181, 143, "");
        canvas.drawText(70, 60, "ATIRAR: [ESPACO]");
        //canvas.drawText(181, 157, "NO SEU CAMINHO E O INIMIGO E VELOZ.");
        //canvas.drawText(181, 164, "MAS USANDO BEM OS RECURSOS E");
        //canvas.drawText(181, 171, "POUPANDO, VOCE NOS LEVARA LONGE!");
        //canvas.drawText(181, 178, "CONTAMOS COM VOCE!");

        canvas.setPenColor(Color::BrightRed);

        canvas.selectFont(&fabgl::FONT_4x6);

        canvas.drawText(70, 70, "PRESSIONE [P] PARA PULAR INTRO");
      }

      return;
    }

    // =========================================================
    // PHASE 3: ATTACK → after 5s go to GAME
    // =========================================================
    if (phase == INTRO_INSTRUCTIONS) {

      if (elapsed >= 5000) {

        gameState = IN_GAME;
        stop();
        return;
      }
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
  TYPE_ASTEROID,
  TYPE_PEGUEPAGUE,
  TYPE_FEIRAPAGA,
  TYPE_OUTER,
  TYPE_FEIRAPAGA_FIRE
};

struct GameScene : public Scene {

  unsigned long gameOverStartTime = 0;
  bool gameOverHandled = false;


  int peguePagueRespawnTime = 0;
  int feiraPagaRespawnTime = 0;

  int peguePagueAnimFrame = 0;
  int peguePagueAnimDir = 1;
  int peguePagueAnimTimer = 0;

  static const int SPRITESCOUNT = 36;

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

  fabgl::Sprite* peguePague = &sprites[28];

  fabgl::Sprite* feiraPaga = &sprites[29];

  bool peguePagueExploding = false;
  int peguePagueExplosionFrame = 0;
  int peguePagueExplosionTimer = 0;

  bool peguePagueActive = false;
  int peguePagueHits = 0;
  float peguePagueX = 0;

  bool feiraPagaExploding = false;
  int feiraPagaExplosionFrame = 0;
  int feiraPagaExplosionTimer = 0;

  bool feiraPagaActive = false;
  int feiraPagaHits = 0;
  float feiraPagaX = 0;

  int feiraPagaAnimFrame = 0;
  int feiraPagaAnimTimer = 0;

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

  fabgl::Sprite* outer = &sprites[30];

  bool outerActive = false;
  bool outerExploding = false;

  int outerHits = 0;

  int outerExplosionFrame = 0;
  int outerExplosionTimer = 0;

  int outerAnimFrame = 0;
  int outerAnimTimer = 0;

  float outerX = 0;

  fabgl::Sprite* feiraPagaFire = &sprites[31];

  bool feiraPagaFireActive = false;
  float feiraPagaFireSpeed = 2;

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

      // asteroid rotation frames
      asteroids[i]->addBitmap(&ASTEROID_0);
      asteroids[i]->addBitmap(&ASTEROID_1);
      asteroids[i]->addBitmap(&ASTEROID_2);
      asteroids[i]->addBitmap(&ASTEROID_3);

      // explosion frames
      asteroids[i]->addBitmap(&EXPLOSION_0);
      asteroids[i]->addBitmap(&EXPLOSION_1);
      asteroids[i]->addBitmap(&EXPLOSION_2);
      asteroids[i]->addBitmap(&EXPLOSION_3);

      asteroids[i]->moveTo(-100, -100);

      asteroidActive[i] = false;

      asteroidExploding[i] = false;

      asteroidExplosionFrame[i] = 0;

      asteroidSpeed[i] = ASTEROID_SPEED;

      addSprite(asteroids[i]);

      spriteType[23 + i] = TYPE_ASTEROID;
    }

    peguePague->addBitmap(&PEGUEEPAGUE_0);
    peguePague->addBitmap(&PEGUEEPAGUE_1);
    peguePague->addBitmap(&PEGUEEPAGUE_2);
    peguePague->addBitmap(&PEGUEEPAGUE_3);

    // explosion frames AFTER
    peguePague->addBitmap(&EXPLOSION_0);
    peguePague->addBitmap(&EXPLOSION_1);
    peguePague->addBitmap(&EXPLOSION_2);
    peguePague->addBitmap(&EXPLOSION_3);

    peguePague->moveTo(-100, -100);

    spriteType[28] = TYPE_PEGUEPAGUE;

    addSprite(peguePague);

    feiraPaga->addBitmap(&FEIRAPAGA_0);
    feiraPaga->addBitmap(&FEIRAPAGA_1);
    feiraPaga->addBitmap(&FEIRAPAGA_2);
    feiraPaga->addBitmap(&FEIRAPAGA_3);

    feiraPagaFire->addBitmap(&FEIRAPAGA_FIRE);  // FEIRAPAGA projectile bitmap
    feiraPagaFire->visible = false;
    feiraPagaFire->moveTo(-100, -100);

    spriteType[31] = TYPE_FEIRAPAGA_FIRE;

    addSprite(feiraPagaFire);

    // explosion frames
    feiraPaga->addBitmap(&EXPLOSION_0);
    feiraPaga->addBitmap(&EXPLOSION_1);
    feiraPaga->addBitmap(&EXPLOSION_2);
    feiraPaga->addBitmap(&EXPLOSION_3);

    feiraPaga->moveTo(-100, -100);

    spriteType[29] = TYPE_FEIRAPAGA;

    addSprite(feiraPaga);

    outer->addBitmap(&OUTER_0);
    outer->addBitmap(&OUTER_1);
    outer->addBitmap(&OUTER_2);
    outer->addBitmap(&OUTER_3);

    // explosion frames
    outer->addBitmap(&EXPLOSION_0);
    outer->addBitmap(&EXPLOSION_1);
    outer->addBitmap(&EXPLOSION_2);
    outer->addBitmap(&EXPLOSION_3);

    outer->moveTo(-100, -100);

    spriteType[30] = TYPE_OUTER;

    addSprite(outer);

    peguePagueRespawnTime = PEGUE_PAG_RESPAWN_INTERVAL;
    feiraPagaRespawnTime = FEIRAPAGA_RESPAWN_INTERVAL;

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
  ////////////////////////// UPDATE_GAME /////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  void update(int updateCount) override {

    if (updateCount < 20)
      return;

    auto keyboard = PS2Controller.keyboard();

    if ((updateCount % 4) == 0 && !gameOver) {
      ///////////////////////////////////////////////////////////////////////
      //////////////////////////////// SCORE ////////////////////////////////
      ///////////////////////////////////////////////////////////////////////
      canvas.setBrushColor(Color::Blue);
      canvas.fillRectangle(PLAY_AREA_RIGHT + 2, 1, 318, 7);
      canvas.selectFont(&fabgl::FONT_4x6);
      canvas.setPenColor(Color::BrightCyan);
      canvas.drawText(PLAY_AREA_RIGHT + 2, 2, "SCORE");
      canvas.drawTextFmt(297, 2, "%05d", SCORE);

      ///////////////////////////////////////////////////////////////////////
      //////////////////////////////// TEMPO ////////////////////////////////
      ///////////////////////////////////////////////////////////////////////
      canvas.setBrushColor(127, 82, 0);
      canvas.fillRectangle(PLAY_AREA_RIGHT + 2, 9, 318, 15);
      canvas.selectFont(&fabgl::FONT_4x6);
      canvas.setPenColor(Color::BrightYellow);
      canvas.drawText(PLAY_AREA_RIGHT + 2, 10, "TEMPO");
      canvas.drawTextFmt(277, 10, "%010d", MISSION_TIME);

      ///////////////////////////////////////////////////////////////////////
      /////////////////////////// PROJECTILE SPEED //////////////////////////
      ///////////////////////////////////////////////////////////////////////
      canvas.setBrushColor(Color::Blue);
      canvas.fillRectangle(PLAY_AREA_RIGHT + 2, 17, 318, 23);
      canvas.selectFont(&fabgl::FONT_4x6);
      canvas.setPenColor(Color::BrightCyan);
      canvas.drawText(PLAY_AREA_RIGHT + 2, 18, "VEL.PROJETIL");
      canvas.drawTextFmt(297, 18, "%05d", (int)(PLAYER_FIRE_SPEED * 100));  //JUST FOR SHOW
      ///////////////////////////////////////////////////////////////////////
      /////////////////////////// PROJECTILE SPEED //////////////////////////
      ///////////////////////////////////////////////////////////////////////
      canvas.setBrushColor(Color::Blue);
      canvas.fillRectangle(PLAY_AREA_RIGHT + 2, 25, 318, 31);
      canvas.selectFont(&fabgl::FONT_4x6);
      canvas.setPenColor(Color::BrightCyan);
      canvas.drawText(PLAY_AREA_RIGHT + 2, 26, "PROJETIL VEL.MAX");
      if (PLAYER_FIRE_SPEED == PLAYER_FIRE_MAX_SPEED) {
        canvas.setPenColor(Color::BrightGreen);
        canvas.setBrushColor(Color::BrightGreen);
        canvas.fillEllipse(314, 28, 4, 4);  //JUST FOR SHOW
      } else {
        canvas.setPenColor(Color::Red);
        canvas.setBrushColor(Color::Red);
        canvas.fillEllipse(314, 28, 4, 4);  //JUST FOR SHOW
      }


      ///////////////////////////////////////////////////////////////////////
      /////////////////////////// PROJECTILE COUNT //////////////////////////
      ///////////////////////////////////////////////////////////////////////
      canvas.setBrushColor(Color::Blue);
      canvas.fillRectangle(PLAY_AREA_RIGHT + 2, 33, 318, 39);
      canvas.selectFont(&fabgl::FONT_4x6);
      canvas.setPenColor(Color::BrightCyan);
      canvas.drawText(PLAY_AREA_RIGHT + 2, 34, "MINICAO");
      //canvas.setPenColor(Color::Yellow);
      canvas.drawTextFmt(297, 34, "%05d", PLAYER_AMO_COUNT);  //JUST FOR SHOW

      ///////////////////////////////////////////////////////////////////////
      /////////////////////////// PROJECTILE READY //////////////////////////
      ///////////////////////////////////////////////////////////////////////
      canvas.setBrushColor(Color::Blue);
      canvas.fillRectangle(PLAY_AREA_RIGHT + 2, 41, 318, 47);
      canvas.selectFont(&fabgl::FONT_4x6);
      canvas.setPenColor(Color::BrightCyan);
      canvas.drawText(PLAY_AREA_RIGHT + 2, 42, "PROJETIL PRONTO");

      if (!playerFire->visible && PLAYER_AMO_COUNT) {

        canvas.setPenColor(Color::BrightGreen);
        canvas.setBrushColor(Color::BrightGreen);
        canvas.fillEllipse(314, 44, 4, 4);  //JUST FOR SHOW
      } else {

        canvas.setPenColor(Color::Red);
        canvas.setBrushColor(Color::Red);
        canvas.fillEllipse(314, 44, 4, 4);  //JUST FOR SHOW
      }

      ///////////////////////////////////////////////////////////////////////
      /////////////////////////// PLAYER SPEED //////////////////////////
      ///////////////////////////////////////////////////////////////////////
      canvas.setBrushColor(127, 82, 0);
      canvas.fillRectangle(PLAY_AREA_RIGHT + 2, 49, 318, 55);
      canvas.selectFont(&fabgl::FONT_4x6);
      canvas.setPenColor(Color::BrightYellow);
      canvas.drawText(PLAY_AREA_RIGHT + 2, 50, "VEL.NAVE");
      //canvas.setPenColor(Color::Yellow);
      canvas.drawTextFmt(293, 50, "%06d", (int)(SCREEN_SPEED * 100));  //JUST FOR SHOW
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

      if (keyboard->isVKDown(fabgl::VK_SPACE) && !playerFired && PLAYER_AMO_COUNT) {

        PLAYER_AMO_COUNT = PLAYER_AMO_COUNT - 1 <= 0 ? 0 : PLAYER_AMO_COUNT - 1;

        playerFire->visible = true;

        playerFired = true;

        playerFireSpeed = PLAYER_FIRE_SPEED;

        playerFire_x = player->x + 2;

        playerFire_y = player->y - 3;

        PLAYER_FIRE_SPEED = PLAYER_FIRE_SPEED - 0.1f <= 2 ? PLAYER_FIRE_SPEED : PLAYER_FIRE_SPEED - 0.1f;  //EVERY TIME FIRE, IT LOOSES SPEED

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

          gameOverStartTime = millis();  // 🔥 start timer
          gameOverHandled = false;       // 🔥 reset flag
        }
      }
    }

    int steps = abs(playerVelX);
    int direction = (playerVelX > 0) ? 1 : -1;

    for (int i = 0; i < steps; i++) {

      player->x += direction;

      player->x = iclamp(player->x,
                         PLAY_AREA_LEFT + 1,
                         PLAY_AREA_RIGHT - player->getWidth() + 1);

      updateSpriteAndDetectCollisions(player);

      if (playerExploding)
        break;
      if (playerExploding || !player->visible)
        break;
    }

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

    if (updateCount % 4 == 0) {

      currentUpdateCount = updateCount;


      if (!gameOver) {
        if (!gameOver) MISSION_TIME++;
        //SCORE++;
        SCREEN_SPEED += 0.05;
        PLAYER_FIRE_SPEED = PLAYER_FIRE_SPEED + 0.0055f >= PLAYER_FIRE_MAX_SPEED ? PLAYER_FIRE_MAX_SPEED : PLAYER_FIRE_SPEED + 0.0055f;
        //if (SCORE % 10 == 0 && PLAYER_AMO_COUNT < PLAYER_MAX_AMO_AUTO) {
        // PLAYER_AMO_COUNT++;
        // }
      }


      float speedIncrease = SCREEN_SPEED / 30;
      ASTEROID_SPEED = 1 + speedIncrease;
      STAR_LAYER3_SPEED = 0.1 + speedIncrease;
      STAR_LAYER0_SPEED = (STAR_LAYER1_SPEED * 2);
      STAR_LAYER1_SPEED = (STAR_LAYER2_SPEED * 2);
      STAR_LAYER2_SPEED = (STAR_LAYER3_SPEED * 2);

      if (gameOver) {

        DisplayController.removeSprites();
        canvas.clear();
        canvas.drawBitmap(0, 0, &ASTRONAUT);  // 🔥 FIRST STORY SCREEN
        //canvas.setBrushColor(Color::Black);
        //canvas.fillRectangle(187, 93, 318, 115);
        canvas.setPenColor(Color::BrightYellow);
        canvas.selectFont(&fabgl::FONT_8x14);
        canvas.drawText(190, 94, "FINTECH INVADERS");
        canvas.selectFont(&fabgl::FONT_4x6);
        //canvas.setPenColor(Color::BrightGreen);
        canvas.drawText(249, 108, "POR ISAAC MOREIRA");
        canvas.drawText(249, 194, "V1.0 (01/04/2026)");

        //canvas.setBrushColor(Color::Blue);
        //canvas.fillRectangle(0, 172, 177, 200);
        canvas.selectFont(&fabgl::FONT_8x14);
        canvas.setPenColor(Color::Red);
        canvas.drawText(1, 174, "GAME OVER");

        unsigned long elapsed = millis() - gameOverStartTime;

        // 🔥 HIGH SCORE → go after 1 second (no input needed)
        if (SCORE > HIGH_SCORE) {

          if (elapsed >= 1000 && !gameOverHandled) {

            gameOverHandled = true;

            HIGH_SCORE = SCORE;

            DisplayController.removeSprites();
            gameState = INPUT_NAME_HI_SCORE;
            stop();
            return;
          }
        } else {

          // 🔥 NORMAL GAME OVER

          // after 5 seconds → go to intro
          if (elapsed >= 5000 && !gameOverHandled) {

            gameOverHandled = true;

            DisplayController.removeSprites();
            gameState = INTRO_SCREEN;
            stop();
            return;
          }

          // OR press SPACE → go immediately
          if (keyboard && keyboard->isKeyboardAvailable() && keyboard->isVKDown(fabgl::VK_SPACE)) {

            if (!gameOverHandled) {

              gameOverHandled = true;

              DisplayController.removeSprites();
              gameState = INTRO_SCREEN;
              stop();
              return;
            }
          }
        }

        return;
      }



      // follow player
      afterburner->x = player->x + 6;
      afterburner->y = player->y + 14;



      ////////////////////////////////////////////////////////
      ////////////////// ASTEROIDS ///////////////////////////

      // ---------- ASTEROID EXPLOSIONS ----------
      for (int i = 0; i < ASTEROID_COUNT; i++) {

        if (asteroidExploding[i]) {

          if (updateCount - asteroidExplosionTimer[i] > 2) {

            asteroidExplosionTimer[i] = updateCount;

            asteroidExplosionFrame[i]++;

            if (asteroidExplosionFrame[i] < EXPLOSION_FRAMES) {

              asteroids[i]->setFrame(asteroidExplosionFrame[i] + 4);

            } else {

              asteroidExploding[i] = false;
              asteroidExplosionFrame[i] = 0;
              asteroidActive[i] = false;

              asteroids[i]->moveTo(-100, -100);
            }
          }
        }
      }


      int asteroidSpawnInterval = (50 - MISSION_TIME / 300) <= 1 ? 1 : (50 - MISSION_TIME / 300);
      if (updateCount % asteroidSpawnInterval == 0)
        spawnAsteroid();

      for (int i = 0; i < ASTEROID_COUNT; i++) {

        if (asteroidActive[i] && !asteroidExploding[i]) {

          int steps = max(1, (int)asteroidSpeed[i]);

          for (int s = 0; s < steps; s++) {
            asteroids[i]->y += 1;

            updateSpriteAndDetectCollisions(asteroids[i]);

            if (asteroidExploding[i] || !asteroids[i]->visible)
              break;
          }

          // asteroid rotation animation
          if (updateCount % 6 == 0) {
            int frame = asteroids[i]->getFrameIndex();
            frame = (frame + 1) % 4;
            asteroids[i]->setFrame(frame);
          }

          if (asteroids[i]->y > DisplayController.getViewPortHeight()) {

            asteroidActive[i] = false;

            asteroids[i]->moveTo(-100, -100);
          }
        }
      }

      if (!peguePagueActive && !peguePagueExploding && MISSION_TIME >= 100) {

        if (random(0, 100) < 2) {  // ~2% chance per update

          peguePagueActive = true;
          peguePagueHits = 0;

          peguePagueExploding = false;
          peguePagueExplosionFrame = 0;
          peguePagueExplosionTimer = 0;
          peguePagueAnimFrame = 0;
          peguePagueAnimDir = 1;
          peguePagueAnimTimer = currentUpdateCount;

          int lane = random(0, PLAYER_AREA_LANES);
          peguePagueX = PLAY_AREA_LEFT + lane * 32 + 8;

          peguePague->setFrame(0);
          peguePague->moveTo((int)peguePagueX, -16);
        }
      }

      // ---------- FEIRAPAGA SPAWN ----------
      bool feiraCondition = (PLAYER_FIRE_SPEED >= 3.0f) || (MISSION_TIME >= 1000);

      if (!feiraPagaActive && !feiraPagaExploding && feiraCondition) {

        if (random(0, 100) < 1) {  // rarer than PeguePague

          feiraPagaActive = true;
          feiraPagaHits = 0;

          feiraPagaExploding = false;
          feiraPagaExplosionFrame = 0;
          feiraPagaExplosionTimer = 0;

          feiraPagaAnimFrame = 0;
          feiraPagaAnimTimer = currentUpdateCount;

          int lane = random(0, PLAYER_AREA_LANES);
          feiraPagaX = PLAY_AREA_LEFT + lane * 32 + 8;

          feiraPaga->setFrame(0);
          feiraPaga->moveTo((int)feiraPagaX, -16);
        }
      }

      bool outerCondition = (MISSION_TIME >= OUTER_RESPAWN_INTERVAL);

      if (!outerActive && !outerExploding && outerCondition) {

        if (random(0, 1000) < 5) {

          outerActive = true;
          outerHits = 0;

          outerExploding = false;
          outerExplosionFrame = 0;
          outerExplosionTimer = 0;

          outerAnimFrame = 0;
          outerAnimTimer = currentUpdateCount;

          int lane = random(0, PLAYER_AREA_LANES);
          outerX = PLAY_AREA_LEFT + lane * 32 + 8;

          outer->setFrame(0);
          outer->moveTo((int)outerX, -16);
        }
      }
      // ---------- PEGUEPAGUE EXPLOSION ----------
      if (peguePagueExploding) {

        if (updateCount - peguePagueExplosionTimer > 2) {

          peguePagueExplosionTimer = updateCount;
          peguePagueExplosionFrame++;

          if (peguePagueExplosionFrame < EXPLOSION_FRAMES) {

            peguePague->setFrame(peguePagueExplosionFrame + 4);

          } else {

            peguePagueExploding = false;
            peguePagueActive = false;

            peguePague->setFrame(0);  // reset sprite to normal ship

            peguePague->moveTo(-100, -100);

            peguePagueRespawnTime = MISSION_TIME + PEGUE_PAG_RESPAWN_INTERVAL;
          }
        }
      }

      // ---------- FEIRAPAGA EXPLOSION ----------
      if (feiraPagaExploding) {
        feiraPagaFireActive = false;
        feiraPagaFire->visible = false;
        feiraPagaFire->moveTo(-100, -100);

        if (updateCount - feiraPagaExplosionTimer > 2) {

          feiraPagaExplosionTimer = updateCount;
          feiraPagaExplosionFrame++;

          if (feiraPagaExplosionFrame < EXPLOSION_FRAMES) {

            feiraPaga->setFrame(feiraPagaExplosionFrame + 4);

          } else {

            feiraPagaExploding = false;
            feiraPagaActive = false;

            feiraPaga->setFrame(0);
            feiraPaga->moveTo(-100, -100);
          }
        }
      }

      if (peguePagueActive) {

        // ---------- PEGUEPAGUE ANIMATION ----------
        if (currentUpdateCount - peguePagueAnimTimer > 4) {

          peguePagueAnimTimer = currentUpdateCount;

          peguePagueAnimFrame += peguePagueAnimDir;

          // bounce at ends
          if (peguePagueAnimFrame >= 3) {
            peguePagueAnimFrame = 3;
            peguePagueAnimDir = -1;
          } else if (peguePagueAnimFrame <= 0) {
            peguePagueAnimFrame = 0;
            peguePagueAnimDir = 1;
          }

          peguePague->setFrame(peguePagueAnimFrame);
        }


        // TARGET = EXACT PLAYER X (no prediction, no float drift)
        int targetX = player->x;

        // difference
        int dx = targetX - peguePague->x;

        // speed limit (tweakable)
        int maxStep = 1;

        // clamp movement
        if (dx > maxStep) dx = maxStep;
        else if (dx < -maxStep) dx = -maxStep;

        // apply movement
        peguePague->x += dx;


        peguePague->y += 1;

        // single collision check AFTER movement
        updateSpriteAndDetectCollisions(peguePague);

        if (peguePague->y > DisplayController.getViewPortHeight()) {

          peguePagueActive = false;
          peguePagueExploding = false;
          peguePagueHits = 0;

          peguePague->moveTo(-100, -100);

          peguePagueRespawnTime = MISSION_TIME + FEIRAPAGA_RESPAWN_INTERVAL;
        }
      }

      // ---------- FEIRAPAGA ANIMATION (LOOP) ----------
      if (feiraPagaActive && !feiraPagaExploding) {

        if (currentUpdateCount - feiraPagaAnimTimer > 4) {

          feiraPagaAnimTimer = currentUpdateCount;

          feiraPagaAnimFrame = (feiraPagaAnimFrame + 1) % 4;  // LOOP

          feiraPaga->setFrame(feiraPagaAnimFrame);
        }
      }



      if (feiraPagaActive && !feiraPagaExploding) {

        int targetX = player->x;
        int dx = targetX - feiraPaga->x;

        int maxStep = 2;

        if (dx > maxStep) dx = maxStep;
        else if (dx < -maxStep) dx = -maxStep;

        feiraPaga->x += dx;
        feiraPaga->y += 1;//MAKE STATIONARY?

        // ---------- FEIRAPAGA SHOOT ----------
        if (!feiraPagaFireActive) {

          if (random(0, 100) < 5) {  // tweak difficulty here

            feiraPagaFireActive = true;

            feiraPagaFire->visible = true;

            feiraPagaFire->moveTo(
              feiraPaga->x + 5,  // adjust to center of sprite
              feiraPaga->y + 10);
          }
        }

        updateSpriteAndDetectCollisions(feiraPaga);

        if (feiraPaga->y > DisplayController.getViewPortHeight()) {

          feiraPagaActive = false;
          feiraPagaExploding = false;
          feiraPagaHits = 0;

          feiraPaga->moveTo(-100, -100);

          feiraPagaFireActive = false;
          feiraPagaFire->visible = false;
          feiraPagaFire->moveTo(-100, -100);
        }
      }

      if (outerActive && !outerExploding) {

        if (currentUpdateCount - outerAnimTimer > 4) {

          outerAnimTimer = currentUpdateCount;

          outerAnimFrame = (outerAnimFrame + 1) % 4;

          outer->setFrame(outerAnimFrame);
        }
      }

      if (outerActive && !outerExploding) {

        int targetX = player->x;
        int dx = targetX - outer->x;

        int maxStep = 5;  // horizontal speed

        if (dx > maxStep) dx = maxStep;
        else if (dx < -maxStep) dx = -maxStep;

        outer->x += dx;

        // vertical speed = 3
        outer->y += 5;

        updateSpriteAndDetectCollisions(outer);

        if (outer->y > DisplayController.getViewPortHeight()) {

          outerActive = false;
          outerExploding = false;
          outerHits = 0;

          outer->moveTo(-100, -100);
        }
      }

      if (outerExploding) {

        if (updateCount - outerExplosionTimer > 2) {

          outerExplosionTimer = updateCount;
          outerExplosionFrame++;

          if (outerExplosionFrame < EXPLOSION_FRAMES) {

            outer->setFrame(outerExplosionFrame + 4);

          } else {

            outerExploding = false;
            outerActive = false;

            outer->setFrame(0);
            outer->moveTo(-100, -100);
          }
        }
      }

      ////////////////////////////////////////////////////////
      ////////////////// FIRE /////////////////////////////////

      if (playerFired) {

        int steps = max(1, (int)playerFireSpeed);

        for (int i = 0; i < steps; i++) {

          if (!playerFire->visible)
            break;

          playerFire->y -= 1;

          updateSpriteAndDetectCollisions(playerFire);

          if (!playerFire->visible)
            break;

          if (playerFire->y <= 0) {
            playerFired = false;
            playerFire->visible = false;
            break;
          }
        }
      }

      // ---------- FEIRAPAGA FIRE UPDATE ----------
      if (feiraPagaFireActive) {

        int steps = max(1, (int)feiraPagaFireSpeed);

        for (int i = 0; i < steps; i++) {

          if (!feiraPagaFire->visible)
            break;

          feiraPagaFire->y += feiraPagaFireSpeed;

          updateSpriteAndDetectCollisions(feiraPagaFire);

          if (!feiraPagaFire->visible)
            break;

          if (feiraPagaFire->y > DisplayController.getViewPortHeight()) {

            feiraPagaFireActive = false;
            feiraPagaFire->visible = false;
            feiraPagaFire->moveTo(-100, -100);

            break;
          }
        }
      }

      ////////////////////////////////////////////////////////

      updateStars();

      updateSprite(afterburner);


      DisplayController.refreshSprites();
    }
  }

  //////////////////////////////////////////////////////////////////////
  ////////////////////////// COLLISIONS ////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  void collisionDetected(Sprite* spriteA, Sprite* spriteB, Point) override {

    int indexA = spriteA - sprites;
    int indexB = spriteB - sprites;

    SpriteType typeA = spriteType[indexA];
    SpriteType typeB = spriteType[indexB];

    if ((typeA == TYPE_PLAYER && typeB == TYPE_OUTER) || (typeB == TYPE_PLAYER && typeA == TYPE_OUTER)) {

      if (!playerExploding) {
        playerExploding = true;
        playerExplosionFrame = 1;
        playerExplosionTimer = currentUpdateCount;
      }

      if (!outerActive || outerExploding)
        return;

      if (SCORE >= 50)
        SCORE -= 50;
      else
        SCORE = 0;

      outerExploding = true;
      outerExplosionFrame = 0;
      outerExplosionTimer = currentUpdateCount;

      outer->setFrame(1);
    }

    // ---------- FEIRAPAGA FIRE vs PLAYER ----------
    if ((typeA == TYPE_FEIRAPAGA_FIRE && typeB == TYPE_PLAYER) || (typeB == TYPE_FEIRAPAGA_FIRE && typeA == TYPE_PLAYER)) {

      if (!playerExploding && playerLifeCount == 0) {
        playerExploding = true;
        playerExplosionFrame = 1;
        playerExplosionTimer = currentUpdateCount;
      } else {
        playerLifeCount--;
      }

      feiraPagaFireActive = false;
      feiraPagaFire->visible = false;
      feiraPagaFire->moveTo(-100, -100);

      return;
    }



    if ((typeA == TYPE_PLAYER && typeB == TYPE_ASTEROID) || (typeB == TYPE_PLAYER && typeA == TYPE_ASTEROID)) {

      Sprite* asteroid = (typeA == TYPE_ASTEROID) ? spriteA : spriteB;

      STAR_LAYER3_SPEED = 0;

      playerFire->visible = false;
      playerFired = false;
      playerFire->moveTo(-100, -100);

      // trigger asteroid explosion
      for (int i = 0; i < ASTEROID_COUNT; i++) {
        asteroidSpeed[i] = 0;

        if (asteroids[i] == asteroid && !asteroidExploding[i]) {

          asteroidExploding[i] = true;
          asteroidExplosionFrame[i] = 1;
          asteroidExplosionTimer[i] = currentUpdateCount;

          asteroids[i]->setFrame(4);

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

      SCORE++;

      for (int i = 0; i < ASTEROID_COUNT; i++) {

        if (asteroids[i] == asteroid && !asteroidExploding[i]) {

          asteroidExploding[i] = true;
          asteroidExplosionFrame[i] = 1;
          asteroidExplosionTimer[i] = currentUpdateCount;

          asteroids[i]->setFrame(4);

          playerFire->visible = false;
          playerFired = false;
          playerFire->moveTo(-100, -100);

          return;  // ← ADD THIS
        }
      }
    }

    if ((typeA == TYPE_PLAYER_FIRE && typeB == TYPE_OUTER) || (typeB == TYPE_PLAYER_FIRE && typeA == TYPE_OUTER)) {

      if (!outerActive || outerExploding)
        return;
      if (!playerFire->visible)
        return;

      playerFire->visible = false;
      playerFired = false;
      playerFire->moveTo(-100, -100);

      outerHits++;

      if (outerHits >= OUTER_KILL_HITS) {

        SCORE += 5;             // stronger reward
        PLAYER_AMO_COUNT += 2;  // bigger reward

        outerExploding = true;
        outerExplosionFrame = 0;
        outerExplosionTimer = currentUpdateCount;

        outer->setFrame(1);
      }

      return;
    }

    if ((typeA == TYPE_PLAYER_FIRE && typeB == TYPE_PEGUEPAGUE) || (typeB == TYPE_PLAYER_FIRE && typeA == TYPE_PEGUEPAGUE)) {

      if (!peguePagueActive || peguePagueExploding)
        return;
      if (!playerFire->visible)
        return;

      playerFire->visible = false;
      playerFired = false;
      playerFire->moveTo(-100, -100);

      peguePagueHits++;

      if (peguePagueHits >= PEGUE_PAGUE_KILL_HITS) {

        SCORE += 30;
        PLAYER_AMO_COUNT += 7;

        peguePagueExploding = true;
        peguePagueExplosionFrame = 0;
        peguePagueExplosionTimer = currentUpdateCount;

        peguePague->setFrame(1);
      }

      return;
    }

    if ((typeA == TYPE_PLAYER && typeB == TYPE_PEGUEPAGUE) || (typeB == TYPE_PLAYER && typeA == TYPE_PEGUEPAGUE)) {

      if (!playerExploding) {
        playerExploding = true;
        playerExplosionFrame = 1;
        playerExplosionTimer = currentUpdateCount;
      }

      if (!peguePagueActive || peguePagueExploding)
        return;

      if (SCORE >= 25)
        SCORE -= 25;
      else
        SCORE = 0;

      peguePagueExploding = true;
      peguePagueExplosionFrame = 0;
      peguePagueExplosionTimer = currentUpdateCount;

      peguePague->setFrame(1);

      // kill player
      playerExploding = true;
      playerExplosionFrame = 1;
      playerExplosionTimer = currentUpdateCount;
    }
    if ((typeA == TYPE_PLAYER_FIRE && typeB == TYPE_FEIRAPAGA) || (typeB == TYPE_PLAYER_FIRE && typeA == TYPE_FEIRAPAGA)) {

      if (!feiraPagaActive || feiraPagaExploding)
        return;
      if (!playerFire->visible)
        return;

      playerFire->visible = false;
      playerFired = false;
      playerFire->moveTo(-100, -100);

      feiraPagaHits++;

      if (feiraPagaHits >= FEIRAPAGA_KILL_HITS) {

        SCORE += 30;
        PLAYER_AMO_COUNT += 10;

        feiraPagaExploding = true;
        feiraPagaExplosionFrame = 0;
        feiraPagaExplosionTimer = currentUpdateCount;

        feiraPaga->setFrame(1);
      }

      return;
    }

    if ((typeA == TYPE_PLAYER && typeB == TYPE_FEIRAPAGA) || (typeB == TYPE_PLAYER && typeA == TYPE_FEIRAPAGA)) {

      if (!playerExploding) {
        playerExploding = true;
        playerExplosionFrame = 1;
        playerExplosionTimer = currentUpdateCount;
      }

      if (!feiraPagaActive || feiraPagaExploding)
        return;

      if (SCORE >= 30)
        SCORE -= 30;
      else
        SCORE = 0;

      feiraPagaExploding = true;
      feiraPagaExplosionFrame = 0;
      feiraPagaExplosionTimer = currentUpdateCount;

      feiraPaga->setFrame(1);
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

        asteroidSpeed[i] = ASTEROID_SPEED;

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
  DisplayController.setResolution(VGA_320x200_70Hz);
  DisplayController.moveScreen(21, 1);

  EEPROM.begin(EEPROM_SIZE);

  // 🔥 FORCE RESET
  HIGH_SCORE = 0;
  strcpy(HIGH_SCORE_NAME, "PLAYER");

  saveHighScore();  // already has commit()

  // loadHighScore(); ❌ not needed after reset
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

  if (gameState == INPUT_NAME_HI_SCORE) {
    InputNameScene input;
    input.start();
  }
}