#include "generated/vtk.meta.c"

// FIXME: testing
internal F64
bswap_f64(F64 value)
{
  U8 *bytes = (uint8_t *)&value;  // Cast to byte array
  U8 swapped_bytes[8];

  // Swap the bytes (endian conversion)
  for(U64 i = 0; i < 8; i++)
  {
    swapped_bytes[i] = bytes[7 - i];  // Reverse the byte order
  }

  F64 swapped_value;
  U8 *swapped_ptr = (U8 *)&swapped_value;

  // Copy swapped bytes back into swapped_value
  for(U64 i = 0; i < 8; i++)
  {
    swapped_ptr[i] = swapped_bytes[i];
  }

  return swapped_value;
}

/////////////////////////////////
//~ Basic Type Functions

internal U64
vtk_hash_from_string(U64 seed, String8 string)
{
  U64 result = XXH3_64bits_withSeed(string.str, string.size, seed);
  return result;
}

internal String8
vtk_hash_part_from_key_string(String8 string)
{
  String8 result = string;
  // k: look for ### patterns, which can replace the entirety of the part of
  // the string that is hashed.
  U64 hash_replace_signifier_pos = str8_find_needle(string, 0, str8_lit("###"), 0);
  if(hash_replace_signifier_pos < string.size)
  {
    result = str8_skip(string, hash_replace_signifier_pos);
  }
  return result;
}

internal String8
vtk_display_part_from_key_string(String8 string)
{
  U64 hash_pos = str8_find_needle(string, 0, str8_lit("##"), 0);
  string.size = hash_pos;
  return string;
}

/////////////////////////////////
//~ Key

internal VTK_Key
vtk_key_from_string(VTK_Key seed, String8 string)
{
  VTK_Key result = {0};
  if(string.size != 0)
  {
    String8 hash_part = vtk_hash_part_from_key_string(string);
    result.u64[0] = vtk_hash_from_string(seed.u64[0], hash_part);
  }
  return result;
}

internal VTK_Key
vtk_key_from_stringf(VTK_Key seed, char* fmt, ...)
{
  Temp scratch = scratch_begin(0,0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);

  VTK_Key result = {0};
  result = vtk_key_from_string(seed, string);
  scratch_end(scratch);
  return result;
}

internal B32
vtk_key_match(VTK_Key a, VTK_Key b)
{
  return MemoryMatchStruct(&a,&b);
}

internal VTK_Key
vtk_key_make(U64 a, U64 b)
{
  VTK_Key result = {a,b};
  return result;
}

internal VTK_Key
vtk_key_zero()
{
  return (VTK_Key){0}; 
}

internal Vec2F32
vtk_2f32_from_key(VTK_Key key)
{
  U32 high = (key.u64[0]>>32);
  U32 low = key.u64[0];
  Vec2F32 ret = {*((F32*)&high), *((F32*)&low)};
  return ret;
}

internal VTK_Key
vtk_key_from_2f32(Vec2F32 key_2f32)
{
  U32 high = *((U32*)(&key_2f32.x));
  U32 low = *((U32*)(&key_2f32.y));
  U64 key = ((U64)high<<32) | ((U64)low);
  VTK_Key ret = {key, 0};
  return ret;
}

/////////////////////////////////
//~ Entry Call Functions

internal void
vtk_init(OS_Handle os_wnd, R_Handle r_wnd)
{
  Arena *arena = arena_alloc();
  vtk_state = push_array(arena, VTK_State, 1);
  vtk_state->arena = arena;
  vtk_state->r_wnd = r_wnd;
  vtk_state->os_wnd = os_wnd;
  vtk_state->dpi = vtk_state->last_dpi = os_dpi_from_window(os_wnd);
  vtk_state->window_rect = vtk_state->last_window_rect = os_client_rect_from_window(os_wnd, 1);
  vtk_state->window_dim = vtk_state->last_window_dim = dim_2f32(vtk_state->window_rect);
  {
    vtk_state->camera.zf = 1000.0;
    vtk_state->camera.zn = 1000.0;
    vtk_state->camera.fov = 0.25;
    vtk_state->camera.position = v3f32(0,-3,-3);
    vtk_state->camera.rotation = make_indentity_quat_f32();
  }

  for(U64 i = 0; i < ArrayCount(vtk_state->frame_arenas); i++)
  {
    vtk_state->frame_arenas[i] = arena_alloc(.reserve_size = MB(64), .commit_size = MB(32));
  }

  for(U64 i = 0; i < ArrayCount(vtk_state->drawlists); i++)
  {
    // NOTE(k): some device offers 256MB memory which is both cpu visiable and device local
    vtk_state->drawlists[i] = vtk_drawlist_alloc(arena, MB(32), MB(32));
  }

  // FIXME: TESTING
  {
    // String8 vtk_path = str8_lit("./data/cube.vtk");
    // String8 vtk_path = str8_lit("./data/windtunnel-0024vh32nbiexakk112md8pvr/pressure_field_mesh.vtk");
    String8 vtk_path = str8_lit("./data/windtunnel-003s4dbyouskfw48y2gewpn18/pressure_field_mesh.vtk");
    VTK_Mesh *mesh = vtk_mesh_from_vtk(arena, vtk_path);
    vtk_state->mesh = mesh;
  }

  // Settings
  for EachEnumVal(VTK_SettingCode, code)
  {
    vtk_state->setting_vals[code] = vtk_setting_code_default_val_table[code];
  }
  vtk_state->setting_vals[VTK_SettingCode_MainFontSize].s32 = vtk_state->setting_vals[VTK_SettingCode_MainFontSize].s32 * (vtk_state->last_dpi/96.f);
  vtk_state->setting_vals[VTK_SettingCode_CodeFontSize].s32 = vtk_state->setting_vals[VTK_SettingCode_CodeFontSize].s32 * (vtk_state->last_dpi/96.f);
  vtk_state->setting_vals[VTK_SettingCode_MainFontSize].s32 = ClampBot(vtk_state->setting_vals[VTK_SettingCode_MainFontSize].s32, vtk_setting_code_default_val_table[VTK_SettingCode_MainFontSize].s32);
  vtk_state->setting_vals[VTK_SettingCode_CodeFontSize].s32 = ClampBot(vtk_state->setting_vals[VTK_SettingCode_CodeFontSize].s32, vtk_setting_code_default_val_table[VTK_SettingCode_CodeFontSize].s32);

  // Fonts
#if BUILD_DEBUG
  vtk_state->cfg_font_tags[VTK_FontSlot_Main] = fnt_tag_from_path(str8_lit("./data/fonts/segoeui.ttf"));
  vtk_state->cfg_font_tags[VTK_FontSlot_Code] = fnt_tag_from_path(str8_lit("./data/fonts/segoeui.ttf"));
  vtk_state->cfg_font_tags[VTK_FontSlot_Icons] = fnt_tag_from_path(str8_lit("./data/fonts/icons.ttf"));
  vtk_state->cfg_font_tags[VTK_FontSlot_IconsExtra] = fnt_tag_from_path(str8_lit("./data/fonts/icons_extra.ttf"));
  vtk_state->cfg_font_tags[VTK_FontSlot_HandWrite] = fnt_tag_from_path(str8_lit("./data/fonts/Virgil.ttf"));
#else
  // String8 font_mono = str8(ttf_Mplus1Code_Medium, ttf_Mplus1Code_Medium_len);
  String8 font_mono = str8(ttf_segoeui, ttf_segoeui_len);
  String8 font_icons = str8(ttf_icons, ttf_icons_len);
  String8 font_icons_extra = str8(ttf_icons_extra, ttf_icons_extra_len);
  String8 font_virgil = str8(ttf_Virgil, ttf_Virgil_len);
  String8 font_xiaolai = str8(ttf_XiaolaiMono_Regular, ttf_XiaolaiMono_Regular_len);
  vtk_state->cfg_font_tags[VTK_FontSlot_Main] = fnt_tag_from_static_data_string(&font_mono);
  vtk_state->cfg_font_tags[VTK_FontSlot_Code] = fnt_tag_from_static_data_string(&font_mono);
  vtk_state->cfg_font_tags[VTK_FontSlot_Icons] = fnt_tag_from_static_data_string(&font_icons);
  vtk_state->cfg_font_tags[VTK_FontSlot_IconsExtra] = fnt_tag_from_static_data_string(&font_icons_extra);
  vtk_state->cfg_font_tags[VTK_FontSlot_HandWrite1] = fnt_tag_from_static_data_string(&font_virgil);
  vtk_state->cfg_font_tags[VTK_FontSlot_HandWrite2] = fnt_tag_from_static_data_string(&font_xiaolai);
#endif

  // Theme 
  MemoryCopy(vtk_state->cfg_theme_target.colors, vtk_theme_preset_colors__handmade_hero, sizeof(vtk_theme_preset_colors__handmade_hero));
  MemoryCopy(vtk_state->cfg_theme.colors, vtk_theme_preset_colors__handmade_hero, sizeof(vtk_theme_preset_colors__handmade_hero));

  //////////////////////////////
  //- k: compute palettes from theme
  {
    VTK_Theme *current = &vtk_state->cfg_theme;

    // ui palette
    for EachEnumVal(VTK_PaletteCode, code)
    {
      vtk_state->cfg_ui_debug_palettes[code].null       = v4f32(1, 0, 1, 1);
      vtk_state->cfg_ui_debug_palettes[code].cursor     = current->colors[VTK_ThemeColor_Cursor];
      vtk_state->cfg_ui_debug_palettes[code].selection  = current->colors[VTK_ThemeColor_SelectionOverlay];
    }
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_Base].background              = current->colors[VTK_ThemeColor_BaseBackground];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_Base].text                    = current->colors[VTK_ThemeColor_Text];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_Base].text_weak               = current->colors[VTK_ThemeColor_TextWeak];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_Base].border                  = current->colors[VTK_ThemeColor_BaseBorder];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_MenuBar].background           = current->colors[VTK_ThemeColor_MenuBarBackground];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_MenuBar].text                 = current->colors[VTK_ThemeColor_Text];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_MenuBar].text_weak            = current->colors[VTK_ThemeColor_TextWeak];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_MenuBar].border               = current->colors[VTK_ThemeColor_MenuBarBorder];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_Floating].background          = current->colors[VTK_ThemeColor_FloatingBackground];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_Floating].text                = current->colors[VTK_ThemeColor_Text];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_Floating].text_weak           = current->colors[VTK_ThemeColor_TextWeak];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_Floating].border              = current->colors[VTK_ThemeColor_FloatingBorder];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_ImplicitButton].background    = current->colors[VTK_ThemeColor_ImplicitButtonBackground];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_ImplicitButton].text          = current->colors[VTK_ThemeColor_Text];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_ImplicitButton].text_weak     = current->colors[VTK_ThemeColor_TextWeak];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_ImplicitButton].border        = current->colors[VTK_ThemeColor_ImplicitButtonBorder];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_PlainButton].background       = current->colors[VTK_ThemeColor_PlainButtonBackground];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_PlainButton].text             = current->colors[VTK_ThemeColor_Text];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_PlainButton].text_weak        = current->colors[VTK_ThemeColor_TextWeak];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_PlainButton].border           = current->colors[VTK_ThemeColor_PlainButtonBorder];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_PositivePopButton].background = current->colors[VTK_ThemeColor_PositivePopButtonBackground];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_PositivePopButton].text       = current->colors[VTK_ThemeColor_Text];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_PositivePopButton].text_weak  = current->colors[VTK_ThemeColor_TextWeak];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_PositivePopButton].border     = current->colors[VTK_ThemeColor_PositivePopButtonBorder];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_NegativePopButton].background = current->colors[VTK_ThemeColor_NegativePopButtonBackground];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_NegativePopButton].text       = current->colors[VTK_ThemeColor_Text];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_NegativePopButton].text_weak  = current->colors[VTK_ThemeColor_TextWeak];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_NegativePopButton].border     = current->colors[VTK_ThemeColor_NegativePopButtonBorder];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_NeutralPopButton].background  = current->colors[VTK_ThemeColor_NeutralPopButtonBackground];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_NeutralPopButton].text        = current->colors[VTK_ThemeColor_Text];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_NeutralPopButton].text_weak   = current->colors[VTK_ThemeColor_TextWeak];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_NeutralPopButton].border      = current->colors[VTK_ThemeColor_NeutralPopButtonBorder];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_ScrollBarButton].background   = current->colors[VTK_ThemeColor_ScrollBarButtonBackground];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_ScrollBarButton].text         = current->colors[VTK_ThemeColor_Text];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_ScrollBarButton].text_weak    = current->colors[VTK_ThemeColor_TextWeak];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_ScrollBarButton].border       = current->colors[VTK_ThemeColor_ScrollBarButtonBorder];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_Tab].background               = current->colors[VTK_ThemeColor_TabBackground];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_Tab].text                     = current->colors[VTK_ThemeColor_Text];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_Tab].text_weak                = current->colors[VTK_ThemeColor_TextWeak];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_Tab].border                   = current->colors[VTK_ThemeColor_TabBorder];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_TabInactive].background       = current->colors[VTK_ThemeColor_TabBackgroundInactive];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_TabInactive].text             = current->colors[VTK_ThemeColor_Text];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_TabInactive].text_weak        = current->colors[VTK_ThemeColor_TextWeak];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_TabInactive].border           = current->colors[VTK_ThemeColor_TabBorderInactive];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_DropSiteOverlay].background   = current->colors[VTK_ThemeColor_DropSiteOverlay];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_DropSiteOverlay].text         = current->colors[VTK_ThemeColor_DropSiteOverlay];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_DropSiteOverlay].text_weak    = current->colors[VTK_ThemeColor_DropSiteOverlay];
    vtk_state->cfg_ui_debug_palettes[VTK_PaletteCode_DropSiteOverlay].border       = current->colors[VTK_ThemeColor_DropSiteOverlay];

    // main palette
    for EachEnumVal(VTK_PaletteCode, code)
    {
      vtk_state->cfg_main_palettes[code].null      = v4f32(1, 0, 1, 1);
      vtk_state->cfg_main_palettes[code].cursor    = current->colors[VTK_ThemeColor_Cursor];
      vtk_state->cfg_main_palettes[code].selection = current->colors[VTK_ThemeColor_SelectionOverlay];
    }
    vtk_state->cfg_main_palettes[VTK_PaletteCode_Base].background              = current->colors[VTK_ThemeColor_BaseBackground];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_Base].text                    = current->colors[VTK_ThemeColor_Text];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_Base].text_weak               = current->colors[VTK_ThemeColor_TextWeak];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_Base].border                  = current->colors[VTK_ThemeColor_BaseBorder];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_MenuBar].background           = current->colors[VTK_ThemeColor_MenuBarBackground];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_MenuBar].text                 = current->colors[VTK_ThemeColor_Text];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_MenuBar].text_weak            = current->colors[VTK_ThemeColor_TextWeak];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_MenuBar].border               = current->colors[VTK_ThemeColor_MenuBarBorder];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_Floating].background          = current->colors[VTK_ThemeColor_FloatingBackground];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_Floating].text                = current->colors[VTK_ThemeColor_Text];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_Floating].text_weak           = current->colors[VTK_ThemeColor_TextWeak];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_Floating].border              = current->colors[VTK_ThemeColor_FloatingBorder];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_ImplicitButton].background    = current->colors[VTK_ThemeColor_ImplicitButtonBackground];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_ImplicitButton].text          = current->colors[VTK_ThemeColor_Text];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_ImplicitButton].text_weak     = current->colors[VTK_ThemeColor_TextWeak];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_ImplicitButton].border        = current->colors[VTK_ThemeColor_ImplicitButtonBorder];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_PlainButton].background       = current->colors[VTK_ThemeColor_PlainButtonBackground];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_PlainButton].text             = current->colors[VTK_ThemeColor_Text];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_PlainButton].text_weak        = current->colors[VTK_ThemeColor_TextWeak];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_PlainButton].border           = current->colors[VTK_ThemeColor_PlainButtonBorder];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_PositivePopButton].background = current->colors[VTK_ThemeColor_PositivePopButtonBackground];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_PositivePopButton].text       = current->colors[VTK_ThemeColor_Text];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_PositivePopButton].text_weak  = current->colors[VTK_ThemeColor_TextWeak];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_PositivePopButton].border     = current->colors[VTK_ThemeColor_PositivePopButtonBorder];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_NegativePopButton].background = current->colors[VTK_ThemeColor_NegativePopButtonBackground];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_NegativePopButton].text       = current->colors[VTK_ThemeColor_Text];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_NegativePopButton].text_weak  = current->colors[VTK_ThemeColor_TextWeak];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_NegativePopButton].border     = current->colors[VTK_ThemeColor_NegativePopButtonBorder];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_NeutralPopButton].background  = current->colors[VTK_ThemeColor_NeutralPopButtonBackground];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_NeutralPopButton].text        = current->colors[VTK_ThemeColor_Text];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_NeutralPopButton].text_weak   = current->colors[VTK_ThemeColor_TextWeak];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_NeutralPopButton].border      = current->colors[VTK_ThemeColor_NeutralPopButtonBorder];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_ScrollBarButton].background   = current->colors[VTK_ThemeColor_ScrollBarButtonBackground];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_ScrollBarButton].text         = current->colors[VTK_ThemeColor_Text];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_ScrollBarButton].text_weak    = current->colors[VTK_ThemeColor_TextWeak];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_ScrollBarButton].border       = current->colors[VTK_ThemeColor_ScrollBarButtonBorder];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_Tab].background               = current->colors[VTK_ThemeColor_TabBackground];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_Tab].text                     = current->colors[VTK_ThemeColor_Text];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_Tab].text_weak                = current->colors[VTK_ThemeColor_TextWeak];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_Tab].border                   = current->colors[VTK_ThemeColor_TabBorder];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_TabInactive].background       = current->colors[VTK_ThemeColor_TabBackgroundInactive];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_TabInactive].text             = current->colors[VTK_ThemeColor_Text];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_TabInactive].text_weak        = current->colors[VTK_ThemeColor_TextWeak];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_TabInactive].border           = current->colors[VTK_ThemeColor_TabBorderInactive];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_DropSiteOverlay].background   = current->colors[VTK_ThemeColor_DropSiteOverlay];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_DropSiteOverlay].text         = current->colors[VTK_ThemeColor_DropSiteOverlay];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_DropSiteOverlay].text_weak    = current->colors[VTK_ThemeColor_DropSiteOverlay];
    vtk_state->cfg_main_palettes[VTK_PaletteCode_DropSiteOverlay].border       = current->colors[VTK_ThemeColor_DropSiteOverlay];
  }
}

// FIXME: testing
internal void 
vtk_update(void)
{
  // camera control
  {
    VTK_Camera *camera = &vtk_state->camera;
    if(vtk_state->sig.f & UI_SignalFlag_LeftDragging)
    {
      Vec2F32 delta = ui_drag_delta();
    }

    Vec3F32 f = {0};
    Vec3F32 s = {0};
    Vec3F32 u = {0};
    Mat4x4F32 rot_mat = mat_4x4f32_from_quat_f32(camera->rotation);
    s = v3f32(rot_mat.v[0][0], rot_mat.v[0][1], rot_mat.v[0][2]); // i
    u = v3f32(rot_mat.v[1][0], rot_mat.v[1][1], rot_mat.v[1][2]); // j
    f = v3f32(rot_mat.v[2][0], rot_mat.v[2][1], rot_mat.v[2][2]); // k

    typedef struct VTK_CameraDragData VTK_CameraDragData;
    struct VTK_CameraDragData
    {
      Vec3F32 drag_start_position;
      QuatF32 drag_start_rotation;
    };

    if(vtk_state->sig.f & UI_SignalFlag_MiddleDragging)
    {
      if(vtk_state->sig.f & UI_SignalFlag_MiddlePressed)
      {
        VTK_CameraDragData drag = {camera->position, camera->rotation};
        ui_store_drag_struct(&drag);
      }
      VTK_CameraDragData drag = *ui_get_drag_struct(VTK_CameraDragData);
      Vec2F32 delta = ui_drag_delta();

      // TODO: how to scale the moving distance
      F32 h_speed_per_screen_px = 4./vtk_state->window_dim.x;
      F32 h_pct = -delta.x * h_speed_per_screen_px;
      Vec3F32 h_dist = scale_3f32(s, h_pct);

      F32 v_speed_per_screen_px = 4./vtk_state->window_dim.y;
      F32 v_pct = -delta.y * v_speed_per_screen_px;
      Vec3F32 v_dist = scale_3f32(u, v_pct);

      Vec3F32 position = drag.drag_start_position;
      position = add_3f32(position, h_dist);
      position = add_3f32(position, v_dist);
      camera->position = position;
    }

    if(vtk_state->sig.f & UI_SignalFlag_RightDragging)
    {
      if(vtk_state->sig.f & UI_SignalFlag_RightPressed)
      {
        VTK_CameraDragData drag = {camera->position, camera->rotation};
        ui_store_drag_struct(&drag);
      }
      VTK_CameraDragData drag = *ui_get_drag_struct(VTK_CameraDragData);
      Vec2F32 delta = ui_drag_delta();

      F32 h_turn = -delta.x * (1.f) * (1.f/vtk_state->window_dim.x);
      F32 v_turn = -delta.y * (0.5f) * (1.f/vtk_state->window_dim.y);

      QuatF32 rotation = drag.drag_start_rotation;
      QuatF32 h_q = make_rotate_quat_f32(v3f32(0,-1,0), h_turn);
      rotation = mul_quat_f32(h_q, rotation);

      Vec3F32 side = mul_quat_f32_v3f32(rotation, v3f32(1,0,0));
      QuatF32 v_q = make_rotate_quat_f32(side, v_turn);
      rotation = mul_quat_f32(v_q, rotation);

      camera->rotation = rotation;
    }

    // Scroll
    if(vtk_state->sig.scroll.x != 0 || vtk_state->sig.scroll.y != 0)
    {
      Vec3F32 dist = scale_3f32(f, -vtk_state->sig.scroll.y/3.f);
      camera->position = add_3f32(dist, camera->position);
    }
  }

  // FIXME: what?
  VTK_Mesh *mesh = vtk_state->mesh;
  // Vec4F32 colors[6] = {v4f32(1,0,0,1), v4f32(0,1,0,1), v4f32(0,0,1,1)};
  Vec4F32 colors[6] = {
    v4f32(1.0f, 0.0f, 0.0f, 1.0f),  // Red
    v4f32(0.0f, 1.0f, 0.0f, 1.0f),  // Green
    v4f32(0.0f, 0.0f, 1.0f, 1.0f),  // Blue
    v4f32(1.0f, 1.0f, 0.0f, 1.0f),  // Yellow
    v4f32(1.0f, 0.0f, 1.0f, 1.0f),  // Magenta
    v4f32(0.0f, 1.0f, 1.0f, 1.0f),  // Cyan
  };
  {
#if 0
    R_Geo3D_Vertex *vertices = push_array(vtk_frame_arena(), R_Geo3D_Vertex, 3);
    vertices[0] = (R_Geo3D_Vertex){.pos = v3f32(0,0,0), .nor = v3f32(0,0,0), .col = v4f32(1,0,0,1)};
    vertices[1] = (R_Geo3D_Vertex){.pos = v3f32(1,0,0), .nor = v3f32(0,0,0), .col = v4f32(0,0,1,1)};
    vertices[2] = (R_Geo3D_Vertex){.pos = v3f32(0,-1,0), .nor = v3f32(0,0,0), .col = v4f32(0,1,0,1)};
    U64 vertex_count = 3;
    U32 *indices = push_array(vtk_frame_arena(), U32, 3);
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    U64 indice_count = 3;
#else
    // vertex
    U64 vertex_count = mesh->point_count;
    R_Geo3D_Vertex *vertices = push_array(vtk_frame_arena(), R_Geo3D_Vertex, vertex_count);
    for(U64 vertex_idx = 0; vertex_idx < vertex_count; vertex_idx++)
    {
      vertices[vertex_idx].pos = mesh->points[vertex_idx].pos;
      vertices[vertex_idx].pos.y = -vertices[vertex_idx].pos.y;

      Vec4F32 color_start = colors[vertex_idx%ArrayCount(colors)];
      Vec4F32 color_end = colors[(vertex_idx+3)%ArrayCount(colors)];

      local_persist F32 t = 0.0;
      local_persist int mul = 1;
      local_persist F32 cycle_duration = 30000.0; // Duration in seconds for a full cycle

      // Increment t with respect to frame_dt and mul, ensuring the value of t stays within 0-1
      t += vtk_state->frame_dt * mul;

      if (t >= cycle_duration) {
        t = cycle_duration;  // Ensure t doesn't exceed the cycle duration
        mul = -1;  // Reverse direction
      }
      else if (t <= 0) {
        t = 0;  // Ensure t doesn't go below 0
        mul = 1;  // Reverse direction
      }

      Vec4F32 color = mix_4f32(color_start, color_end, t/cycle_duration);
      vertices[vertex_idx].col = color;
    }

#if 0
    for(U64 cell_idx = 0; cell_idx < mesh->cell_count; cell_idx+=15)
    {
      VTK_Cell *cell = &mesh->cells[cell_idx];

      // indice
      U64 indice_count = (cell->point_count-2)*3;
      U32 *indices = push_array(vtk_frame_arena(), U32, indice_count);
      U32 indice_idx = 0;

      // polygon to triangles
      for(U64 i = 0; i < (cell->point_count-2); i++)
      {
        U32 a = cell->point_indices[0];
        U32 b = cell->point_indices[i + 1];
        U32 c = cell->point_indices[i + 2];
        indices[indice_idx++] = a;
        indices[indice_idx++] = b;
        indices[indice_idx++] = c;
      }

      // FIXME: hack for now
      {
        R_Geo3D_Vertex *this_vertices = cell_idx == 0 ? vertices : 0;
        U64 this_vertex_count = cell_idx == 0 ? vertex_count : 0;
        VTK_DrawNode *draw_node = vtk_drawlist_push(vtk_frame_arena(), vtk_frame_drawlist(), this_vertices, this_vertex_count, indices, indice_count);
        // draw_node->topology = R_GeoTopologyKind_TriangleStrip;
        draw_node->topology = R_GeoTopologyKind_Triangles;
        draw_node->polygon = R_GeoPolygonKind_Fill;
      }
    }
#else
    U32 *indices = 0;
    for(U64 cell_idx = 0; cell_idx < mesh->cell_count; cell_idx++)
    {
      VTK_Cell *cell = &mesh->cells[cell_idx];

      // indice
      U64 indice_count = (cell->point_count-2)*3;
      // polygon to triangles
      for(U64 i = 0; i < (cell->point_count-2); i++)
      {
        U32 a = cell->point_indices[0];
        U32 b = cell->point_indices[i + 1];
        U32 c = cell->point_indices[i + 2];
        darray_push(vtk_frame_arena(), indices, a);
        darray_push(vtk_frame_arena(), indices, b);
        darray_push(vtk_frame_arena(), indices, c);
      }
    }

    VTK_DrawNode *draw_node = vtk_drawlist_push(vtk_frame_arena(), vtk_frame_drawlist(), vertices, vertex_count, indices, darray_size(indices));
    // draw_node->topology = R_GeoTopologyKind_TriangleStrip;
    draw_node->topology = R_GeoTopologyKind_Triangles;
    draw_node->polygon = R_GeoPolygonKind_Fill;

#endif
#endif
  }

  vtk_drawlist_build(vtk_frame_drawlist());

  // drawing
  DR_BucketScope(vtk_state->bucket_main)
  {
    // camera
#if 0
    // Vec3F32 eye = {3, -9, -3};
    // Mat4x4F32 view_mat = make_look_at_vulkan_4x4f32(eye, v3f32(0,0,0), v3f32(0,-1,0));
#else
    VTK_Camera *camera = &vtk_state->camera;
    Mat4x4F32 rot_mat = mat_4x4f32_from_quat_f32(camera->rotation);
    Mat4x4F32 tran_mat = make_translate_4x4f32(camera->position);
    Mat4x4F32 view_mat = inverse_4x4f32(mul_4x4f32(tran_mat, rot_mat));
#endif

    Rng2F32 viewport = vtk_state->window_rect;
    Vec2F32 viewport_dim = dim_2f32(viewport);
    Mat4x4F32 proj_mat = make_perspective_vulkan_4x4f32(0.25, viewport_dim.x/viewport_dim.y, 0.1, 100);
    R_PassParams_Geo3D *pass_params = dr_geo3d_begin(viewport, view_mat, proj_mat);
    pass_params->omit_light = 1;
    pass_params->lights = 0;
    pass_params->materials = push_array(vtk_frame_arena(), R_Geo3D_Material, R_MAX_MATERIALS_PER_PASS);
    pass_params->textures = push_array(vtk_frame_arena(), R_Geo3D_PackedTextures, R_MAX_MATERIALS_PER_PASS);

    // load default material
    pass_params->materials[0].diffuse_color = v4f32(1,1,1,1);
    pass_params->materials[0].opacity = 1.0f;
    pass_params->material_count = 1;

    U64 inst_idx = 0;
    for(VTK_DrawNode *draw_node = vtk_frame_drawlist()->first_node; draw_node != 0; draw_node = draw_node->next, inst_idx++)
    {
      R_Mesh3DInst *inst = dr_mesh(draw_node->vertices, draw_node->indices,
                                   draw_node->vertices_buffer_offset, draw_node->indices_buffer_offset, draw_node->indice_count,
                                   draw_node->topology, draw_node->polygon,
                                   R_GeoVertexFlag_TexCoord|R_GeoVertexFlag_Normals|R_GeoVertexFlag_RGB,
                                   0, 0,
                                   0, 2, 0);

      // U64 mat_idx = pass_params->material_count;
      // // pass_params->materials[mat_idx].diffuse_color = clr;
      // pass_params->materials[mat_idx].opacity = 1.0;
      // pass_params->materials[mat_idx].has_diffuse_texture = 0;
      // pass_params->textures[mat_idx].array[R_GeoTexKind_Diffuse] = r_handle_zero();
      // pass_params->material_count++;

      // fill inst info 
      Mat4x4F32 xform = make_translate_4x4f32(v3f32(0,-3,0));
      // Mat4x4F32 xform = mat_4x4f32(1.0);
      inst->xform = xform;
      inst->xform_inv = inverse_4x4f32(xform);
      inst->omit_light = 1;
      inst->draw_edge = 0;
      // inst->color_override = colors[inst_idx%ArrayCount(colors)];
      // inst->key 
    }
  }
}

internal B32
vtk_frame(void)
{
  ProfBeginFunction();

  /////////////////////////////////
  //~ Do per-frame resets

  vtk_drawlist_reset(vtk_frame_drawlist());
  arena_clear(vtk_frame_arena());

  /////////////////////////////////
  //~ Remake drawing buckets every frame

  dr_begin_frame();
  vtk_state->bucket_ui = dr_bucket_make();
  vtk_state->bucket_main = dr_bucket_make();

  /////////////////////////////////
  //~ Get events from os
  
  OS_EventList os_events = os_get_events(vtk_frame_arena(), 0);
  vtk_state->last_window_rect = vtk_state->window_rect;
  vtk_state->last_window_dim = dim_2f32(vtk_state->last_window_rect);
  vtk_state->window_rect = os_client_rect_from_window(vtk_state->os_wnd, 0);
  vtk_state->window_res_changed = vtk_state->window_rect.x0 != vtk_state->last_window_rect.x0 || vtk_state->window_rect.x1 != vtk_state->last_window_rect.x1 || vtk_state->window_rect.y0 != vtk_state->last_window_rect.y0 || vtk_state->window_rect.y1 != vtk_state->last_window_rect.y1;
  vtk_state->window_dim = dim_2f32(vtk_state->window_rect);
  vtk_state->last_mouse = vtk_state->mouse;
  vtk_state->mouse = os_window_is_focused(vtk_state->os_wnd) ? os_mouse_from_window(vtk_state->os_wnd) : v2f32(-100,-100);
  vtk_state->mouse_delta = sub_2f32(vtk_state->mouse, vtk_state->last_mouse);
  vtk_state->last_dpi = vtk_state->dpi;
  vtk_state->dpi = os_dpi_from_window(vtk_state->os_wnd);

  /////////////////////////////////
  //~ Calculate avg length in us of last many frames

  U64 frame_time_history_avg_us = 0;
  ProfScope("calculate avg length in us of last many frames")
  {
    U64 num_frames_in_history = Min(ArrayCount(vtk_state->frame_time_us_history), vtk_state->frame_index);
    U64 frame_time_history_sum_us = 0;
    if(num_frames_in_history > 0)
    {
      for(U64 i = 0; i < num_frames_in_history; i++)
      {
        frame_time_history_sum_us += vtk_state->frame_time_us_history[i];
      }
      frame_time_history_avg_us = frame_time_history_sum_us/num_frames_in_history;
    }
  }

  /////////////////////////////////
  //~ Pick target hz

  // pick among a number of sensible targets to snap to, given how well we've been performing
  F32 target_hz = !os_window_is_focused(vtk_state->os_wnd) ? 10.0f : os_get_gfx_info()->default_refresh_rate;
  if(vtk_state->frame_index > 32)
  {
    F32 possible_alternate_hz_targets[] = {target_hz, 60.f, 120.f, 144.f, 240.f};
    F32 best_target_hz = target_hz;
    S64 best_target_hz_frame_time_us_diff = max_S64;
    for(U64 idx = 0; idx < ArrayCount(possible_alternate_hz_targets); idx += 1)
    {
      F32 candidate = possible_alternate_hz_targets[idx];
      if(candidate <= target_hz)
      {
        U64 candidate_frame_time_us = 1000000/(U64)candidate;
        S64 frame_time_us_diff = (S64)frame_time_history_avg_us - (S64)candidate_frame_time_us;
        if(abs_s64(frame_time_us_diff) < best_target_hz_frame_time_us_diff &&
           frame_time_history_avg_us < candidate_frame_time_us + candidate_frame_time_us/4)
        {
          best_target_hz = candidate;
          best_target_hz_frame_time_us_diff = frame_time_us_diff;
        }
      }
    }
    target_hz = best_target_hz;
  }

  vtk_state->frame_dt = 1.f/target_hz;

  /////////////////////////////////
  //~ Fill animation rates

  vtk_state->animation.vast_rate = 1 - pow_f32(2, (-60.f * vtk_state->frame_dt));
  vtk_state->animation.fast_rate = 1 - pow_f32(2, (-50.f * vtk_state->frame_dt));
  vtk_state->animation.fish_rate = 1 - pow_f32(2, (-40.f * vtk_state->frame_dt));
  vtk_state->animation.slow_rate = 1 - pow_f32(2, (-30.f * vtk_state->frame_dt));
  vtk_state->animation.slug_rate = 1 - pow_f32(2, (-15.f * vtk_state->frame_dt));
  vtk_state->animation.slaf_rate = 1 - pow_f32(2, (-8.f  * vtk_state->frame_dt));

  // begin measuring actual per-frame work
  U64 begin_time_us = os_now_microseconds();

  /////////////////////////////////
  //~ Build UI

  ////////////////////////////////
  //- build event list for UI

  UI_EventList ui_events = {0};
  for(OS_Event *os_evt = os_events.first; os_evt != 0; os_evt = os_evt->next)
  {
    UI_Event ui_evt = {0};

    UI_EventKind kind = UI_EventKind_Null;
    switch(os_evt->kind)
    {
      default:{}break;
      case OS_EventKind_Press:     {kind = UI_EventKind_Press;}break;
      case OS_EventKind_Release:   {kind = UI_EventKind_Release;}break;
      case OS_EventKind_MouseMove: {kind = UI_EventKind_MouseMove;}break;
      case OS_EventKind_Text:      {kind = UI_EventKind_Text;}break;
      case OS_EventKind_Scroll:    {kind = UI_EventKind_Scroll;}break;
      case OS_EventKind_FileDrop:  {kind = UI_EventKind_FileDrop;}break;
    }

    ui_evt.kind         = kind;
    ui_evt.key          = os_evt->key;
    ui_evt.modifiers    = os_evt->modifiers;
    ui_evt.string       = os_evt->character ? str8_from_32(ui_build_arena(), str32(&os_evt->character, 1)) : str8_zero();
    ui_evt.paths        = str8_list_copy(ui_build_arena(), &os_evt->strings);
    ui_evt.pos          = os_evt->pos;
    ui_evt.delta_2f32   = os_evt->delta;
    ui_evt.timestamp_us = os_evt->timestamp_us;

    if(ui_evt.key == OS_Key_Backspace && !(ui_evt.modifiers&OS_Modifier_Ctrl) && ui_evt.kind == UI_EventKind_Press)
    {
      ui_evt.kind       = UI_EventKind_Edit;
      ui_evt.flags      = UI_EventFlag_Delete | UI_EventFlag_KeepMark;
      ui_evt.delta_unit = UI_EventDeltaUnit_Char;
      ui_evt.delta_2s32 = v2s32(-1,0);
    }

    if(ui_evt.key == OS_Key_Backspace && (ui_evt.modifiers&OS_Modifier_Ctrl) && ui_evt.kind == UI_EventKind_Press)
    {
      ui_evt.kind       = UI_EventKind_Edit;
      ui_evt.flags      = UI_EventFlag_Delete | UI_EventFlag_KeepMark;
      ui_evt.delta_unit = UI_EventDeltaUnit_Word;
      ui_evt.delta_2s32 = v2s32(-1,0);
    }

    if(ui_evt.kind == UI_EventKind_Text)
    {
      ui_evt.flags = UI_EventFlag_KeepMark;
    }

    if(ui_evt.key == OS_Key_Return && ui_evt.kind == UI_EventKind_Press)
    {
      ui_evt.slot = UI_EventActionSlot_Accept;
    }

    if(ui_evt.key == OS_Key_A && (ui_evt.modifiers & OS_Modifier_Ctrl) && ui_evt.kind == UI_EventKind_Press)
    {
      ui_evt.kind       = UI_EventKind_Navigate;
      ui_evt.flags      = UI_EventFlag_KeepMark;
      ui_evt.delta_unit = UI_EventDeltaUnit_Whole;
      ui_evt.delta_2s32 = v2s32(1,0);

      UI_Event ui_evt_0 = ui_evt;
      ui_evt_0.flags      = 0;
      ui_evt_0.delta_unit = UI_EventDeltaUnit_Whole;
      ui_evt_0.delta_2s32 = v2s32(-1,0);
      ui_event_list_push(ui_build_arena(), &ui_events, &ui_evt_0);
    }

    if(ui_evt.key == OS_Key_X && (ui_evt.modifiers & OS_Modifier_Ctrl) && ui_evt.kind == UI_EventKind_Press)
    {
      ui_evt.kind  = UI_EventKind_Edit;
      ui_evt.flags = UI_EventFlag_Delete | UI_EventFlag_Copy | UI_EventFlag_KeepMark;
    }

    if(ui_evt.key == OS_Key_C && (ui_evt.modifiers & OS_Modifier_Ctrl) && ui_evt.kind == UI_EventKind_Press)
    {
      ui_evt.kind       = UI_EventKind_Edit;
      ui_evt.flags      = UI_EventFlag_Copy | UI_EventFlag_KeepMark;
      ui_evt.delta_unit = UI_EventDeltaUnit_Char;
      ui_evt.delta_2s32 = v2s32(0,0);
    }

    if(ui_evt.key == OS_Key_V && (ui_evt.modifiers & OS_Modifier_Ctrl) && ui_evt.kind == UI_EventKind_Press)
    {
      ui_evt.kind   = UI_EventKind_Edit;
      ui_evt.flags  = UI_EventFlag_Paste;
      // ui_evt.string = os_get_clipboard_text(ui_build_arena());
    }

    if(ui_evt.key == OS_Key_Left && ui_evt.kind == UI_EventKind_Press)
    {
      ui_evt.kind       = UI_EventKind_Navigate;
      ui_evt.flags      = 0;
      ui_evt.delta_unit = ui_evt.modifiers & OS_Modifier_Ctrl ? UI_EventDeltaUnit_Word : UI_EventDeltaUnit_Char;
      ui_evt.delta_2s32 = v2s32(-1,0);
    }

    if(ui_evt.key == OS_Key_Right && ui_evt.kind == UI_EventKind_Press)
    {
      ui_evt.kind       = UI_EventKind_Navigate;
      ui_evt.flags      = 0;
      ui_evt.delta_unit = ui_evt.modifiers & OS_Modifier_Ctrl ? UI_EventDeltaUnit_Word : UI_EventDeltaUnit_Char;
      ui_evt.delta_2s32 = v2s32(1,0);
    }

    // k
    if(ui_evt.key == OS_Key_Up && ui_evt.kind == UI_EventKind_Press)
    {
      ui_evt.kind       = UI_EventKind_Navigate;
      ui_evt.flags      = 0;
      ui_evt.delta_unit = UI_EventDeltaUnit_Line;
      ui_evt.delta_2s32 = v2s32(0,-1);
    }

    // k
    if(ui_evt.key == OS_Key_Down && ui_evt.kind == UI_EventKind_Press)
    {
      ui_evt.kind       = UI_EventKind_Navigate;
      ui_evt.flags      = 0;
      ui_evt.delta_unit = UI_EventDeltaUnit_Line;
      ui_evt.delta_2s32 = v2s32(0,1);
    }

    UI_EventNode *evt_node = ui_event_list_push(ui_build_arena(), &ui_events, &ui_evt);

    if(os_evt->kind == OS_EventKind_WindowClose) {vtk_state->window_should_close = 1;}
  }

  ////////////////////////////////
  //- begin build UI

  {
    // gather font info
    FNT_Tag main_font = vtk_font_from_slot(VTK_FontSlot_Main);
    F32 main_font_size = vtk_font_size_from_slot(VTK_FontSlot_Main);
    FNT_Tag icon_font = vtk_font_from_slot(VTK_FontSlot_Icons);

    // build icon info
    UI_IconInfo icon_info = {0};
    {
      icon_info.icon_font = icon_font;
      icon_info.icon_kind_text_map[UI_IconKind_RightArrow]  = vtk_icon_kind_text_table[VTK_IconKind_RightScroll];
      icon_info.icon_kind_text_map[UI_IconKind_DownArrow]   = vtk_icon_kind_text_table[VTK_IconKind_DownScroll];
      icon_info.icon_kind_text_map[UI_IconKind_LeftArrow]   = vtk_icon_kind_text_table[VTK_IconKind_LeftScroll];
      icon_info.icon_kind_text_map[UI_IconKind_UpArrow]     = vtk_icon_kind_text_table[VTK_IconKind_UpScroll];
      icon_info.icon_kind_text_map[UI_IconKind_RightCaret]  = vtk_icon_kind_text_table[VTK_IconKind_RightCaret];
      icon_info.icon_kind_text_map[UI_IconKind_DownCaret]   = vtk_icon_kind_text_table[VTK_IconKind_DownCaret];
      icon_info.icon_kind_text_map[UI_IconKind_LeftCaret]   = vtk_icon_kind_text_table[VTK_IconKind_LeftCaret];
      icon_info.icon_kind_text_map[UI_IconKind_UpCaret]     = vtk_icon_kind_text_table[VTK_IconKind_UpCaret];
      icon_info.icon_kind_text_map[UI_IconKind_CheckHollow] = vtk_icon_kind_text_table[VTK_IconKind_CheckHollow];
      icon_info.icon_kind_text_map[UI_IconKind_CheckFilled] = vtk_icon_kind_text_table[VTK_IconKind_CheckFilled];
    }

    UI_WidgetPaletteInfo widget_palette_info = {0};
    {
      widget_palette_info.tooltip_palette   = vtk_ui_palette_from_code(VTK_PaletteCode_Floating);
      widget_palette_info.ctx_menu_palette  = vtk_ui_palette_from_code(VTK_PaletteCode_Floating);
      widget_palette_info.scrollbar_palette = vtk_ui_palette_from_code(VTK_PaletteCode_ScrollBarButton);
    }

    // build animation info
    UI_AnimationInfo animation_info = {0};
    {
      animation_info.hot_animation_rate     = vtk_state->animation.fast_rate;
      animation_info.active_animation_rate  = vtk_state->animation.fast_rate;
      animation_info.focus_animation_rate   = 1.f;
      animation_info.tooltip_animation_rate = vtk_state->animation.fast_rate;
      animation_info.menu_animation_rate    = vtk_state->animation.fast_rate;
      animation_info.scroll_animation_rate  = vtk_state->animation.fast_rate;
    }

    // begin & push initial stack values
    ui_begin_build(vtk_state->os_wnd, &ui_events, &icon_info, &widget_palette_info, &animation_info, vtk_state->frame_dt);

    ui_push_font(main_font);
    ui_push_font_size(main_font_size*0.85);
    ui_push_text_raster_flags(FNT_RasterFlag_Smooth);
    ui_push_text_padding(floor_f32(ui_top_font_size()*0.3f));
    ui_push_pref_width(ui_px(floor_f32(ui_top_font_size()*20.f), 1.f));
    ui_push_pref_height(ui_px(floor_f32(ui_top_font_size()*1.65f), 1.f));
    ui_push_palette(vtk_ui_palette_from_code(VTK_PaletteCode_Base));
  }

  ui_set_next_rect(vtk_state->window_rect);
  UI_Box *game_overlay = ui_build_box_from_string(UI_BoxFlag_MouseClickable|UI_BoxFlag_ClickToFocus|UI_BoxFlag_Scroll|UI_BoxFlag_DisableFocusOverlay, str8_lit("###game_overlay"));
  // NOTE(k): ui should happend before this
  vtk_state->sig = ui_signal_from_box(game_overlay);

  // FIXME: testing
  vtk_update();

  /////////////////////////////////
  //~ Cook UI drawing bucket

  ui_end_build();
  DR_BucketScope(vtk_state->bucket_ui)
  {
    vtk_ui_draw();
  }

  /////////////////////////////////
  //~ Submit work

  vtk_state->pre_cpu_time_us = os_now_microseconds()-begin_time_us;

  ProfScope("submit")
  // if(os_window_is_focused(vtk_state->os_wnd))
  {
    r_begin_frame();
    r_window_begin_frame(vtk_state->os_wnd, vtk_state->r_wnd);
    dr_submit_bucket(vtk_state->os_wnd, vtk_state->r_wnd, vtk_state->bucket_ui);
    dr_submit_bucket(vtk_state->os_wnd, vtk_state->r_wnd, vtk_state->bucket_main);
    vtk_state->pixel_hot_key = vtk_key_from_2f32(r_window_end_frame(vtk_state->os_wnd, vtk_state->r_wnd, vtk_state->mouse));
    r_end_frame();
  }

  /////////////////////////////////
  //~ Wait if we still have some cpu time left

  U64 frame_time_target_cap_us = (U64)(1000000/target_hz);
  U64 woik_us = os_now_microseconds()-begin_time_us;
  vtk_state->cpu_time_us = woik_us;
  if(woik_us < frame_time_target_cap_us)
  {
    ProfScope("wait frame target cap")
    {
      while(woik_us < frame_time_target_cap_us)
      {
        // TODO: check if os supports ms granular sleep
        if(1)
        {
          os_sleep_milliseconds((frame_time_target_cap_us-woik_us)/1000);
        }
        woik_us = os_now_microseconds()-begin_time_us;
      }
    }
  }
  else
  {
    // Missed frame rate!
    // TODO(k): proper logging
    fprintf(stderr, "missed frame, over %06.2f ms from %06.2f ms\n", (woik_us-frame_time_target_cap_us)/1000.0, frame_time_target_cap_us/1000.0);

    // tag it
    ProfBegin("MISSED FRAME");
    ProfEnd();
  }

  /////////////////////////////////
  //~ Determine frame time, record it into history

  U64 end_time_us = os_now_microseconds();
  U64 frame_time_us = end_time_us-begin_time_us;
  vtk_state->frame_time_us_history[vtk_state->frame_index%ArrayCount(vtk_state->frame_time_us_history)] = frame_time_us;

  /////////////////////////////////
  //~ Bump frame time counters

  vtk_state->frame_index++;
  vtk_state->time_in_seconds += vtk_state->frame_dt;
  vtk_state->time_in_us += frame_time_us;

  ProfEnd();
  return !vtk_state->window_should_close;
}

/////////////////////////////////
//~ State accessor/mutator

//- frame

internal inline Arena *
vtk_frame_arena(void)
{
  return vtk_state->frame_arenas[vtk_state->frame_index % ArrayCount(vtk_state->frame_arenas)];
}

internal inline VTK_DrawList *
vtk_frame_drawlist(void)
{
  return vtk_state->drawlists[vtk_state->frame_index % ArrayCount(vtk_state->drawlists)];
}

//- color
internal Vec4F32
vtk_rgba_from_theme_color(VTK_ThemeColor color)
{
  return vtk_state->cfg_theme.colors[color];
}

//- code -> palette
internal VTK_Palette *
vtk_palette_from_code(VTK_PaletteCode code)
{
  VTK_Palette *ret = &vtk_state->cfg_main_palettes[code];
  return ret;
}

//- code -> ui palette
internal UI_Palette *
vtk_ui_palette_from_code(VTK_PaletteCode code)
{
  UI_Palette *ret = &vtk_state->cfg_ui_debug_palettes[code];
  return ret;
}

//- fonts/size
internal FNT_Tag
vtk_font_from_slot(VTK_FontSlot slot)
{
  return vtk_state->cfg_font_tags[slot];
}

internal F32
vtk_font_size_from_slot(VTK_FontSlot slot)
{
  F32 result = 0;
  F32 dpi = vtk_state->dpi;
  if(dpi != vtk_state->last_dpi)
  {
    F32 old_dpi = vtk_state->last_dpi;
    F32 new_dpi = dpi;
    vtk_state->last_dpi = dpi;
    S32 *pt_sizes[] =
    {
      &vtk_state->setting_vals[VTK_SettingCode_MainFontSize].s32,
      &vtk_state->setting_vals[VTK_SettingCode_CodeFontSize].s32,
    };
    for(U64 idx = 0; idx < ArrayCount(pt_sizes); idx++)
    {
      F32 ratio = pt_sizes[idx][0] / old_dpi;
      F32 new_pt_size = ratio*new_dpi;
      pt_sizes[idx][0] = (S32)new_pt_size;
    }
  }

  switch(slot)
  {
    case VTK_FontSlot_Code:
    {
      result = (F32)vtk_state->setting_vals[VTK_SettingCode_CodeFontSize].s32;
    }break;
    default:
    case VTK_FontSlot_Main:
    case VTK_FontSlot_Icons:
    {
      result = (F32)vtk_state->setting_vals[VTK_SettingCode_MainFontSize].s32;
    }break;
  }
  return result;
}

/////////////////////////////////
// Dynamic Draw List

internal VTK_DrawList *
vtk_drawlist_alloc(Arena *arena, U64 vertex_buffer_cap, U64 indice_buffer_cap)
{
  ProfBeginFunction();
  VTK_DrawList *ret = push_array(arena, VTK_DrawList, 1);
  ret->vertex_buffer_cap = vertex_buffer_cap;
  ret->indice_buffer_cap = indice_buffer_cap;
  ret->vertices = r_buffer_alloc(R_ResourceKind_Stream, vertex_buffer_cap, 0, 0);
  ret->indices = r_buffer_alloc(R_ResourceKind_Stream, indice_buffer_cap, 0, 0);
  ProfEnd();
  return ret;
}

internal VTK_DrawNode *
vtk_drawlist_push(Arena *arena, VTK_DrawList *drawlist, R_Geo3D_Vertex *vertices_src, U64 vertex_count, U32 *indices_src, U64 indice_count)
{ 
  VTK_DrawNode *ret = push_array(arena, VTK_DrawNode, 1);
  U64 v_buf_size = sizeof(R_Geo3D_Vertex) * vertex_count;
  U64 i_buf_size = sizeof(U32) * indice_count;

  // TODO(XXX): we should use buffer block, we can't just release/realloc a new buffer, since the buffer handle is already handled over to VTK_DrawNode
  AssertAlways(v_buf_size + drawlist->vertex_buffer_cmt <= drawlist->vertex_buffer_cap);
  AssertAlways(i_buf_size + drawlist->indice_buffer_cmt <= drawlist->indice_buffer_cap);

  U64 v_buf_offset = drawlist->vertex_buffer_cmt;
  U64 i_buf_offset = drawlist->indice_buffer_cmt;

  drawlist->vertex_buffer_cmt += v_buf_size;
  drawlist->indice_buffer_cmt += i_buf_size;

  // fill info
  ret->vertices_src = vertices_src;
  ret->vertex_count = vertex_count;
  ret->vertices = drawlist->vertices;
  ret->vertices_buffer_offset = v_buf_offset;
  ret->indices_src = indices_src;
  ret->indice_count = indice_count;
  ret->indices = drawlist->indices;
  ret->indices_buffer_offset = i_buf_offset;

  SLLQueuePush(drawlist->first_node, drawlist->last_node, ret);
  drawlist->node_count++;
  return ret;
}

internal void
vtk_drawlist_reset(VTK_DrawList *drawlist)
{
  // clear per-frame data
  drawlist->first_node = 0;
  drawlist->last_node = 0;
  drawlist->node_count = 0;
  drawlist->vertex_buffer_cmt = 0;
  drawlist->indice_buffer_cmt = 0;
}

internal void
vtk_drawlist_build(VTK_DrawList *drawlist)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);
  R_Geo3D_Vertex *vertices = (R_Geo3D_Vertex*)push_array(scratch.arena, U8, drawlist->vertex_buffer_cmt);
  U32 *indices = (U32*)push_array(scratch.arena, U8, drawlist->indice_buffer_cmt);

  // collect buffer
  R_Geo3D_Vertex *vertices_dst = vertices;
  U32 *indices_dst = indices;
  for(VTK_DrawNode *n = drawlist->first_node; n != 0; n = n->next)  
  {
    MemoryCopy(vertices_dst, n->vertices_src, sizeof(R_Geo3D_Vertex)*n->vertex_count);
    MemoryCopy(indices_dst, n->indices_src, sizeof(U32)*n->indice_count);

    vertices_dst += n->vertex_count; 
    indices_dst += n->indice_count;
  }

  r_buffer_copy(drawlist->vertices, vertices, drawlist->vertex_buffer_cmt);
  r_buffer_copy(drawlist->indices, indices, drawlist->indice_buffer_cmt);
  scratch_end(scratch);
  ProfEnd();
}

/////////////////////////////////
//~ Loader Functions

internal VTK_Mesh *
vtk_mesh_from_vtk(Arena *arena, String8 path)
{
  Temp scratch = scratch_begin(&arena,1);

  // read file data
  OS_Handle file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_ShareRead, path);
  FileProperties props = os_properties_from_file(file);
  String8 data = os_string_from_file_range(scratch.arena, file, r1u64(0, props.size));
  os_file_close(file);

  // parse
  String8 header = {0};
  String8 version = {0};
  B32 ascii = 0;
  B32 binary = 0;
  VTK_MeshKind mesh_kind = VTK_MeshKind_Invalid;
  void *mesh_data = 0;

  U8 *first = data.str;
  U8 *opl = data.str + data.size;
  U8 *c = first;

  // part-1: version and identifier
  {
    U8 *start = c;
    for(; c < opl && *c != '\n'; c++) {}
    U8 *end = c++;
    version = str8_range(start, end);
  }

  // part-2: header
  {
    U8 *start = c;
    for(; c < opl && *c != '\n'; c++) {}
    U8 *end = c++;
    header = str8_range(start, end);
  }

  // part-3: file format
  {
    U8 *start = c;
    for(; c < opl && *c != '\n'; c++) {}
    U8 *end = c++;
    String8 format = str8_range(start, end);
    if(str8_match(format, str8_lit("ASCII"), 0))  ascii  = 1;
    if(str8_match(format, str8_lit("BINARY"), 0)) binary = 1;
  }

  // part-4(optional): dataset structure
  VTK_Point *points = 0;
  U64 point_count = 0;
  VTK_Cell *cells = 0;
  int cell_count = 0;
  {
    U8 *start = c;
    for(; c < opl && *c != '\n'; c++) {}
    U8 *end = c++;
    String8 line = str8_range(start, end);

    if(str8_starts_with(line, str8_lit("DATASET"), 0))
    {
      String8 data_type = str8_skip(line, 8);
      if(str8_match(data_type, str8_lit("POLYDATA"), 0))
      {
        mesh_kind = VTK_MeshKind_PolyData;
      }
      else {NotImplemented;}
    }
    else
    {
      c = start;
    }

    switch(mesh_kind)
    {
      case VTK_MeshKind_Invalid:{}break;
      case VTK_MeshKind_PolyData:
      {
        while(c < opl)
        {
          U8 *start = c;
          for(; c < opl && *c != '\n'; c++);
          U8 *end = c++;
          String8 line = str8_range(start, end);

          if(str8_starts_with(line, str8_lit("POINTS"), 0))
          {
            int n = 0;
            char datatype[20] = {0};
            int nread = sscanf((char*)line.str, "%*s %d %s", &n, datatype);
            point_count = n;

            if(point_count > 0)
            {
              points = push_array(arena, VTK_Point, point_count);
            }

            if(ascii)
            {
              for(U64 i = 0; i < point_count; i++)
              {
                U8 *start = c;
                for(; c < opl && *c != '\n'; c++);
                U8 *end = c++;
                String8 line = str8_range(start, end);
                VTK_Point *point = &points[i];
                int nread = sscanf((char*)line.str, "%f %f %f", &point->pos.x, &point->pos.y, &point->pos.z);
              }
            }

            if(binary)
            {
              U64 element_bytes = 0;
              B32 is_float = 0;
              B32 is_double = 0;
              if(str8_match(str8_cstring(datatype), str8_lit("float"), 0))  {element_bytes = sizeof(float); is_float = 1;}
              if(str8_match(str8_cstring(datatype), str8_lit("double"), 0)) {element_bytes = sizeof(double); is_double = 1;}
              U8 *bytes = c;
              U64 bytes_to_read = element_bytes * 3 * point_count;
              // binary data must immediately follow the newline character (\n) from the previous ASCII keyword and parameter sequence.
              c += (bytes_to_read+1); // skip one more \n
              AssertAlways(is_float || is_double); // FIXME: ...
              for(U64 point_idx = 0; point_idx < point_count; point_idx++)
              {
                VTK_Point *point = &points[point_idx];
                U8 *head = bytes + point_idx*element_bytes*3;

                if(is_float)
                {
                  float *f = (float*)head;
                  point->pos.x = f[0];
                  point->pos.y = f[1];
                  point->pos.z = f[2];
                }
                else if(is_double)
                {
                  double *d = (double*)head;
                  // FIXME: why, for the love of god, we need to swap???
                  point->pos.x = bswap_f64(d[0]);
                  point->pos.y = bswap_f64(d[1]);
                  point->pos.z = bswap_f64(d[2]);
                }
              }
            }
          }

          if(str8_starts_with(line, str8_lit("POLYGONS"), 0))
          {
            // The cell list size is the total number of integer values required to represent the list
            int cell_list_size = 0;
            int nread = sscanf((char*)line.str, "%*s %d %d", &cell_count, &cell_list_size);

            if(cell_count > 0)
            {
              cells = push_array(arena, VTK_Cell, cell_count);
            }

            if(ascii)
            {
              for(U64 i = 0; i < cell_count; i++)
              {
                VTK_Cell *cell = &cells[i];

                // read numPoints
                U8 *mark = c;
                for(; !char_is_space(*c); c++);
                U64 point_count = u64_from_str8(str8_range(mark, c++), 10);

                // alloc indices
                cell->point_indices = push_array(arena, U32, point_count);
                cell->point_count = point_count;

                // read point indices
                for(U64 point_idx = 0; point_idx < point_count; point_idx++)
                {
                  mark = c;
                  for(; !char_is_space(*c); c++);
                  String8 idx_str = str8_range(mark, c++);
                  U64 point_indice = u64_from_str8(idx_str, 10);
                  cell->point_indices[point_idx] = point_indice;
                }
              }
            }
            else if(binary)
            {
              // read offsets
              U64 *offsets = push_array(scratch.arena, U64, cell_count);
              {
                U8 *mark = c;
                for(; *c != '\n'; c++);
                String8 line = str8_range(mark, c++);
                if(str8_starts_with(line, str8_lit("OFFSETS"), 0))
                {
                  char datatype[20];
                  sscanf((char*)line.str, "%*s %s", datatype);
                  B32 is_i32 = 0;
                  B32 is_i64 = 0;
                  if(str8_match(str8_cstring(datatype), str8_lit("vtktypeint64"), 0)) is_i64 = 1;
                  if(str8_match(str8_cstring(datatype), str8_lit("vtktypeint32"), 0)) is_i32 = 1;
                  AssertAlways(is_i32 || is_i64); // FIXME: ...
                  U64 element_bytes = is_i32 ? sizeof(S32) : sizeof(S64);
                  U64 offsets_byte_size = element_bytes * cell_count;
                  U8 *offsets_bytes = c;
                  c += (offsets_byte_size+1);
                  for(U64 cell_idx = 0; cell_idx < cell_count; cell_idx++)
                  {
                    // FIXME: not handling u32, test for now
                    U64 test = ((U64*)offsets_bytes)[cell_idx];
                    test = bswap_u64(test);
                    offsets[cell_idx] = test;
                    // MemoryCopy(&offsets[cell_idx], &offsets_bytes[cell_idx*element_bytes], element_bytes);
                  }
                }
              }

              // read indices
              {
                U8 *mark = c;
                for(; *c != '\n'; c++);
                String8 line = str8_range(mark, c++);
                if(str8_starts_with(line, str8_lit("CONNECTIVITY"), 0))
                {
                  char datatype[20];
                  sscanf((char*)line.str, "%*s %s", datatype);
                  B32 is_i32 = 0;
                  B32 is_i64 = 0;
                  if(str8_match(str8_cstring(datatype), str8_lit("vtktypeint64"), 0)) is_i64 = 1;
                  if(str8_match(str8_cstring(datatype), str8_lit("vtktypeint32"), 0)) is_i32 = 1;
                  AssertAlways(is_i32 || is_i64); // FIXME: ...
                  U64 element_bytes = is_i32 ? sizeof(S32) : sizeof(S64);
                  U64 indice_read = 0;
                  U8 *head = c;
                  for(U64 cell_idx = 0; cell_idx < cell_count; cell_idx++)
                  {
                    // U64 offset = offsets[cell_idx];
                    // FIXME: the offsets[0] is 0 which is weird, assume 3 for now
                    VTK_Cell *cell = &cells[cell_idx];
                    cell->point_indices = push_array(arena, U32, 3);
                    cell->point_count = 3;
                    for(U64 i = 0; i < 3; i++)
                    {
                      // FIXME: don't care about s32 for now, make it work most
                      U64 indice = *((U64*)head);
                      indice = bswap_u64(indice);
                      cell->point_indices[i] = indice;
                      head += sizeof(S64);
                      indice_read++;
                    }
                  }
                  c = head;
                  c++; // skip \n again after the bytes
                }
              }
            }
          }
        }
      }break;
      default:{InvalidPath;}break;
    }
  }

  // part-5(optional): dataset attributes
  {
    // FIXME: ...
  }

  // fill&return
  VTK_Mesh *ret = push_array(arena, VTK_Mesh, 1);
  ret->kind = mesh_kind;
  ret->points = points;
  ret->point_count = point_count;
  ret->cells = cells;
  ret->cell_count = cell_count;
  scratch_end(scratch);
  return ret;
}

internal VTK_Mesh *
vtk_mesh_from_ply(Arena *arena, String8 path)
{
  NotImplemented;
}

/////////////////////////////////
//~ Drawing

internal void
vtk_ui_draw()
{
  Temp scratch = scratch_begin(0,0);
  F32 box_squish_epsilon = 0.001f;
  dr_push_viewport(vtk_state->window_dim);

  // unpack some settings
  F32 border_softness = 1.f;
  F32 rounded_corner_amount = 1.0;

  // DEBUG mouse
  if(0)
  {
    R_Rect2DInst *cursor = dr_rect(r2f32p(ui_state->mouse.x-15,ui_state->mouse.y-15, ui_state->mouse.x+15,ui_state->mouse.y+15), v4f32(0,0.3,1,0.3), 15, 0.0, 0.7);
  }

  // Recusivly drawing boxes
  UI_Box *box = ui_root_from_state(ui_state);
  while(!ui_box_is_nil(box))
  {
    UI_BoxRec rec = ui_box_rec_df_post(box, &ui_nil_box);

    // rjf: get corner radii
    F32 box_corner_radii[Corner_COUNT] =
    {
      box->corner_radii[Corner_00]*rounded_corner_amount,
      box->corner_radii[Corner_01]*rounded_corner_amount,
      box->corner_radii[Corner_10]*rounded_corner_amount,
      box->corner_radii[Corner_11]*rounded_corner_amount,
    };

    // push transparency
    if(box->transparency != 0)
    {
      dr_push_transparency(box->transparency);
    }

    // push squish
    if(box->squish > box_squish_epsilon)
    {
      Vec2F32 box_dim = dim_2f32(box->rect);
      Vec2F32 anchor_off = {0};
      if(box->flags & UI_BoxFlag_SquishAnchored)
      {
        anchor_off.x = box_dim.x/2.0;
      }
      else
      {
        anchor_off.y = -box_dim.y/8.0;
      }
      // NOTE(k): this is not from center
      Vec2F32 origin = {box->rect.x0 + box_dim.x/2 - anchor_off.x, box->rect.y0 - anchor_off.y};

      Mat3x3F32 box2origin_xform = make_translate_3x3f32(v2f32(-origin.x, -origin.y));
      Mat3x3F32 scale_xform = make_scale_3x3f32(v2f32(1-box->squish, 1-box->squish));
      Mat3x3F32 origin2box_xform = make_translate_3x3f32(origin);
      Mat3x3F32 xform = mul_3x3f32(origin2box_xform, mul_3x3f32(scale_xform, box2origin_xform));
      // TODO(k): why we may need to tranpose this, should it already column major? check it later
      xform = transpose_3x3f32(xform);
      dr_push_xform2d(xform);
      dr_push_tex2d_sample_kind(R_Tex2DSampleKind_Linear);
    }

    // draw drop_shadw
    if(box->flags & UI_BoxFlag_DrawDropShadow)
    {
      Rng2F32 drop_shadow_rect = shift_2f32(pad_2f32(box->rect, 8), v2f32(4, 4));
      Vec4F32 drop_shadow_color = vtk_rgba_from_theme_color(VTK_ThemeColor_DropShadow);
      dr_rect(drop_shadow_rect, drop_shadow_color, 0.8f, 0, 8.f);
    }

    // draw background
    if(box->flags & UI_BoxFlag_DrawBackground)
    {
      // main rectangle
      R_Rect2DInst *inst = dr_rect(pad_2f32(box->rect, 1), box->palette->colors[UI_ColorCode_Background], 0, 0, 1.f);
      MemoryCopyArray(inst->corner_radii, box->corner_radii);

      if(box->flags & UI_BoxFlag_DrawHotEffects)
      {
        B32 is_hot = !ui_key_match(box->key, ui_key_zero()) && ui_key_match(box->key, ui_hot_key());
        Vec4F32 hover_color = vtk_rgba_from_theme_color(VTK_ThemeColor_Hover);

        F32 effective_active_t = box->active_t;
        if(!(box->flags & UI_BoxFlag_DrawActiveEffects))
        {
          effective_active_t = 0;
        }
        F32 t = box->hot_t * (1-effective_active_t);

        // brighten
        {
          Vec4F32 color = hover_color;
          color.w *= 0.15f;
          if(!is_hot)
          {
            color.w *= t;
          }
          R_Rect2DInst *inst = dr_rect(pad_2f32(box->rect, 1.f), v4f32(0, 0, 0, 0), 0, 0, border_softness*1.f);
          inst->colors[Corner_00] = color;
          inst->colors[Corner_10] = color;
          inst->colors[Corner_01] = color;
          inst->colors[Corner_11] = color;
          MemoryCopyArray(inst->corner_radii, box_corner_radii);
        }

        // rjf: soft circle around mouse
        if(box->hot_t > 0.01f) DR_ClipScope(box->rect)
        {
          Vec4F32 color = hover_color;
          color.w *= 0.04f;
          if(!is_hot)
          {
            color.w *= t;
          }
          Vec2F32 center = ui_mouse();
          Vec2F32 box_dim = dim_2f32(box->rect);
          F32 max_dim = Max(box_dim.x, box_dim.y);
          F32 radius = box->font_size*12.f;
          radius = Min(max_dim, radius);
          dr_rect(pad_2f32(r2f32(center, center), radius), color, radius, 0, radius/3.f);
        }

        // rjf: slight emboss fadeoff
        if(0)
        {
          Rng2F32 rect = r2f32p(box->rect.x0, box->rect.y0, box->rect.x1, box->rect.y1);
          R_Rect2DInst *inst = dr_rect(rect, v4f32(0, 0, 0, 0), 0, 0, 1.f);
          inst->colors[Corner_00] = v4f32(0.f, 0.f, 0.f, 0.0f*t);
          inst->colors[Corner_01] = v4f32(0.f, 0.f, 0.f, 0.3f*t);
          inst->colors[Corner_10] = v4f32(0.f, 0.f, 0.f, 0.0f*t);
          inst->colors[Corner_11] = v4f32(0.f, 0.f, 0.f, 0.3f*t);
          MemoryCopyArray(inst->corner_radii, box->corner_radii);
        }

        // active effect extension
        if(box->flags & UI_BoxFlag_DrawActiveEffects)
        {
          Vec4F32 shadow_color = vtk_rgba_from_theme_color(VTK_ThemeColor_DropShadow);
          shadow_color.w *= 1.f*box->active_t;

          Vec2F32 shadow_size =
          {
            (box->rect.x1 - box->rect.x0)*0.60f*box->active_t,
            (box->rect.y1 - box->rect.y0)*0.60f*box->active_t,
          };
          shadow_size.x = Clamp(0, shadow_size.x, box->font_size*2.f);
          shadow_size.y = Clamp(0, shadow_size.y, box->font_size*2.f);

          // rjf: top -> bottom dark effect
          {
            R_Rect2DInst *inst = dr_rect(r2f32p(box->rect.x0, box->rect.y0, box->rect.x1, box->rect.y0 + shadow_size.y), v4f32(0, 0, 0, 0), 0, 0, 1.f);
            inst->colors[Corner_00] = inst->colors[Corner_10] = shadow_color;
            inst->colors[Corner_01] = inst->colors[Corner_11] = v4f32(0.f, 0.f, 0.f, 0.0f);
            MemoryCopyArray(inst->corner_radii, box_corner_radii);
          }
          
          // rjf: bottom -> top light effect
          {
            R_Rect2DInst *inst = dr_rect(r2f32p(box->rect.x0, box->rect.y1 - shadow_size.y, box->rect.x1, box->rect.y1), v4f32(0, 0, 0, 0), 0, 0, 1.f);
            inst->colors[Corner_00] = inst->colors[Corner_10] = v4f32(0, 0, 0, 0);
            inst->colors[Corner_01] = inst->colors[Corner_11] = v4f32(1.0f, 1.0f, 1.0f, 0.08f*box->active_t);
            MemoryCopyArray(inst->corner_radii, box_corner_radii);
          }
          
          // rjf: left -> right dark effect
          {
            R_Rect2DInst *inst = dr_rect(r2f32p(box->rect.x0, box->rect.y0, box->rect.x0 + shadow_size.x, box->rect.y1), v4f32(0, 0, 0, 0), 0, 0, 1.f);
            inst->colors[Corner_10] = inst->colors[Corner_11] = v4f32(0.f, 0.f, 0.f, 0.f);
            inst->colors[Corner_00] = shadow_color;
            inst->colors[Corner_01] = shadow_color;
            MemoryCopyArray(inst->corner_radii, box_corner_radii);
          }
          
          // rjf: right -> left dark effect
          {
            R_Rect2DInst *inst = dr_rect(r2f32p(box->rect.x1 - shadow_size.x, box->rect.y0, box->rect.x1, box->rect.y1), v4f32(0, 0, 0, 0), 0, 0, 1.f);
            inst->colors[Corner_00] = inst->colors[Corner_01] = v4f32(0.f, 0.f, 0.f, 0.f);
            inst->colors[Corner_10] = shadow_color;
            inst->colors[Corner_11] = shadow_color;
            MemoryCopyArray(inst->corner_radii, box_corner_radii);
          }
        }
      }
    }

    // draw image
    if(box->flags & UI_BoxFlag_DrawImage)
    {
      R_Rect2DInst *inst = dr_img(box->rect, box->src, box->albedo_tex, box->albedo_clr, 0., 0., 0.);
      inst->white_texture_override = box->albedo_white_texture_override ? 1.0 : 0.0;
    }

    // draw string
    if(box->flags & UI_BoxFlag_DrawText)
    {
      Vec2F32 text_position = ui_box_text_position(box);

      // max width
      F32 max_x = 100000.0f;
      FNT_Run ellipses_run = {0};

      if(!(box->flags & UI_BoxFlag_DisableTextTrunc))
      {
        max_x = (box->rect.x1-text_position.x);
        ellipses_run = fnt_run_from_string(box->font, box->font_size, 0, box->tab_size, 0, str8_lit("..."));
      }

      dr_truncated_fancy_run_list(text_position, &box->display_string_runs, max_x, ellipses_run);
    }

    // NOTE(k): draw focus viz
    if(1)
    {
      B32 focused = (box->flags & (UI_BoxFlag_FocusHot|UI_BoxFlag_FocusActive) &&
                     box->flags & UI_BoxFlag_Clickable);
      B32 disabled = 0;
      for(UI_Box *p = box; !ui_box_is_nil(p); p = p->parent)
      {
        if(p->flags & (UI_BoxFlag_FocusHotDisabled|UI_BoxFlag_FocusActiveDisabled))
        {
          disabled = 1;
          break;
        }
      }
      if(focused)
      {
        Vec4F32 color = v4f32(0.3f, 0.8f, 0.3f, 1.f);
        if(disabled)
        {
          color = v4f32(0.8f, 0.3f, 0.3f, 1.f);
        }
        dr_rect(r2f32p(box->rect.x0-6, box->rect.y0-6, box->rect.x0+6, box->rect.y0+6), color, 2, 0, 1);
        dr_rect(box->rect, color, 2, 2, 1);
      }
      if(box->flags & (UI_BoxFlag_FocusHot|UI_BoxFlag_FocusActive))
      {
        if(box->flags & (UI_BoxFlag_FocusHotDisabled|UI_BoxFlag_FocusActiveDisabled))
        {
          dr_rect(r2f32p(box->rect.x0-6, box->rect.y0-6, box->rect.x0+6, box->rect.y0+6), v4f32(1, 0, 0, 0.2f), 2, 0, 1);
        }
        else
        {
          dr_rect(r2f32p(box->rect.x0-6, box->rect.y0-6, box->rect.x0+6, box->rect.y0+6), v4f32(0, 1, 0, 0.2f), 2, 0, 1);
        }
      }
    }

    // push clip
    if(box->flags & UI_BoxFlag_Clip)
    {
      Rng2F32 top_clip = dr_top_clip();
      Rng2F32 new_clip = pad_2f32(box->rect, -1);
      if(top_clip.x1 != 0 || top_clip.y1 != 0)
      {
        new_clip = intersect_2f32(new_clip, top_clip);
      }
      dr_push_clip(new_clip);
    }

    // k: custom draw list
    if(box->flags & UI_BoxFlag_DrawBucket)
    {
      Mat3x3F32 xform = make_translate_3x3f32(box->position_delta);
      DR_XForm2DScope(xform)
      {
        dr_sub_bucket(box->draw_bucket);
      }
    }

    // call custom draw callback
    if(box->custom_draw != 0)
    {
      box->custom_draw(box, box->custom_draw_user_data);
    }

    // k: pop stacks
    {
      S32 pop_idx = 0;
      for(UI_Box *b = box; !ui_box_is_nil(b) && pop_idx <= rec.pop_count; b = b->parent)
      {
        pop_idx += 1;
        if(b == box && rec.push_count != 0) continue;

        // k: pop clip
        if(b->flags & UI_BoxFlag_Clip)
        {
          dr_pop_clip();
        }

        // rjf: draw overlay
        if(b->flags & UI_BoxFlag_DrawOverlay)
        {
          R_Rect2DInst *inst = dr_rect(b->rect, b->palette->colors[UI_ColorCode_Overlay], 0, 0, 1.f);
          MemoryCopyArray(inst->corner_radii, b->corner_radii);
        }

        //- k: draw the border
        if(b->flags & UI_BoxFlag_DrawBorder)
        {
          R_Rect2DInst *inst = dr_rect(pad_2f32(b->rect, 1.f), b->palette->colors[UI_ColorCode_Border], 0, 1.f, border_softness*1.f);
          MemoryCopyArray(inst->corner_radii, b->corner_radii);

          // rjf: hover effect
          if(b->flags & UI_BoxFlag_DrawHotEffects)
          {
            Vec4F32 color = vtk_rgba_from_theme_color(VTK_ThemeColor_Hover);
            color.w *= b->hot_t;
            R_Rect2DInst *inst = dr_rect(pad_2f32(b->rect, 1), color, 0, 1.f, 1.f);
            inst->colors[Corner_01].w *= 0.2f;
            inst->colors[Corner_11].w *= 0.2f;
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
          }
        }

        // k: debug border rendering
        if(0)
        {
          R_Rect2DInst *inst = dr_rect(pad_2f32(b->rect, 1), v4f32(1, 0, 1, 0.25f), 0, 1.f, 1.f);
          MemoryCopyArray(inst->corner_radii, b->corner_radii);
        }

        // rjf: draw sides
        {
          Rng2F32 r = b->rect;
          F32 half_thickness = 1.f;
          F32 softness = 0.5f;
          if(b->flags & UI_BoxFlag_DrawSideTop)
          {
            dr_rect(r2f32p(r.x0, r.y0-half_thickness, r.x1, r.y0+half_thickness), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
          }
          if(b->flags & UI_BoxFlag_DrawSideBottom)
          {
            dr_rect(r2f32p(r.x0, r.y1-half_thickness, r.x1, r.y1+half_thickness), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
          }
          if(b->flags & UI_BoxFlag_DrawSideLeft)
          {
            dr_rect(r2f32p(r.x0-half_thickness, r.y0, r.x0+half_thickness, r.y1), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
          }
          if(b->flags & UI_BoxFlag_DrawSideRight)
          {
            dr_rect(r2f32p(r.x1-half_thickness, r.y0, r.x1+half_thickness, r.y1), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
          }
        }

        // rjf: draw focus overlay
        if(b->flags & UI_BoxFlag_Clickable && !(b->flags & UI_BoxFlag_DisableFocusOverlay) && b->focus_hot_t > 0.01f)
        {
          Vec4F32 color = vtk_rgba_from_theme_color(VTK_ThemeColor_Focus);
          color.w *= 0.2f*b->focus_hot_t;
          R_Rect2DInst *inst = dr_rect(b->rect, color, 0, 0, 0.f);
          MemoryCopyArray(inst->corner_radii, b->corner_radii);
        }

        // rjf: draw focus border
        if(b->flags & UI_BoxFlag_Clickable && !(b->flags & UI_BoxFlag_DisableFocusBorder) && b->focus_active_t > 0.01f)
        {
          Vec4F32 color = vtk_rgba_from_theme_color(VTK_ThemeColor_Focus);
          color.w *= b->focus_active_t;
          R_Rect2DInst *inst = dr_rect(pad_2f32(b->rect, 0.f), color, 0, 1.f, 1.f);
          MemoryCopyArray(inst->corner_radii, b->corner_radii);
        }

        // rjf: disabled overlay
        if(b->disabled_t >= 0.005f)
        {
          Vec4F32 color = vtk_rgba_from_theme_color(VTK_ThemeColor_DisabledOverlay);
          color.w *= b->disabled_t;
          R_Rect2DInst *inst = dr_rect(b->rect, color, 0, 0, 1);
          MemoryCopyArray(inst->corner_radii, b->corner_radii);
        }

        // rjf: pop squish
        if(b->squish > box_squish_epsilon)
        {
          dr_pop_xform2d();
          dr_pop_tex2d_sample_kind();
        }

        // k: pop transparency
        if(b->transparency != 0)
        {
          dr_pop_transparency();
        }
      }
    }
    box = rec.next;
  }
  dr_pop_viewport();
  scratch_end(scratch);
}
