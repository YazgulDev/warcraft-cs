#pragma once

// Limits bound diagnostic disk/CPU work; F8 replaces this complete snapshot.
struct LoggingSettings {
    bool detailed = true;
    // Graphics failures are independent of optional TRACE summaries; F8 applies the observer toggle.
    bool graphicsDebug = true;
    unsigned intervalMs = 1000;
    unsigned maxFileMB = 8;
    unsigned archiveCount = 3;
};
