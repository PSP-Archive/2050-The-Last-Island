
/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later vvoersion.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "quakedef.h"

#ifdef WIN32
#include "winquake.h"
#endif

#ifdef PSP
#include <pspkernel.h>
#include <psputility.h>
#include "net_dgrm.h"

extern int tcpprotocol;

extern cvar_t	accesspoint;
extern cvar_t	r_wateralpha;
extern cvar_t	r_vsync;
extern cvar_t	in_disable_analog;
extern cvar_t	in_analog_strafe;
extern cvar_t	in_x_axis_adjust;
extern cvar_t	in_y_axis_adjust;
extern cvar_t	crosshair;
extern cvar_t	r_dithering;

extern cvar_t	sp;
extern cvar_t	mp;
extern cvar_t	op;
extern cvar_t	cr;
extern cvar_t	ex;

extern cvar_t	st;
extern cvar_t	se;
extern cvar_t	ld;

extern cvar_t	dt;
extern cvar_t	gba;


extern cvar_t	crosshair;
extern cvar_t	accesspoint;
#endif

#ifdef PSP_MP3_HWDECODE
extern int changeMp3Volume;
#endif

extern qboolean bmg_type_changed;

cvar_t	scr_centermenu = {"scr_centermenu", "1"};
int	m_yofs = 0;

void (*vid_menudrawfn)(void);
void (*vid_menukeyfn)(int key);
void Draw_Fill (int x, int y, int w, int h, int c);
//void ShowMessageDialog(const char *message, int enableYesno);

//void Draw_FadeScreenNew (int x, int y, int x1, int y1, int r, int g, int b, int alpha);

enum {m_none, m_main, m_singleplayer,m_options, m_keys, m_quit,m_language, m_osk, 
m_load,
m_save,
m_sedit} m_state;

void M_Menu_Main_f (void);
     void M_Menu_SinglePlayer_f (void);
		void M_Menu_Load_f (void);
		void M_Menu_Save_f (void);
     void M_Menu_Language_f (void);
	void M_Menu_Options_f (void);
		void M_Menu_Keys_f (void);
	void M_Menu_Quit_f (void);

void M_Main_Draw (void);
	void M_SinglePlayer_Draw (void);
		void M_Load_Draw (void);
		void M_Save_Draw (void);
     void M_Language_Draw (void);
	void M_Options_Draw (void);
		void M_Keys_Draw (void);
	void M_Quit_Draw (void);


void M_Main_Key (int key);
	void M_SinglePlayer_Key (int key);
	void M_Load_Key (int key);
	void M_Save_Key (int key);
      void M_Language_Key (int key);
	void M_Options_Key (int key);
		void M_Keys_Key (int key);
	void M_Quit_Key (int key);

void Con_SetOSKActive(qboolean active);
void M_Menu_OSK_f (char *input, char *output, int outlen);

qboolean	m_entersound;		// play after drawing a frame, so caching
								// won't disrupt the sound
qboolean	m_recursiveDraw;

int			m_return_state;
qboolean	m_return_onerror;
char		m_return_reason [32];


/*
================
M_DrawCharacter

Draws one solid graphics character
================
*/
void M_DrawCharacter (int cx, int line, int num)
{
	Draw_Character ( cx + ((vid.width - 320)>>1), line, num);
}

void M_Print (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, (*str)+128);
		str++;
		cx += 8;
	}
}

void M_PrintWhite (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, *str);
		str++;
		cx += 8;
	}
}

void M_DrawTransPic (int x, int y, qpic_t *pic)
{
	Draw_TransPic (x + ((vid.width - 320)>>1), y, pic);
}

void M_DrawPic (int x, int y, qpic_t *pic)
{
	Draw_Pic (x + ((vid.width - 320)>>1), y, pic);
}

byte identityTable[256];
byte translationTable[256];

void M_BuildTranslationTable(int top, int bottom)
{
	int		j;
	byte	*dest, *source;

	for (j = 0; j < 256; j++)
		identityTable[j] = j;
	dest = translationTable;
	source = identityTable;
	memcpy (dest, source, 256);

	if (top < 128)	// the artists made some backwards ranges.  sigh.
		memcpy (dest + TOP_RANGE, source + top, 16);
	else
		for (j=0 ; j<16 ; j++)
			dest[TOP_RANGE+j] = source[top+15-j];

	if (bottom < 128)
		memcpy (dest + BOTTOM_RANGE, source + bottom, 16);
	else
		for (j=0 ; j<16 ; j++)
			dest[BOTTOM_RANGE+j] = source[bottom+15-j];
}


void M_DrawTransPicTranslate (int x, int y, qpic_t *pic)
{
	Draw_TransPicTranslate (x + ((vid.width - 320)>>1), y, pic, translationTable);
}


void M_DrawTextBox (int x, int y, int width, int lines)
{
	qpic_t	*p;
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	p = Draw_CachePic ("gfx/box_tl.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_ml.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_bl.lmp");
	M_DrawTransPic (cx, cy+8, p);

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		p = Draw_CachePic ("gfx/box_tm.lmp");
		M_DrawTransPic (cx, cy, p);
		p = Draw_CachePic ("gfx/box_mm.lmp");
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			if (n == 1)
				p = Draw_CachePic ("gfx/box_mm2.lmp");
			M_DrawTransPic (cx, cy, p);
		}
		p = Draw_CachePic ("gfx/box_bm.lmp");
		M_DrawTransPic (cx, cy+8, p);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = Draw_CachePic ("gfx/box_tr.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_mr.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_br.lmp");
	M_DrawTransPic (cx, cy+8, p);
}

void M_DrawCheckbox (int x, int y, int on)
{
#if 0
	if (on)
		M_DrawCharacter (x, y, 131);
	else
		M_DrawCharacter (x, y, 129);
#endif
	if (on)
		M_Print (x, y, "on");
	else
		M_Print (x, y, "off");
}

void M_DrawCheckboxNew (int x, int y, int on)
{
#if 0
	if (on)
		M_DrawCharacter (x, y, 131);
	else
		M_DrawCharacter (x, y, 129);
#endif
	if (on)
            M_DrawTransPic (x, y, Draw_CachePic ("gfx/on.lmp") );
	else
            M_DrawTransPic (x, y, Draw_CachePic ("gfx/off.lmp") );
}



//=============================================================================


int m_save_demonum;

/*
================
M_ToggleMenu_f
================
*/
void M_ToggleMenu_f (void)
{

	m_entersound = true;

	if (key_dest == key_menu)
	{
		if (m_state != m_main)
		{
			M_Menu_Main_f ();
			return;
		}
		key_dest = key_game;
		m_state = m_none;
		return;
	}
	if (key_dest == key_console)
	{
			M_Menu_Main_f ();
	}
	else
	{
			M_Menu_Main_f ();
	}
}


//=============================================================================
/* MAIN MENU */

int	m_main_cursor,playing;
#define	MAIN_ITEMS	2


void M_Menu_Main_f (void)
{
	if (key_dest != key_menu)
	{
		m_save_demonum = cls.demonum;
		cls.demonum = -1;
	}
	key_dest = key_menu;
	m_state = m_main;
	m_entersound = true;
                           
    if(!cl.worldmodel)
    {
    if(playing ==0)
     {
    Cbuf_InsertText ("play menu/music2.wav\n");  
    playing =1;
}}

}

void M_Main_Draw (void)
{
	int		f;
	qpic_t	*p;
           

    M_DrawTransPic (sp.value, 50, Draw_CachePic ("gfx/sp.lmp") );
    M_DrawTransPic (ex.value, 210, Draw_CachePic ("gfx/exit.lmp") );

if(m_main_cursor == 0)
{
sp.value = -30;
ex.value = -50;
}
else
{
sp.value = -50;
ex.value = -30;
}

}


void M_Main_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		key_dest = key_game;
		m_state = m_none;
		cls.demonum = m_save_demonum;
		if (cls.demonum != -1 && !cls.demoplayback && cls.state != ca_connected)
			CL_NextDemo ();
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_main_cursor >= MAIN_ITEMS)
			m_main_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;
		break;

	case K_ENTER:
		m_entersound = true;

		switch (m_main_cursor)
		{
		case 0:
            M_Menu_SinglePlayer_f ();
                 break;

		case 1:
                    M_Menu_Quit_f ();
                    break;

		}
	}
}

//=============================================================================
/* SINGLE PLAYER MENU */

int	m_singleplayer_cursor;
#define	SINGLEPLAYER_ITEMS	3


void M_Menu_SinglePlayer_f (void)
{
	key_dest = key_menu;
	m_state = m_singleplayer;
	m_entersound = true;
}


void M_SinglePlayer_Draw (void)
{
	int		f;
	qpic_t	*p;

    M_DrawTransPic (st.value, 50, Draw_CachePic ("gfx/start.lmp") );
    M_DrawTransPic (se.value, 130, Draw_CachePic ("gfx/save.lmp") );
    M_DrawTransPic (ld.value, 210, Draw_CachePic ("gfx/load.lmp") );

	M_DrawPic ( (320-p->width)/2, 4, p);
	M_DrawTransPic (72, 32, Draw_CachePic ("gfx/sp.lmp") );

if(m_singleplayer_cursor == 0)
{
st.value = -30;
se.value = -50;
ld.value = -50;
}
else if(m_singleplayer_cursor == 1)
{
st.value = -50;
se.value = -30;
ld.value = -50;
}
else if(m_singleplayer_cursor == 2)
{
st.value = -50;
se.value = -50;
ld.value = -30;
}

}


void M_SinglePlayer_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_singleplayer_cursor >= SINGLEPLAYER_ITEMS)
			m_singleplayer_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_singleplayer_cursor < 0)
			m_singleplayer_cursor = SINGLEPLAYER_ITEMS - 1;
		break;

	case K_ENTER:
		m_entersound = true;

		switch (m_singleplayer_cursor)
		{
		case 0:
			Cbuf_AddText ("disconnect\n");
			Cbuf_AddText ("maxplayers 1\n");
			Cbuf_AddText ("map 2050hometown\n");
			break;

		case 1:
			M_Menu_Load_f ();
			break;

		case 2:
			M_Menu_Save_f ();
			break;
		}
	}
}

//=============================================================================
/* Language MENU */

int	m_language_cursor;
#define	LANGUAGE_ITEMS	2


void M_Menu_Language_f (void)
{
	key_dest = key_menu;
	m_state = m_language;
	m_entersound = true;
}


void M_Language_Draw (void)
{
	int		f;
	qpic_t	*p;

    M_DrawTransPic (dt.value, 50, Draw_CachePic ("gfx/dt.lmp") );
    M_DrawTransPic (gba.value, 100, Draw_CachePic ("gfx/gba.lmp") );



	M_DrawPic ( (320-p->width)/2, 4, p);
	M_DrawTransPic (72, 32, Draw_CachePic ("gfx/language.lmp") );


if(m_language_cursor == 0)
{
dt.value = -30;
gba.value = -50;
}
else if(m_language_cursor == 1)
{
dt.value = -50;
gba.value = -30;
}


}


void M_Language_Key (int key)
{
	switch (key)
	{

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_language_cursor >= LANGUAGE_ITEMS)
			m_language_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_language_cursor < 0)
			m_language_cursor = LANGUAGE_ITEMS - 1;
		break;

	case K_ENTER:
		m_entersound = true;

		switch (m_language_cursor)
		{
		case 0:
            COM_LoadPackFile(va("%s/pakdt.pak", com_gamedir));
                  M_Menu_Main_f();
			break;

		case 1:
            COM_LoadPackFile(va("%s/pakeng.pak",com_gamedir));
                  M_Menu_Main_f();
			break;


		}
	}
}

//=============================================================================
/* LOAD/SAVE MENU */

int		load_cursor;		// 0 < load_cursor < MAX_SAVEGAMES

#define	MAX_SAVEGAMES		12
char	m_filenames[MAX_SAVEGAMES][SAVEGAME_COMMENT_LENGTH+1];
int		loadable[MAX_SAVEGAMES];

void M_ScanSaves (void)
{
	int		i, j;
	char	name[MAX_OSPATH];
	FILE	*f;
	int		version;

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
	{
		strcpy (m_filenames[i], "--- EMPTY SLOT ---");
		loadable[i] = false;
		sprintf (name, "%s/s%i.sav", com_gamedir, i);
		f = fopen (name, "r");
		if (!f)
			continue;
		fscanf (f, "%i\n", &version);
		fscanf (f, "%79s\n", name);
		strncpy (m_filenames[i], name, sizeof(m_filenames[i])-1);

	// change _ back to space
		for (j=0 ; j<SAVEGAME_COMMENT_LENGTH ; j++)
			if (m_filenames[i][j] == '_')
				m_filenames[i][j] = ' ';
		loadable[i] = true;
		fclose (f);
	}
}

void M_Menu_Load_f (void)
{
	m_entersound = true;
	m_state = m_load;
	key_dest = key_menu;
	M_ScanSaves ();
}


void M_Menu_Save_f (void)
{
	if (!sv.active)
		return;
	if (cl.intermission)
		return;
	if (svs.maxclients != 1)
		return;
	m_entersound = true;
	m_state = m_save;
	key_dest = key_menu;
	M_ScanSaves ();
}


void M_Load_Draw (void)
{
	int		i;
	qpic_t	*p, *b;


	    p = Draw_CachePic ("gfx/load.lmp");
	    M_DrawCharacter (8, 32 + load_cursor*8, 12+((int)(realtime*4)&1));
    
	M_DrawPic ( (320-p->width)/2, 4, p);

	for (i=0 ; i< MAX_SAVEGAMES; i++)
		M_Print (16, 32 + 8*i, m_filenames[i]);


}


void M_Save_Draw (void)
{
	int		i;
	qpic_t	*p, *b;

	    p = Draw_CachePic ("gfx/save.lmp");
	    M_DrawCharacter (8, 32 + load_cursor*8, 12+((int)(realtime*4)&1));
	M_DrawPic ( (320-p->width)/2, 4, p);

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
		M_Print (16, 32 + 8*i, m_filenames[i]);
}


void M_Load_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_SinglePlayer_f ();
		break;

	case K_ENTER:
		S_LocalSound ("misc/menu2.wav");
		if (!loadable[load_cursor])
			return;
		m_state = m_none;
		key_dest = key_game;

	// Host_Loadgame_f can't bring up the loading plaque because too much
	// stack space has been used, so do it now
		SCR_BeginLoadingPlaque ();


	// issue the load command
		Cbuf_AddText (va ("load s%i\n", load_cursor) );
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}


void M_Save_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_SinglePlayer_f ();
		break;

	case K_ENTER:
		m_state = m_none;
		key_dest = key_game;
		Cbuf_AddText (va("save s%i\n", load_cursor));
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}



//=============================================================================
/* OPTIONS MENU */
#define	SLIDER_RANGE	10
#define NUM_SUBMENU 2
#ifdef PSP_HARDWARE_VIDEO
enum 
{
	OPT_CUSTOMIZE = 0,
	OPT_CONSOLE,
	OPT_DEFAULTS,
      OPT_DEFAULTS2,
	OPT_SUBMENU,
	OPTIONS_ITEMS
};
enum 
{
	OPT_SUBMENU_0 = OPT_SUBMENU,
    //OPT_GAP_0,
	OPT_SCRSIZE,	
	OPT_GAMMA,			
    //OPT_GAP_0_1,
	OPT_MUSICTYPE,
	OPT_MUSICVOL,
	OPT_SNDVOL,		
	OPT_CROSSHAIR,
    //OPT_GAP_0_2,
	OPT_TEX_SCALEDOWN,
	OPT_SIMPLE_PART,
      OPT_MENU_MUSIC,
    OPTIONS_ITEMS_0
};
enum 
{
	OPT_SUBMENU_1 = OPT_SUBMENU,
    //OPT_GAP_1,
	OPT_ALWAYRUN,
	OPT_IN_SPEED,
	OPT_IN_TOLERANCE,
	OPT_IN_ACCELERATION,	
	OPT_INVMOUSE,	
	OPT_NOMOUSE,
	OPT_MOUSELOOK,
	OPT_MOUSESTAFE,	
	OPT_IN_X_ADJUST,
	OPT_IN_Y_ADJUST,
    OPTIONS_ITEMS_1
};
#else
enum 
{
	OPT_CUSTOMIZE = 0,
	OPT_CONSOLE,
	OPT_DEFAULTS,
	OPT_DEFAULTS2,
	OPT_SUBMENU,
	OPTIONS_ITEMS
};
enum 
{
	OPT_SUBMENU_0 = OPT_SUBMENU,
    //OPT_GAP_0,
	OPT_SCRSIZE,	
	OPT_GAMMA,			
    //OPT_GAP_0_1,
	OPT_MUSICTYPE,
	OPT_MUSICVOL,
	OPT_SNDVOL,		
	OPT_CROSSHAIR,	
    OPTIONS_ITEMS_0
};
enum 
{
	OPT_SUBMENU_1 = OPT_SUBMENU,
    //OPT_GAP_1,
	OPT_ALWAYRUN,
	OPT_IN_SPEED,
	OPT_IN_TOLERANCE,
	OPT_IN_ACCELERATION,	
	OPT_INVMOUSE,	
	OPT_NOMOUSE,
	OPT_MOUSELOOK,
	OPT_MOUSESTAFE,
	OPT_IN_X_ADJUST,
	OPT_IN_Y_ADJUST,	
    OPTIONS_ITEMS_1
};
#endif	

int	options_cursor;
int m_submenu = 0;

void M_Menu_Options_f (void)
{
	key_dest = key_menu;
	m_state = m_options;
	m_entersound = true;
}

void M_AdjustSliders (int dir)
{
	S_LocalSound ("misc/menu3.wav");

	switch (options_cursor)
	{
		case OPT_SUBMENU:
	        m_submenu += dir;
	        if (m_submenu > NUM_SUBMENU-1)
	        	m_submenu = 0;
	        else if (m_submenu < 0)
	        	m_submenu = NUM_SUBMENU-1;
	        break;	
	}
	
    if (m_submenu == 0)
    {
    	switch (options_cursor)
        {
			case OPT_SCRSIZE:	// screen size
				break;
			case OPT_GAMMA:	// gamma
				v_gamma.value -= dir * 0.05;
				if (v_gamma.value < 0.5)
					v_gamma.value = 0.5;
				if (v_gamma.value > 1)
					v_gamma.value = 1;
				Cvar_SetValue ("gamma", v_gamma.value);
				break;
			case OPT_MUSICVOL:	// music volume
#ifdef WIN32
				bgmvolume.value += dir * 1.0;
#else
				bgmvolume.value += dir * 0.1;
#endif
				if (bgmvolume.value < 0)
					bgmvolume.value = 0;
				if (bgmvolume.value > 1)
					bgmvolume.value = 1;
				Cvar_SetValue ("bgmvolume", bgmvolume.value);
#ifdef PSP_MP3_HWDECODE
				changeMp3Volume = 1;
#endif		
				break;
			case OPT_SNDVOL:	// sfx volume
				volume.value += dir * 0.1;
				if (volume.value < 0)
					volume.value = 0;
				if (volume.value > 1)
					volume.value = 1;
				Cvar_SetValue ("volume", volume.value);
				break;
			case OPT_MUSICTYPE: // bgm type
				if (strcmpi(bgmtype.string,"cd") == 0)
				{
						Cvar_Set("bgmtype","none");
						bmg_type_changed = true;
				}
				else
				{
						Cvar_Set("bgmtype","cd");
						bmg_type_changed = true;
				}
				break;
			case OPT_CROSSHAIR:	
				Cvar_SetValue ("crosshair", !crosshair.value);
				break;
			case OPT_TEX_SCALEDOWN:	
				Cvar_SetValue ("r_tex_scale_down", !r_tex_scale_down.value);
				break;
			case OPT_SIMPLE_PART:	
				Cvar_SetValue ("r_particles_simple", !r_particles_simple.value);
				break;
			case OPT_MENU_MUSIC:
				break;
//#endif	

        }
    }
    else if (m_submenu == 1)
    {
       	switch (options_cursor)
        {
			case OPT_ALWAYRUN:	// allways run
				if (cl_forwardspeed.value > 200)
				{
					Cvar_SetValue ("cl_forwardspeed", 200);
					Cvar_SetValue ("cl_backspeed", 200);
				}
				else
				{
					Cvar_SetValue ("cl_forwardspeed", 200);
					Cvar_SetValue ("cl_backspeed", 200);
				}
				break;

       		case OPT_IN_SPEED:	// mouse speed
				in_sensitivity.value += dir * 0.5;
				if (in_sensitivity.value < 1)
					in_sensitivity.value = 1;
				if (in_sensitivity.value > 11)
					in_sensitivity.value = 11;
				Cvar_SetValue ("sensitivity", in_sensitivity.value);
				break;
       		
       		case OPT_IN_TOLERANCE:	// mouse tolerance
				in_tolerance.value += dir * 0.05;
				if (in_tolerance.value < 0)
					in_tolerance.value = 0;
				if (in_tolerance.value > 1)
					in_tolerance.value = 1;
				Cvar_SetValue ("tolerance", in_tolerance.value);
				break;

       		case OPT_IN_ACCELERATION:	// mouse tolerance
				in_acceleration.value -= dir * 0.25;
				if (in_acceleration.value < 0.5)
					in_acceleration.value = 0.5;
				if (in_acceleration.value > 2)
					in_acceleration.value = 2;
				Cvar_SetValue ("acceleration", in_acceleration.value);
				break;
				
			case OPT_IN_X_ADJUST:	
				in_x_axis_adjust.value += dir*5;
				if (in_x_axis_adjust.value < -127)
					in_x_axis_adjust.value = -127;
				if (in_x_axis_adjust.value > 127)
					in_x_axis_adjust.value = 127;
				Cvar_SetValue ("in_x_axis_adjust", in_x_axis_adjust.value);
				break;
				
			case OPT_IN_Y_ADJUST:	
				in_y_axis_adjust.value += dir*5;
				if (in_y_axis_adjust.value < -127)
					in_y_axis_adjust.value = -127;
				if (in_y_axis_adjust.value > 127)
					in_y_axis_adjust.value = 127;
				Cvar_SetValue ("in_y_axis_adjust", in_y_axis_adjust.value);
				break;
								
			case OPT_INVMOUSE:	// invert mouse
				Cvar_SetValue ("m_pitch", -m_pitch.value);
				break;
			case OPT_NOMOUSE:	// disable mouse
				Cvar_SetValue ("in_disable_analog", !in_disable_analog.value);
				break;
			case OPT_MOUSESTAFE:
				Cvar_SetValue ("in_analog_strafe", !in_analog_strafe.value);
				break;
			case OPT_MOUSELOOK:
				if (in_mlook.state & 1)
					Cbuf_AddText("-mlook");
				else
					Cbuf_AddText("+mlook");
				break;
        }	
    }    
}



void M_DrawSlider (int x, int y, float range)
{
	int	i;

	if (range < 0)
		range = 0;
	if (range > 1)
		range = 1;
	M_DrawCharacter (x-8, y, 128);
	for (i=0 ; i<SLIDER_RANGE ; i++)
		M_DrawCharacter (x + i*8, y, 129);
	M_DrawCharacter (x+i*8, y, 130);
	M_DrawCharacter (x + (SLIDER_RANGE-1)*8 * range, y, 131);
}

void M_Options_Draw (void)
{
	float	 r;
	qpic_t	*p;
float d;


	M_Print (16, 32+(OPT_CUSTOMIZE*8), "    Customize controls");
	M_Print (16, 32+(OPT_CONSOLE*8),   "         Go to console");
	M_Print (16, 32+(OPT_DEFAULTS*8),  "   Default  Config 1");
	M_Print (16, 32+(OPT_DEFAULTS2*8),  "  Default   Config 2");

	switch (m_submenu) {
        case 0:    
		
			M_Print (16, 32+(OPT_GAMMA*8), 	   "            Brightness");
			r = (1.0 - v_gamma.value) / 0.5;
			M_DrawSlider (220, 32+(OPT_GAMMA*8), r);
		
			M_Print (16, 32+(OPT_MUSICVOL*8), "       CD Music Volume");
			r = bgmvolume.value;
			M_DrawSlider (220, 32+(OPT_MUSICVOL*8), r);
		
			M_Print (16, 32+(OPT_SNDVOL*8),   "          Sound Volume");
			r = volume.value;
			M_DrawSlider (220, 32+(OPT_SNDVOL*8), r);

			M_Print (16, 32+(OPT_MUSICTYPE*8),"            Music Type");
			if (strcmpi(bgmtype.string,"cd") == 0)
				M_Print (220, 32+(OPT_MUSICTYPE*8), "CD/MP3");
			else
				M_Print (220, 32+(OPT_MUSICTYPE*8), "None");
		
	
			M_Print (16, 32+(OPT_CROSSHAIR*8),	"        Show Crosshair");
			M_DrawCheckbox (220, 32+(OPT_CROSSHAIR*8), crosshair.value);

		
			M_Print (16, 32+(OPT_TEX_SCALEDOWN*8), "    Texture Scale Down");
			M_DrawCheckbox (220, 32+(OPT_TEX_SCALEDOWN*8), r_tex_scale_down.value);
		
			M_Print (16, 32+(OPT_SIMPLE_PART*8),    "      Simple Particles");
			M_DrawCheckbox (220, 32+(OPT_SIMPLE_PART*8), r_particles_simple.value);

			
			
		//#endif
            break;
        case 1:
        	M_Print (16, 32+(OPT_ALWAYRUN*8),        "            Always Run");
			M_DrawCheckbox (220, 32+(OPT_ALWAYRUN*8), cl_forwardspeed.value > 200);
		    
		#ifdef PSP
			M_Print (16, 32+(OPT_IN_SPEED*8), 		 "           A-Nub Speed");
		#else
			M_Print (16, 32+(OPT_IN_SPEED*8), 		 "           Mouse Speed");
		#endif
			r = (in_sensitivity.value - 1)/10;
			M_DrawSlider (220, 32+(OPT_IN_SPEED*8), r);

			M_Print (16, 32+(OPT_IN_ACCELERATION*8), "    A-Nub Acceleration");
			r = 1.0f -((in_acceleration.value - 0.5f)/1.5f);
			M_DrawSlider (220, 32+(OPT_IN_ACCELERATION*8), r);

			M_Print (16, 32+(OPT_IN_TOLERANCE*8), 	 "      A-Nub Tollerance");
			r = (in_tolerance.value )/1.0f;
			M_DrawSlider (220, 32+(OPT_IN_TOLERANCE*8), r);
		
			M_Print (16, 32+(OPT_IN_X_ADJUST*8), 	 "         Adjust Axis X");	
			r = (128+in_x_axis_adjust.value)/255;
			M_DrawSlider (220, 32+(OPT_IN_X_ADJUST*8), r);

			M_Print (16, 32+(OPT_IN_Y_ADJUST*8), 	 "         Adjust Axis Y");	
			r = (128+in_y_axis_adjust.value)/255;
			M_DrawSlider (220, 32+(OPT_IN_Y_ADJUST*8), r);
		
		
		#ifdef PSP
			M_Print (16, 32+(OPT_INVMOUSE*8),        "          Invert A-Nub");
		#else
			M_Print (16, 32+(OPT_INVMOUSE*8),        "          Invert Mouse");
		#endif
			M_DrawCheckbox (220, 32+(OPT_INVMOUSE*8), m_pitch.value < 0);
		
			M_Print (16, 32+(OPT_MOUSELOOK*8),       "            A-Nub Look");
			M_DrawCheckbox (220, 32+(OPT_MOUSELOOK*8), in_mlook.state & 1);

			M_Print (16, 32+(OPT_NOMOUSE*8),         "         Disable A-Nub");
			M_DrawCheckbox (220, 32+(OPT_NOMOUSE*8), in_disable_analog.value );
		
			M_Print (16, 32+(OPT_MOUSESTAFE*8),		 "         A-Nub Stafing");
			M_DrawCheckbox (220, 32+(OPT_MOUSESTAFE*8), in_analog_strafe.value );
			break;
	}
	
	M_PrintWhite (16, 32+(OPT_SUBMENU*8),	 "        Select Submenu");
    switch (m_submenu)
        {
        case 0:
            M_PrintWhite (220, 32+(OPT_SUBMENU*8), "More options");         
            break;
        case 1:
            M_PrintWhite (220, 32+(OPT_SUBMENU*8), "Less options");         
            break;
        default:
            break;
        }                         	
	// Cursor
	M_DrawCharacter (200, 32 + options_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Options_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;

	case K_ENTER:
		m_entersound = true;
		switch (options_cursor)
		{
		case OPT_CUSTOMIZE:
			M_Menu_Keys_f ();
			break;
		case OPT_CONSOLE:
			m_state = m_none;
			Con_ToggleConsole_f ();
			break;
		case OPT_DEFAULTS:
			Cbuf_AddText ("exec CONFIGs/1/auto.cfg\n");
			break;
		case OPT_DEFAULTS2:
			Cbuf_AddText ("exec CONFIGs/2/auto.cfg\n");
			break;
		default:
			M_AdjustSliders (1);
			break;
		}
		return;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		options_cursor--;
		if (options_cursor < 0) {
			if (m_submenu == 0)
			    options_cursor = OPTIONS_ITEMS_0-1;
	        if (m_submenu == 1)
			    options_cursor = OPTIONS_ITEMS_1-1;
		}
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		options_cursor++;
        if (m_submenu == 0)
            if (options_cursor >= OPTIONS_ITEMS_0)
			    options_cursor = 0;

        if (m_submenu == 1)
            if (options_cursor >= OPTIONS_ITEMS_1)
			    options_cursor = 0;
		break;

	case K_LEFTARROW:
		M_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:
		M_AdjustSliders (1);
		break;
	}
}

//=============================================================================
/* KEYS MENU */

char *bindnames[][2] =
{
{"+attack", 		"attack"},
{"impulse 121", 		"Change weapon"},
{"+jump", 			"Jump / Swim up"},
{"+forward", 		"Walk forward"},
{"+back", 			"Walk backward"},
{"+left", 			"Turn left"},
{"+right", 			"Turn right"},
{"+moveleft", 		"Step left"},
{"+moveright", 		"Step right"},
{"+strafe", 		"Sidestep"},
{"+lookup", 		"Look up"},
{"+lookdown", 		"Look down"},
{"centerview", 		"Center view"},
#ifdef PSP
{"+mlook", 			"Analog nub look"},
#else
{"+mlook", 			"Mouse look"},
{"+klook", 			"Keyboard look"},
#endif
{"+moveup",			"Swim up"},
{"+movedown",		"Swim down"},
{"+turnleft", 		"cam Left"},
{"+turnright", 		"cam right"},




};

#define	NUMCOMMANDS	(sizeof(bindnames)/sizeof(bindnames[0]))

int		keys_cursor;
int		bind_grab;

void M_Menu_Keys_f (void)
{
	key_dest = key_menu;
	m_state = m_keys;
	m_entersound = true;
}


void M_FindKeysForCommand (char *command, int *twokeys)
{
	int		count;
	int		j;
	int		l;
	char	*b;

	twokeys[0] = twokeys[1] = -1;
	l = strlen(command);
	count = 0;

	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp (b, command, l) )
		{
			twokeys[count] = j;
			count++;
			if (count == 2)
				break;
		}
	}
}

void M_UnbindCommand (char *command)
{
	int		j;
	int		l;
	char	*b;

	l = strlen(command);

	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp (b, command, l) )
			Key_SetBinding (j, "");
	}
}


void M_Keys_Draw (void)
{
	int		i, l;
	int		keys[2];
	char	*name;
	int		x, y;
	qpic_t	*p;



#ifdef PSP
	if (bind_grab)
		M_Print (12, 32, "Press a button for this action");
	else
		M_Print (18, 32, "Press CROSS to change, SQUARE to clear");
#else
	if (bind_grab)
		M_Print (12, 32, "Press a key or button for this action");
	else
		M_Print (18, 32, "Enter to change, backspace to clear");
#endif

// search for known bindings
	for (i=0 ; i<NUMCOMMANDS ; i++)
	{
		y = 48 + 8*i;

		M_Print (16, y, bindnames[i][1]);

		l = strlen (bindnames[i][0]);

		M_FindKeysForCommand (bindnames[i][0], keys);

		if (keys[0] == -1)
		{
			M_Print (140, y, "???");
		}
		else
		{
			name = Key_KeynumToString (keys[0]);
			M_Print (140, y, name);
			x = strlen(name) * 8;
			if (keys[1] != -1)
			{
				M_Print (140 + x + 8, y, "or");
				M_Print (140 + x + 32, y, Key_KeynumToString (keys[1]));
			}
		}
	}

	if (bind_grab)
		M_DrawCharacter (130, 48 + keys_cursor*8, '=');
	else
		M_DrawCharacter (130, 48 + keys_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Keys_Key (int k)
{
	char	cmd[80];
	int		keys[2];

	if (bind_grab)
	{	// defining a key
		S_LocalSound ("misc/menu1.wav");
		if (k == K_ESCAPE)
		{
			bind_grab = false;
		}
		else if (k != '`')
		{
			sprintf (cmd, "bind \"%s\" \"%s\"\n", Key_KeynumToString (k), bindnames[keys_cursor][0]);
			Cbuf_InsertText (cmd);
		}

		bind_grab = false;
		return;
	}

	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Options_f ();
		break;

	case K_LEFTARROW:
	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		keys_cursor--;
		if (keys_cursor < 0)
			keys_cursor = NUMCOMMANDS-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		keys_cursor++;
		if (keys_cursor >= NUMCOMMANDS)
			keys_cursor = 0;
		break;

	case K_ENTER:		// go into bind mode
		M_FindKeysForCommand (bindnames[keys_cursor][0], keys);
		S_LocalSound ("misc/menu2.wav");
		if (keys[1] != -1)
			M_UnbindCommand (bindnames[keys_cursor][0]);
		bind_grab = true;
		break;

	case K_BACKSPACE:		// delete bindings
	case K_DEL:				// delete bindings
		S_LocalSound ("misc/menu2.wav");
		M_UnbindCommand (bindnames[keys_cursor][0]);
		break;
	}
}

//=============================================================================
/* QUIT MENU */

int		msgNumber;
int		m_quit_prevstate;
qboolean	wasInMenus;


void M_Menu_Quit_f (void)
{
	if (m_state == m_quit)
		return;
	wasInMenus = (key_dest == key_menu);
	key_dest = key_menu;
	m_quit_prevstate = m_state;
	m_state = m_quit;
	m_entersound = true;
	msgNumber = rand()&7;
}


void M_Quit_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case 'n':
	case 'N':
		if (wasInMenus)
		{
			m_state = m_quit_prevstate;
			m_entersound = true;
		}
		else
		{
			key_dest = key_game;
			m_state = m_none;
		}
		break;

	case 'Y':
	case 'y':
#ifdef PSP
	case K_ENTER:
#endif
		key_dest = key_console;
		Host_Quit_f ();
		break;

	default:
		break;
	}

}


void M_Quit_Draw (void)
{
	if (wasInMenus)
	{
		m_state = m_quit_prevstate;
		m_recursiveDraw = true;
		M_Draw ();
		m_state = m_quit;
	}

	M_Print (64, 100,	"  Press CROSS to quit,  ");
	M_Print (64, 108,	" or CIRCLE to continue. ");

}

//=============================================================================
/* OSK IMPLEMENTATION */
#define CHAR_SIZE 8
#define MAX_Y 8
#define MAX_X 12

#define MAX_CHAR_LINE 36
#define MAX_CHAR      72

int  osk_pos_x = 0;
int  osk_pos_y = 0;
int  max_len   = 0;
int  m_old_state = 0;

char* osk_out_buff = NULL;
char  osk_buffer[128];

char *osk_text [] = 
	{ 
		" 1 2 3 4 5 6 7 8 9 0 - = ` ",
		" q w e r t y u i o p [ ]   ",
		"   a s d f g h j k l ; ' \\ ",
		"     z x c v b n m   , . / ",
		"                           ",
		" ! @ # $ % ^ & * ( ) _ + ~ ",
		" Q W E R T Y U I O P { }   ",
		"   A S D F G H J K L : \" | ",
		"     Z X C V B N M   < > ? "
	};

char *osk_help [] = 
	{ 
		"CONFIRM: ",
		" SQUARE  ",
		"CANCEL:  ",
		" CIRCLE  ",
		"DELETE:  ",
		" TRIAGLE ",
		"ADD CHAR:",
		" CROSS   ",
		""
	};

void M_Menu_OSK_f (char *input, char *output, int outlen)
{
	key_dest = key_menu;
	m_old_state = m_state;
	m_state = m_osk;
	m_entersound = false;
	max_len = outlen;
	strncpy(osk_buffer,input,max_len);
	osk_buffer[outlen] = '\0';
	osk_out_buff = output; 
}

void Con_OSK_f (char *input, char *output, int outlen)
{
	max_len = outlen;
	strncpy(osk_buffer,input,max_len);
	osk_buffer[outlen] = '\0';
	osk_out_buff = output; 
}


void M_OSK_Draw (void)
{
#ifdef PSP
	int x,y;
	int i;
	
	char *selected_line = osk_text[osk_pos_y]; 
	char selected_char[2];
	
	selected_char[0] = selected_line[1+(2*osk_pos_x)];
	selected_char[1] = '\0';
	if (selected_char[0] == ' ' || selected_char[0] == '\t') 
		selected_char[0] = 'X';
		
	y = 20;
	x = 16;

	M_DrawTextBox (10, 10, 		     26, 10);
	M_DrawTextBox (10+(26*CHAR_SIZE),    10,  10, 10);
	M_DrawTextBox (10, 10+(10*CHAR_SIZE),36,  3);
	
	for(i=0;i<=MAX_Y;i++) 
	{
		M_PrintWhite (x, y+(CHAR_SIZE*i), osk_text[i]);
		if (i % 2 == 0)
			M_Print      (x+(27*CHAR_SIZE), y+(CHAR_SIZE*i), osk_help[i]);
		else			
			M_PrintWhite (x+(27*CHAR_SIZE), y+(CHAR_SIZE*i), osk_help[i]);
	}
	
	int text_len = strlen(osk_buffer);
	if (text_len > MAX_CHAR_LINE) {
		
		char oneline[MAX_CHAR_LINE+1];
		strncpy(oneline,osk_buffer,MAX_CHAR_LINE);
		oneline[MAX_CHAR_LINE] = '\0';
		
		M_Print (x+4, y+4+(CHAR_SIZE*(MAX_Y+2)), oneline );
		
		strncpy(oneline,osk_buffer+MAX_CHAR_LINE, text_len - MAX_CHAR_LINE);
		oneline[text_len - MAX_CHAR_LINE] = '\0';
		
		M_Print (x+4, y+4+(CHAR_SIZE*(MAX_Y+3)), oneline );
		M_PrintWhite (x+4+(CHAR_SIZE*(text_len - MAX_CHAR_LINE)), y+4+(CHAR_SIZE*(MAX_Y+3)),"_");
	}
	else {
		M_Print (x+4, y+4+(CHAR_SIZE*(MAX_Y+2)), osk_buffer );
		M_PrintWhite (x+4+(CHAR_SIZE*(text_len)), y+4+(CHAR_SIZE*(MAX_Y+2)),"_");
	}
	M_Print      (x+((((osk_pos_x)*2)+1)*CHAR_SIZE), y+(osk_pos_y*CHAR_SIZE), selected_char);

#endif
}

void M_OSK_Key (int key)
{
#ifdef PSP
	switch (key)
	{
	case K_RIGHTARROW:
		osk_pos_x++;
		if (osk_pos_x > MAX_X)
			osk_pos_x = MAX_X;
		break;
	case K_LEFTARROW:
		osk_pos_x--;
		if (osk_pos_x < 0)
			osk_pos_x = 0;
		break;
	case K_DOWNARROW:
		osk_pos_y++;
		if (osk_pos_y > MAX_Y)
			osk_pos_y = MAX_Y;
		break;
	case K_UPARROW:
		osk_pos_y--;
		if (osk_pos_y < 0)
			osk_pos_y = 0;
		break;
	case K_ENTER: 
		if (max_len > strlen(osk_buffer)) {
			char *selected_line = osk_text[osk_pos_y]; 
			char selected_char[2];
			
			selected_char[0] = selected_line[1+(2*osk_pos_x)];
			
			if (selected_char[0] == '\t')
				selected_char[0] = ' ';
			
			selected_char[1] = '\0';
			strcat(osk_buffer,selected_char);		
		}
		break;
	case K_DEL:
		if (strlen(osk_buffer) > 0) {
			osk_buffer[strlen(osk_buffer)-1] = '\0';	
		}
		break;
	case K_INS:
		strncpy(osk_out_buff,osk_buffer,max_len);
		
		m_state = m_old_state;
		break;
	case K_ESCAPE:
		m_state = m_old_state;
		break;
	default:
		break;
	}
#endif		
}

void Con_OSK_Key (int key)
{
#ifdef PSP
	switch (key)
	{
	case K_RIGHTARROW:
		osk_pos_x++;
		if (osk_pos_x > MAX_X)
			osk_pos_x = MAX_X;
		break;
	case K_LEFTARROW:
		osk_pos_x--;
		if (osk_pos_x < 0)
			osk_pos_x = 0;
		break;
	case K_DOWNARROW:
		osk_pos_y++;
		if (osk_pos_y > MAX_Y)
			osk_pos_y = MAX_Y;
		break;
	case K_UPARROW:
		osk_pos_y--;
		if (osk_pos_y < 0)
			osk_pos_y = 0;
		break;
	case K_ENTER: 
		if (max_len > strlen(osk_buffer)) {
			char *selected_line = osk_text[osk_pos_y]; 
			char selected_char[2];
			
			selected_char[0] = selected_line[1+(2*osk_pos_x)];
			
			if (selected_char[0] == '\t')
				selected_char[0] = ' ';
			
			selected_char[1] = '\0';
			strcat(osk_buffer,selected_char);		
		}
		break;
	case K_DEL:
		if (strlen(osk_buffer) > 0) {
			osk_buffer[strlen(osk_buffer)-1] = '\0';	
		}
		break;
	case K_INS:
		strncpy(osk_out_buff,osk_buffer,max_len);
		Con_SetOSKActive(false);
		break;
	case K_ESCAPE:
		Con_SetOSKActive(false);
		break;
	default:
		break;
	}
#endif		
}	


//=============================================================================
/* Menu Subsystem */


void M_Init (void)
{
	Cmd_AddCommand ("togglemenu", M_ToggleMenu_f);

	Cmd_AddCommand ("menu_main", M_Menu_Main_f);
	Cmd_AddCommand ("menu_singleplayer", M_Menu_SinglePlayer_f);
	Cmd_AddCommand ("menu_language", M_Menu_Language_f);
	Cmd_AddCommand ("menu_quit", M_Menu_Quit_f);
}


void M_Draw (void)
{
	if (m_state == m_none || key_dest != key_menu)
		return;

	if (!m_recursiveDraw)
	{
		scr_copyeverything = 1;

		if (scr_con_current)
		{
			Draw_ConsoleBackground (vid.height);
			VID_UnlockBuffer ();
			S_ExtraUpdate ();
			VID_LockBuffer ();
		}
		else

		scr_fullupdate = 0;
	}
	else
	{
		m_recursiveDraw = false;
	}

	switch (m_state)
	{
	case m_none:
		break;

	case m_main:
		M_Main_Draw ();
		break;

	case m_singleplayer:
		M_SinglePlayer_Draw ();
		break;

	case m_load:
		M_Load_Draw ();
		break;

	case m_save:
		M_Save_Draw ();
		break;

	case m_language:
		M_Language_Draw ();
		break;


	case m_options:
		M_Options_Draw ();
		break;

	case m_keys:
		M_Keys_Draw ();
		break;


	case m_quit:
		M_Quit_Draw ();
		break;


	case m_osk:
		M_OSK_Draw();
		break;


	}

	if (m_entersound)
	{
		S_LocalSound ("misc/menu2.wav");
		m_entersound = false;
	}

	VID_UnlockBuffer ();
	S_ExtraUpdate ();
	VID_LockBuffer ();
}


void M_Keydown (int key)
{
	switch (m_state)
	{
	case m_none:
		return;

	case m_main:
		M_Main_Key (key);
		return;

	case m_singleplayer:
		M_SinglePlayer_Key (key);
		return;

	case m_load:
		M_Load_Key (key);
		return;

	case m_save:
		M_Save_Key (key);
		return;

	case m_language:
		M_Language_Key (key);
		return;

	case m_options:
		M_Options_Key (key);
		return;

	case m_keys:
		M_Keys_Key (key);
		return;


	case m_quit:
		M_Quit_Key (key);
		return;

	case m_osk:
		M_OSK_Key(key);	

	}
}



