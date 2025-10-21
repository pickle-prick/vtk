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

  for(U64 i = 0; i < ArrayCount(vtk_state->frame_arenas); i++)
  {
    vtk_state->frame_arenas[i] = arena_alloc(.reserve_size = MB(64), .commit_size = MB(32));
  }

  for(U64 i = 0; i < ArrayCount(vtk_state->drawlists); i++)
  {
    // NOTE(k): some device offers 256MB memory which is both cpu visiable and device local
    vtk_state->drawlists[i] = vtk_drawlist_alloc(arena, MB(16), MB(16));
  }

  // FIXME: TESTING
  String8 vtk_path = str8_lit("./data/cube.vtk");
  VTK_Mesh *mesh = vtk_mesh_from_vtk(arena, vtk_path);
  vtk_state->mesh = mesh;
}

// FIXME: testing
internal B32 vtk_update(void)
{
  B32 window_should_close = 0;
  for(OS_Event *os_evt = vtk_state->os_events.first; os_evt != 0; os_evt = os_evt->next)
  {
    if(os_evt->kind == OS_EventKind_WindowClose) {window_should_close = 1;}
  }

  vtk_drawlist_reset(vtk_frame_drawlist());

  // FIXME: what?
  VTK_Mesh *mesh = vtk_state->mesh;
  VTK_DrawNode *dn = 0;
  {
#if 0
    R_Geo3D_Vertex *vertices = push_array(vtk_frame_arena(), R_Geo3D_Vertex, 3);
    vertices[0] = (R_Geo3D_Vertex){.pos = v3f32(0,0,0), .nor = v3f32(0,0,0), .col = v4f32(1,0,0,1)};
    vertices[1] = (R_Geo3D_Vertex){.pos = v3f32(0,-1,0), .nor = v3f32(0,0,0), .col = v4f32(0,0,1,1)};
    vertices[2] = (R_Geo3D_Vertex){.pos = v3f32(1,0,0), .nor = v3f32(0,0,0), .col = v4f32(0,1,0,1)};
    U64 vertex_count = 3;
    U32 *indices = push_array(vtk_frame_arena(), U32, 3);
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    U64 indice_count = 3;
#else
    R_Geo3D_Vertex *vertices = push_array(vtk_frame_arena(), R_Geo3D_Vertex, mesh->point_count);
    U64 vertex_count = mesh->point_count;
    Vec4F32 colors[3] = {v4f32(1,0,0,1), v4f32(0,1,0,1), v4f32(0,0,1,1)};
    for(U64 i = 0; i < mesh->point_count; i++)
    {
      vertices[i] = (R_Geo3D_Vertex){.pos = mesh->points[i].pos, .nor = v3f32(0,0,0), .col = v4f32(1,0,0,1)};
      vertices[i].pos.y = -vertices[i].pos.y;
      vertices[i].col = colors[i%3];
    }
    U32 *indices = push_array(vtk_frame_arena(), U32, 36);
    U64 indice_count = 36;
    U32 indice_offset = 0;
    for(U64 cell_idx = 0; cell_idx < mesh->cell_count; cell_idx++)
    {
      VTK_Cell *cell = &mesh->cells[cell_idx];

      indices[0+indice_offset] = cell->indices[0+1];
      indices[1+indice_offset] = cell->indices[1+1];
      indices[2+indice_offset] = cell->indices[2+1];

      indices[3+indice_offset] = cell->indices[0+1];
      indices[4+indice_offset] = cell->indices[2+1];
      indices[5+indice_offset] = cell->indices[3+1];

      indice_offset += 6;
    }
#endif
    dn = vtk_drawlist_push(vtk_frame_arena(), vtk_frame_drawlist(), vertices, vertex_count, indices, indice_count);
    // dn->topology = R_GeoTopologyKind_TriangleStrip;
    dn->topology = R_GeoTopologyKind_Triangles;
    dn->polygon = R_GeoPolygonKind_Fill;
  }

  vtk_drawlist_build(vtk_frame_drawlist());

  // drawing
  DR_BucketScope(vtk_state->bucket_main)
  {
    Vec3F32 eye = {0, -3, -3};
    Rng2F32 viewport = vtk_state->window_rect;
    Vec2F32 viewport_dim = dim_2f32(viewport);
    Mat4x4F32 view_mat = make_look_at_vulkan_4x4f32(eye, v3f32(0,0,0), v3f32(0,-1,0));
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

    // draw mesh
    {
      R_Mesh3DInst *inst = dr_mesh(dn->vertices, dn->indices,
                                   dn->vertices_buffer_offset, dn->indices_buffer_offset, dn->indice_count,
                                   dn->topology, dn->polygon,
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
      // Mat4x4F32 xform = make_translate_4x4f32(v3f32(0,6,0));
      Mat4x4F32 xform = mat_4x4f32(1.0);
      inst->xform = xform;
      inst->xform_inv = inverse_4x4f32(xform);
      inst->omit_light = 1;
      inst->draw_edge = 1;
      inst->depth_test = 1;
      // inst->key 
    }
  }

  return window_should_close;
}

internal B32
vtk_frame(void)
{
  ProfBeginFunction();

  arena_clear(vtk_frame_arena());
  dr_begin_frame();
  vtk_state->bucket_ui = dr_bucket_make();
  vtk_state->bucket_main = dr_bucket_make();

  /////////////////////////////////
  //~ Get events from os
  
  vtk_state->os_events = os_get_events(vtk_frame_arena(), 0);
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

  // FIXME: testing
  vtk_state->window_should_close = vtk_update();

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

  // TODO(XXX): we should use buffer block, we can't just release/realloc a new buffer, since the buffer handle is already handled over to RK_DrawNode
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
  U64 cell_count = 0;
  U64 total_indice_count = 0;
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

          if(str8_starts_with(line, str8_lit("POLYGONS"), 0))
          {
            int n = 0;
            int indice_count = 0;
            int nread = sscanf((char*)line.str, "%*s %d %d", &n, &indice_count);
            int indices_per_cell = 0;
            cell_count = n;
            total_indice_count = indice_count;

            if(cell_count > 0)
            {
              cells = push_array(arena, VTK_Cell, cell_count);
              indices_per_cell = indice_count / n;
            }

            for(U64 i = 0; i < cell_count; i++)
            {
              VTK_Cell *cell = &cells[i];
              cell->indices = push_array(arena, U32, indices_per_cell);
              cell->indice_count = indices_per_cell;
              for(U64 j = 0; j < indices_per_cell; j++)
              {
                U8 *mark = c;
                for(; !char_is_space(*c); c++);
                String8 idx_str = str8_range(mark, c);
                U64 indice = u64_from_str8(idx_str, 10);
                c++;
                cell->indices[j] = indice;
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
  }

  // fill&return
  VTK_Mesh *ret = push_array(arena, VTK_Mesh, 1);
  ret->kind = mesh_kind;
  ret->points = points;
  ret->point_count = point_count;
  ret->cells = cells;
  ret->cell_count = cell_count;
  ret->total_indice_count = total_indice_count;
  scratch_end(scratch);
  return ret;
}

internal VTK_Mesh *
vtk_mesh_from_ply(Arena *arena, String8 path)
{
  NotImplemented;
}
