#include <Arduino.h>
#include <FastLED.h>
#include <arduinoFFT.h>

// generel setup
#define FPS         30

// FastLED setup
#define NUM_LEDS    15
#define CLR_ORD     GRB
#define BRIGHTNESS  5         // low brightness to not fry USB-port while testing around
#define LED_CHIP    WS2812B
#define LED_PIN     13        // Digital Pin

CRGB g_LEDs[NUM_LEDS] = {0};  // LED Array Buffer
unsigned long g_fpsDelay;     // Delay based on FPS
unsigned long g_fpsTime = 0;  // keeping track of time for LED fps

// Sound Setup
#define SOUND_PIN A0          // Analog Pin
#define SAMPLES   32          // Amount of Samples
#define SAMPLE_F  1000        // Frequency of sample collections
#define NOISE     6           // to filter out noise

double g_Real[SAMPLES];
double g_Img[SAMPLES];
arduinoFFT FFT = arduinoFFT(g_Real, g_Img, SAMPLES, SAMPLE_F);
int g_combo;                  // combination of all fft samples
unsigned long g_sp;           // sampling period
unsigned long g_startTime;    // keeping track of time for the sampling period

void displayLEDs() {
  // TODOs:
  // variable color (small g_combo => blueish color; high g_combo => redish color)
  // decay to current g_combo rather than blacking out every LED
  
  FastLED.clear(false);
  for(int i = 0; i < g_combo / 5 && i < NUM_LEDS; i++) {
    g_LEDs[i] = CRGB::OrangeRed;
  }
  FastLED.show();
}

void setup() {
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  FastLED.addLeds<LED_CHIP, LED_PIN, CLR_ORD>(g_LEDs, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  g_fpsDelay = 1000 / FPS;

  analogReference(DEFAULT);
  g_sp = round(1000000 * (1.0 / SAMPLE_F));

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