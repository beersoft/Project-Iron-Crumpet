// ntp_clock.h - Rev 1.0.0
// Real-Time NTP Clock synchronization for the Idle Page
#pragma once

#include <Arduino.h>
#include <time.h>

inline void init_ntp_time(long gmt_offset_sec = 0, int daylight_offset_sec = 3600) {
    configTime(gmt_offset_sec, daylight_offset_sec, "pool.ntp.org", "time.nist.gov");
    Serial.println("[NTP] Synchronizing time with NTP servers...");
}

inline bool get_current_time_str(char* time_buf, size_t buf_len, char* date_buf, size_t date_len) {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo, 100)) {
        snprintf(time_buf, buf_len, "--:--:--");
        snprintf(date_buf, date_len, "NTP SYNCING...");
        return false;
    }
    strftime(time_buf, buf_len, "%H:%M:%S", &timeinfo);
    strftime(date_buf, date_len, "%A, %B %d %Y", &timeinfo);
    return true;
}
