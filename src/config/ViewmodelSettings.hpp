#pragma once

// F8 replaces this visual-only snapshot; camera, aim and movement physics never consume it.
struct ViewmodelSettings {
    bool bob=true,muzzleFlash=true;
    float bobAmount=1,bobFrequency=1.8f,bobResponse=12;
    unsigned flashMilliseconds=65;
    float flashScale=1;
};
