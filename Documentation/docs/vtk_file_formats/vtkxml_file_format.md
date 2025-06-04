## XML File Formats

VTK provides another set of data formats using XML syntax. While these formats are much more complicated than the original VTK format described previously (see [Simple Legacy Formats](vtk_legacy_file_format.md)), they support many more features. The major motivation for their development was to facilitate data streaming and parallel I/O. Some features of the format include support for compression, portable binary encoding, random access, big endian and little endian byte order, multiple file representation of piece data, and new file extensions for different VTK dataset types. XML provides many features as well, especially the ability to extend a file format with application specific tags.

There are two types of VTK XML data files: parallel and serial as described in the following.

* **Serial**. File types designed for reading and writing by applications of only a single process. All of the data are contained
within a single file.

* **Parallel**. File types designed for reading and writing by applications with multiple processes executing in parallel. The dataset is broken into pieces. Each process is assigned a piece or set of pieces to read or write. An individual piece is stored in a corresponding serial file type. The parallel file type does not actually contain any data, but instead describes structural information and then references other serial files containing the data for each piece.

In the XML format, VTK datasets are classified into one of two categories.

* **Structured**. The dataset is a topologically regular array of cells such as pixels and voxels (e.g., image data) or quadrilaterals and hexahedra (e.g., structured grid). Rectangular subsets of the data are described through extents. The structured dataset types are vtkImageData, vtkRectilinearGrid, and vtkStructuredGrid.

* **Unstructured**. The dataset forms a topologically irregular set of points and cells. Subsets of the data are describedusing pieces. The unstructured dataset types are vtkPolyData and vtkUnstructuredGrid.

By convention, each data type and file type is paired with a particular file extension. The types and corresponding extensions are

* ImageData (_.vti_) — Serial vtkImageData (structured).
* PolyData (_.vtp_) — Serial vtkPolyData (unstructured).
* RectilinearGrid (_.vtr_) — Serial vtkRectilinearGrid (structured).
* StructuredGrid (_.vts_) — Serial vtkStructuredGrid (structured).
* UnstructuredGrid (_.vtu_) — Serial vtkUnstructuredGrid (unstructured).
* PImageData (_.pvti_) — Parallel vtkImageData (structured).
* PPolyData (_.pvtp_) — Parallel vtkPolyData (unstructured).
* PRectilinearGrid (_.pvtr_) — Parallel vtkRectilinearGrid (structured).
* PStructuredGrid (_.pvts_) — Parallel vtkStructuredGrid (structured).
* PUnstructuredGrid (_.pvtu_) — Parallel vtkUnstructuredGrid (unstructured).

All of the VTK XML file types are valid XML documents.

**Note:**

There is one case in which the file is not a valid XML document. When the AppendedData section is not encoded as base64, raw binary data is present that may violate the XML specification. This is not default behavior, and must be explicitly enabled by the user.


The document-level element is _VTKFile_:

```xml
<VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
...
</VTKFile>
```

The attributes of the element are:

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; _type_ — The type of the file (the bulleted items in the previous list)..

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; _version_ — File version number in "major.minor" format.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;_byte_order_ — Machine byte order in which data are stored. This is either "BigEndian" or "LittleEndian".

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;_compressor_ — Some data in the file may be compressed. This specifies the subclass of vtkDataCompressor that was used to compress the data.

Nested inside the _VTKFile_ element is an element whose name corresponds to the type of the data format (i.e., the _type_ attribute). This element describes the topology the dataset, and is different for the serial and parallel formats, which are described as follows.

### **Serial XML File Formats**
The _VTKFile_ element contains one element whose name corresponds to the type of dataset the file describes. We refer to this as the dataset element, which is one of _ImageData_, _RectilinearGrid_, _StructuredGrid_, _PolyData_, or _UnstructuredGrid_. The dataset element contains one or more _Piece_ elements, each describing a portion of the dataset. Together, the dataset element and _Piece_ elements specify the entire dataset.

Each piece of a dataset must specify the geometry (points and cells) of that piece along with the data associated with each point or cell. Geometry is specified differently for each dataset type, but every piece of every dataset contains
_PointData_ and _CellData_ elements specifying the data for each point and cell in the piece.

The general structure for each serial dataset format is as follows:

#### **ImageData**
Each ImageData piece specifies its extent within the dataset’s whole extent. The points and cells are described implicitly by the extent, origin, and spacing. Note that the origin and spacing are constant across all pieces, so they are specified as attributes of the _ImageData_ XML element as follows.

```{code-block} xml
:force:
<VTKFile type="ImageData" ...>
  <ImageData WholeExtent="x1 x2 y1 y2 z1 z2"
   Origin="x0 y0 z0" Spacing="dx dy dz">
   <Piece Extent="x1 x2 y1 y2 z1 z2">
      <PointData>...</PointData>
      <CellData>...</CellData>
   </Piece>
   </ImageData>
</VTKFile>
```

#### **RectilinearGrid**
Each RectilinearGrid piece specifies its extent within the dataset’s whole extent. The points are described by the _Coordinates_ element. The cells are described implicitly by the extent.

```{code-block} xml
:force:
<VTKFile type="RectilinearGrid" ...>
  <RectilinearGrid WholeExtent="x1 x2 y1 y2 z1 z2">
    <Piece Extent="x1 x2 y1 y2 z1 z2">
    <PointData>...</PointData>
    <CellData>...</CellData>
    <Coordinates>...</Coordinates>
    </Piece>
  </RectilinearGrid>
</VTKFile>
```

#### **StructuredGrid**
Each StructuredGrid piece specifies its extent within the dataset’s whole extent. The points are described explicitly by the Points element. The cells are described implicitly by the extent.

```{code-block} xml
:force:
<VTKFile type="StructuredGrid" ...>
  <StructuredGrid WholeExtent="x1 x2 y1 y2 z1 z2">
    <Piece Extent="x1 x2 y1 y2 z1 z2">
    <PointData>...</PointData>
    <CellData>...</CellData>
    <Points>...</Points>
    </Piece>
  </StructuredGrid>
</VTKFile>
```

#### **PolyData**
Each PolyData piece specifies a set of points and cells independently from the other pieces. The points are described explicitly by the Points element. The cells are described explicitly by the Verts, Lines, Strips, and Polys elements.

```{code-block} xml
:force:
<VTKFile type="PolyData" ...>
  <PolyData>
    <Piece NumberOfPoints="#" NumberOfVerts="#" NumberOfLines="#"
      NumberOfStrips="#" NumberOfPolys="#">
    <PointData>...</PointData>
    <CellData>...</CellData>
    <Points>...</Points>
    <Verts>...</Verts>
    <Lines>...</Lines>
    <Strips>...</Strips>
    <Polys>...</Polys>
   </Piece>
  </PolyData>
</VTKFile>
```

#### **UnstructuredGrid**
Each UnstructuredGrid piece specifies a set of points and cells independently from the other pieces. The points are described explicitly by the Points element. The cells are described explicitly by the Cells element.

```{code-block} xml
:force:
<VTKFile type="UnstructuredGrid" ...>
  <UnstructuredGrid>
    <Piece NumberOfPoints="#" NumberOfCells="#">
    <PointData>...</PointData>
    <CellData>...</CellData>
    <Points>...</Points>
    <Cells>...</Cells>
    </Piece>
  </UnstructuredGrid>
</VTKFile>
```

Every dataset describes the data associated with its points and cells with PointData and CellData XML elements as follows:
```{code-block} xml
:force:
  <PointData Scalars="Temperature" Vectors="Velocity">
    <DataArray Name="Velocity" .../>
    <DataArray Name="Temperature" .../>
    <DataArray Name="Pressure" .../>
  </PointData>
```

VTK allows an arbitrary number of data arrays to be associated with the points and cells of a dataset. Each data array is described by a DataArray element which, among other things, gives each array a name. The following attributes of PointData and CellData are used to specify the active arrays by name:

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;_Scalars_ — The name of the active scalars array, if any.
<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;_Vectors_ — The name of the active vectors array, if any.
<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;_Normals_ — The name of the active normals array, if any.
<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;_Tensors_ — The name of the active tensors array, if any.
<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;_TCoords_ — The name of the active texture coordinates array, if any.

Some datasets describe their points and cells using different combinations of the following common elements:

* **Points** — The _Points_ element explicitly defines coordinates for each point individually. It contains one _DataArray_ element describing an array with three components per value, each specifying the coordinates of one point.

```{code-block} xml
:force:
    <Points>
      <DataArray NumberOfComponents="3" .../>
    </Points>
```

* **Coordinates** — The _Coordinates_ element defines point coordinates for an extent by specifying the ordinate along each axis for each integer value in the extent’s range. It contains three _DataArray_ elements describing the ordinates along the x-y-z axes, respectively.

```{code-block} xml
:force:
    <Coordinates>
      <DataArray .../>
      <DataArray .../>
      <DataArray .../>
    </Coordinates>
```

* **Verts**, **Lines**, **Strips**, and **Polys** — The _Verts_, _Lines_, _Strips_, and _Polys_ elements define cells explicitly by specifying point connectivity. Cell types are implicitly known by the type of element in which they are specified. Each element contains two _DataArray_ elements. The first array specifies the point connectivity. All the cells’ point lists are concatenated together. The second array specifies the offset into the connectivity array for the end of each cell.

```{code-block} xml
:force:
    <Verts>
      <DataArray type="Int32" Name="connectivity" .../>
      <DataArray type="Int32" Name="offsets" .../>
    </Verts>
```

* **Cells** — The _Cells_ element defines cells explicitly by specifying point connectivity and cell types. It contains three _DataArray_ elements. The first array specifies the point connectivity. All the cells’ point lists are concatenated together. The second array specifies the offset into the connectivity array for the end of each cell. The third array specifies the type of each cell. (Note: the cell types are defined in Figure 2 and Figure 3.)

```{code-block} xml
:force:
    <Cells>
      <DataArray type="Int32" Name="connectivity" .../>
      <DataArray type="Int32" Name="offsets" .../>
      <DataArray type="UInt8" Name="types" .../>
    </Cells>
```

All of the data and geometry specifications use _DataArray_ elements to describe their actual content as follows:

* **DataArray** — The _DataArray_ element stores a sequence of values of one type. There may be one or more components per value.

```{code-block} xml
:force:
    <DataArray type="Float32" Name="vectors" NumberOfComponents="3"
               format="appended" offset="0"/>
    <DataArray type="Float32" Name="scalars" format="binary">
               bAAAAAAAAAAAAIA/AAAAQAAAQEAAAIBA... </DataArray>
    <DataArray type="Int32" Name="offsets" format="ascii">
               10 20 30 ... </DataArray>
```

The attributes of the _DataArray_ elements are described as follows:
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; type — The data type of a single component of the array. This is one of Int8, UInt8, Int16, UInt16, Int32, UInt32, Int64, UInt64, Float32, Float64. Note: the 64-bit integer types are only supported if VTK_USE_64BIT_IDS is on (a CMake variable—see "CMake" on [page 10](https://www.kitware.com/products/books/VTKUsersGuide.pdf#page=24)) or the platform is 64-bit.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Name — The name of the array. This is usually a brief description of the data stored in the array.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;NumberOfComponents — The number of components per value in the array.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;format — The means by which the data values themselves are stored in the file. This is "ascii", "binary", or "appended".

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;offset — If the format attribute is "appended", this specifies the offset from the beginning of the appended data section to the beginning of this array’s data.

The _format_ attribute chooses among the three ways in which data values can be stored:

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;_format="ascii"_ — The data are listed in ASCII directly inside the _DataArray_ element. Whitespace is used for separation.
<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;_format="binary"_ — The data are encoded in base64 and listed contiguously inside the _DataArray_ element. Data may also be compressed before encoding in base64. The byte-order of the data matches that specified by the byte_order attribute of the _VTKFile_ element.
<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;format="appended" — The data are stored in the appended data section. Since many _DataArray_ elements may store their data in this section, the offset attribute is used to specify where each DataArray’s data begins. This format is the default used by VTK’s writers.

The appended data section is stored in an _AppendedData_ element that is nested inside _VTKFile_ after the dataset element:

```{code-block} xml
:force:
  <VTKFile ...>
    ...
    <AppendedData encoding="base64">
                  _QMwEAAAAAAAAA...
    </AppendedData>
  </VTKFile>
```

The appended data section begins with the first character after the underscore inside the _AppendedData_ element. The underscore is not part of the data, but is always present. Data in this section is always in binary form, but can be compressed and/or base64 encoded. The byte-order of the data matches that specified by the byte_order attribute of the _VTKFile_ element. Each _DataArray_’s data are stored contiguously and appended immediately after the previous _DataArray_’s data without a separator. The _DataArray_’s _offset_ attribute indicates the file position offset from the first character after the underscore to the beginning its data.

### **Parallel File Formats**
The parallel file formats do not actually store any data in the file. Instead, the data are broken into pieces, each of which is stored in a serial file of the same dataset type.

The _VTKFile_ element contains one element whose name corresponds to the type of dataset the file describes, but with a "P" prefix. We refer to this as the parallel dataset element, which is one of _PImageData_, _PRectilinearGrid_, _PStructuredGrid_, _PPolyData_, or _PUnstructuredGrid_.

The parallel dataset element and those nested inside specify the types of the data arrays used to store points, pointn data, and cell data (the type of arrays used to store cells is fixed by VTK). The element does not actually contain any data, but instead includes a list of _Piece_ elements that specify the source from which to read each piece. Individual pieces are stored in the corresponding serial file format. The parallel file needs to specify the type and structural information so that readers can update pipeline information without actually reading the pieces’ files.

The general structure for each parallel dataset format is as follows:

#### **PImageData**
The _PImageData_ element specifies the whole extent of the dataset and the number of ghost-levels by which the extents in the individual pieces overlap. The Origin and Spacing attributes implicitly specify the point locations. Each _Piece_ element describes the extent of one piece and the file in which it is stored.

```{code-block} xml
:force: true
  <VTKFile type="PImageData" ...>
    <PImageData WholeExtent="x1 x2 y1 y2 z1 z2"
                GhostLevel="#" Origin="x0 y0 z0" Spacing="dx dy dz">
      <PPointData>...</PPointData>
      <PCellData>...</PCellData>
      <Piece Extent="x1 x2 y1 y2 z1 z2" Source="imageData0.vti"/>
      ...
   </PImageData>
  </VTKFile>
```

#### **PRectilinearGrid**
The _PRectilinearGrid_ element specifies the whole extent of the dataset and the number of ghost-levels by which the extents in the individual pieces overlap. The _PCoordinates_ element describes the type of arrays used to specify the point ordinates along each axis, but does not actually contain the data. Each _Piece_ element describes the extent of one piece and the file in which it is stored.

```{code-block} xml
:force: true
  <VTKFile type="PRectilinearGrid" ...>
    <PRectilinearGrid WholeExtent="x1 x2 y1 y2 z1 z2"
                      GhostLevel="#">
      <PPointData>...</PPointData>
      <PCellData>...</PCellData>
      <PCoordinates>...</PCoordinates>
      <Piece Extent="x1 x2 y1 y2 z1 z2"
             Source="rectilinearGrid0.vtr"/>
      ...
    </PRectilinearGrid>
  </VTKFile>
```

#### **PStructuredGrid**
The _PStructuredGrid_ element specifies the whole extent of the dataset and the number of ghost-levels by which the extents in the individual pieces overlap. The _PPoints_ element describes the type of array used to specify the point locations, but does not actually contain the data. Each _Piece_ element describes the extent of one piece and the file in which it is stored.

```{code-block} xml
:force: true
  <VTKFile type="PStructuredGrid" ...>
    <PStructuredGrid WholeExtent="x1 x2 y1 y2 z1 z2"
                     GhostLevel="#">
      <PPointData>...</PPointData>
      <PCellData>...</PCellData>
      <PPoints>...</PPoints>
      <Piece Extent="x1 x2 y1 y2 z1 z2"
             Source="structuredGrid0.vts"/>
      ...
    </PStructuredGrid>
  </VTKFile>
```

#### **PPolyData**
The _PPolyData_ element specifies the number of ghost-levels by which the individual pieces overlap. The _PPoints_ element describes the type of array used to specify the point locations, but does not actually contain the data. Each _Piece_ element specifies the file in which the piece is stored.

```{code-block} xml
:force: true
  <VTKFile type="PPolyData" ...>
    <PPolyData GhostLevel="#">
      <PPointData>...</PPointData>
      <PCellData>...</PCellData>
      <PPoints>...</PPoints>
      <Piece Source="polyData0.vtp"/>
      ...
    </PPolyData>
  </VTKFile>
```

#### **PUnstructuredGrid**
The _PUnstructuredGrid_ element specifies the number of ghost-levels by which the individual pieces overlap. The _PPoints_ element describes the type of array used to specify the point locations, but does not actually contain the data. Each _Piece_ element specifies the file in which the piece is stored.

```{code-block} xml
:force: true
  <VTKFile type="PUnstructuredGrid" ...>
    <PUnstructuredGrid GhostLevel="0">
      <PPointData>...</PPointData>
      <PCellData>...</PCellData>
      <PPoints>...</PPoints>
      <Piece Source="unstructuredGrid0.vtu"/>
      ...
    </PUnstructuredGrid>
  </VTKFile>
```

Every dataset uses _PPointData_ and _PCellData_ elements to describe the types of data arrays associated with its points and cells.

* **PPointData** and **PCellData** — These elements simply mirror the _PointData_ and _CellData_ elements from the serial file formats. They contain _PDataArray_ elements describing the data arrays, but without any actual data.

```{code-block} xml
:force: true
    <PPointData Scalars="Temperature" Vectors="Velocity">
      <PDataArray Name="Velocity" .../>
      <PDataArray Name="Temperature" .../>
      <PDataArray Name="Pressure" .../>
   </PPointData>
```

For datasets that need specification of points, the following elements mirror their counterparts from the serial file format:

* **PPoints** — The _PPoints_ element contains one _PDataArray_ element describing an array with three components. The data array does not actually contain any data.

```{code-block} xml
:force: true
    <PPoints>
      <PDataArray NumberOfComponents="3" .../>
    </PPoints>
```

* **PCoordinates** — The _PCoordinates_ element contains three _PDataArray_ elements describing the arrays used to specify ordinates along each axis. The data arrays do not actually contain any data.

```{code-block} xml
:force: true
    <PCoordinates>
      <PDataArray .../>
      <PDataArray .../>
      <PDataArray .../>
   </PCoordinates>
```

All of the data and geometry specifications use _PDataArray_ elements to describe the data array types:

* **PDataArray** — The _PDataArray_ element specifies the type, Name, and optionally the NumberOfComponents attributes from the _DataArray_ element. It does not contain the actual data. This can be used by readers to create the data array in their output without needing to read any real data, which is necessary for efficient pipeline updates in some cases.

```xml
    <PDataArray type="Float32" Name="vectors" NumberOfComponents="3"/>
```

### XML File Example
The following is a complete example specifying a vtkPolyData representing a cube with some scalar data on its points and faces. <sup>[1](https://kitware.com/products/books/VTKUsersGuide.pdf)</sup>

```xml
  <?xml version="1.0"?>
  <VTKFile type="PPolyData" version="0.1" byte_order="LittleEndian">
    <PPolyData GhostLevel="0">
      <PPointData Scalars="my_scalars">
        <PDataArray type="Float32" Name="my_scalars"/>
      </PPointData>
        <PCellData Scalars="cell_scalars" Normals="cell_normals">
          <PDataArray type="Int32" Name="cell_scalars"/>
           <PDataArray type="Float32" Name="cell_normals" NumberOfComponents="3"/>
        </PCellData>
        <PPoints>
          <PDataArray type="Float32" NumberOfComponents="3"/>
        </PPoints>
        <Piece Source="polyEx0.vtp"/>
    </PPolyData>
  </VTKFile>


  <?xml version="1.0"?>
    <VTKFile type="PolyData" version="0.1" byte_order="LittleEndian">
      <PolyData>
        <Piece NumberOfPoints="8" NumberOfVerts="0" NumberOfLines="0"
               NumberOfStrips="0" NumberOfPolys="6">
        <Points>
          <DataArray type="Float32" NumberOfComponents="3" format="ascii">
            0 0 0 1 0 0 1 1 0 0 1 0 0 0 1 1 0 1 1 1 1 0 1 1
          </DataArray>
        </Points>
        <PointData Scalars="my_scalars">
          <DataArray type="Float32" Name="my_scalars" format="ascii">
            0 1 2 3 4 5 6 7
         </DataArray>
        </PointData>
        <CellData Scalars="cell_scalars" Normals="cell_normals">
          <DataArray type="Int32" Name="cell_scalars" format="ascii">
           0 1 2 3 4 5
          </DataArray>
          <DataArray type="Float32" Name="cell_normals"
                     NumberOfComponents="3" format="ascii">
            0 0 -1 0 0 1 0 -1 0 0 1 0 -1 0 0 1 0 0
          </DataArray>
        </CellData>
        <Polys>
          <DataArray type="Int32" Name="connectivity" format="ascii">
             0 1 2 3 4 5 6 7 0 1 5 4 2 3 7 6 0 4 7 3 1 2 6 5
          </DataArray>
          <DataArray type="Int32" Name="offsets" format="ascii">
             4 8 12 16 20 24
          </DataArray>
        </Polys>
      </Piece>
    </PolyData>
   </VTKFile>
```
