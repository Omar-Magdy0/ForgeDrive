#pragma once
#include "IconsFontAwesome7.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <implot.h>
#include <stdint.h>
inline ImVec4 ImRGB(uint8_t r, uint8_t g, uint8_t b, float a = 1.0f) {
  constexpr float INV_255 = 1.0f / 255.0f;
  return ImVec4(r * INV_255, g * INV_255, b * INV_255, a);
}
inline ImVec4 ImHEX(uint32_t hex, float a = 1.0f) { return ImRGB((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, a); }
struct ForgeStyle {
  struct Fonts {
    float   Scale    = 1.0f;
    ImFont* H1       = nullptr;
    ImFont* H2       = nullptr;
    ImFont* H3       = nullptr;
    ImFont* N        = nullptr;
    ImFont* N_bold   = nullptr;
    ImFont* N_italic = nullptr;
    ImFont* Mono     = nullptr;
    ImFont* MonoBold = nullptr;
    ImFont* S        = nullptr;
    ImFont* S_bold   = nullptr;
    ImFont* S_italic = nullptr;
    ImFont* IconL    = nullptr;
    ImFont* IconM    = nullptr;
    ImFont* IconS    = nullptr;
  };
  struct Surface {
    ImVec4 Base;
    ImVec4 Hover;
    ImVec4 Selected;
    ImVec4 Border;
  };
  struct CardConfig {
    ForgeStyle::Surface color;
    ImVec2              padding         = ImVec2(12, 10);
    bool                borderSelect    = false;
    float               borderThickness = 3;
    ImGuiChildFlags     cf              = ImGuiWindowFlags_None;
    ImGuiWindowFlags    wf              = ImGuiWindowFlags_NoScrollbar;
  };
  struct ColorPalette {
    // ---------- Boolean ----------
    ImVec4 True    = ImHEX(0x22C55E);
    ImVec4 False   = ImHEX(0xEF4444);
    ImVec4 Unknown = ImHEX(0x9CA3AF);
    // ---------- Status ----------
    ImVec4 Success = ImHEX(0x22C55E);
    ImVec4 Error   = ImHEX(0xEF4444);
    ImVec4 Warning = ImHEX(0xF59E0B);
    ImVec4 Info    = ImHEX(0x3B82F6);
    ImVec4 Neutral = ImHEX(0x9CA3AF);
    // ---------- Connection ----------
    ImVec4 Connected    = ImHEX(0x22C55E);
    ImVec4 Disconnected = ImHEX(0x6B7280);
    ImVec4 Connecting   = ImHEX(0xF59E0B);
    // ---------- Text ----------
    ImVec4 TextPrimary   = ImHEX(0xE5E7EB);
    ImVec4 TextSecondary = ImHEX(0x9CA3AF);
    ImVec4 TextMuted     = ImHEX(0x6B7280);
    ImVec4 TextDisabled  = ImHEX(0x4B5563);
    ImVec4 TextLink      = ImHEX(0x60A5FA);
    // ---------- Accent ----------
    ImVec4 Primary       = ImHEX(0x00BFFF);
    ImVec4 PrimaryHover  = ImHEX(0x38CFFF);
    ImVec4 PrimaryActive = ImHEX(0x009ACB);
    ImVec4 Secondary     = ImHEX(0x1F3042);
    // ---------- Interaction ----------
    ImVec4 Hover    = ImHEX(0xFFFFFF, 0.08f);
    ImVec4 Active   = ImHEX(0xFFFFFF, 0.14f);
    ImVec4 Selected = ImHEX(0x00BFFF, 0.18f);
    ImVec4 Disabled = ImHEX(0x6B7280, 0.35f);
    ImVec4 Focus    = ImHEX(0x00BFFF, 0.65f);
    ImVec4 DragDrop = ImHEX(0x38BDF8);
    // ---------- Backgrounds ----------
    ImVec4 Background         = ImHEX(0x0C1116);
    ImVec4 BackgroundElevated = ImHEX(0x121A22);
    ImVec4 BackgroundPopup    = ImHEX(0x18232C);
    ImVec4 BackgroundModal    = ImHEX(0x080C11, 0.75f);
    // ---------- Surfaces ----------
    Surface A1{ImHEX(0x0C1116), ImHEX(0x18212A), ImHEX(0x1D2D38), ImHEX(0x263743)};
    Surface A2{ImHEX(0x18232C), ImHEX(0x1E2D38), ImHEX(0x243744), ImHEX(0x304653)};
    Surface A3{ImHEX(0x202832), ImHEX(0x263440), ImHEX(0x2C3C4B), ImHEX(0x3A4C5B)};
    Surface A4{ImHEX(0x005B5C), ImHEX(0x003B5C), ImHEX(0x007A8E), ImHEX(0x40566A)};
    // ---------- General borders ----------
    ImVec4 Border       = ImHEX(0x29404D);
    ImVec4 BorderSubtle = ImHEX(0x202D38);
    ImVec4 BorderStrong = ImHEX(0x405465);
    // ---------- Separators ----------
    ImVec4 Separator        = ImHEX(0x354653);
    ImVec4 SeparatorHovered = ImHEX(0x50809B);
    ImVec4 SeparatorActive  = ImHEX(0x00BFFF);
    // ---------- Scrollbars ----------
    ImVec4 Scrollbar        = ImHEX(0x3B4855);
    ImVec4 ScrollbarHovered = ImHEX(0x526476);
    ImVec4 ScrollbarActive  = ImHEX(0x68839B);
    // ---------- Plots ----------
    ImVec4 PlotBackground = ImHEX(0x0C1116);
    ImVec4 PlotGrid       = ImHEX(0x354653, 0.45f);
    ImVec4 PlotAxis       = ImHEX(0x9CA3AF);
    // Trace colors
    ImVec4 Plot1 = ImHEX(0x00BFFF);
    ImVec4 Plot2 = ImHEX(0xA78BFA);
    ImVec4 Plot3 = ImHEX(0x22C55E);
    ImVec4 Plot4 = ImHEX(0xF59E0B);
    ImVec4 Plot5 = ImHEX(0xEF4444);
    ImVec4 Plot6 = ImHEX(0xEC4899);
    // ---------- Miscellaneous ----------
    ImVec4 Shadow     = ImHEX(0x000000, 0.25f);
    ImVec4 TextSelect = ImHEX(0x00BFFF, 0.30f);
    ImVec4 Unsaved    = ImHEX(0xF59E0B);
  };
  void ApplyTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    // ---------- Text ----------
    s.Colors[ImGuiCol_Text]           = colors.TextPrimary;
    s.Colors[ImGuiCol_TextDisabled]   = colors.TextDisabled;
    s.Colors[ImGuiCol_TextLink]       = colors.TextLink;
    s.Colors[ImGuiCol_TextSelectedBg] = colors.TextSelect;
    // ---------- Backgrounds ----------
    s.Colors[ImGuiCol_WindowBg]         = colors.Background;
    s.Colors[ImGuiCol_ChildBg]          = colors.A1.Base;
    s.Colors[ImGuiCol_PopupBg]          = colors.BackgroundPopup;
    s.Colors[ImGuiCol_ModalWindowDimBg] = colors.BackgroundModal;
    // ---------- Borders ----------
    s.Colors[ImGuiCol_Border]       = colors.Border;
    s.Colors[ImGuiCol_BorderShadow] = colors.Shadow;
    // ---------- Frames ----------
    s.Colors[ImGuiCol_FrameBg]        = colors.A2.Base;
    s.Colors[ImGuiCol_FrameBgHovered] = colors.A2.Hover;
    s.Colors[ImGuiCol_FrameBgActive]  = colors.A2.Selected;
    // ---------- Buttons ----------
    s.Colors[ImGuiCol_Button]        = colors.A2.Base;
    s.Colors[ImGuiCol_ButtonHovered] = colors.A2.Hover;
    s.Colors[ImGuiCol_ButtonActive]  = colors.A2.Selected;
    // ---------- Headers ----------
    s.Colors[ImGuiCol_Header]        = colors.A2.Base;
    s.Colors[ImGuiCol_HeaderHovered] = colors.A2.Hover;
    s.Colors[ImGuiCol_HeaderActive]  = colors.A2.Selected;
    // ---------- Checkboxes / Sliders ----------
    s.Colors[ImGuiCol_CheckMark]          = colors.Primary;
    s.Colors[ImGuiCol_CheckboxSelectedBg] = colors.Selected;
    s.Colors[ImGuiCol_SliderGrab]         = colors.Primary;
    s.Colors[ImGuiCol_SliderGrabActive]   = colors.PrimaryHover;
    // ---------- Separators ----------
    s.Colors[ImGuiCol_Separator]        = colors.Separator;
    s.Colors[ImGuiCol_SeparatorHovered] = colors.SeparatorHovered;
    s.Colors[ImGuiCol_SeparatorActive]  = colors.SeparatorActive;
    // ---------- Scrollbars ----------
    s.Colors[ImGuiCol_ScrollbarGrab]        = colors.Scrollbar;
    s.Colors[ImGuiCol_ScrollbarGrabHovered] = colors.ScrollbarHovered;
    s.Colors[ImGuiCol_ScrollbarGrabActive]  = colors.ScrollbarActive;
    // ---------- Tabs ----------
    s.Colors[ImGuiCol_Tab]                       = colors.A2.Base;
    s.Colors[ImGuiCol_TabHovered]                = colors.A2.Hover;
    s.Colors[ImGuiCol_TabSelected]               = colors.A3.Selected;
    s.Colors[ImGuiCol_TabSelectedOverline]       = colors.Primary;
    s.Colors[ImGuiCol_TabDimmed]                 = colors.A1.Base;
    s.Colors[ImGuiCol_TabDimmedSelected]         = colors.A2.Base;
    s.Colors[ImGuiCol_TabDimmedSelectedOverline] = colors.A3.Border;
    // ---------- Tables ----------
    s.Colors[ImGuiCol_TableHeaderBg]     = colors.A2.Base;
    s.Colors[ImGuiCol_TableBorderStrong] = colors.BorderStrong;
    s.Colors[ImGuiCol_TableBorderLight]  = colors.BorderSubtle;
    s.Colors[ImGuiCol_TableRowBg]        = colors.A1.Base;
    s.Colors[ImGuiCol_TableRowBgAlt]     = colors.A2.Base;
    // ---------- Plots ----------
    s.Colors[ImGuiCol_PlotLines]            = colors.Plot1;
    s.Colors[ImGuiCol_PlotLinesHovered]     = colors.PrimaryHover;
    s.Colors[ImGuiCol_PlotHistogram]        = colors.Plot2;
    s.Colors[ImGuiCol_PlotHistogramHovered] = colors.PrimaryHover;
    // ---------- Navigation / Drag Drop ----------
    s.Colors[ImGuiCol_NavCursor]             = colors.Primary;
    s.Colors[ImGuiCol_NavWindowingHighlight] = colors.Focus;
    s.Colors[ImGuiCol_DragDropTarget]        = colors.DragDrop;
    s.Colors[ImGuiCol_DragDropTargetBg]      = colors.Selected;
    s.Colors[ImGuiCol_UnsavedMarker]         = colors.Unsaved;
    // ---------- Title bars ----------
    s.Colors[ImGuiCol_TitleBg]          = colors.A1.Base;
    s.Colors[ImGuiCol_TitleBgActive]    = colors.A2.Base;
    s.Colors[ImGuiCol_TitleBgCollapsed] = colors.A1.Base;
    s.Colors[ImGuiCol_MenuBarBg]        = colors.A1.Base;
    ApplyPlotTheme();
  }
  void ApplyGeometry() {
    ImGuiStyle& style = ImGui::GetStyle();
    // Window / card geometry
    style.WindowMinSize    = ImVec2(400, 400);
    style.WindowPadding    = ImVec2(12, 12);
    style.WindowRounding   = 0;
    style.WindowBorderSize = 1.0f;
    // Child panels / cards
    style.ChildRounding   = 8.0f;
    style.ChildBorderSize = 0.5f;
    style.PopupRounding   = 7.0f;
    style.PopupBorderSize = 1.0f;
    // Widgets
    style.FramePadding    = ImVec2(10.0f, 6.0f) * fonts.Scale;
    style.FrameRounding   = 5.0f;
    style.FrameBorderSize = 0.0f;
    // Buttons
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    // Layout
    style.ItemSpacing      = ImVec2(10.0f, 8.0f) * fonts.Scale;
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.CellPadding      = ImVec2(8.0f, 6.0f);
    // Separators
    style.SeparatorTextBorderSize = 1.0f;
    // Scrollbars
    style.ScrollbarSize     = 12.0f * fonts.Scale;
    style.ScrollbarRounding = 8.0f;
    // Grab handles
    style.GrabMinSize  = 10.0f;
    style.GrabRounding = 5.0f;
    // Selectable
    style.SelectableTextAlign = ImVec2(0, 0.5);
  }
  void ApplyFonts() {
    ImGuiIO& io = ImGui::GetIO();
    // Font Awesome
    ImFontConfig icon_config;
    icon_config.PixelSnapH              = true;
    static const ImWchar icon_ranges[]  = {ICON_MIN_FA, ICON_MAX_FA, 0};
    auto                 FONT_REGULAR   = FONTS_DIR "/DejaVu/DejaVuSans.ttf";
    auto                 FONT_BOLD      = FONTS_DIR "/DejaVu/DejaVuSans-Bold.ttf";
    auto                 FONT_ITALIC    = FONTS_DIR "/DejaVu/DejaVuSans-Oblique.ttf";
    auto                 FONT_MONO      = FONTS_DIR "/DejaVu/DejaVuSansMono.ttf";
    auto                 FONT_MONO_BOLD = FONTS_DIR "/DejaVu/DejaVuSansMono-Bold.ttf";
    fonts.H1                            = io.Fonts->AddFontFromFileTTF(FONT_REGULAR, 20.0f * fonts.Scale);
    fonts.H2                            = io.Fonts->AddFontFromFileTTF(FONT_REGULAR, 18.0f * fonts.Scale);
    fonts.H3                            = io.Fonts->AddFontFromFileTTF(FONT_REGULAR, 16.0f * fonts.Scale);
    fonts.N                             = io.Fonts->AddFontFromFileTTF(FONT_REGULAR, 14.0f * fonts.Scale);
    fonts.N_bold                        = io.Fonts->AddFontFromFileTTF(FONT_BOLD, 14.0f * fonts.Scale);
    fonts.N_italic                      = io.Fonts->AddFontFromFileTTF(FONT_ITALIC, 14.0f * fonts.Scale);
    fonts.Mono                          = io.Fonts->AddFontFromFileTTF(FONT_MONO, 14 * fonts.Scale);
    fonts.MonoBold                      = io.Fonts->AddFontFromFileTTF(FONT_MONO_BOLD, 14 * fonts.Scale);
    fonts.S                             = io.Fonts->AddFontFromFileTTF(FONT_REGULAR, 12.0f * fonts.Scale);
    fonts.S_bold                        = io.Fonts->AddFontFromFileTTF(FONT_BOLD, 12.0f * fonts.Scale);
    fonts.S_italic                      = io.Fonts->AddFontFromFileTTF(FONT_ITALIC, 12.0f * fonts.Scale);
    fonts.IconL = io.Fonts->AddFontFromFileTTF(FONTS_DIR "/FontAwesome7/Font Awesome 7 Free-Solid-900.otf", 24.0f * fonts.Scale, &icon_config, icon_ranges);
    fonts.IconM = io.Fonts->AddFontFromFileTTF(FONTS_DIR "/FontAwesome7/Font Awesome 7 Free-Solid-900.otf", 18.0f * fonts.Scale, &icon_config, icon_ranges);
    fonts.IconS = io.Fonts->AddFontFromFileTTF(FONTS_DIR "/FontAwesome7/Font Awesome 7 Free-Solid-900.otf", 14.0f * fonts.Scale, &icon_config, icon_ranges);
  }
  void ApplyPlotTheme() {
    ImPlotStyle& s                   = ImPlot::GetStyle();
    s.Colors[ImPlotCol_PlotBg]       = colors.PlotBackground;
    s.Colors[ImPlotCol_FrameBg]      = colors.A1.Base;
    s.Colors[ImPlotCol_PlotBorder]   = colors.Border;
    s.Colors[ImPlotCol_LegendBg]     = colors.BackgroundElevated;
    s.Colors[ImPlotCol_LegendBorder] = colors.Border;
    s.Colors[ImPlotCol_AxisGrid]     = colors.PlotGrid;
    s.Colors[ImPlotCol_AxisText]     = colors.TextSecondary;
    s.Colors[ImPlotCol_AxisTick]     = colors.PlotAxis;
    s.Colors[ImPlotCol_InlayText]    = colors.TextPrimary;
  }
  void ApplyPlotColors() {
    ImPlotStyle& style                   = ImPlot::GetStyle();
    style.Colors[ImPlotCol_PlotBg]       = colors.A1.Base;
    style.Colors[ImPlotCol_FrameBg]      = colors.Background;
    style.Colors[ImPlotCol_PlotBorder]   = colors.Border;
    style.Colors[ImPlotCol_LegendBg]     = colors.BackgroundElevated;
    style.Colors[ImPlotCol_LegendBorder] = colors.Border;
    style.Colors[ImPlotCol_AxisGrid]     = colors.BorderSubtle;
  }
  Fonts        fonts;
  ColorPalette colors;
  CardConfig   C1{{ImVec4(0.095f, 0.108f, 0.132f, 1.0f), ImVec4(0.140f, 0.190f, 0.260f, 1.0f), ImVec4(0.140f, 0.220f, 0.320f, 1.0f),
                   ImVec4(0.300f, 0.400f, 0.530f, 1.0f)},
                  ImVec2(12, 10),
                  false,
                  1.0f};
  CardConfig   C2{{ImVec4(0.085f, 0.098f, 0.120f, 1.0f), ImVec4(0.120f, 0.150f, 0.190f, 1.0f), ImVec4(0.150f, 0.200f, 0.270f, 1.0f),
                   ImVec4(0.200f, 0.230f, 0.280f, 0.75f)},
                  ImVec2(10, 8),
                  true,
                  1.0f};
  CardConfig   C3{{ImVec4(0.075f, 0.085f, 0.105f, 1.0f), ImVec4(0.110f, 0.130f, 0.160f, 1.0f), ImVec4(0.150f, 0.200f, 0.270f, 1.0f),
                   ImVec4(0.200f, 0.230f, 0.280f, 0.0f)},
                  ImVec2(8, 6),
                  false,
                  0.0f};
};
inline bool CardBegin(ImGuiID id, const ForgeStyle& style, bool& selected, ImVec2 size = ImVec2(0, 0), int order = 1, bool autoHeight = false) {
  ForgeStyle::CardConfig cfg;
  switch (order) {
  case 1: cfg = style.C1; break;
  case 2: cfg = style.C2; break;
  case 3: cfg = style.C3; break;
  default: break;
  }
  auto wf = cfg.wf | ImGuiWindowFlags_NoBackground;
  auto cf = (cfg.cf | ImGuiChildFlags_AlwaysUseWindowPadding) & ~ImGuiChildFlags_Borders;
  if (autoHeight) cf |= ImGuiChildFlags_AutoResizeY;
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, cfg.padding);
  ImGui::PushStyleColor(ImGuiCol_Button, style.colors.A4.Base);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, style.colors.A4.Hover);
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, style.colors.A4.Selected);
  if (ImGui::BeginChild(id, size, cf, wf)) {
    bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    bool clicked = hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    if (clicked) selected = !selected;
    ImVec4 color        = (selected && !cfg.borderSelect) ? cfg.color.Selected : hovered ? cfg.color.Hover : cfg.color.Base;
    ImVec4 border_color = (selected && cfg.borderSelect) ? cfg.color.Selected : cfg.color.Border;
    ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(), ImGui::GetColorU32(color),
                                              ImGui::GetStyle().ChildRounding);
    float thickness = cfg.borderThickness;
    ImGui::GetWindowDrawList()->AddRect(ImGui::GetWindowPos() + ImVec2(thickness * 0.5f, thickness * 0.5f),
                                        ImGui::GetWindowPos() + ImGui::GetWindowSize() - ImVec2(thickness * 0.5f, thickness * 0.5f),
                                        ImGui::GetColorU32(border_color), ImGui::GetStyle().ChildRounding, 0, thickness);
    return true;
  }
  return false;
}
inline bool CardBegin(ImGuiID id, const ForgeStyle& style, ImVec2 size = ImVec2(0, 0), int order = 1) {
  bool selected = false;
  return CardBegin(id, style, selected, size, order);
}
inline void CardEnd() {
  ImGui::PopStyleVar();
  ImGui::PopStyleColor(3);
  ImGui::EndChild();
}
#include <algorithm>
#include <functional>
struct GridConfig {
  float minItemWidth = 280.0f;
  float spacingX     = 12.0f;
  float spacingY     = 12.0f;
  int   maxColumns   = 4;
};
inline bool GridBegin(const char* id, const GridConfig& cfg) {
  const float available = ImGui::GetContentRegionAvail().x;
  int         columns   = std::max(1, static_cast<int>((available + cfg.spacingX) / (cfg.minItemWidth + cfg.spacingX)));
  columns               = std::min(columns, cfg.maxColumns);
  bool show             = ImGui::BeginTable(id, columns, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings);
  if (show) ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(cfg.spacingX * 0.5f, cfg.spacingY * 0.5f));
  return show;
}
inline bool GridItem() { return ImGui::TableNextColumn(); }
inline void GridEnd() {
  ImGui::EndTable();
  ImGui::PopStyleVar();
}
enum class AlignX : uint8_t { Left, Center, Right };
enum class AlignY : uint8_t { Top, Center, Bottom };
inline ImVec2 SetNextItemAlignment(float width, float height, AlignX x = AlignX::Left, AlignY y = AlignY::Top) {
  ImVec2 avail = ImGui::GetContentRegionAvail();
  ImVec2 pos   = ImGui::GetCursorPos();
  if (x == AlignX::Center) pos.x += (avail.x - width) * 0.5f;
  else if (x == AlignX::Right) pos.x += avail.x - width;
  if (y == AlignY::Center) pos.y += (avail.y - height) * 0.5f;
  else if (y == AlignY::Bottom) pos.y += avail.y - height;
  ImGui::SetCursorPos(pos);
  return pos;
}
