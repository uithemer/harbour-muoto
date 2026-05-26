---
layout: default
title: Sound pack guidelines (removed)
parent: Create theme packs
nav_order: 4
---

# Sound pack guidelines (removed)

{: .warning }
**Sound theming was removed in UI Themer 2.4.4.** Packs may still ship a `sound/` directory, but the engine **ignores** it. Do not publish sound-only packs for UI Themer. This page is kept so authors migrating from older Theme pack support documentation can find the former format.

## Former format

Create WAV audio files: **PCM, 16-bit, 48000 Hz**.

Rename them as:

- `battery_empty.wav`
- `keyboard_letter.wav`
- `start_charging.wav`
- `battery_low.wav`
- `keyboard_option.wav`
- `sw_update.wav`
- `camera_shutter.wav`
- `positive_confirmation.wav`
- `toh_attach.wav`
- `close_app.wav`
- `pulldown_highlight.wav`
- `unlock_device.wav`
- `general_warning.wav`
- `pulldown_lock.wav`
- `video_record_start.wav`
- `jolla-related-message.wav`
- `push_gesture.wav`
- `video_record_stop.wav`

Then copy the files into the pack `sound/` directory.
