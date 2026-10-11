/****************************************************************************
 * libgui
 * Daryl Borth 2026
 * GuiGlyph.h
 ***************************************************************************/
#pragma once

//!A small control icon drawn from shapes: a light badge with a dark symbol or letter, for list rows and hint
//!bars. Not selectable; it is placed by a layout like any element and sized by its kind.
//!`ui.glyph(GLYPH::BUTTON, "A")`, `ui.glyph(GLYPH::UP)`
//!\ingroup grp_controller
class GuiGlyph : public GuiElement
{
	public:
		//!\param kind Which icon
		//!\param letter Text on the badge for BUTTON, BUMPER and STICK ("A", "ZL", "R"); ignored by the others
		GuiGlyph(GLYPH kind, const char * letter = "");
		~GuiGlyph();
		void draw() override;
	private:
		GLYPH kind;
		GuiText * text;
		GuiMesh mesh;
		float meshFeather;
		std::vector<MeshVertex> scratch;
};
