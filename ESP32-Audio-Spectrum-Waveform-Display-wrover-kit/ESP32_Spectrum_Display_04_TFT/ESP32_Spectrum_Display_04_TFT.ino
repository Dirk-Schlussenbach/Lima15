/* ESP32 Audio Spectrum Analyser on an SSD1306/SH1106 Display, 8-bands 125, 250, 500, 1k, 2k, 4k, 8k, 16k
 * Improved noise performance and speed and resolution.
  *####################################################################################################################################  
 This software, the ideas and concepts is Copyright (c) David Bird 2018. All rights to this software are reserved.
 
 Any redistribution or reproduction of any part or all of the contents in any form is prohibited other than the following:
 1. You may print or download to a local hard disk extracts for your personal and non-commercial use only.
 2. You may copy the content to individual third parties for their personal use, but only if you acknowledge the author David Bird as the source of the material.
 3. You may not, except with my express written permission, distribute or commercially exploit the content.
 4. You may not transmit it or store it in any other website or other form of electronic retrieval system for commercial purposes.

 The above copyright ('as annotated') notice and this permission notice shall be included in all copies or substantial portions of the Software and where the
 software use is visible to an end-user.
 
 THE SOFTWARE IS PROVIDED "AS IS" FOR PRIVATE USE ONLY, IT IS NOT FOR COMMERCIAL USE IN WHOLE OR PART OR CONCEPT. FOR PERSONAL USE IT IS SUPPLIED WITHOUT WARRANTY 
 OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 See more at http://www.dsbird.org.uk

*/

#include <Wire.h>
#include "arduinoFFT.h" // Standard Arduino FFT library https://github.com/kosme/arduinoFFT


#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

#define SAMPLES 1024             // Must be a power of 2
#define SAMPLING_FREQUENCY 40000 // Hz, must be 40000 or less due to ADC conversion time. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
#define amplitude 150            // Depending on your audio source level, you may need to increase this value
unsigned int sampling_period_us;
unsigned long microseconds;
byte peak[] = {0,0,0,0,0,0,0,0};
double vReal[SAMPLES];
double vImag[SAMPLES];
unsigned long newTime, oldTime;
int dominant_value;

ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQUENCY);

/////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  tft.init();
  tft.setRotation(3);
  //tft.setTextSize(2);
  tft.invertDisplay(true);
  tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(&FreeMono9pt7b);
  //tft.println(" Initialised default\n");
  //tft.println(" White text");
  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
}

void loop() { 
  tft.drawString("125 250 500 1K  2K 4K 8K 16K",0,0);
  for (int i = 0; i < SAMPLES; i++) {
    newTime = micros();
    //vReal[i] = analogRead(A0); // Using Arduino ADC nomenclature. A conversion takes about 1uS on an ESP32
    
    //vReal[i] = random(400);
   vReal[i] = 70; 
  
  //vReal[i] = analogRead(VP); // Using logical name fo ADC port
  //vReal[i] = analogRead(36); // Using pin number for ADC port
    vImag[i] = 0;
    while ((micros() - newTime) < sampling_period_us) { /* do nothing to wait */ }
  }
  FFT.windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.complexToMagnitude(vReal, vImag, SAMPLES);
  for (int i = 2; i < (SAMPLES/2); i++){ // Don't use sample 0 and only the first SAMPLES/2 are usable.
    // Each array element represents a frequency and its value, is the amplitude. Note the frequencies are not discrete.
    if (vReal[i] > 1500) { // Add a crude noise filter, 10 x amplitude or more
      if (i<=2 )             displayBand(0,(int)vReal[i]); // 125Hz
      if (i >2   && i<=4 )   displayBand(1,(int)vReal[i]); // 250Hz
      if (i >4   && i<=7 )   displayBand(2,(int)vReal[i]); // 500Hz
      if (i >7   && i<=15 )  displayBand(3,(int)vReal[i]); // 1000Hz
      if (i >15  && i<=40 )  displayBand(4,(int)vReal[i]); // 2000Hz
      if (i >40  && i<=70 )  displayBand(5,(int)vReal[i]); // 4000Hz
      if (i >70  && i<=288 ) displayBand(6,(int)vReal[i]); // 8000Hz
      if (i >288           ) displayBand(7,(int)vReal[i]); // 16000Hz
      //Serial.println(i);
    }
    for (byte band = 0; band <= 7; band++) tft.drawFastHLine(1+32*band,128-peak[band],28, TFT_RED); //16 - 64 - 14
  }
  if (millis()%4 == 0) {for (byte band = 0; band <= 7; band++) {if (peak[band] > 0) peak[band] -= 1;}} // Decay the peak
  //tft.display();
}

void displayBand(int band, int dsize){
  int dmax = 50;
  dsize /= amplitude;
  if (dsize > dmax) dsize = dmax;
  for (int s = 0; s <= dsize; s=s+2){    for (byte band = 0; band <= 7; band++) tft.drawFastHLine(1+32*band,128-peak[band],28,TFT_RED);} //16 - 64 - 14
  if (dsize > peak[band]) {peak[band] = dsize;}
}
