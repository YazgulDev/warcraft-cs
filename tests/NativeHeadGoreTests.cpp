// Include the actual adapter so synthetic native layouts can exercise its private palette boundary,
// without adding fixture injection APIs to ordinary game modules.
#include "../src/platform/NativeHeadGore.cpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace {
std::array<unsigned char,0x100> objectBytes={},spriteBytes={},entryBytes={};
std::array<unsigned char,0x200> modelBytes={},geometryBytes={};
std::array<float,36> sourceMatrices={},palettes={};
std::array<unsigned,3> counts={1,1,1};
std::array<unsigned,8> ids={0,1,2,0,0,0,0,0};
std::array<unsigned char,16> visibility={};
uintptr_t entryPointer=uintptr_t(entryBytes.data()),currentObject=uintptr_t(objectBytes.data());
float life=100;
void Store(uintptr_t at,uintptr_t value){std::memcpy(reinterpret_cast<void*>(at),&value,4);}
wc3::Bits __cdecl State(wc3::Handle,int){wc3::Bits bits;std::memcpy(&bits,&life,4);return bits;}
int __cdecl Type(wc3::Handle){return 123;}
BOOL __cdecl UnitType(wc3::Handle,int){return FALSE;}
void __stdcall NativeSkinStub(uintptr_t,uintptr_t){
    // Reproduce native matrix-group averaging, including blended neck/head vertices.
    std::array<float,36> result={};unsigned offset=0;
    for(unsigned group=0;group<counts.size();++group) {
        for(unsigned j=0;j<counts[group];++j) {
            if(ids[offset+j]>=3){palettes=sourceMatrices;return;}
            for(unsigned value=0;value<12;++value)result[group*12+value]+=sourceMatrices[ids[offset+j]*12+value]/counts[group];
        }
        offset+=counts[group];
    }
    palettes=result;
}
void ResetScene(){
    NativeHeadGore::Reset();life=100;currentObject=uintptr_t(objectBytes.data());
    counts={1,1,1};ids={0,1,2,0};
    Store(currentObject+0x28,uintptr_t(spriteBytes.data()));Store(uintptr_t(spriteBytes.data())+0x20,uintptr_t(modelBytes.data()));
    Store(uintptr_t(modelBytes.data())+0x0C,1);Store(uintptr_t(modelBytes.data())+0x10,uintptr_t(&entryPointer));
    visibility[3]=1;Store(uintptr_t(modelBytes.data())+0x20,uintptr_t(visibility.data()));
    Store(entryPointer+0x10,0);Store(uintptr_t(geometryBytes.data())+0x11C,0);
    Store(entryPointer+8,0);Store(entryPointer+0x0C,uintptr_t(geometryBytes.data()));
    Store(uintptr_t(geometryBytes.data())+0xF0,3);Store(uintptr_t(geometryBytes.data())+0xF4,uintptr_t(counts.data()));Store(uintptr_t(geometryBytes.data())+0x100,uintptr_t(ids.data()));
    sourceMatrices={};for(int i=0;i<3;++i){sourceMatrices[i*12]=sourceMatrices[i*12+4]=sourceMatrices[i*12+8]=1;sourceMatrices[i*12+11]=i ? 100.f : 0.f;}
    sourceMatrices[12+10]=-100;sourceMatrices[24+10]=100;palettes=sourceMatrices;
    NativeHeadGore::rigs[123]={{{1,1,{0,0,0}},{2,2,{0,0,0}}},{{0},{1},{2}}};
}
}
namespace wc3 {
Bits (__cdecl* GetUnitState)(Handle,int)=State;
int (__cdecl* GetUnitTypeId)(Handle)=Type;
BOOL (__cdecl* IsUnitType)(Handle,int)=UnitType;
uintptr_t UnitObject(Handle){return currentObject;}
std::string UnitModelPath(int){return {};}
float Real(Bits bits){float value;std::memcpy(&value,&bits,4);return value;}
void Log(const char*,...){}
void Trace(const char*,...){}
}
int main(){
    _set_error_mode(_OUT_TO_STDERR);
    using namespace NativeHeadGore;
    // A real writable arena verifies pointer/stride handling; no game addresses or binaries are used.
    auto arena=static_cast<unsigned char*>(VirtualAlloc(nullptr,0xAD0000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));assert(arena);
    gameBase=uintptr_t(arena);Store(gameBase+0xACBC68,uintptr_t(palettes.data()));originalSkin=NativeSkinStub;
    ResetScene();auto before=Prepare(7,{0,105,100},.35f,true,2);assert(before.target==7);assert(!Commit(before));
    life=0;assert(Commit(before));SkinCorpse(before.model,uintptr_t(sourceMatrices.data()));
    assert(corpses[before.model].contact.root==2);
    for(int i=0;i<24;++i)assert(palettes[i]==sourceMatrices[i]);
    for(int i=24;i<33;++i)assert(palettes[i]==0);assert(palettes[34]==100 && palettes[35]==100);
    // Resurrection regenerates the whole head; unrelated model instances are never modified.
    life=100;SkinCorpse(before.model,uintptr_t(sourceMatrices.data()));assert(palettes==sourceMatrices && corpses.empty());
    assert(!Prepare(7,{0,0,100},.35f,false,2).target);
    assert(!Prepare(7,{0,0,100},.35f,true,999).target);
    ResetScene();before=Prepare(7,{0,-105,100},.35f,true,1);life=0;assert(Commit(before));
    SkinCorpse(before.model+4,uintptr_t(sourceMatrices.data()));assert(palettes==sourceMatrices);
    // Removed/replaced native objects and malformed/unmapped bone groups cannot deform another unit.
    currentObject=0;SkinCorpse(before.model,uintptr_t(sourceMatrices.data()));assert(corpses.empty() && palettes==sourceMatrices);
    ResetScene();before=Prepare(7,{0,105,100},.35f,true,2);life=0;assert(Commit(before));ids[2]=9000;
    SkinCorpse(before.model,uintptr_t(sourceMatrices.data()));assert(palettes==sourceMatrices);ids[2]=2;
    // Invisible/disabled geosets retain stale native palette indices; these must never corrupt reused output.
    ResetScene();before=Prepare(7,{0,105,100},.35f,true,2);life=0;assert(Commit(before));visibility[3]=0;
    SkinCorpse(before.model,uintptr_t(sourceMatrices.data()));assert(palettes==sourceMatrices);
    visibility[3]=1;Store(entryPointer+0x10,1);SkinCorpse(before.model,uintptr_t(sourceMatrices.data()));assert(palettes==sourceMatrices);
    // A shared palette retains the second skull's native contribution; only the struck skull collapses.
    ResetScene();before=Prepare(7,{0,105,100},.35f,true,2);life=0;assert(Commit(before));
    counts[2]=2;ids={0,1,1,2};rigs[123].groups.push_back({1,2});
    SkinCorpse(before.model,uintptr_t(sourceMatrices.data()));
    assert(palettes[24]==.5f && palettes[28]==.5f && palettes[32]==.5f && palettes[34]==0 && palettes[35]==100);
    // Authoring head parents need not participate in native skinning (Crypt Fiend/Naga).
    // Their matrix slots can be stale; the used descendant is the only valid anchor source.
    ResetScene();rigs[123].heads={ {1,1,{0,0,0}}, {2,1,{0,0,0}} };
    ids={0,2,2};sourceMatrices[12+10]=99999;
    before=Prepare(7,{0,105,100},.35f,true,1);life=0;assert(Commit(before));
    SkinCorpse(before.model,uintptr_t(sourceMatrices.data()));assert(palettes[22]==100 && palettes[34]==100);
    // Below-threshold head weights still deform continuously; gameplay's .35 hit threshold is not a skin cutoff.
    for(unsigned bodyBones:{1u,2u}) {
        ResetScene();counts[2]=bodyBones+1;ids={0,1,1,0,0};
        rigs[123].groups.push_back(bodyBones==1 ? std::vector<unsigned>{0,1} : std::vector<unsigned>{0,0,1});
        before=Prepare(7,{0,-105,100},.35f,true,1);life=0;assert(Commit(before));SkinCorpse(before.model,uintptr_t(sourceMatrices.data()));
        float bodyWeight=float(bodyBones)/(bodyBones+1);assert(std::abs(palettes[24]-bodyWeight)<.00001f);
        assert(std::abs(palettes[34]-(-100.f/(bodyBones+1)))<.00001f);
        for(unsigned value=0;value<12;++value)assert(palettes[value]==sourceMatrices[value]);
    }
    // A malformed late mixed matrix cannot leave an earlier head palette partially modified.
    ResetScene();counts[2]=2;ids={0,1,1,2};rigs[123].groups.push_back({1,2});sourceMatrices[24]=std::numeric_limits<float>::quiet_NaN();
    before=Prepare(7,{0,-105,100},.35f,true,1);life=0;assert(Commit(before));
    SkinCorpse(before.model,uintptr_t(sourceMatrices.data()));assert(palettes[12]==1);
    Reset();assert(rigs.empty() && corpses.empty());VirtualFree(arena,0,MEM_RELEASE);
    std::puts("PASS head palette selection, two separate heads, unchanged body/other instances, survivor rejection, resurrection, stale identity and malformed data.");
}
