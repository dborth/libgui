# Input

Every driver's `update()` builds one platform-independent `InputPadData` per
channel per frame: a unified button bitmask (pressed / held / released) that
is a superset of every supported controller, plus per-hardware-profile copies
(GameCube pad, Wiimote, Nunchuk, Classic Controller, Wii U Pro Controller,
Wii U GamePad) so a controller made of several parts - for example a Wiimote
with a Nunchuk - reports each part separately before they are merged. It also
carries the pointer position and angle, whether the pointer is valid, and
whether it came from touch. A persistent `InputController` per channel turns
that into what widgets consume: analog deadzone, directional repeat and scroll
delay timing, and Wiimote orientation, so none of that logic is duplicated per
platform.

* **Pointer smoothing.** The Wii U driver filters the Wiimote IR pointer with
  a One Euro Filter (`OneEuroFilter.h`), which smooths heavily while the
  pointer is nearly still and backs off as it moves faster - a steady cursor
  at rest without lag on fast sweeps.
* **GamePad touch.** Touch positions are scaled onto the design canvas and
  delivered as pointer input; touch-down, hold, and release act as A-button
  press, hold, and release, so touch-driven widgets behave like a pointer
  click.
* **HOME button.** On Wii U the HOME button is delivered to the app as an
  ordinary `INPUT_BTN_HOME` press, and the system HOME menu overlay doesn't
  open on its own, so the app decides what HOME does.
  `WutInputDriver::openHomeButtonOverlay()` opens the overlay on demand (the
  demo's "Wii U Overlay" button).
* **Wiimote orientation.** `InputDriver::setWiimoteOrientation()` selects
  `WIIMOTE_ORIENTATION_VERTICAL` or `WIIMOTE_ORIENTATION_HORIZONTAL`; the
  semantic Accept/Cancel triggers resolve to A/B (vertical) or 2/1
  (horizontal) accordingly.
* **Rumble.** `setRumbleEnabled()` turns rumble on or off globally. Menu
  hover feedback is a short (about 33 ms) tick followed by an enforced quiet
  gap (about 100 ms), so quickly moving across buttons doesn't produce a
  continuous buzz. On the Wii U GamePad the tick uses a reduced-amplitude
  pattern.
* **Wii U GamePad on Wii.** The vendored `libwiidrc` lets a Wii app read a Wii
  U GamePad as an additional controller.


[Back to the README](../README.md)
