#include <config.h>
#include <Arduino.h>
#include <Wire.h>
#include <I2CUtils.h>
#include <MD22.h>
#include <PS2X_lib.h>

#if defined(SCREEN)
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>
#endif
#if defined(LEDS)
  #include <FastLED.h>
#endif

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
byte vibrate = 0;

#if defined(SCREEN)
  // Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
  #define SCREEN_WIDTH 128 // OLED display width, in pixels
  #define SCREEN_HEIGHT 64 // OLED display height, in pixels
  #define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
  #define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif

#if defined(LEDS)
  #define SMALL_RING 24
  #define LARGE_RING 36

  #define NUM_LEDS (SMALL_RING + LARGE_RING)

  CRGB leds[NUM_LEDS];
#endif

// Config moteurs
#define MD22_ADD_BOARD				0x58
#define LEFT_MOTOR					ASSIGN_MOTOR_1
#define RIGHT_MOTOR					ASSIGN_MOTOR_2
MD22 motors = MD22(MD22_ADD_BOARD, MODE_1, 0);

int left = 0;
int right = 0;

// Alternate buildin LED
boolean alt = false;
CRGB color = CRGB::Black;

// Prototypes for functions defined at the end of this file //
// -------------------------------------------------------- //

// Configuration //
// ------------- //
void setup() {
  #if defined(DEBUG)
    Serial.begin(115200);
    Serial.println("Setup");
  #endif

  #if defined(DEBUG)
    Serial.println(" - Configuration I2C");
  #endif
  Wire.begin();
  byte nbDevices = i2cUtils.scan();

  #if defined(DEBUG)
    Serial.print(" * Nb I2C devices : ");
    Serial.println(nbDevices, DEC);
  #endif

  #if defined(LEDS)
    #if defined(DEBUG)
      Serial.println(" - Configuration LEDs");
    #endif

    FastLED.addLeds<NEOPIXEL, 6>(leds, NUM_LEDS);
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB::Green;
      FastLED.show();
      FastLED.delay(10);
    }
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB::Black;
      FastLED.show();
      FastLED.delay(10);
    }
  #endif 

  #if defined(DEBUG)
    Serial.println(" - Configuration des I/O");
  #endif
  pinMode(LED_BUILTIN, OUTPUT);

  #if defined(SCREEN)
    #if defined(DEBUG)
      Serial.println(" - Configuration Ecran OLED");
    #endif

    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS, false, false)) {
      #if defined(DEBUG)
        Serial.println(F("SSD1306 allocation failed"));
      #endif
    
      #if defined(LEDS)
        while(1) {
          for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = alt ? CRGB::DarkRed : CRGB::Black;
          }
          alt = !alt;
          FastLED.show();
          FastLED.delay(1000);
        } 
      #else
        digitalWrite(LED_BUILTIN, HIGH);
        while(true);
      #endif
    }

    // Show initial display buffer contents on the screen --
    // the library initializes this with an Adafruit splash screen.
    display.display();
    delay(1000); // Pause for 2 seconds

    // Clear the buffer
    display.clearDisplay();
  #endif

  #if defined(DEBUG)
    Serial.println(" - Configuration MD22");
  #endif
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
}

// Main loop //
// --------- //
void loop() {

  EVERY_N_SECONDS(1) {
    #if defined(LEDS)
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = alt ? color : CRGB::Black;
      }
      alt = !alt;
      FastLED.show();
    #endif
  }

  EVERY_N_MILLISECONDS(50) {
    ps2x.read_gamepad(false, vibrate);

    if(ps2x.ButtonPressed(PSB_PAD_UP)) {
      vibrate += 10;
    }
    if(ps2x.ButtonPressed(PSB_PAD_DOWN)) {
      vibrate -= 10;
    }

    if(ps2x.ButtonPressed(PSB_START)) {
      color = CRGB::Black;
    }
    if(ps2x.ButtonPressed(PSB_CROSS)) {
      color = CRGB::Blue;
    }
    if(ps2x.ButtonPressed(PSB_SQUARE)) {
      color = CRGB::Red;
    }
    if(ps2x.ButtonPressed(PSB_TRIANGLE)) {
      color = CRGB::Green;
    }
    if(ps2x.ButtonPressed(PSB_CIRCLE)) {
      color = CRGB::Purple;
    }

    int ly = 127 - ps2x.Analog(PSS_LY);
    int rx = 128 - ps2x.Analog(PSS_RX);

    left = map(ly - rx, -256, 255, -128, 127);
    right = map(ly + rx, -256, 255, -128, 127);

    #if defined(SCREEN)
      display.clearDisplay();

      display.setTextSize(1);             // Normal 1:1 pixel scale
      display.setTextColor(SSD1306_WHITE);// Draw white text
      display.setCursor(0,0);             // Start at top-left corner
      display.print("LY   : ");
      display.println(ly);
      display.print("RX   : ");
      display.println(rx);
      display.print("Vib. : ");
      display.println(vibrate);
      display.print("M L  : ");
      display.println(left);
      display.print("M R  : ");
      display.println(right);

      display.display();
    #endif

    motors.generateMouvement(left, right);
  }
}
