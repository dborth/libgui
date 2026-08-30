/****************************************************************************
 * libgui Template
 * Tantric 2009-2026
 *
 * menu.cpp
 * Menu flow routines - handles all menu logic
 ***************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "libgui/Gui.h"
#include "drivers/Platform.h"
#include "menu.h"
#include "demo.h"
#include "filelist.h"
#include "filebrowser.h"

static GuiImageData * pointer[4];
static GuiImage * bgImg = nullptr;
static GuiSound * bgMusic = nullptr;
static GuiWindow * mainWindow = nullptr;

/****************************************************************************
 * UpdateGui
 *
 * Single-threaded GUI update primitive. Performs exactly one frame step,
 * updates the platform input, applies it to the element tree, draws,
 * and renders. Checks the canonical global exit signals and handles the
 * fade-out-on-exit transition before signaling the caller to stop.
 ***************************************************************************/
bool UpdateGui()
{
	if (!mainWindow)
		return true;

	int i;
	float deltaTime = 1.0f / 60.0f;
	platform->getInput()->update(deltaTime);

	for(i = 3; i >= 0; i--)
		mainWindow->update(userInput[i]);

	mainWindow->draw();

	for(i = 3; i >= 0; i--)
	{
		if(userInput[i]->getPadData().validPointer)
			platform->getVideo()->getImageRenderer()->drawTexture(pointer[i]->getTexture(), userInput[i]->getPadData().cursor_x-48, userInput[i]->getPadData().cursor_y-48,
				96, 96, userInput[i]->getPadData().cursor_angle, 1, 1, 255);
	}

	platform->getVideo()->render();

	if(ExitRequested || platform->shutdownRequested())
	{
		for(i = 0; i <= 255; i += 15)
		{
			mainWindow->draw();
			platform->getVideo()->getImageRenderer()->drawRectangle(0,0,platform->getVideo()->getScreenWidth(),platform->getVideo()->getScreenHeight(),(PixelColor){0, 0, 0, (uint8_t)i});
			platform->getVideo()->render();
		}
		return false;
	}

	return true;
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

	GuiText titleTxt(title, 26, (PixelColor){0, 0, 0, 255});
	titleTxt.setAlignment(ALIGN_H::CENTRE, ALIGN_V::TOP);
	titleTxt.setPosition(0,40);
	GuiText msgTxt(msg, 22, (PixelColor){0, 0, 0, 255});
	msgTxt.setAlignment(ALIGN_H::CENTRE, ALIGN_V::MIDDLE);
	msgTxt.setPosition(0,-20);
	msgTxt.setWrap(true, 400);

	GuiText btn1Txt(btn1Label, 22, (PixelColor){0, 0, 0, 255});
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

	GuiText btn2Txt(btn2Label, 22, (PixelColor){0, 0, 0, 255});
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
	mainWindow->setState(STATE::DISABLED);
	mainWindow->appendWithAutoRemove(&promptWindow);
	mainWindow->changeFocus(&promptWindow);

	while(choice == -1)
	{
		if(!UpdateGui()) return -1;

		if(btn1.getState() == STATE::CLICKED)
			choice = 1;
		else if(btn2.getState() == STATE::CLICKED)
			choice = 0;
	}

	promptWindow.setEffect(EFFECT::SLIDE_TOP | EFFECT::SLIDE_OUT, 50);
	while(promptWindow.getEffect() > 0)
	{
		if(!UpdateGui()) return choice;
	}

	mainWindow->setState(STATE::DEFAULT);
	return choice;
}

/****************************************************************************
 * OnScreenKeyboard
 *
 * Opens an on-screen keyboard window, with the data entered being stored
 * into the specified variable.
 ***************************************************************************/
static void OnScreenKeyboard(char * var, uint16_t maxlen)
{
	int save = -1;

	GuiKeyboard keyboard(var, maxlen);

	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND::PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);
	GuiTrigger trigA;
	trigA.setPrimaryTrigger();

	GuiText okBtnTxt("OK", 22, (PixelColor){0, 0, 0, 255});
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

	GuiText cancelBtnTxt("Cancel", 22, (PixelColor){0, 0, 0, 255});
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

	mainWindow->setState(STATE::DISABLED);
	mainWindow->appendWithAutoRemove(&keyboard);
	mainWindow->changeFocus(&keyboard);

	while(save == -1)
	{
		if(!UpdateGui()) return;

		if(okBtn.getState() == STATE::CLICKED)
			save = 1;
		else if(cancelBtn.getState() == STATE::CLICKED)
			save = 0;
	}

	if(save)
	{
		snprintf(var, maxlen, "%s", keyboard.kbtextstr);
	}

	mainWindow->setState(STATE::DEFAULT);
}

/****************************************************************************
 * MenuBrowseDevice
 ***************************************************************************/
static int MenuBrowseDevice()
{
	char title[100];
	int i;

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

	GuiText titleTxt(title, 28, (PixelColor){255, 255, 255, 255});
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
	GuiText backBtnTxt("Go Back", 24, (PixelColor){0, 0, 0, 255});
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

	GuiWindow buttonWindow(platform->getVideo()->getScreenWidth(), platform->getVideo()->getScreenHeight());
	buttonWindow.append(&backBtn);

	mainWindow->appendWithAutoRemove(&titleTxt);
	mainWindow->appendWithAutoRemove(&fileBrowser);
	mainWindow->appendWithAutoRemove(&buttonWindow);

	while(menu == MENU_NONE)
	{
		if(!UpdateGui()) return MENU_EXIT;

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
					mainWindow->setState(STATE::DISABLED);
					// load file
					mainWindow->setState(STATE::DEFAULT);
				}
			}
		}
		if(backBtn.getState() == STATE::CLICKED)
			menu = MENU_SETTINGS;
	}

	return menu;
}

/****************************************************************************
 * MenuSettings
 ***************************************************************************/
static int MenuSettings()
{
	int menu = MENU_NONE;

	GuiText titleTxt("Settings", 28, (PixelColor){255, 255, 255, 255});
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

	GuiText fileBtnTxt("File Browser", 22, (PixelColor){0, 0, 0, 255});
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

	GuiText videoBtnTxt("Video", 22, (PixelColor){0, 0, 0, 255});
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

	GuiText savingBtnTxt1("Saving", 22, (PixelColor){0, 0, 0, 255});
	GuiText savingBtnTxt2("&", 18, (PixelColor){0, 0, 0, 255});
	GuiText savingBtnTxt3("Loading", 22, (PixelColor){0, 0, 0, 255});
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

	GuiText menuBtnTxt("Menu", 22, (PixelColor){0, 0, 0, 255});
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

	GuiText networkBtnTxt("Network", 22, (PixelColor){0, 0, 0, 255});
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

	GuiText exitBtnTxt("Exit", 22, (PixelColor){0, 0, 0, 255});
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

	GuiText resetBtnTxt("Reset Settings", 22, (PixelColor){0, 0, 0, 255});
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

	GuiWindow w(platform->getVideo()->getScreenWidth(), platform->getVideo()->getScreenHeight());
	w.append(&titleTxt);
	w.append(&fileBtn);
	w.append(&videoBtn);
	w.append(&savingBtn);
	w.append(&menuBtn);
	w.append(&networkBtn);
	w.append(&exitBtn);
	w.append(&resetBtn);

	mainWindow->appendWithAutoRemove(&w);

	while(menu == MENU_NONE)
	{
		if(!UpdateGui()) return MENU_EXIT;

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

	GuiText titleTxt("Settings - Saving & Loading", 28, (PixelColor){255, 255, 255, 255});
	titleTxt.setAlignment(ALIGN_H::LEFT, ALIGN_V::TOP);
	titleTxt.setPosition(50,50);

	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND::PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);

	GuiTrigger trigA, trigB;
	trigA.setPrimaryTrigger();
	trigB.setSecondaryTrigger();

	GuiText backBtnTxt("Go Back", 22, (PixelColor){0, 0, 0, 255});
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

	GuiWindow w(platform->getVideo()->getScreenWidth(), platform->getVideo()->getScreenHeight());
	w.append(&backBtn);
	mainWindow->appendWithAutoRemove(&optionBrowser);
	mainWindow->appendWithAutoRemove(&w);
	mainWindow->appendWithAutoRemove(&titleTxt);

	while(menu == MENU_NONE)
	{
		if(!UpdateGui()) return MENU_EXIT;

		ret = optionBrowser.getClickedOption();

		switch (ret)
		{
			case 0:
				Settings.LoadMethod++;
				if(Settings.LoadMethod >= METHOD_LENGTH)
					Settings.LoadMethod = METHOD_AUTO;
				break;

			case 1:
				Settings.SaveMethod++;
				if(Settings.SaveMethod >= METHOD_LENGTH)
					Settings.SaveMethod = METHOD_AUTO;
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
				if (Settings.AutoLoad >= AUTO_LENGTH)
					Settings.AutoLoad = AUTO_OFF;
				break;

			case 6:
				Settings.AutoSave++;
				if (Settings.AutoSave >= AUTO_LENGTH)
					Settings.AutoSave = AUTO_OFF;
				break;
		}

		if(ret >= 0 || firstRun)
		{
			firstRun = false;

			if (Settings.LoadMethod == METHOD_AUTO) sprintf (options.value[0],"Auto Detect");
			else if (Settings.LoadMethod == METHOD_SD) sprintf (options.value[0],"SD");
			else if (Settings.LoadMethod == METHOD_USB) sprintf (options.value[0],"USB");
			else if (Settings.LoadMethod == METHOD_NETWORK) sprintf (options.value[0],"Network");

			if (Settings.SaveMethod == METHOD_AUTO) sprintf (options.value[1],"Auto Detect");
			else if (Settings.SaveMethod == METHOD_SD) sprintf (options.value[1],"SD");
			else if (Settings.SaveMethod == METHOD_USB) sprintf (options.value[1],"USB");
			else if (Settings.SaveMethod == METHOD_NETWORK) sprintf (options.value[1],"Network");

			// crop names for display
			memcpy(options.value[2], Settings.Folder1, 49);
			memcpy(options.value[3], Settings.Folder2, 49);
			memcpy(options.value[4], Settings.Folder3, 49);
			options.value[2][49] = '\0';
			options.value[3][49] = '\0';
			options.value[4][49] = '\0';

			if (Settings.AutoLoad == AUTO_OFF) sprintf (options.value[5],"Off");
			else if (Settings.AutoLoad == AUTO_SOME) sprintf (options.value[5],"Some");
			else if (Settings.AutoLoad == AUTO_ALL) sprintf (options.value[5],"All");

			if (Settings.AutoSave == AUTO_OFF) sprintf (options.value[6],"Off");
			else if (Settings.AutoSave == AUTO_SOME) sprintf (options.value[6],"Some");
			else if (Settings.AutoSave == AUTO_ALL) sprintf (options.value[6],"All");

			optionBrowser.triggerUpdate();
		}

		if(backBtn.getState() == STATE::CLICKED)
		{
			menu = MENU_SETTINGS;
		}
	}

	return menu;
}

/****************************************************************************
 * MainMenu
 ***************************************************************************/
void MainMenu(int menu)
{
	int currentMenu = menu;

	pointer[0] = new GuiImageData(player1_point_png);
	pointer[1] = new GuiImageData(player2_point_png);
	pointer[2] = new GuiImageData(player3_point_png);
	pointer[3] = new GuiImageData(player4_point_png);

	mainWindow = new GuiWindow(platform->getVideo()->getScreenWidth(), platform->getVideo()->getScreenHeight());

	bgImg = new GuiImage(platform->getVideo()->getScreenWidth(), platform->getVideo()->getScreenHeight(), (PixelColor){50, 50, 50, 255});
	bgImg->setStripe(30);
	mainWindow->append(bgImg);

	GuiTrigger trigA;
	trigA.setPrimaryTrigger();

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
