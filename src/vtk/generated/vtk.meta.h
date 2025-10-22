// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef VTK_META_H
#define VTK_META_H

typedef enum VTK_IconKind
{
VTK_IconKind_Null,
VTK_IconKind_FolderOpenOutline,
VTK_IconKind_FolderClosedOutline,
VTK_IconKind_FolderOpenFilled,
VTK_IconKind_FolderClosedFilled,
VTK_IconKind_FileOutline,
VTK_IconKind_FileFilled,
VTK_IconKind_Play,
VTK_IconKind_PlayStepForward,
VTK_IconKind_Pause,
VTK_IconKind_Stop,
VTK_IconKind_Info,
VTK_IconKind_WarningSmall,
VTK_IconKind_WarningBig,
VTK_IconKind_Unlocked,
VTK_IconKind_Locked,
VTK_IconKind_LeftArrow,
VTK_IconKind_RightArrow,
VTK_IconKind_UpArrow,
VTK_IconKind_DownArrow,
VTK_IconKind_Gear,
VTK_IconKind_Pencil,
VTK_IconKind_Trash,
VTK_IconKind_Pin,
VTK_IconKind_RadioHollow,
VTK_IconKind_RadioFilled,
VTK_IconKind_CheckHollow,
VTK_IconKind_CheckFilled,
VTK_IconKind_LeftCaret,
VTK_IconKind_RightCaret,
VTK_IconKind_UpCaret,
VTK_IconKind_DownCaret,
VTK_IconKind_UpScroll,
VTK_IconKind_DownScroll,
VTK_IconKind_LeftScroll,
VTK_IconKind_RightScroll,
VTK_IconKind_Add,
VTK_IconKind_Minus,
VTK_IconKind_Thread,
VTK_IconKind_Threads,
VTK_IconKind_Machine,
VTK_IconKind_CircleFilled,
VTK_IconKind_X,
VTK_IconKind_Refresh,
VTK_IconKind_Undo,
VTK_IconKind_Redo,
VTK_IconKind_Save,
VTK_IconKind_Window,
VTK_IconKind_Target,
VTK_IconKind_Clipboard,
VTK_IconKind_Scheduler,
VTK_IconKind_Module,
VTK_IconKind_XSplit,
VTK_IconKind_YSplit,
VTK_IconKind_ClosePanel,
VTK_IconKind_StepInto,
VTK_IconKind_StepOver,
VTK_IconKind_StepOut,
VTK_IconKind_Find,
VTK_IconKind_Palette,
VTK_IconKind_Thumbnails,
VTK_IconKind_Glasses,
VTK_IconKind_Binoculars,
VTK_IconKind_List,
VTK_IconKind_Grid,
VTK_IconKind_QuestionMark,
VTK_IconKind_Person,
VTK_IconKind_Briefcase,
VTK_IconKind_Dot,
VTK_IconKind_COUNT,
} VTK_IconKind;

typedef enum VTK_ThemeColor
{
VTK_ThemeColor_Null,
VTK_ThemeColor_Text,
VTK_ThemeColor_TextPositive,
VTK_ThemeColor_TextNegative,
VTK_ThemeColor_TextNeutral,
VTK_ThemeColor_TextWeak,
VTK_ThemeColor_Cursor,
VTK_ThemeColor_CursorInactive,
VTK_ThemeColor_Focus,
VTK_ThemeColor_Hover,
VTK_ThemeColor_DropShadow,
VTK_ThemeColor_DisabledOverlay,
VTK_ThemeColor_DropSiteOverlay,
VTK_ThemeColor_InactivePanelOverlay,
VTK_ThemeColor_SelectionOverlay,
VTK_ThemeColor_HighlightOverlay,
VTK_ThemeColor_HighlightOverlayError,
VTK_ThemeColor_BaseBackground,
VTK_ThemeColor_BaseBackgroundAlt,
VTK_ThemeColor_BaseBorder,
VTK_ThemeColor_MenuBarBackground,
VTK_ThemeColor_MenuBarBackgroundAlt,
VTK_ThemeColor_MenuBarBorder,
VTK_ThemeColor_FloatingBackground,
VTK_ThemeColor_FloatingBackgroundAlt,
VTK_ThemeColor_FloatingBorder,
VTK_ThemeColor_ImplicitButtonBackground,
VTK_ThemeColor_ImplicitButtonBorder,
VTK_ThemeColor_PlainButtonBackground,
VTK_ThemeColor_PlainButtonBorder,
VTK_ThemeColor_PositivePopButtonBackground,
VTK_ThemeColor_PositivePopButtonBorder,
VTK_ThemeColor_NegativePopButtonBackground,
VTK_ThemeColor_NegativePopButtonBorder,
VTK_ThemeColor_NeutralPopButtonBackground,
VTK_ThemeColor_NeutralPopButtonBorder,
VTK_ThemeColor_ScrollBarButtonBackground,
VTK_ThemeColor_ScrollBarButtonBorder,
VTK_ThemeColor_TabBackground,
VTK_ThemeColor_TabBorder,
VTK_ThemeColor_TabBackgroundInactive,
VTK_ThemeColor_TabBorderInactive,
VTK_ThemeColor_CodeDefault,
VTK_ThemeColor_CodeSymbol,
VTK_ThemeColor_CodeType,
VTK_ThemeColor_CodeLocal,
VTK_ThemeColor_CodeRegister,
VTK_ThemeColor_CodeKeyword,
VTK_ThemeColor_CodeDelimiterOperator,
VTK_ThemeColor_CodeNumeric,
VTK_ThemeColor_CodeNumericAltDigitGroup,
VTK_ThemeColor_CodeString,
VTK_ThemeColor_CodeMeta,
VTK_ThemeColor_CodeComment,
VTK_ThemeColor_CodeLineNumbers,
VTK_ThemeColor_CodeLineNumbersSelected,
VTK_ThemeColor_LineInfoBackground0,
VTK_ThemeColor_LineInfoBackground1,
VTK_ThemeColor_LineInfoBackground2,
VTK_ThemeColor_LineInfoBackground3,
VTK_ThemeColor_LineInfoBackground4,
VTK_ThemeColor_LineInfoBackground5,
VTK_ThemeColor_LineInfoBackground6,
VTK_ThemeColor_LineInfoBackground7,
VTK_ThemeColor_Thread0,
VTK_ThemeColor_Thread1,
VTK_ThemeColor_Thread2,
VTK_ThemeColor_Thread3,
VTK_ThemeColor_Thread4,
VTK_ThemeColor_Thread5,
VTK_ThemeColor_Thread6,
VTK_ThemeColor_Thread7,
VTK_ThemeColor_ThreadUnwound,
VTK_ThemeColor_ThreadError,
VTK_ThemeColor_Breakpoint,
VTK_ThemeColor_CacheLineBoundary,
VTK_ThemeColor_COUNT,
} VTK_ThemeColor;

typedef enum VTK_ThemePreset
{
VTK_ThemePreset_DefaultDark,
VTK_ThemePreset_DefaultLight,
VTK_ThemePreset_VSDark,
VTK_ThemePreset_VSLight,
VTK_ThemePreset_SolarizedDark,
VTK_ThemePreset_SolarizedLight,
VTK_ThemePreset_HandmadeHero,
VTK_ThemePreset_FourCoder,
VTK_ThemePreset_FarManager,
VTK_ThemePreset_COUNT,
} VTK_ThemePreset;

typedef enum VTK_SettingCode
{
VTK_SettingCode_HoverAnimations,
VTK_SettingCode_PressAnimations,
VTK_SettingCode_FocusAnimations,
VTK_SettingCode_TooltipAnimations,
VTK_SettingCode_MenuAnimations,
VTK_SettingCode_ScrollingAnimations,
VTK_SettingCode_TabWidth,
VTK_SettingCode_MainFontSize,
VTK_SettingCode_CodeFontSize,
VTK_SettingCode_COUNT,
} VTK_SettingCode;

C_LINKAGE_BEGIN
extern String8 vtk_icon_kind_text_table[69];
extern String8 vtk_theme_preset_display_string_table[9];
extern String8 vtk_theme_preset_code_string_table[9];
extern Vec4F32 vtk_theme_preset_colors__default_dark[76];
extern Vec4F32 vtk_theme_preset_colors__default_light[76];
extern Vec4F32 vtk_theme_preset_colors__vs_dark[76];
extern Vec4F32 vtk_theme_preset_colors__vs_light[76];
extern Vec4F32 vtk_theme_preset_colors__solarized_dark[76];
extern Vec4F32 vtk_theme_preset_colors__solarized_light[76];
extern Vec4F32 vtk_theme_preset_colors__handmade_hero[76];
extern Vec4F32 vtk_theme_preset_colors__four_coder[76];
extern Vec4F32 vtk_theme_preset_colors__far_manager[76];
extern Vec4F32* vtk_theme_preset_colors_table[9];
extern String8 vtk_theme_color_display_string_table[76];
extern String8 vtk_theme_color_cfg_string_table[76];
extern String8 vtk_setting_code_display_string_table[9];
extern String8 vtk_setting_code_lower_string_table[9];
extern VTK_SettingVal vtk_setting_code_default_val_table[9];
extern Rng1S32 vtk_setting_code_s32_range_table[9];

C_LINKAGE_END

#endif // VTK_META_H
