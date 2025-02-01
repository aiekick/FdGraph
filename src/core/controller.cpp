#include "controller.h"

#include <cmath>
#include <random>
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

Controller::Datas Controller::DefaultDatas;

bool ContrastedFileDialog::m_Selectable(int vIdx, const char* vLabel, bool vSelected, ImGuiSelectableFlags vFlags, const ImVec2& vSizeArg) {
    bool ret = false;
    auto* storage_ptr = ImGui::GetStateStorage();
    const auto imgui_id = ImGui::GetID(vLabel);
    ImGuiCol col = storage_ptr->GetInt(imgui_id);
    const bool pushed = ImGui::PushStyleColorWithContrast1(col, ImGuiCol_Text, ImGui::CustomStyle::puContrastedTextColor, ImGui::CustomStyle::puContrastRatio);
    ret = ImGuiFileDialog::m_Selectable(vIdx, vLabel, vSelected, vFlags, vSizeArg);
    if (ImGui::IsItemHovered()) {
        storage_ptr->SetInt(imgui_id, ImGuiCol_HeaderHovered);
    } else if (vSelected) {
        storage_ptr->SetInt(imgui_id, ImGuiCol_Header);
    } else {
        storage_ptr->SetInt(imgui_id, ImGuiCol_PopupBg);
    }
    if (pushed) {
        ImGui::PopStyleColor();
    }
    return ret;
}

void ContrastedFileDialog::m_drawColumnText(int vColIdx, const char* vLabel, bool vSelected, bool vHovered) {
    auto* storage_ptr = ImGui::GetStateStorage();
    const auto imgui_id = ImGui::GetID(vLabel);
    ImGuiCol col = storage_ptr->GetInt(imgui_id);
    const bool pushed = ImGui::PushStyleColorWithContrast1(col, ImGuiCol_Text, ImGui::CustomStyle::puContrastedTextColor, ImGui::CustomStyle::puContrastRatio);
    ImGuiFileDialog::m_drawColumnText(vColIdx, vLabel, vSelected, vHovered);
    if (vHovered) {
        storage_ptr->SetInt(imgui_id, ImGuiCol_HeaderHovered);
    } else if (vSelected) {
        storage_ptr->SetInt(imgui_id, ImGuiCol_Header);
    } else {
        storage_ptr->SetInt(imgui_id, ImGuiCol_PopupBg);
    }
    if (pushed) {
        ImGui::PopStyleColor();
    }
}

bool Controller::init() {
    m_datas.m_mouseColorV4 = ImVec4(1, 1, 0, 1);
    m_datas.m_poximityColorV4 = ImVec4(0.25, 1, 0, 1);
    m_mouseColor = ImGui::GetColorU32(m_datas.m_mouseColorV4);
    m_poximityColor = ImGui::GetColorU32(m_datas.m_poximityColorV4);
    m_fdGraph.getConfigRef().forceFactor = 1000.0f;
    m_fdGraph.getConfigRef().centralGravityFactor = 5.0f;
    m_fdGraph.getConfigRef().deltaTimeFactor = 10.0f;
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
        if (m_datas.m_drawGrid || m_datas.m_drawScales) {
            ret |= ImGui::SliderFloatDefaultCompact(500.0f, "Major step X", &m_canvas.getConfigRef().gridSize.x, 1.0f, 200.0f, 50.0f, 1.0f);
            ret |= ImGui::SliderFloatDefaultCompact(500.0f, "Major step Y", &m_canvas.getConfigRef().gridSize.y, 1.0f, 200.0f, 50.0f, 1.0f);
            ret |= ImGui::SliderFloatDefaultCompact(500.0f, "Subdivs X", &m_canvas.getConfigRef().gridSubdivs.x, 0.0f, 50.0f, 5.0f, 1.0f);
            ret |= ImGui::SliderFloatDefaultCompact(500.0f, "Subdivs Y", &m_canvas.getConfigRef().gridSubdivs.y, 0.0f, 50.0f, 5.0f, 1.0f);

            ImGui::Separator();
        }

        ret |= ImGui::SliderFloatDefaultCompact(500.0f, "Mouse radius", &m_datas.m_mouseRadius, 0.0f, 500.0f, DefaultDatas.m_mouseRadius);
        ret |= ImGui::SliderFloatDefaultCompact(500.0f, "Nodes scale factor", &m_datas.m_nodeScaleFactor, 0.0f, 20.0f, DefaultDatas.m_nodeScaleFactor);
        ret |= ImGui::SliderFloatDefaultCompact(500.0f, "Links scale factor", &m_datas.m_linkScaleFactor, 0.0f, 20.0f, DefaultDatas.m_linkScaleFactor);
        ret |= ImGui::SliderFloatDefaultCompact(500.0f, "Default lerp value", &m_datas.m_defaultLerpValue, 0.0f, 2.0f, DefaultDatas.m_defaultLerpValue);
        ret |= ImGui::SliderFloatDefaultCompact(500.0f, "Increment lerp value", &m_datas.m_incrementLerpValue, 0.0f, 2.0f, DefaultDatas.m_incrementLerpValue);

        ImGui::Separator();

        static ImVec4 MouseColor{ImVec4(1, 1, 0, 1)};
        static ImVec4 ProximityColor{ImVec4(0.25, 1, 0, 1)};
        if (ImGui::ColorEdit4Default(400.0f, "Mouse color", &m_datas.m_mouseColorV4.x, &DefaultDatas.m_mouseColorV4.x)) {
            m_mouseColor = ImGui::GetColorU32(m_datas.m_mouseColorV4);
        }
        if (ImGui::ColorEdit4Default(400.0f, "Proximity color", &m_datas.m_poximityColorV4.x, &DefaultDatas.m_poximityColorV4.x)) {
            m_poximityColor = ImGui::GetColorU32(m_datas.m_poximityColorV4);
        }

        ImGui::Separator();

        ret |= ImGui::SliderFloatDefaultCompact(500.0f, "Gravity", &m_fdGraph.getConfigRef().centralGravityFactor, 0.0f, 200.0f, 5.0f);
        ret |= ImGui::SliderFloatDefaultCompact(500.0f, "Force", &m_fdGraph.getConfigRef().forceFactor, 0.0f, 100000.0f, 1000.0f);
        ret |= ImGui::SliderFloatDefaultCompact(500.0f, "Convergence speed", &m_fdGraph.getConfigRef().deltaTimeFactor, 0.0f, 50.0f, 10.0f);

        ImU32 m_mouseColor = 0;
        ImU32 m_poximityColor = 0;
        ImGui::EndMenu();
    }

    ImGui::Separator();

    if (ImGui::ContrastedBeginMenu(ICON_FONT_FOLDER " ##Import", "Import")) {
        if (ImGui::ContrastedMenuItem("Import from Csv")) {
            IGFD::FileDialogConfig config;
            config.flags = ImGuiFileDialogFlags_Modal;
            ContrastedFileDialog::Instance()->OpenDialog("ImportCsv", "Import from Csv", ".csv", config);
            ret = true;
        }
        ImGui::EndMenu();
    }
    if (ImGui::ContrastedBeginMenu(ICON_FONT_CONTENT_SAVE " ##Export", "Export")) {
        if (ImGui::ContrastedMenuItem("Export to Csv")) {
            IGFD::FileDialogConfig config;
            config.flags = ImGuiFileDialogFlags_Modal;
            ContrastedFileDialog::Instance()->OpenDialog("ExportToCsv", "Export to csv", ".csv", config);
            ret = true;
        }
        if (ImGui::ContrastedMenuItem("Export to Svg")) {
            IGFD::FileDialogConfig config;
            config.flags = ImGuiFileDialogFlags_Modal;
            ContrastedFileDialog::Instance()->OpenDialog("ExportToSvg", "Export to svg", ".svg", config);
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
        ret |= ImGui::ContrastedMenuItem("Grid", nullptr, &m_datas.m_drawGrid);
        ret |= ImGui::ContrastedMenuItem("Rulers", nullptr, &m_datas.m_drawScales);

        ImGui::Separator();

        ret |= ImGui::ContrastedMenuItem("Nodes", nullptr, &m_drawNodes);
        ret |= ImGui::ContrastedMenuItem("Links", nullptr, &m_drawLinks);

        ImGui::EndMenu();
    }

    ImGui::Separator();

    if (ImGui::ContrastedMenuItem(ICON_FONT_VECTOR_POINT "##NoLink", "No link", m_datas.m_linkingMode == LinkingMode::NONE)) {
        m_datas.m_linkingMode = LinkingMode::NONE;
        ret = true;
    }
    if (ImGui::ContrastedMenuItem(ICON_FONT_VECTOR_LINE "##OneLink", "One link", m_datas.m_linkingMode == LinkingMode::ONE)) {
        m_datas.m_linkingMode = LinkingMode::ONE;
        ret = true;
    }
    if (ImGui::ContrastedMenuItem(ICON_FONT_VECTOR_POLYLINE "##TwoLink", "Two links", m_datas.m_linkingMode == LinkingMode::TWO)) {
        m_datas.m_linkingMode = LinkingMode::TWO;
        ret = true;
    }
    if (ImGui::ContrastedMenuItem(ICON_FONT_VECTOR_POLYGON "##ManyLink", "Many links", m_datas.m_linkingMode == LinkingMode::MANY)) {
        m_datas.m_linkingMode = LinkingMode::MANY;
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

void Controller::clear() {
    m_nodesByTags.clear();
    m_fdGraph.clear();
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
        datas.size = 1.5f + 0.2f * static_cast<float>(datas.connCount);
        datas.color = ez::getRainBowColor(datas.connCount, 50);
        m_aabb.Combine(datas.pos);
    }
}

void Controller::drawCanvas() {
    const auto content_size = ImGui::GetContentRegionAvail();
    if (m_canvas.begin("Canvas", content_size)) {
        drawGraph();
        m_canvas.end();
    }
}

void Controller::drawGraph() {
    if (m_datas.m_drawGrid) {
        m_canvas.drawGrid();
    }
    if (m_datas.m_drawScales) {
        m_canvas.drawScales();
    }
    if (m_firstDraw) {
        m_canvas.resetView();
        m_firstDraw = false;
    }
    if (m_canvas.isHovered()) {
        drawCursor();
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
            m_buildLinkableNodes();
        }
    }
    auto* draw_list_ptr = ImGui::GetWindowDrawList();
    if (draw_list_ptr != nullptr) {
        if (m_drawLinks) {
            for (const auto& link : m_fdGraph.getLinks()) {
                const auto& from_datas = link.getFromNode().lock()->getDatas();
                const auto& to_datas = link.getToNode().lock()->getDatas();
                draw_list_ptr->AddLine(from_datas.pos, to_datas.pos, m_mouseColor, m_datas.m_linkScaleFactor);
            }
        }
        if (m_drawNodes) {
            for (const auto& node_ptr : m_fdGraph.getNodes()) {
                const auto& datas = node_ptr->getDatas<VisualNodeDatas>();
                if (datas.tag.empty()) {
                    draw_list_ptr->AddCircleFilled(datas.pos, datas.size * m_datas.m_nodeScaleFactor, ImGui::GetColorU32(datas.color), 8);
                } else {
                    const auto midSize = ImGui::CalcTextSize(datas.tag.c_str()) * 0.5f;
                    const auto midSizeExtent = midSize * 1.2f * m_datas.m_nodeScaleFactor;
                    draw_list_ptr->AddRectFilled(datas.pos - midSizeExtent, datas.pos + midSizeExtent, ImGui::GetColorU32(datas.color), 5.0f);
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
    } else {
        m_moveClosestNode();
    }
}

void Controller::drawDialogs(const ImVec2& vScreenSize) {
    ImVec2 max = vScreenSize;
    ImVec2 min = max * 0.5f;
    if (ContrastedFileDialog::Instance()->Display("ImportCsv", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, min, max)) {
        if (ContrastedFileDialog::Instance()->IsOk()) {
            m_importCsvFile(ContrastedFileDialog::Instance()->GetFilePathName());
        }
        ContrastedFileDialog::Instance()->Close();
    }
    if (ContrastedFileDialog::Instance()->Display("ExportToCsv", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, min, max)) {
        if (ContrastedFileDialog::Instance()->IsOk()) {
            m_exportCsvFile(ContrastedFileDialog::Instance()->GetFilePathName());
        }
        ContrastedFileDialog::Instance()->Close();
    }
    if (ContrastedFileDialog::Instance()->Display("ExportToSvg", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, min, max)) {
        if (ContrastedFileDialog::Instance()->IsOk()) {
            m_exportSvgFile(ContrastedFileDialog::Instance()->GetFilePathName());
        }
        ContrastedFileDialog::Instance()->Close();
    }
}

ez::xml::Nodes Controller::getXmlNodes(const std::string& vUserDatas) {
    ez::xml::Node node;
    auto& controllerNode = node.addChild("controller");
    controllerNode
        .addChild("grid")  //
        .addAttribute("major_step_x", m_canvas.getConfig().gridSize.x)
        .addAttribute("major_step_y", m_canvas.getConfig().gridSize.y)
        .addAttribute("subdivs_x", m_canvas.getConfig().gridSubdivs.x)
        .addAttribute("subdivs_y", m_canvas.getConfig().gridSubdivs.y);
    controllerNode
        .addChild("mouse")  //
        .addAttribute("radius", m_datas.m_mouseRadius);
    controllerNode
        .addChild("lerp")  //
        .addAttribute("default", m_datas.m_defaultLerpValue)
        .addAttribute("increment", m_datas.m_incrementLerpValue);
    controllerNode
        .addChild("scale")  //
        .addAttribute("nodes", m_datas.m_nodeScaleFactor)
        .addAttribute("links", m_datas.m_linkScaleFactor);
    controllerNode
        .addChild("graph")  //
        .addAttribute("gravity", m_fdGraph.getConfig().centralGravityFactor)
        .addAttribute("force", m_fdGraph.getConfig().forceFactor)
        .addAttribute("convergence", m_fdGraph.getConfig().deltaTimeFactor)
        .addAttribute("linking", static_cast<int32_t>(m_datas.m_linkingMode));
    controllerNode
        .addChild("visibility")  //
        .addAttribute("grid", m_datas.m_drawGrid)
        .addAttribute("scales", m_datas.m_drawScales);
    return node.getChildren();
}

bool Controller::setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) {
    const auto& strName = vNode.getName();
    const auto& strValue = vNode.getContent();
    const auto& strParentName = vParent.getName();
    if (strParentName == "controller") {
        if (strName == "grid") {
            if (vNode.isAttributeExist("major_step_x")) {
                m_canvas.getConfigRef().gridSize.x = vNode.getAttribute<float>("major_step_x");
            }
            if (vNode.isAttributeExist("major_step_y")) {
                m_canvas.getConfigRef().gridSize.y = vNode.getAttribute<float>("major_step_y");
            }
            if (vNode.isAttributeExist("subdivs_x")) {
                m_canvas.getConfigRef().gridSubdivs.x = vNode.getAttribute<float>("subdivs_x");
            }
            if (vNode.isAttributeExist("subdivs_y")) {
                m_canvas.getConfigRef().gridSubdivs.y = vNode.getAttribute<float>("subdivs_y");
            }
        } else if (strName == "mouse") {
            if (vNode.isAttributeExist("radius")) {
                m_datas.m_mouseRadius = vNode.getAttribute<float>("radius");
            }
        } else if (strName == "lerp") {
            if (vNode.isAttributeExist("default")) {
                m_datas.m_defaultLerpValue = vNode.getAttribute<float>("default");
            }
            if (vNode.isAttributeExist("increment")) {
                m_datas.m_incrementLerpValue = vNode.getAttribute<float>("increment");
            }
        } else if (strName == "scale") {
            if (vNode.isAttributeExist("nodes")) {
                m_datas.m_nodeScaleFactor = vNode.getAttribute<float>("nodes");
            }
            if (vNode.isAttributeExist("links")) {
                m_datas.m_linkScaleFactor = vNode.getAttribute<float>("links");
            }
        } else if (strName == "graph") {
            if (vNode.isAttributeExist("gravity")) {
                m_fdGraph.getConfigRef().centralGravityFactor = vNode.getAttribute<float>("gravity");
            }
            if (vNode.isAttributeExist("force")) {
                m_fdGraph.getConfigRef().forceFactor = vNode.getAttribute<float>("force");
            }
            if (vNode.isAttributeExist("convergence")) {
                m_fdGraph.getConfigRef().deltaTimeFactor = vNode.getAttribute<float>("convergence");
            }
            if (vNode.isAttributeExist("linking")) {
                m_datas.m_linkingMode = static_cast<LinkingMode>(vNode.getAttribute<int32_t>("linking"));
            }
        } else if (strName == "visibility") {
            if (vNode.isAttributeExist("grid")) {
                m_datas.m_drawGrid = vNode.getAttribute<bool>("grid");
            }
            if (vNode.isAttributeExist("scales")) {
                m_datas.m_drawScales = vNode.getAttribute<bool>("scales");
            }
        }
    }
    return true;
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

void Controller::m_startStopClient() {}

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
        size_t tag_pos = vTag.find("[TAG]");
        size_t num_pos = vTag.find("[NUM]");
        if (tag_pos != std::string::npos) {
            datas.tag = vTag.substr(tag_pos + 5);
        } else if (num_pos != std::string::npos) {
            // Num
        } else {
            datas.tag = vTag;
        }
        // random pos in [-SIZE:SIZE]
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> distrib(-1.0, 1.0);
        datas.pos.x = distrib(gen) * m_canvas.getRect().GetSize().x * 0.5f;
        datas.pos.y = distrib(gen) * m_canvas.getRect().GetSize().y * 0.5f;
        m_nodesByTags[vTag] = base_node_ptr;
        return base_node_ptr;
    }
    return m_nodesByTags.at(vTag);
}

void Controller::m_moveCursor(const ez::fvec2& vCursorPos) {
    m_cursorPos = vCursorPos;
}

void Controller::m_moveClosestNode() {
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        m_closestNode = m_findClosestNode(m_cursorPos);
    }
    if (!m_closestNode.expired()) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            auto mp = ImGui::GetMousePos();
            auto& datas = m_closestNode.lock()->getDatasRef<VisualNodeDatas>();
            datas.pos = datas.pos.lerp(mp, m_lerpValue);
            if (m_lerpValue < 1.0f) {
                m_lerpValue += m_datas.m_incrementLerpValue;
            }
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            m_closestNode.reset();
            m_lerpValue = m_datas.m_defaultLerpValue;
        }
    }
}

void Controller::m_buildLinkableNodes() {
    m_linkableNodes.clear();
    if (m_datas.m_linkingMode != LinkingMode::NONE) {
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
                if ((m_datas.m_linkingMode == LinkingMode::TWO && idx >= 2) ||  //
                    (m_datas.m_linkingMode == LinkingMode::ONE && idx >= 1)) {
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
            vDrawnListPtr->AddLine(m_cursorPos, node.lock()->getDatas().pos, m_poximityColor, m_datas.m_linkScaleFactor);
        }
        vDrawnListPtr->AddCircleFilled(m_cursorPos, 3.0f * m_datas.m_nodeScaleFactor, m_poximityColor, 32);
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
    return m_datas.m_mouseRadius * m_canvas.getView().getInvScale();
}

ez::FdGraph::NodeWeak Controller::m_findClosestNode(const ez::fvec2& vCursorPos) {
    ez::FdGraph::NodeWeak ret;
    for (const auto& node_ptr : m_fdGraph.getNodes()) {
        const auto& datas = node_ptr->getDatas<VisualNodeDatas>();
        if (ret.expired()) {
            ret = node_ptr;
        } else {
            const auto& close_datas = ret.lock()->getDatas<VisualNodeDatas>();
            const auto c0 = (vCursorPos - datas.pos).lengthSquared();
            const auto c1 = (vCursorPos - close_datas.pos).lengthSquared();
            if (c0 < c1) {
                ret = node_ptr;
            }
        }
    }
    return ret;
}

#define CSV_HEADER "node1;node2;x1;y1;color1;x2;y2;color2"

void Controller::m_importCsvFile(const std::string& vFilePathName) {
    std::ifstream file(vFilePathName);
    if (file.is_open()) {
        clear();
        std::string row;
        while (std::getline(file, row)) {
            if (row == CSV_HEADER) {
                continue;
            }
            auto rows = ez::str::splitStringToVector(row, ';');
            ez::FdGraph::NodePtr node1_ptr;
            ez::FdGraph::NodePtr node2_ptr;
            if (rows.size() > 1) {  // only the two's first columns
                node1_ptr = m_createNode(rows.at(0)).lock();
                node2_ptr = m_createNode(rows.at(1)).lock();
                m_fdGraph.addLink(node1_ptr, node2_ptr);
            }
            if (rows.size() > 4) {
                auto& datas1 = node1_ptr->getDatasRef<VisualNodeDatas>();
                datas1.pos.x = ez::fvariant(rows.at(2)).GetF();
                datas1.pos.y = ez::fvariant(rows.at(3)).GetF();
                datas1.color = ImGui::ColorConvertU32ToFloat4(  //
                    ez::uvariant(rows.at(4)).GetU());
            }
            if (rows.size() > 7) {
                auto& datas2 = node2_ptr->getDatasRef<VisualNodeDatas>();
                datas2.pos.x = ez::fvariant(rows.at(5)).GetF();
                datas2.pos.y = ez::fvariant(rows.at(6)).GetF();
                datas2.color = ImGui::ColorConvertU32ToFloat4(  //
                    ez::uvariant(rows.at(7)).GetU());
            }
        }
        file.close();
    }
}

void Controller::m_exportCsvFile(const std::string& vFilePathName) {
    std::ofstream file(vFilePathName);
    if (file.is_open()) {
        file << CSV_HEADER << std::endl;
        for (const auto& link : m_fdGraph.getLinks()) {
            const auto& node1_datas = link.getFromNode().lock()->getDatas<VisualNodeDatas>();
            if (node1_datas.tag.empty()) {
                file << "[NUM]" << link.getFromNode().lock().get();
            } else {
                file << "[TAG]" << node1_datas.tag;
            }
            file << ";";
            const auto& node2_datas = link.getToNode().lock()->getDatas<VisualNodeDatas>();
            if (node2_datas.tag.empty()) {
                file << "[NUM]" << link.getToNode().lock().get();
            } else {
                file << "[TAG]" << node2_datas.tag;
            }
            file << ";" << node1_datas.pos.x << ";" << node1_datas.pos.y << ";" << ImGui::GetColorU32(node1_datas.color);
            file << ";" << node2_datas.pos.x << ";" << node2_datas.pos.y << ";" << ImGui::GetColorU32(node2_datas.color);
            file << std::endl;
        }
        file.close();
    }
}

void Controller::m_exportSvgFile(const std::string& vFilePathName) {
    auto canvasSize = m_aabb.GetExtents() * 2.25f;
    auto midCanvasSize = canvasSize * 0.5f;
    ez::img::Svg svg(canvasSize);
    for (const auto& link : m_fdGraph.getLinks()) {
        const auto& from_datas = link.getFromNode().lock()->getDatas();
        const auto& to_datas = link.getToNode().lock()->getDatas();
        svg.addLine(from_datas.pos + midCanvasSize, to_datas.pos + midCanvasSize, "yellow");
    }
    for (const auto& node_ptr : m_fdGraph.getNodes()) {
        const auto& datas = node_ptr->getDatas<VisualNodeDatas>();
        if (datas.tag.empty()) {
            svg.addCircle(datas.pos + midCanvasSize, datas.size, "black");
        } else {
            const auto midSize = ImGui::CalcTextSize(datas.tag.c_str()) * 0.5f;
            const auto midSizeExtent = midSize * 1.2f;
            svg.addRectangle(datas.pos - midSizeExtent + midCanvasSize, datas.pos + midSizeExtent + midCanvasSize, "black");
            svg.addText(datas.pos - midSize + midCanvasSize, datas.tag, "white");
        }
    }
    svg.exportToFile(vFilePathName);
}
