#pragma once

#include <map>
#include <list>
#include <stack>
#include <mutex>
#include <thread>
#include <memory>
#include <atomic>
#include <cstdint>
#include <imguipack.h>
#include <ezlibs/ezTools.hpp>
#include <ezlibs/ezFdGraph.hpp>
#include <ezlibs/ezNamedPipe.hpp>
#include <ezlibs/ezCmdProcessor.hpp>
#include <ezlibs/ezAABB.hpp>

struct VisualNodeDatas : public ez::FdGraph::NodeDatas {
    float radius = 1.0f;
    std::string tag;
    ImVec4 color;
    VisualNodeDatas() = default;
};

class Controller {
private:
    enum class LinkingMode {  //
        NONE = 0,
        ONE,
        TWO,
        MANY,
        Count
    } m_linkingMode{LinkingMode::NONE};
    bool m_firstDraw = true;
    bool m_drawGrid = true;
    bool m_drawScales = false;
    ez::fAABB m_aabb;
    float m_mouseRadius = 100.0f;
    ImU32 m_mouseColor = 0;
    ImU32 m_poximityColor = 0;
    ez::fvec2 m_cursorPos;
    ez::FdGraph m_fdGraph;
    std::mutex m_mutex;
    bool m_wasPressed = false;
    bool m_wasDragging = false;
    std::list<ez::FdGraph::NodeWeak> m_linkableNodes;
    std::list<std::pair<ez::FdGraph::NodeWeak, float>> m_tmpLinkableNodes;
    ez::CmdProcessor m_cmdProcessor;
    ez::NamedPipe::Server::Ptr m_serverPtr;
    std::atomic<bool> m_threadWorking{true};
    std::thread m_namedPipeControllerThread;
    std::stack<std::string> m_cmdStack;

public:
    bool init();
    void unit();
    void update();
    bool drawMenu(float vMaxWidth, ImCanvas& vCanvas);
    bool drawStatusControl(float vMaxWidth);
    void drawGraph(ImCanvas& vCanvas);
    void drawCursor(ImCanvas& vCanvas);
    void drawDialogs(const ImVec2& vScreenSize);

private:
    void m_saveToSvgFile(const std::string& vFilePathName);
    void m_startNamedPipeServer();
    void m_namedPipeServerWorker();
    void m_createNode(const ez::fvec2& vNodePos);
    void m_moveCursor(const ez::fvec2& vCursorPos);
    void m_buildLinkableNodes();
    void m_drawLinkableNodes(ImDrawList* vDrawnListPtr, ImCanvas& vCanvas);
    void m_createLinks(const ez::FdGraph::NodeWeak& vNode);

public:  // singleton
    static Controller* Instance() {
        static Controller _instance;
        return &_instance;
    }
};
