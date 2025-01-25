#include "serverController.h"
#include <cmath>
#include <ostream>
#include <ezlibs/ezLog.hpp>
#include <ezlibs/ezImGui.hpp>

bool ServerController::init() {
    m_linkNodeMode = ImWidgets::QuickStringCombo(0, {"Link to shortest node in radius", "Link to all nodes in radius"});
    m_mouseColor = ImGui::GetColorU32(ImVec4(1, 1, 0, 1));
    m_poximityColor = ImGui::GetColorU32(ImVec4(0.25, 1, 0, 1));
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
    return true;
}

void ServerController::unit() {
    m_threadWorking = false;
    m_namedPipeServerControllerThread.join();
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

void ServerController::drawGraph(ImCanvas& vCanvas) {
    m_fdGraph.updateForces(ImGui::GetIO().DeltaTime);
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
    

void ServerController::m_namedPipeServerWorker() {
    auto server_ptr = ez::NamedPipe::Server::create("FdGraph", 512);
    if (server_ptr != nullptr) {
        LogVarDebugInfo("%s", "ServerController \"FdGraph\" started");
        while (m_threadWorking) {
            if (server_ptr->isMessageReceived()) {
                m_mutex.lock();
                if (!m_cmdProcessor.decode(server_ptr->readString())) {
                    LogVarDebugInfo("%s", "fail to decode received command");
                }
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
