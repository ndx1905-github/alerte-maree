 /*
  ColorduinoPlasma - Plasma demo using Colorduino Library for Arduino
  Copyright (c) 2011 Sam C. Lin lincomatic@hotmail.com ALL RIGHTS RESERVED
  based on  Color cycling plasma   
    Version 0.1 - 8 July 2009
    Copyright (c) 2009 Ben Combee.  All right reserved.
    Copyright (c) 2009 Ken Corey.  All right reserved.
    Copyright (c) 2008 Windell H. Oskay.  All right reserved.
    Copyright (c) 2011 Sam C. Lin All Rights Reserved
  This demo is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This demo is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
Pour programmer le colorduino, utiliser une arduino uno dont on a enlevé la puce
utiliser les branchements 
5V --> VDD
GND --> GND
0(RX) --> RXD
1(TX) --> TXD
RST --> DTR 


*/

#include <Colorduino.h>
#include <Wire.h>  // connexion serie avec le calculateur de maree

const int marnage=8; //marnage -> amplitude affichage (nombre de leds)
float dureeMaree=372.616667*719/720;  // duree -> estimation durée moyenne d'un cycle en minutes entre PM et BM et correction dérive 1 minute toutes les 12h
int deltaMinutes=255*3 ; // temps depuis la dernière marée est au max 373*2= 746. Valeur max transmise par le calculateur de maréee est 255. 3*255=765
int UpdateTime=0 ;
int message ;
int coefficientMaree = 253 ; // 251 mortes eaux / 252 faible maree / 253 normal / 254 forte maree / 255 vives eaux 

typedef struct
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
} ColorRGB;

//a color with 3 components: h, s and v
typedef struct 
{
  unsigned char h;
  unsigned char s;
  unsigned char v;
} ColorHSV;

// increase the scaling of the plasma to see more details of the color transition.
unsigned char plasma[ColorduinoScreenWidth][ColorduinoScreenHeight];
long paletteShift;


void HSVtoRGB(void *vRGB, void *vHSV) 
{
  float r, g, b, h, s, v;
  float f, p, q, t;
  int i;
  ColorRGB *colorRGB=(ColorRGB *)vRGB;
  ColorHSV *colorHSV=(ColorHSV *)vHSV;

  h = (float)(colorHSV->h / 256.0);
  s = (float)(colorHSV->s / 256.0);
  v = (float)(colorHSV->v / 256.0);

//if saturation is 0, the color is a shade of grey
  if(s == 0.0) {
    b = v;
    g = b;
    r = g;
  }
//if saturation > 0, more complex calculations are needed
  else
  {
    h *= 6.0; //to bring hue to a number between 0 and 6, better for the calculations
    i = (int)(floor(h)); //e.g. 2.7 becomes 2 and 3.01 becomes 3 or 4.9999 becomes 4
    f = h - i; //the fractional part of h

    p = (float)(v * (1.0 - s));
    q = (float)(v * (1.0 - (s * f)));
    t = (float)(v * (1.0 - (s * (1.0 - f))));

    switch(i)
    {
      case 0: r=v; g=t; b=p; break;
      case 1: r=q; g=v; b=p; break;
      case 2: r=p; g=v; b=t; break;
      case 3: r=p; g=q; b=v; break;
      case 4: r=t; g=p; b=v; break;
      case 5: r=v; g=p; b=q; break;
      default: r = g = b = 0; break;
    }
  }
  
  colorRGB->r = (int)(min(r,g) * 15.0);           //  A bidouiller
  colorRGB->g = (int)(g * 45.0);                  //  pour modifier
  colorRGB->b = (int)(max(max(r,g),b) * 155.0);   //  la couleur de la mer

}

unsigned int RGBtoINT(void *vRGB)
{
  ColorRGB *colorRGB=(ColorRGB *)vRGB;

  return (((unsigned int)colorRGB->r)<<16) + (((unsigned int)colorRGB->g)<<8) + (unsigned int)colorRGB->b;
}


float
dist(float a, float b, float c, float d) 
{
  return sqrt((c-a)*(c-a)+(d-b)*(d-b));
}


void
plasma_morph()
{
  unsigned char x,y;
  float value;
  ColorRGB colorRGB;
  ColorHSV colorHSV;

  for(y = 0; y < ColorduinoScreenHeight; y++)
    for(x = 0; x < ColorduinoScreenWidth; x++) {
      {
  value = sin(dist(x + paletteShift, y, 128.0, 128.0) / 8.0)
    + sin(dist(x, y, 64.0, 64.0) / 8.0)
    + sin(dist(x, y + paletteShift / 7, 192.0, 64) / 7.0)
    + sin(dist(x, y, 192.0, 100.0) / 8.0);
  colorHSV.h=(unsigned char)((value) * 128)&0xff;
  colorHSV.s=255; 
  colorHSV.v=255;
  HSVtoRGB(&colorRGB, &colorHSV);
  
  Colorduino.SetPixel(x, y, colorRGB.r, colorRGB.g, colorRGB.b);
      }
  }
  paletteShift++;

}

void ColorFill(unsigned char R,unsigned char G,unsigned char B)
{
  PixelRGB *p = Colorduino.GetPixel(0,0);
  for (unsigned char y=0;y<ColorduinoScreenWidth;y++) {
    for(unsigned char x=0;x<ColorduinoScreenHeight;x++) {
      p->r = R;
      p->g = G;
      p->b = B;
      p++;
    }
  }
  
}

void setup()
{
 
  Wire.begin(0x09);                 // initialisation du bus I2C avec comme adresse 9 en decimal soit 0x09 en hexadecimal

  Wire.onReceive(receiveEvent);   //  Attacher une fonction a declencher lorsque quelque chose est recu

  Serial.begin(9600);
  Colorduino.Init();  

  unsigned char whiteBalVal[3] = {14,63,63}; // balance des blancs {14,63,63} OU {1,4,4} moins lumineux
  Colorduino.SetWhiteBal(whiteBalVal);
  
//  deltaMinutes = 240 ; // test valeur
   
  paletteShift=128000;
  unsigned char bcolor;

  for(unsigned char y = 0; y < ColorduinoScreenHeight; y++)
    for(unsigned char x = 0; x < ColorduinoScreenWidth; x++)
    {

      bcolor = (unsigned char)
      (
            128.0 + (128.0 * sin(x*8.0 / 16.0))
          + 128.0 + (128.0 * sin(y*8.0 / 16.0))
      ) / 2;
      plasma[x][y] = bcolor;
    }
    
}

void receiveEvent() {
  message = Wire.read() ; // lire le caractere recu sur le bus I2C
  if ( message > 250 ) { coefficientMaree = message ; }
    else { deltaMinutes = message *3 ; }
  
}

void loop()
{

//deltaMinutes = deltaMinutes + int(millis()/60000) - UpdateTime ; 

  
plasma_morph();
plage(deltaMinutes); // fonction qui affiche la hauteur de plage-sable

delay (160);

/*
if ( deltaMinutes == 255*3 || abs ( UpdateTime - int ( millis () / 60000 ) > 60) ) {    // au debut et si on n'a pas recu de mise a jour depuis plus de x min
    for (int j = 0 ; j < 8 ; j++) { 
        for (int i = 0 ; i < 8 ; i++) {Colorduino.SetPixel(i,j,30,30,30);} // alors on affiche un ecran gris
        }
    }
*/

Colorduino.FlipPage(); 

Serial.println(message);
Serial.println(deltaMinutes);
}

int hauteur(int deltaMinutes)
{
  return round(8-marnage*sq(sin(radians(90*deltaMinutes/dureeMaree))));  // hauteur d'eau sur une echelle de 8
}

void plage(int deltaMinutes)
{

//d'abord colonne de gauche i=0 soit maintenant
for (int j = hauteur(deltaMinutes+60*0) ; j < 8 ; j++) {
   Colorduino.SetPixel(0,j,255,255,25); // jaune foncé
    }

//puis i=1  soit neutralisé en colonne noire, soit affiche la force du coefficient
for (int j = 0 ; j < 8 ; j++) { Colorduino.SetPixel(1,j,0,0,0);} // colonne noire de séparation
switch(coefficientMaree)                                         // le cas échéant on surimprime le coeff de maree
    {
      case 251: for (int j = 3 ; j < 5 ; j++) { Colorduino.SetPixel(1,j,20,100,20);} break; //mortes eaux
      case 252: for (int j = 2 ; j < 6 ; j++) { Colorduino.SetPixel(1,j,20,200,20);} break; //faible maree
      case 253:  break;                                                                       //normal : on laisse la colonne vide 
      case 254: for (int j = 1 ; j < 7 ; j++) { Colorduino.SetPixel(1,j,100,20,20);} break; //forte maree
      case 255: for (int j = 0 ; j < 8 ; j++) { Colorduino.SetPixel(1,j,200,0,0);} break;  //vives eaux
    }


//et enfin i=2 à 7 qui affiche en réalité i=1 à 6 soit maree dans une heure, deux heures etc.
for (int i = 1 ; i < 7 ; i++) {
  if (hauteur(deltaMinutes+60*i)==constrain(hauteur(deltaMinutes+60*i),0,7))
  {
  for (int j = hauteur(deltaMinutes+60*i) ; j < 8 ; j++) {
    Colorduino.SetPixel(i+1,j,255,255,25); // jaune foncé pour le sable
    }
  } 
}

}
