#include "serverController.h"
#include <cmath>
#include <ostream>
#include <ezlibs/ezLog.hpp>
#include <ezlibs/ezImGui.hpp>

bool ServerController::init() {
    m_mouseColor = ImGui::GetColorU32(ImVec4(1, 1, 0, 1));
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
    const auto w = 120.0f;
    /*
    if (m_modes.display(w, "##Modes")) {
        return m_evalExpr(m_expr.GetText());
    }
    if (m_expr.DisplayInputText(vMaxWidth - w, "", NUM_STR)) {
        return m_evalExpr(m_expr.GetText());
    }
    */
    return false;
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

void ServerController::drawGraph() {
    m_fdGraph.update(ImGui::GetIO().DeltaTime);
    auto *draw_list_ptr = ImGui::GetWindowDrawList();
    if (draw_list_ptr != nullptr) {
        for (const auto& link : m_fdGraph.getLinks()) {
            auto node_from_ptr = link.from.lock();
            auto node_to_ptr = link.to.lock();
            draw_list_ptr->AddLine(node_from_ptr->position, node_to_ptr->position, m_mouseColor);
        }
        for (const auto& node_ptr : m_fdGraph.getNodes()) {
            draw_list_ptr->AddCircleFilled(node_ptr->position, node_ptr->radius, m_mouseColor);
        }
    }
}

void ServerController::drawCursor() {
    auto* draw_list_ptr = ImGui::GetWindowDrawList();
    if (draw_list_ptr != nullptr) {
        draw_list_ptr->AddCircle(m_cursorPos, m_mouseRadius, m_mouseColor);
    }
    if (ImGui::IsWindowHovered()) {
        m_cursorPos = ImGui::GetMousePos();
    }
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        m_wasDragging = true;
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (!m_wasDragging) {
            m_createNode(m_cursorPos);
            m_wasDragging = false;
        }
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
    base_node_ptr->position = vNodePos;
    for (const auto& other_node_ptr : m_fdGraph.getNodes()) {
        if (ez::length(other_node_ptr->position - ez::fvec2(vNodePos)) < m_mouseRadius) {
            base_node_ptr->radius += 1.0f;
            m_fdGraph.addLink(other_node_ptr, base_node_ptr);
        }
    }
}

void ServerController::m_moveCursor(const ez::fvec2& vCursorPos) {
    m_cursorPos = vCursorPos;
}
