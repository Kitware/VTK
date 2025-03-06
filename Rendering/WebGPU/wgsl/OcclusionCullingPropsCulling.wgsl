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
@group(0) @binding(3) var hierarchicalZBuffer: texture_2d<f32>;

@group(1) @binding(0) var<storage, read_write> outputBoundsIndices: array<u32>;
@group(1) @binding(1) var<storage, read_write> outputBoundsIndicesCount: atomic<u32>;
@group(1) @binding(2) var<storage, read_write> outputBoundsIndicesCulled: array<u32>;
@group(1) @binding(3) var<storage, read_write> outputBoundsIndicesCulledCount: atomic<u32>;

/**
 * Checks whether the given bounds (modeled by 8 clip space points i.e. before
 * perspective divide) overlap the camera or not, meaning that some corners
 * of the bounds are in front of the camera while other corners are behind the camera
 *
 * Returns true if some corners are in front while other are behind
 * Returns false if corners are all behind or all in front
 */
fn IsOverlappingCamera(boundsCornersClipSpace: array<vec4<f32>, 8>) -> bool
{
  var allInFrontOfCamera: bool = true;
  var allBehindCamera: bool = true;

  for (var i = 0; i < 8; i++)
  {
    // Testing if the point is behind the near plane in clip space
    let isPointPehind: bool = boundsCornersClipSpace[i].z < -boundsCornersClipSpace[i].w;

    allBehindCamera &= isPointPehind;
    allInFrontOfCamera &= !isPointPehind;
  }

  // If the points are not ALL in front and not ALL behind, this means
  // that we have both cases which means overlapping
  return !allBehindCamera && !allInFrontOfCamera;
}

/**
 * Tests the given clip space bounds-corners for frustum culling.
 *
 * Returns true if the given bounds are not within the view frustum (and the bounds should be culled)
 * Returns false if the given bounds are visible
 */
fn IsFrustumCulled(boundsCornersClipSpace: array<vec4<f32>, 8>) -> bool
{
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
            oneInside |= boundsCornersClipSpace[cornerIndex][0] > -boundsCornersClipSpace[cornerIndex][3];
            break;
          }

          case 1:
          {
            // inside +x plane?
            oneInside |= boundsCornersClipSpace[cornerIndex][0] < boundsCornersClipSpace[cornerIndex][3];
            break;
          }

          case 2:
          {
            // inside -y plane?
            oneInside |= boundsCornersClipSpace[cornerIndex][1] > -boundsCornersClipSpace[cornerIndex][3];
            break;
          }

          case 3:
          {
            // inside +y plane?
            oneInside |= boundsCornersClipSpace[cornerIndex][1] < boundsCornersClipSpace[cornerIndex][3];
            break;
          }

          case 4:
          {
            // inside near plane?
            let inFrontOfNearPlane = boundsCornersClipSpace[cornerIndex][2] > -boundsCornersClipSpace[cornerIndex][3];

            oneInside |= inFrontOfNearPlane;
            break;
          }

          case 5:
          {
            // inside far plane?
            oneInside |= boundsCornersClipSpace[cornerIndex][2] < boundsCornersClipSpace[cornerIndex][3];
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

  return needsCulling;
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

  let isFrustumCulled = IsFrustumCulled(cornersClipSpace);
  if (isFrustumCulled)
  {
    // All points of the bounding box are behind or not inside the view
    // frustum, not rendering the object

    let notPassedIndex = atomicAdd(&outputBoundsIndicesCulledCount, 1);
    outputBoundsIndicesCulled[notPassedIndex] = propIndex;

    return;
  }
  else
  {
    let isOverlapping = IsOverlappingCamera(cornersClipSpace);

    if (isOverlapping)
    {
      // Partially visible (some points of the bounds are behind the camera,
      // other are in front), we're going to assume that the bounding
      // box of the object spans the whole screen.

      // Z of 0.0f because the object is overlapping the camera so it is
      // litteraly on the screen
      screenSpaceMin = vec3f(0.0f, 0.0f, 0.0f);
      screenSpaceMax = vec3f(1.0f, 1.0f, 0.0f);
    }
    else
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
  }

  // Minus (1, 1) to avoid being outside of the texture when screenSpaceMin/Max .x or .y is 1
  let mip0Dims = textureDimensions(hierarchicalZBuffer, 0);
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

  let mipMapDims = vec2i(textureDimensions(hierarchicalZBuffer, mipMapLevel));
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
      let depth = textureLoad(hierarchicalZBuffer, vec2i(x, y), mipMapLevel).r;
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
