#include <Wire.h>
#include <MPU6050.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

MPU6050 mpu;

const int buttonPin = 2;
const int switchPin = 3;
const int potPin = A0;

float angleX = 0, angleY = 0;
float gyroXoffset = 0, gyroYoffset = 0;

int gameMode = 0;
unsigned long lastSwitchTime = 0;

int pingPongScore = 0;
int airCombatScore = 0;
int crawlerScore = 0;

void setup() {
  Serial.begin(9600);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(switchPin, INPUT_PULLUP);
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("OLED initialization failed"));
    while(1);
  }
  mpu.initialize();
  if (!mpu.testConnection()) {
    display.println("MPU6050 Error!");
    display.display();
    while(1);
  }
  calibrateGyro();
  showMenu();
}

void calibrateGyro() {
  display.clearDisplay();
  display.println("Calibrating Gyro...");
  display.println("Keep sensor still!");
  display.display();
  long gx = 0, gy = 0;
  const int samples = 500;
  for(int i=0; i<samples; i++) {
    int16_t gx_temp, gy_temp, gz_temp;
    mpu.getRotation(&gx_temp, &gy_temp, &gz_temp);
    gx += gx_temp;
    gy += gy_temp;
    delay(5);
  }
  gyroXoffset = gx / samples;
  gyroYoffset = gy / samples;
}

void showMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Select Game:");
  display.println("1. Ping Pong (Gyro)");
  display.println("2. Air Combat (Pot)");
  display.println("3. The Crawler (Both)");
  display.display();
  delay(2000);
}

void loop() {
  if(digitalRead(switchPin) == LOW && millis() - lastSwitchTime > 500) {
    gameMode = (gameMode + 1) % 3;
    lastSwitchTime = millis();
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Selected Game: ");
    display.println(gameMode + 1);
    display.display();
    delay(1000);
  }
  readGyro();
  switch(gameMode) {
    case 0: 
      pingPongGame();
      break;
    case 1:
      airCombatGame();
      break;
    case 2:
      crawlerGame();
      break;
  }
}

void readGyro() {
  static unsigned long lastTime = 0;
  float dt = (millis() - lastTime) / 1000.0;
  lastTime = millis();
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  float gyroXrate = (gx - gyroXoffset) / 131.0;
  float gyroYrate = (gy - gyroYoffset) / 131.0;
  float accAngleX = atan2(ay, az) * 180/PI;
  float accAngleY = atan2(-ax, az) * 180/PI;
  angleX = 0.96 * (angleX + gyroXrate * dt) + 0.04 * accAngleX;
  angleY = 0.96 * (angleY + gyroYrate * dt) + 0.04 * accAngleY;
}

int paddleY = SCREEN_HEIGHT/2;
int ballX = SCREEN_WIDTH/2;
int ballY = SCREEN_HEIGHT/2;
int ballSpeedX = 2;
int ballSpeedY = 2;
const int paddleHeight = 15;
const int paddleWidth = 3;
bool gameStarted = false;

void pingPongGame() {
  paddleY = map(angleY, -30, 30, 0, SCREEN_HEIGHT - paddleHeight);
  if(digitalRead(buttonPin) == LOW && !gameStarted) {
    gameStarted = true;
    ballSpeedX = abs(ballSpeedX);
    ballSpeedY = random(-2, 3);
    if(ballSpeedY == 0) ballSpeedY = 1;
  }
  if(gameStarted) {
    ballX += ballSpeedX;
    ballY += ballSpeedY;
    if(ballY <= 0 || ballY >= SCREEN_HEIGHT-1) {
      ballSpeedY *= -1;
      ballY = constrain(ballY, 1, SCREEN_HEIGHT-2);
    }
    if(ballX <= paddleWidth && ballY >= paddleY && ballY <= paddleY + paddleHeight) {
      ballSpeedX = abs(ballSpeedX);
      ballSpeedX += 0.5;
      ballSpeedY += (ballY - (paddleY + paddleHeight/2)) * 0.05;
      pingPongScore+=1;
    }
    if(ballX >= SCREEN_WIDTH-1) {
      ballSpeedX = -abs(ballSpeedX);
      ballX = SCREEN_WIDTH-2;
    }
    if(ballX < 0) {
      pingPongScore = max(0, pingPongScore - 1);
      gameStarted = false;
      ballX = SCREEN_WIDTH/2;
      ballY = SCREEN_HEIGHT/2;
      ballSpeedX = 2;
      ballSpeedY = 2;
    }
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(SCREEN_WIDTH-80, 0);
  display.print("Score:");
  display.setCursor(SCREEN_WIDTH-30, 0);
  display.print(pingPongScore);
  display.fillRect(0, paddleY, paddleWidth, paddleHeight, WHITE);
  display.drawFastVLine(SCREEN_WIDTH-1, 0, SCREEN_HEIGHT, WHITE);
  display.fillCircle(ballX, ballY, 2, WHITE);
  if(!gameStarted) {
    display.setCursor(SCREEN_WIDTH/2-30, SCREEN_HEIGHT/2);
    display.print("Press Button");
  }
  display.display();
  delay(20);
}

int planeX = SCREEN_WIDTH/2;
int planeY = SCREEN_HEIGHT-10;
int bulletX = -1;
int bulletY = -1;
int enemyX = random(10, SCREEN_WIDTH-10);
int enemyY = 10;
int enemySpeed = 1;
unsigned long lastEnemySpawn = 0;

void airCombatGame() {
  planeX = map(analogRead(potPin), 0, 1023, 10, SCREEN_WIDTH-10);
  if(digitalRead(buttonPin) == LOW && bulletY < 0) {
    bulletX = planeX;
    bulletY = planeY - 5;
  }
  if(bulletY >= 0) {
    bulletY -= 3;
    if(bulletY < 0) bulletY = -1;
  }
  enemyX += enemySpeed;
  if(enemyX <= 0 || enemyX >= SCREEN_WIDTH-5) {
    enemySpeed *= -1;
    enemyY += 5;
  }
  if(bulletY >=0 && abs(bulletX - enemyX) < 5 && abs(bulletY - enemyY) < 5) {
    airCombatScore++;
    bulletY = -1;
    enemyX = random(10, SCREEN_WIDTH-10);
    enemyY = 10;
    enemySpeed = random(1, 3) * (random(0,2) ? 1 : -1);
  }
  if(abs(planeX - enemyX) < 8 && abs(planeY - enemyY) < 8) {
    airCombatScore = max(0, airCombatScore - 1);
    enemyX = random(10, SCREEN_WIDTH-10);
    enemyY = 10;
    enemySpeed = random(1, 3) * (random(0,2) ? 1 : -1);
  }
  if(millis() - lastEnemySpawn > 5000 && enemyY > SCREEN_HEIGHT/2) {
    lastEnemySpawn = millis();
    enemyX = random(10, SCREEN_WIDTH-10);
    enemyY = 10;
    enemySpeed = random(1, 3) * (random(0,2) ? 1 : -1);
  }
  if(enemyY > SCREEN_HEIGHT) {
    enemyX = random(10, SCREEN_WIDTH-10);
    enemyY = 10;
    enemySpeed = random(1, 3) * (random(0,2) ? 1 : -1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(SCREEN_WIDTH-80, 0);
  display.print("Score:");
  display.setCursor(SCREEN_WIDTH-30, 0);
  display.print(airCombatScore);
  display.fillTriangle(planeX, planeY, planeX-5, planeY+10, planeX+5, planeY+10, WHITE);
  if(bulletY >= 0) display.fillCircle(bulletX, bulletY, 2, WHITE);
  display.fillTriangle(enemyX, enemyY, enemyX-5, enemyY+5, enemyX+5, enemyY+5, WHITE);
  display.display();
  delay(50);
}

int crawlerX = SCREEN_WIDTH/2;
int crawlerY = SCREEN_HEIGHT/2;
int targetX = random(10, SCREEN_WIDTH-10);
int targetY = random(10, SCREEN_HEIGHT-10);
unsigned long lastTargetTime = 0;
int targetRadius = 3;

void crawlerGame() {
  crawlerX = map(analogRead(potPin), 0, 1023, 5, SCREEN_WIDTH-5);
  crawlerY = map(angleX, -30, 30, 5, SCREEN_HEIGHT-5);
  if(abs(crawlerX - targetX) < (5 + targetRadius) && abs(crawlerY - targetY) < (5 + targetRadius)) {
    crawlerScore++;
    targetX = random(10, SCREEN_WIDTH-10);
    targetY = random(10, SCREEN_HEIGHT-10);
    lastTargetTime = millis();
    targetRadius = 3;
  }
  if(millis() - lastTargetTime > 3000) {
    targetRadius = max(1, targetRadius - 1);
    lastTargetTime = millis();
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(SCREEN_WIDTH-80, 0);
  display.print("Score:");
  display.setCursor(SCREEN_WIDTH-30, 0);
  display.print(crawlerScore);
  display.fillCircle(crawlerX, crawlerY, 5, WHITE);
  display.drawCircle(targetX, targetY, targetRadius, WHITE);
  display.setCursor(0, 0);
  display.print("Time: ");
  display.print(3 - (millis() - lastTargetTime)/1000);
  display.display();
  delay(50);
}
