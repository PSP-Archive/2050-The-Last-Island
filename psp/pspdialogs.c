#include "pspdialogs.h"

/* pspdlg list */
unsigned int __attribute__((aligned(16))) mylist[262144];

void pspShowErrorDialog(const int error, u8 language)
{
	pspUtilityMsgDialogParams dialog;

	SceSize dialog_size = sizeof(dialog);

	memset(&dialog, 0, dialog_size);

	dialog.base.size = dialog_size;
    
	if(language == DIALOG_LANGUAGE_AUTO)
		sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE,&dialog.base.language);
	else
		dialog.base.language = language;

	sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN, &dialog.base.buttonSwap);

	dialog.base.graphicsThread = 0x11;
	dialog.base.accessThread = 0x13;
	dialog.base.fontThread = 0x12;
	dialog.base.soundThread = 0x10;

	dialog.errorValue = 0;
	dialog.errorValue = error;

	sceUtilityMsgDialogInitStart(&dialog);

	for(;;)
	{
		minimalRender();

		switch(sceUtilityMsgDialogGetStatus())
		{    
			case PSP_UTILITY_DIALOG_VISIBLE:
				sceUtilityMsgDialogUpdate(2);
				break;
	    
			case PSP_UTILITY_DIALOG_QUIT:
				sceUtilityMsgDialogShutdownStart();
				break;
	    
			case PSP_UTILITY_DIALOG_NONE:
				return;    
		}
		sceDisplayWaitVblankStart();
		sceGuSwapBuffers();
	}
}

void pspShowMessageDialog(char *message, u8 language)
{
    pspUtilityMsgDialogParams dialog;
    SceSize dialog_size = sizeof(dialog);

    memset(&dialog, 0, dialog_size);

    dialog.base.size = dialog_size;
    
    if(language == DIALOG_LANGUAGE_AUTO)
 		sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE,&dialog.base.language);
    else
		dialog.base.language = language;

    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN, &dialog.base.buttonSwap);

    dialog.base.graphicsThread = 0x11;
    dialog.base.accessThread = 0x13;
    dialog.base.fontThread = 0x12;
    dialog.base.soundThread = 0x10;


   dialog.mode = PSP_UTILITY_MSGDIALOG_MODE_TEXT;

    strcpy(dialog.message, message);

    sceUtilityMsgDialogInitStart(&dialog);


    for(;;) 
    {
		minimalRender();

		switch(sceUtilityMsgDialogGetStatus()) 
		{

			case 2:
				sceUtilityMsgDialogUpdate(2);
				break;
	    
			case 3:
				sceUtilityMsgDialogShutdownStart();
				break;
	    
			case 0:
			return;
	    
		}
		sceDisplayWaitVblankStart();	
		sceGuSwapBuffers();
	}
}

void pspShowOSK(OSK_HELPER *oskhelper, u8 language)
{
	int done=0;
	unsigned short buffer[128];

	SceUtilityOskData data;
	memset(&data, 0, sizeof(data));
	data.language = 2;         // key glyphs: 0-1=hiragana, 2+=western
	data.lines = oskhelper->lines;   
	data.unk_24 = 1;         // set to 1
	data.desc = oskhelper->title;
	data.intext = oskhelper->pretext;
	data.outtextlength = oskhelper->textlength;
	data.outtextlimit = oskhelper->textlimit;
	data.outtext = (unsigned short*)buffer;

	SceUtilityOskParams osk;
	memset(&osk, 0, sizeof(osk));
	osk.base.size = sizeof(osk);

    if(language == DIALOG_LANGUAGE_AUTO)
 		sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE,&osk.base.language);
    else
		osk.base.language = language;   

    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN, &osk.base.buttonSwap);
	osk.base.graphicsThread = 0x11;
	osk.base.accessThread = 0x13;
	osk.base.fontThread = 0x12;
	osk.base.soundThread = 0x10;
	osk.unk_48 = 1;
	osk.data = &data;

	sceUtilityOskInitStart(&osk);

	while(!done)
	{

		minimalRender();

		switch(sceUtilityOskGetStatus())
		{
			case PSP_UTILITY_DIALOG_INIT :
				break;
			case PSP_UTILITY_DIALOG_VISIBLE :
				sceUtilityOskUpdate(2); // 2 is taken from ps2dev.org recommendation
            	break;
			case PSP_UTILITY_DIALOG_QUIT :
            	sceUtilityOskShutdownStart();
            	break;
			case PSP_UTILITY_DIALOG_FINISHED :
            	done = 1;
            	break;
			case PSP_UTILITY_DIALOG_NONE :
				break;
		}

		int i,j=0;
		for(i = 0; data.outtext[i]; i++)
		{
			if (data.outtext[i]!='\0' && data.outtext[i]!='\n' && data.outtext[i]!='\r')
			{
				oskhelper->text[j] = data.outtext[i];
            	j++;
			}
		}

		sceDisplayWaitVblankStart();
		sceGuSwapBuffers();
	}
}

void minimalRender()
{
      sceGuStart(GU_DIRECT,mylist);
      sceGuClearColor(0xff000000);
      sceGuClearDepth(0);
      sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
      sceGuFinish();
      sceGuSync(0,0);
}


