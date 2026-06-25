## Add ability to blur the skybox background

VTK now has the ability to blur the background when a skybox is used.

- Use `vtkRenderer::SetSkyboxBlurEnabled(bool)` to enable the skybox blur.
- Use `vtkRenderer::SetSkyboxBlurRadius(float)` to change the amount of blur.

![skybox_blur_image](./add-skybox-blur-image.png)

*Skybox blur example*
