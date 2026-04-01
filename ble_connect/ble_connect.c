/**
 * BLE Connect - Flipper Zero Native App
 *
 * Skeleton .fap for BLE scanning, connection, and MAC spoofing.
 * Compile with: ufbt (from this directory)
 * Install: copy dist/ble_connect.fap to SD Card/apps/Bluetooth/
 *
 * Requires Momentum or compatible firmware.
 */

#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_bt.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/popup.h>

// View IDs
typedef enum {
    BleConnectViewMenu,
    BleConnectViewPopup,
} BleConnectView;

// Menu item IDs
typedef enum {
    BleConnectMenuScan,
    BleConnectMenuConnect,
    BleConnectMenuSpoofMac,
    BleConnectMenuSpamPair,
    BleConnectMenuAbout,
} BleConnectMenuItem;

// App state
typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    Submenu* submenu;
    Popup* popup;
} BleConnectApp;

// --- Stub implementations ---
// These are placeholders for future BLE functionality.
// Each shows a popup describing what will be implemented.

static void ble_connect_show_popup(BleConnectApp* app, const char* header, const char* text) {
    popup_set_header(app->popup, header, 64, 10, AlignCenter, AlignTop);
    popup_set_text(app->popup, text, 64, 40, AlignCenter, AlignCenter);
    view_dispatcher_switch_to_view(app->view_dispatcher, BleConnectViewPopup);
}

/**
 * TODO: BLE Scan Implementation
 *
 * Use furi_hal_bt APIs to scan for nearby BLE devices.
 * The STM32WB55 supports BLE 5.4 scanning via the M0+ core.
 *
 * Approach:
 * 1. Stop current BLE app: furi_hal_bt_change_app(FuriHalBtProfileNone, ...)
 * 2. Configure scan parameters via HCI commands
 * 3. Start active/passive scan
 * 4. Collect advertisement reports (device name, MAC, RSSI)
 * 5. Display results in a list view
 * 6. Restart BLE app when done: furi_hal_bt_change_app(FuriHalBtProfileSerial, ...)
 *
 * Key headers: <furi_hal_bt.h>, <bt/bt_service/bt.h>
 * Reference: github.com/chris-bc/wendigo (Flipper BLE scanner)
 */
static void ble_connect_scan(BleConnectApp* app) {
    ble_connect_show_popup(
        app,
        "BLE Scan",
        "Not yet implemented.\n"
        "Will scan for nearby\n"
        "BLE devices and show\n"
        "name, MAC, RSSI.");
}

/**
 * TODO: BLE Connect Implementation
 *
 * Initiate a BLE connection to a discovered device.
 *
 * Approach:
 * 1. Select target from scan results
 * 2. Use HCI LE Create Connection command
 * 3. Handle connection events (success/failure/timeout)
 * 4. Discover GATT services on connected device
 * 5. Display connection status and available services
 *
 * Note: Most devices require pairing/bonding after connection.
 * The STM32WB55 BLE stack supports SMP (Security Manager Protocol)
 * for Just Works, Numeric Comparison, and Passkey pairing.
 */
static void ble_connect_connect(BleConnectApp* app) {
    ble_connect_show_popup(
        app,
        "BLE Connect",
        "Not yet implemented.\n"
        "Will connect to a\n"
        "selected BLE device\n"
        "and enumerate services.");
}

/**
 * TODO: MAC Spoofing Implementation
 *
 * Change the Flipper's BLE MAC address to impersonate another device.
 *
 * Approach:
 * 1. Store original MAC for restoration
 * 2. Modify BLE address via:
 *    - furi_hal_bt API (if exposed), OR
 *    - Direct HCI command: HCI_LE_Set_Random_Address (0x2005)
 *    - Gap.c modification for persistent spoofing
 * 3. Restart BLE advertising with new address
 *
 * Limitations:
 * - MAC spoofing alone does NOT bypass BLE bonding
 * - Bonded devices use IRK (Identity Resolving Key) for verification
 * - Only works against devices using Just Works pairing or no auth
 *
 * Reference: salmg.net/2023/02/03/flipper-zero-changing-bluetooth-mac-address/
 */
static void ble_connect_spoof_mac(BleConnectApp* app) {
    ble_connect_show_popup(
        app,
        "MAC Spoof",
        "Not yet implemented.\n"
        "Will change BLE MAC\n"
        "to impersonate another\n"
        "device address.");
}

/**
 * TODO: Pairing Request Spam Implementation
 *
 * Rapidly send BLE connection/pairing requests to a target device.
 *
 * Approach:
 * 1. Select target MAC from scan results
 * 2. Loop: connect -> send pairing request -> disconnect -> repeat
 * 3. Some devices auto-accept after repeated requests (rare)
 * 4. Most devices will show repeated pairing dialogs
 *
 * This differs from BLE beacon spam (ble_spam.js):
 * - Beacon spam = broadcast fake advertisements (one-to-many)
 * - Pairing spam = targeted connection attempts (one-to-one)
 *
 * Note: Actual auto-pairing bypass is not feasible without the
 * original bonding keys. This is a stress-test / research tool.
 */
static void ble_connect_spam_pair(BleConnectApp* app) {
    ble_connect_show_popup(
        app,
        "Pair Spam",
        "Not yet implemented.\n"
        "Will send repeated\n"
        "pairing requests to\n"
        "a target device.");
}

static void ble_connect_about(BleConnectApp* app) {
    ble_connect_show_popup(
        app,
        "BLE Connect v0.1",
        "BLE scanner, connector\n"
        "and MAC spoofer for\n"
        "Flipper Zero.\n"
        "github: kingdamienjl");
}

// --- Menu callback ---
static void ble_connect_menu_callback(void* context, uint32_t index) {
    BleConnectApp* app = context;
    switch(index) {
    case BleConnectMenuScan:
        ble_connect_scan(app);
        break;
    case BleConnectMenuConnect:
        ble_connect_connect(app);
        break;
    case BleConnectMenuSpoofMac:
        ble_connect_spoof_mac(app);
        break;
    case BleConnectMenuSpamPair:
        ble_connect_spam_pair(app);
        break;
    case BleConnectMenuAbout:
        ble_connect_about(app);
        break;
    }
}

// --- Navigation callbacks ---
static uint32_t ble_connect_nav_menu(void* context) {
    UNUSED(context);
    return BleConnectViewMenu;
}

static uint32_t ble_connect_nav_exit(void* context) {
    UNUSED(context);
    return VIEW_NONE;
}

// --- App lifecycle ---
static BleConnectApp* ble_connect_app_alloc(void) {
    BleConnectApp* app = malloc(sizeof(BleConnectApp));

    app->gui = furi_record_open(RECORD_GUI);
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    // Submenu
    app->submenu = submenu_alloc();
    submenu_set_header(app->submenu, "BLE Connect");
    submenu_add_item(app->submenu, "Scan Devices", BleConnectMenuScan, ble_connect_menu_callback, app);
    submenu_add_item(app->submenu, "Connect", BleConnectMenuConnect, ble_connect_menu_callback, app);
    submenu_add_item(app->submenu, "Spoof MAC", BleConnectMenuSpoofMac, ble_connect_menu_callback, app);
    submenu_add_item(app->submenu, "Spam Pairing", BleConnectMenuSpamPair, ble_connect_menu_callback, app);
    submenu_add_item(app->submenu, "About", BleConnectMenuAbout, ble_connect_menu_callback, app);
    view_set_previous_callback(submenu_get_view(app->submenu), ble_connect_nav_exit);
    view_dispatcher_add_view(app->view_dispatcher, BleConnectViewMenu, submenu_get_view(app->submenu));

    // Popup (for stub messages)
    app->popup = popup_alloc();
    popup_set_timeout(app->popup, 3000);
    popup_enable_timeout(app->popup);
    view_set_previous_callback(popup_get_view(app->popup), ble_connect_nav_menu);
    view_dispatcher_add_view(app->view_dispatcher, BleConnectViewPopup, popup_get_view(app->popup));

    return app;
}

static void ble_connect_app_free(BleConnectApp* app) {
    view_dispatcher_remove_view(app->view_dispatcher, BleConnectViewMenu);
    view_dispatcher_remove_view(app->view_dispatcher, BleConnectViewPopup);
    submenu_free(app->submenu);
    popup_free(app->popup);
    view_dispatcher_free(app->view_dispatcher);
    furi_record_close(RECORD_GUI);
    free(app);
}

// --- Entry point ---
int32_t ble_connect_app(void* p) {
    UNUSED(p);

    BleConnectApp* app = ble_connect_app_alloc();

    view_dispatcher_switch_to_view(app->view_dispatcher, BleConnectViewMenu);
    view_dispatcher_run(app->view_dispatcher);

    ble_connect_app_free(app);
    return 0;
}
