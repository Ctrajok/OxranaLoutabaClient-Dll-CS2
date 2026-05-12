#include "ui_theme.hpp"
#include "../config/config_vars.hpp"

#include <cmath>
#include <algorithm>

namespace ui {
namespace theme {

// ---------------------------------------------------------------------------
// Default palette values (Cyberpunk-2077-style)
//   Primary   = cyan     #00E5FF  ->  (0.00, 0.90, 1.00)
//   Secondary = magenta  #FF0080  ->  (1.00, 0.00, 0.50)
// ---------------------------------------------------------------------------
static Palette g_palette = {};

static ImU32 PackRGBA(float r, float g, float b, float a)
{
    auto clamp01 = [](float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); };
    int R = (int)(clamp01(r) * 255.0f + 0.5f);
    int G = (int)(clamp01(g) * 255.0f + 0.5f);
    int B = (int)(clamp01(b) * 255.0f + 0.5f);
    int A = (int)(clamp01(a) * 255.0f + 0.5f);
    return IM_COL32(R, G, B, A);
}

static float Luminance(const ImVec4& c)
{
    // Rec.709 luminance — good enough for "is this color too dark?" heuristic.
    return 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z;
}

static ImVec4 Lerp(const ImVec4& a, const ImVec4& b, float t)
{
    return ImVec4(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t);
}

const Palette& Colors()
{
    return g_palette;
}

void RebuildFromUserAccent(const ImVec4& primaryIn, const ImVec4& secondaryIn)
{
    // ---- R1.6: luminance safety fallback ----
    ImVec4 primary = primaryIn;
    ImVec4 secondary = secondaryIn;
    if (Luminance(primary) < 0.05f && Luminance(secondary) < 0.05f)
    {
        primary   = ImVec4(0.00f, 0.90f, 1.00f, 1.00f);  // cyan
        secondary = ImVec4(1.00f, 0.00f, 0.50f, 1.00f);  // magenta
    }

    // Make sure alpha is 1 for ImGui style slots; we'll dim where needed.
    primary.w   = 1.0f;
    secondary.w = 1.0f;

    ImVec4 accentDimV  = ImVec4(primary.x, primary.y, primary.z, 0.55f);
    ImVec4 accentMidV  = Lerp(primary, secondary, 0.5f);

    // Backgrounds — dark neutral cyber base, slight blue tint.
    ImVec4 bgBase     = ImVec4(0.050f, 0.055f, 0.075f, 0.94f);   // ~#0D0E13
    ImVec4 bgPanel    = ImVec4(0.080f, 0.085f, 0.110f, 0.90f);   // ~#14161C
    ImVec4 bgOverlay  = ImVec4(0.060f, 0.065f, 0.090f, 0.96f);
    ImVec4 bgInput    = ImVec4(0.110f, 0.120f, 0.155f, 1.00f);

    // Text — near-white primary, muted grey secondary.
    ImVec4 textPri    = ImVec4(0.92f, 0.94f, 0.97f, 1.00f);
    ImVec4 textMuted  = ImVec4(0.55f, 0.58f, 0.66f, 1.00f);

    // Status
    ImVec4 ok         = ImVec4(0.30f, 1.00f, 0.55f, 1.00f);
    ImVec4 warn       = ImVec4(1.00f, 0.80f, 0.20f, 1.00f);
    ImVec4 danger     = ImVec4(1.00f, 0.30f, 0.35f, 1.00f);

    // Lines
    ImVec4 grid       = ImVec4(1.00f, 1.00f, 1.00f, 0.024f);     // ~6/255 alpha
    ImVec4 border     = ImVec4(1.00f, 1.00f, 1.00f, 0.08f);

    g_palette.Accent           = PackRGBA(primary.x, primary.y, primary.z, 1.0f);
    g_palette.AccentDim        = PackRGBA(primary.x, primary.y, primary.z, 0.55f);
    g_palette.AccentSecondary  = PackRGBA(secondary.x, secondary.y, secondary.z, 1.0f);
    g_palette.AccentMid        = PackRGBA(accentMidV.x, accentMidV.y, accentMidV.z, 1.0f);

    g_palette.BgBase           = PackRGBA(bgBase.x, bgBase.y, bgBase.z, bgBase.w);
    g_palette.BgPanel          = PackRGBA(bgPanel.x, bgPanel.y, bgPanel.z, bgPanel.w);
    g_palette.BgOverlay        = PackRGBA(bgOverlay.x, bgOverlay.y, bgOverlay.z, bgOverlay.w);
    g_palette.BgInputField     = PackRGBA(bgInput.x, bgInput.y, bgInput.z, bgInput.w);

    g_palette.TextPrimary      = PackRGBA(textPri.x, textPri.y, textPri.z, 1.0f);
    g_palette.TextMuted        = PackRGBA(textMuted.x, textMuted.y, textMuted.z, 1.0f);

    g_palette.Success          = PackRGBA(ok.x, ok.y, ok.z, 1.0f);
    g_palette.Warning          = PackRGBA(warn.x, warn.y, warn.z, 1.0f);
    g_palette.Danger           = PackRGBA(danger.x, danger.y, danger.z, 1.0f);

    g_palette.GridLine         = PackRGBA(grid.x, grid.y, grid.z, grid.w);
    g_palette.BorderSubtle     = PackRGBA(border.x, border.y, border.z, border.w);

    g_palette.AccentV          = primary;
    g_palette.AccentSecondaryV = secondary;
    g_palette.BgBaseV          = bgBase;
    g_palette.BgPanelV         = bgPanel;
    g_palette.BgOverlayV       = bgOverlay;
    g_palette.BgInputFieldV    = bgInput;
    g_palette.TextPrimaryV     = textPri;
    g_palette.TextMutedV       = textMuted;
}

// Initialize palette at static-init time so Colors() works even before the
// first explicit RebuildFromUserAccent call.
struct PaletteInit
{
    PaletteInit()
    {
        RebuildFromUserAccent(
            ImVec4(0.00f, 0.90f, 1.00f, 1.00f),
            ImVec4(1.00f, 0.00f, 0.50f, 1.00f));
    }
};
static PaletteInit s_paletteInit;

// ---------------------------------------------------------------------------
// AnimatedValue / timing helpers
// ---------------------------------------------------------------------------
float SafeDeltaTime()
{
    float dt = ImGui::GetIO().DeltaTime;
    if (!(dt == dt)) return 0.0f;           // NaN guard
    if (dt < 0.0f) return 0.0f;
    if (dt > 0.05f) return 0.05f;
    return dt;
}

void AnimatedValue::Tick(float dt)
{
    // Master kill-switch: if animations disabled, snap to target instantly.
    if (!config::g_uiAnimationsEnabled)
    {
        current = target;
    }
    else
    {
        if (!(dt == dt) || dt < 0.0f) dt = 0.0f;
        float k = dt * speed;
        if (k > 1.0f) k = 1.0f;
        current += (target - current) * k;
    }

    // Bound invariant: clamp and scrub NaN/Inf.
    if (!(current == current)) current = target;          // NaN -> target
    if (current < 0.0f) current = 0.0f;
    if (current > 1.0f) current = 1.0f;
}

ImU32 LerpColor(ImU32 a, ImU32 b, float t)
{
    if (t <= 0.0f) return a;
    if (t >= 1.0f) return b;

    int ar = (a      ) & 0xFF, ag = (a >> 8) & 0xFF, ab_ = (a >> 16) & 0xFF, aa = (a >> 24) & 0xFF;
    int br = (b      ) & 0xFF, bg = (b >> 8) & 0xFF, bb_ = (b >> 16) & 0xFF, ba = (b >> 24) & 0xFF;

    int r = ar + (int)((br  - ar ) * t + 0.5f);
    int g = ag + (int)((bg  - ag ) * t + 0.5f);
    int bC = ab_ + (int)((bb_ - ab_) * t + 0.5f);
    int aC = aa + (int)((ba  - aa ) * t + 0.5f);

    return IM_COL32(r, g, bC, aC);
}

float EaseOutCubic(float t)
{
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    float u = 1.0f - t;
    return 1.0f - u * u * u;
}

float EaseOutBack(float t)
{
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float u = t - 1.0f;
    return 1.0f + c3 * u * u * u + c1 * u * u;
}

// Shared open-animation progress (AnimatedValue reused across frames).
static float s_openAnimProgress = 1.0f; // start "fully open" so first frame isn't dark

void ResetOpenAnimation()
{
    s_openAnimProgress = 0.0f;
}

float& OpenAnimationProgress()
{
    return s_openAnimProgress;
}

} // namespace theme
} // namespace ui
