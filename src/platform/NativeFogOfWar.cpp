#include "NativeFogOfWar.hpp"
#include "WarcraftApi.hpp"

void NativeFogOfWar::Update(bool disabled) {
    if (!disabled) {
        // F6, menus and cinematics return both independent switches to their captured map state.
        if (applied_) {
            wc3::FogEnable(fog_);wc3::FogMaskEnable(mask_);applied_=false;
            wc3::Log("Fog of war restored fog=%d mask=%d",fog_,mask_);
        }
        return;
    }
    if (!applied_) {
        fog_=wc3::IsFogEnabled()!=FALSE;mask_=wc3::IsFogMaskEnabled()!=FALSE;
        applied_=true;
        wc3::Log("Fog of war disabled in FPS; saved fog=%d mask=%d",fog_,mask_);
    }
    // Suppress explored fog and unexplored black mask; map scripts may re-enable either during play.
    if (wc3::IsFogEnabled()) wc3::FogEnable(FALSE);
    if (wc3::IsFogMaskEnabled()) wc3::FogMaskEnable(FALSE);
}
