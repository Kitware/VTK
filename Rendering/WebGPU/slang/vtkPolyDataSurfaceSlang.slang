// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

import slangkit_surface;

#if defined(VTK_PIPELINE_POINTS) || defined(VTK_PIPELINE_POINTS_SHAPED)
#define VTK_POINTS_PIPELINE 1
#elif defined(VTK_PIPELINE_LINES) || defined(VTK_PIPELINE_LINES_THICK) || \
  defined(VTK_PIPELINE_LINES_ROUND) || defined(VTK_PIPELINE_LINES_MITER)
#define VTK_LINES_PIPELINE 1
#if !defined(VTK_PIPELINE_LINES)
#define VTK_WIDE_LINES_PIPELINE 1
#endif
#else
#define VTK_TRIANGLES_PIPELINE 1
#endif

type_param M : IMaterial;
type_param L : ILightingModel;
type_param N : INormalModifier;
type_param O : ISurfaceOutput;
type_param V : IVertexModifier;
type_param H : IShaderHooks;
typealias Default_IMaterial       = HostDrivenMaterial;
typealias Default_ILightingModel  = PhongLighting;
typealias Default_INormalModifier = GeometricNormal;
typealias Default_ISurfaceOutput  = StandardOutput;
typealias Default_IVertexModifier = IdentityVertex;
typealias Default_IShaderHooks    = DefaultHooks;

struct SceneTransform
{
    float4   viewport;
    float4x4 view;
    float4x4 projection;
    float3x3 normal;
    float4x4 inverted_projection;
    uint     flags;
};

static const uint VTK_LIGHT_TYPE_HEADLIGHT = 1u;

struct SceneLight
{
    uint   light_type;
    uint   positional;
    float  cone_angle;
    float  exponent;
    float3 color;
    float3 direction_vc;
    float3 position_vc;
    float3 attenuation;
};

SceneLight loadSceneLight(ByteAddressBuffer lights, uint i)
{
    uint       base = 16u + 80u * i;
    SceneLight l;
    l.light_type   = lights.Load(base + 0u);
    l.positional   = lights.Load(base + 4u);
    l.cone_angle   = asfloat(lights.Load(base + 8u));
    l.exponent     = asfloat(lights.Load(base + 12u));
    l.color        = asfloat(lights.Load3(base + 16u));
    l.direction_vc = asfloat(lights.Load3(base + 32u));
    l.position_vc  = asfloat(lights.Load3(base + 48u));
    l.attenuation  = asfloat(lights.Load3(base + 64u));
    return l;
}

struct ActorTransform
{
    float4x4 world;
    float3x3 normal;
    float4x4 texture_coordinate;
};

struct ActorRenderOptions
{
    float point_size;
    float line_width;
    float edge_width;
    uint  flags;
    uint  flags_2;
    float coin_polygon_factor;
    float coin_polygon_offset;
    float coin_line_factor;
    float coin_line_offset;
    float coin_point_offset;
};

struct ActorColorOptions
{
    float3 ambient_color;
    float3 diffuse_color;
    float3 specular_color;
    float3 edge_color;
    float3 vertex_color;
    float  ambient_intensity;
    float  diffuse_intensity;
    float  specular_intensity;
    float  specular_power;
    float  opacity;
    uint   interpolation_type;
    uint   id;
    // The PBR material follows; this program only needs the layout to reach the backface block.
    float  metallic;
    float  roughness;
    float  base_f0;
    float  anisotropy;
    float  anisotropy_rotation;
    float  coat_strength;
    float  coat_roughness;
    float  coat_f0;
    float  coat_normal_scale;
    float  normal_scale;
    float3 emissive_factor;
    float  occlusion_strength;
    float3 edge_tint;
    float3 coat_color;
    float3 base_specular_color;
    float  base_specular_weight;
};

struct Actor
{
    ActorTransform     transform;
    ActorRenderOptions render_options;
    ActorColorOptions  color_options;
    ActorColorOptions  backface_color_options;
};

static const uint VTK_SURFACE = 2u;
uint getRepresentation(uint flags) { return flags & 0x3u; }
bool getEdgeVisibility(uint flags) { return ((flags >> 2u) & 0x1u) == 1u; }
bool getVertexVisibility(uint flags) { return ((flags >> 3u) & 0x1u) == 1u; }
bool getRenderPointsAsSpheres(uint flags) { return ((flags >> 5u) & 0x1u) == 1u; }
static const uint POINT_2D_ROUND  = 0u;
static const uint POINT_2D_SQUARE = 1u;
uint getPoint2DShape(uint flags) { return ((flags >> 7u) & 0x1u) == 1u ? POINT_2D_SQUARE : POINT_2D_ROUND; }
bool getUseLineWidthForEdgeThickness(uint flags) { return ((flags >> 4u) & 0x1u) == 1u; }
bool getRenderLinesAsTubes(uint flags) { return ((flags >> 6u) & 0x1u) == 1u; }
bool isLightingEnabled(uint flags_2) { return (flags_2 & 0x1u) == 1u; }
bool hasBackfaceProperty(uint flags_2) { return (flags_2 & 0x2u) != 0u; }
bool showTexturesOnBackface(uint flags_2) { return (flags_2 & 0x4u) != 0u; }
bool getUseParallelProjection(uint flags) { return (flags & 0x1u) == 1u; }

struct ClippingPlanes
{
    float4 plane_equations[6];
    uint   count;
};

[vk::binding(0, 0)] ConstantBuffer<SceneTransform> scene_transform;
[vk::binding(1, 0)] ByteAddressBuffer              scene_lights;
#ifdef VTK_DEEP_FRAMEBUFFER
struct FragmentNode
{
    uint  color;
    float depth;
    uint  cellId;
    uint  propId;
    uint  compositeId;
    uint  processId;
    uint  next;
};
[vk::binding(2, 0)] RWStructuredBuffer<Atomic<uint>> abufferHeads;
[vk::binding(3, 0)] RWStructuredBuffer<FragmentNode> abufferNodes;
[vk::binding(4, 0)] RWStructuredBuffer<Atomic<uint>> abufferCounter;
#endif
#ifndef VTK_DEPTH_BITS
#define VTK_DEPTH_BITS 24
#endif
[vk::binding(0, 1)] StructuredBuffer<Actor>        actor_buffer;
#ifdef VTK_ACTOR_TEXTURE
[vk::binding(1, 1)] SamplerState                   actor_texture_sampler;
[vk::binding(2, 1)] Texture2D<float4>              actor_texture;
#endif
[vk::binding(16, 1)] ConstantBuffer<Shading<M, L, N, O, V>> shading;
[vk::binding(32, 1)] ConstantBuffer<HookBlock<H>> hookBlock;

[vk::binding(0, 2)] StructuredBuffer<float> point_coordinates;
#ifdef VTK_POINT_COLORS_BINDING
[vk::binding(VTK_POINT_COLORS_BINDING, 2)] StructuredBuffer<float> point_colors;
#endif
#ifdef VTK_POINT_NORMALS_BINDING
[vk::binding(VTK_POINT_NORMALS_BINDING, 2)] StructuredBuffer<float> point_normals;
#endif
#ifdef VTK_POINT_TANGENTS_BINDING
[vk::binding(VTK_POINT_TANGENTS_BINDING, 2)] StructuredBuffer<float> point_tangents;
#endif
#ifdef VTK_POINT_UVS_BINDING
[vk::binding(VTK_POINT_UVS_BINDING, 2)] StructuredBuffer<float> point_uvs;
#endif
#ifdef VTK_POINT_COLOR_UVS_BINDING
[vk::binding(VTK_POINT_COLOR_UVS_BINDING, 2)] StructuredBuffer<float> point_color_uvs;
#endif
#ifdef VTK_POINT_COLOR_TEXTURE_BINDING
[vk::binding(VTK_POINT_COLOR_SAMPLER_BINDING, 2)] SamplerState point_color_sampler;
[vk::binding(VTK_POINT_COLOR_TEXTURE_BINDING, 2)] Texture2D<float4> point_color_texture;
#endif
#ifdef VTK_CELL_COLORS_BINDING
[vk::binding(VTK_CELL_COLORS_BINDING, 2)] StructuredBuffer<float> cell_colors;
#endif
#ifdef VTK_CELL_NORMALS_BINDING
[vk::binding(VTK_CELL_NORMALS_BINDING, 2)] StructuredBuffer<float> cell_normals;
#endif
#ifdef VTK_CLIPPING_PLANES_BINDING
[vk::binding(VTK_CLIPPING_PLANES_BINDING, 2)] StructuredBuffer<ClippingPlanes> clipping_planes;
#endif

[vk::binding(0, 3)] StructuredBuffer<uint> connectivity;
#ifdef VTK_HOMOGENEOUS_CELL_SIZE
[vk::binding(1, 3)] ConstantBuffer<uint> cell_id_offset;
#else
[vk::binding(1, 3)] StructuredBuffer<uint>  cell_ids;
#ifdef VTK_TRIANGLES_PIPELINE
[vk::binding(2, 3)] StructuredBuffer<float> edge_array;
#endif
#endif

struct VertexOutput
{
    float4                    position     : SV_Position;
    float4                    color        : VTK_COLOR;
    float2                    uv           : VTK_UV;
    float2                    lut_uv       : VTK_LUT_UV;
    float4                    position_VC  : POSITION_VC;
    float3                    normal_VC    : NORMAL_VC;
    float3                    tangent_VC   : TANGENT_VC;
#ifdef VTK_TRIANGLES_PIPELINE
    float3                    edge_dists   : EDGE_DISTS;
    nointerpolation float     hide_edge    : HIDE_EDGE;
#endif
#ifdef VTK_PIPELINE_POINTS_SHAPED
    float2                    p_coord      : P_COORD;
#endif
#ifdef VTK_WIDE_LINES_PIPELINE
    float                     dist_to_centerline : DIST_TO_CENTERLINE;
#endif
#ifdef VTK_CLIPPING_PLANES_BINDING
    float3                    clip_dists_0 : CLIP_DISTS_0;
    float3                    clip_dists_1 : CLIP_DISTS_1;
#endif
    nointerpolation uint4     ids          : VTK_IDS;
    nointerpolation uint2     group_ids    : VTK_GROUP_IDS;
};

float3 pointPosition(uint point_id)
{
    return float3(point_coordinates[3u * point_id], point_coordinates[3u * point_id + 1u],
                  point_coordinates[3u * point_id + 2u]);
}

// The actor's texture coordinate matrix, including the homogeneous divide, like OpenGL's tcMatrix.
float2 transformTextureCoordinates(float2 uv)
{
    float4 transformed =
        mul(actor_buffer[0].transform.texture_coordinate, float4(uv, 0.f, 1.f));
    return transformed.xy / transformed.w;
}

float3 computeFaceNormal(float3 pA, float3 pB, float3 pC)
{
    return normalize(cross(normalize(pB - pA), normalize(pC - pA)));
}

VertexSample fetchPoint(uint point_id, uint cell_id, uint primitive_id,
                        float4x4 model_view_projection, float3 fallbackNormal)
{
    VertexSample v = VertexSample();
    v.position    = pointPosition(point_id);
    v.pointId     = point_id;
    v.cellId      = cell_id;
    v.primitiveId = primitive_id;
    v.modelToClip = model_view_projection;
#if defined(VTK_POINT_NORMALS_BINDING)
    v.normal = float3(point_normals[3u * point_id], point_normals[3u * point_id + 1u],
                      point_normals[3u * point_id + 2u]);
#elif defined(VTK_CELL_NORMALS_BINDING)
    v.normal = float3(cell_normals[3u * cell_id], cell_normals[3u * cell_id + 1u],
                      cell_normals[3u * cell_id + 2u]);
#else
    v.normal = fallbackNormal;
#endif
#ifdef VTK_POINT_TANGENTS_BINDING
    v.tangent = float3(point_tangents[3u * point_id], point_tangents[3u * point_id + 1u],
                       point_tangents[3u * point_id + 2u]);
#endif
#ifdef VTK_POINT_UVS_BINDING
    v.uv = transformTextureCoordinates(
        float2(point_uvs[2u * point_id], point_uvs[2u * point_id + 1u]));
#endif
#if defined(VTK_POINT_COLORS_BINDING)
    v.color = float4(point_colors[4u * point_id], point_colors[4u * point_id + 1u],
                     point_colors[4u * point_id + 2u], point_colors[4u * point_id + 3u]);
#elif defined(VTK_CELL_COLORS_BINDING)
    v.color = float4(cell_colors[4u * cell_id], cell_colors[4u * cell_id + 1u],
                     cell_colors[4u * cell_id + 2u], cell_colors[4u * cell_id + 3u]);
#endif
    return shading.vertex.modify(v);
}

VertexSample fetchVertex(uint index, uint cell_id, uint primitive_id,
                         float4x4 model_view_projection)
{
    return fetchPoint(connectivity[index], cell_id, primitive_id, model_view_projection,
                      float3(0.f, 0.f, 1.f));
}

#ifdef VTK_TRIANGLES_PIPELINE
VertexSample fetchTriangleVertex(uint primitive_id, uint k, uint cell_id,
                                 float4x4 model_view_projection)
{
    uint   point_id = connectivity[primitive_id * 3u + k];
    float3 face     = float3(0.f, 0.f, 1.f);
#if !defined(VTK_POINT_NORMALS_BINDING) && !defined(VTK_CELL_NORMALS_BINDING)
    uint next_point_id = connectivity[primitive_id * 3u + (k + 1u) % 3u];
    uint prev_point_id = connectivity[primitive_id * 3u + (k + 2u) % 3u];
    face = computeFaceNormal(pointPosition(point_id), pointPosition(next_point_id),
                             pointPosition(prev_point_id));
#endif
    return fetchPoint(point_id, cell_id, primitive_id, model_view_projection, face);
}
#endif

#ifdef VTK_PIPELINE_POINTS_SHAPED
static const float2 kPointQuad[4] = { float2(-1.f, -1.f), float2(1.f, -1.f), float2(-1.f, 1.f),
                                      float2(1.f, 1.f) };
#endif
#if defined(VTK_PIPELINE_LINES_THICK) || defined(VTK_PIPELINE_LINES_MITER)
static const float2 kLineQuad[4] = { float2(0.f, -0.5f), float2(1.f, -0.5f), float2(0.f, 0.5f),
                                     float2(1.f, 0.5f) };
#endif
#ifdef VTK_PIPELINE_LINES_ROUND
static const float3 kRoundLine[36] = {
    float3(0.f, -0.5f, 0.f), float3(0.f, -0.5f, 1.f), float3(0.f, 0.5f, 1.f),
    float3(0.f, -0.5f, 0.f), float3(0.f, 0.5f, 1.f), float3(0.f, 0.5f, 0.f),
    float3(0.f, 0.f, 0.f), float3(3.061616997868383e-17f, 0.5f, 0.f),
    float3(-0.2938926261462365f, 0.4045084971874737f, 0.f),
    float3(0.f, 0.f, 0.f), float3(-0.2938926261462365f, 0.4045084971874737f, 0.f),
    float3(-0.47552825814757677f, 0.15450849718747375f, 0.f),
    float3(0.f, 0.f, 0.f), float3(-0.47552825814757677f, 0.15450849718747375f, 0.f),
    float3(-0.4755282581475768f, -0.15450849718747364f, 0.f),
    float3(0.f, 0.f, 0.f), float3(-0.4755282581475768f, -0.15450849718747364f, 0.f),
    float3(-0.2938926261462366f, -0.40450849718747367f, 0.f),
    float3(0.f, 0.f, 0.f), float3(-0.2938926261462366f, -0.40450849718747367f, 0.f),
    float3(-9.184850993605148e-17f, -0.5f, 0.f),
    float3(0.f, 0.f, 1.f), float3(-9.184850993605148e-17f, -0.5f, 1.f),
    float3(0.29389262614623646f, -0.4045084971874738f, 1.f),
    float3(0.f, 0.f, 1.f), float3(0.29389262614623646f, -0.4045084971874738f, 1.f),
    float3(0.47552825814757677f, -0.1545084971874738f, 1.f),
    float3(0.f, 0.f, 1.f), float3(0.47552825814757677f, -0.1545084971874738f, 1.f),
    float3(0.4755282581475768f, 0.1545084971874736f, 1.f),
    float3(0.f, 0.f, 1.f), float3(0.4755282581475768f, 0.1545084971874736f, 1.f),
    float3(0.2938926261462367f, 0.4045084971874736f, 1.f),
    float3(0.f, 0.f, 1.f), float3(0.2938926261462367f, 0.4045084971874736f, 1.f),
    float3(1.5308084989341916e-16f, 0.5f, 1.f)
};
#endif

float2 toScreen(float4 p_DC, float2 resolution)
{
    return resolution * (0.5f * p_DC.xy / p_DC.w + 0.5f);
}

float4 fromScreen(float2 p, float4 p_DC, float2 resolution)
{
    return float4(p_DC.w * ((2.f * p) / resolution - 1.f), p_DC.z, p_DC.w);
}

[shader("vertex")]
VertexOutput vertexMain(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID
                        )
{
    VertexOutput output = VertexOutput();
    Actor        actor  = actor_buffer[0];

    float4x4 model_view_projection =
        mul(scene_transform.projection, mul(scene_transform.view, actor.transform.world));
    float3x3 normal_model_view = mul(scene_transform.normal, actor.transform.normal);

#if defined(VTK_PIPELINE_POINTS)
    uint pull_vertex_id = vertex_id;
    uint primitive_id   = pull_vertex_id;
#elif defined(VTK_PIPELINE_POINTS_SHAPED)
    uint pull_vertex_id = instance_id;
    uint p_coord_id     = vertex_id;
    uint primitive_id   = pull_vertex_id;
#elif defined(VTK_PIPELINE_LINES)
    uint pull_vertex_id = vertex_id;
    uint primitive_id   = pull_vertex_id / 2u;
#elif defined(VTK_WIDE_LINES_PIPELINE)
    uint p_coord_id   = vertex_id;
    uint primitive_id = instance_id;
#else
    uint pull_vertex_id = vertex_id;
    uint primitive_id   = pull_vertex_id / 3u;
    uint use_vertex_id  = pull_vertex_id % 3u;
#endif
#ifdef VTK_HOMOGENEOUS_CELL_SIZE
    uint cell_id = primitive_id + cell_id_offset;
#else
    uint cell_id = cell_ids[primitive_id];
#endif

#if defined(VTK_TRIANGLES_PIPELINE)
    VertexSample tri[3];
    for (uint k = 0u; k < 3u; ++k)
        tri[k] = fetchTriangleVertex(primitive_id, k, cell_id, model_view_projection);
    VertexSample self = tri[use_vertex_id];
#elif defined(VTK_WIDE_LINES_PIPELINE)
    float width = actor.render_options.line_width;
    if (width < 1.f)
        width = 1.f;
    uint p0_id = 2u * primitive_id;
    uint p1_id = p0_id + 1u;
#ifdef VTK_PIPELINE_LINES_ROUND
    float3 p_coord = kRoundLine[p_coord_id];
    bool   at_p1   = p_coord.z == 1.f;
#else
    float2 p_coord = kLineQuad[p_coord_id];
    bool   at_p1   = p_coord.x == 1.f;
#endif
    uint         pull_vertex_id = at_p1 ? p1_id : p0_id;
    VertexSample end0 = fetchVertex(p0_id, cell_id, primitive_id, model_view_projection);
    VertexSample end1 = fetchVertex(p1_id, cell_id, primitive_id, model_view_projection);
    VertexSample self = at_p1 ? end1 : end0;
#else
    VertexSample self = fetchVertex(pull_vertex_id, cell_id, primitive_id, model_view_projection);
#endif
    uint point_id = connectivity[pull_vertex_id];

    VertexContext vc = vertexContext(self);
    vc.pointId           = point_id;
    vc.normalModelToView = normal_model_view;
    hookBlock.hooks.vertex(vc);
    float4 vertexMC = vc.positionMC;

#if defined(VTK_PIPELINE_POINTS_SHAPED)
    float4 vertex_DC     = mul(model_view_projection, vertexMC);
    float2 resolution    = scene_transform.viewport.zw;
    float2 vertex_screen = toScreen(vertex_DC, resolution);
    float  point_size    = actor.render_options.point_size;
    if (point_size < 1.f)
        point_size = 1.f;
    output.p_coord  = kPointQuad[p_coord_id];
    output.position = fromScreen(vertex_screen + 0.5f * point_size * output.p_coord, vertex_DC,
                                 resolution);
#elif defined(VTK_WIDE_LINES_PIPELINE)
    float4 p0_DC      = mul(model_view_projection, float4(end0.position, 1.f));
    float4 p1_DC      = mul(model_view_projection, float4(end1.position, 1.f));
    float2 resolution = scene_transform.viewport.zw;
    float2 p0_screen  = toScreen(p0_DC, resolution);
    float2 p1_screen  = toScreen(p1_DC, resolution);
    float2 x_basis = normalize(p1_screen - p0_screen);
    float2 y_basis = float2(-x_basis.y, x_basis.x);
#ifdef VTK_PIPELINE_LINES_ROUND
    float2 adjusted_p1 = p0_screen + (p_coord.x * x_basis + p_coord.y * y_basis) * width;
    float2 adjusted_p2 = p1_screen + (p_coord.x * x_basis + p_coord.y * y_basis) * width;
    output.position    = fromScreen(lerp(adjusted_p1, adjusted_p2, p_coord.z),
                                    lerp(p0_DC, p1_DC, p_coord.z), resolution);
#else
    float2 adjusted_p1 = p0_screen + p_coord.x * x_basis + p_coord.y * y_basis * width;
    float2 adjusted_p2 = p1_screen + p_coord.x * x_basis + p_coord.y * y_basis * width;
    output.position    = fromScreen(lerp(adjusted_p1, adjusted_p2, p_coord.x),
                                    lerp(p0_DC, p1_DC, p_coord.x), resolution);
#endif
#if defined(VTK_PIPELINE_LINES_MITER) && !defined(VTK_HOMOGENEOUS_CELL_SIZE)
    bool is_polyline_rl = false;
    bool is_polyline_lr = false;
    uint cell_count, cell_stride;
    cell_ids.GetDimensions(cell_count, cell_stride);
    if (primitive_id > 0u && p_coord.x == 0.f)
        is_polyline_rl = cell_ids[primitive_id - 1u] == cell_ids[primitive_id];
    else if (primitive_id < cell_count - 1u && p_coord.x == 1.f)
        is_polyline_lr = cell_ids[primitive_id + 1u] == cell_ids[primitive_id];
    if (is_polyline_lr || is_polyline_rl) {
        uint   q0_id = is_polyline_lr ? pull_vertex_id - 1u : pull_vertex_id + 1u;
        uint   q2_id = is_polyline_lr ? pull_vertex_id + 2u : pull_vertex_id - 2u;
        float2 pos   = p_coord.x == 1.f ? float2(1.f - p_coord.x, -p_coord.y) : p_coord;
        float4 q0_DC = mul(model_view_projection,
            float4(fetchVertex(q0_id, cell_id, primitive_id, model_view_projection).position, 1.f));
        float4 q1_DC = mul(model_view_projection, vertexMC);
        float4 q2_DC = mul(model_view_projection,
            float4(fetchVertex(q2_id, cell_id, primitive_id, model_view_projection).position, 1.f));
        float2 q0_screen = toScreen(q0_DC, resolution);
        float2 q1_screen = toScreen(q1_DC, resolution);
        float2 q2_screen = toScreen(q2_DC, resolution);
        float2 tangent   = normalize(normalize(q2_screen - q1_screen) + normalize(q1_screen - q0_screen));
        float2 normal    = float2(-tangent.y, tangent.x);
        float2 q01       = q1_screen - q0_screen;
        float2 q21       = q1_screen - q2_screen;
        float2 q01_normal = normalize(float2(-q01.y, q01.x));
        float  sigma     = sign(dot(q01 + q21, normal));
        if (sign(pos.y) == sigma) {
            float2 offset = 0.5f * width * -sigma * normal / dot(normal, q01_normal);
            if (length(offset) < min(length(q01), length(q21)))
                output.position = fromScreen(q1_screen + offset, q1_DC, resolution);
        }
    }
#endif
    output.dist_to_centerline = p_coord.y;
#else
    output.position = mul(model_view_projection, vertexMC);
#endif
    output.position = projectVertex(hookBlock.hooks, vc, output.position);

#ifdef VTK_CLIPPING_PLANES_BINDING
    ClippingPlanes planes = clipping_planes[0];
    for (uint i = 0u; i < planes.count && i < 3u; i++)
        output.clip_dists_0[i % 3u] = dot(planes.plane_equations[i], vertexMC);
    for (uint i = 3u; i < planes.count; i++)
        output.clip_dists_1[i % 3u] = dot(planes.plane_equations[i], vertexMC);
#endif

    output.position_VC = mul(scene_transform.inverted_projection, output.position);

    output.ids       = uint4(cell_id, point_id, primitive_id, actor.color_options.id);
    output.group_ids = uint2(0u);

    output.color = vc.color;
    output.uv    = vc.uv;
#ifdef VTK_POINT_COLOR_UVS_BINDING
    output.lut_uv = transformTextureCoordinates(
        float2(point_color_uvs[2u * point_id], point_color_uvs[2u * point_id + 1u]));
#endif

#ifdef VTK_TRIANGLES_PIPELINE
    uint flags = actor.render_options.flags;
    if (getRepresentation(flags) == VTK_SURFACE && getEdgeVisibility(flags)) {
        float4 p0_3D_DC = mul(model_view_projection, float4(tri[0].position, 1.f));
        float4 p1_3D_DC = mul(model_view_projection, float4(tri[1].position, 1.f));
        float4 p2_3D_DC = mul(model_view_projection, float4(tri[2].position, 1.f));
        float2 p0_DC    = p0_3D_DC.xy / p0_3D_DC.w;
        float2 p1_DC    = p1_3D_DC.xy / p1_3D_DC.w;
        float2 p2_DC    = p2_3D_DC.xy / p2_3D_DC.w;
        float2 scale    = scene_transform.viewport.zw * 0.5f;
#ifdef VTK_HOMOGENEOUS_CELL_SIZE
        float edge_value = -1.f;
#else
        float edge_value = edge_array[primitive_id];
#endif
        if (use_vertex_id == 0u) {
            float2 v10   = scale * (p1_DC - p0_DC);
            float2 v20   = scale * (p2_DC - p0_DC);
            float  area0 = abs(v10.x * v20.y - v10.y * v20.x);
            float  h0    = area0 / length(v10 - v20);
            output.edge_dists = float3(h0 * p0_3D_DC.w, 0.f, 0.f);
        } else if (use_vertex_id == 1u) {
            float2 v01   = scale * (p0_DC - p1_DC);
            float2 v21   = scale * (p2_DC - p1_DC);
            float  area1 = abs(v01.x * v21.y - v01.y * v21.x);
            float  h1    = area1 / length(v01 - v21);
            output.edge_dists = float3(0.f, h1 * p1_3D_DC.w, 0.f);
        } else {
            float2 v02   = scale * (p0_DC - p2_DC);
            float2 v12   = scale * (p1_DC - p2_DC);
            float  area2 = abs(v02.x * v12.y - v02.y * v12.x);
            float  h2    = area2 / length(v02 - v12);
            output.edge_dists = float3(0.f, 0.f, h2 * p2_3D_DC.w);
        }
        output.hide_edge = edge_value;
    }
#endif

#if !defined(VTK_TRIANGLES_PIPELINE) && !defined(VTK_POINT_NORMALS_BINDING) && \
  !defined(VTK_CELL_NORMALS_BINDING)
    output.normal_VC = float3(0.f, 0.f, 1.f);
#else
    output.normal_VC = mul(normal_model_view, vc.normalMC);
#endif
    output.tangent_VC = mul(normal_model_view, vc.tangentMC);
    return output;
}

struct VtkSurface : ISurfaceSource
{
    VertexOutput vertex;
    Actor        actor;

    // Back faces of triangles without scalar colors take the backface property's material.
    ActorColorOptions colorOptions(FragmentContext f)
    {
#if defined(VTK_TRIANGLES_PIPELINE) && !defined(VTK_POINT_COLORS_BINDING) && \
  !defined(VTK_CELL_COLORS_BINDING) && !defined(VTK_POINT_COLOR_UVS_BINDING)
        if (!f.frontFacing && hasBackfaceProperty(actor.render_options.flags_2))
            return actor.backface_color_options;
#endif
        return actor.color_options;
    }

    void colors(inout FragmentContext f)
    {
        ActorColorOptions options = colorOptions(f);
        float3 ambientColor = options.ambient_color;
        float3 diffuseColor = options.diffuse_color;
        float  opacity      = 1.f;
#ifdef VTK_HAS_VERTEX_COLORS
        ambientColor = vertex.color.rgb;
        diffuseColor = vertex.color.rgb;
        opacity      = vertex.color.a;
#else
        ambientColor = ambientColor * vertex.color.rgb;
        diffuseColor = diffuseColor * vertex.color.rgb;
        opacity      = opacity * vertex.color.a;
#ifdef VTK_POINT_COLOR_TEXTURE_BINDING
        float4 lut_tex_color = point_color_texture.Sample(point_color_sampler, vertex.lut_uv);
        ambientColor = lut_tex_color.rgb;
        diffuseColor = lut_tex_color.rgb;
        opacity      = opacity * lut_tex_color.a;
#endif
#endif
#ifdef VTK_ACTOR_TEXTURE
        float4 actor_tex_color = actor_texture.Sample(actor_texture_sampler, vertex.uv);
#ifdef VTK_TRIANGLES_PIPELINE
        // Sample before branching so texture derivatives stay in uniform control flow.
        if (!f.frontFacing && !showTexturesOnBackface(actor.render_options.flags_2))
            actor_tex_color = float4(1.f);
#endif
        ambientColor = ambientColor * actor_tex_color.rgb;
        diffuseColor = diffuseColor * actor_tex_color.rgb;
        opacity      = opacity * actor_tex_color.a;
#endif
#ifdef VTK_POINTS_PIPELINE
        if (getVertexVisibility(actor.render_options.flags)) {
            ambientColor = options.vertex_color;
            diffuseColor = options.vertex_color;
            opacity      = 1.f;
        }
#endif
        f.ambientColor  = ambientColor;
        f.diffuseColor  = diffuseColor;
        f.opacity       = opacity;
        f.specularColor = options.specular_color;
        f.specularPower = options.specular_power;
    }

    override void geometry(inout FragmentContext f)
    {
#if defined(VTK_PIPELINE_POINTS_SHAPED)
        float d            = length(vertex.p_coord);
        uint  shape        = getPoint2DShape(actor.render_options.flags);
        bool  draw_spheres = getRenderPointsAsSpheres(actor.render_options.flags);
        if ((shape == POINT_2D_ROUND || draw_spheres) && d > 1.f)
            discard;
        float  point_size = clamp(actor.render_options.point_size, 1.f, 100000.f);
        float3 normalVC   = normalize(vertex.normal_VC);
        if (draw_spheres) {
            normalVC   = normalize(float3(vertex.p_coord, 1.f));
            normalVC.z = sqrt(1.f - d * d);
            float r = point_size / (scene_transform.viewport.z * scene_transform.projection[0][0]);
            if (getUseParallelProjection(scene_transform.flags)) {
                f.depth = vertex.position.z + normalVC.z * r * scene_transform.projection[2][2];
            } else {
                float sp = -scene_transform.projection[2][2];
                f.depth  = (sp - vertex.position.z) / (normalVC.z * r - 1.f) + sp;
            }
        }
#elif defined(VTK_WIDE_LINES_PIPELINE)
        float3 normalVC = normalize(vertex.normal_VC);
        if (getRenderLinesAsTubes(actor.render_options.flags)) {
            normalVC.z = 1.f - 2.f * abs(vertex.dist_to_centerline);
        } else if (normalVC.z < 0.f) {
            normalVC = normalize(-vertex.normal_VC);
        }
#elif defined(VTK_POINTS_PIPELINE) || defined(VTK_LINES_PIPELINE)
        float3 normalVC = normalize(vertex.normal_VC);
#else
        float3 normalVC = normalize(vertex.normal_VC);
        if (!f.frontFacing) {
            if (normalVC.z < 0.f)
                normalVC = normalize(-vertex.normal_VC);
        } else if (normalVC.z < 0.f) {
            normalVC.z = -normalVC.z;
        }

        uint flags = actor.render_options.flags;
        if (getRepresentation(flags) == VTK_SURFACE && getEdgeVisibility(flags)) {
            float line_width = getUseLineWidthForEdgeThickness(flags) ? actor.render_options.line_width
                                                                       : actor.render_options.edge_width;
            float3 dists = vertex.edge_dists * vertex.position.w;
            float  d;
            if (vertex.hide_edge < 0.f) {
                d = min(dists[0], min(dists[1], dists[2]));
            } else {
                uint  mask = uint(vertex.hide_edge + 0.5f);
                float dmin = 3.4e38f;
                if ((mask & 1u) != 0u) dmin = min(dmin, dists[2]);
                if ((mask & 2u) != 0u) dmin = min(dmin, dists[0]);
                if ((mask & 4u) != 0u) dmin = min(dmin, dists[1]);
                d = dmin;
            }
            float half_line_width = 0.5f * line_width;
            float I = d < half_line_width
                ? 1.f
                : exp2(-2.f * (d - half_line_width) * (d - half_line_width));
            f.diffuseColor = lerp(f.diffuseColor, actor.color_options.edge_color, I);
            f.ambientColor = lerp(f.ambientColor, actor.color_options.edge_color, I);
            if (getRenderLinesAsTubes(flags) && d < 1.1f * half_line_width)
                normalVC.z = 1.f - (d / half_line_width);
        }
#endif
        f.normalVC = normalVC;
    }

    SurfaceSample sample(FragmentContext f)
    {
        SurfaceSample s = surfaceSample(f);
        s.userData = uint4(vertex.ids.x, vertex.ids.w, vertex.group_ids.x, vertex.group_ids.y);
        s.camera.shadingToClip = scene_transform.projection;
        s.camera.parallel      = getUseParallelProjection(scene_transform.flags);
        ActorColorOptions options = colorOptions(f);
        s.host.baseColor     = options.diffuse_intensity * f.diffuseColor;
        s.host.ambientColor  = options.ambient_intensity * f.ambientColor;
        s.host.specularColor = options.specular_intensity * f.specularColor;
        s.host.opacity       = options.opacity * f.opacity;
        return s;
    }

    uint lightCount(SurfaceSample s)
    {
#ifdef VTK_UNLIT
        return 0u;
#else
        return isLightingEnabled(actor.render_options.flags_2) ? scene_lights.Load(0u) : 0u;
#endif
    }

    LightSample light(uint i, SurfaceSample s)
    {
        SceneLight  sl    = loadSceneLight(scene_lights, i);
        LightSample light = LightSample();
        light.radiance    = sl.color;
        if (sl.light_type == VTK_LIGHT_TYPE_HEADLIGHT) {
            light.direction = float3(0.f, 0.f, 1.f);
        } else if (sl.positional == 0u) {
            light.direction = -normalize(sl.direction_vc);
        } else {
            float3 lvec = normalize(sl.position_vc - s.position);
            light.direction = lvec;
            if (sl.cone_angle < 90.f) {
                float spot_cos = dot(-lvec, normalize(sl.direction_vc));
                light.radiance = spot_cos > cos(radians(sl.cone_angle))
                    ? sl.color * pow(spot_cos, sl.exponent)
                    : float3(0.f);
            }
        }
        return light;
    }

    float3 ambientRadiance(SurfaceSample s) { return float3(1.f); }

    override float depth(FragmentContext f, SurfaceSample s)
    {
#if defined(VTK_POINTS_PIPELINE)
        float c_factor = 0.f;
        float c_offset = actor.render_options.coin_point_offset;
#elif defined(VTK_LINES_PIPELINE)
        float c_factor = actor.render_options.coin_line_factor;
        float c_offset = actor.render_options.coin_line_offset;
#else
        float c_factor = actor.render_options.coin_polygon_factor;
        float c_offset = actor.render_options.coin_polygon_offset;
#endif
        float cscale = length(float2(ddx(f.depth), ddy(f.depth)));
        return f.depth + c_factor * cscale + c_offset * exp2(-float(VTK_DEPTH_BITS));
    }

    override void reject() { discard; }
};

struct FragmentOutput
{
    float4 color : SV_Target0;
    uint4  ids   : SV_Target1;
    float  depth : SV_Depth;
};

[shader("fragment")]
FragmentOutput fragmentMain(VertexOutput vertex, bool is_front_facing : SV_IsFrontFace)
{
#ifdef VTK_CLIPPING_PLANES_BINDING
    uint plane_count = clipping_planes[0].count;
    for (uint i = 0u; i < plane_count && i < 3u; i++)
        if (vertex.clip_dists_0[i % 3u] < 0.f)
            discard;
    for (uint i = 3u; i < plane_count; i++)
        if (vertex.clip_dists_1[i % 3u] < 0.f)
            discard;
#endif
    FragmentContext f = FragmentContext();
    f.positionVC  = vertex.position_VC;
    f.tangentVC   = vertex.tangent_VC;
    f.uv          = vertex.uv;
    f.fragCoord   = vertex.position;
    f.depth       = vertex.position.z;
    f.primitiveId = vertex.ids.z;
    f.cellId      = vertex.ids.x;
    f.frontFacing = is_front_facing;
#ifdef VTK_SELECT_POINTS
    f.ids = uint4(vertex.ids.y + 1u, vertex.ids.w + 1u, vertex.group_ids.x + 1u,
                  vertex.group_ids.y + 1u);
#else
    f.ids = uint4(vertex.ids.x + 1u, vertex.ids.w + 1u, vertex.group_ids.x + 1u,
                  vertex.group_ids.y + 1u);
#endif

    VtkSurface source = { vertex, actor_buffer[0] };
    shadeSurface(shading, hookBlock.hooks, source, f);
#ifdef VTK_POINT_SELECTION_DEPTH_PASS
    f.ids   = uint4(0u);
    f.depth = min(f.depth + 0.0005f, 1.f);
#endif

    FragmentOutput output;
    output.color = f.color;
    output.ids   = f.ids;
    output.depth = f.depth;
#ifdef VTK_DEEP_FRAMEBUFFER
    uint2 coord = uint2(vertex.position.xy);
    uint  pixel = coord.x + coord.y * uint(scene_transform.viewport.z);
    uint  headCount, headStride;
    abufferHeads.GetDimensions(headCount, headStride);
    if (pixel < headCount) {
        uint node = abufferCounter[0].add(1u);
        uint nodeCount, nodeStride;
        abufferNodes.GetDimensions(nodeCount, nodeStride);
        if (node < nodeCount) {
            uint previous = abufferHeads[pixel].exchange(node);
            abufferNodes[node].color       = packUnorm4x8(output.color);
            abufferNodes[node].depth       = output.depth;
            abufferNodes[node].cellId      = output.ids.x;
            abufferNodes[node].propId      = output.ids.y;
            abufferNodes[node].compositeId = output.ids.z;
            abufferNodes[node].processId   = output.ids.w;
            abufferNodes[node].next        = previous;
        }
    }
#endif
    return output;
}
