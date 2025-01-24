#pragma once

#include <vector>
#include <thread>
#include <memory>
#include <atomic>
#include <cstdint>
#include <imguipack.h>
#include <ezlibs/ezFdGraph.hpp>
#include <ezlibs/ezNamedPipe.hpp>

typedef std::vector<ImVec2> P2dArray;
typedef std::vector<int32_t> IntArray;

class Controller {
private:
    float m_mouseRadius = 100.0f;
    ImU32 m_mouseColor = 0;
    ez::FdGraph m_fdGraph;
    std::atomic<bool> m_threadWorking;
    std::thread m_namedPipeServerThread;

public:
    bool init();
    void unit();
    bool drawInput(float vMaxWidth);
    bool drawControl(float vMaxWidth);
    void drawGraph();
    void drawCursor();

private:
    void m_namedPipeServerWorker();

public:  // singleton
    static Controller* Instance() {
        static Controller _instance;
        return &_instance;
    }
};
