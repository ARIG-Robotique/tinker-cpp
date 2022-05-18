//#define DEBUG
//#define SCREEN

#include <Arduino.h>
#include <FastLED.h>
#include <Wire.h>

#if defined(SCREEN)
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>
#endif

#if defined(SCREEN)
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif

#define NUM_LEDS_FOUILLE 1
#define NUM_LEDS_VENTOUSE 2
#define NUM_LEDS_STOCK 6

#define NB_BANDEAU_LED 1 // nombre de groupes de led 1, 2 ou 3

#if NB_BANDEAU_LED == 1
#define NUM_LEDS_BANDEAU_1 (NUM_LEDS_FOUILLE + NUM_LEDS_VENTOUSE + NUM_LEDS_STOCK)
#elif NB_BANDEAU_LED == 2
#define NUM_LEDS_BANDEAU_1 (NUM_LEDS_FOUILLE)
#define NUM_LEDS_BANDEAU_2 (NUM_LEDS_STOCK + NUM_LEDS_VENTOUSE)
#elif NB_BANDEAU_LED == 3
#define NUM_LEDS_BANDEAU_1 (NUM_LEDS_FOUILLE)
#define NUM_LEDS_BANDEAU_2 (NUM_LEDS_VENTOUSE)
#define NUM_LEDS_BANDEAU_3 (NUM_LEDS_STOCK)
#endif

CRGB ledsBandeau1[NUM_LEDS_BANDEAU_1];
#if NB_BANDEAU_LED > 1
CRGB ledsBandeau2[NUM_LEDS_BANDEAU_2];
#endif
#if NB_BANDEAU_LED > 2
CRGB ledsBandeau3[NUM_LEDS_BANDEAU_3];
#endif

enum CarreFouille : byte {
  INCONNU,
  JAUNE,
  VIOLET,
  INTERDIT
};

// Private variables //
// ----------------- //
int i2cAddress = 0x3C;

volatile CarreFouille carreFouille = INCONNU;

// Prototypes for functions defined at the end of this file //
// -------------------------------------------------------- //
#if defined(SCREEN) 
void printStatesScreen(int value, String name);
#endif
void i2cOnReceive(int length);
void i2cOnRequest();
void processReceive(int length, boolean wire);

void readCarreFouille();
boolean between(int value, int medium);

void couleurCarreFouille(CRGB couleur);
void couleurStock(uint8_t index, CRGB couleur);
void couleurVentouse(uint8_t index, CRGB couleur);

// Configuration //
// ------------- //
void setup()
{
#if defined(DEBUG)
  Serial.begin(115200);
  Serial.println("Setup");

  Serial.println(" - Configuration des I/O");
#endif

  analogReference(EXTERNAL);
  pinMode(A0, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

#if defined(SCREEN)
  #if defined(DEBUG)
    Serial.println(" - Configuration Ecran OLED");
  #endif
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
  #if defined(DEBUG)
    Serial.println(F("SSD1306 allocation failed"));
  #endif
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
#endif

  
  Wire.begin(i2cAddress);
  Wire.onReceive(i2cOnReceive);
  Wire.onRequest(i2cOnRequest);
#if defined(DEBUG)
  Serial.print(" - I2C [OK] (Addresse : ");
  Serial.print(i2cAddress, HEX);
  Serial.println(")");

  Serial.print(" - Configuration bandeau(x) LEDs : ");
  Serial.println(NB_BANDEAU_LED);
#endif

  FastLED.addLeds<NEOPIXEL, 6>(ledsBandeau1, NUM_LEDS_BANDEAU_1);
#if NB_BANDEAU_LED > 1
  FastLED.addLeds<NEOPIXEL, 5>(ledsBandeau2, NUM_LEDS_BANDEAU_2);
#endif
#if NB_BANDEAU_LED > 2
  FastLED.addLeds<NEOPIXEL, 3>(ledsBandeau3, NUM_LEDS_BANDEAU_3);
#endif
}

// Main loop //
// --------- //
void loop() {
  EVERY_N_MILLIS(2) {
    readCarreFouille();
  }

  EVERY_N_SECONDS(1) {
    digitalWrite(LED_BUILTIN, digitalRead(LED_BUILTIN) == LOW ? HIGH : LOW);
  }

#if defined(DEBUG)
  if (Serial.available()) {
    processReceive(7, false);
  }
#endif

  FastLED.show();
}

void i2cOnRequest() {
  Wire.write(carreFouille);
}

void i2cOnReceive(int length) {
  processReceive(length, true);
}

void processReceive(int length, boolean wire) {
  char c = wire ? Wire.read() : Serial.read();
  switch (c) {
    // demande de valeur du carré de fouille (only Serial)
    case 'F':
      Serial.print("F: 0x0");
      Serial.println(carreFouille, HEX);
      break;

    // changement de couleur du stock
    case 'S':
      if (length < 7) {
#if defined(DEBUG)
        Serial.print("Pas assez de bits reçus pour le stock");
#endif
        break;
      }
      for (uint8_t i = 0; i < 6; i++) {
        c = wire ? Wire.read() : Serial.read();
        switch (c) {
          case 'R': couleurStock(i, CRGB::Red); break;
          case 'G': couleurStock(i, CRGB::Green); break;
          case 'B': couleurStock(i, CRGB::Blue); break;
          case '?': couleurStock(i, CRGB::Yellow); break;
          default: couleurStock(i, CRGB::Black); break;
        }
      }
      if (!wire) {
        Serial.println("S: OK");
      }
      break;

    // Changement de couleur des ventouses
    case 'V':
      if (length < 3) {
#if defined(DEBUG)
        Serial.print("Pas assez de bits reçus pour les ventouses");
#endif
        break;
      }
      for (uint8_t i = 0; i < 2; i++) {
        c = wire ? Wire.read() : Serial.read();
        switch (c) {
          case 'R': couleurVentouse(i, CRGB::Red); break;
          case 'G': couleurVentouse(i, CRGB::Green); break;
          case 'B': couleurVentouse(i, CRGB::Blue); break;
          case '?': couleurVentouse(i, CRGB::Yellow); break;
          default: couleurVentouse(i, CRGB::Black); break;
        }
      }
      if (!wire) {
        Serial.println("V: OK");
      }
      break;
  }
}

#if defined(SCREEN)
void printCarreFouilleScreen(int value, String name) {
  display.clearDisplay();

  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(name);
  display.println(value, DEC);

  display.display();
}
#endif

void readCarreFouille() {
  static int lastAnalogValue;
  static int cpt;

  int analogValue = analogRead(A0);
  int diff = abs(analogValue - lastAnalogValue);

  if (diff < 5) {
    cpt++;
  } else {
    cpt = 0;
  }
  lastAnalogValue = analogValue;

  if (cpt > 3) {
    cpt = 3;
    if (between(analogValue, 327)) {
#if defined(SCREEN)      
      printCarreFouilleScreen(analogValue, "VIOLET");
#endif
      carreFouille = VIOLET;
      couleurCarreFouille(CRGB::Purple);
    
    } else if (between(analogValue, 511)) {
#if defined(SCREEN)       
      printCarreFouilleScreen(analogValue, "JAUNE");
#endif     
      carreFouille = JAUNE;
      couleurCarreFouille(CRGB::Yellow);
    
    } else if (between(analogValue, 843)) {
#if defined(SCREEN)     
      printCarreFouilleScreen(analogValue, "INTERDIT");
#endif
      carreFouille = INTERDIT;
      couleurCarreFouille(CRGB::Red);
    
    } else {
#if defined(SCREEN)       
      printCarreFouilleScreen(analogValue, "INCONNU");
#endif
      carreFouille = INCONNU;
      couleurCarreFouille(CRGB::Black);
    }
  
  } else {
#if defined(SCREEN) 
    printCarreFouilleScreen(analogValue, "INCONNU");
#endif
    carreFouille = INCONNU;
    couleurCarreFouille(CRGB::Black);
  }
}

void couleurCarreFouille(CRGB couleur) {
  ledsBandeau1[0] = couleur;
}

void couleurVentouse(uint8_t index, CRGB couleur) {
#if NB_BANDEAU_LED == 1
  ledsBandeau1[index + NUM_LEDS_FOUILLE + NUM_LEDS_STOCK] = couleur;
#elif NB_BANDEAU_LED == 2
  ledsBandeau2[index + NUM_LEDS_STOCK] = couleur;
#else
  ledsBandeau3[index] = couleur;
#endif
}

void couleurStock(uint8_t index, CRGB couleur) {
#if NB_BANDEAU_LED == 1
  ledsBandeau1[index + NUM_LEDS_FOUILLE] = couleur;
#else
  ledsBandeau2[index] = couleur;
#endif
}

boolean between(int value, int medium) {
  return (value >= medium - 10 && value <= medium + 10);
}