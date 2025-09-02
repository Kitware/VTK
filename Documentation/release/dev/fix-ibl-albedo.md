# Use lambertian diffuse factor for image based illumination

In the Cook-Torrance BRDF, the diffuse illumination component uses a
lambert factor. This factor is simply the albedo color divided by pi.

The shader currently uses this lambertian diffuse factor for direct
illumination but was missing for image based illumination.

So, we're putting the diffuse lambert term into the image-based lighting
diffuse component. This will have an affect of cutting albedo to 1/3-ish
(which is not the same as cutting full illumination to 1/3 of the
original, because there is more that affects the final image than just
this lambertian factor). Image-based diffuse effects will be dimmer.

## Results

|Before|After|
|---|---|
|![](https://vtk.org/files/ExternalData/SHA512/8e3116a176b37f4dd421b631505d69654fabf64e58efcecb815d27ba7d882b1765ea5b9360088823d766fa931d5c24a595c265342af9971f2f67329a2603f1cc)| ![](https://vtk.org/files/ExternalData/SHA512/b211abab5008506d07338526586d21183e3968fcc133e6bbe9d360a09ff32dfd7d620b6f2e8febc0ec894eeb9c429ca0e45864a11fa78235889db57b30d66f95)|

## References:
- https://learnopengl.com/PBR/Theory
- https://google.github.io/filament/Filament.html#materialsystem/diffusebrdf
- https://learnopengl.com/PBR/IBL/Diffuse-irradiance (the term c/π is the lambertian diffuse component)
- https://google.github.io/filament/Filament.html#toc5.3.4.6 (shown in the implementation as Fd_lambert().)
