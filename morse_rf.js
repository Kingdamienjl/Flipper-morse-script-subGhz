// Morse Code RF Transmitter - Flipper Zero (Momentum)
// Save to: SD Card/apps/Scripts/morse_rf.js
// Uses event_loop GUI API (post-Oct-2024 Momentum firmware)

// Manual lowercase since to_lower_case() is not in this runtime
function lower(s) {
    let out = "";
    for (let i = 0; i < s.length; i++) {
        let c = s.at(i);
        if (c >= 65 && c <= 90) {
            out = out + chr(c + 32);
        } else {
            out = out + chr(c);
        }
    }
    return out;
}

let eventLoop = require("event_loop");
let gui = require("gui");
let submenuView = require("gui/submenu");
let textInputView = require("gui/text_input");
let storage = require("storage");
let subghz = require("subghz");
let notify = require("notification");

// Morse code lookup table
let alphabet = {
    'a': '.-',    'b': '-...',  'c': '-.-.', 'd': '-..',
    'e': '.',     'f': '..-.',  'g': '--.',  'h': '....',
    'i': '..',    'j': '.---',  'k': '-.-',  'l': '.-..',
    'm': '--',    'n': '-.',    'o': '---',  'p': '.--.',
    'q': '--.-',  'r': '.-.',   's': '...',  't': '-',
    'u': '..-',   'v': '...-',  'w': '.--',  'x': '-..-',
    'y': '-.--',  'z': '--..',  ' ': ' ',
    '1': '.----', '2': '..---', '3': '...--', '4': '....-',
    '5': '.....', '6': '-....', '7': '--...', '8': '---..',
    '9': '----.', '0': '-----',
};

// Config file paths (SD card apps_data)
let pfx = "/ext/apps_data/morse_rf_";
let cfg_freq = pfx + "frequency.txt";
let cfg_msg = pfx + "message.txt";
let cfg_delay = pfx + "delay.txt";
let cfg_repeat = pfx + "repeat.txt";
let sub_file = pfx + "signal.sub";

function read_file(path, fallback) {
    if (!storage.fileExists(path)) {
        write_file(path, fallback);
        return fallback;
    }
    let file = storage.openFile(path, "r", "open_existing");
    if (file === undefined) return fallback;
    let info = storage.stat(path);
    let sz = 64;
    if (info !== undefined) sz = info.size;
    if (sz === 0) { file.close(); return fallback; }
    let data = file.read("ascii", sz);
    file.close();
    if (data === undefined) return fallback;
    return data;
}

function write_file(path, text) {
    let file = storage.openFile(path, "w", "create_always");
    if (file === undefined) { print("ERR write: " + path); return; }
    file.write(text);
    file.close();
}

// Load saved config or defaults
let def_freq = read_file(cfg_freq, "433920000");
let def_msg = read_file(cfg_msg, "sos");
let def_delay = read_file(cfg_delay, "200");
let def_repeat = read_file(cfg_repeat, "0");

// Number to string - manual conversion for mJS strict mode
function num2str(n) {
    if (n === 0) { return "0"; }
    let neg = false;
    if (n < 0) { neg = true; n = -n; }
    let s = "";
    while (n > 0) {
        let d = n % 10;
        s = chr(48 + d) + s;
        n = (n - d) / 10;
    }
    if (neg) { s = "-" + s; }
    return s;
}

// Build a Flipper SubGhz RAW .sub file from morse-encoded message
// Timing: dot=1unit, dash=3units, intra-char gap=1unit,
//         inter-char gap=3units, word gap=7units
function build_sub_file(message, freq_hz, delay_ms, repeat_count) {
    let unit = delay_ms * 1000; // microseconds
    let raw = [];

    for (let r = 0; r <= repeat_count; r++) {
        for (let ci = 0; ci < message.length; ci++) {
            let ch = chr(message.at(ci));
            let morse = alphabet[ch];
            if (morse === undefined) continue;

            if (ch === ' ') {
                // Word gap: extend existing inter-char gap (3u) to word gap (7u)
                // Need to add 4 more units to reach 7 total
                if (raw.length > 0 && raw[raw.length - 1] < 0) {
                    raw[raw.length - 1] = raw[raw.length - 1] - (unit * 4);
                } else {
                    raw.push(-(unit * 7));
                }
                continue;
            }

            // Encode each dot/dash
            for (let mi = 0; mi < morse.length; mi++) {
                let sym = chr(morse.at(mi));
                if (sym === '.') {
                    raw.push(unit);       // carrier on for 1 unit
                    raw.push(-unit);      // carrier off for 1 unit (intra-char gap)
                } else if (sym === '-') {
                    raw.push(unit * 3);   // carrier on for 3 units
                    raw.push(-unit);      // carrier off for 1 unit (intra-char gap)
                }
            }

            // Extend trailing intra-char gap (1u) to inter-char gap (3u)
            if (raw.length > 0 && raw[raw.length - 1] < 0) {
                raw[raw.length - 1] = raw[raw.length - 1] - (unit * 2);
            }
        }

        // Between repeats: add word-length gap
        if (r < repeat_count && raw.length > 0) {
            if (raw[raw.length - 1] < 0) {
                // Already have 3u inter-char gap, extend to 7u
                raw[raw.length - 1] = raw[raw.length - 1] - (unit * 4);
            } else {
                raw.push(-(unit * 7));
            }
        }
    }

    // Ensure file ends with carrier off
    if (raw.length > 0 && raw[raw.length - 1] > 0) {
        raw.push(-unit);
    }

    // Build .sub file content
    let out = "Filetype: Flipper SubGhz RAW File\n";
    out = out + "Version: 1\n";
    out = out + "Frequency: " + num2str(freq_hz) + "\n";
    out = out + "Preset: FuriHalSubGhzPresetOok650Async\n";
    out = out + "Protocol: RAW\n";

    let line = "RAW_Data:";
    let count = 0;
    for (let i = 0; i < raw.length; i++) {
        line = line + " " + num2str(raw[i]);
        count++;
        if (count >= 500) {
            out = out + line + "\n";
            line = "RAW_Data:";
            count = 0;
        }
    }
    if (count > 0) { out = out + line + "\n"; }

    return out;
}

let radio_setup = false;

function send_morse() {
    let freq_s = read_file(cfg_freq, "433920000");
    let message = read_file(cfg_msg, "sos");
    let delay_s = read_file(cfg_delay, "60");
    let repeat_s = read_file(cfg_repeat, "0");

    print("Msg: " + message + " @ " + freq_s + "Hz");

    let content = build_sub_file(message, parseInt(freq_s), parseInt(delay_s), parseInt(repeat_s));
    write_file(sub_file, content);

    if (!radio_setup) {
        subghz.setup();
        radio_setup = true;
    }
    notify.blink("green", "short");
    print("TX...");

    let result = subghz.transmitFile(sub_file);
    if (result === false) {
        print("TX FAIL");
        notify.error();
    } else {
        print("TX OK");
        notify.success();
    }
}

// --- GUI Setup ---
// submenuView.makeWith(props, children) — children is the 2nd arg (array of strings)
let views = {
    mainMenu: submenuView.makeWith(
        { header: "Morse RF TX" },
        ["Send signal", "Set frequency", "Set message", "Set delay (ms)", "Toggle repeat"]
    ),
    freqInput: textInputView.makeWith({
        header: "Frequency (Hz)",
        defaultText: def_freq,
        defaultTextClear: true,
        minLength: 6,
        maxLength: 15,
    }),
    msgInput: textInputView.makeWith({
        header: "Message",
        defaultText: def_msg,
        defaultTextClear: true,
        minLength: 1,
        maxLength: 100,
    }),
    delayInput: textInputView.makeWith({
        header: "Delay ms",
        defaultText: def_delay,
        defaultTextClear: true,
        minLength: 1,
        maxLength: 5,
    }),
    repeatInput: textInputView.makeWith({
        header: "0=once 1=repeat",
        defaultText: def_repeat,
        defaultTextClear: true,
        minLength: 1,
        maxLength: 3,
    }),
};

// Main menu handler — no closures in mJS, pass refs as extra args
eventLoop.subscribe(views.mainMenu.chosen, function (_sub, index, gui, views, eventLoop) {
    if (index === 0) {
        send_morse();
        gui.viewDispatcher.switchTo(views.mainMenu);
    } else if (index === 1) {
        gui.viewDispatcher.switchTo(views.freqInput);
    } else if (index === 2) {
        gui.viewDispatcher.switchTo(views.msgInput);
    } else if (index === 3) {
        gui.viewDispatcher.switchTo(views.delayInput);
    } else if (index === 4) {
        gui.viewDispatcher.switchTo(views.repeatInput);
    }
}, gui, views, eventLoop);

// Text input handlers — save to config file and return to menu
eventLoop.subscribe(views.freqInput.input, function (_sub, text, gui, views) {
    write_file(cfg_freq, text);
    notify.success();
    gui.viewDispatcher.switchTo(views.mainMenu);
}, gui, views);

eventLoop.subscribe(views.msgInput.input, function (_sub, text, gui, views) {
    write_file(cfg_msg, lower(text));
    notify.success();
    gui.viewDispatcher.switchTo(views.mainMenu);
}, gui, views);

eventLoop.subscribe(views.delayInput.input, function (_sub, text, gui, views) {
    write_file(cfg_delay, text);
    notify.success();
    gui.viewDispatcher.switchTo(views.mainMenu);
}, gui, views);

eventLoop.subscribe(views.repeatInput.input, function (_sub, text, gui, views) {
    write_file(cfg_repeat, text);
    notify.success();
    gui.viewDispatcher.switchTo(views.mainMenu);
}, gui, views);

// Back button returns to main menu (or exits if already there)
eventLoop.subscribe(gui.viewDispatcher.navigation, function (_sub, _, gui, views, eventLoop) {
    gui.viewDispatcher.switchTo(views.mainMenu);
}, gui, views, eventLoop);

// Start the app
gui.viewDispatcher.switchTo(views.mainMenu);
eventLoop.run();
