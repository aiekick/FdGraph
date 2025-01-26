#include "serverController.h"
#include <cmath>
#include <ostream>
#include <ezlibs/ezLog.hpp>
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

bool ServerController::init() {
    m_linkNodeMode = ImWidgets::QuickStringCombo(0, {"Link to shortest", "Link to all"});
    m_mouseColor = ImGui::GetColorU32(ImVec4(1, 1, 0, 1));
    m_poximityColor = ImGui::GetColorU32(ImVec4(0.25, 1, 0, 1));
    return true;
}

void ServerController::unit() {
    m_threadWorking = false;
    if (m_namedPipeServerControllerThread.joinable()) {
        m_namedPipeServerControllerThread.join();
    }
}

bool ServerController::drawInput(float vMaxWidth) {
    bool ret = false;
    ret |= m_linkNodeMode.display(250.0f, "##nodeLinking");
    return ret;
}

bool ServerController::drawControl(float vMaxWidth) {
    bool change = false;
    /*
    float w = (vMaxWidth - ImGui::GetStyle().ItemSpacing.x * 8.0f - ImGui::GetStyle().FramePadding.x * 2.0f) * 0.25f;
    change |= ImGui::SliderFloatDefaultCompact(w, "step size", &m_stepSize, 0.0f, 100.0f, 10.0f, 0.0f, "%.6f");
    change |= ImGui::SliderFloatDefaultCompact(w, "angle lim inf", &m_angleLimInf, 0.0f, 1.0f, 0.998f, 0.0f, "%.6f");
    change |= ImGui::SliderFloatDefaultCompact(w, "angle factor", &m_angleFactor, m_angleLimInf, 1.0f, 1.0f, 0.0f, "%.6f");
    change |= ImGui::SliderSizeTDefaultCompact(w, "count", &m_countPoints, 0U, m_points.size(), m_points.size());
    if (change) {
        m_evalExpr(m_expr.GetText());
    }
    */
    return change;
}

void ServerController::update() {
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
    m_fdGraph.updateForces(ImGui::GetIO().DeltaTime);
}

void ServerController::drawGraph(ImCanvas& vCanvas) {
    auto *draw_list_ptr = ImGui::GetWindowDrawList();
    if (draw_list_ptr != nullptr) {
        for (const auto& link : m_fdGraph.getLinks()) {
            auto node_from_ptr = link.from.lock();
            auto node_to_ptr = link.to.lock();
            draw_list_ptr->AddLine(node_from_ptr->pos, node_to_ptr->pos, m_mouseColor);
        }
        for (const auto& node_ptr : m_fdGraph.getNodes()) {
            draw_list_ptr->AddCircleFilled(node_ptr->pos, node_ptr->radius, m_mouseColor);
        }
        if (m_linkNodeMode.getIndex() == 0U) { // link to shortest node
            if (vCanvas.isHovered() && ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
                if (!m_shortestNode.expired()) {
                    auto node_ptr = m_shortestNode.lock();
                    draw_list_ptr->AddLine(m_cursorPos, node_ptr->pos, m_poximityColor);
                    draw_list_ptr->AddCircle(m_cursorPos, 3.0f, m_poximityColor);
                }
            }
        }
    }
}

void ServerController::drawCursor(ImCanvas& vCanvas) {
    m_shortestNode.reset();
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
        ImGui::SetTooltip("%.2f,%.2f", mp.x, mp.y);
        vCanvas.resume();
    }
}

void ServerController::m_startNamedPipeServer() {
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
    m_namedPipeServerControllerThread = std::thread(&ServerController::m_namedPipeServerWorker, this);
}

void ServerController::m_namedPipeServerWorker() {
    auto server_ptr = ez::NamedPipe::Server::create("FdGraph", 64);
    if (server_ptr != nullptr) {
        LogVarDebugInfo("%s", "ServerController \"FdGraph\" started");
        while (m_threadWorking) {
            if (server_ptr->isMessageReceived()) {
                const auto msg = server_ptr->readString();
                m_mutex.lock();
                m_cmdStack.push(msg);
                m_mutex.unlock();
            }
        }
        LogVarDebugInfo("%s", "ServerController \"FdGraph\" endded");
        server_ptr.reset();
    }
}

void ServerController::m_createNode(const ez::fvec2& vNodePos) {
    auto base_node_ptr = m_fdGraph.addNode().lock();
    base_node_ptr->pos = vNodePos;
    if (m_linkNodeMode.getIndex() == 1U) {  // link to all nodes in radius
        for (const auto& other_node_ptr : m_fdGraph.getNodes()) {
            if (other_node_ptr != base_node_ptr) {
                if (ez::length(other_node_ptr->pos - ez::fvec2(vNodePos)) < m_mouseRadius) {
                    m_fdGraph.addLink(other_node_ptr, base_node_ptr);
                }
            }
        }
    } else if (m_linkNodeMode.getIndex() == 0U) {  // link to shortest node
        if (!m_shortestNode.expired()) {
            m_fdGraph.addLink(m_shortestNode, base_node_ptr);
        }
    }
}

void ServerController::m_moveCursor(const ez::fvec2& vCursorPos) {
    m_cursorPos = vCursorPos;
    m_shortestNode.reset();
    if (m_linkNodeMode.getIndex() == 0U) {  // link to shortest node
        if (!m_fdGraph.getNodes().empty()) {
            float shortest_len = 1e9f;
            for (const auto& other_node_ptr : m_fdGraph.getNodes()) {
                float new_len = ez::length(other_node_ptr->pos - ez::fvec2(m_cursorPos));
                if (new_len < m_mouseRadius && new_len < shortest_len) {
                    shortest_len = new_len;
                    m_shortestNode = other_node_ptr;
                }
            }
        }
    }
}
