## ViewsScivis: getting a scene out of a view

Two things come out of a scene: a picture of it, and the geometry in it. Both are
`vtkScivisExporter`, reached as `view->GetExporter()`:

```cpp
view->GetExporter()->SaveScreenshot("frame.png");
view->GetExporter()->ExportScene("scene.gltf");
```

Each picks its format from the file extension, so choosing one is choosing a
name. Images are written as `.png`, `.jpg`, `.tif` or `.bmp`; geometry as
`.gltf`, `.obj`, `.vrml`/`.wrl` or `.x3d`. An extension neither understands is
refused rather than half written.

`CaptureImage()` hands the picture back as a `vtkImageData` instead of writing
it, for an application with somewhere else to put it.

### How a capture comes out

`Magnification` and `TransparentBackground` describe every capture the exporter
makes rather than being repeated at each call, so an application that wants
everything at twice the window size says so once:

```cpp
view->GetExporter()->SetMagnification(2);
view->GetExporter()->TransparentBackgroundOn();
```

A transparent capture comes back with an alpha channel. Taking one means turning
the view's background off for as long as it takes to read the window, which is
put back afterwards, so the view looks the same before and after.

The scene is brought up to date and drawn before anything is written, so what
comes out is what is on screen.

### Python

The exporter is a sub-object like the selector, so it configures from the view's
constructor the same way:

```python
view = vtkScivisView(exporter={"magnification": 2, "transparent_background": True})
view.exporter.SaveScreenshot("frame.png")
```
