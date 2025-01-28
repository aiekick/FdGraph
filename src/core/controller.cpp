#include "controller.h"

#include <cmath>
#include <ostream>
#include <res/fontIcons.h>
#include <ezlibs/ezLog.hpp>
#include <ezlibs/ezCsv.hpp>
#include <ezlibs/ezSvg.hpp>
#include <ezlibs/ezImGui.hpp>

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
    m_canvas.getConfigRef().zoomFactor = 0.1f;
    m_canvas.getConfigRef().zoomStep = 0.1f;
    m_canvas.getConfigRef().draggingButton = ImGuiMouseButton_Right;
    return true;
}

void Controller::unit() {
    m_threadWorking = false;
    if (m_namedPipeControllerThread.joinable()) {
        m_namedPipeControllerThread.join();
    }
}

bool Controller::drawMenu(float vMaxWidth) {
    bool ret = false;

    ImGui::Separator();
    
    if (ImGui::ContrastedBeginMenu(ICON_FONT_TUNE " ##Tuning", "Tuning")) {
        if (m_drawGrid || m_drawScales) {
            ret |= ImGui::SliderFloatDefaultCompact(250.0f, "Major step X", &m_canvas.getConfigRef().gridSize.x, 1.0f, 200.0f, 50.0f, 1.0f);
            ret |= ImGui::SliderFloatDefaultCompact(250.0f, "Major step Y", &m_canvas.getConfigRef().gridSize.y, 1.0f, 200.0f, 50.0f, 1.0f);
            ret |= ImGui::SliderFloatDefaultCompact(250.0f, "Subdivs X", &m_canvas.getConfigRef().gridSubdivs.x, 0.0f, 50.0f, 5.0f, 1.0f);
            ret |= ImGui::SliderFloatDefaultCompact(250.0f, "Subdivs Y", &m_canvas.getConfigRef().gridSubdivs.y, 0.0f, 50.0f, 5.0f, 1.0f);

            ImGui::Separator();
        }
        
        ret |= ImGui::SliderFloatDefaultCompact(250.0f, "Gravity", &m_fdGraph.getConfigRef().centralGravityFactor, 0.0f, 200.0f, 1.1f, 1.0f);
        ret |= ImGui::SliderFloatDefaultCompact(250.0f, "Force", &m_fdGraph.getConfigRef().forceFactor, 0.0f, 100000.0f, 1000.0f, 1.0f);
        ret |= ImGui::SliderFloatDefaultCompact(250.0f, "Convergence speed", &m_fdGraph.getConfigRef().deltaTimeFactor, 0.0f, 20.0f, 2.0f, 1.0f);

        ImGui::EndMenu();
    }

    ImGui::Separator();

    if (ImGui::ContrastedBeginMenu(ICON_FONT_FOLDER " ##Import", "Import")) {
        if (ImGui::ContrastedMenuItem("Import from Csv")) {
            IGFD::FileDialogConfig config;
            config.flags = ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::Instance()->OpenDialog("ImportCsv", "Import from Csv", ".csv", config);
            ret = true;
        }
        ImGui::EndMenu();
    }
    if (ImGui::ContrastedBeginMenu(ICON_FONT_CONTENT_SAVE " ##Export", "Export")) {
        if (ImGui::ContrastedMenuItem("Export to Csv")) {
            IGFD::FileDialogConfig config;
            config.flags = ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::Instance()->OpenDialog("ExportToCsv", "Export to csv", ".csv", config);
            ret = true;
        }
        if (ImGui::ContrastedMenuItem("Export to Svg")) {
            IGFD::FileDialogConfig config;
            config.flags = ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::Instance()->OpenDialog("ExportToSvg", "Export to svg", ".svg", config);
            ret = true;
        }
        ImGui::EndMenu();
    }

    ImGui::Separator();

    if (ImGui::ContrastedBeginMenu(ICON_FONT_TRASH_CAN " ##Reset", "Reset")) {
        if (ImGui::ContrastedMenuItem("Canvas")) {
            m_firstDraw = true;
            ret = true;
        }
        if (ImGui::ContrastedMenuItem("Graph")) {
            m_fdGraph.clear();
            ret = true;
        }
        ImGui::EndMenu();
    }

    if (ImGui::ContrastedMenuItem(ICON_FONT_IMAGE_FILTER_CENTER_FOCUS "##ZoomContent", "Zoom to content")) {
        m_canvas.zoomToContent(ImRect(m_aabb.lowerBound, m_aabb.upperBound));
        ret = true;
    }

    ImGui::Separator();

    if (ImGui::ContrastedBeginMenu(ICON_FONT_EYE "##Visibility", "Visibility")) {
        ret |= ImGui::ContrastedMenuItem("Grid", nullptr, &m_drawGrid);
        ret |= ImGui::ContrastedMenuItem("Rulers", nullptr, &m_drawScales);

        ImGui::Separator();

        ret |= ImGui::ContrastedMenuItem("Nodes", nullptr, &m_drawNodes);
        ret |= ImGui::ContrastedMenuItem("Links", nullptr, &m_drawLinks);

        ImGui::EndMenu();
    }

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

    if (ImGui::ContrastedMenuItem(ICON_FONT_SERVER "##Server", "Server mode")) {
        ret = true;
        EZ_TOOLS_DEBUG_BREAK;
    }

    if (ImGui::ContrastedMenuItem(ICON_FONT_ACCESS_POINT "##Client", "Client mode")) {
        ret = true;
        EZ_TOOLS_DEBUG_BREAK;
    }

    ImGui::Separator();

    if (ImGui::ContrastedBeginMenu(ICON_FONT_BUG " ##Debug", "Debug view")) {
        
        ImGui::EndMenu();
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
        datas.color = ez::getRainBowColor(datas.connCount, 50);
        m_aabb.Combine(datas.pos);
    }
}

void Controller::drawCanvas() {
    const auto content_size = ImGui::GetContentRegionAvail();
    if (m_canvas.begin("Canvas", content_size)) {
        drawGraph();
        drawCursor();
        m_canvas.end();
    }
}

void Controller::drawGraph() {
    if (m_drawGrid) {
        m_canvas.drawGrid();
    }
    if (m_drawScales) {
        m_canvas.drawScales();
    }
    if (m_firstDraw) {
        m_canvas.resetView();
        m_firstDraw = false;
    }
    if (m_canvas.isHovered() && ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
        m_buildLinkableNodes();
    }
    auto *draw_list_ptr = ImGui::GetWindowDrawList();
    if (draw_list_ptr != nullptr) {
        if (m_drawLinks) {
            for (const auto& link : m_fdGraph.getLinks()) {
                const auto& from_datas = link.getFromNode().lock()->getDatas();
                const auto& to_datas = link.getToNode().lock()->getDatas();
                draw_list_ptr->AddLine(from_datas.pos, to_datas.pos, m_mouseColor);
            }
        }
        if (m_drawNodes) {
            for (const auto& node_ptr : m_fdGraph.getNodes()) {
                const auto& datas = node_ptr->getDatas<VisualNodeDatas>();
                if (datas.tag.empty()) {
                    draw_list_ptr->AddCircleFilled(datas.pos, datas.radius, ImGui::GetColorU32(datas.color), 8);
                } else {
                    const auto midSize = ImGui::CalcTextSize(datas.tag.c_str()) * 0.5f;
                    const auto midSizeExtent = midSize * 1.2f;
                    draw_list_ptr->AddRectFilled(datas.pos - midSizeExtent, datas.pos + midSizeExtent, ImGui::GetColorU32(datas.color), 2);
                    draw_list_ptr->AddText(datas.pos - midSize, ImGui::GetColorU32(ImGuiCol_Text), datas.tag.c_str());
                }
            }
        }
        if (m_drawNodes || m_drawLinks) {
            if (m_canvas.isHovered() && ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
                m_drawLinkableNodes(draw_list_ptr);
            }
        }
    }
}

void Controller::drawCursor() {
    if (m_canvas.isHovered()) {
        m_moveCursor(ImGui::GetMousePos());
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
            auto* draw_list_ptr = ImGui::GetWindowDrawList();
            if (draw_list_ptr != nullptr) {
                draw_list_ptr->AddCircle(m_cursorPos, m_getMouseRadius(), m_mouseColor, 32);
            }
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                m_wasDragging = true;
            }
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                if (!m_wasDragging) {
                    m_createNode(m_cursorPos);
                }
                m_wasDragging = false;
            }
        }
    }
}

void Controller::drawDialogs(const ImVec2& vScreenSize) {
    ImVec2 max = vScreenSize;
    ImVec2 min = max * 0.5f;
    if (ImGuiFileDialog::Instance()->Display("ImportCsv", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, min, max)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            m_importCsvFile(ImGuiFileDialog::Instance()->GetFilePathName());
        }
        ImGuiFileDialog::Instance()->Close();
    }
    if (ImGuiFileDialog::Instance()->Display("ExportToCsv", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, min, max)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            m_exportCsvFile(ImGuiFileDialog::Instance()->GetFilePathName());
        }
        ImGuiFileDialog::Instance()->Close();
    }
    if (ImGuiFileDialog::Instance()->Display("ExportToSvg", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, min, max)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            m_exportSvgFile(ImGuiFileDialog::Instance()->GetFilePathName());
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

void Controller::m_startStopServer() {
    // stop if started
    if (m_namedPipeControllerThread.joinable()) {
        m_threadWorking = false;
        m_namedPipeControllerThread.join();
    } else {
        // start if not started
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
}

void Controller::m_startStopClient() {
    
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
    auto& datas = base_node_ptr->getDatasRef<VisualNodeDatas>();
    datas.pos = vNodePos;
    m_createLinks(base_node_ptr);
}

ez::FdGraph::NodeWeak Controller::m_createNode(const std::string& vTag) {
    if (m_nodesByTags.find(vTag) == m_nodesByTags.end()) {  // not found
        auto base_node_ptr = m_fdGraph.addNode(VisualNodeDatas()).lock();
        auto& datas = base_node_ptr->getDatasRef<VisualNodeDatas>();
        datas.tag = vTag;
        datas.pos = ez::FdGraph::randomPosition();
        m_nodesByTags[vTag] = base_node_ptr;
        return base_node_ptr;
    }
    return m_nodesByTags.at(vTag);
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
                if (other_len < m_getMouseRadius()) {
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

void Controller::m_drawLinkableNodes(ImDrawList* vDrawnListPtr) {
    if (!m_linkableNodes.empty()) {
        for (const auto& node : m_linkableNodes) {
            vDrawnListPtr->AddLine(m_cursorPos, node.lock()->getDatas().pos, m_poximityColor);
        }
        vDrawnListPtr->AddCircle(m_cursorPos, 3.0f, m_poximityColor, 32);
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

float Controller::m_getMouseRadius() const {
    return m_mouseRadius * m_canvas.getView().getInvScale();
}

void Controller::m_importCsvFile(const std::string& vFilePathName) {
    std::ifstream file(vFilePathName);
    if (file.is_open()) {
        m_fdGraph.clear();
        std::string row;
        while (std::getline(file, row)) {
            auto rows = ez::str::splitStringToVector(row, ';');
            if (rows.size() > 1) { // only the two's first columns
                auto node1 = m_createNode(rows.at(0));
                auto node2 = m_createNode(rows.at(1));
                m_fdGraph.addLink(node1, node2);
            }
        }
        file.close();
    }
}

void Controller::m_exportCsvFile(const std::string& vFilePathName) {
    EZ_TOOLS_DEBUG_BREAK;
}

void Controller::m_exportSvgFile(const std::string& vFilePathName) {
    EZ_TOOLS_DEBUG_BREAK;
}
