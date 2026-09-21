## ViewsScivis: text overlays

`vtkTextOverlayRepresentation` draws a string over the scene — a title, a frame
number, a units label. It is added and removed like anything else a view shows:

```cpp
vtkNew<vtkTextOverlayRepresentation> title;
title->SetText("Frame 12");
title->SetPosition(20, 20);
title->GetTextProperty()->SetFontSize(24);
view->AddRepresentation(title);
```

```python
view += vtkTextOverlayRepresentation(
    text="Frame 12", position=(20, 20), font_size=24, color="white")
```

### A representation with no data

This is the first representation in the module with nothing behind it, and it is
what `vtkScivisRepresentation` exists for. There is no input to connect, no array
to color by, no selection and no color map — a text overlay is shown, hidden,
added and removed, which is the whole of what a view asks of the things it shows.

So it derives from `vtkScivisRepresentation` rather than
`vtkScivisDataRepresentation`, and implements two methods instead of the seven a
representation of data has to answer. The view passes it over where only data can
answer: it gets no scalar bar of its own, and it does not keep one alive for an
array nothing else is drawing. It does not move the camera either, being drawn in
two dimensions over the scene rather than sitting in it.

### What is on the class

The text and where it sits. How it is drawn — font, size, color, opacity, bold,
italic, justification — belongs to `vtkTextProperty`, which `GetTextProperty()`
hands out rather than mirroring one method at a time. `GetTextActor()` is there
for what neither covers, such as a scaled text box.

In Python the flat spelling still reads as though those were the
representation's own, because a property the class does not define is routed to
the object that owns it:

```python
text.font_size = 24     # the text property
text.bold = True        # the text property
text.color = "white"    # named colors, resolved here
```
