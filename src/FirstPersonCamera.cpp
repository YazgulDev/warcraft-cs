#include "FirstPersonCamera.hpp"
#include "WarcraftApi.hpp"
#include "LookAngles.hpp"
#include "MapCameraGuard.hpp"
#include <algorithm>
#include <cmath>

void FirstPersonCamera::Update(float eyeX, float eyeY, float eyeZ, float yaw, float pitch, float fov) const {
    constexpr float radians = 0.01745329252f;
    float zero = 0, distance = 100, farZ = 5000;
    // The same normalized angles drive camera fields, movement and weapon rays after any number of full turns.
    float angle = LookAngles::Normalize(pitch), rotation = LookAngles::Normalize(yaw);
    // Cancel Warcraft's terrain contribution, rather than treating ZOFFSET as eye height.
    float terrainBaseline = wc3::Real(wc3::GetCameraTargetPositionZ()) - wc3::Real(wc3::GetCameraField(6));
    wc3::SetCameraField(0, &distance, &zero);
    distance = std::max(1.0f, wc3::Real(wc3::GetCameraField(0)));
    // The native minimum distance is not zero: aim the target ahead by the actual
    // distance so the camera eye stays at the player's collision-tested position.
    float horizontal = distance * std::cos(pitch * radians);
    float targetX = eyeX + horizontal * std::cos(yaw * radians);
    float targetY = eyeY + horizontal * std::sin(yaw * radians);
    float offsetZ = eyeZ + distance * std::sin(pitch * radians) - terrainBaseline;
    MapCameraGuard::PositionEyeTarget(&targetX, &targetY);
    wc3::SetCameraField(1, &farZ, &zero); wc3::SetCameraField(2, &angle, &zero);
    wc3::SetCameraField(3, &fov, &zero); wc3::SetCameraField(4, &zero, &zero);
    wc3::SetCameraField(5, &rotation, &zero); wc3::SetCameraField(6, &offsetZ, &zero);
}
