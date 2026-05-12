#pragma once

// Cyber theme: centralized palette, dimensions, and animation framework.
// Implementation: ui_theme.cpp

#include "../../imgui-1.92.5/imgui.h"

namespace ui {
namespace theme {

// ---------------------------------------------------------------------------
// Palette
// ---------------------------------------------------------------------------
// All colors are pre-cached ImU32 so hot-path render code doesn't call
// ColorConvertFloat4ToU32 every frame.
struct Palette
{
    // Accents
    ImU32 Accent;           // primary neon (cyan by default)
    ImU32 AccentDim;        // accent * 0.55 alpha (for secondary fills)
    ImU32 AccentSecondary;  // secondary neon (magenta by default)
    ImU32 AccentMid;        // lerp(Accent, AccentSecondary, 0.5) — cached for gradient borders

    // Backgrounds
    ImU32 BgBase;           // main window background
    ImU32 BgPanel;           // inside CyberBeginPanel
    ImU32 BgOverlay;         // popup / context menus
    ImU32 BgInputField;      // slider / combo frame

    // Text
    ImU32 TextPrimary;
    ImU32 TextMuted;

    // Status
    ImU32 Success;
    ImU32 Warning;
    ImU32 Danger;

    // Lines
    ImU32 GridLine;          // scanline overlay
    ImU32 BorderSubtle;      // faint inner outline

    // Float-form originals for places that still need ImVec4 (e.g. ImGuiStyle.Colors)
    ImVec4 AccentV;
    ImVec4 AccentSecondaryV;
    ImVec4 BgBaseV;
    ImVec4 BgPanelV;
    ImVec4 BgOverlayV;
    ImVec4 BgInputFieldV;
    ImVec4 TextPrimaryV;
    ImVec4 TextMutedV;
};

const Palette& Colors();

// Recompute all derived colors from two user-chosen accents.
// Implements luminance safety fallback when both accents are too dark.
void RebuildFromUserAccent(const ImVec4& primary, const ImVec4& secondary);

// ---------------------------------------------------------------------------
// Dimensions (constexpr — compile-time)
// ---------------------------------------------------------------------------
constexpr float kWindowW        = 780.0f;
constexpr float kWindowH        = 520.0f;
constexpr float kSidebarW       = 170.0f;
constexpr float kHeaderH        = 44.0f;
constexpr float kFooterH        = 28.0f;
constexpr float kPanelRounding  = 6.0f;
constexpr float kWidgetRounding = 4.0f;
constexpr float kSidebarItemH   = 46.0f;
constexpr float kContentPadding = 18.0f;

// ---------------------------------------------------------------------------
// Animation framework
// ---------------------------------------------------------------------------
// Framerate-independent lerp with bounded output in [0, 1].
// Typical usage:
//   auto* anim = ImGui::GetStateStorage()->GetFloatRef(id, 0.0f);
//   AnimatedValue v{*anim, target, speed};
//   v.Tick(SafeDeltaTime());
//   *anim = v.current;
struct AnimatedValue
{
    float  current;
    float  target;
    float  speed;   // larger = faster convergence

    // Advances current toward target. dt should be SafeDeltaTime().
    // Guarantees: current stays in [0, 1], never NaN/Inf.
    void Tick(float dt);
};

// Clamps ImGui::GetIO().DeltaTime to [0, 0.05] — protects animations from
// frame hiccups (alt-tab pauses, breakpoints, etc.).
float SafeDeltaTime();

// Per-channel linear interpolation of two pre-packed ImU32 colors.
ImU32 LerpColor(ImU32 a, ImU32 b, float t);

// Scalar easing helpers (t in [0, 1]).
float EaseOutCubic(float t);
float EaseOutBack(float t);

// Called once when the menu transitions from closed->open to reset
// the open-animation progress to 0 (so fade-in replays).
void ResetOpenAnimation();

// Retrieve current open-animation progress in [0, 1].
// Driven by the menu draw code each frame via Tick.
float& OpenAnimationProgress();

} // namespace theme
} // namespace ui
