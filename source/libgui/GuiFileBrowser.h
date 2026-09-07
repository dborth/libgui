/****************************************************************************
 * libgui
 * Daryl Borth 2009-2026
 * GuiFileBrowser.h
 ***************************************************************************/
#pragma once

#define FILE_PAGESIZE 			8

//!Display a list of files
class GuiFileBrowser : public GuiElement
{
	public:
		//!\param w Width
		//!\param h Height
		GuiFileBrowser(int w, int h);
		~GuiFileBrowser();
		void resetState();
		void setFocus(int f);
		void draw() override;
		//!Forces the visible page to refresh from the underlying browser
		//!data, eg. after the directory listing changed externally.
		void triggerUpdate();
		void update(InputController * c);
		GuiButton * fileList[FILE_PAGESIZE];
	protected:
		GuiText * fileListText[FILE_PAGESIZE];
		GuiImage * fileListBg[FILE_PAGESIZE];
		GuiImage * fileListFolder[FILE_PAGESIZE];

		GuiButton * arrowUpBtn;
		GuiButton * arrowDownBtn;
		GuiButton * scrollbarBoxBtn;

		GuiImage * bgFileSelectionImg;
		GuiImage * scrollbarImg;
		GuiImage * arrowDownImg;
		GuiImage * arrowDownOverImg;
		GuiImage * arrowUpImg;
		GuiImage * arrowUpOverImg;
		GuiImage * scrollbarBoxImg;
		GuiImage * scrollbarBoxOverImg;

		GuiImageData * bgFileSelection;
		GuiImageData * bgFileSelectionEntry;
		GuiImageData * fileFolder;
		GuiImageData * scrollbar;
		GuiImageData * arrowDown;
		GuiImageData * arrowDownOver;
		GuiImageData * arrowUp;
		GuiImageData * arrowUpOver;
		GuiImageData * scrollbarBox;
		GuiImageData * scrollbarBoxOver;

		GuiSound * btnSoundOver;
		GuiSound * btnSoundClick;
		GuiTrigger * trigA;
		GuiTrigger * trigHeldA;

		int selectedItem;
		int numEntries;
		bool listChanged;
};
