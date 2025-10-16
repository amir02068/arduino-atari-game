#include <Wire.h>
#include <MPU6050.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

MPU6050 mpu;

// پین‌های کنترل
const int buttonPin = 2;    // دکمه انتخاب/شلیک
const int switchPin = 3;    // سوئیچ تغییر بازی
const int potPin = A0;      // پتانسیومتر

// متغیرهای ژیروسکوپ
float angleX = 0, angleY = 0;
float gyroXoffset = 0, gyroYoffset = 0;

// متغیرهای حالت بازی
int gameMode = 0; // 0: پینگ پنگ (ژیروسکوپ)، 1: جنگ هوایی (پتانسیومتر)، 2: خزنده (ترکیبی)
unsigned long lastSwitchTime = 0;

// متغیرهای امتیاز
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
  // تغییر بازی با سوئیچ
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

  // خواندن ژیروسکوپ
  readGyro();

  // اجرای بازی انتخاب شده
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

// ----------------------
// بازی پینگ پنگ (کنترل با ژیروسکوپ)
// ----------------------
int paddleY = SCREEN_HEIGHT/2;
int ballX = SCREEN_WIDTH/2;
int ballY = SCREEN_HEIGHT/2;
int ballSpeedX = 2;
int ballSpeedY = 2;
const int paddleHeight = 15;
const int paddleWidth = 3;
bool gameStarted = false;

void pingPongGame() {
  // کنترل پدل با ژیروسکوپ (محور X)
  paddleY = map(angleY, -30, 30, 0, SCREEN_HEIGHT - paddleHeight);
  
  // شروع بازی با دکمه
  if(digitalRead(buttonPin) == LOW && !gameStarted) {
    gameStarted = true;
    ballSpeedX = abs(ballSpeedX); // همیشه به سمت راست شروع شود
    ballSpeedY = random(-2, 3); // جهت تصادفی عمودی
    if(ballSpeedY == 0) ballSpeedY = 1;
  }
  
  if(gameStarted) {
    // حرکت توپ
    ballX += ballSpeedX;
    ballY += ballSpeedY;
    
    // برخورد با دیوارهای بالا و پایین
    if(ballY <= 0 || ballY >= SCREEN_HEIGHT-1) {
      ballSpeedY *= -1;
      ballY = constrain(ballY, 1, SCREEN_HEIGHT-2);
    }
    
    // برخورد با پدل چپ
    if(ballX <= paddleWidth && ballY >= paddleY && ballY <= paddleY + paddleHeight) {
      ballSpeedX = abs(ballSpeedX); // به راست برگردد
      // افزایش سرعت پس از هر ضربه
      ballSpeedX += 0.5;
      ballSpeedY += (ballY - (paddleY + paddleHeight/2)) * 0.05;
      pingPongScore+=1;
    }
    
    // برخورد با دیوار راست
    if(ballX >= SCREEN_WIDTH-1) {
      ballSpeedX = -abs(ballSpeedX); // به چپ برگردد
      ballX = SCREEN_WIDTH-2;
    }
    
    // اگر توپ از سمت چپ خارج شد (امتیاز منفی)
    if(ballX < 0) {
      pingPongScore = max(0, pingPongScore - 1);
      gameStarted = false;
      ballX = SCREEN_WIDTH/2;
      ballY = SCREEN_HEIGHT/2;
      ballSpeedX = 2;
      ballSpeedY = 2;
    }
  }
  
  // رسم بازی
  display.clearDisplay();
  
  // نمایش امتیاز
  display.setTextSize(1);
  display.setCursor(SCREEN_WIDTH-80, 0);
  display.print("Score:");
  display.setCursor(SCREEN_WIDTH-30, 0);
  display.print(pingPongScore);
  
  // پدل
  display.fillRect(0, paddleY, paddleWidth, paddleHeight, WHITE);
  
  // دیوار مقابل
  display.drawFastVLine(SCREEN_WIDTH-1, 0, SCREEN_HEIGHT, WHITE);
  
  // توپ
  display.fillCircle(ballX, ballY, 2, WHITE);
  
  if(!gameStarted) {
    display.setCursor(SCREEN_WIDTH/2-30, SCREEN_HEIGHT/2);
    display.print("Press Button");
  }
  
  display.display();
  delay(20);
}

// ----------------------
// بازی جنگ هوایی (کنترل با پتانسیومتر)
// ----------------------
int planeX = SCREEN_WIDTH/2;
int planeY = SCREEN_HEIGHT-10;
int bulletX = -1;
int bulletY = -1;
int enemyX = random(10, SCREEN_WIDTH-10);
int enemyY = 10;
int enemySpeed = 1;
unsigned long lastEnemySpawn = 0;

void airCombatGame() {
  // کنترل هواپیما با پتانسیومتر
  planeX = map(analogRead(potPin), 0, 1023, 10, SCREEN_WIDTH-10);
  
  // شلیک گلوله
  if(digitalRead(buttonPin) == LOW && bulletY < 0) {
    bulletX = planeX;
    bulletY = planeY - 5;
  }
  
  // حرکت گلوله
  if(bulletY >= 0) {
    bulletY -= 3;
    if(bulletY < 0) bulletY = -1;
  }
  
  // حرکت دشمن
  enemyX += enemySpeed;
  if(enemyX <= 0 || enemyX >= SCREEN_WIDTH-5) {
    enemySpeed *= -1;
    enemyY += 5;
  }
  
  // تشخیص برخورد گلوله با دشمن
  if(bulletY >=0 && abs(bulletX - enemyX) < 5 && abs(bulletY - enemyY) < 5) {
    airCombatScore++;
    bulletY = -1;
    enemyX = random(10, SCREEN_WIDTH-10);
    enemyY = 10;
    enemySpeed = random(1, 3) * (random(0,2) ? 1 : -1);
  }
  
  // تشخیص برخورد دشمن با هواپیما
  if(abs(planeX - enemyX) < 8 && abs(planeY - enemyY) < 8) {
    airCombatScore = max(0, airCombatScore - 1);
    enemyX = random(10, SCREEN_WIDTH-10);
    enemyY = 10;
    enemySpeed = random(1, 3) * (random(0,2) ? 1 : -1);
  }
  
  // ایجاد دشمن جدید هر 5 ثانیه
  if(millis() - lastEnemySpawn > 5000 && enemyY > SCREEN_HEIGHT/2) {
    lastEnemySpawn = millis();
    enemyX = random(10, SCREEN_WIDTH-10);
    enemyY = 10;
    enemySpeed = random(1, 3) * (random(0,2) ? 1 : -1);
  }
  
  // اگر دشمن از پایین خارج شد
  if(enemyY > SCREEN_HEIGHT) {
    enemyX = random(10, SCREEN_WIDTH-10);
    enemyY = 10;
    enemySpeed = random(1, 3) * (random(0,2) ? 1 : -1);
  }
  
  // رسم بازی
  display.clearDisplay();
  
  // نمایش امتیاز
  display.setTextSize(1);
  display.setCursor(SCREEN_WIDTH-80, 0);
  display.print("Score:");
  display.setCursor(SCREEN_WIDTH-30, 0);
  display.print(airCombatScore);
  
  // هواپیمای بازیکن
  display.fillTriangle(
    planeX, planeY,
    planeX-5, planeY+10,
    planeX+5, planeY+10,
    WHITE
  );
  
  // گلوله
  if(bulletY >= 0) display.fillCircle(bulletX, bulletY, 2, WHITE);
  
  // دشمن
  display.fillTriangle(
    enemyX, enemyY,
    enemyX-5, enemyY+5,
    enemyX+5, enemyY+5,
    WHITE
  );
  
  display.display();
  delay(50);
}

// ----------------------
// بازی خزنده (کنترل ترکیبی)
// ----------------------
int crawlerX = SCREEN_WIDTH/2;
int crawlerY = SCREEN_HEIGHT/2;
int targetX = random(10, SCREEN_WIDTH-10);
int targetY = random(10, SCREEN_HEIGHT-10);
unsigned long lastTargetTime = 0;
int targetRadius = 3;

void crawlerGame() {
  // کنترل X با پتانسیومتر و Y با ژیروسکوپ
  crawlerX = map(analogRead(potPin), 0, 1023, 5, SCREEN_WIDTH-5);
  crawlerY = map(angleX, -30, 30, 5, SCREEN_HEIGHT-5);
  
  // تشخیص برخورد با هدف
  if(abs(crawlerX - targetX) < (5 + targetRadius) && abs(crawlerY - targetY) < (5 + targetRadius)) {
    crawlerScore++;
    targetX = random(10, SCREEN_WIDTH-10);
    targetY = random(10, SCREEN_HEIGHT-10);
    lastTargetTime = millis();
    targetRadius = 3;
  }
  
  // تغییر اندازه هدف با گذشت زمان
  if(millis() - lastTargetTime > 3000) {
    targetRadius = max(1, targetRadius - 1);
    lastTargetTime = millis();
  }
  
  // رسم بازی
  display.clearDisplay();
  
  // نمایش امتیاز
  display.setTextSize(1);
  display.setCursor(SCREEN_WIDTH-80, 0);
  display.print("Score:");
  display.setCursor(SCREEN_WIDTH-30, 0);
  display.print(crawlerScore);
  
  // خزنده
  display.fillCircle(crawlerX, crawlerY, 5, WHITE);
  
  // هدف
  display.drawCircle(targetX, targetY, targetRadius, WHITE);
  
  // نمایش زمان باقیمانده برای کوچک شدن هدف
  display.setCursor(0, 0);
  display.print("Time: ");
  display.print(3 - (millis() - lastTargetTime)/1000);
  
  display.display();
  delay(50);
}
