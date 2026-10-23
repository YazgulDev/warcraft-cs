#include "GameBuild.hpp"
#include "ActorJumpView.hpp"
#include <cmath>
#include <cstring>

void ActorJumpView::Install(uintptr_t base,Hook hook) {
    // This adapter requires engine internals that are not yet verified on 1.27a.
    if(wc3::ActiveBuild()==wc3::GameBuildKind::Classic127a) return;

    // CSprite's native 4D4594 call passes matrix, alpha and two 3-vectors to the full skinning update.
    const unsigned char signature[]={0xD9,0x44,0x24,0x04,0x53,0xDC,0x25};
    bool valid=!std::memcmp(reinterpret_cast<void*>(base + wc3::Rva(0x50D240)),signature,sizeof(signature));
    bool installed=valid && hook(reinterpret_cast<void*>(base + wc3::Rva(0x50D240)),reinterpret_cast<void*>(Apply),reinterpret_cast<void**>(&original_));
    if(installed)context_=this;
    wc3::Log("Actor jump skinning adapter verified=%d installed=%d",valid,installed);
}
void ActorJumpView::Begin(wc3::Handle unit,float feet,bool refreshPose) {
    End();if(!unit || !original_){Reset();return;}
    float x=wc3::Real(wc3::GetUnitX(unit)),y=wc3::Real(wc3::GetUnitY(unit));
    float lift=feet-wc3::Ground(x,y);auto object=wc3::UnitObject(unit);
    if(!object || !std::isfinite(lift)){Reset();return;}
    auto sprite=*reinterpret_cast<uintptr_t*>(object+0x28);if(!sprite){Reset();return;}
    auto model=*reinterpret_cast<uintptr_t*>(sprite+0x20);if(!model){Reset();return;}
    if(unit!=unit_ || model!=model_)Reset();
    unit_=unit;sprite_=sprite;model_=model;lift_=lift;thread_=GetCurrentThreadId();
    // Main-body skinning happens before RenderWorld. Reuse its latest native inputs, not a stale root matrix.
    // The controller repairs attack time after native skinning has already run. Re-skin at that
    // corrected time even on the ground, otherwise rendered bones retain the reset first frames.
    drawing_=true;if(inputs_ && (refreshPose || std::abs(lift_)>=.01f))Lift();
}
void __fastcall ActorJumpView::Apply(uintptr_t model,float* matrix,float alpha,float* first,float* second) {
    auto view=context_;
    if(!view || model!=view->model_ || GetCurrentThreadId()!=view->thread_ || !matrix || !first || !second) {
        original_(model,matrix,alpha,first,second);return;
    }
    // Preserve the exact native draw inputs; one local matrix lifts the whole skinned body and attachments.
    std::memcpy(view->matrix_,matrix,sizeof(view->matrix_));
    std::memcpy(view->first_,first,sizeof(view->first_));std::memcpy(view->second_,second,sizeof(view->second_));view->alpha_=alpha;
    view->inputs_=true;
    if(view->drawing_ && std::abs(view->lift_)>=.01f)view->Lift();
    else original_(model,matrix,alpha,first,second);
}
void ActorJumpView::Lift() {
    float raised[12];std::memcpy(raised,matrix_,sizeof(raised));raised[11]+=lift_;
    original_(model_,raised,alpha_,first_,second_);restore_=true;
    static DWORD last=0;DWORD now=GetTickCount();
    if(now-last>=150) {last=now;wc3::Trace("Actor jump model lift=%.1f nativeFly=%.2f modelZ=%.1f",lift_,wc3::Real(wc3::GetUnitFlyHeight(unit_)),*reinterpret_cast<float*>(model_+0x90));}
}
void ActorJumpView::End() {
    auto object=restore_ && unit_ && wc3::GetUnitTypeId(unit_) ? wc3::UnitObject(unit_) : 0;
    // Restore through the same native skinning path after drawing, never through a unit movement/height setter.
    if(object && *reinterpret_cast<uintptr_t*>(object+0x28)==sprite_ &&
        *reinterpret_cast<uintptr_t*>(sprite_+0x20)==model_)
        original_(model_,matrix_,alpha_,first_,second_);
    restore_=false;drawing_=false; // Keep identity/input capture alive between world passes.
}
