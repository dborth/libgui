# Audio

`AudioDriver` provides fixed one-shot PCM voices and exactly one streamed OGG
track at a time. `GuiSound` wraps both: a looping sound is treated as music
and takes the single stream, and a non-looping sound is a sound effect on a
voice. `GuiSound::setDefaultVolume(VOLUME_TYPE::MUSIC, ...)` and
`VOLUME_TYPE::SFX` set independent global volumes (0-100) for the two
categories; changing the music volume takes effect immediately on the track
that is playing. OGG decoding is done by the platform-independent
`GuiSoundOggPlayer` (Tremor) on a background thread, on every platform. On
Wii U, sound is mixed to both the TV and the GamePad.


[Back to the README](../README.md)
