//VTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

in vec2 texCoord;

uniform mat4 VCDCMatrix;
uniform mat4 MCVCMatrix;

uniform sampler2D fluidZTexture;
uniform sampler2D fluidThicknessTexture;
uniform sampler2D fluidNormalTexture;
uniform sampler2D fluidColorTexture;

uniform sampler2D opaqueRGBATexture;
uniform sampler2D opaqueZTexture;

uniform int displayModeOpaqueSurface = 0;
uniform int displayModeSurfaceNormal = 0;
uniform int hasVertexColor           = 0;
uniform float vertexColorPower = 0.1f;
uniform float vertexColorScale = 1.0f;

uniform float refractionScale      = 1.0f;
uniform float attennuationScale    = 1.0f;
uniform float additionalReflection = 0.1f;
uniform float refractiveIndex      = 1.33f;

uniform vec3 fluidOpaqueColor       = vec3(0, 0, 0.95);
uniform vec3 fluidAttennuationColor = vec3(0.5, 0.2, 0.05);

// Texture maps
//VTK::TMap::Dec


// the output of this shader
//VTK::Output::Dec





// TODO:
// TODO:
// TODO:
// TODO:
// TODO: this is a fixed light position
// Need to get prpoer light information
const vec4  lightPos           = vec4(-10, 10, 0, 1);
const vec3  ambientColor       = vec3(0.1, 0.1, 0.1);

// These are fluid material, can be changed by user
const vec3  fluidSpecularColor = vec3(1, 1, 1);
const float fluidShininess     = 150.0f;








// This should not be changed
const float fresnelPower = 5.0f;

// Near and far plane of the frustum
// These are fixed value, can be changed to the correct value based on the projection matrix
const float farZ  = 1000.0f;
const float nearZ = 0.1f;
vec3 uvToEye(float eyeDepth) {
    float x  = texCoord.x * 2.0 - 1.0;
    float y  = texCoord.y * 2.0 - 1.0;
    float zn = ((farZ + nearZ) / (farZ - nearZ) * eyeDepth + 2 * farZ * nearZ / (farZ - nearZ)) / eyeDepth;

    vec4 clipPos = vec4(x, y, zn, 1.0f);
    vec4 viewPos = inverse(VCDCMatrix) * clipPos;
    return viewPos.xyz / viewPos.w;
}

vec3 computeAttennuation(float thickness) {
    return vec3(exp(-fluidAttennuationColor.r * thickness),
                exp(-fluidAttennuationColor.g * thickness),
                exp(-fluidAttennuationColor.b * thickness));
}

void main() {
    float fdepth = texture(fluidZTexture, texCoord).r;

    float odepth = texture(opaqueZTexture, texCoord).r;
    if(fdepth < -999.0f || fdepth >= 0) { discard; }

    gl_FragDepth = fdepth;
    vec3  N      = texture(fluidNormalTexture, texCoord).xyz;
    if(displayModeSurfaceNormal == 1) {
        gl_FragData[0] = vec4(N, 1);
        return;
    }





    // TODO:
    // TODO:
    // TODO:
    // TODO:
    // TODO: This block use a fixed light
    // TODO: Modify this block to use proper lights
    vec3  position = uvToEye(fdepth);
    vec3  viewer   = normalize(-position.xyz);
    vec3  lightDir = normalize(vec3(MCVCMatrix * lightPos) - position);
    vec3  H        = normalize(lightDir + viewer);
    float specular = pow(max(0.0f, dot(H, N)), fluidShininess);
    float diffuse  = max(0.0f, dot(lightDir, N));

    if(displayModeOpaqueSurface == 1) {
        if(hasVertexColor == 0) {
            gl_FragData[0] = vec4(ambientColor + fluidOpaqueColor * diffuse + fluidSpecularColor * specular, 1.0);
        } else {
            vec3 tmp = texture(fluidColorTexture, texCoord).xyz;
            tmp.r = 1.0 - pow(tmp.r, vertexColorPower) * vertexColorScale;
            tmp.g = 1.0 - pow(tmp.g, vertexColorPower) * vertexColorScale;
            tmp.b = 1.0 - pow(tmp.b, vertexColorPower) * vertexColorScale;
            gl_FragData[0] = vec4(ambientColor + tmp * diffuse + fluidSpecularColor * specular, 1.0);
        }
        return;
    }

    float eta = 1.0 / refractiveIndex;          // Ratio of indices of refraction
    float F   = ((1.0 - eta) * (1.0 - eta)) / ((1.0 + eta) * (1.0 + eta));

    //Fresnel Reflection
    float fresnelRatio  = clamp(F + (1.0 - F) * pow((1.0 - dot(viewer, N)), fresnelPower), 0, 1);
    vec3  reflectionDir = reflect(-viewer, N);







    // TODO:
    // TODO:
    // TODO:
    // TODO:
    // TODO:
    // TODO: This is fixed reflection color, must be changed to sample from environment texture
    vec3 reflectionColor = vec3(1, 1, 1);

    // Should be this:
    // vec3  reflectionColor = texture(cubeMapTexture, reflectionDir).xyz;

    float fthick = texture(fluidThicknessTexture, texCoord).r * attennuationScale;
    vec3 volumeColor;
    if(hasVertexColor == 0) {
      //Color Attenuation from Thickness (Beer's Law)
        volumeColor = computeAttennuation(fthick);
    } else {
        vec3 tmp = texture(fluidColorTexture, texCoord).xyz;
        tmp.r = 1.0 - pow(tmp.r, vertexColorPower) * vertexColorScale;
        tmp.g = 1.0 - pow(tmp.g, vertexColorPower) * vertexColorScale;
        tmp.b = 1.0 - pow(tmp.b, vertexColorPower) * vertexColorScale;

        volumeColor = vec3(exp(-tmp.r * fthick),
                           exp(-tmp.g * fthick),
                           exp(-tmp.b * fthick));


    }

    vec3 refractionDir   = refract(-viewer, N, eta);
    vec3 refractionColor = volumeColor * texture(opaqueRGBATexture, texCoord + refractionDir.xy * refractionScale).xyz;

    fresnelRatio = mix(fresnelRatio, 1.0, additionalReflection);
    vec3 finalColor = mix(refractionColor, reflectionColor, fresnelRatio) + fluidSpecularColor * specular;
    gl_FragData[0] = vec4(finalColor, 1);
}
