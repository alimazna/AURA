#ifndef AURA_DESKTOP_AURAICONS_H
#define AURA_DESKTOP_AURAICONS_H

// A lightweight, dependency-free icon set drawn entirely with Dear ImGui's
// fixed-function draw list (lines, rectangles, circles, triangles). No texture
// atlas, no GPU effects, no external icon font: the same icons render on the
// OpenGL 3.3 backend and the legacy OpenGL 2.1 backend, which keeps the terminal
// usable on Intel HD Graphics 3000-class hardware.
//
// Icons are intentionally small and restrained; they support the navigation and
// section titles, they never dominate the interface.

#include <imgui.h>

namespace aura {
namespace desktop {
namespace icons {

enum class Icon {
    None,
    Dashboard,    // grid
    Market,       // activity line
    Timeframes,   // clock
    Signals,      // bolt
    Risk,         // shield
    Positions,    // stacked layers
    Observation,  // eye
    Research,     // magnifier
    Knowledge,    // book
    Candidates,   // list
    Validation,   // check badge
    Approval,     // stamp / check
    Governance,   // scales
    Evolution,    // branch
    Incidents,    // warning triangle
    Schedule,     // calendar
    Recovery,     // rotate
    Health,       // pulse
    Audit,        // clipboard
    Configuration // sliders
};

// Maps a canonical section title to an icon. Presentation only.
inline Icon icon_for_section(const char* section) {
    if (section == nullptr) return Icon::None;
    const char* s = section;
    auto eq = [](const char* a, const char* b) {
        while (*a && *b) { if (*a != *b) return false; ++a; ++b; }
        return *a == *b;
    };
    if (eq(s, "System Overview")) return Icon::Dashboard;
    if (eq(s, "Market / Data Health")) return Icon::Market;
    if (eq(s, "Timeframes")) return Icon::Timeframes;
    if (eq(s, "Signals")) return Icon::Signals;
    if (eq(s, "Risk")) return Icon::Risk;
    if (eq(s, "Shadow Positions")) return Icon::Positions;
    if (eq(s, "Prediction / Observation")) return Icon::Observation;
    if (eq(s, "Research")) return Icon::Research;
    if (eq(s, "Knowledge")) return Icon::Knowledge;
    if (eq(s, "Candidates")) return Icon::Candidates;
    if (eq(s, "Validation")) return Icon::Validation;
    if (eq(s, "Approval Center")) return Icon::Approval;
    if (eq(s, "Evolution Graph")) return Icon::Evolution;
    if (eq(s, "Incidents")) return Icon::Incidents;
    if (eq(s, "Schedule / Operating Window")) return Icon::Schedule;
    if (eq(s, "Checkpoints / Recovery")) return Icon::Recovery;
    if (eq(s, "Health / Watchdog")) return Icon::Health;
    if (eq(s, "Audit")) return Icon::Audit;
    if (eq(s, "Configuration / Version")) return Icon::Configuration;
    return Icon::None;
}

// Draws `ic` centred at `c` within a `size` box, in colour `col`. `size` is the
// nominal glyph box (recommended 14..16 px). `thickness` defaults to 1.4 px.
inline void draw_icon(ImDrawList* dl, Icon ic, ImVec2 c, float size, ImU32 col,
                      float thickness = 1.4f) {
    if (dl == nullptr || ic == Icon::None) return;
    const float h = size * 0.5f;
    const float x0 = c.x - h, x1 = c.x + h;
    const float y0 = c.y - h, y1 = c.y + h;
    const float mid = (x0 + x1) * 0.5f;
    const float my = (y0 + y1) * 0.5f;
    const float q = size * 0.28f;  // quarter

    switch (ic) {
        case Icon::Dashboard: {
            const float g = 1.5f;
            dl->AddRect(ImVec2(x0, y0), ImVec2(mid - g, my - g), col, 1.0f, 0, thickness);
            dl->AddRect(ImVec2(mid + g, y0), ImVec2(x1, my - g), col, 1.0f, 0, thickness);
            dl->AddRect(ImVec2(x0, my + g), ImVec2(mid - g, y1), col, 1.0f, 0, thickness);
            dl->AddRect(ImVec2(mid + g, my + g), ImVec2(x1, y1), col, 1.0f, 0, thickness);
            break;
        }
        case Icon::Market: {
            const ImVec2 p[5] = {{x0, y1 - q}, {x0 + size * 0.25f, my}, {mid, y0 + q},
                                 {x1 - size * 0.22f, my - q * 0.2f}, {x1, y0}};
            dl->AddPolyline(p, 5, col, 0, thickness);
            break;
        }
        case Icon::Timeframes: {
            dl->AddCircle(c, h, col, 0, thickness);
            dl->AddLine(ImVec2(c.x, c.y), ImVec2(c.x, c.y - h * 0.55f), col, thickness);
            dl->AddLine(ImVec2(c.x, c.y), ImVec2(c.x + h * 0.45f, c.y), col, thickness);
            break;
        }
        case Icon::Signals: {
            const ImVec2 p[4] = {{c.x - q * 0.4f, y0}, {c.x - q * 0.7f, my},
                                 {c.x + q * 0.1f, my}, {c.x + q * 0.5f, y1}};
            dl->AddPolyline(p, 4, col, 0, thickness + 0.2f);
            break;
        }
        case Icon::Risk: {
            const ImVec2 p[5] = {{mid, y0}, {x1, y0 + q * 0.6f}, {x1 - q * 0.3f, y1},
                                 {x0 + q * 0.3f, y1}, {x0, y0 + q * 0.6f}};
            dl->AddPolyline(p, 5, col, 0, thickness);
            break;
        }
        case Icon::Positions: {
            for (int i = 0; i < 3; ++i) {
                const float oy = y0 + i * q * 0.62f;
                dl->AddRect(ImVec2(x0 + i * 1.5f, oy), ImVec2(x1 - i * 1.5f, oy + q * 0.5f), col,
                            0.0f, 0, thickness);
            }
            break;
        }
        case Icon::Observation: {
            // A lens: two arcs approximated by polylines plus a pupil.
            const ImVec2 top[3] = {{x0, my}, {mid, y0 + q * 0.3f}, {x1, my}};
            const ImVec2 bot[3] = {{x1, my}, {mid, y1 - q * 0.3f}, {x0, my}};
            dl->AddPolyline(top, 3, col, 0, thickness);
            dl->AddPolyline(bot, 3, col, 0, thickness);
            dl->AddCircle(c, q * 0.42f, col, 0, thickness);
            break;
        }
        case Icon::Research: {
            dl->AddCircle(ImVec2(c.x - q * 0.18f, c.y - q * 0.18f), h * 0.62f, col, 0, thickness);
            dl->AddLine(ImVec2(c.x + q * 0.32f, c.y + q * 0.32f), ImVec2(x1, y1), col, thickness);
            break;
        }
        case Icon::Knowledge: {
            dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), col, 1.0f, 0, thickness);
            dl->AddLine(ImVec2(x0 + q * 0.5f, y0), ImVec2(x0 + q * 0.5f, y1), col, thickness);
            break;
        }
        case Icon::Candidates:
        case Icon::Audit: {
            dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), col, 1.0f, 0, thickness);
            for (int i = 0; i < 3; ++i) {
                const float yy = y0 + q * (0.7f + i * 0.7f);
                dl->AddLine(ImVec2(x0 + q * 0.4f, yy), ImVec2(x1 - q * 0.4f, yy), col, thickness);
            }
            if (ic == Icon::Audit)
                dl->AddRect(ImVec2(mid - q * 0.5f, y0 - 1.5f), ImVec2(mid + q * 0.5f, y0 + 2.0f), col);
            break;
        }
        case Icon::Validation:
        case Icon::Approval: {
            dl->AddCircle(c, h, col, 0, thickness);
            dl->AddLine(ImVec2(c.x - q * 0.5f, c.y), ImVec2(c.x - q * 0.1f, c.y + q * 0.45f), col,
                        thickness + 0.2f);
            dl->AddLine(ImVec2(c.x - q * 0.1f, c.y + q * 0.45f), ImVec2(c.x + q * 0.6f, c.y - q * 0.4f),
                        col, thickness + 0.2f);
            break;
        }
        case Icon::Governance: {
            dl->AddLine(ImVec2(mid, y0), ImVec2(mid, y1 - q * 0.2f), col, thickness);
            dl->AddLine(ImVec2(mid - h, y0 + q * 0.5f), ImVec2(mid + h, y0 + q * 0.5f), col, thickness);
            dl->AddTriangle(ImVec2(mid - h, y0 + q * 0.5f), ImVec2(mid - h * 0.5f, y1),
                            ImVec2(mid - h * 1.5f, y1), col);
            dl->AddTriangle(ImVec2(mid + h, y0 + q * 0.5f), ImVec2(mid + h * 1.5f, y1),
                            ImVec2(mid + h * 0.5f, y1), col);
            break;
        }
        case Icon::Evolution: {
            dl->AddLine(ImVec2(x0, my), ImVec2(mid, my), col, thickness);
            dl->AddLine(ImVec2(mid, my), ImVec2(x1, y0 + q * 0.4f), col, thickness);
            dl->AddLine(ImVec2(mid, my), ImVec2(x1, y1 - q * 0.4f), col, thickness);
            dl->AddCircleFilled(ImVec2(x0, my), 2.0f, col);
            dl->AddCircleFilled(ImVec2(x1, y0 + q * 0.4f), 2.0f, col);
            dl->AddCircleFilled(ImVec2(x1, y1 - q * 0.4f), 2.0f, col);
            break;
        }
        case Icon::Incidents: {
            dl->AddTriangle(ImVec2(mid, y0), ImVec2(x1, y1), ImVec2(x0, y1), col, thickness);
            dl->AddLine(ImVec2(mid, y0 + q * 0.8f), ImVec2(mid, y1 - q * 0.7f), col, thickness);
            dl->AddCircleFilled(ImVec2(mid, y1 - q * 0.3f), 1.2f, col);
            break;
        }
        case Icon::Schedule: {
            dl->AddRect(ImVec2(x0, y0 + q * 0.4f), ImVec2(x1, y1), col, 1.0f, 0, thickness);
            dl->AddLine(ImVec2(x0, y0 + q * 0.9f), ImVec2(x1, y0 + q * 0.9f), col, thickness);
            dl->AddLine(ImVec2(x0 + q * 0.6f, y0), ImVec2(x0 + q * 0.6f, y0 + q * 0.6f), col, thickness);
            dl->AddLine(ImVec2(x1 - q * 0.6f, y0), ImVec2(x1 - q * 0.6f, y0 + q * 0.6f), col, thickness);
            break;
        }
        case Icon::Recovery: {
            dl->PathArcTo(c, h, 0.6f, 5.2f, 16);
            dl->PathStroke(col, 0, thickness);
            dl->AddTriangleFilled(ImVec2(c.x + h * 0.55f, c.y - h * 0.55f),
                                  ImVec2(c.x + h * 0.95f, c.y - h * 0.05f),
                                  ImVec2(c.x + h * 0.15f, c.y - h * 0.05f), col);
            break;
        }
        case Icon::Health: {
            const ImVec2 p[6] = {{x0, my}, {x0 + q * 0.6f, my}, {mid - q * 0.2f, y0},
                                 {mid + q * 0.3f, y1}, {x1 - q * 0.6f, my}, {x1, my}};
            dl->AddPolyline(p, 6, col, 0, thickness);
            break;
        }
        case Icon::Configuration: {
            for (int i = 0; i < 2; ++i) {
                const float yy = my + (i == 0 ? -q * 0.55f : q * 0.55f);
                const float kx = (i == 0 ? mid - q * 0.3f : mid + q * 0.4f);
                dl->AddLine(ImVec2(x0, yy), ImVec2(x1, yy), col, thickness);
                dl->AddCircleFilled(ImVec2(kx, yy), 2.2f, col);
            }
            break;
        }
        default:
            dl->AddCircleFilled(c, 1.6f, col);
            break;
    }
}

}  // namespace icons
}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_AURAICONS_H
