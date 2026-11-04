#pragma once
#include "WarcraftApi.hpp"

// A native presentation flag is leased for one damage transaction; armor, triggers and death stay native.
namespace NativeExplosiveDeath {
void Configure(uintptr_t module);
void Reset();
bool Available();
#ifdef WCS_NATIVE_INTERACTIONS_TEST
// Synthetic backend injection is excluded from normal and gameplay-test Player builds.
void TestBinding(void (__cdecl* setter)(wc3::Handle,BOOL),unsigned offset,uint32_t mask);
#endif
class Lease {
public:
    Lease(wc3::Handle actor,wc3::Handle victim,bool enabled);
    ~Lease();
    void Release();
    Lease(const Lease&)=delete;
    Lease& operator=(const Lease&)=delete;
private:
    wc3::Handle victim_=0;uintptr_t object_=0;unsigned generation_=0;int type_=0;bool restore_=false;
};
}
