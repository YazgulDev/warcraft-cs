#pragma once
#include <algorithm>
#include <cmath>

struct UnitHudLayout {
    int x=0,y=0,portrait=0,bars=0,gap=0;
    static UnitHudLayout Calculate(int width,int height,float percent) {
        if(width<320 || height<240)return {};
        // Keep the full block inside narrow windows, including the largest editable scale.
        int pad=std::max(12,int(height*.022f)),gap=std::max(4,int(height*.006f));
        int side=std::max(32,std::min(int(std::lround(height*.096f*percent/100)),(width-2*pad-gap)/4));
        return {pad,height-pad-side,side,side*5/2,gap};
    }
};
