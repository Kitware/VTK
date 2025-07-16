# Fix light transforms when computing shadow maps

The `vtkShadowMapBakerPass` computes shadow maps for each light by simulating a `vtkCamera` for each
light and compositing the rendered images. However, an unintentional side-effect of this was that
the original light's transforms were being affected by this. In other words, the lights would be
corrupt after the shadow map computation. To circumvent this, we cache the light
transforms prior to the shadow map computation and then reset them after.

![](http://vtk.org/files/ExternalData/SHA512/9412ed54080c7a972d786a269538a3dbd56a5aa070f2b35ba448dfd3cca211e7e4b4d196e07c8ff2d56ff4bb7b0855e5584758c09457d3d6f3c760ee47a4dded)
