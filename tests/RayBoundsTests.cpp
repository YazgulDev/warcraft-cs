#include "../src/geometry/RayBounds.hpp"
#include "../src/geometry/ModelTransform.hpp"
#include <cstdio>
#include <cstdlib>

static void Require(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
int main() {
    // Exercise missed mounted/flying surfaces, strict range, parallel rays and ordering independently of Warcraft.
    Bounds3 horse = {{-80,-52,0},{100,52,210}};
    float direction[] = {1,0,0}, origin[] = {-300,48,160}, entry = 0;
    Require(IntersectBounds(horse, origin, direction, 400, entry) && std::abs(entry-220)<0.001f,
        "horse/rider upper edge must hit the front surface");
    Require(!IntersectBounds(horse, origin, direction, 219, entry), "range ends before the surface");
    origin[1] = 53;
    Require(!IntersectBounds(horse, origin, direction, 400, entry), "parallel ray outside the width must miss");
    Bounds3 flyer = {{-70,-120,180},{70,120,290}};
    float vertical[] = {0,0,1}, below[] = {0,0,96};
    Require(IntersectBounds(flyer, below, vertical, 300, entry) && std::abs(entry-84)<0.001f,
        "vertical aim must hit a flying model at its actual height");
    float inside[] = {0,0,220};
    Require(IntersectBounds(flyer, inside, direction, 100, entry) && entry==0,
        "very close model containing the muzzle must still register");
    float away[] = {-1,0,0}; origin[1] = 0;
    Require(!IntersectBounds(horse, origin, away, 400, entry), "model behind the muzzle must miss");
    float front[] = {-300,0,100};
    Require(IntersectBounds(horse, front, direction, 230, entry), "wide model enters range before its center");
    float limit = entry;
    Bounds3 behind = {{0,-52,0},{120,52,210}};
    Require(!IntersectBounds(behind, front, direction, limit, entry), "nearer model must occlude a target behind it");
    // A two-times elven gate with independent height scale must include both leaves and the upper panels.
    ModelTransform gate;
    float matrix[]={0,-2,0,2,0,0,0,0,1.5f};
    std::copy(matrix,matrix+9,gate.matrix);
    gate.position[0]=1984;gate.position[1]=-6592;gate.position[2]=128;
    Bounds3 panels={{-58,-337,-19},{53,334,268}};
    float ray[]={0,1,0}, localOrigin[3],localDirection[3];
    for (float side : {-600.f,0.f,600.f}) {
        for(float height : {20.f,200.f,380.f}) {
            float point[]={1984+side,-7092,128+height};
            Require(gate.LocalRay(point,ray,localOrigin,localDirection),"rendered gate transform must be invertible");
            Require(IntersectBounds(panels,localOrigin,localDirection,1000,entry)&&std::abs(entry-384)<.01f,
                "full scaled gate leaves must hit at the same world distance, including their upper panels");
        }
    }
    float beyond[]={1984+700,-7092,300};
    Require(gate.LocalRay(beyond,ray,localOrigin,localDirection)&&
        !IntersectBounds(panels,localOrigin,localDirection,1000,entry),"space beyond the visible gate must miss");
    // A 45-degree native row-vector matrix must not turn into the opposite-facing diagonal gate.
    float c=std::sqrt(.5f),nativeMatrix[]={2*c,2*c,0,-2*c,2*c,0,0,0,1.5f};
    gate.RendererMatrix(nativeMatrix);
    float diagonal[]={c,c,0};
    for(float side : {-600.f,600.f}) {
        float point[]={1984-500*c-side*c,-6592-500*c+side*c,350};
        Require(gate.LocalRay(point,diagonal,localOrigin,localDirection)&&
            IntersectBounds(panels,localOrigin,localDirection,1000,entry)&&std::abs(entry-384)<.02f,
            "native diagonal gate basis must cover both visible leaves at the proper world distance");
    }
    gate.matrix[8]=0;
    Require(!gate.LocalRay(beyond,ray,localOrigin,localDirection),"degenerate models cannot produce false hits");
    std::puts("Hitbox invariants passed: rider, flyer, range, occlusion, scaled gate leaves/height, empty space");
}
