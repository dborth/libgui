/****************************************************************************
 * libgui
 *
 * Daryl Borth 2009-2026
 * GuiOptionBrowser.h
 ***************************************************************************/
#pragma once

#define MAX_OPTIONS 			150
#define OPTION_PAGESIZE 		8

//!Fixed-size name/value pair list backing a GuiOptionBrowser. A blank
//!name[] entry is skipped as a separator (see GuiOptionBrowser::findMenuItem()).
typedef struct _optionlist {
	int length; //!< Number of valid entries in name[]/value[]
	char name[MAX_OPTIONS][50]; //!< Option names; a blank name is a separator
	char value[MAX_OPTIONS][50]; //!< Option values, shown right-aligned next to each name
} OptionList;

//!Display a list of menu options
//!\ingroup grp_core
class GuiOptionBrowser : public GuiElement
{
	public:
		//!\param w Width
		//!\param h Height
		//!\param l Option list to display - must outlive this GuiOptionBrowser
		GuiOptionBrowser(int w, int h, OptionList * l);
		~GuiOptionBrowser();
		//!Sets the x offset of the name column
		void setCol1Position(int x);
		//!Sets the x offset of the value column
		void setCol2Position(int x);
		//!Finds the next non-blank option index from currentItem, skipping
		//!blank-named entries used as separators.
		//!\param currentItem Index to search from
		//!\param direction +1 to search forward, -1 to search backward
		//!\return the found index, or -1 if none remain in that direction
		int findMenuItem(int currentItem, int direction);
		//!\return the option index that was clicked this frame, or -1 if none was
		int getClickedOption();
		void resetState();
		void setFocus(int f);
		void draw() override;
		//!Forces the visible page to refresh from the underlying options list.
		void triggerUpdate();
		void resetText();
		void update(InputController * c);
		//!One value label per visible row of the current page. Public so
		//!the app can restyle individual values (eg. colour a setting).
		GuiText * optionVal[OPTION_PAGESIZE];
	protected:
		//!\cond INTERNAL
		int optionIndex[OPTION_PAGESIZE];
		GuiButton * optionBtn[OPTION_PAGESIZE];
		GuiText * optionTxt[OPTION_PAGESIZE];
		GuiImage * optionBg[OPTION_PAGESIZE];

		int selectedItem;
		int listOffset;
		OptionList * options;

		GuiButton * arrowUpBtn;
		GuiButton * arrowDownBtn;

		GuiImage * bgOptionsImg;
		GuiImage * scrollbarImg;
		GuiImage * arrowDownImg;
		GuiImage * arrowDownOverImg;
		GuiImage * arrowUpImg;
		GuiImage * arrowUpOverImg;

		GuiImageData * bgOptions;
		GuiImageData * bgOptionsEntry;
		GuiImageData * scrollbar;
		GuiImageData * arrowDown;
		GuiImageData * arrowDownOver;
		GuiImageData * arrowUp;
		GuiImageData * arrowUpOver;

		GuiSound * btnSoundOver;
		GuiSound * btnSoundClick;
		GuiTrigger * trigA;

		bool listChanged;
		//!\endcond
};
