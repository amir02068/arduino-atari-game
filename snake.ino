
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int potX = A0;
const int potY = A1;

#define SNAKE_MAX_LENGTH 50
int snakeX[SNAKE_MAX_LENGTH];
int snakeY[SNAKE_MAX_LENGTH];
int snakeLength = 3;
int snakeDir = 1;
int foodX, foodY;
unsigned long lastSnakeMove = 0;
int snakeDelay = 200;
int snakeScore = 0;

void initSnake() {
  snakeLength = 3;
  snakeX[0] = SCREEN_WIDTH / 2;
  snakeY[0] = SCREEN_HEIGHT / 2;
  for (int i = 1; i < snakeLength; i++) {
    snakeX[i] = snakeX[i - 1] - 3;
    snakeY[i] = snakeY[i - 1];
  }
  snakeDir = 1;
  spawnFood();
  snakeScore = 0;
  snakeDelay = 200;
}

void spawnFood() {
  foodX = random(0, SCREEN_WIDTH / 3) * 3;
  foodY = random(0, SCREEN_HEIGHT / 3) * 3;
}

void setup() {
  Serial.begin(9600);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 20);
  display.println("Nokia Snake Game");
  display.setCursor(20, 40);
  display.println("Move Pots to start");
  display.display();
  delay(2000);
  initSnake();
}

void loop() {
  int xValue = analogRead(potX);
  int yValue = analogRead(potY);

  if (xValue < 300 && snakeDir != 1) snakeDir = 3;
  else if (xValue > 700 && snakeDir != 3) snakeDir = 1;
  if (yValue < 300 && snakeDir != 2) snakeDir = 0;
  else if (yValue > 700 && snakeDir != 0) snakeDir = 2;

  if (millis() - lastSnakeMove > snakeDelay) {
    lastSnakeMove = millis();
    for (int i = snakeLength - 1; i > 0; i--) {
      snakeX[i] = snakeX[i - 1];
      snakeY[i] = snakeY[i - 1];
    }
    if (snakeDir == 0) snakeY[0] -= 3;
    else if (snakeDir == 1) snakeX[0] += 3;
    else if (snakeDir == 2) snakeY[0] += 3;
    else if (snakeDir == 3) snakeX[0] -= 3;
    if (snakeX[0] < 0 || snakeX[0] >= SCREEN_WIDTH || snakeY[0] < 0 || snakeY[0] >= SCREEN_HEIGHT) {
      gameOver();
      return;
    }
    for (int i = 1; i < snakeLength; i++) {
      if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
        gameOver();
        return;
      }
    }
    int dx = abs(snakeX[0] - foodX);
    int dy = abs(snakeY[0] - foodY);
    if (dx < 3 && dy < 3) {
      snakeLength++;
      snakeScore++;
      if (snakeDelay > 50) snakeDelay -= 10;
      spawnFood();
    }
    drawGame();
  }
}

void drawGame() {
  display.clearDisplay();
  display.fillRect(foodX, foodY, 3, 3, WHITE);
  for (int i = 0; i < snakeLength; i++) {
    display.fillRect(snakeX[i], snakeY[i], 3, 3, WHITE);
  }
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Score: ");
  display.print(snakeScore);
  display.display();
}

void gameOver() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(20, 20);
  display.println("GAME OVER");
  display.setTextSize(1);
  display.setCursor(30, 50);
  display.print("Score: ");
  display.print(snakeScore);
  display.display();
  delay(3000);
  initSnake();
}
