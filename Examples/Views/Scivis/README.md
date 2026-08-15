# ViewsScivis examples

Plain Python, no GUI toolkit: each one builds a view, puts something in it, and
enters the interactive event loop with `view.Start()`. Close the window to end.

```sh
python surface.py
```

| | |
| --- | --- |
| `surface.py` | surfaces, their properties, the light kit and the camera |
| `coloring.py` | coloring by an array, shared color maps, the scalar bar that describes the scene |
| `annotations.py` | a text overlay and grid axes that follow the scene, neither with data behind it |
| `volume.py` | a volume and a surface in the same view |
| `selection.py` | turning a region of the screen into a selection |
| `export.py` | writing the scene out as a picture and as geometry |

They are meant to be read in that order: the first shows what a view is, and
each one after it adds a part of the module.

The examples under `Examples/GUI/Imgui` do the same things inside an imgui
application, with controls for what is set here in code.
