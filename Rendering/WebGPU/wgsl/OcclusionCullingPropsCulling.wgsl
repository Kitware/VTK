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

@group(0) @binding(0) var<uniform> mvpMatrix: mat4x4f;
@group(0) @binding(1) var<storage, read> inputBounds: array<Bound>;
@group(0) @binding(2) var<uniform> inputBoundsCount: u32;
@group(0) @binding(3) var hiZBuffer: texture_2d<f32>;

@group(1) @binding(0) var<storage, read_write> outputBoundsIndices: array<u32>;
@group(1) @binding(1) var<storage, read_write> outputBoundsIndicesCount: atomic<u32>;
@group(1) @binding(2) var<storage, read_write> outputBoundsIndicesCulled: array<u32>;
@group(1) @binding(3) var<storage, read_write> outputBoundsIndicesCulledCount: atomic<u32>;

/**
 * Checks whether the given bounds (modeled by 8 view space points, after
 * perspective divide) are entirely visible by the camera, partially
 * (some points of the bounds are behind the camera but not all) or
 * not at all (all the point are behind the camera)
 *
 * Returns
 *  - 0 if the bounds' points are all behind the camera
 *  - 1 if the bounds' points are all in front of the camera
 *  - 2 if the bounds' points are partially in front and behind the camera
 */
fn get_visibility_of_bounds(screenSpacePoints: array<vec3f, 8>) -> i32
{
  var allBehind: bool = true;
  var allInFront: bool = true;
  for (var i = 0; i < 8; i++)
  {
    let isPointPehind: bool = screenSpacePoints[i].z < -1 ||  screenSpacePoints[i].z > 1;

    allBehind &= isPointPehind;
    allInFront &= !isPointPehind;
  }

  if (allBehind)
  {
    return 0;
  }
  else if (allInFront)
  {
    return 1;
  }
  else
  {
    return 2;
  }
}

@compute
@workgroup_size(32, 1, 1)
fn computeMain(@builtin(global_invocation_id) id: vec3<u32>)
{
  let propIndex = id.x;
  if (propIndex >= inputBoundsCount)
  {
    return;
  }

  let propBounds = inputBounds[propIndex];

  var boundVertices: array<vec4<f32>, 8>;
  boundVertices[0] = vec4<f32>(propBounds.xMin, propBounds.yMin, propBounds.zMin, 1.0f);
  boundVertices[1] = vec4<f32>(propBounds.xMin, propBounds.yMin, propBounds.zMax, 1.0f);
  boundVertices[2] = vec4<f32>(propBounds.xMin, propBounds.yMax, propBounds.zMin, 1.0f);
  boundVertices[3] = vec4<f32>(propBounds.xMin, propBounds.yMax, propBounds.zMax, 1.0f);
  boundVertices[4] = vec4<f32>(propBounds.xMax, propBounds.yMin, propBounds.zMin, 1.0f);
  boundVertices[5] = vec4<f32>(propBounds.xMax, propBounds.yMin, propBounds.zMax, 1.0f);
  boundVertices[6] = vec4<f32>(propBounds.xMax, propBounds.yMax, propBounds.zMin, 1.0f);
  boundVertices[7] = vec4<f32>(propBounds.xMax, propBounds.yMax, propBounds.zMax, 1.0f);

  // Transforming the 8 corners into clip space
  let cornersClipSpace = array<vec4<f32>, 8>(
    mvpMatrix * boundVertices[0],
    mvpMatrix * boundVertices[1],
    mvpMatrix * boundVertices[2],
    mvpMatrix * boundVertices[3],
    mvpMatrix * boundVertices[4],
    mvpMatrix * boundVertices[5],
    mvpMatrix * boundVertices[6],
    mvpMatrix * boundVertices[7]
  );

  // Perspective divide
  let cornersScreenSpace = array<vec3<f32>, 8>(
    cornersClipSpace[0].xyz / cornersClipSpace[0].w,
    cornersClipSpace[1].xyz / cornersClipSpace[1].w,
    cornersClipSpace[2].xyz / cornersClipSpace[2].w,
    cornersClipSpace[3].xyz / cornersClipSpace[3].w,
    cornersClipSpace[4].xyz / cornersClipSpace[4].w,
    cornersClipSpace[5].xyz / cornersClipSpace[5].w,
    cornersClipSpace[6].xyz / cornersClipSpace[6].w,
    cornersClipSpace[7].xyz / cornersClipSpace[7].w
  );

  var screenSpaceMin = cornersScreenSpace[0];
  var screenSpaceMax = cornersScreenSpace[0];

  let visiblity = get_visibility_of_bounds(cornersScreenSpace);
  if (visiblity == 0)
  {
    // All points of the bounding box are behind, not rendering the object

    let notPassedIndex = atomicAdd(&outputBoundsIndicesCulledCount, 1);
    outputBoundsIndicesCulled[notPassedIndex] = propIndex;

    return;
  }
  else if (visiblity == 1)
  {
    // All points of the bounding box are in front, we have to compute the min and
    // max of the bounding box of the object

    // Finding the minimum and maximum of the screen space bounding box of the prop
    // Starting at 1 because the min and max are initialized to [0]
    for (var i = 1; i < 8; i++)
    {
      screenSpaceMin = min(screenSpaceMin, cornersScreenSpace[i]);
      screenSpaceMax = max(screenSpaceMax, cornersScreenSpace[i]);
    }

    // Only scaling XY * 0.5f + 0.5f because the depth is already in [0, 1] in Vulkan
    screenSpaceMin = screenSpaceMin * vec3f(0.5f, 0.5f, 0.5f);
    screenSpaceMin = screenSpaceMin + vec3f(0.5f, 0.5f, 0.5f);
    screenSpaceMax = screenSpaceMax * vec3f(0.5f, 0.5f, 0.5f);
    screenSpaceMax = screenSpaceMax + vec3f(0.5f, 0.5f, 0.5f);

    screenSpaceMin = clamp(screenSpaceMin, vec3f(0.0f), vec3f(1.0f));
    screenSpaceMax = clamp(screenSpaceMax, vec3f(0.0f), vec3f(1.0f));

    // Reversing because the MVP matrix is made for OpenGL and it flips the Y
    // compared to how our textures are read by Vulkan (only applicable for the Vulkan backend).
    // We want (0, 0) top left corner.
    screenSpaceMin.y = 1.0f - screenSpaceMin.y;
    screenSpaceMax.y = 1.0f - screenSpaceMax.y;

    // Reversing min.y and max.y to keep a logic of max being numerically bigger than min
    // (which wouldn't be the case without this reversing since we had to flip the Y above
    // in the code with 1.0f - screenSpaceMin.y and 1.0f - screenSpaceMax.y)
    let temp = screenSpaceMin.y;
    screenSpaceMin.y = screenSpaceMax.y;
    screenSpaceMax.y = temp;
  }
  else if (visiblity == 2)
  {
    // Partially visible (some points of the bounds are behind the camera,
    // other are in front), we're going to assume that the bounding
    // box of the object spans the whole screen.

    // Z of 0.0f because the object is overlapping the camera so it is
    // litteraly on the screen
    screenSpaceMin = vec3f(0.0f, 0.0f, 0.0f);
    screenSpaceMax = vec3f(1.0f, 1.0f, 0.0f);
  }

  // Minus (1, 1) to avoid being outside of the texture when screenSpaceMin/Max .x or .y is 1
  let mip0Dims = textureDimensions(hiZBuffer, 0);
  var viewportMin = screenSpaceMin.xy * vec2f(mip0Dims);
  var viewportMax = screenSpaceMax.xy * vec2f(mip0Dims);

  // + 1 here because the max is included. If the projected min and max of the bbox is
  // 11 and 12 respectively for example, that's 2 pixels wide, not 1 so we +1
  var bboxWidth = (viewportMax.x - viewportMin.x) + 1;
  var bboxHeight = (viewportMax.y - viewportMin.y) + 1;
  bboxWidth = min(bboxWidth, f32(mip0Dims.x));
  bboxHeight = min(bboxHeight, f32(mip0Dims.y));

  let largestExtent = max(bboxWidth, bboxHeight);
  // Choosing a mip level so that the screen space bbox spans 2x2 pixel on the depth
  // buffer mipmap
  let mipMapLevel = i32(ceil(log2(largestExtent / 2.0f)));
  let scaleDownFactor = pow(2.0f, f32(mipMapLevel));
  let scaledDownDims = vec2f(floor(vec2f(mip0Dims) / scaleDownFactor));

  let minScaled = vec2i(floor(screenSpaceMin.xy * scaledDownDims));
  let maxScaled = vec2i(ceil(screenSpaceMax.xy * scaledDownDims));

  let mipMapDims = vec2i(textureDimensions(hiZBuffer, mipMapLevel));
  var onePixelVisible: bool = false;

  // We want at least a 1x1 bbox
  let upperX = max(maxScaled.x, minScaled.x + 1);
  let upperY = max(maxScaled.y, minScaled.y + 1);

  // For loop needed here because we don't know what size the
  // bounding box is going to be exactly
  for (var y = minScaled.y; y < upperY; y++)
  {
    for (var x = minScaled.x; x < upperX; x++)
    {
      let depth = textureLoad(hiZBuffer, vec2i(x, y), mipMapLevel).r;
      if (x >= mipMapDims.x || y >= mipMapDims.x || x < 0 || y < 0)
      {
        // If some pixels of the screen space rectangle (bounds) of the object
        // are out of the viewport, we're going to conservatively assume the
        // object is visible since we cannot cleanly test the depth (out of the viewport)
        onePixelVisible = true;

        break;
      }

      onePixelVisible |= depth >= screenSpaceMin.z;

      if (onePixelVisible)
      {
        break;
      }
    }

    if (onePixelVisible)
    {
      break;
    }
  }

  if (onePixelVisible)
  {
    // Passed the culling test

    let passedIndex = atomicAdd(&outputBoundsIndicesCount, 1);
    outputBoundsIndices[passedIndex] = propIndex;
  }
  else
  {
    // Culled

    let notPassedIndex = atomicAdd(&outputBoundsIndicesCulledCount, 1);
    outputBoundsIndicesCulled[notPassedIndex] = propIndex;
  }
}
