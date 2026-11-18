// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Slang types that customize vtkWebGPUPolyDataMapper's shaders. CustomShading.cxx loads this
// module and picks a type for an interface with vtkWebGPUShaderProperty::SetSlangSpecialization().
module custom_shading;

import slangkit_hooks;   // IShaderHooks, from SlangKit
import slangkit_shading; // ILightingModel, from SlangKit
import custom_palette;   // next to this file

// Paints stripes over VTK's colors. color() runs after VTK has applied scalars and textures.
public struct Stripes : IShaderHooks
{
    public override void color(inout FragmentContext f)
    {
        if (frac(f.positionVC.y * kStripesPerUnit) > 0.5f)
            f.diffuseColor = kStripeColor;
    }
};

// Diffuse lighting in a few flat bands, without the specular term.
public struct Toon : ILightingModel
{
    public float3 direct(MaterialSample m, SurfaceSample s, LightSample light)
    {
        float d = max(dot(s.normal, light.direction), 0.f);
        return m.baseColor * light.radiance * (floor(d * kToonBands) / kToonBands);
    }

    public float3 ambient(MaterialSample m, SurfaceSample s, float3 ambientRadiance)
    {
        return m.ambientColor * ambientRadiance;
    }

    public float3 resolve(float3 radiance, MaterialSample m)
    {
        return radiance;
    }
};
