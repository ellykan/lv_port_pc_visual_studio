#include <Windows.h>

#include <LvglWindowsIconResource.h>

#include "app/App.h"
#include "hal/Clock.h"
#include "lvgl/lvgl.h"
#include "lvgl/examples/lv_examples.h"
#include "lvgl/demos/lv_demos.h"

// ---------------------------------------------------------------------------
// Dev keys (simulator only). Click the simulator window, then:
//   F1 battery 85%   F2 15% (low)   F3 4% (critical)   F4 60% charging
//   F5 water reminder   F6 morning letter   F7 lock (needs a PIN)
//   1 night (2:00)  2 morning (8:05)  3 day (12:00)  4 evening (18:00)
//   5 wind-down (0:15)
// ---------------------------------------------------------------------------

// True once per key press, and only while the simulator window is in front,
// so typing in Visual Studio doesn't trigger anything.
static bool devKey(int vk) {
    static bool down[256] = {};
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    bool now = pid == GetCurrentProcessId() && (GetAsyncKeyState(vk) & 0x8000);
    bool hit = now && !down[vk];
    down[vk] = now;
    return hit;
}

// Jump the simulator's clock to h:m today.
static void simTime(int h, int m) {
    LocalTime t;
    if (!hal::localTime(t)) return;
    t.hour = (uint8_t)h; t.minute = (uint8_t)m; t.second = 0;
    hal::setLocalTime(t);
}

static void handleDevKeys() {
    if (devKey(VK_F1)) App::devSetBattery(85, false);
    if (devKey(VK_F2)) App::devSetBattery(15, false);
    if (devKey(VK_F3)) App::devSetBattery(4, false);
    if (devKey(VK_F4)) App::devSetBattery(60, true);
    if (devKey(VK_F5)) App::devPost(Ev::ReminderDue, 0);
    if (devKey(VK_F6)) App::devPost(Ev::GoodMorning);
    if (devKey(VK_F7)) App::devLockNow();

    if (devKey('1')) simTime(2, 0);
    if (devKey('2')) simTime(8, 5);
    if (devKey('3')) simTime(12, 0);
    if (devKey('4')) simTime(18, 0);
    if (devKey('5')) simTime(0, 15);
}

int main()
{
    lv_init();

    /*
     * Optional workaround for users who wants UTF-8 console output.
     * If you don't want that behavior can comment them out.
     *
     * Suggested by jinsc123654.
     */
#if LV_TXT_ENC == LV_TXT_ENC_UTF8
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    int32_t zoom_level = 100; // TESTING: switch to 50 for approximate size of 1.75
    bool allow_dpi_override = false;
    bool simulator_mode = true;
    lv_display_t* display = lv_windows_create_display(
        L"LVGL Windows Simulator Display 1",
        466, //originally 800
        466, // originally 480
        zoom_level,
        allow_dpi_override,
        simulator_mode);
    if (!display)
    {
        return -1;
    }

    HWND window_handle = lv_windows_get_display_window_handle(display);
    if (!window_handle)
    {
        return -1;
    }

    HICON icon_handle = LoadIconW(
        GetModuleHandleW(NULL),
        MAKEINTRESOURCE(IDI_LVGL_WINDOWS));
    if (icon_handle)
    {
        SendMessageW(
            window_handle,
            WM_SETICON,
            TRUE,
            (LPARAM)icon_handle);
        SendMessageW(
            window_handle,
            WM_SETICON,
            FALSE,
            (LPARAM)icon_handle);
    }

    lv_indev_t* pointer_indev = lv_windows_acquire_pointer_indev(display);
    if (!pointer_indev)
    {
        return -1;
    }

    lv_indev_t* keypad_indev = lv_windows_acquire_keypad_indev(display);
    if (!keypad_indev)
    {
        return -1;
    }

    lv_indev_t* encoder_indev = lv_windows_acquire_encoder_indev(display);
    if (!encoder_indev)
    {
        return -1;
    }

    // Simulate the round 466x466 AMOLED: cover everything outside the circle in black.
    // Lives on the top layer, so it stays above every screen and doesn't take touches.
    lv_obj_t* mask = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(mask);
    lv_obj_set_size(mask, 466 + 200, 466 + 200);   // 100px ring on each side reaches the corners
    lv_obj_center(mask);
    lv_obj_set_style_radius(mask, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(mask, 100, 0);
    lv_obj_set_style_border_color(mask, lv_color_black(), 0);
    lv_obj_set_style_border_opa(mask, LV_OPA_COVER, 0);
    lv_obj_set_clickable(mask, false);

    App::begin(); // formerly lv_demo_widgets();
    //lv_demo_benchmark();

    while (1)
    {
        App::tick();
        handleDevKeys();
        uint32_t time_till_next = lv_timer_handler();
        lv_delay_ms(time_till_next);
    }

    return 0;
}
