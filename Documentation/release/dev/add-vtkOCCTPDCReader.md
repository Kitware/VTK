## Adding a new OpenCascade Reader

VTK now provides a reader that produces a vtkPartinionedDataSetCollection that represents
the structures found in a STEP, IGES, BREP, Binary BREP, or XBF files/streams using OpenCascade.  The collection
will also contain a VTK DataAssembly which is used to capture the hierarchy of
the geometry.

It is similar to the existing vtkOCCTReader which produces vtkMultiBlockDatasets but with the following enhancements:

* You can set the reader to process files or streams
* The reader provides a **CanReadFile** method that will return true if the file contents are supported by the reader
* You can tell the reader to automatically deduce the format of the incoming data or explicitly set the format.
* The reader produces a vtkDataAssembly representation that reflects the CAD assembly structure
* The reader does not produce the equivalent *unnamed* Blocks that the original reader produces.  The reader will only create nodes in the assembly iff the corresponding OpenCascade
assembly label has a name.  If it does not then all of the label's descendants will be added to its
corresponding parent in the assembly.
* In addition, when dealing with OpenCascade labels that reference other geometric labels, the resulting VTK DataAssembly node will be named based on the geometric label name and not the reference label.  This results
in a hierarchy that better represents what is displayed in CAD packages.
* The reader provides a format option called AUTO that will attempt to determine the format of data.
* As with the original reader, the reader will pull normals, colors, and UV parametric information if present; however, in the case of color, if a shape does not have color directly associated with it, the reader will see
if the color is inherited by checking its ancestors.  The generated cell-data color arrays contain
RGBA values, preserving alpha transparency when supported by OpenCASCADE.
* If an assembly label shares a name with one of its siblings, the reader will append a '_' followed by a number to make the name unique.
* You can request that the reader produce a map (represented as a 2-component vtkIntArray named *NodeMapping* attached as Field data to the output) which represents each renamed vtkDataAssembly node and the node that shared its original name.

### Concerning Color

`vtkOCCTPDCReader` now resolves XCAF surface colors for individual faces and
curve colors for individual edges. Definition-level styles assigned to
containing shells, wires, or other subshapes are propagated to their face and
edge output. The generated cell-data color arrays contain sRGB RGBA values,
preserving alpha transparency when it is present in the source data and
supported by OpenCASCADE. Colors assigned to assembly instances are not
applied.

**Note**: Colors assigned to assembly instances are not applied.

## OpenCASCADE 8.x Support

In addition, both OpenCASCADE readers no longer depend on deprecated collection aliases or
typedef-only headers that are not part of a standard OpenCASCADE 8
installation. Existing OpenCASCADE 7.x support is preserved.
