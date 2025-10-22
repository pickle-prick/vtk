#ifndef VTK_CORE_H
#define VTK_CORE_H

////////////////////////////////
//~ Key

typedef struct VTK_Key VTK_Key;
struct VTK_Key
{
  U64 u64[2];
};

////////////////////////////////
//~ Setting Types

typedef struct VTK_SettingVal VTK_SettingVal;
struct VTK_SettingVal
{
  B32 set;
  S32 s32;
};

////////////////////////////////
//~ Font slot

typedef enum VTK_FontSlot
{
  VTK_FontSlot_Main,
  VTK_FontSlot_Code,
  VTK_FontSlot_Icons,
  VTK_FontSlot_IconsExtra,
  VTK_FontSlot_HandWrite,
  VTK_FontSlot_COUNT
} VTK_FontSlot;

////////////////////////////////
//~ Palettes

typedef enum VTK_ColorCode
{
  VTK_ColorCode_Null,
  VTK_ColorCode_Background,
  VTK_ColorCode_Text,
  VTK_ColorCode_TextWeak,
  VTK_ColorCode_Border,
  VTK_ColorCode_Overlay,
  VTK_ColorCode_Cursor,
  VTK_ColorCode_Selection,
  VTK_ColorCode_COUNT
} VTK_ColorCode;

typedef struct VTK_Palette VTK_Palette;
struct VTK_Palette
{
  union
  {
    Vec4F32 colors[VTK_ColorCode_COUNT];
    struct
    {
      Vec4F32 null;
      Vec4F32 background;
      Vec4F32 text;
      Vec4F32 text_weak;
      Vec4F32 border;
      Vec4F32 overlay;
      Vec4F32 cursor;
      Vec4F32 selection;
    };
  };
};

typedef struct VTK_WidgetPaletteInfo VTK_WidgetPaletteInfo;
struct VTK_WidgetPaletteInfo
{
  VTK_Palette *tooltip_palette;
  VTK_Palette *ctx_menu_palette;
  VTK_Palette *scrollbar_palette;
};

/////////////////////////////////
//~ Dynamic drawing (in immediate mode fashion)

typedef struct VTK_DrawNode VTK_DrawNode;
struct VTK_DrawNode
{
  VTK_DrawNode *next;

  R_GeoPolygonKind polygon;
  R_GeoTopologyKind topology;

  // vertex
  // src
  R_Geo3D_Vertex *vertices_src;
  U64 vertex_count;
  // dst
  R_Handle vertices;
  U64 vertices_buffer_offset;

  // indice
  // src
  U32 *indices_src;
  U64 indice_count;
  // dst
  R_Handle indices;
  U64 indices_buffer_offset;

  // for font altas
  R_Handle albedo_tex;
};

typedef struct VTK_DrawList VTK_DrawList;
struct VTK_DrawList
{
  // per-build
  VTK_DrawNode *first_node;
  VTK_DrawNode *last_node;
  U64 node_count;
  U64 vertex_buffer_cmt;
  U64 indice_buffer_cmt;

  // persistent
  R_Handle vertices;
  R_Handle indices;
  U64 vertex_buffer_cap;
  U64 indice_buffer_cap;
};

/////////////////////////////////
//~ Generated code

#include "generated/vtk.meta.h"

/////////////////////////////////
//~ Theme Types 

typedef struct VTK_Theme VTK_Theme;
struct VTK_Theme
{
  Vec4F32 colors[VTK_ThemeColor_COUNT];
};

typedef enum VTK_PaletteCode
{
  VTK_PaletteCode_Base,
  VTK_PaletteCode_MenuBar,
  VTK_PaletteCode_Floating,
  VTK_PaletteCode_ImplicitButton,
  VTK_PaletteCode_PlainButton,
  VTK_PaletteCode_PositivePopButton,
  VTK_PaletteCode_NegativePopButton,
  VTK_PaletteCode_NeutralPopButton,
  VTK_PaletteCode_ScrollBarButton,
  VTK_PaletteCode_Tab,
  VTK_PaletteCode_TabInactive,
  VTK_PaletteCode_DropSiteOverlay,
  VTK_PaletteCode_COUNT
} VTK_PaletteCode;

/////////////////////////////////
//~ Camera

typedef struct VTK_Camera VTK_Camera;
struct VTK_Camera
{
  F32 fov;
  F32 zn;
  F32 zf;
  Vec3F32 position;
  QuatF32 rotation;
};

/////////////////////////////////
//~ VTK Core Data Model Types

//- basic

typedef enum VTK_TopologyKind
{
  VTK_TopologyKind_Point,
  VTK_TopologyKind_Line,
  VTK_TopologyKind_LineStrip,
  VTK_TopologyKind_Triangle,
  VTK_TopologyKind_TriangleStrip,
  VTK_TopologyKind_Quad,
  VTK_TopologyKind_COUNT,
} VTK_TopologyKind;

typedef enum VTK_MeshKind
{
  VTK_MeshKind_Invalid = -1,
  VTK_MeshKind_ImageData,
  VTK_MeshKind_StructuredGrid,
  VTK_MeshKind_RectilinearGrid,
  VTK_MeshKind_UnstructuredGrid,
  VTK_MeshKind_PolyData,
} VTK_MeshKind;

//- attribute

// The Visualization Toolkit supports the following dataset attributes:
//   scalars (one to four components)
//   vectors
//   normals
//   texture coordinates (1D, 2D, and 3D)
//   tensors
//   field data. In addition, a lookup table using the RGBA color specification, associated with the scalar data, can be defined as well. Dataset attributes are supported for both points and cells.
// typedef enum VTK_AttributeKind
// {
//   VTK_AttributeKind_Scalar,
//   VTK_AttributeKind_Vector,
//   VTK_AttributeKind_Tensor,
//   VTK_AttributeKind_COUNT,
// } VTK_AttributeKind;
// 
// typedef struct VTK_Attribute VTK_Attribute;
// struct VTK_Attribute
// {
// };
// 
// typedef struct VTK_AttributeList VTK_AttributeList;
// struct VTK_AttributeList
// {
//   // FIXME: ...
//   U64 count;
// };

//- mesh type

// FIXME ...

typedef struct VTK_PolyData VTK_PolyData;
struct VTK_PolyData
{
};

//- data type

typedef struct VTK_Point VTK_Point;
struct VTK_Point
{
  Vec3F32 pos;
  // scalars, vectors, tensors, colors, normals e.g.
};

typedef struct VTK_Cell VTK_Cell;
struct VTK_Cell
{
  U32 *point_indices;
  U64 point_count;
  // scalars, vectors, tensors, colors, normals e.g.
};

// Any spatially referenced information and usually consists of geometrical representations of a surface or volume in 3D space
typedef struct VTK_Mesh VTK_Mesh;
struct VTK_Mesh
{
  VTK_MeshKind kind;

  union
  {
    // FIXME: ...
    VTK_PolyData poly_data;
  } v;

  VTK_Point *points;
  U64 point_count;
  VTK_Cell *cells;
  U64 cell_count;

  // FIXME: ...
  // point_data;
  // cell_data;
  // field_data; // Field data is not directly associated with either the points or cells but still should be attached to the mesh. This may be a string array storing notes, or even indices of a Collision.
};

////////////////////////////////
//~ State 

typedef struct VTK_State VTK_State;
struct VTK_State
{
  Arena *arena;
  Arena *frame_arenas[2];

  R_Handle r_wnd;
  OS_Handle os_wnd;
  UI_EventList *events;
  UI_Signal sig;

  // drawing buckets
  DR_Bucket *bucket_ui;
  DR_Bucket *bucket_main;

  // drawlists
  VTK_DrawList *drawlists[2];

  // frame history info
  U64 frame_index;
  U64 frame_time_us_history[64];
  F64 time_in_seconds;
  U64 time_in_us;

  // frame parameters
  F32 frame_dt;
  F32 cpu_time_us;
  F32 pre_cpu_time_us;

  // window
  Rng2F32 window_rect;
  Vec2F32 window_dim;
  Rng2F32 last_window_rect;
  Vec2F32 last_window_dim;
  B32 window_res_changed;
  F32 dpi;
  F32 last_dpi;
  B32 window_should_close;

  // mouse
  Vec2F32 mouse;
  Vec2F32 last_mouse;
  Vec2F32 mouse_delta; // frame delta
  B32 cursor_hidden;

  // palette
  // VTK_IconInfo           icon_info;
  VTK_WidgetPaletteInfo  widget_palette_info;

  // user interaction state
  VTK_Key pixel_hot_key; // hot pixel key from renderer

  VTK_Camera camera;

  // theme
  VTK_Theme cfg_theme_target;
  VTK_Theme cfg_theme;
  FNT_Tag cfg_font_tags[VTK_FontSlot_COUNT];

  // palette
  UI_Palette cfg_ui_debug_palettes[VTK_PaletteCode_COUNT]; // derivative from theme
  VTK_Palette cfg_main_palettes[VTK_PaletteCode_COUNT]; // derivative from theme

  // global Settings
  VTK_SettingVal setting_vals[VTK_SettingCode_COUNT];

  // animation info
  struct
  {
    F32 vast_rate;
    F32 fast_rate;
    F32 fish_rate;
    F32 slow_rate;
    F32 slug_rate;
    F32 slaf_rate;
  } animation;

  // FIXME: TESTING
  VTK_Mesh *mesh;
};

/////////////////////////////////
//~ Globals

global VTK_State *vtk_state;

/////////////////////////////////
//~ Basic Type Functions

internal U64     vtk_hash_from_string(U64 seed, String8 string);
internal String8 vtk_hash_part_from_key_string(String8 string);
internal String8 vtk_display_part_from_key_string(String8 string);

/////////////////////////////////
//~ Key

internal VTK_Key  vtk_key_from_string(VTK_Key seed, String8 string);
internal VTK_Key  vtk_key_from_stringf(VTK_Key seed, char* fmt, ...);
internal B32     vtk_key_match(VTK_Key a, VTK_Key b);
internal VTK_Key  vtk_key_make(U64 a, U64 b);
internal VTK_Key  vtk_key_zero();
internal Vec2F32 vtk_2f32_from_key(VTK_Key key);
internal VTK_Key  vtk_key_from_2f32(Vec2F32 key_2f32);

/////////////////////////////////
//~ Entry Call Functions

internal void vtk_init(OS_Handle os_wnd, R_Handle r_wnd);
internal B32  vtk_frame(void);

/////////////////////////////////
//~ State accessor/mutator

//- frame
internal inline Arena *vtk_frame_arena(void);
internal inline VTK_DrawList *vtk_frame_drawlist(void);

//- color
internal Vec4F32 vtk_rgba_from_theme_color(VTK_ThemeColor color);

//- code -> palette
internal VTK_Palette* vtk_palette_from_code(VTK_PaletteCode code);

//- code -> ui palette
internal UI_Palette* vtk_ui_palette_from_code(VTK_PaletteCode code);

//- fonts/size
internal FNT_Tag vtk_font_from_slot(VTK_FontSlot slot);
internal F32     vtk_font_size_from_slot(VTK_FontSlot slot);

/////////////////////////////////
// Dynamic Draw List

internal VTK_DrawList* vtk_drawlist_alloc(Arena *arena, U64 vertex_buffer_cap, U64 indice_buffer_cap);
internal VTK_DrawNode* vtk_drawlist_push(Arena *arena, VTK_DrawList *drawlist, R_Geo3D_Vertex *vertices_src, U64 vertex_count, U32 *indices_src, U64 indice_count);
internal void          vtk_drawlist_reset(VTK_DrawList *drawlist);
internal void          vtk_drawlist_build(VTK_DrawList *drawlist); /* upload buffer from cpu to gpu */

/////////////////////////////////
//~ Loader Functions

internal VTK_Mesh *vtk_mesh_from_vtk(Arena *arena, String8 path);
internal VTK_Mesh *vtk_mesh_from_ply(Arena *arena, String8 path);

/////////////////////////////////
//~ Drawing

internal void vtk_ui_draw();

#endif
