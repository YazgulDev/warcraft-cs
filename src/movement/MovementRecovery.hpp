#pragma once
#include <array>
#include <cstddef>
#include <cmath>

// A small history of actually visited grounded positions. Geometry validation belongs to
// the platform adapter and must run again before restoring any point from this history.
class MovementRecovery {
public:
    struct Point {float x=0,y=0,z=0;};
    void Reset(){count_=next_=0;}
    void Record(float x,float y,float z,float spacing) {
        if(!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))return;
        if(count_) {
            const auto& last=points_[(next_+points_.size()-1)%points_.size()];
            if(std::hypot(last.x-x,last.y-y)<spacing && std::abs(last.z-z)<spacing)return;
        }
        points_[next_]={x,y,z};next_=(next_+1)%points_.size();if(count_<points_.size())++count_;
    }
    template<class Validate>
    bool Find(float x,float y,float z,float maximumDistance,float minimumRise,Validate validate,Point& result) const {
        // Prefer recent points, but retain the upper bank when the current point is a lower pit floor.
        for(std::size_t i=0;i<count_;++i) {
            const auto& point=points_[(next_+points_.size()-1-i)%points_.size()];
            if(std::hypot(point.x-x,point.y-y)>maximumDistance || point.z<z+minimumRise ||
                (std::hypot(point.x-x,point.y-y)<.01f && std::abs(point.z-z)<.01f))continue;
            if(validate(point)){result=point;return true;}
        }
        return false;
    }
private:
    std::array<Point,32> points_={};
    std::size_t count_=0,next_=0;
};
