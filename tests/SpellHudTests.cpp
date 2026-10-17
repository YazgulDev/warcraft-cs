#include "../src/platform/NativeAbilityInfo.hpp"
#include "../src/platform/NativeCommandCard.hpp"
#include "../src/platform/NativeAbilitySlots.hpp"
#include "../src/presentation/SpellCooldownWedge.hpp"
#include <windows.h>
#include "../src/platform/SpellIconImage.hpp"
#include "../src/presentation/SpellHudLayout.hpp"
#include <array>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>

template<class T> void Store(unsigned char* at,T value) {std::memcpy(at,&value,sizeof(value));}
int main() {
    static_assert(sizeof(uintptr_t)==4,"The supported native layout is x86.");
    std::array<unsigned char,256> ability={},data={},ui={},running={},clock={},locator={},type={},vtable={};
    std::array<const char*,2> arts={"map\\level1.blp","map\\level2.blp"};
    Store(ability.data()+0x54,uintptr_t(data.data()));Store(data.data()+0x28,uintptr_t(ui.data()));
    Store(ui.data()+0xB0,2u);Store(ui.data()+0xB4,uintptr_t(arts.data()));
    Store(vtable.data(),uintptr_t(locator.data()));Store(locator.data()+12,uintptr_t(type.data()));
    strcpy_s(reinterpret_cast<char*>(type.data()+8),type.size()-8,".?AVCAgentTimer@@");
    Store(ability.data()+0xD0,uintptr_t(vtable.data()+4));Store(ability.data()+0xDC,uintptr_t(running.data()));
    Store(running.data()+4,105.f);Store(running.data()+0xC,uintptr_t(clock.data()));Store(clock.data()+0x40,100.f);
    // Read the production pointer chain and simulation clock, including pause, reset and malformed state.
    SpellHudSlot slot;slot.level=2;NativeAbilityInfo::Read(uintptr_t(ability.data()),slot);
    assert(slot.icon==arts[1] && slot.cooldownKnown && slot.cooldown==5);
    Store(ability.data()+0xB4,10.f);Store(ability.data()+0x40,1);
    NativeAbilityInfo::Read(uintptr_t(ability.data()),slot);assert(slot.cooldownTotal==10 && slot.disabled);
    std::array<unsigned char,2*0x68> levelData={};
    Store(data.data()+0x50,2u);Store(data.data()+0x54,uintptr_t(levelData.data()));
    Store(levelData.data()+0x68+0x10,75);Store(levelData.data()+0x68+0x14,12.f);Store(ability.data()+0xB4,0.f);
    slot={};slot.level=2;NativeAbilityInfo::Read(uintptr_t(ability.data()),slot,0);assert(slot.noMana && slot.cooldownTotal==12);
    NativeAbilityInfo::Read(uintptr_t(ability.data()),slot,100);assert(!slot.noMana);
    // Native command art changes on toggles; an unrelated selected unit cannot replace this icon.
    std::array<unsigned char,0x500> gameUI={};std::array<unsigned char,0x180> bar={};
    std::array<uintptr_t,3> itemRows={};uintptr_t buttonsAt=0;
    std::array<unsigned char,12*0x1C0> buttons={};std::array<unsigned char,0x6D8> buttonData={};
    Store(gameUI.data()+0x3C8,uintptr_t(bar.data()));Store(bar.data()+0x154,uintptr_t(itemRows.data()));
    buttonsAt=uintptr_t(buttons.data());itemRows[2]=uintptr_t(&buttonsAt);
    Store(buttons.data()+11*0x1C0+0x190,uintptr_t(buttonData.data()));
    Store(buttonData.data()+4,77);Store(buttonData.data()+0x6D4,uintptr_t(ability.data()));Store(buttonData.data()+0x594,100);
    strcpy_s(reinterpret_cast<char*>(buttonData.data()+0x5C8),260,"map\\toggle-on.blp");slot.ability=77;
    assert(NativeCommandCard::Apply(uintptr_t(gameUI.data()),uintptr_t(ability.data()),50,slot) && slot.commandKnown && slot.noMana);
    assert(slot.icon=="map\\toggle-on.blp");
    strcpy_s(reinterpret_cast<char*>(buttonData.data()+0x5C8),260,"map\\toggle-off.blp");
    assert(NativeCommandCard::Apply(uintptr_t(gameUI.data()),uintptr_t(ability.data()),150,slot) && !slot.noMana && slot.icon=="map\\toggle-off.blp");
    Store(buttonData.data()+0x6D4,uintptr_t(ability.data())+4);
    assert(!NativeCommandCard::Apply(uintptr_t(gameUI.data()),uintptr_t(ability.data()),0,slot) && slot.icon=="map\\toggle-off.blp");
    for(float part:{.1f,.25f,.5f,1.f,2.f}) {
        auto wedge=SpellCooldownWedge(part,1);assert(wedge.size()>2);
        for(const auto& p:wedge)assert(p[0]>=-.0001f && p[0]<=1.0001f && p[1]>=-.0001f && p[1]<=1.0001f);
    }
    assert(SpellCooldownWedge(0,10).empty() && SpellCooldownWedge(NAN,10).empty());
    // A runtime ability replacement comes from the actor's live list, not the original research rawcode.
    auto nativeImage=static_cast<unsigned char*>(VirtualAlloc(nullptr,0xA80000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));assert(nativeImage);
    const unsigned char resolve[]={0x8B,0x01,0xC3};std::memcpy(nativeImage+0x4786B0,resolve,sizeof(resolve));
    std::array<unsigned char,0x200> actor={};Store(actor.data()+0x1DC,uintptr_t(ability.data()));
    Store(ability.data(),uintptr_t(nativeImage)+0x880000);Store(ability.data()+0x34,77);Store(ability.data()+0x24,uint32_t(-1));Store(ability.data()+0x28,uint32_t(-1));
    Store(ui.data()+0x50,0);Store(ui.data()+0x54,2);Store(ui.data()+0x64,0);
    auto live=NativeAbilitySlots::Read(uintptr_t(nativeImage),uintptr_t(actor.data()));assert(live[0]==77);
    Store(ability.data()+0x34,88);live=NativeAbilitySlots::Read(uintptr_t(nativeImage),uintptr_t(actor.data()));assert(live[0]==88);
    Store(ui.data()+0x64,1);live=NativeAbilitySlots::Read(uintptr_t(nativeImage),uintptr_t(actor.data()));assert(!live[0]);
    Store(ui.data()+0x64,0);Store(ability.data()+0x24,uintptr_t(ability.data()));Store(ability.data()+0x28,uint32_t(0));
    live=NativeAbilitySlots::Read(uintptr_t(nativeImage),uintptr_t(actor.data()));assert(live[0]==88); // Corrupt cycles terminate.
    // Readonly point/unit order flags use the verified x86 getter and an actor-owned command button.
    Store(buttonData.data()+0x6D4,uintptr_t(ability.data()));
    unsigned char flagsGetter[]={0x8B,0x44,0x24,0x04,0x56,0x8B,0xF1,0xB8,2,0,0,0,0x5E,0xC2,4,0};
    std::memcpy(nativeImage+0x372C30,flagsGetter,sizeof(flagsGetter));
    assert(NativeCommandCard::PointOnly(uintptr_t(nativeImage),uintptr_t(gameUI.data()),uintptr_t(ability.data()),77,852000));
    for(unsigned flags:{0u,4u,6u,8u}) {
        Store(nativeImage+0x372C30+8,flags);
        assert(!NativeCommandCard::PointOnly(uintptr_t(nativeImage),uintptr_t(gameUI.data()),uintptr_t(ability.data()),77,852000));
    }
    Store(nativeImage+0x372C30+8,2u);Store(buttonData.data()+0x6D4,uintptr_t(ability.data())+4);
    assert(!NativeCommandCard::PointOnly(uintptr_t(nativeImage),uintptr_t(gameUI.data()),uintptr_t(ability.data()),77,852000));
    Store(buttonData.data()+0x6D4,uintptr_t(ability.data()));nativeImage[0x372C30]=0;
    assert(!NativeCommandCard::PointOnly(uintptr_t(nativeImage),uintptr_t(gameUI.data()),uintptr_t(ability.data()),77,852000));
    VirtualFree(nativeImage,0,MEM_RELEASE);
    Store(ability.data()+0x24,0u);Store(ability.data()+0x28,0u);
    NativeAbilityInfo::Read(uintptr_t(ability.data()),slot);assert(slot.cooldown==5); // No wall-clock ticking during pause.
    Store(clock.data()+0x40,102.5f);NativeAbilityInfo::Read(uintptr_t(ability.data()),slot);assert(slot.cooldown==2.5f);
    Store(clock.data()+0x40,107.f);NativeAbilityInfo::Read(uintptr_t(ability.data()),slot);assert(!slot.cooldown);
    Store(running.data()+4,INFINITY);NativeAbilityInfo::Read(uintptr_t(ability.data()),slot);assert(!slot.cooldownKnown);
    type[8]='X';slot={};NativeAbilityInfo::Read(uintptr_t(ability.data()),slot);assert(!slot.cooldownKnown);
    // An authored two-pixel BLP exercises BGRA palette conversion and packed 4-bit alpha.
    std::vector<uint8_t> bytes(1183);auto word=[&](size_t at,uint32_t value){std::memcpy(bytes.data()+at,&value,4);};
    word(0,0x31504C42);word(4,1);word(8,4);word(12,2);word(16,1);word(28,1180);word(92,3);
    bytes[156+5*4]=7;bytes[156+5*4+1]=11;bytes[156+5*4+2]=31;bytes[1180]=bytes[1181]=5;bytes[1182]=0xAF;
    SpellIconImage image;assert(image.Decode(bytes));assert(image.width==2 && image.height==1);
    assert(image.pixels[0]==31 && image.pixels[1]==11 && image.pixels[2]==7 && image.pixels[3]==255 && image.pixels[7]==170);
    word(8,0);word(92,2);assert(image.Decode(bytes) && image.pixels[3]==255);
    word(28,0xFFFFFFFE);assert(!image.Decode(bytes));word(28,1180);word(12,0xFFFFFFFF);assert(!image.Decode(bytes));
    assert(!image.Decode({1,2,3}));
    // Authored solid-color JPEG verifies BLP's BGR conversion without distributing any game art.
    const std::vector<uint8_t> jpeg={0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0xFF, 0xDB, 0x00, 0x43, 0x00, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x04, 0x03, 0x02, 0x02, 0x02, 0x02, 0x05, 0x04, 0x04, 0x03, 0x04, 0x06, 0x05, 0x06, 0x06, 0x06, 0x05, 0x06, 0x06, 0x06, 0x07, 0x09, 0x08, 0x06, 0x07, 0x09, 0x07, 0x06, 0x06, 0x08, 0x0B, 0x08, 0x09, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x06, 0x08, 0x0B, 0x0C, 0x0B, 0x0A, 0x0C, 0x09, 0x0A, 0x0A, 0x0A, 0xFF, 0xDB, 0x00, 0x43, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x05, 0x03, 0x03, 0x05, 0x0A, 0x07, 0x06, 0x07, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0xFF, 0xC0, 0x00, 0x11, 0x08, 0x00, 0x08, 0x00, 0x08, 0x03, 0x01, 0x22, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11, 0x01, 0xFF, 0xC4, 0x00, 0x15, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0xFF, 0xC4, 0x00, 0x14, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xC4, 0x00, 0x15, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x09, 0xFF, 0xC4, 0x00, 0x14, 0x11, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xDA, 0x00, 0x0C, 0x03, 0x01, 0x00, 0x02, 0x11, 0x03, 0x11, 0x00, 0x3F, 0x00, 0x98, 0xE0, 0x29, 0x20, 0xAD, 0xFF, 0xD9};
    bytes.assign(160+jpeg.size(),0);word(0,0x31504C42);word(4,0);word(12,8);word(16,8);
    word(28,160);word(92,uint32_t(jpeg.size()));std::copy(jpeg.begin(),jpeg.end(),bytes.begin()+160);
    assert(image.Decode(bytes) && image.width==8 && image.height==8);
    assert(std::abs(int(image.pixels[0])-180)<5 && std::abs(int(image.pixels[2])-20)<5);
    assert(image.Decode(jpeg) && std::abs(int(image.pixels[0])-20)<5 && std::abs(int(image.pixels[2])-180)<5);
    for(auto size:{std::pair<int,int>{800,600},{1024,768},{1920,1080},{3840,2160},{200,160}})for(float percent:{4.f,7.f,14.f}) {
        auto at=SpellHudLayout::Calculate(size.first,size.second,percent);
        assert(at.side>0 && at.x>=0 && at.y>=0 && at.x+4*at.side+3*at.gap<=size.first && at.y+at.side<size.second-35);
    }
    assert(!SpellHudLayout::Calculate(100,100,7).side);
    std::puts("PASS live native cooldown/art reads, pause/reset/invalid timer handling, bounded BLP decoding and spell HUD layout.");
}
