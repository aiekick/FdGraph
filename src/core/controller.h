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
    ImCanvas m_canvas;
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
    bool m_drawNodes = true;
    bool m_drawLinks = true;
    std::list<ez::FdGraph::NodeWeak> m_linkableNodes;
    std::list<std::pair<ez::FdGraph::NodeWeak, float>> m_tmpLinkableNodes;
    ez::CmdProcessor m_cmdProcessor;
    ez::NamedPipe::Server::Ptr m_serverPtr;
    std::atomic<bool> m_threadWorking{true};
    std::thread m_namedPipeControllerThread;
    std::stack<std::string> m_cmdStack;
    std::unordered_map<std::string, ez::FdGraph::NodeWeak> m_nodesByTags;

public:
    bool init();
    void unit();
    void update();
    bool drawMenu(float vMaxWidth);
    bool drawStatusControl(float vMaxWidth);

    void drawCanvas();
    void drawGraph();
    void drawCursor();
    void drawDialogs(const ImVec2& vScreenSize);

private:
    float m_getMouseRadius() const;
    void m_namedPipeServerWorker();
    void m_createNode(const ez::fvec2& vNodePos);
    ez::FdGraph::NodeWeak m_createNode(const std::string& vTag);
    void m_moveCursor(const ez::fvec2& vCursorPos);
    void m_buildLinkableNodes();
    void m_drawLinkableNodes(ImDrawList* vDrawnListPtr);
    void m_createLinks(const ez::FdGraph::NodeWeak& vNode);
    void m_startStopServer();
    void m_startStopClient();
    void m_importCsvFile(const std::string& vFilePathName);
    void m_exportCsvFile(const std::string& vFilePathName);
    void m_exportSvgFile(const std::string& vFilePathName);

public:  // singleton
    static Controller* Instance() {
        static Controller _instance;
        return &_instance;
    }
};
