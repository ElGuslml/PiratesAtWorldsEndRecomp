// potcawe - XenosNative for POTCAWE: the game's D3D entry points and its D3D device layout.
//
// The XDK Direct3D library is linked into default.xex (XDK 2.0.5426). The device code is at
// 0x822AD000-0x822C7000, the draw functions apart at 0x824ABAD8-0x824AC818. Every address and offset
// here, with its evidence, is in docs/xenos-native-potcawe.md. The hooks only run the original
// function unless a XenosNative feature is on (--xenos_native_trace).

#include "xenos_native_potcawe.h"

#include <xenos_native/hooks.h>

// Draws. All scene meshes come from sub_823DF3A8 through the engine's D3D9-style wrappers
// sub_823DF278 (DrawIndexedPrimitive) and sub_823DF2C8 (DrawPrimitive).
XENOS_NATIVE_HOOK(824AC3C0, DrawIndexedVertices)
XENOS_NATIVE_HOOK(824ABFD8, DrawVertices)
XENOS_NATIVE_HOOK(824ABF78, DrawVerticesUP)  // calls BeginVertices, inlines the end
XENOS_NATIVE_HOOK(824ABAD8, BeginVertices)

// Render targets, clears, resolves, tiling. The engine sets targets through sub_822B55A0, an alias
// that branches to SetRenderTarget.
XENOS_NATIVE_HOOK(822B4F60, SetRenderTarget)
XENOS_NATIVE_HOOK(822B52C8, SetDepthStencilSurface)
XENOS_NATIVE_HOOK(822C6AA0, Clear)  // calls ClearF
XENOS_NATIVE_HOOK(822C6978, ClearF)
XENOS_NATIVE_HOOK(822C2048, Resolve)
XENOS_NATIVE_HOOK(822BCC58, BeginTiling)
XENOS_NATIVE_HOOK(822BD1E0, EndTiling)
XENOS_NATIVE_HOOK(822BD698, BeginZPass)
XENOS_NATIVE_HOOK(822BD9E0, EndZPass)
XENOS_NATIVE_HOOK(822BCAE8, SetPredication)
XENOS_NATIVE_HOOK(822B4E28, SetViewport)
XENOS_NATIVE_HOOK(822B4550, SetScissorRect)

// Shaders, vertex input, textures, constants. Most constants are written by the engine straight into
// the device (about 44 functions), without a call.
XENOS_NATIVE_HOOK(822B7E18, SetVertexShader)
XENOS_NATIVE_HOOK(822B7B60, SetPixelShader)
XENOS_NATIVE_HOOK(822B7FE8, SetVertexDeclaration)
XENOS_NATIVE_HOOK(822B46A0, SetStreamSource)
XENOS_NATIVE_HOOK(822B47C0, SetIndices)
XENOS_NATIVE_HOOK(822AC668, SetTexture)
XENOS_NATIVE_HOOK(822B7848, SetVertexShaderConstantF)
XENOS_NATIVE_HOOK(822B7950, SetPixelShaderConstantF)

// Resources: typed thunks over the common Lock (sub_822ACA58) and Unlock (sub_822ACC90), and the
// cache flush over a range that Unlock (and the ring code) calls.
XENOS_NATIVE_HOOK(822ACEE0, VertexBufferLock)
XENOS_NATIVE_HOOK(822ACF30, VertexBufferUnlock)
XENOS_NATIVE_HOOK(822ACF40, IndexBufferLock)
XENOS_NATIVE_HOOK(822ACF88, IndexBufferUnlock)
XENOS_NATIVE_HOOK(822AA700, TextureUnlock)
XENOS_NATIVE_HOOK(822AF398, FlushCpuRange)

// Command buffer, queries, frame. Swap runs twice per frame: from Synchronize (sub_822B5D50, an alias
// with no front buffer) and from Present.
XENOS_NATIVE_HOOK(822AEC50, KickSegment)
XENOS_NATIVE_HOOK(822B8120, SetShaderGPRAllocation)  // runs prebuilt buffers of 1-point draws
XENOS_NATIVE_HOOK(822BA590, QueryIssue)
XENOS_NATIVE_HOOK(822BA818, QueryGetData)
XENOS_NATIVE_HOOK(822B58B0, Swap)
XENOS_NATIVE_HOOK(822B5D58, Present)

namespace potcawe {

const xenos_native::Profile& XenosNativeProfile() {
  static const xenos_native::Profile profile = [] {
    xenos_native::Profile p;
    p.game = "Pirates of the Caribbean: At World's End (425607D1)";
    p.xdk_version = "2.0.5426";
    xenos_native::DeviceLayout& d = p.device;
    d.dirty_masks = 0;
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
    return p;
  }();
  return profile;
}

}  // namespace potcawe
