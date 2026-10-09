#pragma once

// Limits bound diagnostic disk/CPU work; F8 replaces this complete snapshot.
struct LoggingSettings {
    bool detailed = true;
    unsigned intervalMs = 1000;
    unsigned maxFileMB = 8;
    unsigned archiveCount = 3;
};
