#pragma once
#include "GraphicsApi.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>

// Legacy attribute stacks do not own GLSL programs or separate program pipelines.
// Each mod pass uses fixed-function vertices; an inherited shader can silently discard them.
class FixedFunctionState {
public:
    FixedFunctionState() {
        if(GraphicsBackend::IsD3D9() || !wglGetCurrentContext())return;
        int major=1,minor=1;
        if(auto version=reinterpret_cast<const char*>(glGetString(GL_VERSION)))sscanf_s(version,"%d.%d",&major,&minor);
        auto extensions=reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
        auto has=[&](const char* name){return extensions && std::strstr(extensions,name);};
        if(major>=2) {
            use_=Proc<Use>("glUseProgram");
            if(use_){glGetIntegerv(0x8B8D,&program_);use_(0);}
        }
        // CURRENT_PROGRAM may be zero while a fragment shader in a bound pipeline is still active.
        if(major>4 || (major==4 && minor>=1) || has("GL_ARB_separate_shader_objects")) {
            pipeline_=Proc<Use>("glBindProgramPipeline");
            if(pipeline_){glGetIntegerv(0x825A,&pipelineName_);pipeline_(0);}
        }
        vertex_=has("GL_ARB_vertex_program");fragment_=has("GL_ARB_fragment_program");
        discard_=major>=3 || has("GL_EXT_transform_feedback");
        if(vertex_){vertexEnabled_=glIsEnabled(0x8620);glDisable(0x8620);}
        if(fragment_){fragmentEnabled_=glIsEnabled(0x8804);glDisable(0x8804);}
        if(discard_){discardEnabled_=glIsEnabled(0x8C89);glDisable(0x8C89);}
    }
    ~FixedFunctionState() {
        // Restore explicitly: glPopAttrib does not restore modern program bindings.
        if(discard_)Enable(0x8C89,discardEnabled_);
        if(fragment_)Enable(0x8804,fragmentEnabled_);
        if(vertex_)Enable(0x8620,vertexEnabled_);
        if(pipeline_)pipeline_(GLuint(pipelineName_));
        if(use_)use_(GLuint(program_));
    }
    FixedFunctionState(const FixedFunctionState&)=delete;
    FixedFunctionState& operator=(const FixedFunctionState&)=delete;
private:
    using Use=void (APIENTRY*)(GLuint);
    template<class T>static T Proc(const char* name) {
        auto value=wglGetProcAddress(name);auto address=reinterpret_cast<uintptr_t>(value);
        return address>3 && address!=UINTPTR_MAX ? reinterpret_cast<T>(value) : nullptr;
    }
    static void Enable(GLenum state,bool enabled){if(enabled)glEnable(state);else glDisable(state);}
    Use use_=nullptr,pipeline_=nullptr;
    GLint program_=0,pipelineName_=0;
    bool vertex_=false,fragment_=false,discard_=false;
    bool vertexEnabled_=false,fragmentEnabled_=false,discardEnabled_=false;
};
