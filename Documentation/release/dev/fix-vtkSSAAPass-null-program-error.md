## Fix vtkSSAAPass null program error when changing renderers

You can now safely change the list of renderers without encountering null program errors in `vtkSSAAPass`.

```py
ssaaPass = vtkSSAAPass()

renderer1 = vtkRenderer()
renderer1.AddActor(actor)
renderer1.SetPass(ssaaPass)
renderWindow.AddRenderer(newRenderer)
renderWindow.Render()

# Now change the renderer
renderer2 = vtkRenderer()
renderer2.AddActor(actor)
renderer2.SetPass(ssaaPass)
renderWindow.RemoveRenderer(renderer1)
renderWindow.AddRenderer(renderer2)
renderWindow.Render()
```
