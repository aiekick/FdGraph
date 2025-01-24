#include "controller.h"
#include <cmath>
#include <ostream>
#include <ezlibs/ezTools.hpp>
#include <ezlibs/ezExpr.hpp>
#include <ezlibs/ezImGui.hpp>


bool Controller::init() {
    m_mouseColor = ImGui::GetColorU32(ImVec4(1, 1, 0, 1));
    return m_pipe.initClient("FdGraph");
}

void Controller::unit() {
    m_pipe.unit();
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
}

void Controller::drawCursor() {
    const auto& mouse_pos = ImGui::GetMousePos();
    auto *draw_list_ptr = ImGui::GetWindowDrawList();
    if (draw_list_ptr != nullptr) {
        draw_list_ptr->AddCircle(mouse_pos, m_mouseRadius, m_mouseColor);
    }
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        m_createNode(mouse_pos);
    }
}

void Controller::m_createNode(const ez::fvec2& vNodePos) {
    std::string msg = "CreateNode(" + vNodePos.string() + ")";
    ez::NamedPipe::DatasBuffer buffer(msg.begin(), msg.end());
    m_pipe.write(buffer);
}
