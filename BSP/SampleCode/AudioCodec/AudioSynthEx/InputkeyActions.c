/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) Nuvoton Technology Corp. All rights reserved.                                              */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/	 

// ---------------------------------------------------------------------------------------------------------
//	Functions:
//		- Direct trigger key and matrix key falling, rising, long pressing handling functions.
//			* Direct trigger key handling functions are referenced in a lookup table "g_asTriggerKeyHandler" (in "IO\ConfigIO.h").
//			* Matrix key handling functions are reference in a lookup talbe "g_asMatrixKeyHandler" (in "IO\ConfigIO.h").
//			* Both lookup tables are used by keypad library to call at key action being identified.
//		- Default trigger key and matrix key handler is "Default_KeyHandler()"
//
//	Reference "Readme.txt" for more information.
// ---------------------------------------------------------------------------------------------------------

#include "App.h"
#include "Keypad.h"

extern S_APP g_sApp;
extern volatile UINT8 g_u8AppCtrl;
extern S_BUF_CTRL *g_sAudioSynthExAppInBufCtrlList[];

extern BOOL App_StartPlay(void);
extern void App_PowerDown(void);

void Playback_KeypadHandler(UINT32 u32Param)
{		
	if ( (g_u8AppCtrl&APPCTRL_PLAY) == 0 )
	{
		App_StartPlay();
	}
	else
	{
		Playback_StopPlay();			
	}
}

void PlayNext_KeypadHandler(UINT32 u32Param)
{
	if ( g_u8AppCtrl&APPCTRL_PLAY )	 
	{
		Playback_StopPlay();			
	}

	if (g_u8AppCtrl&APPCTRL_MODE_EQUATION )
	{
		if ( g_sApp.g_u16TotalEquationNum == 0 )
			return;
		g_sApp.g_u8PlayID = (g_sApp.g_u8PlayID+1)% g_sApp.g_u16TotalEquationNum;
	}
	else
	{
		if ( g_sApp.g_u16TotalAudioNum == 0 )
			return;
		g_sApp.g_u8PlayID = (g_sApp.g_u8PlayID+1)% g_sApp.g_u16TotalAudioNum;
	}

	App_StartPlay();
}

void PlayPrev_KeypadHandler(UINT32 u32Param)
{ 
	if ( g_u8AppCtrl&APPCTRL_PLAY )	 
	{
		Playback_StopPlay();			
	}

	if ( g_sApp.g_u8PlayID == 0 )
	{	
		if (g_u8AppCtrl&APPCTRL_MODE_EQUATION )
		{
			if ( g_sApp.g_u16TotalEquationNum == 0 )
				return;
			g_sApp.g_u8PlayID = g_sApp.g_u16TotalEquationNum-1;
		}
		else
		{
			if ( g_sApp.g_u16TotalAudioNum == 0 )
				return;
			g_sApp.g_u8PlayID = g_sApp.g_u16TotalAudioNum-1;
		}
	}
	else
		g_sApp.g_u8PlayID --;
	 
	App_StartPlay();
}

void PlayMode_KeypadHandler(UINT32 u32Param)
{ 
	if ( g_u8AppCtrl&APPCTRL_MODE_EQUATION )
	{
		g_u8AppCtrl &= ~APPCTRL_MODE_EQUATION;
		OUT4(1);
	}
	else
	{
		g_u8AppCtrl |= APPCTRL_MODE_EQUATION;
		OUT4(0);
	}
	g_sApp.g_u8PlayID = 0;
}

void PowerDown_KeypadHandler(UINT32 u32Param)
{
	App_PowerDown();
}

void PlayPause_KeypadHandler(UINT32 u32Param)
{
	if ((g_u8AppCtrl&APPCTRL_PAUSE) == 0)
	{
		Playback_PauseCtrl(0, TRUE);
		g_u8AppCtrl |= APPCTRL_PAUSE;
	}
	else
	{
		Playback_PauseCtrl(0, FALSE);
		g_u8AppCtrl &= ~APPCTRL_PAUSE;
	}
}

void PlayMute_KeypadHandler(UINT32 u32Param)
{
	if ((g_u8AppCtrl&APPCTRL_MUTE) == 0)
	{
		Playback_MuteCtrl(0, TRUE);
		g_u8AppCtrl |= APPCTRL_MUTE;
	}
	else
	{
		Playback_MuteCtrl(0, FALSE);
		g_u8AppCtrl &= ~APPCTRL_MUTE;
	}
}

void Default_KeyHandler(UINT32 u32Param)
{
	// Within this function, key state can be checked with these keywords:
	//	TGnF: direct trigger key n is in falling state(pressing)
	//	TGnR: direct trigger key n is in rising state(releasing)
	//	TGnP: direct trigger key n is in long pressing state
	//	KEYmF: matrix key m is in falling state(pressing)
	//	KEYmR: matrix key m is in rising state(releasing)
	//	KEYmP: matrix key m is in long pressing state
	// the maxium value of n is defined by "TRIGGER_KEY_COUNT" in "IO/ConfigIO.h"
	// the maxium value of m is defined by "MATRIX_KEY_COUNT"  in "IO/ConfigIO.h"
	switch(u32Param)
	{
		case TG1F:
			Playback_KeypadHandler(0);
			break;
		case TG2F:
			PlayNext_KeypadHandler(0);
			break;
		case TG3F:
			PlayPrev_KeypadHandler(0);
			break;	
		case TG4P:
			PowerDown_KeypadHandler(0);
			break;	
		case TG5F:
			PlayMode_KeypadHandler(0);
			break;	
		case TG6F:
			PlayPause_KeypadHandler(0);
			break;
	}
}
