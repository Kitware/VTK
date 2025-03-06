// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

struct Bound
{
  xMin: f32,
  xMax: f32,
  yMin: f32,
  yMax: f32,
  zMin: f32,
  zMax: f32
};

struct CameraPlane
{
  // The four A, B, C, D of the plane equation (Ax+By+Cz+D=0)
  coeffs: array<f32, 4>
};

// Bounds of the object to cull in world space
@group(0) @binding(0) var<storage, read> inputBounds: array<Bound>;
// Output list of the indices non culled bounds
@group(0) @binding(1) var<storage, read_write> outputIndices: array<u32>;
// How many bounds were not culled
@group(0) @binding(2) var<storage, read_write> outputCount: atomic<u32>;
// View-projection matrix of the camera whose frustum is going to cull the objects
@group(0) @binding(3) var<uniform> viewProjectionMatrix: mat4x4f;

@compute @workgroup_size(32, 1, 1)
fn frustumCullingEntryPoint(@builtin(global_invocation_id) id: vec3<u32>)
{
  let objectIndex = id.x;
  // Thread bounds checking
  if (objectIndex >= arrayLength(&inputBounds))
  {
    return;
  }

  let bound = inputBounds[objectIndex];
  // Reconstructing the 8 vertices of the bounding box
  var boundVertices: array<vec4<f32>, 8>;
  boundVertices[0] = vec4<f32>(bound.xMin, bound.yMin, bound.zMin, 1.0f);
  boundVertices[1] = vec4<f32>(bound.xMin, bound.yMin, bound.zMax, 1.0f);
  boundVertices[2] = vec4<f32>(bound.xMin, bound.yMax, bound.zMin, 1.0f);
  boundVertices[3] = vec4<f32>(bound.xMin, bound.yMax, bound.zMax, 1.0f);
  boundVertices[4] = vec4<f32>(bound.xMax, bound.yMin, bound.zMin, 1.0f);
  boundVertices[5] = vec4<f32>(bound.xMax, bound.yMin, bound.zMax, 1.0f);
  boundVertices[6] = vec4<f32>(bound.xMax, bound.yMax, bound.zMin, 1.0f);
  boundVertices[7] = vec4<f32>(bound.xMax, bound.yMax, bound.zMax, 1.0f);

  // Transforming the 8 corners into clip space
  let cornersCS = array<vec4<f32>, 8>(
    viewProjectionMatrix * boundVertices[0],
    viewProjectionMatrix * boundVertices[1],
    viewProjectionMatrix * boundVertices[2],
    viewProjectionMatrix * boundVertices[3],
    viewProjectionMatrix * boundVertices[4],
    viewProjectionMatrix * boundVertices[5],
    viewProjectionMatrix * boundVertices[6],
    viewProjectionMatrix * boundVertices[7]
  );

  // For each frustum plane (6), we're going to check whether the bounding is completely
  // outside (outside means on the left of the left plane of the frustum, or below the bottom
  // plane of the frustum, etc ...) of it or not.
  var needsCulling = false;
  for (var coordinateIndex = 0; coordinateIndex < 6; coordinateIndex++)
  {
    // Is there at least one corner inside the view volume (meaning that this view
    // frustum plane doesn't cull the object)?
    var oneInside = false;
    for (var cornerIndex = 0; cornerIndex < 8; cornerIndex++)
    {
      // Note that this big switch case does not cause thread divergence.
      // The for loop on coordinate index is executed by all threads at the same
      // time. No threads will have a different value of coordinateIndex which means
      // no divergence in the switch.
      switch (coordinateIndex)
      {
          case 0:
          {
            // inside -x plane?
            oneInside |= cornersCS[cornerIndex][0] > -cornersCS[cornerIndex][3];
            break;
          }

          case 1:
          {
            // inside +x plane?
            oneInside |= cornersCS[cornerIndex][0] < cornersCS[cornerIndex][3];
            break;
          }

          case 2:
          {
            // inside -y plane?
            oneInside |= cornersCS[cornerIndex][1] > -cornersCS[cornerIndex][3];
            break;
          }

          case 3:
          {
            // inside +y plane?
            oneInside |= cornersCS[cornerIndex][1] < cornersCS[cornerIndex][3];
            break;
          }

          case 4:
          {
            // inside near plane? We're using [0, 1] for the z range in clip space
            // (we chose that when getting the projection matrix from the vtkCamera)
            oneInside |= cornersCS[cornerIndex][2] > 0;
            break;
          }

          case 5:
          {
            // inside far plane?
            oneInside |= cornersCS[cornerIndex][2] < cornersCS[cornerIndex][3];
            break;
          }

          default:
          {
            // Default block required by WGSL syntax
            break;
          }
      }

      if (oneInside)
      {
        // We found one corner inside the view volume, this frustum view plane
        // does not cull the object. Breaking to skip to the next frustum plane to test

        break;
      }
    }

    // If we found that none of the corners of the bounding box are inside the view volume,
    // this means that we just found a plane of the view frustum that separates the object from
    // the view frustum and the object can be culled
    if (!oneInside)
    {
      needsCulling = true;

      break;
    }
  }

  if (!needsCulling)
  {
    // Adding one to the number of objects that passed the test.
    // atomicAdd returns the value of the variable before it is incremented.
    // This returned value is going to be our index into the buffer of prop indices
    // that passed the culling test
    let outputIndex = atomicAdd(&outputCount, 1);
    outputIndices[outputIndex] = objectIndex;
  }
}
