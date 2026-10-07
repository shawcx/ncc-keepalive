// ncc-keepalive: boot an OpenNCC camera and keep it alive for V4L2 clients.
//
// On OpenNCC Frame firmware (MV03.x) the camera is a UVC webcam, but it only
// stays booted while a process holds it open through libOpenNCC_native. This
// program does exactly that and nothing else: it loads the firmware, selects
// 1080p, enables auto exposure, then sleeps. It never opens /dev/openncc, so
// VLC, ffmpeg, OpenCV or any other V4L2 program can use the camera meanwhile.
//
// Usage:  ncc-keepalive [-q]      (-q: no status output)
// Stop:   Ctrl-C or SIGTERM. The camera resets to its bootloader shortly after.

#include <csignal>
#include <cstdio>
#include <cstring>
#include <unistd.h>

#include "cameraCtrl.h"
#include "native_vpu_api.h"

namespace {

constexpr const char* kFirmware = "/usr/lib/openncc/OpenNcc.mvcmd";
constexpr int kDevice = 0;         // first camera found
constexpr int kSensorMode1080p = 0; // mode 1 (4K) crashes firmware MV03.01.01

volatile sig_atomic_t g_running = 1;
bool g_quiet = false;

void onSignal(int) { g_running = 0; }

void info(const char* msg) {
    if (!g_quiet) {
        std::printf("ncc-keepalive: %s\n", msg);
        std::fflush(stdout);
    }
}

int fail(const char* msg) {
    std::fprintf(stderr, "ncc-keepalive: %s\n", msg);
    return 1;
}

}  // namespace

int main(int argc, char** argv) {
    g_quiet = argc > 1 && std::strcmp(argv[1], "-q") == 0;

    const int found = ncc_dev_number_get();
    if (found <= 0) {
        return fail("no unbooted OpenNCC camera found "
                    "(unplugged, or another program already has it)");
    }

    if (ncc_dev_init(const_cast<char*>(kFirmware), found) <= 0) {
        return fail("firmware upload failed");
    }

    device_ctrl_select_sensor_mode(kDevice, kSensorMode1080p);

    if (ncc_dev_start(kDevice) != 0) {
        return fail("device start failed");
    }

    camera_ctrl_ae_enabled(kDevice);

    char version[64] = {0};
    device_ctrl_get_fw_version(kDevice, version, sizeof version);
    if (!g_quiet) {
        std::printf("ncc-keepalive: firmware %s, /dev/openncc ready, Ctrl-C to stop\n", version);
        std::fflush(stdout);
    }

    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);
    while (g_running) {
        sleep(1);
    }

    info("stopping, camera will reset");
    return 0;
}
