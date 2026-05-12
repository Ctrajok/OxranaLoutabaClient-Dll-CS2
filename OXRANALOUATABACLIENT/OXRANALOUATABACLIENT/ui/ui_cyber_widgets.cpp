#include "ui_cyber_widgets.hpp"
#include "ui_theme.hpp"
#include "../config/config_vars.hpp"

#include "../../imgui-1.92.5/imgui.h"

#include <cmath>
#include <cstring>
#include <cstdio>

namespace ui {

using namespace ui::theme;

// ===========================================================================
// Internal helpers
// ===========================================================================
namespace {

// Small float trig wrappers so we don't depend on <math.h>'s cosf/sinf.
inline float fcos(float a) { return (float)std::cos((float)a); }
inline float fsin(float a) { return (float)std::sin((float)a); }

// Animated float stored via ImGui state storage (no out-params needed).
static float TickAnim(ImGuiID id, float target, float speed)
{
    float* p = ImGui::GetStateStorage()->GetFloatRef(id, target);
    AnimatedValue v{ *p, target, speed };
    v.Tick(SafeDeltaTime());
    *p = v.current;
    return v.current;
}

} // namespace

// ===========================================================================
// Overlay primitives
// ===========================================================================
void CyberDrawGlassFill(ImVec2 mn, ImVec2 mx, float rounding)
{
    const Palette& P = Colors();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Base fill
    dl->AddRectFilled(mn, mx, P.BgBase, rounding);

    // Vertical vignette — top slightly lighter, bottom darker.
    ImU32 topTint    = IM_COL32(40, 44, 64, 32);
    ImU32 bottomTint = IM_COL32(0, 0, 0, 64);
    dl->AddRectFilledMultiColor(mn, mx, topTint, topTint, bottomTint, bottomTint);

    // Top inner highlight line
    dl->AddLine(ImVec2(mn.x + 1.0f, mn.y + 1.0f), ImVec2(mx.x - 1.0f, mn.y + 1.0f),
                IM_COL32(255, 255, 255, 20), 1.0f);
}

void CyberDrawGradientBorder(ImVec2 mn, ImVec2 mx, float rounding)
{
    const Palette& P = Colors();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Dual-color accent border: primary (cyan) + secondary (magenta) offset for a neon feel.
    // Outer soft halo first, then two crisp lines.
    ImU32 haloCol = (P.Accent & 0x00FFFFFFu) | ((ImU32)32 << 24);
    dl->AddRect(ImVec2(mn.x - 1.0f, mn.y - 1.0f), ImVec2(mx.x + 1.0f, mx.y + 1.0f),
                haloCol, rounding + 1.0f, 0, 1.0f);

    // Primary accent outline
    dl->AddRect(mn, mx, P.Accent, rounding, 0, 1.5f);

    // Magenta accent — only on top-right and bottom-left edges to fake a gradient direction
    ImU32 secCol = (P.AccentSecondary & 0x00FFFFFFu) | ((ImU32)140 << 24);
    // top-right corner hint
    dl->AddLine(ImVec2(mx.x - rounding - 24.0f, mn.y), ImVec2(mx.x - rounding, mn.y), secCol, 1.5f);
    dl->AddLine(ImVec2(mx.x, mn.y + rounding), ImVec2(mx.x, mn.y + rounding + 24.0f), secCol, 1.5f);
    // bottom-left corner hint
    dl->AddLine(ImVec2(mn.x, mx.y - rounding - 24.0f), ImVec2(mn.x, mx.y - rounding), secCol, 1.5f);
    dl->AddLine(ImVec2(mn.x + rounding, mx.y), ImVec2(mn.x + rounding + 24.0f, mx.y), secCol, 1.5f);
}

void CyberDrawHoverGlow(ImVec2 mn, ImVec2 mx, float rounding, float alpha01)
{
    if (!config::g_uiHoverGlowEnabled) return;
    if (alpha01 <= 0.001f) return;

    const Palette& P = Colors();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 base = P.Accent;

    float steps[3] = { 1.0f, 3.0f, 5.0f };
    float as[3]    = { alpha01, alpha01 * 0.5f, alpha01 * 0.25f };

    for (int i = 0; i < 3; ++i)
    {
        int alpha = (int)(as[i] * 180.0f);
        if (alpha <= 0) continue;
        ImU32 col = (base & 0x00FFFFFFu) | ((ImU32)alpha << 24);
        ImVec2 p0 = ImVec2(mn.x - steps[i], mn.y - steps[i]);
        ImVec2 p1 = ImVec2(mx.x + steps[i], mx.y + steps[i]);
        dl->AddRect(p0, p1, col, rounding + steps[i], 0, 1.5f);
    }
}

void CyberDrawScanlines(ImVec2 mn, ImVec2 mx)
{
    if (!config::g_uiScanlineEnabled) return;

    const Palette& P = Colors();
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    dl->PushClipRect(mn, mx, true);

    float intensity = config::g_uiScanlineIntensity;
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;
    int alpha = (int)(24.0f * intensity);
    if (alpha <= 0) { dl->PopClipRect(); return; }
    ImU32 col = (P.GridLine & 0x00FFFFFFu) | ((ImU32)alpha << 24);

    for (float y = mn.y; y < mx.y; y += 3.0f)
    {
        dl->AddLine(ImVec2(mn.x, y), ImVec2(mx.x, y), col, 1.0f);
    }

    dl->PopClipRect();
}

// ===========================================================================
// CyberToggle
// ===========================================================================
bool CyberToggle(const char* label, bool* v)
{
    const Palette& P = Colors();
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    const float width  = 40.0f;
    const float height = 20.0f;
    const float radius = height * 0.5f;

    ImGui::InvisibleButton(label, ImVec2(width, height));
    bool clicked = ImGui::IsItemClicked();
    if (clicked) *v = !*v;
    bool hovered = ImGui::IsItemHovered();

    ImGuiID id        = ImGui::GetID(label);
    float   anim      = TickAnim(id, *v ? 1.0f : 0.0f, 14.0f);
    float   hoverAnim = TickAnim((ImGuiID)(id ^ 0xA5A5A5A5u), hovered ? 1.0f : 0.0f, 12.0f);

    // Track color: input-field -> accent, lerped via anim.
    ImU32 trackCol = LerpColor(P.BgInputField, P.Accent, anim);

    if (*v) {
        CyberDrawHoverGlow(p, ImVec2(p.x + width, p.y + height), radius, anim * 0.6f + hoverAnim * 0.4f);
    } else if (hovered) {
        CyberDrawHoverGlow(p, ImVec2(p.x + width, p.y + height), radius, hoverAnim);
    }

    dl->AddRectFilled(p, ImVec2(p.x + width, p.y + height), trackCol, radius);

    // Knob with a subtle shadow
    float knobX = p.x + radius + (width - radius * 2.0f) * anim;
    dl->AddCircleFilled(ImVec2(knobX + 1.0f, p.y + radius + 1.0f), radius - 2.0f, IM_COL32(0, 0, 0, 80));
    dl->AddCircleFilled(ImVec2(knobX, p.y + radius), radius - 2.0f, IM_COL32(245, 248, 255, 255));

    ImGui::SameLine();
    ImGui::TextUnformatted(label);
    return clicked;
}

// ===========================================================================
// CyberButton
// ===========================================================================
bool CyberButton(const char* label, ImVec2 size)
{
    const Palette& P = Colors();

    if (size.x == 0.0f || size.y == 0.0f) {
        ImVec2 ts = ImGui::CalcTextSize(label);
        if (size.x == 0.0f) size.x = ts.x + 28.0f;
        if (size.y == 0.0f) size.y = 32.0f;
    }

    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(label, size);
    bool clicked = ImGui::IsItemClicked();
    bool hovered = ImGui::IsItemHovered();
    bool held    = ImGui::IsItemActive();

    ImGuiID id        = ImGui::GetID(label);
    float   hoverAnim = TickAnim((ImGuiID)(id ^ 0x5A5A5A5Au), hovered ? 1.0f : 0.0f, 12.0f);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p1 = ImVec2(p.x + size.x, p.y + size.y);

    if (hovered) CyberDrawHoverGlow(p, p1, kWidgetRounding, hoverAnim);

    ImU32 fillCol;
    if (held) {
        fillCol = LerpColor(P.BgInputField, P.Accent, 0.25f);
    } else if (hovered) {
        fillCol = LerpColor(P.BgInputField, P.Accent, 0.12f * hoverAnim);
    } else {
        fillCol = P.BgInputField;
    }
    dl->AddRectFilled(p, p1, fillCol, kWidgetRounding);

    CyberDrawGradientBorder(p, p1, kWidgetRounding);

    ImVec2 ts = ImGui::CalcTextSize(label);
    ImVec2 tp = ImVec2(p.x + (size.x - ts.x) * 0.5f, p.y + (size.y - ts.y) * 0.5f);
    dl->AddText(tp, P.TextPrimary, label);

    return clicked;
}

// ===========================================================================
// CyberSlider / CyberSliderInt
// ===========================================================================
bool CyberSlider(const char* label, float* v, float vmin, float vmax, const char* fmt)
{
    const Palette& P = Colors();
    ImGui::PushStyleColor(ImGuiCol_FrameBg,          P.BgInputField);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,   LerpColor(P.BgInputField, P.Accent, 0.15f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,    LerpColor(P.BgInputField, P.Accent, 0.30f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab,       P.Accent);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, P.AccentMid);
    bool r = ImGui::SliderFloat(label, v, vmin, vmax, fmt);
    ImGui::PopStyleColor(5);
    return r;
}

bool CyberSliderInt(const char* label, int* v, int vmin, int vmax, const char* fmt)
{
    const Palette& P = Colors();
    ImGui::PushStyleColor(ImGuiCol_FrameBg,          P.BgInputField);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,   LerpColor(P.BgInputField, P.Accent, 0.15f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,    LerpColor(P.BgInputField, P.Accent, 0.30f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab,       P.Accent);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, P.AccentMid);
    bool r = ImGui::SliderInt(label, v, vmin, vmax, fmt);
    ImGui::PopStyleColor(5);
    return r;
}

// ===========================================================================
// CyberCombo
// ===========================================================================
bool CyberCombo(const char* label, int* current_item, const char* const items[], int items_count)
{
    const Palette& P = Colors();
    ImGui::PushStyleColor(ImGuiCol_FrameBg,        P.BgInputField);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, LerpColor(P.BgInputField, P.Accent, 0.15f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  LerpColor(P.BgInputField, P.Accent, 0.30f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg,        P.BgOverlay);
    ImGui::PushStyleColor(ImGuiCol_Header,         LerpColor(P.BgInputField, P.Accent, 0.25f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered,  LerpColor(P.BgInputField, P.Accent, 0.40f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,   P.Accent);
    bool r = ImGui::Combo(label, current_item, items, items_count);
    ImGui::PopStyleColor(7);
    return r;
}

// ===========================================================================
// CyberColorEdit4
// ===========================================================================
bool CyberColorEdit4(const char* label, ImVec4* col, ImGuiColorEditFlags flags)
{
    ImGuiColorEditFlags f = flags;
    if ((f & ImGuiColorEditFlags_NoInputs) == 0 && (f & ImGuiColorEditFlags_DisplayRGB) == 0)
        f |= ImGuiColorEditFlags_NoInputs;
    return ImGui::ColorEdit4(label, (float*)col, f);
}

// ===========================================================================
// CyberSettingsButton (gear icon, drawn with primitives)
// ===========================================================================
bool CyberSettingsButton(const char* id)
{
    const Palette& P = Colors();
    ImVec2 p = ImGui::GetCursorScreenPos();
    const float sz = 18.0f;

    ImGui::InvisibleButton(id, ImVec2(sz, sz));
    bool clicked = ImGui::IsItemClicked();
    bool hovered = ImGui::IsItemHovered();

    ImGuiID gid       = ImGui::GetID(id);
    float   hoverAnim = TickAnim((ImGuiID)(gid ^ 0x3C3C3C3Cu), hovered ? 1.0f : 0.0f, 12.0f);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 c = ImVec2(p.x + sz * 0.5f, p.y + sz * 0.5f);
    float r1 = sz * 0.28f;
    float r2 = sz * 0.45f;

    ImU32 iconCol = LerpColor(P.TextMuted, P.Accent, hoverAnim);

    // 6 radial spokes
    for (int i = 0; i < 6; ++i)
    {
        float a  = (3.14159265f / 3.0f) * (float)i;
        float cx = fcos(a);
        float cy = fsin(a);
        dl->AddLine(ImVec2(c.x + cx * r1, c.y + cy * r1),
                    ImVec2(c.x + cx * r2, c.y + cy * r2),
                    iconCol, 1.5f);
    }
    dl->AddCircle(c, r1, iconCol, 16, 1.5f);
    dl->AddCircleFilled(c, sz * 0.10f, iconCol);

    if (hovered) {
        CyberDrawHoverGlow(p, ImVec2(p.x + sz, p.y + sz), sz * 0.5f, hoverAnim);
    }

    return clicked;
}

// ===========================================================================
// Layout primitives
// ===========================================================================
bool CyberBeginPanel(const char* id, ImVec2 size)
{
    const Palette& P = Colors();
    ImVec2 p = ImGui::GetCursorScreenPos();

    if (size.x == 0.0f) size.x = ImGui::GetContentRegionAvail().x;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, P.BgPanel);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, kPanelRounding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));

    bool opened = ImGui::BeginChild(id, size, ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);

    // Top highlight line (drawn in parent draw-list for simplicity)
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddLine(ImVec2(p.x + 6.0f, p.y + 1.0f),
                ImVec2(p.x + size.x - 6.0f, p.y + 1.0f),
                IM_COL32(255, 255, 255, 20), 1.0f);
    (void)P;

    return opened;
}

void CyberEndPanel()
{
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

void CyberSectionHeader(const char* label)
{
    const Palette& P = Colors();
    ImGui::PushStyleColor(ImGuiCol_Text, P.TextMuted);
    ImGui::SetWindowFontScale(1.05f);
    ImGui::TextUnformatted(label);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    ImVec2 lineStart = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddLine(lineStart, ImVec2(lineStart.x + w, lineStart.y), P.Accent, 1.0f);
    ImGui::Dummy(ImVec2(0, 4));
}

bool CyberSidebarItem(const char* id, const char* icon, const char* label, const char* sublabel, bool isActive)
{
    const Palette& P = Colors();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    const float h = kSidebarItemH;

    ImGui::InvisibleButton(id, ImVec2(w, h));
    bool clicked = ImGui::IsItemClicked();
    bool hovered = ImGui::IsItemHovered();

    ImGuiID gid        = ImGui::GetID(id);
    float   hoverAnim  = TickAnim((ImGuiID)(gid ^ 0xB1B1B1B1u), hovered ? 1.0f : 0.0f, 10.0f);
    float   activeAnim = TickAnim((ImGuiID)(gid ^ 0x2D2D2D2Du), isActive ? 1.0f : 0.0f, 10.0f);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p1 = ImVec2(p.x + w, p.y + h);

    // Hover fill
    if (hoverAnim > 0.0f) {
        int alpha = (int)(24.0f * hoverAnim);
        dl->AddRectFilled(p, p1, (P.Accent & 0x00FFFFFFu) | ((ImU32)alpha << 24), 4.0f);
    }

    // Active background + left glow bar
    if (activeAnim > 0.0f) {
        int alpha = (int)(40.0f * activeAnim);
        dl->AddRectFilled(p, p1, (P.Accent & 0x00FFFFFFu) | ((ImU32)alpha << 24), 4.0f);
        dl->AddRectFilled(ImVec2(p.x, p.y + 6.0f), ImVec2(p.x + 3.0f, p1.y - 6.0f), P.Accent, 1.5f);
    }

    ImU32 textCol = LerpColor(P.TextMuted, P.TextPrimary, activeAnim + hoverAnim * 0.3f);
    dl->AddText(ImVec2(p.x + 14.0f, p.y + 10.0f), textCol, icon);
    dl->AddText(ImVec2(p.x + 40.0f, p.y + 8.0f),  textCol, label);

    if (sublabel && sublabel[0]) {
        ImFont* font = ImGui::GetFont();
        float   baseSize = ImGui::GetFontSize();
        dl->AddText(font, baseSize * 0.85f, ImVec2(p.x + 40.0f, p.y + 24.0f), P.TextMuted, sublabel);
    }

    return clicked;
}

} // namespace ui
