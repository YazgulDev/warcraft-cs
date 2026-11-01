#pragma once

// Active native waygates use map-authored destinations; proximity never invents a campaign exit.
struct PortalSettings {
    bool enabled=true;
    float radius=160,heightTolerance=160,cooldownSeconds=1;
};
