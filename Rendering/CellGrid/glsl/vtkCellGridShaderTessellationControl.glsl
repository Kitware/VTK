//VTK::System::Dec
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

//VTK::Camera::Dec

layout(vertices = {PatchSize}) out;

flat in int cellIdVSOutput[];
flat in int sideIdVSOutput[];
flat in int instanceIdVSOutput[];
smooth in vec3 pcoordVSOutput[];

patch out int cellIdTCSOutput;
patch out int sideIdTCSOutput;
patch out int instanceIdTCSOutput;
out vec3 pcoordTCSOutput[];

uniform ivec2 tessellation_levels_range;
uniform float max_distance;

#if {PatchSize} == 2
void prepareTessellationLevels()
{{
  // ----------------------------------------------------------------------
  int min_tl = tessellation_levels_range[0];
  int max_tl = tessellation_levels_range[1];

  // ----------------------------------------------------------------------
  // Step 1: transform each vertex into eye space
  vec4 eyeSpacePos0 = MCVCMatrix * gl_in[0].gl_Position;
  vec4 eyeSpacePos1 = MCVCMatrix * gl_in[1].gl_Position;

  // ----------------------------------------------------------------------
  // Step 2: "distance" from camera scaled between 0 and 1
  float distance0 = clamp((abs(eyeSpacePos0.z)) / (max_distance), 0.0f, 1.0f);
  float distance1 = clamp((abs(eyeSpacePos1.z)) / (max_distance), 0.0f, 1.0f);

  // ----------------------------------------------------------------------
  // Step 3: interpolate edge tessellation level based on closer vertex
  float tessLevel0 = mix(max_tl, min_tl, min(distance1, distance0));

  // ----------------------------------------------------------------------
  // Step 4: set the corresponding outer edge tessellation levels
  gl_TessLevelOuter[0] = 1;
  gl_TessLevelOuter[1] = tessLevel0;
}}
#elif {PatchSize} == 3
void prepareTessellationLevels()
{{
  // ----------------------------------------------------------------------
  int min_tl = tessellation_levels_range[0];
  int max_tl = tessellation_levels_range[1];

  // ----------------------------------------------------------------------
  // Step 1: transform each vertex into eye space
  vec4 eyeSpacePos00 = MCVCMatrix * gl_in[0].gl_Position;
  vec4 eyeSpacePos01 = MCVCMatrix * gl_in[1].gl_Position;
  vec4 eyeSpacePos11 = MCVCMatrix * gl_in[2].gl_Position;

  // ----------------------------------------------------------------------
  // Step 2: "distance" from camera scaled between 0 and 1
  float distance00 = clamp((abs(eyeSpacePos00.z)) / (max_distance), 0.0f, 1.0f);
  float distance01 = clamp((abs(eyeSpacePos01.z)) / (max_distance), 0.0f, 1.0f);
  float distance11 = clamp((abs(eyeSpacePos11.z)) / (max_distance), 0.0f, 1.0f);

  // ----------------------------------------------------------------------
  // Step 3: interpolate edge tessellation level based on closer vertex
  float tessLevel0 = mix(max_tl, min_tl, min(distance11, distance00));
  float tessLevel1 = mix(max_tl, min_tl, min(distance00, distance01));
  float tessLevel2 = mix(max_tl, min_tl, min(distance01, distance11));

  // ----------------------------------------------------------------------
  // Step 4: set the corresponding outer edge tessellation levels
  gl_TessLevelOuter[0] = tessLevel0;
  gl_TessLevelOuter[1] = tessLevel1;
  gl_TessLevelOuter[2] = tessLevel2;

  // ----------------------------------------------------------------------
  // Step 5: set the inner tessellation levels to the max of the three edges
  gl_TessLevelInner[0] = max(tessLevel0, max(tessLevel1, tessLevel2));
}}
#elif {PatchSize} == 4
void prepareTessellationLevels()
{{
  // ----------------------------------------------------------------------
  int min_tl = tessellation_levels_range[0];
  int max_tl = tessellation_levels_range[1];

  // ----------------------------------------------------------------------
  // Step 1: transform each vertex into eye space
  vec4 eyeSpacePos00 = MCVCMatrix * gl_in[0].gl_Position;
  vec4 eyeSpacePos01 = MCVCMatrix * gl_in[1].gl_Position;
  vec4 eyeSpacePos10 = MCVCMatrix * gl_in[2].gl_Position;
  vec4 eyeSpacePos11 = MCVCMatrix * gl_in[3].gl_Position;

  // ----------------------------------------------------------------------
  // Step 2: "distance" from camera scaled between 0 and 1
  float distance00 = clamp((abs(eyeSpacePos00.z)) / (max_distance), 0.0f, 1.0f);
  float distance01 = clamp((abs(eyeSpacePos01.z)) / (max_distance), 0.0f, 1.0f);
  float distance10 = clamp((abs(eyeSpacePos10.z)) / (max_distance), 0.0f, 1.0f);
  float distance11 = clamp((abs(eyeSpacePos11.z)) / (max_distance), 0.0f, 1.0f);

  // ----------------------------------------------------------------------
  // Step 3: interpolate edge tessellation level based on closer vertex
  float tessLevel0 = mix(max_tl, min_tl, min(distance10, distance00));
  float tessLevel1 = mix(max_tl, min_tl, min(distance00, distance01));
  float tessLevel2 = mix(max_tl, min_tl, min(distance01, distance11));
  float tessLevel3 = mix(max_tl, min_tl, min(distance11, distance10));

  // ----------------------------------------------------------------------
  // Step 4: set the corresponding outer edge tessellation levels
  gl_TessLevelOuter[0] = tessLevel0;
  gl_TessLevelOuter[1] = tessLevel1;
  gl_TessLevelOuter[2] = tessLevel2;
  gl_TessLevelOuter[3] = tessLevel3;

  // ----------------------------------------------------------------------
  // Step 5: set the inner tessellation levels to the max of the two parallel edges
  gl_TessLevelInner[0] = max(tessLevel1, tessLevel3);
  gl_TessLevelInner[1] = max(tessLevel0, tessLevel2);
}}
#endif

void main()
{{
  if (gl_InvocationID == 0)
  {{
    prepareTessellationLevels();

    // It's okay to copy only the first value, because all patch vertices come from the same cell/side
    cellIdTCSOutput = cellIdVSOutput[0];
    sideIdTCSOutput = sideIdVSOutput[0];
  }}

  instanceIdTCSOutput = instanceIdVSOutput[gl_InvocationID];
  gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
  pcoordTCSOutput[gl_InvocationID] = pcoordVSOutput[gl_InvocationID];
}}
