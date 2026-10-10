#pragma once

// Own only the temporary FPS override; the map retains its ordinary fog/mask preferences.
class NativeFogOfWar {
public:
    void Update(bool disabled);
    // Map unload invalidates the snapshot; never restore an old map's settings into the new one.
    void Reset() { applied_=false; }
private:
    bool applied_=false, fog_=true, mask_=true;
};
