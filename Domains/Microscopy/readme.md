# vtkOpenSlideReader
- A new image reader for vtk
- Wraps open source [openslide library](https://github.com/openslide/openslide "openslide") which implements read support for many whole slide image formats
- Mainly from microscopy domain
- Requires openslide libraries for building

# Known issues
- Ubuntu 14.04 contains incorrectly patched version of openjpeg (dependency of openslide), and thus openslide is unable to decode certain .svs files. This issue is not present in later versions of ubuntu or fedora 23.
