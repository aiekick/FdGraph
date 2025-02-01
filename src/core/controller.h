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
#include <ezlibs/ezXmlConfig.hpp>

class ContrastedFileDialog : public IGFD::FileDialog {
protected:
    bool m_Selectable(int vRowIdx, const char* vLabel, bool vSelected, ImGuiSelectableFlags vFlags, const ImVec2& vSizeArg) override;
    void m_drawColumnText(int vColIdx, const char* vLabel, bool /*vSelected*/, bool /*vHovered*/) override;

public:
    // Singleton for easier accces form anywhere but only one dialog at a time
    // vCopy or vForce can be used for share a memory pointer in a new memory space like a dll module
    static ContrastedFileDialog* Instance() {
        static ContrastedFileDialog _instance;
        return &_instance;
    }
};

struct VisualNodeDatas : public ez::FdGraph::NodeDatas {
    float size = 1.0f;
    std::string tag;
    ImVec4 color;
    VisualNodeDatas() = default;
};

class Controller : public ez::xml::Config {
private:
    ImCanvas m_canvas;
    enum class LinkingMode {  //
        NONE = 0,
        ONE,
        TWO,
        MANY,
        Count
    };
    struct Datas {
        LinkingMode m_linkingMode{LinkingMode::NONE};
        bool m_drawGrid{true};
        bool m_drawScales{false};
        float m_mouseRadius{100.0f};
        ImVec4 m_mouseColorV4{ImVec4(1, 1, 0, 1)};
        ImVec4 m_poximityColorV4{ImVec4(0.25, 1, 0, 1)};
        float m_nodeScaleFactor{2.5f};
        float m_linkScaleFactor{2.5f};
        float m_defaultLerpValue{0.2f};
        float m_incrementLerpValue{0.02f};
    } m_datas;
    static Datas DefaultDatas;
    bool m_firstDraw = true;
    ez::fAABB m_aabb;
    ImU32 m_mouseColor = 0;
    ImU32 m_poximityColor = 0;
    ez::fvec2 m_cursorPos;
    ez::FdGraph m_fdGraph;
    std::mutex m_mutex;
    bool m_wasPressed = false;
    bool m_wasDragging = false;
    bool m_drawNodes = true;
    bool m_drawLinks = true;
    float m_lerpValue{m_datas.m_defaultLerpValue};
    ez::FdGraph::NodeWeak m_closestNode;
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
    void clear();
    void update();
    bool drawMenu(float vMaxWidth);
    bool drawStatusControl(float vMaxWidth);

    void drawCanvas();
    void drawGraph();
    void drawCursor();
    void drawDialogs(const ImVec2& vScreenSize);

    ez::xml::Nodes getXmlNodes(const std::string& vUserDatas = "") override;
    bool setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) override;

private:
    ez::FdGraph::NodeWeak m_findClosestNode(const ez::fvec2& vCursorPos);
    float m_getMouseRadius() const;
    void m_namedPipeServerWorker();
    void m_createNode(const ez::fvec2& vNodePos);
    ez::FdGraph::NodeWeak m_createNode(const std::string& vTag);
    void m_moveCursor(const ez::fvec2& vCursorPos);
    void m_moveClosestNode();
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
