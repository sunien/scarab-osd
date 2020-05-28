#ifdef PIXELOSD

#include "FrSkyPixelOsd.h"

SoftwareSerial osdSerial(PIXELOSDRX, PIXELOSDTX);
FrSkyPixelOsd osd(&osdSerial);
bool osdSerialConfigured = false;
uint8_t MAX_screen_cols;

void OSD_Setup()
{
  if(osdSerialConfigured == false) 
  {
    osd.begin(57600);
    osdSerialConfigured = true;
  }
  MAX_screen_size = 480;
  MAX_screen_cols = 30;
  flags.signaltype = 1; // PAL
  flags.signalauto = flags.signaltype;
}

void OSD_DrawScreen()
{
  uint16_t xx;

#ifdef SCREENTEST
   for(xx = 0; xx < MAX_screen_size; ++xx) {
     screen[xx] = 48 + (xx %10) ;
      if ((xx%30)==8)
        screen[xx] = 48 + ((xx/30) % 10) ; 
      if ((xx%30)==7)
        screen[xx] = ' ' ; 
      if ((xx%30)==9)
        screen[xx] = ' ' ;    
      if ((xx>120)&&(xx<300))
        screen[xx] = ' ' ;    
   }    
   for (uint8_t X = 0; X <= 3; X++) {
     OSD_WriteString_P(PGMSTR(&(screen_test[X])), (LINE*5)+10+(X*LINE));
   }      
#endif // SCREENTEST

//  osd.cmdTransactionBegin();
  for(xx = 0; xx < MAX_screen_size; ++xx) {
    osd.cmdDrawGridChar(xx % MAX_screen_cols, xx / MAX_screen_cols, (uint8_t)screen[xx], 0);
#ifdef CANVAS_SUPPORT
    if (!canvasMode) // Don't erase in canvas mode
#endif
    {
      screen[xx] = ' ';
    }
  }
//  osd.cmdTransactionCommit();
}

void OSD_WriteString(const char *string, int addr)
{
  char *screenp = &screen[addr];
  while (*string) {
    *screenp++ = *string++;
  }

}

// Copy string from progmem into the screen buffer
void OSD_WriteString_P(const char *string, int Adresse)
{
  char c;
  char *screenp = &screen[Adresse];
  while((c = (char)pgm_read_byte(string++)) != 0)
    *screenp++ = c;
}

#ifdef CANVAS_SUPPORT
void OSD_ClearScreen(void)
{
  for(uint16_t xx = 0; xx < MAX_screen_size; ++xx) {
    screen[xx] = ' ';
  }
}
#endif

void OSD_WriteNVM(uint8_t char_address)
{
  FrSkyPixelOsd::osd_chr_data_t font;
  FrSkyPixelOsd::osd_chr_data_t response;
  for(uint8_t x = 0; x < 54; x++) font.data[x] = serialBuffer[1+x];
  for(uint8_t x = 0; x < 10; x++) font.metadata[x] = 0;
  osd.cmdWriteFont(char_address, &font, &response);
}

#if defined(AUTOCAM) || defined(MAXSTALLDETECT)
void OSD_CheckStatus(void){
}
#endif

#if defined LOADFONT_DEFAULT || defined LOADFONT_LARGE || defined LOADFONT_BOLD || defined DISPLAYFONTS
void OSD_DisplayFont()
{
  for(uint16_t x = 0; x < 256; x++) {
    screen[90+x] = x;
  }
}

#ifndef DISPLAYFONTS
void OSD_UpdateFont()
{ 
  for(uint16_t x = 0; x < 256; x++){
    for(uint8_t i = 0; i < 54; i++){
      serialBuffer[1+i] = (uint8_t)pgm_read_byte(fontdata+(64*x)+i);
    }
    OSD_WriteNVM(x);
    ledstatus=!ledstatus;
    if (ledstatus==true){
      digitalWrite(LEDPIN,HIGH);
    }
    else{
      digitalWrite(LEDPIN,LOW);
    }
  }
}
#endif
#endif

#endif // PIXELOSD
