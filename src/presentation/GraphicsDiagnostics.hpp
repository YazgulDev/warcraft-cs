#pragma once
#include "GraphicsApi.hpp"
#include "../config/LoggingSettings.hpp"

// One graphics-thread scope observes driver messages and drains errors at named mod-pass boundaries.
class GraphicsDiagnostics {
public:
    explicit GraphicsDiagnostics(const LoggingSettings& settings);
    ~GraphicsDiagnostics();
    void Phase(const char* next);
    static bool Result(const char* operation,long result,unsigned parameter=0,unsigned value=0);
    GraphicsDiagnostics(const GraphicsDiagnostics&)=delete;
    GraphicsDiagnostics& operator=(const GraphicsDiagnostics&)=delete;
private:
    using Callback=void (APIENTRY*)(GLenum,GLenum,GLuint,GLenum,GLsizei,const char*,const void*);
    using Install=void (APIENTRY*)(Callback,const void*);
    static void APIENTRY Driver(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const char* message,const void* user);
    void Drain();
    const char* phase_="host-before-overlay";
    HGLRC context_=nullptr;
    Install install_=nullptr;
    Callback previous_=nullptr;
    const void* previousUser_=nullptr;
    bool enabled_=false,khr_=false,output_=false,synchronous_=false,trace_=false,driverError_=false;
};
