/****************************************************************************
 * libgui
 * Daryl Borth 2009-2026
 * GuiFileBrowser.h
 ***************************************************************************/

#ifndef GUI_FILE_BROWSER_H
#define GUI_FILE_BROWSER_H

#define FILE_PAGESIZE 			10

//!Display a list of files
class GuiFileBrowser : public GuiElement
{
	public:
		GuiFileBrowser(int w, int h);
		~GuiFileBrowser();
		void resetState();
		void setFocus(int f);
	void draw() override;
		void triggerUpdate();
		void update(GuiInputController * c);
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

#endif // GUIFILEBROWSER_H
