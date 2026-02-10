## Fix handling of image null pointer in Rendering/Matplotlib

Some architectures (bigendian s390x) generate an image object with
null scalar pointer in vtkMatplotlibMathTextUtilities::RenderOneCell
causing segfaults. RenderOneCell() now returns false when a null
pointer is found.
