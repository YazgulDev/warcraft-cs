#include "../src/presentation/ReticleView.hpp"
#include "../src/presentation/ReticleDiagnostics.hpp"
#include "../src/platform/DiagnosticLog.hpp"
#include <windows.h>
#include <gl/GL.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <iterator>
#include <string>

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
static std::string ReadLog(const char* directory) {
    std::ifstream stream(std::string(directory) + "\\WarcraftCS.log", std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}
int main(int argc, char** argv) {
    Require(argc == 2, "An isolated journal directory is required");
    LoggingSettings logging; logging.intervalMs = 60000;
    DiagnosticLog::Configure(logging); DiagnosticLog::Open(argv[1]);
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
    // Skip and mode transitions must be recorded immediately despite the long sample interval.
    ReticleDiagnostics::Skip("unit-hidden", 320, 240);
    std::string skipped = ReadLog(argv[1]);
    ReticleDiagnostics::Skip("unit-hidden", 320, 240);
    Require(ReadLog(argv[1]) == skipped, "Repeated skip must respect diagnostic interval");
    Projection(320, 240); glClearColor(1, 1, 1, 1); glClear(GL_COLOR_BUFFER_BIT);
    ReticleView::DrawScope(160, 120, 80, 2);
    unsigned char center[4], outside[4];
    glReadPixels(160, 120, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, center);
    glReadPixels(170, 130, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, outside);
    Require(center[0] == 0 && center[1] == 0 && center[2] == 0 && outside[0] == 255,
        "Scope hairs must survive zero line stipple without filling the clear lens");
    Require(glGetError() == GL_NO_ERROR, "Reticle emitted an OpenGL error");

    Projection(320, 240);
    // A pending host error and poisoned inherited state must be diagnosed separately from our valid quads.
    glEnable(GL_POLYGON_STIPPLE); glEnable(GL_COLOR_LOGIC_OP); glLogicOp(GL_NOOP);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glEnable(0xFFFFFFFF);
    ReticleView::DrawHipFire(320, 240, 2);
    Require(GreenPixels(320, 240) == 80, "Diagnostic sampling changed crosshair pixels");
    std::string journal = ReadLog(argv[1]);
    Require(journal.find("mode=hip-fire") != std::string::npos &&
        journal.find("center=160.00,120.00 gap=13.00 thickness=2.00 recoil=2.00") != std::string::npos &&
        journal.find("quads=4 vertices=16") != std::string::npos, "Hip-fire geometry diagnostics missing");
    Require(journal.find("mode=scope") != std::string::npos && journal.find("radius=80.00") != std::string::npos &&
        journal.find("quads=2 vertices=8") != std::string::npos, "Scope transition diagnostics missing");
    Require(journal.find("phase=inherited viewport=0,0,320,240 scissor=") != std::string::npos &&
        journal.find("colorMask=0000") != std::string::npos && journal.find("colorMask=1111") != std::string::npos &&
        journal.find("phase=prepared") != std::string::npos, "Inherited/prepared state diagnostics missing");
    Require(journal.find("phase=before-draw-host code=0500") != std::string::npos &&
        journal.find("observedErrors=0 submission=completed") != std::string::npos &&
        journal.find("phase=draw-and-restore code=") == std::string::npos, "Host GL error attribution failed");
    ReticleView::DrawHipFire(320, 240, 2);
    Require(ReadLog(argv[1]) == journal, "Repeated draw must respect diagnostic interval");
    logging.detailed = false; DiagnosticLog::Configure(logging);
    // Disabled diagnostics must neither write transition records nor consume the game's GL error flag.
    glEnable(0xFFFFFFFF); ReticleView::DrawScope(160, 120, 80, 2);
    ReticleDiagnostics::Skip("controller-fault");
    Require(glGetError() == GL_INVALID_ENUM, "Disabled diagnostics consumed the host error");
    Require(ReadLog(argv[1]) == journal, "Detailed=false must suppress reticle diagnostic transitions");
    wglMakeCurrent(nullptr, nullptr);
    logging.detailed = true; DiagnosticLog::Configure(logging);
    ReticleDiagnostics::Skip("no-gl-context");
    Require(ReadLog(argv[1]).find("reticle skipped reason=no-gl-context") != std::string::npos,
        "Missing-context skip must work without GL state queries");
    DiagnosticLog::Close();
    wglDeleteContext(context); ReleaseDC(window, dc); DestroyWindow(window);
    std::puts("Reticle pixels/state, draw/skip diagnostics, GL error attribution and sampling passed.");
}
