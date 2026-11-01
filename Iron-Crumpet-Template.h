#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "config_manager.h"

// =============================================================================
// ACTIVE TEMPLATE: IRON CRUMPET
// Template mapping logic for the 65-channel SimHub Iron Crumpet dashboard
// =============================================================================
namespace DashTemplate_IronCrumpet {

    static const char* TEMPLATE_PATH = "/Dashtemplates/Iron Crumpet/Iron Crumpet.djson";

    // -------------------------------------------------------------------------
    // TIME STRING FORMATTER
    // Formats incoming time strings into clean xx:xx:xx.x format
    // -------------------------------------------------------------------------
    inline void format_simhub_time_string(char* dest, size_t dest_len, const char* src) {
        if (!dest || dest_len == 0) return;

        if (!src || strlen(src) == 0 || strcmp(src, "00:00:00") == 0 || strcmp(src, "00:00:00.000") == 0) {
            snprintf(dest, dest_len, "--:--:--.-");
            return;
        }

        int colons = 0;
        for (const char* p = src; *p; p++) {
            if (*p == ':') colons++;
        }

        int p1 = 0, p2 = 0, p3 = 0, ms = 0;

        if (colons >= 2) {
            if (sscanf(src, "%d:%d:%d%*c%d", &p1, &p2, &p3, &ms) >= 3) {
                while (ms >= 10) ms /= 10;
                snprintf(dest, dest_len, "%02d:%02d:%02d.%1d", p1, p2, p3, ms);
                return;
            }
        } else if (colons == 1) {
            if (sscanf(src, "%d:%d%*c%d", &p1, &p2, &ms) >= 2) {
                while (ms >= 10) ms /= 10;
                snprintf(dest, dest_len, "00:%02d:%02d.%1d", p1, p2, ms);
                return;
            }
        }

        snprintf(dest, dest_len, "--:--:--.-");
    }

    // -------------------------------------------------------------------------
    // TELEMETRY KEY TRANSLATOR
    // Maps channel numbers and SimHub diff IDs to human-readable names
    // -------------------------------------------------------------------------
    inline const char* get_common_name(const char* key) {
        if (!key) return "UNKNOWN";
        if (strcmp(key, "1")  == 0 || strcmp(key, "1r0")  == 0) return "SPEED";
        if (strcmp(key, "2")  == 0 || strcmp(key, "2r0")  == 0) return "RPM";
        if (strcmp(key, "3")  == 0 || strcmp(key, "3r0")  == 0) return "GEAR";
        if (strcmp(key, "4")  == 0 || strcmp(key, "4r0")  == 0) return "LAP TIME";
        if (strcmp(key, "5")  == 0 || strcmp(key, "5r0")  == 0) return "DELTA";
        if (strcmp(key, "6")  == 0 || strcmp(key, "6r0")  == 0) return "LAST LAP";
        if (strcmp(key, "7")  == 0 || strcmp(key, "7r0")  == 0) return "BEST LAP";
        if (strcmp(key, "8")  == 0 || strcmp(key, "8r0")  == 0) return "LAP NUM";
        if (strcmp(key, "9")  == 0 || strcmp(key, "9r0")  == 0) return "POS";
        if (strcmp(key, "10") == 0 || strcmp(key, "10r0") == 0) return "FUEL";
        if (strcmp(key, "11") == 0 || strcmp(key, "11r0") == 0) return "BIAS";
        if (strcmp(key, "12") == 0 || strcmp(key, "12r0") == 0) return "TC LEVEL";
        if (strcmp(key, "13") == 0 || strcmp(key, "13r0") == 0) return "ABS LEVEL";
        if (strcmp(key, "14") == 0 || strcmp(key, "14r0") == 0) return "ENG MAP";
        if (strcmp(key, "15") == 0 || strcmp(key, "15r0") == 0) return "PIT LIM";
        if (strcmp(key, "16") == 0 || strcmp(key, "16r0") == 0) return "FL PRES";
        if (strcmp(key, "17") == 0 || strcmp(key, "17r0") == 0) return "FR PRES";
        if (strcmp(key, "18") == 0 || strcmp(key, "18r0") == 0) return "RL PRES";
        if (strcmp(key, "19") == 0 || strcmp(key, "19r0") == 0) return "RR PRES";
        if (strcmp(key, "20") == 0 || strcmp(key, "20r0") == 0) return "FL TEMP";
        if (strcmp(key, "21") == 0 || strcmp(key, "21r0") == 0) return "FR TEMP";
        if (strcmp(key, "22") == 0 || strcmp(key, "22r0") == 0) return "RL TEMP";
        if (strcmp(key, "23") == 0 || strcmp(key, "23r0") == 0) return "RR TEMP";
        if (strcmp(key, "24") == 0 || strcmp(key, "24r0") == 0) return "FL BRK";
        if (strcmp(key, "25") == 0 || strcmp(key, "25r0") == 0) return "FR BRK";
        if (strcmp(key, "26") == 0 || strcmp(key, "26r0") == 0) return "RL BRK";
        if (strcmp(key, "27") == 0 || strcmp(key, "27r0") == 0) return "RR BRK";
        if (strcmp(key, "28") == 0 || strcmp(key, "28r0") == 0) return "MAX RPM";
        if (strcmp(key, "29") == 0 || strcmp(key, "29r0") == 0) return "LIGHTS";
        if (strcmp(key, "30") == 0 || strcmp(key, "30r0") == 0) return "AIR TEMP";
        if (strcmp(key, "31") == 0 || strcmp(key, "31r0") == 0) return "WATER TEMP";
        if (strcmp(key, "32") == 0 || strcmp(key, "32r0") == 0) return "OIL TEMP";
        if (strcmp(key, "33") == 0 || strcmp(key, "33r0") == 0) return "OIL PRES";
        if (strcmp(key, "34") == 0 || strcmp(key, "34r0") == 0) return "FUEL PRES";
        if (strcmp(key, "35") == 0 || strcmp(key, "35r0") == 0) return "ROAD TEMP";
        if (strcmp(key, "36") == 0 || strcmp(key, "36r0") == 0) return "FLAG";
        if (strcmp(key, "37") == 0 || strcmp(key, "37r0") == 0) return "TC SLIP";
        if (strcmp(key, "38") == 0 || strcmp(key, "38r0") == 0) return "TC CUT";
        if (strcmp(key, "39") == 0 || strcmp(key, "39r0") == 0) return "SESSION";
        if (strcmp(key, "40") == 0 || strcmp(key, "40r0") == 0) return "TOTAL LAPS";
        if (strcmp(key, "41") == 0 || strcmp(key, "41r0") == 0) return "TC ACTIVE";
        if (strcmp(key, "42") == 0 || strcmp(key, "42r0") == 0) return "ABS ACTIVE";
        if (strcmp(key, "43") == 0 || strcmp(key, "43r0") == 0) return "FLASH";
        if (strcmp(key, "44") == 0 || strcmp(key, "44r0") == 0) return "WIPERS";
        if (strcmp(key, "45") == 0 || strcmp(key, "45r0") == 0) return "TURN L";
        if (strcmp(key, "46") == 0 || strcmp(key, "46r0") == 0) return "TURN R";
        if (strcmp(key, "47") == 0 || strcmp(key, "47r0") == 0) return "SPOTTER L";
        if (strcmp(key, "48") == 0 || strcmp(key, "48r0") == 0) return "SPOTTER R";
        if (strcmp(key, "49") == 0 || strcmp(key, "49r0") == 0) return "OPP DIST";
        if (strcmp(key, "50") == 0 || strcmp(key, "50r0") == 0) return "OPP GAP";
        if (strcmp(key, "51") == 0 || strcmp(key, "51r0") == 0) return "OPP OFFSET";
        if (strcmp(key, "52") == 0 || strcmp(key, "52r0") == 0) return "ROAD CURVE";
        if (strcmp(key, "53") == 0 || strcmp(key, "53r0") == 0) return "OPP SPEED";
        if (strcmp(key, "54") == 0 || strcmp(key, "54r0") == 0) return "OPP AHEAD DIST";
        if (strcmp(key, "55") == 0 || strcmp(key, "55r0") == 0) return "OPP AHEAD GAP";
        if (strcmp(key, "56") == 0 || strcmp(key, "56r0") == 0) return "TRACK POS";
        if (strcmp(key, "57") == 0 || strcmp(key, "57r0") == 0) return "ERS CHG";
        if (strcmp(key, "58") == 0 || strcmp(key, "58r0") == 0) return "ERS MODE";
        if (strcmp(key, "59") == 0 || strcmp(key, "59r0") == 0) return "MGUK REC";
        if (strcmp(key, "60") == 0 || strcmp(key, "60r0") == 0) return "MGUK DEL";
        if (strcmp(key, "61") == 0 || strcmp(key, "61r0") == 0) return "ERS DEPL";
        if (strcmp(key, "62") == 0 || strcmp(key, "62r0") == 0) return "CAR CLASS";
        if (strcmp(key, "64") == 0 || strcmp(key, "64r0") == 0) return "GAME NAME";
        if (strcmp(key, "65") == 0 || strcmp(key, "65r0") == 0) return "CAR NAME";

        return "UNKNOWN";
    }

    // -------------------------------------------------------------------------
    // TEMPLATE DIFF INGESTION & STRUCT MAPPING
    // -------------------------------------------------------------------------
    template <typename T>
    inline void parse_template_diff(JsonObject d, T& telem) {
        if (!d.containsKey("T")) return;
        JsonObject t = d["T"];

        auto parse_bool = [&](const char* k) -> int {
            if (!t.containsKey(k)) return -1;
            if (t[k].is<bool>()) return t[k].as<bool>() ? 1 : 0;
            if (t[k].is<int>()) return (t[k].as<int>() > 0) ? 1 : 0;
            if (t[k].is<const char*>()) {
                const char* s = t[k].as<const char*>();
                if (!s) return 0;
                if (strcasecmp(s, "true") == 0 || strcmp(s, "1") == 0) return 1;
                return 0;
            }
            return 0;
        };

        auto parse_str = [&](const char* k, char* dest, size_t max_len) {
            if (t.containsKey(k)) {
                const char* s = t[k].as<const char*>();
                if (s) {
                    strncpy(dest, s, max_len - 1);
                    dest[max_len - 1] = '\0';
                }
            }
        };

        auto parse_time_str = [&](const char* k, char* dest, size_t max_len) {
            if (t.containsKey(k)) {
                const char* s = t[k].as<const char*>();
                if (s) {
                    format_simhub_time_string(dest, max_len, s);
                }
            }
        };

        // 1. SPEED
        if (t.containsKey("1"))        telem.speed_kmh = t["1"].as<float>();
        else if (t.containsKey("1r0")) telem.speed_kmh = t["1r0"].as<float>();

        // 2. RPM
        if (t.containsKey("2")) {
            telem.rpm = t["2"].as<int>();
            telem.engine_running = (telem.rpm > 350);
        } else if (t.containsKey("2r0")) {
            telem.rpm = t["2r0"].as<int>();
            telem.engine_running = (telem.rpm > 350);
        }

        // 3. GEAR
        auto parse_gear = [&](const char* k) {
            if (!t.containsKey(k)) return false;
            if (t[k].is<const char*>()) {
                const char* g = t[k].as<const char*>();
                if (!g || strlen(g) == 0 || strcmp(g, "0") == 0) {
                    strcpy(telem.gear, "N");
                } else if (strcmp(g, "-1") == 0) {
                    strcpy(telem.gear, "R");
                } else {
                    strncpy(telem.gear, g, sizeof(telem.gear) - 1);
                    telem.gear[sizeof(telem.gear) - 1] = '\0';
                }
                return true;
            } else if (t[k].is<int>()) {
                int g_val = t[k].as<int>();
                if (g_val == 0) strcpy(telem.gear, "N");
                else if (g_val < 0) strcpy(telem.gear, "R");
                else snprintf(telem.gear, sizeof(telem.gear), "%d", g_val);
                return true;
            }
            return false;
        };
        if (!parse_gear("3")) parse_gear("3r0");

        // 4. CURRENT LAP TIME
        parse_time_str("4",   telem.current_lap_time, sizeof(telem.current_lap_time));
        parse_time_str("4r0", telem.current_lap_time, sizeof(telem.current_lap_time));

        // 5. DELTA TO BEST
        if (t.containsKey("5")) {
            telem.delta_to_best = t["5"].as<float>();
            telem.delta_valid   = true;
        } else if (t.containsKey("5r0")) {
            telem.delta_to_best = t["5r0"].as<float>();
            telem.delta_valid   = true;
        }

        // 6. LAST LAP TIME
        parse_time_str("6",   telem.last_lap_time, sizeof(telem.last_lap_time));
        parse_time_str("6r0", telem.last_lap_time, sizeof(telem.last_lap_time));

        // 7. BEST LAP TIME
        parse_time_str("7",   telem.best_lap_time, sizeof(telem.best_lap_time));
        parse_time_str("7r0", telem.best_lap_time, sizeof(telem.best_lap_time));

        // 8. CURRENT LAP
        if (t.containsKey("8"))        telem.current_lap = t["8"].as<int>();
        else if (t.containsKey("8r0")) telem.current_lap = t["8r0"].as<int>();

        // 9. POSITION
        if (t.containsKey("9"))        telem.position = t["9"].as<int>();
        else if (t.containsKey("9r0")) telem.position = t["9r0"].as<int>();

        // 10. FUEL PERCENT
        if (t.containsKey("10"))        telem.fuel_pct = t["10"].as<float>();
        else if (t.containsKey("10r0")) telem.fuel_pct = t["10r0"].as<float>();

        // 11. BRAKE BIAS
        if (t.containsKey("11"))        telem.brake_bias = t["11"].as<float>();
        else if (t.containsKey("11r0")) telem.brake_bias = t["11r0"].as<float>();

        // 12. TC LEVEL
        if (t.containsKey("12")) {
            telem.tc_level = t["12"].as<int>();
            telem.tc_available = (telem.tc_level > 0);
        } else if (t.containsKey("12r0")) {
            telem.tc_level = t["12r0"].as<int>();
            telem.tc_available = (telem.tc_level > 0);
        }

        // 13. ABS LEVEL
        if (t.containsKey("13")) {
            telem.abs_level = t["13"].as<int>();
            telem.abs_available = (telem.abs_level > 0);
        } else if (t.containsKey("13r0")) {
            telem.abs_level = t["13r0"].as<int>();
            telem.abs_available = (telem.abs_level > 0);
        }

        // 14. ENGINE MAP
        if (t.containsKey("14"))        telem.engine_map = t["14"].as<int>();
        else if (t.containsKey("14r0")) telem.engine_map = t["14r0"].as<int>();

        // 15. PIT LIMITER
        int pit_state = parse_bool("15");
        if (pit_state < 0) pit_state = parse_bool("15r0");
        if (pit_state >= 0) {
            telem.pit_limiter_on = (pit_state == 1);
            telem.pit_limiter_available = true;
        }

        // 16-19. TYRE PRESSURES
        if (t.containsKey("16"))        telem.fl.pressure = t["16"].as<float>();
        else if (t.containsKey("16r0")) telem.fl.pressure = t["16r0"].as<float>();

        if (t.containsKey("17"))        telem.fr.pressure = t["17"].as<float>();
        else if (t.containsKey("17r0")) telem.fr.pressure = t["17r0"].as<float>();

        if (t.containsKey("18"))        telem.rl.pressure = t["18"].as<float>();
        else if (t.containsKey("18r0")) telem.rl.pressure = t["18r0"].as<float>();

        if (t.containsKey("19"))        telem.rr.pressure = t["19"].as<float>();
        else if (t.containsKey("19r0")) telem.rr.pressure = t["19r0"].as<float>();

        // 20-23. TYRE TEMPERATURES
        if (t.containsKey("20"))        telem.fl.temp_c = t["20"].as<float>();
        else if (t.containsKey("20r0")) telem.fl.temp_c = t["20r0"].as<float>();

        if (t.containsKey("21"))        telem.fr.temp_c = t["21"].as<float>();
        else if (t.containsKey("21r0")) telem.fr.temp_c = t["21r0"].as<float>();

        if (t.containsKey("22"))        telem.rl.temp_c = t["22"].as<float>();
        else if (t.containsKey("22r0")) telem.rl.temp_c = t["22r0"].as<float>();

        if (t.containsKey("23"))        telem.rr.temp_c = t["23"].as<float>();
        else if (t.containsKey("23r0")) telem.rr.temp_c = t["23r0"].as<float>();

        // 24-27. BRAKE TEMPERATURES
        if (t.containsKey("24"))        telem.fl.brake_c = t["24"].as<float>();
        else if (t.containsKey("24r0")) telem.fl.brake_c = t["24r0"].as<float>();

        if (t.containsKey("25"))        telem.fr.brake_c = t["25"].as<float>();
        else if (t.containsKey("25r0")) telem.fr.brake_c = t["25r0"].as<float>();

        if (t.containsKey("26"))        telem.rl.brake_c = t["26"].as<float>();
        else if (t.containsKey("26r0")) telem.rl.brake_c = t["26r0"].as<float>();

        if (t.containsKey("27"))        telem.rr.brake_c = t["27"].as<float>();
        else if (t.containsKey("27r0")) telem.rr.brake_c = t["27r0"].as<float>();

        // 28. MAX RPM
        if (t.containsKey("28"))        telem.max_rpm = t["28"].as<int>();
        else if (t.containsKey("28r0")) telem.max_rpm = t["28r0"].as<int>();

        // 29. HEADLIGHTS
        int light_state = parse_bool("29");
        if (light_state < 0) light_state = parse_bool("29r0");
        if (light_state >= 0) {
            telem.headlights_on = (light_state == 1);
            telem.lights_available = true;
        }

        // 30. AIR TEMPERATURE
        if (t.containsKey("30"))        telem.air_temperature = t["30"].as<float>();
        else if (t.containsKey("30r0")) telem.air_temperature = t["30r0"].as<float>();

        // 31. WATER TEMPERATURE
        if (t.containsKey("31"))        telem.water_temperature = t["31"].as<float>();
        else if (t.containsKey("31r0")) telem.water_temperature = t["31r0"].as<float>();

        // 32. OIL TEMPERATURE
        if (t.containsKey("32"))        telem.oil_temperature = t["32"].as<float>();
        else if (t.containsKey("32r0")) telem.oil_temperature = t["32r0"].as<float>();

        // 33. OIL PRESSURE
        if (t.containsKey("33"))        telem.oil_pressure = t["33"].as<float>();
        else if (t.containsKey("33r0")) telem.oil_pressure = t["33r0"].as<float>();

        // 34. FUEL PRESSURE
        if (t.containsKey("34"))        telem.fuel_pressure = t["34"].as<float>();
        else if (t.containsKey("34r0")) telem.fuel_pressure = t["34r0"].as<float>();

        // 35. ROAD TEMPERATURE
        if (t.containsKey("35"))        telem.road_temperature = t["35"].as<float>();
        else if (t.containsKey("35r0")) telem.road_temperature = t["35r0"].as<float>();

        // 36. SAFETY FLAG
        parse_str("36",   telem.flag_name, sizeof(telem.flag_name));
        parse_str("36r0", telem.flag_name, sizeof(telem.flag_name));

        // 37. TC SLIP ANGLE
        if (t.containsKey("37"))        telem.tc_slip = t["37"].as<int>();
        else if (t.containsKey("37r0")) telem.tc_slip = t["37r0"].as<int>();

        // 38. TC POWER CUT
        if (t.containsKey("38"))        telem.tc_cut = t["38"].as<int>();
        else if (t.containsKey("38r0")) telem.tc_cut = t["38r0"].as<int>();

        // 39. SESSION TYPE
        parse_str("39",   telem.session_type, sizeof(telem.session_type));
        parse_str("39r0", telem.session_type, sizeof(telem.session_type));

        // 40. TOTAL LAPS
        if (t.containsKey("40"))        telem.total_laps = t["40"].as<int>();
        else if (t.containsKey("40r0")) telem.total_laps = t["40r0"].as<int>();

        // 41. TC ACTIVE
        int tc_act = parse_bool("41");
        if (tc_act < 0) tc_act = parse_bool("41r0");
        if (tc_act >= 0) telem.tc_active = (tc_act == 1);

        // 42. ABS ACTIVE
        int abs_act = parse_bool("42");
        if (abs_act < 0) abs_act = parse_bool("42r0");
        if (abs_act >= 0) telem.abs_active = (abs_act == 1);

        // 43. LIGHT FLASH
        int flash_state = parse_bool("43");
        if (flash_state < 0) flash_state = parse_bool("43r0");
        if (flash_state >= 0) telem.highbeam_flash_on = (flash_state == 1);

        // 44. WIPERS
        int wiper_state = parse_bool("44");
        if (wiper_state < 0) wiper_state = parse_bool("44r0");
        if (wiper_state >= 0) {
            telem.wipers_on = (wiper_state == 1);
            telem.wipers_available = true;
        }

        // 45. TURN INDICATOR LEFT
        int turn_l = parse_bool("45");
        if (turn_l < 0) turn_l = parse_bool("45r0");
        if (turn_l >= 0) telem.turn_indicator_left = (turn_l == 1);

        // 46. TURN INDICATOR RIGHT
        int turn_r = parse_bool("46");
        if (turn_r < 0) turn_r = parse_bool("46r0");
        if (turn_r >= 0) telem.turn_indicator_right = (turn_r == 1);

        // 47. SPOTTER LEFT
        int spot_l = parse_bool("47");
        if (spot_l < 0) spot_l = parse_bool("47r0");
        if (spot_l >= 0) telem.spotter_left = (spot_l == 1);

        // 48. SPOTTER RIGHT
        int spot_r = parse_bool("48");
        if (spot_r < 0) spot_r = parse_bool("48r0");
        if (spot_r >= 0) telem.spotter_right = (spot_r == 1);

        // 49. OPPONENT BEHIND DISTANCE
        if (t.containsKey("49"))        telem.opponent_behind_dist = t["49"].as<float>();
        else if (t.containsKey("49r0")) telem.opponent_behind_dist = t["49r0"].as<float>();

        // 50. OPPONENT BEHIND GAP
        if (t.containsKey("50"))        telem.opponent_behind_gap = t["50"].as<float>();
        else if (t.containsKey("50r0")) telem.opponent_behind_gap = t["50r0"].as<float>();

        // 51. OPPONENT LATERAL OFFSET
        if (t.containsKey("51"))        telem.opponent_behind_offset = t["51"].as<float>();
        else if (t.containsKey("51r0")) telem.opponent_behind_offset = t["51r0"].as<float>();

        // 52. ROAD CURVATURE
        if (t.containsKey("52"))        telem.road_curvature = t["52"].as<float>();
        else if (t.containsKey("52r0")) telem.road_curvature = t["52r0"].as<float>();

        // 53. OPPONENT SPEED
        if (t.containsKey("53"))        telem.opponent_behind_speed = t["53"].as<float>();
        else if (t.containsKey("53r0")) telem.opponent_behind_speed = t["53r0"].as<float>();

        // 54. OPPONENT AHEAD DISTANCE
        if (t.containsKey("54"))        telem.opponent_ahead_dist = t["54"].as<float>();
        else if (t.containsKey("54r0")) telem.opponent_ahead_dist = t["54r0"].as<float>();

        // 55. OPPONENT AHEAD GAP
        if (t.containsKey("55"))        telem.opponent_ahead_gap = t["55"].as<float>();
        else if (t.containsKey("55r0")) telem.opponent_ahead_gap = t["55r0"].as<float>();

        // 56. TRACK PROGRESS PERCENT
        auto parse_norm_progress = [&](const char* k) {
            if (!t.containsKey(k)) return false;
            float val = t[k].as<float>();
            telem.track_progress_pct = (val > 1.0f) ? (val / 100.0f) : val;
            return true;
        };
        if (!parse_norm_progress("56")) parse_norm_progress("56r0");

        // 57. ERS BATTERY CHARGE PERCENT
        if (t.containsKey("57"))        telem.ers_charge_pct = t["57"].as<float>();
        else if (t.containsKey("57r0")) telem.ers_charge_pct = t["57r0"].as<float>();

        // 58. ERS DEPLOYMENT MODE / MAP
        if (t.containsKey("58"))        telem.ers_mode = t["58"].as<int>();
        else if (t.containsKey("58r0")) telem.ers_mode = t["58r0"].as<int>();

        // 59. MGU-K RECOVERY LEVEL
        if (t.containsKey("59"))        telem.mguk_recovery = t["59"].as<int>();
        else if (t.containsKey("59r0")) telem.mguk_recovery = t["59r0"].as<int>();

        // 60. MGU-K DELIVERY MODE
        if (t.containsKey("60"))        telem.mguk_delivery = t["60"].as<int>();
        else if (t.containsKey("60r0")) telem.mguk_delivery = t["60r0"].as<int>();

        // 61. ERS ENERGY DEPLOYED THIS LAP (%)
        if (t.containsKey("61"))        telem.ers_lap_deployed = t["61"].as<float>();
        else if (t.containsKey("61r0")) telem.ers_lap_deployed = t["61r0"].as<float>();

        // 62. CAR CLASS (Respects Manual Override)
        if (app_config.class_override_idx == 0) {
            parse_str("62",   telem.car_class, sizeof(telem.car_class));
            parse_str("62r0", telem.car_class, sizeof(telem.car_class));
        }

        // 64. GAME NAME (Respects Manual Override)
        if (app_config.game_override_idx == 0) {
            parse_str("64",   telem.game_name, sizeof(telem.game_name));
            parse_str("64r0", telem.game_name, sizeof(telem.game_name));
        }

        // 65. CAR MODEL / NAME
        parse_str("65",   telem.car_name, sizeof(telem.car_name));
        parse_str("65r0", telem.car_name, sizeof(telem.car_name));
    }
}