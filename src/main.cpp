#include <Arduino.h>
#include <FastLED.h>
#include <arduinoFFT.h>

// generel setup
#define FPS         30

// FastLED setup
#define NUM_LEDS    15
#define CLR_ORD     GRB
#define BRIGHTNESS  16        // low brightness to not fry USB-port while testing around
#define LED_CHIP    WS2812B
#define LED_PIN     13        // Digital Pin

CRGB g_LEDs[NUM_LEDS] = {0};  // LED Array Buffer
unsigned long g_fpsDelay;     // Delay based on FPS
unsigned long g_fpsTime = 0;  // keeping track of time for LED fps

// Sound Setup
#define SOUND_PIN A0          // Analog Pin
#define SAMPLES   32          // Amount of Samples
#define SAMPLE_F  1000        // Frequency of sample collections
#define NOISE     50          // to filter out noise

double g_Real[SAMPLES];
double g_Img[SAMPLES];
arduinoFFT FFT = arduinoFFT(g_Real, g_Img, SAMPLES, SAMPLE_F);
int g_combo;                  // combination of all fft samples
unsigned long g_sp;           // sampling period
unsigned long g_startTime;    // keeping track of time for the sampling period

int last_limited_val = 0;     // will become static if i decide to keep it - which is rather unlikely

void displayLEDs() {
  // limited value is based on the maximal input and the amount of LEDs
  // in this case the analog-in of the micro maxes out around 3000 split on 15 LEDs
  // results in ~200 per LED

  // despite being heavly weighted torwards the new value, gives slight desync between color explosion and music
  // TODO: different method of display!
  int limited_val = ((g_combo / 200 <= NUM_LEDS ? g_combo / 200 : NUM_LEDS) * 0.7) + (last_limited_val * 0.3);
  last_limited_val = limited_val;
  byte hue = 90 - map(limited_val, 0, 15, 0, 82);  // hue of 0 is red, 90 is green
  
  FastLED.clear(false);
  fill_solid(g_LEDs, limited_val, CHSV(hue, 255, 255));  // automatic transformation from CHSV to CRGB
  FastLED.show();
}

void setup() {
  delay(1000);  // sanity delay

  // FastLED setup II
  pinMode(LED_PIN, OUTPUT);
  FastLED.addLeds<LED_CHIP, LED_PIN, CLR_ORD>(g_LEDs, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  g_fpsDelay = 1000 / FPS;

  // sound setup II
  analogReference(DEFAULT);
  g_sp = round(1000000 * (1.0 / SAMPLE_F));

  // display rediness!
  g_LEDs[1] = CRGB::Red;
  FastLED.show();
  delay(1000);
  g_LEDs[1] = CRGB::Black;
  FastLED.show();
}

void loop() {
  // Collecting the samples
  for(int i = 0; i < SAMPLES; i++) {
    g_startTime = micros();
    g_Real[i] = analogRead(SOUND_PIN);
    g_Img[i] = 0;
    while (micros() < (g_startTime + g_sp)) { /* timeout */}
  }

  // Let fft do its thing
  FFT.DCRemoval();
  FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(FFT_FORWARD);
  FFT.ComplexToMagnitude();

  // combining the non-noise samples to get overall loudness
  g_combo = 0;
  // first entry and last half are supposed to be useless - therefore skip!
  for(int i = 1; i < SAMPLES / 2; i++) {
    if (g_Real[i] < NOISE) continue;

    g_combo += static_cast<int>(g_Real[i]);
  }

  // display on LEDs
  if (g_fpsDelay < millis() - g_fpsTime) {
    displayLEDs();
    g_fpsTime = millis();
  }
}