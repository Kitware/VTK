// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

@group(0) @binding(0) var<storage, read> pointBuffer: array<f32>;
@group(0) @binding(1) var<storage, read_write> pointDepthBuffer: array<atomic<u32>>;
// The color of a point is encoded as RGBA representation, 8 bits per channel, packed in a single u32.
// R, G, B and A can then be individually unpacked by getting each 8 bits individually
@group(0) @binding(2) var<storage, read> pointColorBuffer: array<u32>;
@group(0) @binding(3) var currentFrameBuffer: texture_storage_2d<bgra8unorm, write>;
@group(0) @binding(4) var<uniform> mvpMatrix: mat4x4f;

/**
 * Returns true if the given point is in the frustum of the camera, false otherwise.
 *
 * This avoids rasterizing points that are behind the camera (but that would be
 * projected on the viewport anyways) and this also increases performance by not
 * rendering points that are not visible.
 */
fn pointInFrustum(clipSpacePoint: vec4f) -> bool
{
  let xCheck = clipSpacePoint.x >= -clipSpacePoint.w && clipSpacePoint.x <= clipSpacePoint.w;
  let yCheck = clipSpacePoint.y >= -clipSpacePoint.w && clipSpacePoint.y <= clipSpacePoint.w;
  let zCheck = clipSpacePoint.z >= -clipSpacePoint.w && clipSpacePoint.z <= clipSpacePoint.w;

  return xCheck && yCheck && zCheck;
}

@compute
@workgroup_size(256, 1, 1)
fn pointCloudRenderEntryPoint(@builtin(global_invocation_id) id: vec3<u32>, @builtin(num_workgroups) num_workgroups: vec3<u32>)
{
  let framebufferDims = textureDimensions(currentFrameBuffer);

  var pointIndex = u32(id.x);

  // While loop so that threads render more than one point if necessary
  while (pointIndex * 3 < arrayLength(&pointBuffer))
  {
    var point = vec4f(pointBuffer[pointIndex * 3 + 0], pointBuffer[pointIndex * 3 + 1], pointBuffer[pointIndex * 3 + 2], 1.0f);

    var projectedPoint = mvpMatrix * point;

    if (pointInFrustum(projectedPoint))
    {
      projectedPoint.x /= projectedPoint.w;
      projectedPoint.y /= projectedPoint.w;
      projectedPoint.z /= projectedPoint.w;

      // We are going to want to index the point in the 'pointDepthBuffer' buffer.
      // This requires converting the pixel coordinates of the point into a linear
      // index for indexing the buffer. After the projection by the camera matrix
      // (and after perspective divide), the point is in NDC space in [-1, 1].
      // We bring it back to [0, 1] to be able to compute the pixel coordinates easily
      projectedPoint += vec4f(1.0f, 1.0f, 1.0f, 0.0f);
      projectedPoint *= vec4f(0.5f, 0.5f, 0.5f, 1.0f);

      projectedPoint.x *= f32(framebufferDims.x);
      projectedPoint.y *= f32(framebufferDims.y);

      let pixelIndex = u32(projectedPoint.x) + u32(projectedPoint.y) * framebufferDims.x;
      let pixelIndexForDepthBuffer = u32(projectedPoint.x) + (framebufferDims.y - u32(projectedPoint.y)) * framebufferDims.x;
      let quantizedDepth = u32(projectedPoint.z * pow(2.0, 32));
      atomicMin(&pointDepthBuffer[pixelIndexForDepthBuffer], quantizedDepth);

      if (atomicLoad(&pointDepthBuffer[pixelIndexForDepthBuffer]) == quantizedDepth)
      {
        // Only storing if we are the closest point

        // Reversing the y-coordinate on storing here because we're using VTK's projection
        // matrix which is OpenGL which has a y-up NDC space but Vulkan (and Metal/DX12)
        // uses Y-down
        let storeCoords = vec2u(u32(projectedPoint.x), framebufferDims.y - u32(projectedPoint.y));

        var color = vec4f(0.8f, 0.8f, 0.8f, 1.0f);
        if (pointIndex < arrayLength(&pointColorBuffer))
        {
          // Unpacking the one u32 point color to independent RGBA channels
          let r = f32((pointColorBuffer[pointIndex] & u32(0xFF <<  0)) >>  0) / 255.0f;
          let g = f32((pointColorBuffer[pointIndex] & u32(0xFF <<  8)) >>  8) / 255.0f;
          let b = f32((pointColorBuffer[pointIndex] & u32(0xFF << 16)) >> 16) / 255.0f;
          let a = f32((pointColorBuffer[pointIndex] & u32(0xFF << 24)) >> 24) / 255.0f;

          color = vec4f(r, g, b, a);
        }

        textureStore(currentFrameBuffer, storeCoords, color);
      }
    }

    // Offsetting the pointIndex in the pointBuffer to render the next point
    pointIndex += num_workgroups.x * 256;
  }
}
