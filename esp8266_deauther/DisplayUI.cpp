/* This software is licensed under the MIT License: https://github.com/spacehuhntech/esp8266_deauther */

#include "DisplayUI.h"

#include "settings.h"

// ===== adjustable ===== //
void DisplayUI::configInit() {
    // initialize display
    display.init();

    /*
       In case of a compiler (conversion char/uint8_t) error,
       make sure to have version 4 of the display library installed
       https://github.com/ThingPulse/esp8266-oled-ssd1306/releases/tag/4.0.0
     */
    display.setFont(DejaVu_Sans_Mono_12);

    display.setContrast(255);

    if (FLIP_DIPLAY) display.flipScreenVertically();

    display.clear();
    display.display();
}

void DisplayUI::configOn() {
    display.displayOn();
}

void DisplayUI::configOff() {
    display.displayOff();
}

void DisplayUI::updatePrefix() {
    display.clear();
}

void DisplayUI::updateSuffix() {
    display.display();
}

void DisplayUI::drawString(int x, int y, String str) {
    display.drawString(x, y, replaceUtf8(str, String(QUESTIONMARK)));
}

void DisplayUI::drawString(int row, String str) {
    drawString(0, row * lineHeight, str);
}

void DisplayUI::drawLine(int x1, int y1, int x2, int y2) {
    display.drawLine(x1, y1, x2, y2);
}

// ====================== //


DisplayUI::DisplayUI() {}

DisplayUI::~DisplayUI() {}


void DisplayUI::setup() {
    configInit();
    setupButtons();
    buttonTime = currentTime;

#ifdef RTC_DS3231
    bool h12;
    bool PM_time;
    clock.setClockMode(false);
    clockHour   = clock.getHour(h12, PM_time);
    clockMinute = clock.getMinute();
#else // ifdef RTC_DS3231
    clockHour   = random(12);
    clockMinute = random(60);
#endif // ifdef RTC_DS3231

    // ===== MENUS ===== //

    // MAIN MENU
    createMenu(&mainMenu, NULL, [this]() {
        addMenuNode(&mainMenu, D_SCAN, &scanMenu, ICON_scan);       /// SCAN
        addMenuNode(&mainMenu, D_SHOW, &showMenu, ICON_show);       // SHOW
        addMenuNode(&mainMenu, D_ATTACK, &attackMenu, ICON_attack); // ATTACK
        addMenuNode(&mainMenu, D_PACKET_MONITOR, [this]() {         // PACKET MONITOR
            scan.start(SCAN_MODE_SNIFFER, 0, SCAN_MODE_OFF, 0, false, wifi_channel);
            mode = DISPLAY_MODE::PACKETMONITOR;
        }, ICON_packet);
        addMenuNode(&mainMenu, D_CLOCK, &clockMenu, ICON_clock); // CLOCK

#ifdef HIGHLIGHT_LED
        addMenuNode(&mainMenu, D_LED, [this]() {     // LED
            highlightLED = !highlightLED;
            digitalWrite(HIGHLIGHT_LED, highlightLED);
        }, ICON_led);
#endif // ifdef HIGHLIGHT_LED
    });

    // SCAN MENU
    createMenu(&scanMenu, &mainMenu, [this]() {
        addMenuNode(&scanMenu, D_SCAN_APST, [this]() { // SCAN AP + ST
            scan.start(SCAN_MODE_ALL, 15000, SCAN_MODE_OFF, 0, true, wifi_channel);
            mode = DISPLAY_MODE::LOADSCAN;
        }, ICON_scan);
        addMenuNode(&scanMenu, D_SCAN_AP, [this]() { // SCAN AP
            scan.start(SCAN_MODE_APS, 0, SCAN_MODE_OFF, 0, true, wifi_channel);
            mode = DISPLAY_MODE::LOADSCAN;
        }, ICON_ap);
        addMenuNode(&scanMenu, D_SCAN_ST, [this]() { // SCAN ST
            scan.start(SCAN_MODE_STATIONS, 30000, SCAN_MODE_OFF, 0, true, wifi_channel);
            mode = DISPLAY_MODE::LOADSCAN;
        }, ICON_station);
    });

    // SHOW MENU
    createMenu(&showMenu, &mainMenu, [this]() {
        addMenuNode(&showMenu, [this]() { // Accesspoints 0 [0]
            return leftRight(str(D_ACCESSPOINTS), (String)accesspoints.count(), maxLen - 4);
        }, &apListMenu, ICON_ap);
        addMenuNode(&showMenu, [this]() { // Stations 0 [0]
            return leftRight(str(D_STATIONS), (String)stations.count(), maxLen - 4);
        }, &stationListMenu, ICON_station);
        addMenuNode(&showMenu, [this]() { // Names 0 [0]
            return leftRight(str(D_NAMES), (String)names.count(), maxLen - 4);
        }, &nameListMenu, ICON_name);
        addMenuNode(&showMenu, [this]() { // SSIDs 0
            return leftRight(str(D_SSIDS), (String)ssids.count(), maxLen - 4);
        }, &ssidListMenu, ICON_ssid);
    });

    // AP LIST MENU
    createMenu(&apListMenu, &showMenu, [this]() {
        // add APs to list
        int c = accesspoints.count();

        for (int i = 0; i < c; i++) {
            addMenuNode(&apListMenu, [i]() {
                return b2a(accesspoints.getSelected(i)) + accesspoints.getSSID(i);
            }, [this, i]() {
                accesspoints.getSelected(i) ? accesspoints.deselect(i) : accesspoints.select(i);
            }, [this, i]() {
                selectedID = i;
                changeMenu(&apMenu);
            });
        }
        addMenuNode(&apListMenu, D_SELECT_ALL, [this]() { // SELECT ALL
            accesspoints.selectAll();
            changeMenu(&apListMenu);
        });
        addMenuNode(&apListMenu, D_DESELECT_ALL, [this]() { // DESELECT ALL
            accesspoints.deselectAll();
            changeMenu(&apListMenu);
        });
        addMenuNode(&apListMenu, D_REMOVE_ALL, [this]() { // REMOVE ALL
            accesspoints.removeAll();
            goBack();
        });
    });

    // STATION LIST MENU
    createMenu(&stationListMenu, &showMenu, [this]() {
        // add stations to list
        int c = stations.count();

        for (int i = 0; i < c; i++) {
            addMenuNode(&stationListMenu, [i]() {
                return b2a(stations.getSelected(i)) +
                (stations.hasName(i) ? stations.getNameStr(i) : stations.getMacVendorStr(i));
            }, [this, i]() {
                stations.getSelected(i) ? stations.deselect(i) : stations.select(i);
            }, [this, i]() {
                selectedID = i;
                changeMenu(&stationMenu);
            });
        }

        addMenuNode(&stationListMenu, D_SELECT_ALL, [this]() { // SELECT ALL
            stations.selectAll();
            changeMenu(&stationListMenu);
        });
        addMenuNode(&stationListMenu, D_DESELECT_ALL, [this]() { // DESELECT ALL
            stations.deselectAll();
            changeMenu(&stationListMenu);
        });
        addMenuNode(&stationListMenu, D_REMOVE_ALL, [this]() { // REMOVE ALL
            stations.removeAll();
            goBack();
        });
    });

    // NAME LIST MENU
    createMenu(&nameListMenu, &showMenu, [this]() {
        // add device names to list
        int c = names.count();

        for (int i = 0; i < c; i++) {
            addMenuNode(&nameListMenu, [i]() {
                return names.getSelectedStr(i) + names.getName(i);
            }, [this, i]() {
                names.getSelected(i) ? names.deselect(i) : names.select(i);
            }, [this, i]() {
                selectedID = i;
                changeMenu(&nameMenu);
            });
        }
        addMenuNode(&nameListMenu, D_SELECT_ALL, [this]() { // SELECT ALL
            names.selectAll();
            changeMenu(&nameListMenu);
        });
        addMenuNode(&nameListMenu, D_DESELECT_ALL, [this]() { // DESELECT ALL
            names.deselectAll();
            changeMenu(&nameListMenu);
        });
        addMenuNode(&nameListMenu, D_REMOVE_ALL, [this]() { // REMOVE ALL
            names.removeAll();
            goBack();
        });
    });

    // SSID LIST MENU
    createMenu(&ssidListMenu, &showMenu, [this]() {
        addMenuNode(&ssidListMenu, D_CLONE_APS, [this]() { // CLONE APs
            ssids.cloneSelected(true);
            changeMenu(&ssidListMenu);
            ssids.save(false);
        });
        addMenuNode(&ssidListMenu, [this]() {
            return b2a(ssids.getRandom()) + str(D_RANDOM_MODE); // *RANDOM MODE
        }, [this]() {
            if (ssids.getRandom()) ssids.disableRandom();
            else ssids.enableRandom(10);
            changeMenu(&ssidListMenu);
        });

        // add ssids to list
        int c = ssids.count();

        for (int i = 0; i < c; i++) {
            addMenuNode(&ssidListMenu, [i]() {
                return ssids.getName(i).substring(0, ssids.getLen(i));
            }, [this, i]() {
                selectedID = i;
                changeMenu(&ssidMenu);
            }, [this, i]() {
                ssids.remove(i);
                changeMenu(&ssidListMenu);
                ssidListMenu.selected = i;
            });
        }

        addMenuNode(&ssidListMenu, D_REMOVE_ALL, [this]() { // REMOVE ALL
            ssids.removeAll();
            goBack();
        });
    });

    // AP MENU
    createMenu(&apMenu, &apListMenu, [this]() {
        addMenuNode(&apMenu, [this]() {
            return accesspoints.getSelectedStr(selectedID)  + accesspoints.getSSID(selectedID); // *<ssid>
        }, [this]() {
            accesspoints.getSelected(selectedID) ? accesspoints.deselect(selectedID) : accesspoints.select(selectedID);
        });
        addMenuNode(&apMenu, [this]() {
            return str(D_ENCRYPTION) + accesspoints.getEncStr(selectedID);
        }, NULL);                                                                          // Encryption: -/WPA2
        addMenuNode(&apMenu, [this]() {
            return str(D_RSSI) + (String)accesspoints.getRSSI(selectedID);
        }, NULL);                                                                          // RSSI: -90
        addMenuNode(&apMenu, [this]() {
            return str(D_CHANNEL) + (String)accesspoints.getCh(selectedID);
        }, NULL);                                                                          // Channel: 11
        addMenuNode(&apMenu, [this]() {
            return accesspoints.getMacStr(selectedID);
        }, NULL);                                                                          // 00:11:22:00:11:22
        addMenuNode(&apMenu, [this]() {
            return str(D_VENDOR) + accesspoints.getVendorStr(selectedID);
        }, NULL);                                                                          // Vendor: INTEL
        addMenuNode(&apMenu, [this]() {
            return accesspoints.getSelected(selectedID) ? str(D_DESELECT) : str(D_SELECT); // SELECT/DESELECT
        }, [this]() {
            accesspoints.getSelected(selectedID) ? accesspoints.deselect(selectedID) : accesspoints.select(selectedID);
        });
        addMenuNode(&apMenu, D_CLONE, [this]() { // CLONE
            ssids.add(accesspoints.getSSID(selectedID), accesspoints.getEnc(selectedID) != ENC_TYPE_NONE, 60, true);
            changeMenu(&showMenu);
            ssids.save(false);
        });
        addMenuNode(&apMenu, D_REMOVE, [this]() { // REMOVE
            accesspoints.remove(selectedID);
            apListMenu.list->remove(apListMenu.selected);
            goBack();
        });
    });

    // STATION MENU
    createMenu(&stationMenu, &stationListMenu, [this]() {
        addMenuNode(&stationMenu, [this]() {
            return stations.getSelectedStr(selectedID) +
            (stations.hasName(selectedID) ? stations.getNameStr(selectedID) : stations.getMacVendorStr(selectedID)); // <station
            // name>
        }, [this]() {
            stations.getSelected(selectedID) ? stations.deselect(selectedID) : stations.select(selectedID);
        });
        addMenuNode(&stationMenu, [this]() {
            return stations.getMacStr(selectedID);
        }, NULL);                                             // 00:11:22:00:11:22
        addMenuNode(&stationMenu, [this]() {
            return str(D_VENDOR) + stations.getVendorStr(selectedID);
        }, NULL);                                             // Vendor: INTEL
        addMenuNode(&stationMenu, [this]() {
            return str(D_AP) + stations.getAPStr(selectedID); // AP: someAP
        }, [this]() {
            int apID = accesspoints.find(stations.getAP(selectedID));

            if (apID >= 0) {
                selectedID = apID;
                changeMenu(&apMenu);
            }
        });
        addMenuNode(&stationMenu, [this]() {
            return str(D_PKTS) + String(*stations.getPkts(selectedID));
        }, NULL);                                                                      // Pkts: 12
        addMenuNode(&stationMenu, [this]() {
            return str(D_CHANNEL) + String(stations.getCh(selectedID));
        }, NULL);                                                                      // Channel: 11
        addMenuNode(&stationMenu, [this]() {
            return str(D_SEEN) + stations.getTimeStr(selectedID);
        }, NULL);                                                                      // Seen: <1min

        addMenuNode(&stationMenu, [this]() {
            return stations.getSelected(selectedID) ? str(D_DESELECT) : str(D_SELECT); // SELECT/DESELECT
        }, [this]() {
            stations.getSelected(selectedID) ? stations.deselect(selectedID) : stations.select(selectedID);
        });
        addMenuNode(&stationMenu, D_REMOVE, [this]() { // REMOVE
            stations.remove(selectedID);
            stationListMenu.list->remove(stationListMenu.selected);
            goBack();
        });
    });

    // NAME MENU
    createMenu(&nameMenu, &nameListMenu, [this]() {
        addMenuNode(&nameMenu, [this]() {
            return names.getSelectedStr(selectedID) + names.getName(selectedID); // <station name>
        }, [this]() {
            names.getSelected(selectedID) ? names.deselect(selectedID) : names.select(selectedID);
        });
        addMenuNode(&nameMenu, [this]() {
            return names.getMacStr(selectedID);
        }, NULL);                                                                   // 00:11:22:00:11:22
        addMenuNode(&nameMenu, [this]() {
            return str(D_VENDOR) + names.getVendorStr(selectedID);
        }, NULL);                                                                   // Vendor: INTEL
        addMenuNode(&nameMenu, [this]() {
            return str(D_AP) + names.getBssidStr(selectedID);
        }, NULL);                                                                   // AP: 00:11:22:00:11:22
        addMenuNode(&nameMenu, [this]() {
            return str(D_CHANNEL) + (String)names.getCh(selectedID);
        }, NULL);                                                                   // Channel: 11

        addMenuNode(&nameMenu, [this]() {
            return names.getSelected(selectedID) ? str(D_DESELECT) : str(D_SELECT); // SELECT/DESELECT
        }, [this]() {
            names.getSelected(selectedID) ? names.deselect(selectedID) : names.select(selectedID);
        });
        addMenuNode(&nameMenu, D_REMOVE, [this]() { // REMOVE
            names.remove(selectedID);
            nameListMenu.list->remove(nameListMenu.selected);
            goBack();
        });
    });

    // SSID MENU
    createMenu(&ssidMenu, &ssidListMenu, [this]() {
        addMenuNode(&ssidMenu, [this]() {
            return ssids.getName(selectedID).substring(0, ssids.getLen(selectedID));
        }, NULL);                                                   // SSID
        addMenuNode(&ssidMenu, [this]() {
            return str(D_ENCRYPTION) + ssids.getEncStr(selectedID); // WPA2
        }, [this]() {
            ssids.setWPA2(selectedID, !ssids.getWPA2(selectedID));
        });
        addMenuNode(&ssidMenu, D_REMOVE, [this]() { // REMOVE
            ssids.remove(selectedID);
            ssidListMenu.list->remove(ssidListMenu.selected);
            goBack();
        });
    });

    // ATTACK MENU
    createMenu(&attackMenu, &mainMenu, [this]() {
        addMenuNode(&attackMenu, [this]() { // *DEAUTH 0/0
            if (attack.isRunning()) return leftRight(b2a(deauthSelected) + str(D_DEAUTH),
                                                     (String)attack.getDeauthPkts() + SLASH +
                                                     (String)attack.getDeauthMaxPkts(), maxLen - 4);
            else return leftRight(b2a(deauthSelected) + str(D_DEAUTH), (String)scan.countSelected(), maxLen - 4);
        }, [this]() { // deauth
            deauthSelected = !deauthSelected;

            if (attack.isRunning()) {
                attack.start(beaconSelected, deauthSelected, false, probeSelected, true,
                             settings::getAttackSettings().timeout * 1000);
            }
        }, ICON_deauth);
        addMenuNode(&attackMenu, [this]() { // *BEACON 0/0
            if (attack.isRunning()) return leftRight(b2a(beaconSelected) + str(D_BEACON),
                                                     (String)attack.getBeaconPkts() + SLASH +
                                                     (String)attack.getBeaconMaxPkts(), maxLen - 4);
            else return leftRight(b2a(beaconSelected) + str(D_BEACON), (String)ssids.count(), maxLen - 4);
        }, [this]() { // beacon
            beaconSelected = !beaconSelected;

            if (attack.isRunning()) {
                attack.start(beaconSelected, deauthSelected, false, probeSelected, true,
                             settings::getAttackSettings().timeout * 1000);
            }
        }, ICON_beacon);
        addMenuNode(&attackMenu, [this]() { // *PROBE 0/0
            if (attack.isRunning()) return leftRight(b2a(probeSelected) + str(D_PROBE),
                                                     (String)attack.getProbePkts() + SLASH +
                                                     (String)attack.getProbeMaxPkts(), maxLen - 4);
            else return leftRight(b2a(probeSelected) + str(D_PROBE), (String)ssids.count(), maxLen - 4);
        }, [this]() { // probe
            probeSelected = !probeSelected;

            if (attack.isRunning()) {
                attack.start(beaconSelected, deauthSelected, false, probeSelected, true,
                             settings::getAttackSettings().timeout * 1000);
            }
        }, ICON_probe);
        addMenuNode(&attackMenu, [this]() { // START
            return leftRight(str(attack.isRunning() ? D_STOP_ATTACK : D_START_ATTACK),
                             attack.getPacketRate() > 0 ? (String)attack.getPacketRate() : String(), maxLen - 4);
        }, [this]() {
            if (attack.isRunning()) attack.stop();
            else attack.start(beaconSelected, deauthSelected, false, probeSelected, true,
                              settings::getAttackSettings().timeout * 1000);
        }, attack.isRunning() ? ICON_stop : ICON_start);
    });

    // CLOCK MENU
    createMenu(&clockMenu, &mainMenu, [this]() {
        addMenuNode(&clockMenu, D_CLOCK_DISPLAY, [this]() { // CLOCK
            mode = DISPLAY_MODE::CLOCK_DISPLAY;
            display.setFont(ArialMT_Plain_24);
            display.setTextAlignment(TEXT_ALIGN_CENTER);
        }, ICON_clock);
        addMenuNode(&clockMenu, D_CLOCK_SET, [this]() { // CLOCK SET TIME
            mode = DISPLAY_MODE::CLOCK;
            display.setFont(ArialMT_Plain_24);
            display.setTextAlignment(TEXT_ALIGN_CENTER);
        }, ICON_gear);
    });

    // ===================== //

    // enable big-icon carousel view for the main navigation menus
    mainMenu.iconView   = true;
    scanMenu.iconView   = true;
    attackMenu.iconView = true;
    clockMenu.iconView  = true;
    showMenu.iconView   = true;

    // set current menu to main menu
    changeMenu(&mainMenu);
    enabled   = true;
    startTime = currentTime;
}

#ifdef HIGHLIGHT_LED
void DisplayUI::setupLED() {
    pinMode(HIGHLIGHT_LED, OUTPUT);
    digitalWrite(HIGHLIGHT_LED, HIGH);
    highlightLED = true;
}

#endif // ifdef HIGHLIGHT_LED

void DisplayUI::update(bool force) {
    if (!enabled) return;

    // ── Single Boot Button: đọc sự kiện từ GPIO0 ──────────────────────────
    SingleButton::Event bEvt = bootBtn.read();

    // Bất kỳ sự kiện nào: cập nhật buttonTime để giữ màn hình sáng
    if (bEvt != SingleButton::EVENT_NONE) buttonTime = currentTime;

    // Nếu màn đang tắt tạm: bất kỳ thao tác nào đều bật lại màn hình
    if (tempOff) {
        if (bEvt != SingleButton::EVENT_NONE) on();
    } else {
        // ── EVENT_DOWN (nhấn ngắn < 600ms) → Cuộn XUỐNG ──────────────────
        if (bEvt == SingleButton::EVENT_DOWN) {
            scrollCounter = 0;
            scrollTime    = currentTime;
            if (mode == DISPLAY_MODE::MENU) {
                uint8_t oldSel = currentMenu->selected;
                if (currentMenu->selected < currentMenu->list->size() - 1) currentMenu->selected++;
                else currentMenu->selected = 0;
                // trigger slide transition for big-icon carousel menus
                if (currentMenu->iconView && (oldSel != currentMenu->selected)) {
                    iconPrevSel    = oldSel;
                    iconSlideDir   = 1;
                    iconSlideStart = currentTime;
                }
            } else if (mode == DISPLAY_MODE::PACKETMONITOR) {
                scan.setChannel(wifi_channel - 1);
            } else if (mode == DISPLAY_MODE::CLOCK) {
                setTime(clockHour, clockMinute - 1, clockSecond);
            }
        }

        // ── EVENT_SELECT (giữ 600–1999ms) → CHỌN / click ─────────────────
        if (bEvt == SingleButton::EVENT_SELECT) {
            scrollCounter = 0;
            scrollTime    = currentTime;
            switch (mode) {
                case DISPLAY_MODE::MENU:
                    if (currentMenu->list->get(currentMenu->selected).click)
                        currentMenu->list->get(currentMenu->selected).click();
                    break;
                case DISPLAY_MODE::PACKETMONITOR:
                case DISPLAY_MODE::LOADSCAN:
                    scan.stop();
                    mode = DISPLAY_MODE::MENU;
                    break;
                case DISPLAY_MODE::CLOCK:
                case DISPLAY_MODE::CLOCK_DISPLAY:
                    mode = DISPLAY_MODE::MENU;
                    display.setFont(DejaVu_Sans_Mono_12);
                    display.setTextAlignment(TEXT_ALIGN_LEFT);
                    break;
                default:
                    break;
            }
        }

        // ── EVENT_BACK (giữ >= 2000ms) → BACK / thoát ────────────────────
        if (bEvt == SingleButton::EVENT_BACK) {
            scrollCounter = 0;
            scrollTime    = currentTime;
            switch (mode) {
                case DISPLAY_MODE::MENU:
                    goBack();
                    break;
                case DISPLAY_MODE::PACKETMONITOR:
                case DISPLAY_MODE::LOADSCAN:
                    scan.stop();
                    mode = DISPLAY_MODE::MENU;
                    break;
                case DISPLAY_MODE::CLOCK:
                case DISPLAY_MODE::CLOCK_DISPLAY:
                    mode = DISPLAY_MODE::MENU;
                    display.setFont(DejaVu_Sans_Mono_12);
                    display.setTextAlignment(TEXT_ALIGN_LEFT);
                    break;
                default:
                    break;
            }
        }
    }
    // ──────────────────────────────────────────────────────────────────────

    // force continuous redraws while a carousel slide animation is running
    bool animating = (iconSlideDir != 0) && (mode == DISPLAY_MODE::MENU);
    draw(force || animating);

    uint32_t timeout = settings::getDisplaySettings().timeout * 1000;

    if (currentTime > timeout) {
        if (!tempOff) {
            if (buttonTime < currentTime - timeout) off();
        } else {
            if (buttonTime > currentTime - timeout) on();
        }
    }
}

void DisplayUI::on() {
    if (enabled) {
        configOn();
        tempOff    = false;
        buttonTime = currentTime; // update a button time to keep display on
        prntln(D_MSG_DISPLAY_ON);
    } else {
        prntln(D_ERROR_NOT_ENABLED);
    }
}

void DisplayUI::off() {
    if (enabled) {
        configOff();
        tempOff = true;
        prntln(D_MSG_DISPLAY_OFF);
    } else {
        prntln(D_ERROR_NOT_ENABLED);
    }
}

void DisplayUI::setupButtons() {
    // ════════════════════════════════════════════════════════
    //  SINGLE BOOT BUTTON MODE
    //  Nút Boot (GPIO0, active-LOW) thay thế 4 nút UP/DOWN/A/B.
    //  Logic điều hướng đã chuyển sang hàm update() ở trên.
    // ════════════════════════════════════════════════════════
    bootBtn.begin();    // Khởi tạo GPIO0 với INPUT_PULLUP
    up   = nullptr;     // Không dùng SimpleButton objects nữa
    down = nullptr;
    a    = nullptr;
    b    = nullptr;
}

String DisplayUI::getChannel() {
    String ch = String(wifi_channel);

    if (ch.length() < 2) ch = ' ' + ch;
    return ch;
}

void DisplayUI::draw(bool force) {
    if (force || ((currentTime - drawTime > drawInterval) && currentMenu)) {
        drawTime = currentTime;

        updatePrefix();

#ifdef RTC_DS3231
        bool h12;
        bool PM_time;
        clockHour   = clock.getHour(h12, PM_time);
        clockMinute = clock.getMinute();
        clockSecond = clock.getSecond();
#else // ifdef RTC_DS3231
        if (currentTime - clockTime >= 1000) {
            setTime(clockHour, clockMinute, ++clockSecond);
            clockTime += 1000;
        }
#endif // ifdef RTC_DS3231

        switch (mode) {
            case DISPLAY_MODE::BUTTON_TEST:
                drawButtonTest();
                break;

            case DISPLAY_MODE::MENU:
                drawMenu();
                break;

            case DISPLAY_MODE::LOADSCAN:
                drawLoadingScan();
                break;

            case DISPLAY_MODE::PACKETMONITOR:
                drawPacketMonitor();
                break;

            case DISPLAY_MODE::INTRO:
                if (!scan.isScanning() && (currentTime - startTime >= screenIntroTime)) {
                    mode = DISPLAY_MODE::MENU;
                }
                drawIntro();
                break;
            case DISPLAY_MODE::CLOCK:
            case DISPLAY_MODE::CLOCK_DISPLAY:
                drawClock();
                break;
            case DISPLAY_MODE::RESETTING:
                drawResetting();
                break;
        }

        updateSuffix();
    }
}

void DisplayUI::drawButtonTest() {
    // Single Boot Button Mode: hiển thị trạng thái nút Boot (GPIO0)
    bool pressed     = bootBtn.isHeld();
    unsigned long ms = bootBtn.heldDuration();
    drawString(0, String("Boot GPIO0: ") + (pressed ? "HELD" : "open"));
    drawString(1, String("Held: ") + String(ms) + String("ms"));
    if (!pressed)    drawString(2, "Action: --");
    else if (ms < 600)  drawString(2, "Action: DOWN");
    else if (ms < 2000) drawString(2, "Action: SELECT");
    else                drawString(2, "Action: BACK");
    drawString(3, "[ single btn mode ]");
}

void DisplayUI::drawMenu() {
    if (currentMenu->iconView) { drawMenuIcon(); return; }

    String tmp;
    int    tmpLen;
    int    row = (currentMenu->selected / 5) * 5;

    // correct selected if it's off
    if (currentMenu->selected < 0) currentMenu->selected = 0;
    else if (currentMenu->selected >= currentMenu->list->size()) currentMenu->selected = currentMenu->list->size() - 1;

    // draw menu entries
    for (int i = row; i < currentMenu->list->size() && i < row + 5; i++) {
        MenuNode node    = currentMenu->list->get(i);
        bool     hasIcon = (node.icon != NULL);
        bool     sel      = (currentMenu->selected == i);
        int      y        = (i - row) * 12;
        int      textX    = hasIcon ? 15 : 2;
        uint8_t  lineMax  = hasIcon ? 16 : maxLen;

        tmp    = node.getStr();
        tmpLen = tmp.length();

        // horizontal scrolling (only for the selected, overflowing entry)
        if (sel && (tmpLen >= lineMax)) {
            tmp = tmp + tmp;
            tmp = tmp.substring(scrollCounter, scrollCounter + lineMax - 1);

            if (((scrollCounter > 0) && (scrollTime < currentTime - scrollSpeed)) || ((scrollCounter == 0) && (scrollTime < currentTime - scrollSpeed * 4))) {
                scrollTime = currentTime;
                scrollCounter++;
            }

            if (scrollCounter > tmpLen) scrollCounter = 0;
        }

        // selection highlight bar
        if (sel) {
            display.setColor(WHITE);
            display.fillRect(0, y, screenWidth, 12);
            display.setColor(BLACK);
        }

        // per-item icon
        if (hasIcon) display.drawXbm(1, y, ICON_W, ICON_H, node.icon);

        // label
        drawString(textX, y, tmp);

        // restore default draw color
        if (sel) display.setColor(WHITE);
    }
}

// ===== big-icon carousel: one menu item per screen ===== //
void DisplayUI::drawIconScaled(int x, int y, const uint8_t* xbm, uint8_t scale) {
    int widthInXbm = (ICON_W + 7) / 8; // bytes per row (=2 for 12px)
    for (int row = 0; row < ICON_H; row++) {
        for (int col = 0; col < ICON_W; col++) {
            uint8_t b = pgm_read_byte(xbm + (col / 8) + row * widthInXbm);
            if (b & (1 << (col & 7))) {
                display.fillRect(x + col * scale, y + row * scale, scale, scale);
            }
        }
    }
}

void DisplayUI::drawMenuIcon() {
    int total = currentMenu->list->size();

    if (total == 0) return;
    if (currentMenu->selected < 0) currentMenu->selected = 0;
    else if (currentMenu->selected >= total) currentMenu->selected = total - 1;

    int      sel  = currentMenu->selected;
    MenuNode node = currentMenu->list->get(sel);

    display.setColor(WHITE);

    const int iy      = 2;
    const int centerX = (screenWidth - ICON_W) / 2; // 48 for 32px icon

    // --- big 32x32 icon with horizontal slide transition ---
    uint32_t elapsed = currentTime - iconSlideStart;
    if ((iconSlideDir != 0) && (elapsed < iconSlideDur) && (iconPrevSel < total)) {
        int prog  = (int)(elapsed * screenWidth / iconSlideDur); // 0..128
        int newX  = centerX + iconSlideDir * (screenWidth - prog);
        int prevX = newX - iconSlideDir * screenWidth;
        MenuNode prevNode = currentMenu->list->get(iconPrevSel);
        if (prevNode.icon) display.drawXbm(prevX, iy, ICON_W, ICON_H, prevNode.icon);
        if (node.icon)     display.drawXbm(newX,  iy, ICON_W, ICON_H, node.icon);
    } else {
        iconSlideDir = 0;
        if (node.icon) display.drawXbm(centerX, iy, ICON_W, ICON_H, node.icon);
    }

    // --- left/right navigation chevrons ---
    display.setFont(DejaVu_Sans_Mono_12);
    if (total > 1) {
        int chY = iy + ICON_H / 2 - 8;
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.drawString(0, chY, "<");
        display.setTextAlignment(TEXT_ALIGN_RIGHT);
        display.drawString(screenWidth, chY, ">");
    }

    // --- label under the icon ---
    String label  = node.getStr();
    int    labelY = iy + ICON_H + 4; // ~38
    if ((int)label.length() <= maxLen) {
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.drawString(screenWidth / 2, labelY, replaceUtf8(label, String(QUESTIONMARK)));
    } else {
        String txt = label + "   " + label;
        txt = txt.substring(scrollCounter, scrollCounter + maxLen);
        if (((scrollCounter > 0) && (scrollTime < currentTime - scrollSpeed)) ||
            ((scrollCounter == 0) && (scrollTime < currentTime - scrollSpeed * 4))) {
            scrollTime = currentTime;
            scrollCounter++;
        }
        if (scrollCounter > (int)label.length() + 3) scrollCounter = 0;
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.drawString(0, labelY, replaceUtf8(txt, String(QUESTIONMARK)));
    }

    // --- position indicator (dots or n/total) at the bottom ---
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    if (total <= 9) {
        const int gap = 7;
        int startX    = screenWidth / 2 - ((total - 1) * gap) / 2;
        int dy        = 60;
        for (int i = 0; i < total; i++) {
            if (i == sel) display.fillCircle(startX + i * gap, dy, 2);
            else display.drawCircle(startX + i * gap, dy, 1);
        }
    } else {
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.drawString(screenWidth / 2, 54, String(sel + 1) + "/" + String(total));
    }

    display.setTextAlignment(TEXT_ALIGN_LEFT);
}

void DisplayUI::drawLoadingScan() {
    String percentage;

    if (scan.isScanning()) {
        percentage = String(scan.getPercentage()) + '%';
    } else {
        percentage = str(DSP_SCAN_DONE);
    }

    drawString(0, leftRight(str(DSP_SCAN_FOR), scan.getMode(), maxLen));
    drawString(1, leftRight(str(DSP_APS), String(accesspoints.count()), maxLen));
    drawString(2, leftRight(str(DSP_STS), String(stations.count()), maxLen));
    drawString(3, leftRight(str(DSP_PKTS), String(scan.getPacketRate()) + str(DSP_S), maxLen));
    drawString(4, center(percentage, maxLen));
}

void DisplayUI::drawPacketMonitor() {
    double scale = scan.getScaleFactor(sreenHeight - lineHeight - 2);

    String headline = leftRight(str(D_CH) + getChannel() + String(' ') + String('[') + String(scan.deauths) + String(']'), String(scan.getPacketRate()) + str(D_PKTS), maxLen);

    drawString(0, 0, headline);

    if (scan.getMaxPacket() > 0) {
        int i = 0;
        int x = 0;
        int y = 0;

        while (i < SCAN_PACKET_LIST_SIZE && x < screenWidth) {
            y = (sreenHeight-1) - (scan.getPackets(i) * scale);
            i++;

            // Serial.printf("%d,%d -> %d,%d\n", x, (sreenHeight-1), x, y);
            drawLine(x, (sreenHeight-1), x, y);
            x++;

            // Serial.printf("%d,%d -> %d,%d\n", x, (sreenHeight-1), x, y);
            drawLine(x, (sreenHeight-1), x, y);
            x++;
        }
        // Serial.println("---------");
    }
}

void DisplayUI::drawIntro() {
    // ===== Animated boot sequence ===== //
    uint32_t t     = currentTime - startTime; // elapsed time since boot (ms)
    int      cx    = screenWidth / 2;         // horizontal center
    int      cy    = 22;                        // broadcast center (lowered so rings fit)

    display.setColor(WHITE);

    // --- pulsing WiFi broadcast animation (base dot + expanding arcs) ---
    display.fillCircle(cx, cy, 2);
    uint8_t arcs = (t / 200) % 4; // 0..3 pulsing rings
    for (uint8_t k = 1; k <= arcs; k++) {
        // upper two quadrants only; max radius 18 stays fully on-screen (22-18=4)
        display.drawCircleQuads(cx, cy, k * 6, 0b00000011);
    }

    // --- title: slides in from the left during the first 500 ms ---
    int slide = 0;
    if (t < 500) slide = -(int)((500 - t) * screenWidth / 500);

    display.setFont(DejaVu_Sans_Mono_12);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(cx + slide, 30, replaceUtf8(str(D_INTRO_0), String(QUESTIONMARK))); // ESP8266 Deauther
    if (t > 600)  display.drawString(cx, 42, replaceUtf8(str(D_INTRO_1), String(QUESTIONMARK))); // by @Spacehuhn

    // reset alignment/font so the menu (drawn next) renders correctly
    display.setTextAlignment(TEXT_ALIGN_LEFT);

    // --- bottom loading bar tracks boot progress ---
    uint8_t progress = (t >= screenIntroTime) ? 100 : (uint8_t)(t * 100 / screenIntroTime);
    display.drawProgressBar(4, 56, screenWidth - 8, 6, progress);
}

void DisplayUI::drawClock() {
    String clockTime = String(clockHour);

    clockTime += ':';
    if (clockMinute < 10) clockTime += '0';
    clockTime += String(clockMinute);

    display.drawString(64, 20, clockTime);
}

void DisplayUI::drawResetting() {
    drawString(2, center(str(D_RESETTING), maxLen));
}

void DisplayUI::clearMenu(Menu* menu) {
    while (menu->list->size() > 0) {
        menu->list->remove(0);
    }
}

void DisplayUI::changeMenu(Menu* menu) {
    if (menu) {
        // only open list menu if it has nodes
        if (((menu == &apListMenu) && (accesspoints.count() == 0)) ||
            ((menu == &stationListMenu) && (stations.count() == 0)) ||
            ((menu == &nameListMenu) && (names.count() == 0))) {
            return;
        }

        if (currentMenu) clearMenu(currentMenu);
        currentMenu           = menu;
        currentMenu->selected = 0;
        buttonTime            = currentTime;

        if (selectedID < 0) selectedID = 0;

        if (currentMenu->parentMenu) {
            addMenuNode(currentMenu, D_BACK, currentMenu->parentMenu); // add [BACK]
            currentMenu->selected = 1;
        }

        if (currentMenu->build) currentMenu->build();
    }
}

void DisplayUI::goBack() {
    if (currentMenu->parentMenu) changeMenu(currentMenu->parentMenu);
}

void DisplayUI::createMenu(Menu* menu, Menu* parent, std::function<void()>build) {
    menu->list       = new SimpleList<MenuNode>;
    menu->parentMenu = parent;
    menu->selected   = 0;
    menu->build      = build;
    menu->iconView   = false;
}

// ===== core: full signature with icon ===== //
void DisplayUI::addMenuNode(Menu* menu, std::function<String()>getStr, std::function<void()>click,
                            std::function<void()>hold, const uint8_t* icon) {
    menu->list->add(MenuNode{ getStr, click, hold, icon });
}

// ===== original signatures (no icon) ===== //
void DisplayUI::addMenuNode(Menu* menu, std::function<String()>getStr, std::function<void()>click,
                            std::function<void()>hold) {
    addMenuNode(menu, getStr, click, hold, (const uint8_t*)NULL);
}

void DisplayUI::addMenuNode(Menu* menu, std::function<String()>getStr, std::function<void()>click) {
    addMenuNode(menu, getStr, click, NULL, (const uint8_t*)NULL);
}

void DisplayUI::addMenuNode(Menu* menu, std::function<String()>getStr, Menu* next) {
    addMenuNode(menu, getStr, next, (const uint8_t*)NULL);
}

void DisplayUI::addMenuNode(Menu* menu, const char* ptr, std::function<void()>click) {
    addMenuNode(menu, ptr, click, (const uint8_t*)NULL);
}

void DisplayUI::addMenuNode(Menu* menu, const char* ptr, Menu* next) {
    addMenuNode(menu, ptr, next, (const uint8_t*)NULL);
}

// ===== icon-carrying variants ===== //
void DisplayUI::addMenuNode(Menu* menu, std::function<String()>getStr, std::function<void()>click, const uint8_t* icon) {
    addMenuNode(menu, getStr, click, NULL, icon);
}

void DisplayUI::addMenuNode(Menu* menu, std::function<String()>getStr, Menu* next, const uint8_t* icon) {
    addMenuNode(menu, getStr, [this, next]() {
        changeMenu(next);
    }, NULL, icon);
}

void DisplayUI::addMenuNode(Menu* menu, const char* ptr, std::function<void()>click, const uint8_t* icon) {
    addMenuNode(menu, [ptr]() {
        return str(ptr);
    }, click, NULL, icon);
}

void DisplayUI::addMenuNode(Menu* menu, const char* ptr, Menu* next, const uint8_t* icon) {
    addMenuNode(menu, [ptr]() {
        return str(ptr);
    }, next, icon);
}

void DisplayUI::setTime(int h, int m, int s) {
    if (s >= 60) {
        s = 0;
        m++;
    }

    if (m >= 60) {
        m = 0;
        h++;
    }

    if (h >= 24) {
        h = 0;
    }

    if (s < 0) {
        s = 59;
        m--;
    }

    if (m < 0) {
        m = 59;
        h--;
    }

    if (h < 0) {
        h = 23;
    }

    clockHour   = h;
    clockMinute = m;
    clockSecond = s;

#ifdef RTC_DS3231
    clock.setHour(clockHour);
    clock.setMinute(clockMinute);
    clock.setSecond(clockSecond);
#endif // ifdef RTC_DS3231
}
