#pragma once
#include "WarcraftApi.hpp"

// Intercept the actual skinning transform, leaving unit height, pathing and native animation clocks intact.
class ActorJumpView {
public:
    using Hook=bool (*)(void*,void*,void**);
    void Install(uintptr_t base,Hook hook);
    void Begin(wc3::Handle unit,float feet);
    void End();
    void Reset() {unit_=0;model_=0;restore_=false;inputs_=false;drawing_=false;} // Discard invalid model pointers on map unload.
private:
    using Transform=void (__fastcall*)(uintptr_t,float*,float,float*,float*);
    static void __fastcall Apply(uintptr_t model,float* matrix,float alpha,float* first,float* second);
    void Lift();
    inline static ActorJumpView* context_=nullptr;
    inline static Transform original_=nullptr;
    wc3::Handle unit_=0;
    uintptr_t sprite_=0,model_=0;
    DWORD thread_=0;
    float lift_=0,alpha_=1,matrix_[12]={},first_[3]={},second_[3]={};
    bool restore_=false,inputs_=false,drawing_=false;
};
