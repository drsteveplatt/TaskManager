#include <Arduino.h>
//#include "Streaming.h"
#include "rgb_hsv.h"

// Code strategy from http://en.wikipedia.org/wiki/HSL_and_HSV
// modified for byte operations -- since S and V are actually [0 255]
// but represent values [0 1], we <<8 after multiplication.

/*! \brief Convert HSV to RGB

    Converts an HSV value to the corresponding RGB value.
    \param h - hue
    \param s - saturation
    \param v - value
    \param r - red output
    \param g - green output
    \param b - blue output
*/
void hsv_to_rgb(byte h, byte s, byte v, byte&r, byte&g, byte& b) {
    // code based on http://www.kasperkamperman.com/blog/arduino/arduino-programming-hsb-to-rgb/
      int base;
      int hue;

      hue = ((long int)h*360)/255;
      if(hue==360) hue=0;

  if (s == 0) { // Acromatic color (gray). Hue doesn't mind.
    r=v;
    g=v;
    b=v;
  } else  {

    base = ((255 - s) * v)>>8;
    switch(hue/60) {
    case 0:
        r = v;
        g = (((v-base)*hue)/60)+base;
        b = base;
    break;

    case 1:
        r = (((v-base)*(60-(hue%60)))/60)+base;
        g = v;
        b = base;
    break;

    case 2:
        r = base;
        g = v;
        b = (((v-base)*(hue%60))/60)+base;
    break;

    case 3:
        r = base;
        g = (((v-base)*(60-(hue%60)))/60)+base;
        b = v;
    break;

    case 4:
        r = (((v-base)*(hue%60))/60)+base;
        g = base;
        b = v;
    break;

    case 5:
        r = v;
        g = base;
        b = (((v-base)*(60-(hue%60)))/60)+base;
    break;
    }

  }
}

/*! \brief Convert RGB to HSV
    Converts an HSV value to the corresponding RGB value.
    \param r - red
    \param g - green
    \param b - blue
    \param h - hue output
    \param s - saturation output
    \param v - value output
*/
void rgb_to_hsv(byte r, byte g, byte b, byte& h, byte& s, byte& v) {
    byte M, m;
    unsigned int L, Hp;
    unsigned int sTmp;
    M = (r>g) ? ((r>b)?r:b) : ((g>b)?g:b);    // max
    m = (r<g) ? ((r<b)?r:b) : ((g<b)?g:b);    // min

    v = M;
    if(v==0) { h = 0; s = 0; return; }  // black

    sTmp = (((unsigned int)M-m)<<8)/v;
    s = (sTmp>255)?255:sTmp;
    if(s==0) { h=0; return; }           // gray of some sort

    if(M==r) {
            h = 43 * (g-b) / (M-m);
    } else if(M==g) {
        h = 85 + 43 * (b-r) / (M-m);
    } else {
        h = 171 + 43 * (r-g) / (M-m);
    }

}
