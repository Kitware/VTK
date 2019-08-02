//VTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Template for the polydata mappers fragment shader

uniform mat4 VCDCMatrix;

uniform sampler2D fluidZTexture;
uniform int       viewportWidth;
uniform int       viewportHeight;

in vec2 texCoord;
// the output of this shader
//VTK::Output::Dec

// TODO: May rewrite this to use the proper near and far values from projection matrix
// Near and far plane of the frustum
const float far  = 1000.0f;
const float near = 0.1f;
vec3 uvToEye(vec2 texCoordVal, float depth) {
    float x  = texCoordVal.x * 2.0 - 1.0;
    float y  = texCoordVal.y * 2.0 - 1.0;
    float zn = ((far + near) / (far - near) * depth + 2 * far * near / (far - near)) / depth;

    vec4 clipPos = vec4(x, y, zn, 1.0f);
    vec4 viewPos = inverse(VCDCMatrix) * clipPos;
    return viewPos.xyz / viewPos.w;
}

void main() {
    float x     = texCoord.x;
    float y     = texCoord.y;
    float depth = texture(fluidZTexture, vec2(x, y)).r;

    if(depth < -999.0f) {
        gl_FragData[0] = vec4(0, 1, 0, 1);
        return;
    }

    float pixelWidth  = 1.0 / float(viewportWidth);
    float pixelHeight = 1.0 / float(viewportHeight);
    float xp          = texCoord.x + pixelWidth;
    float xn          = texCoord.x - pixelWidth;
    float yp          = texCoord.y + pixelHeight;
    float yn          = texCoord.y - pixelHeight;

    float depthxp = texture(fluidZTexture, vec2(xp, y)).r;
    float depthxn = texture(fluidZTexture, vec2(xn, y)).r;
    float depthyp = texture(fluidZTexture, vec2(x, yp)).r;
    float depthyn = texture(fluidZTexture, vec2(x, yn)).r;

    vec3 position   = uvToEye(vec2(x, y), depth);
    vec3 positionxp = uvToEye(vec2(xp, y), depthxp);
    vec3 positionxn = uvToEye(vec2(xn, y), depthxn);
    vec3 dxl        = position - positionxn;
    vec3 dxr        = positionxp - position;

    vec3 dx = (abs(dxr.z) < abs(dxl.z)) ? dxr : dxl;

    vec3 positionyp = uvToEye(vec2(x, yp), depthyp);
    vec3 positionyn = uvToEye(vec2(x, yn), depthyn);
    vec3 dyb        = position - positionyn;
    vec3 dyt        = positionyp - position;

    vec3 dy = (abs(dyt.z) < abs(dyb.z)) ? dyt : dyb;

    vec3 N = normalize(cross(dx, dy));
    if(isnan(N.x) || isnan(N.y) || isnan(N.y) ||
       isinf(N.x) || isinf(N.y) || isinf(N.z)) {
        N = vec3(0, 0, 1);
    }

    gl_FragData[0] = vec4(N, 1);
}
