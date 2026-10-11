/****************************************************************************
 * libgui
 * Daryl Borth 2026
 * GuiControllerView.h
 ***************************************************************************/
#pragma once
#include "GuiControllerDraw.h"

//!Draws a controller from a GuiControllerLayout and shows what is happening on it: controls light up
//!while pressed, a chosen control is highlighted (optionally with a line from a list row to it), and
//!sticks follow their position. It is a picture, not a button: selection lives in the list beside it,
//!and the list tells the view what to highlight.
//!\ingroup grp_controller
class GuiControllerView : public GuiElement
{
	public:
		//!\param layout The controller to draw (must outlive the view)
		//!\param scale Multiplies the layout's units
		GuiControllerView(const GuiControllerLayout& layout, float scale = 1.0f);
		~GuiControllerView();

		//!Changes the controller drawn
		void setLayout(const GuiControllerLayout& layout);
		//!Changes the size
		void setScale(float scale);
		//!Highlights one control (0 = none)
		void setHighlight(int id);
		//!Highlights several controls (a D-pad, a group of face buttons)
		void setHighlight(const std::vector<int>& ids);
		//!Gets the highlighted control ids
		const std::vector<int>& getHighlight() const { return highlighted; }
		//!Draws a line from a point (design-canvas coordinates, such as a list row's edge) to the first
		//!highlighted control, or removes it
		void setLeader(bool on, float fromX = 0.0f, float fromY = 0.0f);
		//!Sets which controls are drawn as pressed; asked every frame (null for none)
		void setLit(std::function<bool(int)> fn) { lit = fn; }
		//!Sets an analog stick's position (-1 to 1 each)
		void setStick(int id, float x, float y);
		//!Gets the center of a control in design-canvas coordinates (false if the layout has no such control)
		bool getSpotCentre(int id, float& x, float& y) const;

		void draw() override;
	private:
		struct SpotCache {
			GuiMesh mesh[4];
			bool built[4];
		};

		void rebuild();
		void clearLabels();
		void emit(const GuiMesh& m);

		const GuiControllerLayout * layout;
		float scale;
		std::vector<int> highlighted;
		bool leader;
		float leaderX, leaderY;
		std::function<bool(int)> lit;
		std::vector<float> stickX, stickY;		//!< Per spot (only used by sticks)
		GuiMesh body;
		std::vector<SpotCache> spots;
		std::vector<GuiText *> labels;			//!< Per spot, null where there is no label
		std::vector<bool> labelLit;
		float meshFeather;
		bool dirty;
		std::vector<MeshVertex> scratch;
		float originX, originY;
		int alpha;
};
