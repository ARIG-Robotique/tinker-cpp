#include <config.h>
#include <Arduino.h>
#include <Wire.h>
#include <I2CUtils.h>
#include <MD22.h>
#include <PS2X_lib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FastLED.h>
#include <Adafruit_PWMServoDriver.h>

// PSX Configuration
#define PS2_DAT        9  //14    
#define PS2_CMD        10  //15
#define PS2_SEL        11  //16
#define PS2_CLK        12  //17
#define pressures      false // analog reading of push-butttons 
#define rumble         true // motor rumbling
PS2X ps2x; // Static instantiation of the library

int psxErrorState = 0;
byte type = 0;

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define SMALL_RING 24
#define LARGE_RING 36

#define NUM_LEDS (SMALL_RING + LARGE_RING)

CRGB leds[NUM_LEDS];
uint8_t brightness = 50;

// Config moteurs
#define PCT_MOTORS          65
#define MD22_ADD_BOARD			0x58
#define LEFT_MOTOR					ASSIGN_MOTOR_2
#define RIGHT_MOTOR					ASSIGN_MOTOR_1
MD22 motors = MD22(MD22_ADD_BOARD, MODE_1, 100);

int left = 0;
int right = 0;
int speed = 0;
int turn = 0;

// Config Servos
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates
#define USSERVOMIN  500 // µ-second min
#define USSERVOMAX  2150 // µ-second max
#define NBSERVO 16 // 0 to 15
uint8_t servonum = 3;
uint16_t valueServo[NBSERVO];
uint8_t deltaRegServo = 100; // incrément de reglage servo
int valJoyDrtY = 0; // valeur de l'axe Y du joystick Droit (maper sur la plage de commande dervo)


// Alternate buildin LED
boolean alt = false;

void modifyBrightness(uint8_t stepValue);

void addGlitter(fract8 v);

void rainbow();
void rainbowWithGlitter();
void confetti();
void sinelon();
void juggle();
void bpm();
void animMoveTinker();
void guilleLed();
void BougeServo();

// List of patterns to cycle through.  Each is defined as a separate function below.
#define FRAMES_PER_SECOND 120
#define FASTLED_DELAY (1000 / FRAMES_PER_SECOND)
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = {guilleLed, animMoveTinker, rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm};

int8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0;                  // rotating "base color" used by many of the patterns
int8_t idxLed =0;

// Configuration //
// ------------- //
void setup() {
  #if defined(DEBUG)
    Serial.begin(115200);
    Serial.println("Setup");
  #endif

  #if defined(DEBUG)
    Serial.println(" - Configuration LEDs");
  #endif

  FastLED.addLeds<NEOPIXEL, 6>(leds, NUM_LEDS);
  FastLED.setBrightness(brightness);

  #if defined(DEBUG)
    Serial.println(" - Configuration I2C");
  #endif
  Wire.begin();
  byte nbDevices = i2cUtils.scan();
  #if defined(DEBUG)
    Serial.print(" * Nb I2C devices : ");
    Serial.println(nbDevices, DEC);
  #endif

  #if defined(DEBUG)
    Serial.println(" - Configuration Ecran OLED");
  #endif

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS, false, false)) {
    #if defined(DEBUG)
      Serial.println(F("SSD1306 allocation failed"));
    #endif
  
    while(1) {
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = alt ? CRGB::DarkRed : CRGB::Black;
      }
      alt = !alt;
      FastLED.show();
      FastLED.delay(1000);
    } 
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();

  // Show a small animation on LEDs
  for (int i = 0; i < NUM_LEDS + 20; i++) {
    fadeToBlackBy(leds, NUM_LEDS, 60);
    if (i < NUM_LEDS) {
      leds[i] = CRGB::Green;
    }
    FastLED.show();
    FastLED.delay(FASTLED_DELAY);
  }

  #if defined(DEBUG)
    Serial.println(" - Configuration des I/O");
  #endif
  pinMode(LED_BUILTIN, OUTPUT);

  #if defined(DEBUG)
    Serial.println(" - Configuration MD22");
  #endif
  motors.init();
  motors.assignMotors(LEFT_MOTOR, RIGHT_MOTOR);
  motors.stopAll();

  // Setup the PS2X library
  do {
    #if defined(DEBUG)
      Serial.println(" - Configuration PS2 Remote controller");
    #endif  

    psxErrorState = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble);

    #if defined(DEBUG)
      if(psxErrorState == 0) {
        Serial.print("Found Controller, configured successful ");
        Serial.print("[pressures = ");
        Serial.print(pressures ? "true ; " : "false ; ");
        Serial.print("rumble = ");
        Serial.println(rumble ? "true]" : "false]");
        
      } else if(psxErrorState == 1) {
        Serial.println("No controller found, check wiring, see readme.txt to enable debug. visit www.billporter.info for troubleshooting tips");
      
      } else if(psxErrorState == 2) {
        Serial.println("Controller found but not accepting commands. see readme.txt to enable debug. Visit www.billporter.info for troubleshooting tips");

      } else if(psxErrorState == 3) {
        Serial.println("Controller refusing to enter Pressures mode, may not support it. ");
      }
          
      type = ps2x.readType(); 
      switch(type) {
        case 0:
          Serial.println("Unknown Controller type found ");
          break;
        case 1:
          Serial.println("DualShock Controller found ");
          break;
        case 2:
          Serial.println("GuitarHero Controller found ");
          break;
      case 3:
          Serial.println("Wireless Sony DualShock Controller found ");
          break;
      }
    #endif
  } while (psxErrorState != 0);

  ps2x.read_gamepad(true, 100); 
  delay(1000);
  ps2x.read_gamepad(false, 0); 

  // Setup Servos
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates
  delay(10);
  for (int i=0 ; i<NBSERVO ; i++) {
    valueServo[i]=1500;
    pwm.writeMicroseconds(servonum, valueServo[i]);
  }
  
}

// Main loop //
// --------- //
void loop() {

  // do some periodic updates
  EVERY_N_MILLISECONDS(20) { 
     // slowly cycle the "base color" through the rainbow
    gHue++; 
  }

  EVERY_N_MILLISECONDS(50) {
    ps2x.read_gamepad(false, 0);
    // actions sur servos
    if(ps2x.ButtonPressed(PSB_SELECT)) {
      BougeServo();
    }
    if(ps2x.ButtonPressed(PSB_TRIANGLE)) {
      if(deltaRegServo==100){ 
        deltaRegServo=10;
      } else if(deltaRegServo==10) {
        deltaRegServo=1;
      } else {
        deltaRegServo=100;
      }
    }
    if(ps2x.ButtonPressed(PSB_R1)) {
      servonum++;
      if(servonum>=NBSERVO){
        servonum=0;
      }
    }
    if(ps2x.ButtonPressed(PSB_L1)) {
      servonum--;
      if(servonum>=NBSERVO){ //because Uint
        servonum=NBSERVO-1;
      }
    }
    if(ps2x.ButtonPressed(PSB_R2)) {
      valueServo[servonum]+=deltaRegServo;
      if(valueServo[servonum]>USSERVOMAX){
        valueServo[servonum]=USSERVOMAX;
      }
      pwm.writeMicroseconds(servonum, valueServo[servonum]);
    }
    if(ps2x.ButtonPressed(PSB_L2)) {
      valueServo[servonum]-=deltaRegServo;
      if(valueServo[servonum]<USSERVOMIN){
        valueServo[servonum]=USSERVOMIN;
      }
      pwm.writeMicroseconds(servonum, valueServo[servonum]);
    }
    valJoyDrtY = map(ps2x.Analog(PSS_RY), 127, 0, 1000, 2000);
    if (valJoyDrtY<USSERVOMIN){
      valJoyDrtY=USSERVOMIN;
    } else if (valJoyDrtY>USSERVOMAX){
      valJoyDrtY=USSERVOMAX;
    }    
    pwm.writeMicroseconds(servonum, valJoyDrtY);

    // actions sur LED
    if(ps2x.ButtonPressed(PSB_PAD_LEFT)) {
      if (gCurrentPatternNumber - 1 >= 0) {
        gCurrentPatternNumber--;
      }
    }
    if(ps2x.ButtonPressed(PSB_PAD_RIGHT)) {
      if (gCurrentPatternNumber + 1 < ARRAY_SIZE(gPatterns) - 1) {
        gCurrentPatternNumber++;
      }
    }

    if (ps2x.ButtonPressed(PSB_PAD_UP)) {
      modifyBrightness(5);
    }
    if (ps2x.ButtonPressed(PSB_PAD_DOWN)) {
      modifyBrightness(-5);
    }

    // actions sur moteurs
    speed = map(ps2x.Analog(PSS_LY), 255, 0, -PCT_MOTORS, PCT_MOTORS);
    if (speed < 2 && speed > -2) {
      speed = 0;
    }
    turn = map(ps2x.Analog(PSS_LX), 0, 255, PCT_MOTORS, -PCT_MOTORS);
    if (turn < 2 && turn > -2) {
      turn = 0;
    }

    left = ((speed - turn) * 127) / 100;
    right = ((speed + turn) * 127) / 100;

    motors.generateMouvement(left, right);

    // affichage
    display.clearDisplay();

    display.setTextSize(1);             // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);// Draw white text
    display.setCursor(0,0);             // Start at top-left corner
    display.print("Spd: ");
    display.print(speed);
    display.print(" %  ML: ");
    display.println(left);
    display.print("Trn: ");
    display.print(turn);
    display.print(" %  MR: ");
    display.println(right);
    display.println("");
    display.print("N.serv : ");
    display.println(servonum);
    display.print("Value  : ");
    display.println(valueServo[servonum]);
    display.print("D : ");
    display.println(deltaRegServo);
    display.print("Joystick : ");
    display.println(valJoyDrtY);

    display.display();
  }

  EVERY_N_MILLISECONDS(300){
    idxLed++;
    if(idxLed>NUM_LEDS-1){
      idxLed=0;
    }
  }
  gPatterns[gCurrentPatternNumber]();
  FastLED.show();
  FastLED.delay(FASTLED_DELAY);

}

// Prototypes implementations //
// -------------------------- //

void modifyBrightness(uint8_t stepValue) {
  brightness += stepValue;
  FastLED.setBrightness(brightness);
}

void rainbow() {
  // FastLED's built-in rainbow generator
  fill_rainbow(leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter() {
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter(fract8 chanceOfGlitter) {
  if (random8() < chanceOfGlitter) {
    leds[random16(NUM_LEDS)] += CRGB::White;
  }
}

void confetti() {
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy(leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV(gHue + random8(64), 200, 255);
}

void sinelon() {
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = beatsin16(13, 0, NUM_LEDS - 1);
  leds[pos] += CHSV(gHue, 255, 192);
}

void bpm() {
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
  for (int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy(leds, NUM_LEDS, 20);
  byte dothue = 0;
  for (int i = 0; i < 8; i++) {
    leds[beatsin16(i + 7, 0, NUM_LEDS - 1)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

void animMoveTinker() {
  fadeToBlackBy(leds, NUM_LEDS, 20);

  // Small ring speed value
  uint8_t nbLeds = map(abs(speed), 0, 100, 0, SMALL_RING);
  if (speed > 0) {
    for (int i = 0 ; i < nbLeds ; i++) {
      leds[i] = CHSV(gHue, 255, 255);
    }
  } else if (speed < 0) {
    for (int i = SMALL_RING - 1 ; i >= SMALL_RING - nbLeds ; i--) {
      leds[i] = CHSV(gHue, 255, 255);
    }
  }

  // Large ring turn motor value
  nbLeds = map(abs(turn), 0, 100, 0, LARGE_RING);
  if (turn > 0) {
    for (int i = 0 ; i < nbLeds ; i++) {
      leds[i + SMALL_RING] = CHSV(gHue, 255, 255);
    }
  } else if (turn < 0) {
    for (int i = SMALL_RING + LARGE_RING - 1 ; i >= SMALL_RING + LARGE_RING - nbLeds ; i--) {
      leds[i] = CHSV(gHue, 255, 255);
    }
  }
}

void guilleLed() {
  //FastLED.clear();
  fadeToBlackBy(leds, NUM_LEDS, 3);
  // Turn the LED on, then pause
  leds[idxLed] = CRGB::Red;

}

// Faire bouger un servo en essui-glace de butée à butée puis retour position init
void BougeServo() {
  for (uint16_t i = valueServo[servonum]; i < USSERVOMAX; i+=deltaRegServo) {
    pwm.writeMicroseconds(servonum, i);
  }
  pwm.writeMicroseconds(servonum, USSERVOMAX);
  delay(500);
  for (uint16_t i = USSERVOMAX; i > USSERVOMIN; i-=deltaRegServo) {
    pwm.writeMicroseconds(servonum, i);
  }
  pwm.writeMicroseconds(servonum, USSERVOMIN);
  delay(500);
  for (uint16_t i = USSERVOMIN; i < valueServo[servonum]; i+=deltaRegServo) {
    if (i>valueServo[servonum]){
      i=valueServo[servonum];
    }
    pwm.writeMicroseconds(servonum, i);
  }
  pwm.writeMicroseconds(servonum, valueServo[servonum]);
  delay(500);
}