#pragma once

#include <vector>
#include <thread>
#include <memory>
#include <atomic>
#include <cstdint>
#include <imguipack.h>
#include <ezlibs/ezTools.hpp>
#include <ezlibs/ezFdGraph.hpp>
#include <ezlibs/ezNamedPipe.hpp>

typedef std::vector<ImVec2> P2dArray;
typedef std::vector<int32_t> IntArray;

class Controller {
private:
    float m_mouseRadius = 100.0f;
    ImU32 m_mouseColor = 0;
    ez::FdGraph m_fdGraph;
    std::unique_ptr<ez::NamedPipe> m_namedPipeClientPtr;

public:
    bool init();
    void unit();
    bool drawInput(float vMaxWidth);
    bool drawControl(float vMaxWidth);
    void drawGraph();
    void drawCursor();

private:
    void m_createNode(const ez::fvec2& vNodePos);

public:  // singleton
    static Controller* Instance() {
        static Controller _instance;
        return &_instance;
    }
};
