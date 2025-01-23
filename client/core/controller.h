#pragma once

#include <vector>
#include <cstdint>
#include <imguipack.h>
#include <ezlibs/ezFdGraph.hpp>

typedef std::vector<ImVec2> P2dArray;
typedef std::vector<int32_t> IntArray;

class Controller {
private:
    float m_mouseRadius = 100.0f;
    ImU32 m_mouseColor = 0;

public:
    bool init();
    void unit();
    bool drawInput(float vMaxWidth);
    bool drawControl(float vMaxWidth);
    void drawGraph();
    void drawCursor();

private:
    ez::FdGraph m_graph;

public:  // singleton
    static Controller* Instance() {
        static Controller _instance;
        return &_instance;
    }
};
