#pragma once
#include "WarcraftApi.hpp"
#include "../movement/MovementPhysics.hpp"
#include <algorithm>

// Collision owns the actor's world boundary; RTS camera limits must never define a walking wall.
struct MovementWorldBounds {
    float minX, maxX, minY, maxY;
    static MovementWorldBounds Read(float radius) {
        auto rect=wc3::GetWorldBounds();
        MovementWorldBounds bounds={wc3::Real(wc3::GetRectMinX(rect))+radius,
            wc3::Real(wc3::GetRectMaxX(rect))-radius,wc3::Real(wc3::GetRectMinY(rect))+radius,
            wc3::Real(wc3::GetRectMaxY(rect))-radius};
        wc3::RemoveRect(rect);
        return bounds;
    }
    void Constrain(wc3::Handle unit, MovementPhysics& movement, float& x, float& y) const {
        // Native orders/scripts may leave an already out-of-bounds actor. Repair even
        // with no input, stopping only outward motion and retaining motion along the edge.
        float boundedX=std::clamp(x,minX,maxX),boundedY=std::clamp(y,minY,maxY);
        if(boundedX!=x){
            if((x<minX && movement.VelocityX()<0) || (x>maxX && movement.VelocityX()>0))movement.BlockX();
            x=boundedX;wc3::SetUnitX(unit,&x);
        }
        if(boundedY!=y){
            if((y<minY && movement.VelocityY()<0) || (y>maxY && movement.VelocityY()>0))movement.BlockY();
            y=boundedY;wc3::SetUnitY(unit,&y);
        }
    }
};
