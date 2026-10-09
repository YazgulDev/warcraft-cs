#include "../src/presentation/ReticleView.hpp"
#include <windows.h>
#include <gl/GL.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

static void Require(bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}
static int GreenPixels(int width, int height) {
    std::vector<unsigned char> pixels(size_t(width) * height * 4);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    int count = 0;
    for (size_t i = 0; i < pixels.size(); i += 4)
        if (pixels[i + 1] > 200 && pixels[i] < 180 && pixels[i + 2] < 120) ++count;
    return count;
}
static void Projection(int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT);
}
int main() {
    // A hidden native WGL window tests actual rasterized pixels without taking over the user's input.
    HWND window = CreateWindowA("STATIC", "Reticle regression", WS_POPUP, 0, 0, 640, 480, nullptr, nullptr, nullptr, nullptr);
    Require(window != nullptr, "Cannot create WGL test window");
    HDC dc = GetDC(window);
    PIXELFORMATDESCRIPTOR format = {};
    format.nSize = sizeof(format); format.nVersion = 1;
    format.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL; format.iPixelType = PFD_TYPE_RGBA;
    format.cColorBits = 32; format.iLayerType = PFD_MAIN_PLANE;
    int pixelFormat = ChoosePixelFormat(dc, &format);
    Require(pixelFormat && SetPixelFormat(dc, pixelFormat, &format), "Cannot set WGL pixel format");
    HGLRC context = wglCreateContext(dc);
    Require(context && wglMakeCurrent(dc, context), "Cannot create WGL context");
    std::printf("GL renderer: %s\n", glGetString(GL_RENDERER));

    Projection(320, 240);
    // This inherited state makes the original GL_LINES path draw zero pixels on a valid GL context.
    glEnable(GL_LINE_STIPPLE); glLineStipple(1, 0); glLineWidth(2); glColor4f(.5f, 1, .25f, 1);
    glBegin(GL_LINES); glVertex2f(145, 120); glVertex2f(155, 120); glEnd();
    Require(GreenPixels(320, 240) == 0, "Original line-stipple failure was not reproduced");

    for (int width : {320, 321, 640}) {
        int height = width == 640 ? 480 : 240;
        Projection(width, height);
        ReticleView::DrawHipFire(float(width), float(height), 0);
        Require(GreenPixels(width, height) == 80, "Filled crosshair must retain four ten-by-two-pixel arms");
        Projection(width, height);
        // Poison independent polygon/color state too; drawing must restore the game's original values.
        unsigned char emptyPattern[128] = {};
        glPolygonStipple(emptyPattern); glEnable(GL_POLYGON_STIPPLE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); glEnable(GL_COLOR_LOGIC_OP); glLogicOp(GL_NOOP);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        ReticleView::DrawHipFire(float(width), float(height), 2);
        Require(GreenPixels(width, height) == 80, "Inherited masks must not hide recoil-expanded crosshair");
        GLboolean mask[4]; GLint modes[2];
        glGetBooleanv(GL_COLOR_WRITEMASK, mask); glGetIntegerv(GL_POLYGON_MODE, modes);
        Require(!mask[0] && !mask[1] && !mask[2] && !mask[3] && modes[0] == GL_LINE && modes[1] == GL_LINE,
            "Reticle failed to restore Warcraft color/polygon state");
        Require(glIsEnabled(GL_LINE_STIPPLE) && glIsEnabled(GL_POLYGON_STIPPLE) && glIsEnabled(GL_COLOR_LOGIC_OP),
            "Reticle failed to restore Warcraft enables");
        glDisable(GL_POLYGON_STIPPLE); glDisable(GL_COLOR_LOGIC_OP); glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    Projection(320, 240); glClearColor(1, 1, 1, 1); glClear(GL_COLOR_BUFFER_BIT);
    ReticleView::DrawScope(160, 120, 80, 2);
    unsigned char center[4], outside[4];
    glReadPixels(160, 120, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, center);
    glReadPixels(170, 130, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, outside);
    Require(center[0] == 0 && center[1] == 0 && center[2] == 0 && outside[0] == 255,
        "Scope hairs must survive zero line stipple without filling the clear lens");
    Require(glGetError() == GL_NO_ERROR, "Reticle emitted an OpenGL error");
    wglMakeCurrent(nullptr, nullptr); wglDeleteContext(context); ReleaseDC(window, dc); DestroyWindow(window);
    std::puts("Reticle rasterization/state regression passed.");
}
