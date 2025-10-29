## Add composite data texturing

![composite-poly-data-texturing-image](add-composite-poly-data-texturing-image)

`vtkCompositePolyDataMapper` can now handle textures for composite data. Textures can be assign with a block flat index, thus texturing each block differently. To do so, the function `vtkCompositePolyDataMapper::SetBlockTextureImage()` can be called giving the flat index and the associated image data.
