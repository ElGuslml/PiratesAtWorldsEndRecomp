// potcawe - NativeXRenderer for POTCAWE: the game's D3D entry points and its D3D device layout.
//
// The XDK Direct3D library is linked into default.xex (XDK 2.0.5426). The device code is at
// 0x822AD000-0x822C7000, the draw functions apart at 0x824ABAD8-0x824AC818. Every address and offset
// here, with its evidence, is in docs/native-x-renderer-potcawe.md. The hooks only run the original
// function unless a NativeXRenderer feature is on (--native_x_renderer_trace).

#include "native_x_renderer_potcawe.h"

#include <native_x_renderer/hooks.h>

// Draws. All scene meshes come from sub_823DF3A8 through the engine's D3D9-style wrappers
// sub_823DF278 (DrawIndexedPrimitive) and sub_823DF2C8 (DrawPrimitive).
NATIVE_X_RENDERER_HOOK(824AC3C0, DrawIndexedVertices)
NATIVE_X_RENDERER_HOOK(824ABFD8, DrawVertices)
NATIVE_X_RENDERER_HOOK(824ABF78, DrawVerticesUP)  // calls BeginVertices, inlines the end
NATIVE_X_RENDERER_HOOK(824ABAD8, BeginVertices)

// Render targets, clears, resolves, tiling. The engine sets targets through sub_822B55A0, an alias
// that branches to SetRenderTarget.
NATIVE_X_RENDERER_HOOK(822B4F60, SetRenderTarget)
NATIVE_X_RENDERER_HOOK(822B52C8, SetDepthStencilSurface)
NATIVE_X_RENDERER_HOOK(822C6AA0, Clear)  // calls ClearF
NATIVE_X_RENDERER_HOOK(822C6978, ClearF)
NATIVE_X_RENDERER_HOOK(822C2048, Resolve)
NATIVE_X_RENDERER_HOOK(822BCC58, BeginTiling)
NATIVE_X_RENDERER_HOOK(822BD1E0, EndTiling)
NATIVE_X_RENDERER_HOOK(822BD698, BeginZPass)
NATIVE_X_RENDERER_HOOK(822BD9E0, EndZPass)
NATIVE_X_RENDERER_HOOK(822BCAE8, SetPredication)
NATIVE_X_RENDERER_HOOK(822B4E28, SetViewport)
NATIVE_X_RENDERER_HOOK(822B4550, SetScissorRect)

// Shaders, vertex input, textures, constants. Most constants are written by the engine straight into
// the device (about 44 functions), without a call.
NATIVE_X_RENDERER_HOOK(822B7E18, SetVertexShader)
NATIVE_X_RENDERER_HOOK(822B7B60, SetPixelShader)
NATIVE_X_RENDERER_HOOK(822B7FE8, SetVertexDeclaration)
NATIVE_X_RENDERER_HOOK(822B46A0, SetStreamSource)
NATIVE_X_RENDERER_HOOK(822B47C0, SetIndices)
NATIVE_X_RENDERER_HOOK(822AC668, SetTexture)
NATIVE_X_RENDERER_HOOK(822B7848, SetVertexShaderConstantF)
NATIVE_X_RENDERER_HOOK(822B7950, SetPixelShaderConstantF)

// Resources: typed thunks over the common Lock (sub_822ACA58) and Unlock (sub_822ACC90), and the
// cache flush over a range that Unlock (and the ring code) calls.
NATIVE_X_RENDERER_HOOK(822ACEE0, VertexBufferLock)
NATIVE_X_RENDERER_HOOK(822ACF30, VertexBufferUnlock)
NATIVE_X_RENDERER_HOOK(822ACF40, IndexBufferLock)
NATIVE_X_RENDERER_HOOK(822ACF88, IndexBufferUnlock)
NATIVE_X_RENDERER_HOOK(822AA700, TextureUnlock)
NATIVE_X_RENDERER_HOOK(822AF398, FlushCpuRange)

// Command buffer, queries, frame. Swap runs once per frame, from the engine's Synchronize
// (sub_822B5D50, an alias of Swap with no front buffer); Present does not call it.
NATIVE_X_RENDERER_HOOK(822AEC50, KickSegment)
// New segment from the allocator (device +48/+52/+56): from KickSegment (sub_822AEAC8), from the
// reservation sub_822AEE50 when a reservation does not fit after the kick, and from Present.
NATIVE_X_RENDERER_HOOK(822AE950, NewSegment)
NATIVE_X_RENDERER_HOOK(822B8120, SetShaderGPRAllocation)  // runs prebuilt buffers of 1-point draws
NATIVE_X_RENDERER_HOOK(822BA590, QueryIssue)
NATIVE_X_RENDERER_HOOK(822BA818, QueryGetData)
NATIVE_X_RENDERER_HOOK(822B58B0, Swap)
NATIVE_X_RENDERER_HOOK(822B5D58, Present)

namespace potcawe {

const native_x_renderer::Profile& NativeXRendererProfile() {
  static const native_x_renderer::Profile profile = [] {
    native_x_renderer::Profile p;
    p.game = "Pirates of the Caribbean: At World's End (425607D1)";
    p.xdk_version = "2.0.5426";
    p.output_width = 1280;  // 720p, 16:9, whatever the console's video mode
    p.output_height = 720;
    // The end of the engine's post-processing: the finished scene, blended with the last frame, copied to
    // 0x09B2E000; the HUD composite draws it, then the HUD over it (docs/native-x-renderer-potcawe.md).
    p.scene_resolve_caller = 0x824583EC;
    // src/aspect.cpp widens the game's 16:9 cameras: NativeXRenderer fills outputs of any aspect ratio.
    p.widens_cameras = true;
    native_x_renderer::DeviceLayout& d = p.device;
    // Six dirty masks from +0: VS constants, PS constants, register groups, fetch slots (low half, slot 0
    // in bit 31), misc, tiling-dependent state.
    d.vs_constants_dirty = 0;
    d.ps_constants_dirty = 8;
    d.fetch_constants_dirty = 24;
    d.fetch_dirty_slot0_bit = 31;
    d.command_buffer_write = 48;
    d.fetch_constants = 1024;
    d.vs_float_constants = 1792;
    d.ps_float_constants = 5888;
    d.bool_loop_constants = 9984;
    d.register_groups = {{
        {10240, 0x2000, 16},
        {10316, 0x2100, 21},
        {10400, 0x2180, 5},
        {10420, 0x2200, 12},
        {10468, 0x2280, 21},
        {10552, 0x2300, 38},
        {10704, 0x2380, 8},
    }};
    d.fence_counter = 10780;
    d.write_back_block = 10768;
    d.flags = 10812;
    d.vertex_declaration = 11684;
    d.index_buffer = 12300;
    d.render_targets = 12304;
    d.depth_stencil = 12320;
    d.vertex_buffers = 12324;
    d.textures = 12408;
    d.viewport = 12512;
    d.pixel_shader = 12556;
    d.vertex_shader = 12560;
    d.predication_mask = 12572;
    d.tile_count = 12612;
    d.tile_rects = 12616;
    native_x_renderer::ObjectLayout& o = p.objects;
    o.texture_fetch = 28;
    o.surface_info = 24;
    o.surface_color_depth_info = 28;
    o.surface_size = 36;
    o.surface_width_minus_one = {18, 14};
    o.surface_height_minus_one = {3, 15};
    o.buffer_address = 24;
    o.buffer_size = 28;
    return p;
  }();
  return profile;
}

}  // namespace potcawe
