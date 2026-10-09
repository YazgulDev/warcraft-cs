#include "SpriteTransform.hpp"

bool SpriteTransform::Read(uintptr_t sprite, ModelTransform& transform) {
    if (!sprite) return false;
    uintptr_t table=0, matrixGetter=0, positionGetter=0; SIZE_T got=0;
    auto read=[&](uintptr_t address,void* value,size_t length) {
        return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),value,length,&got)&&got==length;
    };
    if (!read(sprite,&table,4)||!read(table+0x38,&matrixGetter,4)||!read(table+0x3C,&positionGetter,4) ||
        !matrixGetter || !positionGetter) return false;
    // These verified CSprite virtual getters both take output buffers; cached unit offsets are not shared.
    using Getter=float* (__thiscall*)(uintptr_t,float*);
    float matrix[9]={};
    reinterpret_cast<Getter>(matrixGetter)(sprite,matrix);
    // Convert the native row-vector basis before inversion, including gates placed at oblique angles.
    transform.RendererMatrix(matrix);
    reinterpret_cast<Getter>(positionGetter)(sprite,transform.position);
    float origin[3]={},direction[3]={1,0,0},localOrigin[3],localDirection[3];
    return transform.LocalRay(origin,direction,localOrigin,localDirection);
}
