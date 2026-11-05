<p align="center">
  <img src="assets/icon.png" width="96" alt="TouchTamer icon">
</p>

<h1 align="center">TouchTamer</h1>

<p align="center">
  Touchpad gestures for Windows: change brightness and volume with a swipe, and move windows with five fingers.
</p>

<p align="center">
  <b>English</b> · <a href="README.id.md">Bahasa Indonesia</a>
</p>

---

TouchTamer is a small background program for Windows laptops. Its gestures work the same in every application, and on the empty desktop too.

| What you want | What you do |
|---|---|
| Change **brightness** | Hold `Ctrl`+`Shift`, then swipe up or down with **2 fingers** on the **left half** of the touchpad |
| Change **volume** | Hold `Ctrl`+`Shift`, then swipe up or down with **2 fingers** on the **right half** of the touchpad |
| **Move a window** | Put the cursor over the window, then swipe with **5 fingers** |
| Change **window size** | `Alt`+`Up` and `Alt`+`Down` |
| Change **window width** | `Alt`+`Right` and `Alt`+`Left` |

It is one small file of about 130 KB. It uses about 2.6 MB of its own memory and close to 0% CPU while idle, needs no administrator rights, and never connects to the internet.

## Contents

- [Will it work on my laptop?](#will-it-work-on-my-laptop)
- [Install](#install)
- [How to use it](#how-to-use-it)
- [Change the settings](#change-the-settings)
- [Pause, restart, or turn off at startup](#pause-restart-or-turn-off-at-startup)
- [Uninstall](#uninstall)
- [Troubleshooting](#troubleshooting)
- [Known limits](#known-limits)
- [Privacy](#privacy)
- [Build from source](#build-from-source)
- [License](#license)

## Will it work on my laptop?

> **Please note:** TouchTamer has been tested on only one laptop so far, a Lenovo IdeaPad Flex 5 14ITL05 with Windows 11. Other laptops should work if they meet the requirements below, but this has not been proven yet. If you try it, please [open an issue](https://github.com/falahdrrhmn1/TouchTamer/issues) and say how it went.

| You need | For | How to check |
|---|---|---|
| Windows 10 or 11, 64-bit | Everything | Press `Win`+`Pause`, or open **Settings → System → About**. Windows 10 has not been tested yet |
| A **Precision Touchpad** | All gestures | See below |
| A touchpad that reads **5 fingers** | Moving windows | The installer tells you on Windows 11 24H2 or later |
| A **built-in laptop screen** | Brightness | External monitors are not supported. The gesture simply does nothing there |

The `Alt`+arrow hotkeys work without a touchpad.

### Check your touchpad

- **Windows 11:** open **Settings → Bluetooth & devices → Touchpad**. You have a Precision Touchpad if you see the sections **Three-finger gestures** and **Four-finger gestures**.
- **Windows 10:** open **Settings → Devices → Touchpad**. You have one if the page says **Your PC has a precision touchpad**.

## Install

### 1. Download

1. Open <https://github.com/falahdrrhmn1/TouchTamer>.
2. Click the green **Code** button, then **Download ZIP**.

Or use this direct link: [download the ZIP](https://github.com/falahdrrhmn1/TouchTamer/archive/refs/heads/main.zip).

### 2. Extract the ZIP

1. Open your **Downloads** folder.
2. Right-click `TouchTamer-main.zip`, then choose **Extract All…** and **Extract**.
3. Open the new `TouchTamer-main` folder. You should see `install.cmd` in it.

Do not run the installer from inside the ZIP without extracting it. It cannot find its files that way.

### 3. Run the installer

Double-click **`install.cmd`**.

If a blue window says **Windows protected your PC**, click **More info**, then **Run anyway**. Windows shows this for programs without a paid digital signature, and TouchTamer does not have one. [Check the file yourself](#check-the-file-yourself) if you want to be sure.

The installer does the following:

- Checks your Windows version and touchpad.
- Copies the program to `%LOCALAPPDATA%\Programs\TouchTamer`.
- Adds a shortcut so TouchTamer starts when you sign in.
- Starts TouchTamer right away.

It does not ask for administrator rights. A black window shows the results. Press any key to close it.

### 4. Try it

TouchTamer has no window or tray icon. It runs in the background, and you only see a small panel when a value changes.

1. Hold `Ctrl`+`Shift`.
2. Put 2 fingers on the left half of the touchpad and swipe up.
3. A panel with the brightness percentage appears at the bottom of the screen.

### 5. Keep the folder

Keep the extracted folder if you want to use `uninstall.cmd` later. You can also delete it, then download it again whenever you need it, or [remove TouchTamer by hand](#remove-it-by-hand).

### Check the file yourself

The expected fingerprint is in [`bin/TouchTamer.exe.sha256`](bin/TouchTamer.exe.sha256). To compare, open PowerShell in the extracted folder and run:

```powershell
Get-FileHash .\bin\TouchTamer.exe -Algorithm SHA256
```

The two values must match. You can also read the source code in [`src`](src) and [build the program yourself](#build-from-source).

## How to use it

### Brightness and volume

1. Hold `Ctrl`+`Shift`, and keep holding them during the swipe.
2. Put 2 fingers on the touchpad:
   - **Left half** for brightness.
   - **Right half** for volume.
3. Swipe **up to increase** or **down to decrease**.

Good to know:

- The side is chosen when the swipe starts. Your fingers can drift across the middle after that.
- A swipe across the full height of the touchpad changes the value by 60%. You can [change this](#change-the-settings).
- Normal two-finger scrolling, without `Ctrl`+`Shift`, is not affected.

### Move a window

1. Put the cursor over the window you want to move.
2. Put **5 fingers** on the touchpad.
3. Slide them in any direction. The window and the cursor move together.
4. Lift your fingers to finish.

Good to know:

- **Long moves:** lift your fingers, put them back, and keep going. You will still be moving the same window, because the cursor is still over it.
- **Full-screen windows:** the window first returns to its normal size, then follows your fingers. This is the same as dragging a maximized window by its title bar.
- **Windows it ignores:** the desktop, the taskbar, and borderless full screen such as games, videos, and browsers in `F11` mode.
- **Windows cannot get lost:** the top of the window always stays reachable on the screen.
- **Three and four fingers:** your Windows gestures keep working, because TouchTamer only reacts to five fingers.

### Window size: Alt+Up and Alt+Down

These change the active window, stepping through these sizes:

| Level | Size |
|---|---|
| Highest | Real full screen (maximized) |
| 2 | 95% of the screen, centered |
| 3 | 90%, centered |
| 4 | 75%, centered |
| Lowest | 40%, centered |

- `Alt`+`Up` makes the window bigger. At the highest level, it goes back to the lowest level.
- `Alt`+`Down` makes the window smaller. At the lowest level, it stays there.
- The percentages leave out the taskbar, so a window is never hidden behind it.

### Window width: Alt+Right and Alt+Left

- `Alt`+`Right` widens the active window by 40 pixels, up to the edge of the screen.
- `Alt`+`Left` narrows it by 40 pixels, down to at least 300 pixels.
- On a full-screen window, `Alt`+`Left` leaves full screen first, and `Alt`+`Right` does nothing.

> **Note:** in browsers and File Explorer, `Alt`+`Left` and `Alt`+`Right` normally mean Back and Forward, and in File Explorer `Alt`+`Up` means "up one folder". TouchTamer takes these keys over while it runs. If you want the originals back, [turn the hotkeys off](#change-the-settings).

## Change the settings

1. Press `Win`+`R`, paste the line below, and press `Enter`:
   ```
   %LOCALAPPDATA%\Programs\TouchTamer
   ```
2. Open `TouchTamer.ini` with Notepad.
3. Change a value and save the file.
4. [Restart TouchTamer](#pause-restart-or-turn-off-at-startup).

| Setting | Default | What it does |
|---|---|---|
| `level_gesture` | `1` | Brightness and volume gesture. `0` turns it off |
| `gesture_mod` | `ctrl+shift` | Keys to hold during that gesture. Use `ctrl`, `alt`, `shift`, or a combination |
| `full_height_pct` | `60` | Percent change for a swipe across the full touchpad height |
| `move_gesture` | `1` | Five-finger window move. `0` turns it off |
| `move_speed_pct` | `150` | Window speed. Bigger is faster |
| `size_hotkeys` | `1` | `Alt`+`Up` and `Alt`+`Down`. `0` turns them off |
| `width_hotkeys` | `1` | `Alt`+`Right` and `Alt`+`Left`. `0` gives Back and Forward back to your browser |
| `hide_after_ms` | `850` | How long the panel stays visible, in milliseconds |
| `alpha` | `245` | Panel opacity, from 40 to 255 |
| `volume_native_osd` | `0` | `1` shows the Windows volume overlay instead of the panel |
| `debug` | `0` | `1` writes a diagnostic log, `TouchTamer.log`, next to the program |

Installing a newer version keeps your `TouchTamer.ini`.

## Pause, restart, or turn off at startup

**Stop it for now:** open **Task Manager** (`Ctrl`+`Shift`+`Esc`), find **TouchTamer touchpad gestures**, then click **End task**.

**Start it again:** open the install folder as shown above, then double-click `TouchTamer.exe`.

**Restart it:** do both of the above. TouchTamer reads its settings when it starts.

**Do not start it with Windows:** open **Task Manager → Startup apps**, select **TouchTamer**, then click **Disable**.

## Uninstall

Double-click **`uninstall.cmd`** in the extracted folder. If you no longer have the folder, [download it again](#1-download).

It stops TouchTamer, removes the startup shortcut, and deletes the program files. Your brightness and volume stay at their current levels.

### Remove it by hand

1. End TouchTamer in Task Manager.
2. Press `Win`+`R`, type `shell:startup`, and press `Enter`. Delete the `TouchTamer` shortcut.
3. Press `Win`+`R`, type `%LOCALAPPDATA%\Programs`, and press `Enter`. Delete the `TouchTamer` folder.

## Troubleshooting

| Problem | Try this |
|---|---|
| Nothing happens at all | Check that **TouchTamer touchpad gestures** is in Task Manager. If not, start it from the install folder |
| Gestures do nothing, but `Alt`+arrows work | [Check your touchpad](#check-your-touchpad). TouchTamer needs a Precision Touchpad |
| Brightness does not change | Brightness works only on built-in laptop screens. Volume should still work |
| Volume or brightness changes twice as fast | Another copy may be running, or another tool is reacting too. Close it in Task Manager |
| A page zooms or scrolls during the gesture | Windows still sends the scroll to some applications. Try a different `gesture_mod`, such as `ctrl+alt` |
| Five fingers do nothing | Your touchpad may read fewer than 5 fingers, and the installer warns about this on Windows 11 24H2 or later. Put all five fingers down before moving them |
| A window will not move or resize | Windows blocks this for windows running as administrator, such as Task Manager |
| `Alt`+arrows do not work | Another program may own those keys. Set `debug=1`, restart, and look for `hotkey ... not available` in `TouchTamer.log` |
| I want Back and Forward in my browser | Set `width_hotkeys=0` and restart |
| A setting has no effect | Save the file, then restart TouchTamer. It reads settings only when it starts |
| The installer says files are missing | Extract the whole ZIP first, then run `install.cmd` from the extracted folder |
| Windows or antivirus blocks it | The program has no paid signature. [Check the file](#check-the-file-yourself), then allow it if you trust it |

Still stuck? [Open an issue](https://github.com/falahdrrhmn1/TouchTamer/issues) with your laptop model, your Windows version, and what you tried. A `TouchTamer.log` helps too. Read it before sharing, because it can contain names of application window types.

## Known limits

- Tested on only one laptop so far, so other touchpads may behave differently.
- The panel always appears on the main screen.
- Windows running as administrator cannot be moved or resized. Running TouchTamer itself as administrator is not needed for anything else.
- Gestures do not work on the Windows sign-in and permission (UAC) screens.
- A palm resting on the touchpad can count as a finger.

## Privacy

- TouchTamer has no network code and sends nothing anywhere.
- It does not record what you type. It only checks whether `Ctrl`, `Alt`, and `Shift` are held during a gesture, and it registers the `Alt`+arrow keys.
- Finger positions are used for the gesture and then discarded.
- The log file is off by default. When turned on, it stays on your computer and lists gestures, values, and application window types, but not your typing or your document names.

## Build from source

You need a 64-bit MinGW-w64 compiler with `g++` and `windres` on `PATH`, such as [w64devkit](https://github.com/skeeto/w64devkit). Then run this in the repository folder:

```bat
build.cmd
```

This creates `bin\TouchTamer.exe`. To build the tests too:

```bat
build.cmd test
build\test_gesture.exe
python tests\e2e_windows.py
```

The end-to-end test opens its own empty window and moves the cursor for about a minute, so leave the keyboard and touchpad alone while it runs. It never touches your other windows.

| Path | Contents |
|---|---|
| `src/touchtamer.cpp` | The program |
| `src/gesture.h` | Gesture logic, separate from Windows so it can be tested |
| `tests/` | Unit tests and the end-to-end test |
| `scripts/` | Installer and uninstaller |
| `bin/` | The ready-to-use program and its default settings |

## License

[MIT](LICENSE). You can use, change, and share it freely. TouchTamer comes with no warranty.
