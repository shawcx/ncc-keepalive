# ncc-keepalive

Use an [OpenNCC](https://github.com/EyecloudAi/openncc) camera (Intel Movidius
Myriad X, USB `03e7:2485`) as an ordinary Video4Linux webcam on Linux, so VLC,
ffmpeg, OBS, OpenCV and anything else that reads `/dev/video*` can use it.

## Why this exists

With the [OpenNCC Frame](https://github.com/EyecloudAi/opennccframe) firmware the
camera enumerates as a standard UVC device, "Eyecloud AI Camera". Two things stop
it being a plug-and-play webcam:

1. The camera has no flash. Its firmware is uploaded over USB every time it is
   plugged in, by Eyecloud's host library.
2. A watchdog in the firmware resets the camera to its bare bootloader a few
   seconds after that host library disconnects.

Eyecloud's own examples hold the V4L2 device exclusively, so nothing else can
read it while they run. `ncc-keepalive` boots the camera through the library,
enables auto exposure, and then just sleeps. It never opens the video node, which
leaves it free for whatever you want to run.

## Prerequisites

Install the OpenNCC Frame native SDK. This puts the host library, boot tool,
firmware and model zoo in `/usr/lib/openncc`, headers in
`/usr/include/openncc`, and a udev rule that makes the camera world-accessible
and creates the `/dev/openncc` symlink.

```sh
git clone https://github.com/EyecloudAi/opennccframe.git
cd opennccframe
sudo ./install.sh ubuntu
```

Build dependencies: a C++11 compiler, `make` and `libusb-1.0-0-dev`. `v4l-utils`
is optional but useful for inspecting controls.

```sh
sudo apt install build-essential libusb-1.0-0-dev v4l-utils
```

## Build

```sh
make
```

## Use

Start the keepalive, then open the camera in any V4L2 program:

```sh
./ncc-keepalive &
vlc v4l2:///dev/openncc:chroma=I420:width=1920:height=1080
ffplay -f v4l2 -i /dev/openncc
ffmpeg -f v4l2 -i /dev/openncc -frames:v 1 snapshot.png
```

Stop it with Ctrl-C or `pkill ncc-keepalive`. The camera resets to its
bootloader a few seconds later and `/dev/openncc` disappears. Pass `-q` to
silence the program's own status lines; the library's boot log still goes to
stderr and can be dropped with `2>/dev/null`.

To keep the camera up whenever you are logged in, install the user service:

```sh
mkdir -p ~/.config/systemd/user
sed "s|@BIN@|$PWD/ncc-keepalive|" ncc-keepalive.service > ~/.config/systemd/user/ncc-keepalive.service
systemctl --user enable --now ncc-keepalive
```

## Camera behaviour worth knowing

- **Plug it in the right way up.** The Type-C port only links at USB 3 in one
  orientation. Reversed it enumerates at USB 2.0, boots, and then sends no
  frames. `lsusb -t` or `dmesg` shows whether you got SuperSpeed.
- **Output is 1920x1080 YUV 4:2:0 only.** The sensor reports a 3840x2160 mode,
  but selecting it crashes firmware MV03.01.01, and the header for the mode
  selector says "Currently only support 1080P".
- **White balance is automatic and locked.** The UVC controls for it are
  read-only, and the SDK's own white-balance call times out and hangs the
  camera. Brightness, contrast, saturation, hue, gamma, sharpness and backlight
  compensation are writable through `v4l2-ctl -d /dev/openncc`.
- **The UVC descriptor advertises 1 to 15 fps,** but the sensor runs at 30 fps and
  V4L2 clients receive frames at that rate.
- **Unplugging mid-stream leaves clients stuck** on a dead device. Restart the
  keepalive after reconnecting. If the camera stops enumerating at all after a
  crash ("device not accepting address" in `dmesg`), unplug it for a few seconds.
- **The camera's AI engine is idle** in this setup. Running models on the Myriad X
  needs Eyecloud's native API or GStreamer plugins instead of, or alongside, this
  program.

## Tested with

OpenNCC USB camera with SC8238 sensor, firmware MV03.01.01 from opennccframe,
Linux Mint 22.3 (Ubuntu 24.04 base) with kernel 7.0, VLC 3.0.20 and ffmpeg 6.1.
