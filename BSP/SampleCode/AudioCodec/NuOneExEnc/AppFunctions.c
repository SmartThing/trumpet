/*---------------------------------------------------------------------------------------------------------*/
/*																										   */
/* Copyright (c) Nuvoton Technology	Corp. All rights reserved.											   */
/*																										   */
/*---------------------------------------------------------------------------------------------------------*/

// ---------------------------------------------------------------------------------------------------------
//	Functions:
//		- Functions to handle main operations:
//			* Initiate application.
//			* Start audio playback.
//			* Stop  audio playback.
//			* Produce PCM data for audio playback.
//			* Start audio recording.
//			* Stop  audio recording.
//			* Use recorded data to do:
//				a. encoding
//				b. doing voice effect
//				c. write to storage
//				d. etc.
//		- The above functions use codes in "xxxApp" folder and "Utility" folder to complete necessary operations.
//
//	Reference "Readme.txt" for more information.
// ---------------------------------------------------------------------------------------------------------

#include "App.h"
#include "MicSpk.h"
#include "SPIFlashUtil.h"

extern UINT8 SPIFlash_Initiate(void);

extern void PowerDown_Enter(void);

extern void PowerDown(void);

extern void PowerDown_Exit(void);

extern S_APP g_sApp;

extern volatile UINT8 g_u8AppCtrl;

extern UINT32 g_u32SPIFlashRecAddr;	// Current recoding address in SPIFlash.

extern S_SPIFLASH_HANDLER g_sSpiFlash; 

//---------------------------------------------------------------------------------------------------------
// Function: App_Initiate
//
// Description:
//	Initiate application.
//
// Argument:
//
// Return:
//
//---------------------------------------------------------------------------------------------------------
void App_Initiate(void)
{
	g_u8AppCtrl = APPCTRL_NO_ACTION;
	
	// Initiate the audio playback.
	Playback_Initiate();
	
	// Configure the UltraIO curve output funciton's interrupt priority.
	// Lower this priority if other interrupt services should be higher than UltraIO curve output function!
	#if ( ULTRAIO_FW_CURVE_ENABLE )
	NVIC_SetPriority(ULTRAIO_TMR_IRQ, 1);
	#endif	
	
	// Light stand by(PB8) led for initial ready().
	OUT3(0);
}

//---------------------------------------------------------------------------------------------------------
// Function: App_StartPlay
//
// Description:                                                                                           
//	Start audio playback.
//
// Return:
// 	FALSE: fail
//	TRUE:  success
//---------------------------------------------------------------------------------------------------------
BOOL App_StartPlay(void)
{
	// Initiate NuOneEx audio decode lib with callback functions stored in g_asAppCallBack[0].
	NuOneExApp_DecodeInitiate(&g_sApp.sNuOneExAppDecode, (UINT8 *)&g_sApp.uTempBuf, 0);
	
	// Start NuOneEx decode lib to decode NuOneEx file stored from "sAudioChunkInfo.u32AudioChunkAddr"
	// And decode the first frame of PCMs.
	if ( NuOneExApp_DecodeStartPlayByAddr(&g_sApp.sNuOneExAppDecode, AUDIOROM_STORAGE_START_ADDR, 0) == 0)
		return FALSE;
	
	// Light playback led(PB9) for display status.
	OUT4(0);
	
	// Start Ultraio Timer & HW pwm for UltraIO curve output
	ULTRAIO_START();
	
	// Start to playback audio. 
	Playback_StartPlay();

	return TRUE;
}

//---------------------------------------------------------------------------------------------------------
// Description:                                                                                            
//	Stop audio playback.                                                                             
//
// Return:
// 	FALSE: fail
//	TRUE:  success
//---------------------------------------------------------------------------------------------------------
BOOL App_StopPlay(void)
{
	// Stop speaker.
	Playback_StopPlay();
	
	// Stop Ultraio Timer & HW pwm.
	ULTRAIO_STOP();

	OUT4(1);
	
	return TRUE;
}

//---------------------------------------------------------------------------------------------------------
// Function: App_StartRec
//
// Description:                                                                                           
//	Record PCM data from MIC.
//
// Return:
// 	FALSE: fail
//	TRUE:  success
//---------------------------------------------------------------------------------------------------------
BOOL App_StartRec(void)
{
	S_AUDIOCHUNK_HEADER sAudioChunkHeader;

	// Initiate NuOneEx audio encode lib with temp buffer provided for lib.
	NuOneExApp_EncodeInitiate(&g_sApp.sNuOneExAppEncode, (UINT8 *)&g_sApp.uTempBuf);
	
	// Start to encode NuOneEx data with sample rate, bit per frame and saved data information into audio chunk header.
	if (NuOneExApp_EncodeStart(&g_sApp.sNuOneExAppEncode, &sAudioChunkHeader, ADC_SAMPLE_RATE, E_NUONEEX_ENCODE_BPS_10) == FALSE)
		return FALSE;
	
	// SPIFlash utility function provide encode data write into SPIFlash.
	// detail info please refer "SPIFlashUtil.h"
	SPIFlahUtil_StartWriteEncodeData(&sAudioChunkHeader, AUDIOROM_STORAGE_START_ADDR, NULL);
	
	// Light record led for display status.
	OUT5(0);
	
	// Start to record PCM data into buffer for produc NuOneEx encode data.
	Record_StartRec();
	
	return TRUE;
}

//---------------------------------------------------------------------------------------------------------
// Description:                                                                                            
//	Stop to record PCM data.                                                                             
//
// Return:
// 	FALSE: fail
//	TRUE:  success
//---------------------------------------------------------------------------------------------------------
void App_StopRec(void)
{
	// Stop mic.
	Record_StopRec();
	
	// Stop to NuOneEx encode process.
	NuOneExApp_EncodeEnd(&g_sApp.sNuOneExAppEncode);
	
	// Write audio chunk header(encode data length) into SPIFlash.
	SPIFlashUtil_EndWriteEncodeData();

	OUT5(1);			
}

//---------------------------------------------------------------------------------------------------------
// Function: App_ProcessPlay
//
// Description:                                                                                            
//   Produce PCM data for audio playback
//
// Return:
//	FALSE: No PCM produced for audio playback
//	TRUE:  Have PCMs produced for audio playback                                      
//---------------------------------------------------------------------------------------------------------
BOOL App_ProcessPlay(void)
{
	UINT8 u8ActiveProcessCount = 0;
	
	// Continue decode NuOneEx data to produce PCMs for audio playback.
	if ( NuOneExApp_DecodeProcess(&g_sApp.sNuOneExAppDecode) == TRUE )
		u8ActiveProcessCount ++;

	if ( u8ActiveProcessCount )
		return TRUE;

	return FALSE;
}

//---------------------------------------------------------------------------------------------------------
// Function: App_ProcessRec
//
// Description:                                                                                            
//   Record PCM data for providing Beat-detect.
//
// Return:
//	FALSE: No PCM produced for audio playback
//	TRUE:  Have PCMs produced for audio playback                                      
//---------------------------------------------------------------------------------------------------------
BOOL App_ProcessRec(void)
{
	// Write NuOneEx encode data into SPIFlash.
	SPIFlashUtil_WriteEncodeData(&g_sApp.sNuOneExAppEncode.sEncodeBufCtrl);

	// Check current record address is out size of SPIFlash.
	if (g_u32SPIFlashRecAddr >= g_sSpiFlash.u32FlashSize)
		return FALSE;	

	// Keep encode PCM buffer data to NuOneEx lib.
	NuOneExApp_EncodeProcess(&g_sApp.sNuOneExAppEncode);
	
	return TRUE;
}

//---------------------------------------------------------------------------------------------------------
// Function: App_PowerDown
//
// Description:                                                                                            
//   Process flow of power-down for application. Include,
//   1. App_PowerDownProcess:Pre-process befor entering power-down.
//   2. PowerDown:Power down base process(PowerDown.c).
//   3. App_WakeUpProcess:Post-process after exiting power-down.
//   User could disable or enable this flow from flag(POWERDOWN_ENABLE) in ConfigApp.h.
//---------------------------------------------------------------------------------------------------------
void App_PowerDown(void)
{
	App_StopPlay();
	App_StopRec();
	
	#if(POWERDOWN_ENABLE)
	PowerDown_Enter();
	PowerDown();
	PowerDown_Exit();
	#endif
}
