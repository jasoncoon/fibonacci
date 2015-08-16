#include "FastLED/FastLED.h"
FASTLED_USING_NAMESPACE;

#include "application.h"

#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)

#define DATA_PIN    A5
#define CLK_PIN     A3
// #define COLOR_ORDER RGB
#define LED_TYPE    WS2801
#define NUM_LEDS    100

#define NUM_VIRTUAL_LEDS 101
#define LAST_VISIBLE_LED 99

CRGB leds[NUM_VIRTUAL_LEDS];

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
 
typedef uint8_t (*SimplePattern)();
typedef SimplePattern SimplePatternList[];
typedef struct { SimplePattern drawFrame;  char name[32]; } PatternAndName;
typedef PatternAndName PatternAndNameList[];
 
// List of patterns to cycle through.  Each is defined as a separate function below.

const PatternAndNameList patterns = { 
  { pride,                  "Pride" },
  { radialPaletteShift,     "Radial Palette Shift" },
  { verticalPaletteBlend,   "Vertical Palette Shift" },
  { horizontalPaletteBlend, "Horizontal Palette Shift" },
  { spiral1,                "Spiral 1" },
  { spiral2,                "Spiral 2" },
  { wave,                   "Wave" },
  { rainbow,                "Rainbow" },
  { rainbowWithGlitter,     "Rainbow With Glitter" },
  { confetti,               "Confetti" },
  { sinelon,                "Sinelon" },
  { bpm,                    "Beat" },
  { juggle,                 "Juggle" },
  { fire,                   "Fire" },
  { water,                  "Water" },
  { analogClock,            "Analog Clock" },
  { analogClockColor,       "Analog Clock Color" },
  { fastAnalogClock,        "Fast Analog Clock Test" },
  { showSolidColor,         "Solid Color" }
};

uint8_t brightness = 32;

int patternCount = ARRAY_SIZE(patterns);
int patternIndex = 0;
char patternName[32] = "Pride";
int power = 1;
int flipClock = 0;

int timezone = -6;
unsigned long lastTimeSync = millis();

uint8_t gHue = 0; // rotating "base color" used by many of the patterns

uint8_t paletteIndex = 0;

// List of palettes to cycle through.

CRGBPalette16 palettes[] = {
  RainbowColors_p,
  RainbowStripeColors_p,
  CloudColors_p,
  OceanColors_p,
  ForestColors_p,
  HeatColors_p,
  LavaColors_p,
  PartyColors_p,
};

uint8_t paletteCount = ARRAY_SIZE(palettes);

CRGBPalette16 currentPalette(CRGB::Black);
CRGBPalette16 targetPalette = palettes[paletteIndex];

CRGB solidColor = CRGB::Blue;
int r = 0;
int g = 0;
int b = 255;

SYSTEM_MODE(SEMI_AUTOMATIC);

int offlinePin = D7;

void setup() {
    FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN>(leds, NUM_LEDS);
    FastLED.setBrightness(brightness);
    fill_solid(leds, NUM_LEDS, CRGB::Blue);
    FastLED.show();
    
    pinMode(offlinePin, INPUT_PULLUP);
    
    if(digitalRead(offlinePin) == HIGH) {
        Spark.connect();
    }
    
    // Serial.begin(9600);
    
    // load settings from EEPROM
    brightness = EEPROM.read(0);
    if(brightness < 1)
        brightness = 1;
    else if(brightness > 255)
        brightness = 255;
    
    FastLED.setBrightness(brightness);
    FastLED.setDither(brightness < 255);
    
    uint8_t timezoneSign = EEPROM.read(1);
    if(timezoneSign < 1)
        timezone = -EEPROM.read(2);
    else
        timezone = EEPROM.read(2);
    
    if(timezone < -12)
        power = -12;
    else if (power > 13)
        power = 13;
    
    patternIndex = EEPROM.read(3);
    if(patternIndex < 0)
        patternIndex = 0;
    else if (patternIndex >= patternCount)
        patternIndex = patternCount - 1;
    
    flipClock = EEPROM.read(4);
    if(flipClock < 0)
        flipClock = 0;
    else if (flipClock > 1)
        flipClock = 1;
        
    r = EEPROM.read(5);
    g = EEPROM.read(6);
    b = EEPROM.read(7);
    
    if(r == 0 && g == 0 && b == 0) {
        r = 0;
        g = 0;
        b = 255;
    }
    
    solidColor = CRGB(r, b, g);
    
    Spark.function("variable", setVariable);
    Spark.function("patternIndex", setPatternIndex);
    Spark.function("patternName", setPatternName);
    
    Spark.variable("power", &power, INT);
    Spark.variable("brightness", &brightness, INT);
    Spark.variable("timezone", &timezone, INT);
    Spark.variable("flipClock", &flipClock, INT);
    Spark.variable("patternCount", &patternCount, INT);
    Spark.variable("patternIndex", &patternIndex, INT);
    Spark.variable("patternName", patternName, STRING);
    Spark.variable("r", &r, INT);
    Spark.variable("g", &g, INT);
    Spark.variable("b", &b, INT);
    
    Time.zone(timezone);
}

void loop() {
    if (Spark.connected() && millis() - lastTimeSync > ONE_DAY_MILLIS) {
    // Request time synchronization from the Spark Cloud
    Spark.syncTime();
    lastTimeSync = millis();
    }
    
    if(power < 1) {
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
      FastLED.delay(250);
      return;
    }
    
    uint8_t delay = patterns[patternIndex].drawFrame();
    
    // send the 'leds' array out to the actual LED strip
    FastLED.show();
    // insert a delay to keep the framerate modest
    FastLED.delay(delay); 
    
    // blend the current palette to the next
    uint8_t maxChanges = 24;
    nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);
    
    EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
    
    // slowly change to a new palette
    EVERY_N_SECONDS(10) {
        paletteIndex++;
        if (paletteIndex >= paletteCount) paletteIndex = 0;
        targetPalette = palettes[paletteIndex];
    };
}

int setVariable(String args) {
    if(args.startsWith("pwr:")) {
        return setPower(args.substring(4));
    }
    else if (args.startsWith("brt:")) {
        return setBrightness(args.substring(4));
    }
    else if (args.startsWith("tz:")) {
        return setTimezone(args.substring(3));
    }
    else if (args.startsWith("flpclk:")) {
        return setFlipClock(args.substring(7));
    }
    else if (args.startsWith("r:")) {
        r = parseByte(args.substring(2));
        solidColor.r = r;
        EEPROM.write(5, r);
        patternIndex = patternCount - 1;
        return r;
    }
    else if (args.startsWith("g:")) {
        g = parseByte(args.substring(2));
        solidColor.g = g;
        EEPROM.write(6, g);
        patternIndex = patternCount - 1;
        return g;
    }
    else if (args.startsWith("b:")) {
        b = parseByte(args.substring(2));
        solidColor.b = b;
        EEPROM.write(7, b);
        patternIndex = patternCount - 1;
        return b;
    }
    
    return -1;
}

int setPower(String args) {
    power = args.toInt();
    if(power < 0)
        power = 0;
    else if (power > 1)
        power = 1;
    
    return power;
}

int setBrightness(String args)
{
    brightness = args.toInt();
    if(brightness < 1)
        brightness = 1;
    else if(brightness > 255)
        brightness = 255;
        
    FastLED.setBrightness(brightness);
    FastLED.setDither(brightness < 255);
    
    EEPROM.write(0, brightness);
    
    return brightness;
}

int setTimezone(String args) {
    timezone = args.toInt();
    if(timezone < -12)
        power = -12;
    else if (power > 13)
        power = 13;
    
    Time.zone(timezone);
    
    if(timezone < 0)
        EEPROM.write(1, 0);
    else
        EEPROM.write(1, 1);
    
    EEPROM.write(2, abs(timezone));
    
    return power;
}

int setFlipClock(String args) {
    flipClock = args.toInt();
    if(flipClock < 0)
        flipClock = 0;
    else if (flipClock > 1)
        flipClock = 1;
    
    EEPROM.write(4, flipClock);
    
    return flipClock;
}

byte parseByte(String args) {
    int c = args.toInt();
    if(c < 0)
        c = 0;
    else if (c > 255)
        c = 255;
    
    return c;
}

int setPatternIndex(String args)
{
    patternIndex = args.toInt();
    if(patternIndex < 0)
        patternIndex = 0;
    else if (patternIndex >= patternCount)
        patternIndex = patternCount - 1;
        
    EEPROM.write(3, patternIndex);
    
    return patternIndex;
}

int setPatternName(String args)
{
    int index = args.toInt();
    if(index < 0)
        index = 0;
    else if (index >= patternCount)
        index = patternCount - 1;
    
    strcpy(patternName, patterns[index].name);
    
    return index;
}

uint8_t rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 255 / NUM_LEDS);
  return 8;
}

uint8_t rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
  return 8;
}

uint8_t confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
  return 8;
}

uint8_t sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16(13,0,NUM_LEDS);
  leds[pos] += CHSV( gHue, 255, 192);
  return 8;
}

uint8_t bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(currentPalette, gHue+(i*2), beat-gHue+(i*10));
  }
  
  return 8;
}

uint8_t juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16(i+7,0,NUM_LEDS)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
  
  return 8;
}

uint8_t fire() {
    heatMap(HeatColors_p, true);
    
    return 15;
}

CRGBPalette16 icePalette = CRGBPalette16(CRGB::Black, CRGB::Blue, CRGB::Aqua, CRGB::White);

uint8_t water() {
    heatMap(icePalette, false);
    
    return 15;
}

uint8_t analogClock() {
    dimAll(220);
    
    drawAnalogClock(Time.second(), Time.minute(), Time.hourFormat12(), true, true);
    
    return 8;
}

uint8_t analogClockColor() {
    fill_solid(leds, NUM_LEDS, solidColor);
    
    drawAnalogClock(Time.second(), Time.minute(), Time.hourFormat12(), false, true);
    
    return 8;
}

byte fastSecond = 0;
byte fastMinute = 0;
byte fastHour = 1;

uint8_t fastAnalogClock() {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    
    drawAnalogClock(fastSecond, fastMinute, fastHour, false, false);
        
    fastMinute++;
    
    // fastSecond++;
    
    // if(fastSecond >= 60) {
    //     fastSecond = 0;
    //     fastMinute++;
    // }
     
    if(fastMinute >= 60) {
        fastMinute = 0;
        fastHour++;
    }
    
    if(fastHour >= 13) {
        fastHour = 1;
    }
        
    return 125;
}

uint8_t showSolidColor() {
    fill_solid(leds, NUM_LEDS, solidColor);
    
    return 30;
}

// This function draws rainbows with an ever-changing,
// widely-varying set of parameters.
uint8_t pride() 
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;
 
  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);
  
  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;
  
  for( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);
    
    CRGB newcolor = CHSV( hue8, sat8, bri8);
    
    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS-1) - pixelnumber;
    
    nblend( leds[pixelnumber], newcolor, 64);
  }
  
  return 15;
}

uint8_t radialPaletteShift()
{
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    // leds[i] = ColorFromPalette( currentPalette, gHue + sin8(i*16), brightness);
    leds[i] = ColorFromPalette(currentPalette, i + gHue, 255, BLEND);
  }

  return 8;
}

const uint8_t spiral1Count = 13;
const uint8_t spiral1Length = 7;

uint8_t spiral1Arms[spiral1Count][spiral1Length] = {
    { 0, 14, 27, 40, 53, 66, 84 },
    { 1, 15, 28, 41, 54, 67, 76 },
    { 2, 16, 29, 42, 55, 68, 85 },
    { 3, 17, 30, 43, 56, 77, 86 },
    { 4, 18, 31, 44, 57, 69, 78 },
    { 5, 19, 32, 45, 58, 70, 87 },
    { 6, 20, 33, 46, 59, 79, 94 },
    { 7, 21, 34, 47, 60, 71, 88 },
    { 8, 22, 35, 48, 61, 80, 89 },
    { 9, 23, 36, 49, 62, 72, 81 },
    { 10, 24, 37, 50, 63, 73, 82 },
    { 11, 25, 38, 51, 64, 74, 83 },
    { 12, 13, 26, 39, 52, 65, 75 }
};

const uint8_t spiral2Count = 21;
const uint8_t spiral2Length = 4;

uint8_t spiral2Arms[spiral2Count][spiral2Length] = {
    { 0, 26, 51, 73 },
    { 1, 14, 39, 64 },
    { 15, 27, 52, 74 },
    { 2, 28, 40, 65 },
    { 16, 41, 53, 75 },
    { 3, 29, 54, 66 },
    { 4, 17, 42, 67 },
    { 18, 30, 55, 76 },
    { 5, 31, 43, 68 },
    { 19, 44, 56, 85 },
    { 6, 32, 57, 77 },
    { 7, 20, 45, 69 },
    { 21, 33, 58, 78 },
    { 8, 34, 46, 70 },
    { 9, 22, 47, 59 },
    { 23, 35, 60, 79 },
    { 10, 36, 48, 71 },
    { 24, 49, 61, 88 },
    { 11, 37, 62, 80 },
    { 12, 25, 50, 72 },
    { 13, 38, 63, 81 }
};

template <size_t armCount, size_t armLength>
void fillSpiral(uint8_t (&spiral)[armCount][armLength]) {
    fill_solid(leds, NUM_LEDS, ColorFromPalette(currentPalette, 0, 255, BLEND));
    
    uint8_t offset = 0;
    
    for(uint8_t i = 0; i < armCount; i++) {
        CRGB color = ColorFromPalette(currentPalette, gHue + offset, 255, BLEND);
        
        for(uint8_t j = 0; j < armLength; j++) {
            leds[spiral[i][j]] = color;
        }
        
        offset += 255 / armCount;
    }
}

uint8_t spiral1() {
    fillSpiral(spiral1Arms);
    
    return 0;
}

uint8_t spiral2() {
    fillSpiral(spiral2Arms);
    
    return 0;
}

// Params for width and height
const uint8_t kMatrixWidth = 10;
const uint8_t kMatrixHeight = 10;

uint8_t xyMap[kMatrixHeight][kMatrixWidth][2] = {
    { { 100, 100 }, { 100, 100 }, { 100, 100 }, { 21, 100 }, { 7, 20 }, { 6, 100 }, { 32, 100 }, { 19, 100 }, { 100, 100 }, { 100, 100 } },
    { { 100, 100 }, { 8, 100 }, { 34, 100 }, { 33, 46 }, { 58, 100 }, { 45, 100 }, { 57, 100 }, { 44, 100 }, { 5, 31 }, { 122, 100 } },
    { { 9, 100 }, { 22, 100 }, { 47, 100 }, { 59, 100 }, { 70, 100 }, { 69, 78 }, { 77, 100 }, { 56, 100 }, { 43, 100 }, { 18, 100 } },
    { { 133, 100 }, { 35, 100 }, { 60, 100 }, { 79, 100 }, { 87, 93 }, { 86, 100 }, { 85, 100 }, { 68, 100 }, { 141, 100 }, { 30, 4 } },
    { { 23, 100 }, { 48, 100 }, { 71, 100 }, { 94, 100 }, { 99, 100 }, { 98, 100 }, { 92, 100 }, { 76, 100 }, { 55, 100 }, { 17, 42 } },
    { { 10, 36 }, { 61, 100 }, { 80, 100 }, { 88, 100 }, { 95, 100 }, { 96, 97 }, { 91, 100 }, { 84, 100 }, { 67, 100 }, { 29, 3 } },
    { { 24, 100 }, { 49, 62 }, { 162, 100 }, { 89, 100 }, { 90, 100 }, { 166, 100 }, { 75, 83 }, { 66, 100 }, { 54, 100 }, { 16, 100 } },
    { { 173, 100 }, { 37, 100 }, { 50, 72 }, { 81, 100 }, { 73, 100 }, { 74, 82 }, { 65, 100 }, { 179, 100 }, { 41, 53 }, { 181, 100 } },
    { { 183, 100 }, { 11, 100 }, { 25, 100 }, { 63, 100 }, { 51, 100 }, { 64, 100 }, { 52, 100 }, { 40, 27 }, { 28, 100 }, { 2, 100 } },
    { { 193, 100 }, { 195, 100 }, { 12, 100 }, { 13, 38 }, { 26, 100 }, { 0, 39 }, { 14, 100 }, { 1, 27 }, { 15, 100 }, { 201, 100 } }
};

void setPixelXY(uint8_t x, uint8_t y, CRGB color)
{
    if((x >= kMatrixWidth) || (y >= kMatrixHeight)) {
        return;
    }
    
    for(uint8_t z = 0; z < 2; z++) {
        uint8_t i = xyMap[y][x][z];
        
        leds[i] = color;
    }
}

uint8_t horizontalPaletteBlend()
{
    uint8_t offset = 0;
    
    for(uint8_t x = 0; x <= kMatrixWidth; x++)
    {
        CRGB color = ColorFromPalette(currentPalette, gHue + offset, 255, BLEND);
        
        for(uint8_t y = 0; y <= kMatrixHeight; y++)
        {
            setPixelXY(x, y, color);
        }
        
        offset++;
    }
    
    return 15;
}

uint8_t verticalPaletteBlend()
{
    uint8_t offset = 0;
    
    for(uint8_t y = 0; y <= kMatrixHeight; y++)
    {
        CRGB color = ColorFromPalette(currentPalette, gHue + offset, 255, BLEND);
        
        for(uint8_t x = 0; x <= kMatrixWidth; x++)
        {
            setPixelXY(x, y, color);
        }
        
        offset++;
    }
    
    return 15;
}

const uint8_t maxX = kMatrixWidth - 1;
const uint8_t maxY = kMatrixHeight - 1;
const uint8_t scale = 256 / kMatrixWidth;

uint8_t wave()
{
    static uint8_t rotation = 0;
    static uint8_t theta = 0;
    static uint8_t waveCount = 1;
    
    uint8_t n = 0;

    switch (rotation) {
        case 0:
            for (int x = 0; x < kMatrixWidth; x++) {
                n = quadwave8(x * 2 + theta) / scale;
                setPixelXY(x, n, ColorFromPalette(currentPalette, x + gHue, 255, BLEND));
                if (waveCount == 2)
                    setPixelXY(x, maxY - n, ColorFromPalette(currentPalette, x + gHue, 255, BLEND));
            }
            break;

        case 1:
            for (int y = 0; y < kMatrixHeight; y++) {
                n = quadwave8(y * 2 + theta) / scale;
                setPixelXY(n, y, ColorFromPalette(currentPalette, y + gHue, 255, BLEND));
                if (waveCount == 2)
                    setPixelXY(maxX - n, y, ColorFromPalette(currentPalette, y + gHue, 255, BLEND));
            }
            break;

        case 2:
            for (int x = 0; x < kMatrixWidth; x++) {
                n = quadwave8(x * 2 - theta) / scale;
                setPixelXY(x, n, ColorFromPalette(currentPalette, x + gHue));
                if (waveCount == 2)
                    setPixelXY(x, maxY - n, ColorFromPalette(currentPalette, x + gHue, 255, BLEND));
            }
            break;

        case 3:
            for (int y = 0; y < kMatrixHeight; y++) {
                n = quadwave8(y * 2 - theta) / scale;
                setPixelXY(n, y, ColorFromPalette(currentPalette, y + gHue, 255, BLEND));
                if (waveCount == 2)
                    setPixelXY(maxX - n, y, ColorFromPalette(currentPalette, y + gHue, 255, BLEND));
            }
            break;
    }
    
    dimAll(254);

    EVERY_N_SECONDS(10) 
    {  
        rotation = random(0, 4);
        // waveCount = random(1, 3);
    };
    
    theta++;
    
    return 0;
}

void heatMap(CRGBPalette16 palette, bool up) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    
    // Add entropy to random number generator; we use a lot of it.
    random16_add_entropy(random(256));
    
    uint8_t cooling = 55;
    uint8_t sparking = 120;
    
    // Array of temperature readings at each simulation cell
    static const uint8_t halfLedCount = NUM_LEDS / 2;
    static byte heat[2][halfLedCount];
    
    byte colorindex;
    
    for(uint8_t x = 0; x < 2; x++) {
        // Step 1.  Cool down every cell a little
        for( int i = 0; i < halfLedCount; i++) {
          heat[x][i] = qsub8( heat[x][i],  random8(0, ((cooling * 10) / halfLedCount) + 2));
        }
        
        // Step 2.  Heat from each cell drifts 'up' and diffuses a little
        for( int k= halfLedCount - 1; k >= 2; k--) {
          heat[x][k] = (heat[x][k - 1] + heat[x][k - 2] + heat[x][k - 2] ) / 3;
        }
        
        // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
        if( random8() < sparking ) {
          int y = random8(7);
          heat[x][y] = qadd8( heat[x][y], random8(160,255) );
        }
        
        // Step 4.  Map from heat cells to LED colors
        for( int j = 0; j < halfLedCount; j++) {
            // Scale the heat value from 0-255 down to 0-240
            // for best results with color palettes.
            colorindex = scale8(heat[x][j], 240);
            
            CRGB color = ColorFromPalette(palette, colorindex);
            
            if(up) {
                if(x == 0) {
                    leds[(halfLedCount - 1) - j] = color;
                }
                else {
                    leds[halfLedCount + j] = color;
                }
            }
            else {
                if(x == 0) {
                    leds[j] = color;
                }
                else {
                    leds[(NUM_LEDS - 1) - j] = color;
                }
            }
        }
    }
}

int oldSecTime = 0;
int oldSec = 0;

void drawAnalogClock(byte second, byte minute, byte hour, boolean drawMillis, boolean drawSecond) {
    if(Time.second() != oldSec){
        oldSecTime = millis();
        oldSec = Time.second();
    }
    
    int millisecond = millis() - oldSecTime;
    
    int secondIndex = map(second, 0, 59, 0, NUM_LEDS);
    int minuteIndex = map(minute, 0, 59, 0, NUM_LEDS);
    int hourIndex = map(hour * 5, 5, 60, 0, NUM_LEDS);
    int millisecondIndex = map(secondIndex + millisecond * .06, 0, 60, 0, NUM_LEDS);
    
    if(millisecondIndex >= NUM_LEDS)
        millisecondIndex -= NUM_LEDS;
    
    hourIndex += minuteIndex / 12;
    
    if(hourIndex >= NUM_LEDS)
        hourIndex -= NUM_LEDS;
    
    // see if we need to reverse the order of the LEDS
    if(flipClock == 1) {
        int max = NUM_LEDS - 1;
        secondIndex = max - secondIndex;
        minuteIndex = max - minuteIndex;
        hourIndex = max - hourIndex;
        millisecondIndex = max - millisecondIndex;
    }
    
    if(secondIndex >= NUM_LEDS)
        secondIndex = NUM_LEDS - 1;
    else if(secondIndex < 0)
        secondIndex = 0;
    
    if(minuteIndex >= NUM_LEDS)
        minuteIndex = NUM_LEDS - 1;
    else if(minuteIndex < 0)
        minuteIndex = 0;
        
    if(hourIndex >= NUM_LEDS)
        hourIndex = NUM_LEDS - 1;
    else if(hourIndex < 0)
        hourIndex = 0;
        
    if(millisecondIndex >= NUM_LEDS)
        millisecondIndex = NUM_LEDS - 1;
    else if(millisecondIndex < 0)
        millisecondIndex = 0;
    
    if(drawMillis)
        leds[millisecondIndex] += CRGB(0, 0, 127); // Blue
        
    if(drawSecond)
        leds[secondIndex] += CRGB(0, 0, 127); // Blue
        
    leds[minuteIndex] += CRGB::Green;
    leds[hourIndex] += CRGB::Red;
}

// scale the brightness of all pixels down
void dimAll(byte value)
{
  for (int i = 0; i < NUM_LEDS; i++){
    leds[i].nscale8(value);
  }
}

void addGlitter( uint8_t chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}
