#include "controller.h"
#include <cmath>
#include <ostream>
#include <ezlibs/ezTools.hpp>
#include <ezlibs/ezExpr.hpp>
#include <ezlibs/ezImGui.hpp>
#include <ezlibs/ezNamedPipe.hpp>

bool Controller::init() {
    m_mouseColor = ImGui::GetColorU32(ImVec4(1, 1, 0, 1));
    m_namedPipeServerThread = std::thread(&m_namedPipeServerWorker, this, std::ref(m_threadWorking));
    return true;
}

void Controller::unit() {
    m_threadWorking = false;
    m_namedPipeServerThread.join();
}

bool Controller::drawInput(float vMaxWidth) {
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

bool Controller::drawControl(float vMaxWidth) {
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

void Controller::drawGraph() {
    m_fdGraph.update(ImGui::GetIO().DeltaTime);
    auto *draw_list_ptr = ImGui::GetWindowDrawList();
    if (draw_list_ptr != nullptr) {
        for (const auto& link : m_fdGraph.getLinks()) {
            auto node_from_ptr = link.from.lock();
            auto node_to_ptr = link.to.lock();
            draw_list_ptr->AddLine(node_from_ptr->position, node_to_ptr->position, m_mouseColor);
        }
        for (const auto& node_ptr : m_fdGraph.getNodes()) {
            draw_list_ptr->AddCircleFilled(node_ptr->position, 20.0f, m_mouseColor);
        }
    }
}

void Controller::drawCursor() {
    const auto& mouse_pos = ImGui::GetMousePos();
    auto* draw_list_ptr = ImGui::GetWindowDrawList();
    if (draw_list_ptr != nullptr) {
        draw_list_ptr->AddCircle(mouse_pos, m_mouseRadius, m_mouseColor);
    }
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        auto node = m_fdGraph.addNode();
        node.lock()->position = mouse_pos;
        for (const auto& node_ptr : m_fdGraph.getNodes()) {
            if (ez::length(node_ptr->position - ez::fvec2(mouse_pos)) < m_mouseRadius) {
                m_fdGraph.addLink(node_ptr, node);
            }
        }
    }
}

void Controller::m_namedPipeServerWorker() {
    ez::NamedPipe pipeServer("FdGraph", false);
    std::cout << "Server \"FdGraph\" started" << std::endl;
    while (m_threadWorking) {
        std::string message = pipeServer.read();
        std::cout << "Server received: " << message << std::endl;
    }
    std::cout << "Server \"FdGraph\" endded" << std::endl;
}
