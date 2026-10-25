# Theming

The core defines no look of its own. Every color, metric and tuning number that
`GuiTheme` holds comes from the application, which defines one function:

```cpp
// source/theme.cpp
#include "libgui/GuiTheme.h"

void GuiThemeInit(GuiTheme& t)
{
    t.background = { 18, 22, 29, 255 };
    t.accent     = { 255, 150, 40, 255 };
    // ... every other field
}
```

`GuiThemeInit()` is declared by `GuiTheme.h` and defined by the application,
like `main()`. A program that does not define it fails to link, so a forgotten
theme can never show up as an invisible UI. It runs once, the first time
anything calls `GuiTheme::current()`, and must not call `GuiTheme::current()`
itself. A `GuiTheme` made any other way (`GuiTheme t;`) has every field zero.

`source/theme.cpp` is the template's theme. Copy it and change the values to
restyle an application; the library files under `source/libgui/` do not change.

## What is in a theme

| Group | Fields | Used by |
|---|---|---|
| Colors | `background`, `surface`, `surfaceRaised`, `outline`, `outlineStrong`, `accent`, `accentLight`, `accentDark`, `text`, `textMuted`, `textOnAccent`, `textInverse`, `good`, `highlight`, `mark`, `shadow` | the style recipes, widgets, glyphs |
| Metrics | `radius`, `strokeWidth`, `padding`, `spacing`, `shadowBlur` | cards, panels, layouts |
| Glyphs | `glyphSize`, `glyphBumperWidth`, `glyphBumperHeight`, `glyphFontSize`, `glyphFontSizeSmall` | `GuiGlyph` |
| Recipe tuning | `shadowOffsetY`, `glowGrow`, `focusGlowAlpha`, `pressGlowAlpha`, `buttonGlowBlur`, `buttonGlowAlpha`, `rowPressAlpha`, `cardDisabledAlpha`, `buttonDisabledAlpha`, `buttonStrokeLight`, `barDarken` | `cardStyle()`, `buttonStyle()`, `rowStyle()`, `barStyle()` |

Colors that depend on each other are the application's choice, not the
library's: the template sets `highlight` to a mix of `surfaceRaised` and
`outlineStrong`, which is one line in `GuiThemeInit()`. `textInverse` is the ink
on a badge filled with `text` (the letter or symbol on a glyph), so it reads on
`text` the way `textOnAccent` reads on `accent`.

## The recipes

`panelStyle()`, `cardStyle()`, `buttonStyle()`, `chipStyle()`, `rowStyle()` and
`barStyle()` build a `GuiShapeStyle` from the theme. They decide structure
(which color goes at which end of a gradient, which states glow) and take every
number from the fields above. An application that wants a look the recipes do
not give builds its own `GuiShapeStyle` and passes it with `.style(...)`; it can
still read colors from `GuiTheme::current()`.
