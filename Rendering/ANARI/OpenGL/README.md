# VTK::RenderingANARIOpenGL

## Introduction

`VTK::RenderingANARIOpenGL` is a module that can be used to display the ANARI rendered frame in a render window. This module implements a render pass that can be used for onscreen rendering.

---

## Using ANARI onscreen

The following code snippet is the intended way to show the ANARI rendered frame to an OpenGL render window:

```c++
// Setup an actor
vtkNew<vtkSphereSource> source;
vtkNew<vtkPolyDataMapper> mapper;
mapper->SetInputConnection(source->GetOutputPort());
vtkNew<vtkActor> actor;
actor->SetMapper(mapper);

vtkNew<vtkRenderWindowInteractor> interactor;
vtkNew<vtkRenderWindow> renderWindow;
interactor->SetRenderWindow(renderWindow);
vtkNew<vtkRenderer> renderer;
renderer->AddActor(actor);
renderWindow->AddRenderer(renderer);

// Initialize ANARI pass
vtkNew<vtkAnariPass> anariPass;
anariPass->GetAnariDevice()->SetupAnariDeviceFromLibrary("environment", "default", false);
renderer->SetPass(anariPass);

// Start scene rendering / interaction
interactor->Start();
```

## Factory preferences

When both ANARI and OpenGL are activated on a VTK build, the factory may not pick the right class for `vtkRenderWindow`. In this case, the override attribute `SupportRenderPass=true` helps the factory to select the `vtkOpenGLRenderWindow`. See [](/advanced/runtime_settings.md#override-attributes) for more information about this topic.
