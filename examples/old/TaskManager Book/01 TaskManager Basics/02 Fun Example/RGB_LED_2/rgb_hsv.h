#ifndef RGB_HSV_H_INCLUDED
#define RGB_HSV_H_INCLUDED

void hsv_to_rgb(byte h, byte s, byte v, byte& r, byte& g, byte& b);

void rgb_to_hsv(byte r, byte g, byte b, byte& h, byte& s, byte& v);

#endif // RGB_HSV_H_INCLUDED
