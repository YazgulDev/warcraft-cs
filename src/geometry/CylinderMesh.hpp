#pragma once
#include "RayTriangle.hpp"
#include "ModelTransform.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <vector>

// Static object narrow phase. A cylinder touches surfaces at its own height, not the
// enclosing box of a whole arch, column or staircase. No native movement rules live here.
class CylinderMesh {
public:
    std::vector<Triangle3> triangles;
    CylinderMesh Transformed(const ModelTransform& transform) const {
        CylinderMesh result;result.triangles=triangles;
        for(auto& triangle:result.triangles)for(auto& point:triangle.vertices) {
            float local[3]={point[0],point[1],point[2]};
            for(int row=0;row<3;++row) {
                point[row]=transform.position[row];
                for(int col=0;col<3;++col)point[row]+=transform.matrix[row*3+col]*local[col];
            }
        }
        const auto* m=transform.matrix;
        float determinant=m[0]*(m[4]*m[8]-m[5]*m[7])-m[1]*(m[3]*m[8]-m[5]*m[6])+m[2]*(m[3]*m[7]-m[4]*m[6]);
        // Mirrored native bases must retain outward winding for floor/ceiling classification.
        if(determinant<0)for(auto& triangle:result.triangles)for(int axis=0;axis<3;++axis)
            std::swap(triangle.vertices[1][axis],triangle.vertices[2][axis]);
        return result;
    }
    // Highest upward-facing surface under any part of the circular foot; never use a remote tower top.
    bool Floor(float x,float y,float radius,float reachable,float minimumNormal,float& height,float* normal=nullptr,bool clipReachable=true) const {
        bool found=false;
        for(const auto& triangle:triangles) {
            float n[3];Normal(triangle,n);
            if(n[2]<minimumNormal)continue;
            float low,high;
            // A sloping triangle can cross the feet even when its uphill end is overhead.
            // Keep the reachable part instead of discarding that entire support surface.
            Polygon polygon;
            for(const auto& point:triangle.vertices)polygon.push_back({point[0],point[1],point[2]});
            // Keep the historical whole-triangle test when the player selects classic movement.
            if(clipReachable)polygon=Clip(polygon,reachable+.001f,false);
            if(Heights(triangle,polygon,x,y,radius,low,high) && (clipReachable || high<=reachable+.01f) && (!found || high>height)) {
                height=high;found=true;
                if(normal)for(int axis=0;axis<3;++axis)normal[axis]=n[axis];
            }
        }
        return found;
    }
    bool Ceiling(float x,float y,float radius,float feet,float& height) const {
        bool found=false;
        for(const auto& triangle:triangles) {
            float n[3];Normal(triangle,n);
            if(n[2]>=-.0001f)continue;
            float low,high;
            if(Heights(triangle,x,y,radius,low,high) && low>feet+.01f && (!found || low<height)) {
                height=low;found=true;
            }
        }
        return found;
    }
    bool Contact(float x,float y,float radius,float bottom,float top,float* normal=nullptr) const {
        for(const auto& triangle:triangles) {
            float low[3],high[3];
            for(int axis=0;axis<3;++axis) {
                low[axis]=std::min({triangle.vertices[0][axis],triangle.vertices[1][axis],triangle.vertices[2][axis]});
                high[axis]=std::max({triangle.vertices[0][axis],triangle.vertices[1][axis],triangle.vertices[2][axis]});
            }
            // Reject distant triangles before allocating clipped polygons in the hot side sweep.
            if(high[0]<x-radius || low[0]>x+radius || high[1]<y-radius || low[1]>y+radius ||
                high[2]<=bottom+.01f || low[2]>=top-.01f)continue;
            Polygon polygon;
            for(const auto& point:triangle.vertices)polygon.push_back({point[0],point[1],point[2]});
            // Clip to the occupied height before projection: upper branches/arch lintels cannot widen the body.
            polygon=Clip(polygon,bottom+.01f,true);polygon=Clip(polygon,top-.01f,false);
            if(polygon.empty())continue;
            float nearestX=0,nearestY=0,distance=std::numeric_limits<float>::infinity();
            bool inside=Inside(polygon,x,y);
            for(size_t i=0;i<polygon.size();++i) {
                const auto& a=polygon[i];const auto& b=polygon[(i+1)%polygon.size()];
                float dx=b[0]-a[0],dy=b[1]-a[1],length=dx*dx+dy*dy;
                float t=length>0 ? std::clamp(((x-a[0])*dx+(y-a[1])*dy)/length,0.f,1.f) : 0;
                float px=a[0]+dx*t,py=a[1]+dy*t,d=std::hypot(x-px,y-py);
                if(d<distance){distance=d;nearestX=x-px;nearestY=y-py;}
            }
            if(!inside && distance>=radius-.001f)continue;
            if(normal) {
                float n[3];Normal(triangle,n);float length=std::hypot(n[0],n[1]);
                // Rounded exterior corners use the closest point; an existing overlap behind
                // a face must recover outward, never reinterpret its inward vector as an exit.
                if(!inside && distance>.0001f && nearestX*n[0]+nearestY*n[1]>=0) {
                    normal[0]=nearestX/distance;normal[1]=nearestY/distance;
                } else {
                    normal[0]=length>.0001f ? n[0]/length : 0;normal[1]=length>.0001f ? n[1]/length : 0;
                }
            }
            return true;
        }
        return false;
    }
private:
    using Point=std::array<float,3>;
    using Polygon=std::vector<Point>;
    static void Normal(const Triangle3& triangle,float* n) {
        float a[3],b[3];
        for(int i=0;i<3;++i){a[i]=triangle.vertices[1][i]-triangle.vertices[0][i];b[i]=triangle.vertices[2][i]-triangle.vertices[0][i];}
        n[0]=a[1]*b[2]-a[2]*b[1];n[1]=a[2]*b[0]-a[0]*b[2];n[2]=a[0]*b[1]-a[1]*b[0];
        float length=std::sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);
        if(length>0)for(int i=0;i<3;++i)n[i]/=length;
    }
    static Polygon Clip(const Polygon& source,float z,bool above) {
        Polygon result;
        if(source.empty())return result;
        auto a=source.back();bool aIn=above ? a[2]>=z : a[2]<=z;
        for(const auto& b:source) {
            bool bIn=above ? b[2]>=z : b[2]<=z;
            if(aIn!=bIn) {
                float t=(z-a[2])/(b[2]-a[2]);Point cut;
                for(int i=0;i<3;++i)cut[i]=a[i]+t*(b[i]-a[i]);
                result.push_back(cut);
            }
            if(bIn)result.push_back(b);
            a=b;aIn=bIn;
        }
        return result;
    }
    static bool Inside(const Polygon& polygon,float x,float y) {
        bool positive=false,negative=false;float area=0;
        for(size_t i=0;i<polygon.size();++i) {
            const auto& a=polygon[i];const auto& b=polygon[(i+1)%polygon.size()];
            float cross=(b[0]-a[0])*(y-a[1])-(b[1]-a[1])*(x-a[0]);
            positive=positive || cross>.0001f;negative=negative || cross<-.0001f;
            area+=a[0]*b[1]-a[1]*b[0];
        }
        return std::abs(area)>.0001f && !(positive && negative);
    }
    static bool Heights(const Triangle3& triangle,float x,float y,float radius,float& low,float& high) {
        Polygon polygon;
        for(const auto& point:triangle.vertices)polygon.push_back({point[0],point[1],point[2]});
        return Heights(triangle,polygon,x,y,radius,low,high);
    }
    static bool Heights(const Triangle3& triangle,const Polygon& polygon,float x,float y,float radius,float& low,float& high) {
        if(polygon.empty())return false;
        float n[3];Normal(triangle,n);
        if(std::abs(n[2])<.0001f)return false;
        bool found=false;low=std::numeric_limits<float>::infinity();high=-low;
        auto take=[&](float px,float py) {
            float z=triangle.vertices[0][2]-(n[0]*(px-triangle.vertices[0][0])+n[1]*(py-triangle.vertices[0][1]))/n[2];
            low=std::min(low,z);high=std::max(high,z);found=true;
        };
        // Extrema of a plane over a triangle intersected with a disk occur at a vertex,
        // an edge/circle crossing, or the circle's uphill/downhill tangent point.
        if(Inside(polygon,x,y))take(x,y);
        for(size_t i=0;i<polygon.size();++i) {
            const auto& a=polygon[i];const auto& b=polygon[(i+1)%polygon.size()];
            if(std::hypot(a[0]-x,a[1]-y)<=radius)take(a[0],a[1]);
            float dx=b[0]-a[0],dy=b[1]-a[1],ox=a[0]-x,oy=a[1]-y;
            float aa=dx*dx+dy*dy,bb=2*(ox*dx+oy*dy),cc=ox*ox+oy*oy-radius*radius;
            float discriminant=bb*bb-4*aa*cc;
            if(aa>.0001f && discriminant>=0)for(float sign:{-1.f,1.f}) {
                float t=(-bb+sign*std::sqrt(discriminant))/(2*aa);
                if(t>=0 && t<=1)take(a[0]+dx*t,a[1]+dy*t);
            }
        }
        float gradient=std::hypot(n[0],n[1]);
        if(gradient>.0001f)for(float sign:{-1.f,1.f}) {
            float px=x+sign*radius*n[0]/gradient,py=y+sign*radius*n[1]/gradient;
            if(Inside(polygon,px,py))take(px,py);
        }
        return found;
    }
};
