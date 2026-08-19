/****************************************************************************
 * libgui Template
 * Tantric 2009
 *
 * menu.cpp
 * Menu flow routines - handles all menu logic
 ***************************************************************************/

#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <wiiuse/wpad.h>

#include "libgui/Gui.h"
#include "video.h"
#include "menu.h"
#include "demo.h"
#include "input.h"
#include "filelist.h"
#include "filebrowser.h"

#define THREAD_SLEEP 100

static GuiImageData * pointer[4];
static GuiImage * bgImg = nullptr;
static GuiSound * bgMusic = nullptr;
static GuiWindow * mainWindow = nullptr;
static lwp_t guithread = LWP_THREAD_NULL;
static bool guiHalt = true;

/****************************************************************************
 * ResumeGui
 *
 * Signals the GUI thread to start, and resumes the thread. This is called
 * after finishing the removal/insertion of new elements, and after initial
 * GUI setup.
 ***************************************************************************/
static void
ResumeGui()
{
	guiHalt = false;
	LWP_ResumeThread (guithread);
}

/****************************************************************************
 * HaltGui
 *
 * Signals the GUI thread to stop, and waits for GUI thread to stop
 * This is necessary whenever removing/inserting new elements into the GUI.
 * This eliminates the possibility that the GUI is in the middle of accessing
 * an element that is being changed.
 ***************************************************************************/
static void
HaltGui()
{
	guiHalt = true;

	// wait for thread to finish
	while(!LWP_ThreadIsSuspended(guithread))
		usleep(THREAD_SLEEP);
}

/****************************************************************************
 * WindowPrompt
 *
 * Displays a prompt window to user, with information, an error message, or
 * presenting a user with a choice
 ***************************************************************************/
int
WindowPrompt(const char *title, const char *msg, const char *btn1Label, const char *btn2Label)
{
	int choice = -1;

	GuiWindow promptWindow(448,288);
	promptWindow.setAlignment(ALIGN_H::CENTRE, ALIGN_V::MIDDLE);
	promptWindow.setPosition(0, -10);
	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND::PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);
	GuiTrigger trigA;
	trigA.setPrimaryTrigger();

	GuiImageData dialogBox(dialogue_box_png);
	GuiImage dialogBoxImg(&dialogBox);

	GuiText titleTxt(title, 26, (GuiColor){0, 0, 0, 255});
	titleTxt.setAlignment(ALIGN_H::CENTRE, ALIGN_V::TOP);
	titleTxt.setPosition(0,40);
	GuiText msgTxt(msg, 22, (GuiColor){0, 0, 0, 255});
	msgTxt.setAlignment(ALIGN_H::CENTRE, ALIGN_V::MIDDLE);
	msgTxt.setPosition(0,-20);
	msgTxt.setWrap(true, 400);

	GuiText btn1Txt(btn1Label, 22, (GuiColor){0, 0, 0, 255});
	GuiImage btn1Img(&btnOutline);
	GuiImage btn1ImgOver(&btnOutlineOver);
	GuiButton btn1(btnOutline.getWidth(), btnOutline.getHeight());

	if(btn2Label)
	{
		btn1.setAlignment(ALIGN_H::LEFT, ALIGN_V::BOTTOM);
		btn1.setPosition(20, -25);
	}
	else
	{
		btn1.setAlignment(ALIGN_H::CENTRE, ALIGN_V::BOTTOM);
		btn1.setPosition(0, -25);
	}

	btn1.setLabel(&btn1Txt);
	btn1.setImage(&btn1Img);
	btn1.setImageOver(&btn1ImgOver);
	btn1.setSoundOver(&btnSoundOver);
	btn1.setTrigger(&trigA);
	btn1.setState(STATE::SELECTED);
	btn1.setEffectGrow();

	GuiText btn2Txt(btn2Label, 22, (GuiColor){0, 0, 0, 255});
	GuiImage btn2Img(&btnOutline);
	GuiImage btn2ImgOver(&btnOutlineOver);
	GuiButton btn2(btnOutline.getWidth(), btnOutline.getHeight());
	btn2.setAlignment(ALIGN_H::RIGHT, ALIGN_V::BOTTOM);
	btn2.setPosition(-20, -25);
	btn2.setLabel(&btn2Txt);
	btn2.setImage(&btn2Img);
	btn2.setImageOver(&btn2ImgOver);
	btn2.setSoundOver(&btnSoundOver);
	btn2.setTrigger(&trigA);
	btn2.setEffectGrow();

	promptWindow.append(&dialogBoxImg);
	promptWindow.append(&titleTxt);
	promptWindow.append(&msgTxt);
	promptWindow.append(&btn1);

	if(btn2Label)
		promptWindow.append(&btn2);

	promptWindow.setEffect(EFFECT::SLIDE_TOP | EFFECT::SLIDE_IN, 50);
	HaltGui();
	mainWindow->setState(STATE::DISABLED);
	mainWindow->append(&promptWindow);
	mainWindow->changeFocus(&promptWindow);
	ResumeGui();

	while(choice == -1)
	{
		usleep(THREAD_SLEEP);

		if(btn1.getState() == STATE::CLICKED)
			choice = 1;
		else if(btn2.getState() == STATE::CLICKED)
			choice = 0;
	}

	promptWindow.setEffect(EFFECT::SLIDE_TOP | EFFECT::SLIDE_OUT, 50);
	while(promptWindow.getEffect() > 0)
		usleep(THREAD_SLEEP);
	HaltGui();
	mainWindow->remove(&promptWindow);
	mainWindow->setState(STATE::DEFAULT);
	ResumeGui();
	return choice;
}

/****************************************************************************
 * UpdateGUI
 *
 * Primary thread to allow GUI to respond to state changes, and draws GUI
 ***************************************************************************/

static void *
UpdateGUI (void *arg)
{
	(void)arg; // unused
	int i;

	while(1)
	{
		if(guiHalt)
		{
			LWP_SuspendThread(guithread);
		}
		else
		{
			UpdatePads();
			mainWindow->draw();

			for(i=3; i >= 0; i--) // so that player 1's cursor appears on top!
			{
				if(userInput[i]->getPadData().validPointer)
					Menu_DrawImg(userInput[i]->getPadData().cursor_x-48, userInput[i]->getPadData().cursor_y-48,
						96, 96, pointer[i]->getImage(), userInput[i]->getPadData().cursor_angle, 1, 1, 255);
				DoRumble(i);
			}

			Menu_Render();

			for(i=0; i < 4; i++)
				mainWindow->update(userInput[i]);

			if(ExitRequested)
			{
				for(i = 0; i <= 255; i += 15)
				{
					mainWindow->draw();
					Menu_DrawRectangle(0,0,screenwidth,screenheight,(GuiColor){0, 0, 0, (u8)i},1);
					Menu_Render();
				}
				ExitApp();
			}
		}
	}
	return nullptr;
}

/****************************************************************************
 * InitGUIThread
 *
 * Startup GUI threads
 ***************************************************************************/
void
InitGUIThreads()
{
	LWP_CreateThread (&guithread, UpdateGUI, nullptr, nullptr, 24576, 70);
}

/****************************************************************************
 * OnScreenKeyboard
 *
 * Opens an on-screen keyboard window, with the data entered being stored
 * into the specified variable.
 ***************************************************************************/
static void OnScreenKeyboard(char * var, u16 maxlen)
{
	int save = -1;

	GuiKeyboard keyboard(var, maxlen);

	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND::PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);
	GuiTrigger trigA;
	trigA.setPrimaryTrigger();

	GuiText okBtnTxt("OK", 22, (GuiColor){0, 0, 0, 255});
	GuiImage okBtnImg(&btnOutline);
	GuiImage okBtnImgOver(&btnOutlineOver);
	GuiButton okBtn(btnOutline.getWidth(), btnOutline.getHeight());

	okBtn.setAlignment(ALIGN_H::LEFT, ALIGN_V::BOTTOM);
	okBtn.setPosition(25, -25);

	okBtn.setLabel(&okBtnTxt);
	okBtn.setImage(&okBtnImg);
	okBtn.setImageOver(&okBtnImgOver);
	okBtn.setSoundOver(&btnSoundOver);
	okBtn.setTrigger(&trigA);
	okBtn.setEffectGrow();

	GuiText cancelBtnTxt("Cancel", 22, (GuiColor){0, 0, 0, 255});
	GuiImage cancelBtnImg(&btnOutline);
	GuiImage cancelBtnImgOver(&btnOutlineOver);
	GuiButton cancelBtn(btnOutline.getWidth(), btnOutline.getHeight());
	cancelBtn.setAlignment(ALIGN_H::RIGHT, ALIGN_V::BOTTOM);
	cancelBtn.setPosition(-25, -25);
	cancelBtn.setLabel(&cancelBtnTxt);
	cancelBtn.setImage(&cancelBtnImg);
	cancelBtn.setImageOver(&cancelBtnImgOver);
	cancelBtn.setSoundOver(&btnSoundOver);
	cancelBtn.setTrigger(&trigA);
	cancelBtn.setEffectGrow();

	keyboard.append(&okBtn);
	keyboard.append(&cancelBtn);

	HaltGui();
	mainWindow->setState(STATE::DISABLED);
	mainWindow->append(&keyboard);
	mainWindow->changeFocus(&keyboard);
	ResumeGui();

	while(save == -1)
	{
		usleep(THREAD_SLEEP);

		if(okBtn.getState() == STATE::CLICKED)
			save = 1;
		else if(cancelBtn.getState() == STATE::CLICKED)
			save = 0;
	}

	if(save)
	{
		snprintf(var, maxlen, "%s", keyboard.kbtextstr);
	}

	HaltGui();
	mainWindow->remove(&keyboard);
	mainWindow->setState(STATE::DEFAULT);
	ResumeGui();
}

/****************************************************************************
 * MenuBrowseDevice
 ***************************************************************************/
static int MenuBrowseDevice()
{
	char title[100];
	int i;

	ShutoffRumble();

	// populate initial directory listing
	if(BrowseDevice() <= 0)
	{
		int choice = WindowPrompt(
		"Error",
		"Unable to display files on selected load device.",
		"Retry",
		"Check Settings");

		if(choice)
			return MENU_BROWSE_DEVICE;
		else
			return MENU_SETTINGS;
	}

	int menu = MENU_NONE;

	sprintf(title, "Browse Files");

	GuiText titleTxt(title, 28, (GuiColor){255, 255, 255, 255});
	titleTxt.setAlignment(ALIGN_H::LEFT, ALIGN_V::TOP);
	titleTxt.setPosition(100,50);

	GuiTrigger trigA, trigB;
	trigA.setPrimaryTrigger();
	trigB.setSecondaryTrigger();

	GuiFileBrowser fileBrowser(552, 248);
	fileBrowser.setAlignment(ALIGN_H::CENTRE, ALIGN_V::TOP);
	fileBrowser.setPosition(0, 100);

	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);
	GuiText backBtnTxt("Go Back", 24, (GuiColor){0, 0, 0, 255});
	GuiImage backBtnImg(&btnOutline);
	GuiImage backBtnImgOver(&btnOutlineOver);
	GuiButton backBtn(btnOutline.getWidth(), btnOutline.getHeight());
	backBtn.setAlignment(ALIGN_H::LEFT, ALIGN_V::BOTTOM);
	backBtn.setPosition(30, -35);
	backBtn.setLabel(&backBtnTxt);
	backBtn.setImage(&backBtnImg);
	backBtn.setImageOver(&backBtnImgOver);
	backBtn.setTrigger(&trigA);
	backBtn.setTrigger(&trigB);
	backBtn.setEffectGrow();

	GuiWindow buttonWindow(screenwidth, screenheight);
	buttonWindow.append(&backBtn);

	HaltGui();
	mainWindow->append(&titleTxt);
	mainWindow->append(&fileBrowser);
	mainWindow->append(&buttonWindow);
	ResumeGui();

	while(menu == MENU_NONE)
	{
		usleep(THREAD_SLEEP);

		// update file browser based on arrow buttons
		// set MENU_EXIT if A button pressed on a file
		for(i=0; i < FILE_PAGESIZE; i++)
		{
			if(fileBrowser.fileList[i]->getState() == STATE::CLICKED)
			{
				fileBrowser.fileList[i]->resetState();
				// check corresponding browser entry
				if(browserList[browser.selIndex].isdir)
				{
					if(BrowserChangeFolder())
					{
						fileBrowser.resetState();
						fileBrowser.fileList[0]->setState(STATE::SELECTED);
						fileBrowser.triggerUpdate();
					}
					else
					{
						menu = MENU_BROWSE_DEVICE;
						break;
					}
				}
				else
				{
					ShutoffRumble();
					mainWindow->setState(STATE::DISABLED);
					// load file
					mainWindow->setState(STATE::DEFAULT);
				}
			}
		}
		if(backBtn.getState() == STATE::CLICKED)
			menu = MENU_SETTINGS;
	}
	HaltGui();
	mainWindow->remove(&titleTxt);
	mainWindow->remove(&buttonWindow);
	mainWindow->remove(&fileBrowser);
	return menu;
}

/****************************************************************************
 * MenuSettings
 ***************************************************************************/
static int MenuSettings()
{
	int menu = MENU_NONE;

	GuiText titleTxt("Settings", 28, (GuiColor){255, 255, 255, 255});
	titleTxt.setAlignment(ALIGN_H::LEFT, ALIGN_V::TOP);
	titleTxt.setPosition(50,50);

	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND::PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);
	GuiImageData btnLargeOutline(button_large_png);
	GuiImageData btnLargeOutlineOver(button_large_over_png);

	GuiTrigger trigA;
	trigA.setPrimaryTrigger();
	GuiTrigger trigHome;
	trigHome.setButtonOnlyTrigger(-1, GUI_BTN_HOME);

	GuiText fileBtnTxt("File Browser", 22, (GuiColor){0, 0, 0, 255});
	fileBtnTxt.setWrap(true, btnLargeOutline.getWidth()-30);
	GuiImage fileBtnImg(&btnLargeOutline);
	GuiImage fileBtnImgOver(&btnLargeOutlineOver);
	GuiButton fileBtn(btnLargeOutline.getWidth(), btnLargeOutline.getHeight());
	fileBtn.setAlignment(ALIGN_H::LEFT, ALIGN_V::TOP);
	fileBtn.setPosition(50, 120);
	fileBtn.setLabel(&fileBtnTxt);
	fileBtn.setImage(&fileBtnImg);
	fileBtn.setImageOver(&fileBtnImgOver);
	fileBtn.setSoundOver(&btnSoundOver);
	fileBtn.setTrigger(&trigA);
	fileBtn.setEffectGrow();

	GuiText videoBtnTxt("Video", 22, (GuiColor){0, 0, 0, 255});
	videoBtnTxt.setWrap(true, btnLargeOutline.getWidth()-30);
	GuiImage videoBtnImg(&btnLargeOutline);
	GuiImage videoBtnImgOver(&btnLargeOutlineOver);
	GuiButton videoBtn(btnLargeOutline.getWidth(), btnLargeOutline.getHeight());
	videoBtn.setAlignment(ALIGN_H::CENTRE, ALIGN_V::TOP);
	videoBtn.setPosition(0, 120);
	videoBtn.setLabel(&videoBtnTxt);
	videoBtn.setImage(&videoBtnImg);
	videoBtn.setImageOver(&videoBtnImgOver);
	videoBtn.setSoundOver(&btnSoundOver);
	videoBtn.setTrigger(&trigA);
	videoBtn.setEffectGrow();

	GuiText savingBtnTxt1("Saving", 22, (GuiColor){0, 0, 0, 255});
	GuiText savingBtnTxt2("&", 18, (GuiColor){0, 0, 0, 255});
	GuiText savingBtnTxt3("Loading", 22, (GuiColor){0, 0, 0, 255});
	savingBtnTxt1.setPosition(0, -20);
	savingBtnTxt3.setPosition(0, +20);
	GuiImage savingBtnImg(&btnLargeOutline);
	GuiImage savingBtnImgOver(&btnLargeOutlineOver);
	GuiButton savingBtn(btnLargeOutline.getWidth(), btnLargeOutline.getHeight());
	savingBtn.setAlignment(ALIGN_H::RIGHT, ALIGN_V::TOP);
	savingBtn.setPosition(-50, 120);
	savingBtn.setLabel(&savingBtnTxt1, 0);
	savingBtn.setLabel(&savingBtnTxt2, 1);
	savingBtn.setLabel(&savingBtnTxt3, 2);
	savingBtn.setImage(&savingBtnImg);
	savingBtn.setImageOver(&savingBtnImgOver);
	savingBtn.setSoundOver(&btnSoundOver);
	savingBtn.setTrigger(&trigA);
	savingBtn.setEffectGrow();

	GuiText menuBtnTxt("Menu", 22, (GuiColor){0, 0, 0, 255});
	menuBtnTxt.setWrap(true, btnLargeOutline.getWidth()-30);
	GuiImage menuBtnImg(&btnLargeOutline);
	GuiImage menuBtnImgOver(&btnLargeOutlineOver);
	GuiButton menuBtn(btnLargeOutline.getWidth(), btnLargeOutline.getHeight());
	menuBtn.setAlignment(ALIGN_H::CENTRE, ALIGN_V::TOP);
	menuBtn.setPosition(-125, 250);
	menuBtn.setLabel(&menuBtnTxt);
	menuBtn.setImage(&menuBtnImg);
	menuBtn.setImageOver(&menuBtnImgOver);
	menuBtn.setSoundOver(&btnSoundOver);
	menuBtn.setTrigger(&trigA);
	menuBtn.setEffectGrow();

	GuiText networkBtnTxt("Network", 22, (GuiColor){0, 0, 0, 255});
	networkBtnTxt.setWrap(true, btnLargeOutline.getWidth()-30);
	GuiImage networkBtnImg(&btnLargeOutline);
	GuiImage networkBtnImgOver(&btnLargeOutlineOver);
	GuiButton networkBtn(btnLargeOutline.getWidth(), btnLargeOutline.getHeight());
	networkBtn.setAlignment(ALIGN_H::CENTRE, ALIGN_V::TOP);
	networkBtn.setPosition(125, 250);
	networkBtn.setLabel(&networkBtnTxt);
	networkBtn.setImage(&networkBtnImg);
	networkBtn.setImageOver(&networkBtnImgOver);
	networkBtn.setSoundOver(&btnSoundOver);
	networkBtn.setTrigger(&trigA);
	networkBtn.setEffectGrow();

	GuiText exitBtnTxt("Exit", 22, (GuiColor){0, 0, 0, 255});
	GuiImage exitBtnImg(&btnOutline);
	GuiImage exitBtnImgOver(&btnOutlineOver);
	GuiButton exitBtn(btnOutline.getWidth(), btnOutline.getHeight());
	exitBtn.setAlignment(ALIGN_H::LEFT, ALIGN_V::BOTTOM);
	exitBtn.setPosition(100, -35);
	exitBtn.setLabel(&exitBtnTxt);
	exitBtn.setImage(&exitBtnImg);
	exitBtn.setImageOver(&exitBtnImgOver);
	exitBtn.setSoundOver(&btnSoundOver);
	exitBtn.setTrigger(&trigA);
	exitBtn.setTrigger(&trigHome);
	exitBtn.setEffectGrow();

	GuiText resetBtnTxt("Reset Settings", 22, (GuiColor){0, 0, 0, 255});
	GuiImage resetBtnImg(&btnOutline);
	GuiImage resetBtnImgOver(&btnOutlineOver);
	GuiButton resetBtn(btnOutline.getWidth(), btnOutline.getHeight());
	resetBtn.setAlignment(ALIGN_H::RIGHT, ALIGN_V::BOTTOM);
	resetBtn.setPosition(-100, -35);
	resetBtn.setLabel(&resetBtnTxt);
	resetBtn.setImage(&resetBtnImg);
	resetBtn.setImageOver(&resetBtnImgOver);
	resetBtn.setSoundOver(&btnSoundOver);
	resetBtn.setTrigger(&trigA);
	resetBtn.setEffectGrow();

	HaltGui();
	GuiWindow w(screenwidth, screenheight);
	w.append(&titleTxt);
	w.append(&fileBtn);
	w.append(&videoBtn);
	w.append(&savingBtn);
	w.append(&menuBtn);

#ifdef HW_RVL
	w.append(&networkBtn);
#endif

	w.append(&exitBtn);
	w.append(&resetBtn);

	mainWindow->append(&w);

	ResumeGui();

	while(menu == MENU_NONE)
	{
		usleep(THREAD_SLEEP);

		if(fileBtn.getState() == STATE::CLICKED)
		{
			menu = MENU_BROWSE_DEVICE;
		}
		else if(videoBtn.getState() == STATE::CLICKED)
		{
			menu = MENU_SETTINGS_FILE;
		}
		else if(savingBtn.getState() == STATE::CLICKED)
		{
			menu = MENU_SETTINGS_FILE;
		}
		else if(menuBtn.getState() == STATE::CLICKED)
		{
			menu = MENU_SETTINGS_FILE;
		}
		else if(networkBtn.getState() == STATE::CLICKED)
		{
			menu = MENU_SETTINGS_FILE;
		}
		else if(exitBtn.getState() == STATE::CLICKED)
		{
			menu = MENU_EXIT;
		}
		else if(resetBtn.getState() == STATE::CLICKED)
		{
			resetBtn.resetState();

			int choice = WindowPrompt(
				"Reset Settings",
				"Are you sure that you want to reset your settings?",
				"Yes",
				"No");
			if(choice == 1)
			{
				// reset settings
			}
		}
	}

	HaltGui();
	mainWindow->remove(&w);
	return menu;
}

/****************************************************************************
 * MenuSettingsFile
 ***************************************************************************/

static int MenuSettingsFile()
{
	int menu = MENU_NONE;
	int ret;
	int i = 0;
	bool firstRun = true;
	OptionList options;
	sprintf(options.name[i++], "Load Device");
	sprintf(options.name[i++], "Save Device");
	sprintf(options.name[i++], "Folder 1");
	sprintf(options.name[i++], "Folder 2");
	sprintf(options.name[i++], "Folder 3");
	sprintf(options.name[i++], "Auto Load");
	sprintf(options.name[i++], "Auto Save");
	options.length = i;

	GuiText titleTxt("Settings - Saving & Loading", 28, (GuiColor){255, 255, 255, 255});
	titleTxt.setAlignment(ALIGN_H::LEFT, ALIGN_V::TOP);
	titleTxt.setPosition(50,50);

	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND::PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);

	GuiTrigger trigA, trigB;
	trigA.setPrimaryTrigger();
	trigB.setSecondaryTrigger();

	GuiText backBtnTxt("Go Back", 22, (GuiColor){0, 0, 0, 255});
	GuiImage backBtnImg(&btnOutline);
	GuiImage backBtnImgOver(&btnOutlineOver);
	GuiButton backBtn(btnOutline.getWidth(), btnOutline.getHeight());
	backBtn.setAlignment(ALIGN_H::LEFT, ALIGN_V::BOTTOM);
	backBtn.setPosition(100, -35);
	backBtn.setLabel(&backBtnTxt);
	backBtn.setImage(&backBtnImg);
	backBtn.setImageOver(&backBtnImgOver);
	backBtn.setSoundOver(&btnSoundOver);
	backBtn.setTrigger(&trigA);
	backBtn.setTrigger(&trigB);
	backBtn.setEffectGrow();

	GuiOptionBrowser optionBrowser(552, 248, &options);
	optionBrowser.setPosition(0, 108);
	optionBrowser.setAlignment(ALIGN_H::CENTRE, ALIGN_V::TOP);
	optionBrowser.setCol2Position(185);

	HaltGui();
	GuiWindow w(screenwidth, screenheight);
	w.append(&backBtn);
	mainWindow->append(&optionBrowser);
	mainWindow->append(&w);
	mainWindow->append(&titleTxt);
	ResumeGui();

	while(menu == MENU_NONE)
	{
		usleep(THREAD_SLEEP);

		ret = optionBrowser.getClickedOption();

		switch (ret)
		{
			case 0:
				Settings.LoadMethod++;
				break;

			case 1:
				Settings.SaveMethod++;
				break;

			case 2:
				OnScreenKeyboard(Settings.Folder1, 256);
				break;

			case 3:
				OnScreenKeyboard(Settings.Folder2, 256);
				break;

			case 4:
				OnScreenKeyboard(Settings.Folder3, 256);
				break;

			case 5:
				Settings.AutoLoad++;
				if (Settings.AutoLoad > 2)
					Settings.AutoLoad = 0;
				break;

			case 6:
				Settings.AutoSave++;
				if (Settings.AutoSave > 3)
					Settings.AutoSave = 0;
				break;
		}

		if(ret >= 0 || firstRun)
		{
			firstRun = false;

			// correct load/save methods out of bounds
			if(Settings.LoadMethod > 4)
				Settings.LoadMethod = 0;
			if(Settings.SaveMethod > 6)
				Settings.SaveMethod = 0;

			if (Settings.LoadMethod == METHOD_AUTO) sprintf (options.value[0],"Auto Detect");
			else if (Settings.LoadMethod == METHOD_SD) sprintf (options.value[0],"SD");
			else if (Settings.LoadMethod == METHOD_USB) sprintf (options.value[0],"USB");
			else if (Settings.LoadMethod == METHOD_DVD) sprintf (options.value[0],"DVD");
			else if (Settings.LoadMethod == METHOD_SMB) sprintf (options.value[0],"Network");

			if (Settings.SaveMethod == METHOD_AUTO) sprintf (options.value[1],"Auto Detect");
			else if (Settings.SaveMethod == METHOD_SD) sprintf (options.value[1],"SD");
			else if (Settings.SaveMethod == METHOD_USB) sprintf (options.value[1],"USB");
			else if (Settings.SaveMethod == METHOD_SMB) sprintf (options.value[1],"Network");
			else if (Settings.SaveMethod == METHOD_MC_SLOTA) sprintf (options.value[1],"MC Slot A");
			else if (Settings.SaveMethod == METHOD_MC_SLOTB) sprintf (options.value[1],"MC Slot B");

			// crop names for display
			memcpy(options.value[2], Settings.Folder1, 49);
			memcpy(options.value[3], Settings.Folder2, 49);
			memcpy(options.value[4], Settings.Folder3, 49);
			options.value[2][49] = '\0';
			options.value[3][49] = '\0';
			options.value[4][49] = '\0';

			if (Settings.AutoLoad == 0) sprintf (options.value[5],"Off");
			else if (Settings.AutoLoad == 1) sprintf (options.value[5],"Some");
			else if (Settings.AutoLoad == 2) sprintf (options.value[5],"All");

			if (Settings.AutoSave == 0) sprintf (options.value[6],"Off");
			else if (Settings.AutoSave == 1) sprintf (options.value[6],"Some");
			else if (Settings.AutoSave == 2) sprintf (options.value[6],"All");

			optionBrowser.triggerUpdate();
		}

		if(backBtn.getState() == STATE::CLICKED)
		{
			menu = MENU_SETTINGS;
		}
	}
	HaltGui();
	mainWindow->remove(&optionBrowser);
	mainWindow->remove(&w);
	mainWindow->remove(&titleTxt);
	return menu;
}

/****************************************************************************
 * MainMenu
 ***************************************************************************/
void MainMenu(int menu)
{
	int currentMenu = menu;

	#ifdef HW_RVL
	pointer[0] = new GuiImageData(player1_point_png);
	pointer[1] = new GuiImageData(player2_point_png);
	pointer[2] = new GuiImageData(player3_point_png);
	pointer[3] = new GuiImageData(player4_point_png);
	#endif

	mainWindow = new GuiWindow(screenwidth, screenheight);

	bgImg = new GuiImage(screenwidth, screenheight, (GuiColor){50, 50, 50, 255});
	bgImg->colorStripe(30);
	mainWindow->append(bgImg);

	GuiTrigger trigA;
	trigA.setPrimaryTrigger();

	ResumeGui();

	bgMusic = new GuiSound(bg_music_ogg, bg_music_ogg_size, SOUND::OGG);
	bgMusic->setVolume(50);
	bgMusic->play(); // startup music

	while(currentMenu != MENU_EXIT)
	{
		switch (currentMenu)
		{
			case MENU_SETTINGS:
				currentMenu = MenuSettings();
				break;
			case MENU_SETTINGS_FILE:
				currentMenu = MenuSettingsFile();
				break;
			case MENU_BROWSE_DEVICE:
				currentMenu = MenuBrowseDevice();
				break;
			default: // unrecognized menu
				currentMenu = MenuSettings();
				break;
		}
	}

	ResumeGui();
	ExitRequested = 1;
	while(1) usleep(THREAD_SLEEP);

	HaltGui();

	bgMusic->stop();
	delete bgMusic;
	delete bgImg;
	delete mainWindow;

	delete pointer[0];
	delete pointer[1];
	delete pointer[2];
	delete pointer[3];

	mainWindow = nullptr;
}
