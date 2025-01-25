#pragma once

#include <mutex>
#include <vector>
#include <thread>
#include <memory>
#include <atomic>
#include <cstdint>
#include <imguipack.h>
#include <ezlibs/ezTools.hpp>
#include <ezlibs/ezFdGraph.hpp>
#include <ezlibs/ezNamedPipe.hpp>
#include <ezlibs/ezCmdProcessor.hpp>

class ServerController {
private:
    ImWidgets::QuickStringCombo m_linkNodeMode;
    float m_mouseRadius = 100.0f;
    ImU32 m_mouseColor = 0;
    ImU32 m_poximityColor = 0;
    ez::fvec2 m_cursorPos;
    ez::FdGraph m_fdGraph;
    std::mutex m_mutex;
    bool m_wasPressed = false;
    bool m_wasDragging = false;
    ez::FdGraph::NodeWeak m_shortestNode;
    ez::CmdProcessor m_cmdProcessor;
    ez::NamedPipe::Server::Ptr m_serverPtr;
    std::atomic<bool> m_threadWorking{true};
    std::thread m_namedPipeServerControllerThread;

public:
    bool init();
    void unit();
    bool drawInput(float vMaxWidth);
    bool drawControl(float vMaxWidth);
    void drawGraph(ImCanvas& vCanvas);
    void drawCursor(ImCanvas& vCanvas);

private:
    void m_namedPipeServerWorker();
    void m_createNode(const ez::fvec2& vNodePos);
    void m_moveCursor(const ez::fvec2& vCursorPos);

public:  // singleton
    static ServerController* Instance() {
        static ServerController _instance;
        return &_instance;
    }
};
