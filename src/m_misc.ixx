//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
//
// $Log:$
//
// DESCRIPTION:
//	Main loop menu stuff.
//	Default Config File.
//	PCX Screenshots.
//
//-----------------------------------------------------------------------------
module;

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <ctype.h>

#include "doomdef.h"

#include "z_zone.h"

#include "m_argv.h"
#include "m_swap.h"

#include "w_wad.h"

#include "i_video.h"
#include "v_video.h"

#include "hu_stuff.h"

// State.
#include "doomstat.h"

// Data.
#include "dstrings.h"
export module m_misc;

//
// M_DrawText
// Returns the final X coordinate
// HU_Init must have been called to init the font
//
patch_t *hu_font[HU_FONTSIZE];

int M_DrawText(int x, int y, boolean direct, char *string) {
  int c;
  int w;

  while (*string) {
    c = toupper(*string) - HU_FONTSTART;
    string++;
    if (c < 0 || c > HU_FONTSIZE) {
      x += 4;
      continue;
    }

    w = SHORT(hu_font[c]->width);
    if (x + w > SCREENWIDTH)
      break;
    if (direct)
      V_DrawPatchDirect(x, y, 0, hu_font[c]);
    else
      V_DrawPatch(x, y, 0, hu_font[c]);
    x += w;
  }

  return x;
}

//
// DEFAULTS
//
int usemouse;
int usejoystick;

extern int key_right;
extern int key_left;
extern int key_up;
extern int key_down;

extern int key_strafeleft;
extern int key_straferight;

extern int key_fire;
extern int key_use;
extern int key_strafe;
extern int key_speed;

export int mousebfire;
export int mousebstrafe;
export int mousebforward;

export int joybfire;
export int joybstrafe;
export int joybuse;
export int joybspeed;

export int viewwidth;
export int viewheight;

export int mouseSensitivity;

// Show messages has default, 0 = off, 1 = on
export int showMessages;

// Blocky mode, has default, 0 = high, 1 = normal
export int detailLevel;

export int screenblocks;

// machine-independent sound params
export int numChannels;

// UNIX hack, to be removed.
#ifdef SNDSERV
extern const char *sndserver_filename;
extern int mb_used;
#endif

#ifdef LINUX
char *mousetype;
char *mousedev;
#endif

extern char *chat_macros[];

typedef struct {
  const char *name;
  int *location;
  int defaultvalue;
  int scantranslate; // PC scan code hack
  int untranslated;  // lousy hack
} default_t;

default_t defaults[] = {
    {"mouse_sensitivity", &mouseSensitivity, 5},
    {"sfx_volume", &snd_SfxVolume, 8},
    {"music_volume", &snd_MusicVolume, 8},
    {"show_messages", &showMessages, 1},

#ifdef NORMALUNIX
    {"key_right", &key_right, KEY_RIGHTARROW},
    {"key_left", &key_left, KEY_LEFTARROW},
    {"key_up", &key_up, KEY_UPARROW},
    {"key_down", &key_down, KEY_DOWNARROW},
    {"key_strafeleft", &key_strafeleft, ','},
    {"key_straferight", &key_straferight, '.'},

    {"key_fire", &key_fire, KEY_RCTRL},
    {"key_use", &key_use, ' '},
    {"key_strafe", &key_strafe, KEY_RALT},
    {"key_speed", &key_speed, KEY_RSHIFT},

// UNIX hack, to be removed.
#ifdef SNDSERV
    {"sndserver", (int *)&sndserver_filename, (int)"sndserver"},
    {"mb_used", &mb_used, 2},
#endif

#endif

#ifdef LINUX
    {"mousedev", (int *)&mousedev, (int)"/dev/ttyS0"},
    {"mousetype", (int *)&mousetype, (int)"microsoft"},
#endif

    {"use_mouse", &usemouse, 1},
    {"mouseb_fire", &mousebfire, 0},
    {"mouseb_strafe", &mousebstrafe, 1},
    {"mouseb_forward", &mousebforward, 2},

    {"use_joystick", &usejoystick, 0},
    {"joyb_fire", &joybfire, 0},
    {"joyb_strafe", &joybstrafe, 1},
    {"joyb_use", &joybuse, 3},
    {"joyb_speed", &joybspeed, 2},

    {"screenblocks", &screenblocks, 9},
    {"detaillevel", &detailLevel, 0},

    {"snd_channels", &numChannels, 3},

    {"usegamma", &usegamma, 0},
    /*
    {"chatmacro0", (int *) &chat_macros[0],
    reinterpret_cast<intptr_t>(HUSTR_CHATMACRO0) },
    {"chatmacro1", (int *) &chat_macros[1],
    reinterpret_cast<intptr_t>(HUSTR_CHATMACRO1) },
    {"chatmacro2", (int *) &chat_macros[2],
    reinterpret_cast<intptr_t>(HUSTR_CHATMACRO2) },
    {"chatmacro3", (int *) &chat_macros[3],
    reinterpret_cast<intptr_t>(HUSTR_CHATMACRO3) },
    {"chatmacro4", (int *) &chat_macros[4],
    reinterpret_cast<intptr_t>(HUSTR_CHATMACRO4) },
    {"chatmacro5", (int *) &chat_macros[5],
    reinterpret_cast<intptr_t>(HUSTR_CHATMACRO5) },
    {"chatmacro6", (int *) &chat_macros[6],
    reinterpret_cast<intptr_t>(HUSTR_CHATMACRO6) },
    {"chatmacro7", (int *) &chat_macros[7],
    reinterpret_cast<intptr_t>(HUSTR_CHATMACRO7) },
    {"chatmacro8", (int *) &chat_macros[8],
    reinterpret_cast<intptr_t>(HUSTR_CHATMACRO8) },
    {"chatmacro9", (int *) &chat_macros[9],
    reinterpret_cast<intptr_t>(HUSTR_CHATMACRO9) }*/

};

int numdefaults;
const char *defaultfile;

//
// M_SaveDefaults
//
export void M_SaveDefaults(void) {
  int i;
  int v;
  FILE *f;

  f = fopen(defaultfile, "w");
  if (!f)
    return; // can't write the file, but don't complain

  for (i = 0; i < numdefaults; i++) {
    if (defaults[i].defaultvalue > -0xfff && defaults[i].defaultvalue < 0xfff) {
      v = *defaults[i].location;
      fprintf(f, "%s\t\t%i\n", defaults[i].name, v);
    } else {
      fprintf(f, "%s\t\t\"%s\"\n", defaults[i].name,
              *(char **)(defaults[i].location));
    }
  }

  fclose(f);
}

//
// M_LoadDefaults
//
extern byte scantokey[128];

export void M_LoadDefaults(void) {
  int i;
  FILE *f;
  char def[80];
  char strparm[100];
  char *newstring;
  int parm;
  boolean isstring;

  // set everything to base values
  numdefaults = sizeof(defaults) / sizeof(defaults[0]);
  for (i = 0; i < numdefaults; i++)
    *defaults[i].location = defaults[i].defaultvalue;

  // check for a custom default file
  i = M_CheckParm("-config");
  if (i && i < myargc - 1) {
    defaultfile = myargv[i + 1].c_str();
    printf("	default file: %s\n", defaultfile);
  } else {
    // @TODO JONNY Circular module dependency here
    //defaultfile = basedefault;
  }

  // read the file in, overriding any set defaults
  f = fopen(defaultfile, "r");
  if (f) {
    while (!feof(f)) {
      isstring = false;
      if (fscanf(f, "%79s %[^\n]\n", def, strparm) == 2) {
        if (strparm[0] == '"') {
          // get a string default
          isstring = true;
          const auto len = strlen(strparm);
          newstring = (char *)malloc(len);
          strparm[len - 1] = 0;
          strcpy(newstring, strparm + 1);
        } else if (strparm[0] == '0' && strparm[1] == 'x')
          sscanf(strparm + 2, "%x", &parm);
        else
          sscanf(strparm, "%i", &parm);
        for (i = 0; i < numdefaults; i++)
          if (!strcmp(def, defaults[i].name)) {
            if (!isstring)
              *defaults[i].location = parm;
            else
              *defaults[i].location =
                  static_cast<int>(reinterpret_cast<intptr_t>(newstring));
            break;
          }
      }
    }

    fclose(f);
  }
}

//
// SCREEN SHOTS
//

typedef struct {
  char manufacturer;
  char version;
  char encoding;
  char bits_per_pixel;

  unsigned short xmin;
  unsigned short ymin;
  unsigned short xmax;
  unsigned short ymax;

  unsigned short hres;
  unsigned short vres;

  unsigned char palette[48];

  char reserved;
  char color_planes;
  unsigned short bytes_per_line;
  unsigned short palette_type;

  char filler[58];
  unsigned char data; // unbounded
} pcx_t;

//
// WritePCXfile
//
void WritePCXfile(char *filename, byte *data, int width, int height,
                  byte *palette) {
  int i;
  pcx_t *pcx;
  byte *pack;

  pcx = static_cast<pcx_t *>(malloc(width * height * 2 + 1000));

  pcx->manufacturer = 0x0a; // PCX id
  pcx->version = 5;         // 256 color
  pcx->encoding = 1;        // uncompressed
  pcx->bits_per_pixel = 8;  // 256 color
  pcx->xmin = 0;
  pcx->ymin = 0;
  pcx->xmax = SHORT(width - 1);
  pcx->ymax = SHORT(height - 1);
  pcx->hres = SHORT(width);
  pcx->vres = SHORT(height);
  memset(pcx->palette, 0, sizeof(pcx->palette));
  pcx->color_planes = 1; // chunky image
  pcx->bytes_per_line = SHORT(width);
  pcx->palette_type = SHORT(2); // not a grey scale
  memset(pcx->filler, 0, sizeof(pcx->filler));

  // pack the image
  pack = &pcx->data;

  for (i = 0; i < width * height; i++) {
    if ((*data & 0xc0) != 0xc0)
      *pack++ = *data++;
    else {
      *pack++ = 0xc1;
      *pack++ = *data++;
    }
  }

  // write the palette
  *pack++ = 0x0c; // palette ID byte
  for (i = 0; i < 768; i++)
    *pack++ = *palette++;

  // write output file
  const auto length = pack - (byte *)pcx;
  // M_WriteFile (filename, pcx, static_cast<int>(length));

  free(pcx);
}

//
// M_ScreenShot
//
export void M_ScreenShot(void) {
  // JONNY TODO
}