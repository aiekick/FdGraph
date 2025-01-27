#include "controller.h"
#include <cmath>
#include <ostream>
#include <ezlibs/ezLog.hpp>
#include <ezlibs/ezImGui.hpp>
#include <res/fontIcons.h>

/* CLIENT

bool ClientController::init() {
    m_clientPtr = ez::NamedPipe::Client::create("FdGraph");
    return (m_clientPtr != nullptr);
}

void ClientController::unit() {
    m_clientPtr.reset();
}

bool ClientController::drawInput(float vMaxWidth) {
    const auto w = 120.0f;
return false;
}

bool ClientController::drawControl(float vMaxWidth) {
    bool change = false;
    return change;
}

void ClientController::drawGraph() {}

void ClientController::drawCursor() {
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&  //
        !ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        m_createNode(ImGui::GetMousePos());
    }
    m_moveCursor(ImGui::GetMousePos());
}

void ClientController::m_createNode(const ez::fvec2& vNodePos) {
    const auto msg = m_cmdProcessor.encode("CreateNode", vNodePos.array());
    if (!msg.empty()) {
        m_clientPtr->writeString(msg);
    }
}

void ClientController::m_moveCursor(const ez::fvec2& vCursorPos) {
    if (m_lastCursorPos != vCursorPos) {
        const auto msg = m_cmdProcessor.encode("MoveCursor", vCursorPos.array());
        if (!msg.empty()) {
            m_clientPtr->writeString(msg);
            m_lastCursorPos = vCursorPos;
        }
    }
}
*/

bool Controller::init() {
    m_mouseColor = ImGui::GetColorU32(ImVec4(1, 1, 0, 1));
    m_poximityColor = ImGui::GetColorU32(ImVec4(0.25, 1, 0, 1));
    m_fdGraph.getConfigRef().forceFactor = 500.0f;
    m_fdGraph.getConfigRef().centralGravityFactor = 5.0f;
    return true;
}

void Controller::unit() {
    m_threadWorking = false;
    if (m_namedPipeControllerThread.joinable()) {
        m_namedPipeControllerThread.join();
    }
}

bool Controller::drawMenu(float vMaxWidth, ImCanvas& vCanvas) {
    bool ret = false;

    ImGui::Separator();
    
    if (ImGui::ContrastedBeginMenu(ICON_FONT_TUNE " ##Tuning")) {
        if (m_drawGrid || m_drawScales) {
            ImGui::SliderFloatDefaultCompact(250.0f, "Major step X", &vCanvas.getConfigRef().gridSize.x, 1.0f, 200.0f, 50.0f, 1.0f);
            ImGui::SliderFloatDefaultCompact(250.0f, "Major step Y", &vCanvas.getConfigRef().gridSize.y, 1.0f, 200.0f, 50.0f, 1.0f);
            ImGui::SliderFloatDefaultCompact(250.0f, "Subdivs X", &vCanvas.getConfigRef().gridSubdivs.x, 0.0f, 50.0f, 5.0f, 1.0f);
            ImGui::SliderFloatDefaultCompact(250.0f, "Subdivs Y", &vCanvas.getConfigRef().gridSubdivs.y, 0.0f, 50.0f, 5.0f, 1.0f);
        }
        
        ImGui::Separator();

        ImGui::SliderFloatDefaultCompact(250.0f, "Gravity", &m_fdGraph.getConfigRef().centralGravityFactor, 0.0f, 5.0f, 1.1f, 1.0f);
        ImGui::SliderFloatDefaultCompact(250.0f, "Force", &m_fdGraph.getConfigRef().forceFactor, 0.0f, 2000.0f, 500.0f, 1.0f);

        ImGui::EndMenu();
    }

    ImGui::Separator();

    if (ImGui::ContrastedMenuItem(ICON_FONT_TRASH_CAN "##ResetCanvas", "Reset canvas")) {
        m_firstDraw = true;
    }

    if (ImGui::ContrastedMenuItem(ICON_FONT_IMAGE_FILTER_CENTER_FOCUS "##ZoomContent", "Zoom to content")) {
        vCanvas.zoomToContent(ImRect(m_aabb.lowerBound, m_aabb.upperBound));
        ret = true;
    }

    ImGui::Separator();

    ImGui::ContrastedMenuItem(ICON_FONT_GRID "##CanvasGrid", "Show grid", &m_drawGrid);
    ImGui::ContrastedMenuItem(ICON_FONT_RULER "##CanvasRulers", "Show rulers", &m_drawScales);

    ImGui::Separator();
    
    if (ImGui::ContrastedMenuItem(ICON_FONT_VECTOR_POINT "##NoLink", "No link", m_linkingMode == LinkingMode::NONE)) {
        m_linkingMode = LinkingMode::NONE;
        ret = true;
    }
    if (ImGui::ContrastedMenuItem(ICON_FONT_VECTOR_LINE "##OneLink", "One link", m_linkingMode == LinkingMode::ONE)) {
        m_linkingMode = LinkingMode::ONE;
        ret = true;
    }
    if (ImGui::ContrastedMenuItem(ICON_FONT_VECTOR_POLYLINE "##TwoLink", "Two links", m_linkingMode == LinkingMode::TWO)) {
        m_linkingMode = LinkingMode::TWO;
        ret = true;
    }
    if (ImGui::ContrastedMenuItem(ICON_FONT_VECTOR_POLYGON "##ManyLink", "Many links", m_linkingMode == LinkingMode::MANY)) {
        m_linkingMode = LinkingMode::MANY;
        ret = true;
    }
    ImGui::Separator();

    if (ImGui::ContrastedMenuItem(ICON_FONT_FLOPPY "##SaveSvg", "Save to svg")) {
        IGFD::FileDialogConfig config;
        config.flags = ImGuiFileDialogFlags_Modal;
        ImGuiFileDialog::Instance()->OpenDialog("SaveToSvg", "Save to svg", ".svg", config);
    }

    return ret;
}

bool Controller::drawStatusControl(float vMaxWidth) {
    bool change = false;
    return change;
}

void Controller::update() {
    if (m_namedPipeControllerThread.joinable()) {
        std::vector<std::string> arr;
        m_mutex.lock();
        arr.reserve(m_cmdStack.size());
        while (!m_cmdStack.empty()) {
            arr.push_back(m_cmdStack.top());
            m_cmdStack.pop();
        }
        m_mutex.unlock();
        for (const auto& a : arr) {
            if (!m_cmdProcessor.decode(a)) {
                LogVarDebugInfo("%s", "fail to decode received command");
            }
        }
    }
    m_fdGraph.updateForces(ImGui::GetIO().DeltaTime);
    m_aabb.Set(0.0f, 0.0f);
    for (auto& node_ptr : m_fdGraph.getNodes()) {
        auto& datas = node_ptr->getDatasRef<VisualNodeDatas>();
        datas.mass = 5.0f + 5.0f * static_cast<float>(datas.connCount);
        datas.radius = 1.5f + 0.2f * static_cast<float>(datas.connCount);
        m_aabb.Combine(datas.pos);
    }
}

void Controller::drawGraph(ImCanvas& vCanvas) {
    if (m_drawGrid) {
        vCanvas.drawGrid();
    }
    if (m_drawScales) {
        vCanvas.drawScales();
    }
    if (m_firstDraw) {
        vCanvas.resetView();
        m_firstDraw = false;
    }
    if (vCanvas.isHovered() && ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
        m_buildLinkableNodes();
    }
    auto *draw_list_ptr = ImGui::GetWindowDrawList();
    if (draw_list_ptr != nullptr) {
        for (const auto& link : m_fdGraph.getLinks()) {
            const auto& from_datas = link.from.lock()->getDatas();
            const auto& to_datas = link.to.lock()->getDatas();
            draw_list_ptr->AddLine(from_datas.pos, to_datas.pos, m_mouseColor);
        }
        for (const auto& node_ptr : m_fdGraph.getNodes()) {
            const auto& datas = node_ptr->getDatas<VisualNodeDatas>();
            draw_list_ptr->AddCircleFilled(datas.pos, datas.radius, ImGui::GetColorU32(datas.color), 8);
        }
        if (vCanvas.isHovered() && ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
            m_drawLinkableNodes(draw_list_ptr, vCanvas);
        }
    }
}

void Controller::drawCursor(ImCanvas& vCanvas) {
    if (vCanvas.isHovered() && ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
        auto* draw_list_ptr = ImGui::GetWindowDrawList();
        if (draw_list_ptr != nullptr) {
            draw_list_ptr->AddCircle(m_cursorPos, m_mouseRadius, m_mouseColor);
        }
        const auto mp = ImGui::GetMousePos();
        m_moveCursor(mp);
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            m_wasDragging = true;
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (!m_wasDragging) {
                m_createNode(m_cursorPos);
            }
            m_wasDragging = false;
        }
        vCanvas.suspend();
        // ImGui::SetTooltip("%.2f,%.2f", mp.x, mp.y);
        vCanvas.resume();
    }
}

void Controller::drawDialogs(const ImVec2& vScreenSize) {
    ImVec2 max = vScreenSize;
    ImVec2 min = max * 0.5f;
    if (ImGuiFileDialog::Instance()->Display("SaveToSvg", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, min, max)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            m_saveToSvgFile(ImGuiFileDialog::Instance()->GetFilePathName());
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

void Controller::m_startNamedPipeServer() {
    m_cmdProcessor.registerCmd(  //
        "CreateNode",
        [this](const ez::CmdProcessor::Command& vCmd, const ez::CmdProcessor::Arguments& vArgs) {  //
            m_createNode(vArgs);
        });
    m_cmdProcessor.registerCmd(  //
        "MoveCursor",
        [this](const ez::CmdProcessor::Command& vCmd, const ez::CmdProcessor::Arguments& vArgs) {  //
            m_moveCursor(vArgs);
        });
    m_threadWorking = true;
    m_namedPipeControllerThread = std::thread(&Controller::m_namedPipeServerWorker, this);
}

void Controller::m_namedPipeServerWorker() {
    auto server_ptr = ez::NamedPipe::Server::create("FdGraph", 64);
    if (server_ptr != nullptr) {
        LogVarDebugInfo("%s", "Controller \"FdGraph\" started");
        while (m_threadWorking) {
            if (server_ptr->isMessageReceived()) {
                const auto msg = server_ptr->readString();
                m_mutex.lock();
                m_cmdStack.push(msg);
                m_mutex.unlock();
            }
        }
        LogVarDebugInfo("%s", "Controller \"FdGraph\" endded");
        server_ptr.reset();
    }
}

void Controller::m_createNode(const ez::fvec2& vNodePos) {
    auto base_node_ptr = m_fdGraph.addNode(VisualNodeDatas()).lock();
    base_node_ptr->getDatasRef().pos = vNodePos;
    static int32_t color_idx = 0;
    static const int32_t color_idx_count = 100;
    base_node_ptr->getDatasRef<VisualNodeDatas>().color = ez::getRainBowColor(color_idx++, color_idx_count);
    m_createLinks(base_node_ptr);
}

void Controller::m_moveCursor(const ez::fvec2& vCursorPos) {
    m_cursorPos = vCursorPos;
}
void Controller::m_buildLinkableNodes() {
    m_linkableNodes.clear();
    if (m_linkingMode != LinkingMode::NONE) {
        if (!m_fdGraph.getNodes().empty()) {
            m_tmpLinkableNodes.clear();
            auto cursorPos = ez::fvec2(m_cursorPos);
            for (const auto& other_node_ptr : m_fdGraph.getNodes()) {
                float other_len = ez::length(other_node_ptr->getDatas().pos - cursorPos);
                if (other_len < m_mouseRadius) {
                    auto it = std::lower_bound(
                        m_tmpLinkableNodes.begin(),
                        m_tmpLinkableNodes.end(),
                        other_len,  //
                        [](const std::pair<ez::FdGraph::NodeWeak, float>& a, float b) { return a.second < b; });
                    m_tmpLinkableNodes.insert(it, std::make_pair(other_node_ptr, other_len));
                }
            }
            size_t idx = 0U;
            for (const auto& node : m_tmpLinkableNodes) {
                if ((m_linkingMode == LinkingMode::TWO && idx >= 2) || //
                    (m_linkingMode == LinkingMode::ONE && idx >= 1)) {
                    break;
                }
                m_linkableNodes.push_back(node.first);
                ++idx;
            }
        }
    }
}

void Controller::m_drawLinkableNodes(ImDrawList* vDrawnListPtr, ImCanvas& vCanvas) {
    if (!m_linkableNodes.empty()) {
        for (const auto& node : m_linkableNodes) {
            vDrawnListPtr->AddLine(m_cursorPos, node.lock()->getDatas().pos, m_poximityColor);
        }
        vDrawnListPtr->AddCircle(m_cursorPos, 3.0f, m_poximityColor);
    }
}

void Controller::m_createLinks(const ez::FdGraph::NodeWeak& vNode) {
    if (!m_linkableNodes.empty()) {
        for (const auto& node : m_linkableNodes) {
            m_fdGraph.addLink(node, vNode);
        }
        m_linkableNodes.clear();
    }
}

void Controller::m_saveToSvgFile(const std::string& vFilePathName) {
    EZ_TOOLS_DEBUG_BREAK;
}
