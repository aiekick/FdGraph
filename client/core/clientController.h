#pragma once

#include <vector>
#include <thread>
#include <memory>
#include <atomic>
#include <cstdint>
#include <imguipack.h>
#include <ezlibs/ezTime.hpp>
#include <ezlibs/ezTools.hpp>
#include <ezlibs/ezFdGraph.hpp>
#include <ezlibs/ezNamedPipe.hpp>
#include <ezlibs/ezCmdProcessor.hpp>

class ClientController {
private:
    ez::CmdProcessor m_cmdProcessor;
    ez::NamedPipe::Client::Ptr m_clientPtr;
    ez::fvec2 m_lastCursorPos;

public:
    bool init();
    void unit();
    bool drawInput(float vMaxWidth);
    bool drawControl(float vMaxWidth);
    void drawGraph();
    void drawCursor();

private:
    void m_createNode(const ez::fvec2& vNodePos);
    void m_moveCursor(const ez::fvec2& vCursorPos);

public:  // singleton
    static ClientController* Instance() {
        static ClientController _instance;
        return &_instance;
    }
};
