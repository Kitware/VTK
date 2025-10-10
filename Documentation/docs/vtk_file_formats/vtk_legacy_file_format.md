# Simple Legacy Formats

The legacy VTK file formats consist of five basic parts.

1. The first part is the file version and identifier. This part contains the single line: ```vtk DataFile Version x.x.``` This line must be exactly as shown with the exception of the version number x.x, which will vary with different releases of VTK. (Note: the current version number is 3.0. Version 1.0 and 2.0 files are compatible with version 3.0 files.)

2. The second part is the header. The header consists of a character string terminated by end-of-line character \n. The header is 256 characters maximum. The header can be used to describe the data and include any other pertinent information.

3. The next part is the file format. The file format describes the type of file, either ASCII or binary. On this line the single word ASCII or BINARY must appear.

4. The fourth part is the dataset structure. The geometry part describes the geometry and topology of the dataset. This part begins with a line containing the keyword _DATASET_ followed by a keyword describing the type of dataset. Then, depending upon the type of dataset, other keyword/data combinations define the actual data.

5. The final part describes the dataset attributes. This part begins with the keywords _POINT_DATA_ or _CELL_DATA_, followed by an integer number specifying the number of points or cells, respectively. (It doesn’t matter whether _POINT_DATA_ or _CELL_DATA_ comes first.) Other keyword/data combinations then define the actual dataset attribute values (i.e., scalars, vectors, tensors, normals, texture coordinates, or field data).

An overview of the file format is shown in Figure 1:


<table style="border-collapse: collapse; margin-left: 40px; margin-right: auto;">
    <tr><td style="white-space: nowrap; font-family: monospace">vtk DataFile Version 2.0</td><td>(1)</td></tr>
    <tr><td style="white-space: nowrap; font-family: monospace">Really cool data</td><td>(2)</td></tr>
    <tr><td style="white-space: nowrap; font-family: monospace">ASCII | BINARY</td><td>(3)</td></tr>
    <tr><td style="white-space: nowrap; font-family: monospace">DATASET <b><i>type</i></b><br>...</td><td>(4)</td></tr>
    <tr><td style="white-space: nowrap; font-family: monospace">POINT_DATA <b><i>type</i></b><br>...<br>CELL_DATA <b><i>type</i></b><br>...</td><td>(5)</td></tr>
</table>
Key:
<table style="border-collapse: collapse; margin-left: 40px; margin-right: auto;">
  <tr>
    <td style="white-space: nowrap; vertical-align: top;"><b>Part 1:</b> File version and identifier</td>
    <td rowspan="2" style="white-space:nowrap; vertical-align: top;"><b>Part 4:</b> Dataset structure: Geometry/Topology.<br>
      <b><i>type</i></b> is one of:
      <ul style="list-style-type: none;">
        <li>STRUCTURED_POINTS</li>
        <li>STRUCTURED_GRID</li>
        <li>UNSTRUCTURED_GRID</li>
        <li>POLYDATA</li>
        <li>RECTILINEAR_GRID</li>
        <li>FIELD</li>
      </ul>
    </td>
  </tr>
  <tr>
    <td style="vertical-align: top;"><b>Part 2:</b> Header (256 characters maximum, terminated with the newline \n character)</td>
  </tr>
  <tr>
    <td style="vertical-align: top;"><b>Part 3:</b> File format, either ASCII or BINARY</td>
    <td style="vertical-align: top;"><b>Part 5:</b> Dataset attributes. The number of data items <i>n</i> of each type must match the number
        of points or cells in the dataset. (If <i>type</i> is FIELD, point and cell data should be omitted.)</td>
  </tr>
</table>

<a name="Figure1">**Figure 1:**</a> **Overview of five parts of VTK data file format.**



 The first three parts are mandatory, but the other two are optional. Thus you have the flexibility of mixing and matching dataset attributes and geometry, either by operating system file
manipulation or using VTK filters to merge data. Keywords are case insensitive, and may be separated by whitespace. Before describing the data file formats please note the following.

* _dataType_ is one of the types _bit_, _unsigned_char_, _char_, _unsigned_short_, _short_, _unsigned_int_, _int_, _unsigned_long_, _long_, _float_, or _double_. These keywords are used to describe the form of the data, both for reading from file, as well as constructing the appropriate internal objects. Not all data types are supported for all classes.

* All keyword phrases are written in ASCII form whether the file is binary or ASCII. The binary section of the file (if in binary form) is the data proper; i.e., the numbers that define points coordinates, scalars, cell indices, and so forth.

* Indices are 0-offset. Thus the first point is point id 0.

* If both the data attribute and geometry/topology part are present in the file, then the number of data values defined in the data attribute part must exactly match the number of points or cells defined in the geometry/topology part.

* Cell types and indices are of type _int_.

* Binary data must be placed into the file immediately after the "newline" _(\n)_ character from the previous ASCII keyword and parameter sequence.

* The geometry/topology description must occur prior to the data attribute description.

## Binary Files

Binary files in VTK are portable across different computer systems as long as you observe two conditions. First, make sure that the byte ordering of the data is correct, and second, make sure that the length of each data type is consistent.

Most of the time VTK manages the byte ordering of binary files for you. When you write a binary file on one computer and read it in from another computer, the bytes representing the data will be automatically swapped as necessary. For example, binary files written on a Sun are stored in big endian order, while those on a PC are stored in little endian order. As a result, files written on a Sun workstation require byte swapping when read on a PC. (See the class vtkByteSwap for implementation details.) The VTK data files described here are written in big endian form.

Some file formats, however, do not explicitly define a byte ordering form. You will find that data read or written by external programs, or the classes vtkVolume16Reader, vtkMCubesReader, and vtkMCubesWriter may have a different byte order depending on the system of origin. In such cases, VTK allows you to specify the byte order by using the methods

```
SetDataByteOrderToBigEndian()
SetDataByteOrderToLittleEndian()
```

Another problem with binary files is that systems may use a different number of bytes to represent an integer or other native type. For example, some 64-bit systems will represent an integer with 8-bytes, while others represent an integer with 4-bytes. Currently, the *Visualization Toolkit* cannot handle transporting binary files across systems with incompatible data length. In this case, use ASCII file formats instead.

## Dataset Format
The *Visualization Toolkit* supports five different dataset formats: structured points, structured grid, rectilinear grid, unstructured grid, and polygonal data. Data with implicit topology (structured data such as vtkImageData and vtkStructuredGrid) are ordered with x increasing fastest, then y, then z. These formats are as follows.

* **Structured Points**. The file format supports 1D, 2D, and 3D structured point datasets. The dimensions nx, ny, nz must be greater than or equal to 1. The data spacing sx, sy, sz must be greater than 0. (Note: in the version 1.0 data file, spacing was referred to as "aspect ratio". ASPECT_RATIO can still be used in version 2.0 data files, but is discouraged.)
<br><br>DATASET STRUCTURED_POINTS<br>
DIMENSIONS <b><i>n<sub>x</sub> n<sub>y</sub> n<sub>z</sub></i></b><br>
ORIGIN <b><i>x y z</i></b><br>
SPACING <b><i>s<sub>x</sub> s<sub>y</sub> s<sub>z</sub></i></b>

* **Structured Grid**. The file format supports 1D, 2D, and 3D structured grid datasets. The dimensions nx, ny, nz must be greater than or equal to 1. The point coordinates are defined by the data in the _POINTS_ section. This consists of x-y-z data values for each point.
<br><br>DATASET STRUCTURED_GRID<br>
DIMENSIONS <b><i>n<sub>x</sub> n<sub>y</sub> n<sub>z</sub></i></b><br>
POINTS <b><i>n dataType<br>
p<sub>0x</sub> p<sub>0y</sub> p<sub>0z</sub><br>
p<sub>1x</sub> p<sub>1y</sub> p<sub>1z</sub><br>
...<br>
p<sub>(n-1)x</sub> p<sub>(n-1)y</sub> p<sub>(n-1)z</sub></i></b><br>


* **Rectilinear Grid**. A rectilinear grid defines a dataset with regular topology, and semi-regular geometry aligned along the x-y-z coordinate axes. The geometry is defined by three lists of monotonically increasing coordinate values, one list for each of the x-y-z coordinate axes. The topology is defined by specifying the grid dimensions, which must be greater than or equal to 1.
<br><br>DATASET RECTILINEAR_GRID<br>
DIMENSIONS <b><i>n<sub>x</sub> n<sub>y</sub> n<sub>z</sub></i></b><br>
X_COORDINATES <b><i>n<sub>x</sub> dataType</i></b><br>
<b><i>x<sub>0</sub> x<sub>1</sub> ... x<sub>(nx-1)</sub></i></b><br>
Y_COORDINATES <b><i>n<sub>y</sub> dataType</i></b><br>
<b><i>y<sub>0</sub> y<sub>1</sub> ... y<sub>(ny-1)</sub></i></b><br>
Z_COORDINATES <b><i>n<sub>z</sub> dataType</i></b><br>
<b><i>z<sub>0</sub> z<sub>1</sub> ... z<sub>(nz-1)</sub></i></b><br>


* **Polygonal Data**. The polygonal dataset consists of arbitrary combinations of surface graphics primitives vertices (and polyvertices), lines (and polylines), polygons (of various types), and triangle strips. Polygonal data is defined by the _POINTS_, _VERTICES_, _LINES_, _POLYGONS_, or _TRIANGLE_STRIPS_ sections. The _POINTS_ definition is the same as we saw for structured grid datasets. The _VERTICES_, _LINES_, _POLYGONS_, or _TRIANGLE_STRIPS_ keywords define the polygonal dataset topology. Each of these keywords requires two parameters: the number of cells n and the size of the cell list size. The cell list size is the total number of integer values required to represent the list (i.e., sum of numPoints and connectivity indices over each cell). None of the keywords _VERTICES_, _LINES_, _POLYGONS_, or _TRIANGLE_STRIPS_ is required.
<br><br>DATASET POLYDATA<br>
POINTS <b><i>n dataType<br>
p<sub>0x</sub> p<sub>0y</sub> p<sub>0z</sub><br>
p<sub>1x</sub> p<sub>1y</sub> p<sub>1z</sub><br>
...<br>
p<sub>(n-1)x</sub> p<sub>(n-1)y</sub> p<sub>(n-1)z</sub></i></b><br>
<br>VERTICES <b><i>n size<br>
numPoints<sub>0</sub>, i<sub>0</sub>, j<sub>0</sub>, k<sub>0</sub>, ...<br>
numPoints<sub>1</sub>, i<sub>1</sub>, j<sub>1</sub>, k<sub>1</sub>, ...<br>
...<br>
numPoints<sub>n-1</sub>, i<sub>n-1</sub>, j<sub>n-1</sub>, k<sub>n-1</sub>, ...</i></b><br>
<br>LINES <b><i>n size<br>
numPoints<sub>0</sub>, i<sub>0</sub>, j<sub>0</sub>, k<sub>0</sub>, ...<br>
numPoints<sub>1</sub>, i<sub>1</sub>, j<sub>1</sub>, k<sub>1</sub>, ...<br>
...<br>
numPoints<sub>n-1</sub>, i<sub>n-1</sub>, j<sub>n-1</sub>, k<sub>n-1</sub>, ...</i></b><br>
<br>POLYGONS <b><i>n size<br>
numPoints<sub>0</sub>, i<sub>0</sub>, j<sub>0</sub>, k<sub>0</sub>, ...<br>
numPoints<sub>1</sub>, i<sub>1</sub>, j<sub>1</sub>, k<sub>1</sub>, ...<br>
...<br>
numPoints<sub>n-1</sub>, i<sub>n-1</sub>, j<sub>n-1</sub>, k<sub>n-1</sub>, ...</i></b><br>
<br>TRIANGLE_STRIPS <b><i>n size<br>
numPoints<sub>0</sub>, i<sub>0</sub>, j<sub>0</sub>, k<sub>0</sub>, ...<br>
numPoints<sub>1</sub>, i<sub>1</sub>, j<sub>1</sub>, k<sub>1</sub>, ...<br>
...<br>
numPoints<sub>n-1</sub>, i<sub>n-1</sub>, j<sub>n-1</sub>, k<sub>n-1</sub>, ...</i></b><br>


* **Unstructured Grid**. The unstructured grid dataset consists of arbitrary combinations of any possible cell type. Unstructured grids are defined by points, cells, and cell types. The CELLS keyword requires two parameters: the number of cells n and the size of the cell list size. The cell list size is the total number of integer values required to represent the list (i.e., sum of numPoints and connectivity indices over each cell). The CELL_TYPES keyword requires a single parameter: the number of cells n. This value should match the value specified by the CELLS keyword. The cell types data is a single integer value per cell that specified cell type (see vtkCell.h or Figure 2).
<br><br>DATASET UNSTRUCTURED_GRID<br>
POINTS <b><i>n dataType<br>
p<sub>0x</sub> p<sub>0y</sub> p<sub>0z</sub><br>
p<sub>1x</sub> p<sub>1y</sub> p<sub>1z</sub><br>
...<br>
p<sub>(n-1)x</sub> p<sub>(n-1)y</sub> p<sub>(n-1)z</sub></i></b><br>
<br>CELLS <b><i>n size<br>
numPoints<sub>0</sub>, i<sub>0</sub>, j<sub>0</sub>, k<sub>0</sub>, ...<br>
numPoints<sub>1</sub>, i<sub>1</sub>, j<sub>1</sub>, k<sub>1</sub>, ...<br>
numPoints<sub>2</sub>, i<sub>2</sub>, j<sub>2</sub>, k<sub>2</sub>, ...<br>
...<br>
numPoints<sub>n-1</sub>, i<sub>n-1</sub>, j<sub>n-1</sub>, k<sub>n-1</sub>, ...</i></b><br>
<br>CELL_TYPES <b><i>n<br>
type<sub>0</sub><br>
type<sub>1</sub><br>
type<sub>2</sub><br>
...<br>
type<sub>n-1</sub></i></b><br>

* **Field**. Field data is a general format without topological and geometric structure, and without a particular dimensionality. Typically field data is associated with the points or cells of a dataset. However, if the FIELD type is specified as the dataset type (see Figure 1), then a general VTK data object is defined. Use the format described in the next section to define a field. Also see "Working With Field Data" on [page 249](https://www.kitware.com/products/books/VTKUsersGuide.pdf#page=263) and the fourth example in this chapter [Legacy File Examples](#legacy-file-examples).

## Dataset Attribute Format

The *Visualization Toolkit* supports the following dataset attributes: scalars (one to four components), vectors, normals, texture coordinates (1D, 2D, and 3D), tensors, and field data. In addition, a lookup table using the RGBA color specification, associated with the scalar data, can be defined as well. Dataset attributes are supported for both points and cells.

Each type of attribute data has a dataName associated with it. This is a character string (without embedded whitespace) used to identify a particular data. The dataName is used by the VTK readers to extract data. As a result, more than one attribute data of the same type can be included in a file. For example, two different scalar fields defined on the dataset points, pressure and temperature, can be contained in the same file. (If the appropriate dataName is not specified in the VTK reader, then the first data of that type is extracted from the file.)

* **Scalars**. Scalar definition includes specification of a lookup table. The definition of a lookup table is optional. If not specified, the default VTK table will be used (and tableName should be "default"). Also note that the numComp variable is optional—by default the number of components is equal to one. (The parameter numComp must range between 1 and 4 inclusive; in versions of VTK prior to 2.3 this parameter was not supported.)
<br><br>SCALARS <b><i>dataName dataType numComp</i></b><br>
LOOKUP_TABLE <b><i>tableName<br>
s<sub>0</sub><br>
s<sub>1</sub><br>
...<br>
s<sub>n-1</sub></i></b><br>
<br>The definition of color scalars (i.e., unsigned char values directly mapped to color) varies depending upon the number of values (nValues) per scalar. If the file format is ASCII, the color scalars are defined using nValues float values between (0,1). If the file format is BINARY, the stream of data consists of nValues unsigned char values per scalar value.
<br><br>COLOR_SCALARS <b><i>dataName nValues<br>
c<sub>00</sub> c<sub>01</sub> ... c<sub>0(nValues-1)</sub><br>
c<sub>10</sub> c<sub>11</sub> ... c<sub>1(nValues-1)</sub><br>
...<br>
c<sub>(n-1)0</sub> c<sub>(n-1)1</sub> ... c<sub>(n-1)(nValues-1)</sub></i></b>

* **Lookup Table**. The *tableName* field is a character string (without embedded white space) used to identify the lookup table. This label is used by the VTK reader to extract a specific table.
Each entry in the lookup table is a rgba[4] (red-green-blue-alpha) array (alpha is opacity where alpha=0 is transparent). If the file format is ASCII, the lookup table values must be float values between (0,1). If the file format is BINARY, the stream of data must be four unsigned char values per table entry.
<br><br>LOOKUP_TABLE <b><i>tableName size<br>
r<sub>0</sub> g<sub>0</sub> b<sub>0</sub> a<sub>0</sub><br>
r<sub>1</sub> g<sub>1</sub> b<sub>1</sub> a<sub>1</sub><br>
...<br>
r<sub>size-1</sub> g<sub>size-1</sub> b<sub>size-1</sub> a<sub>size-1</sub></i></b>

* **Vectors**.
<br><br>VECTORS <b><i>dataName dataType<br>
v<sub>0x</sub> v<sub>0y</sub> v<sub>0z</sub><br>
v<sub>1x</sub> v<sub>1y</sub> v<sub>1z</sub><br>
...<br>
v<sub>(n-1)x</sub> v<sub>(n-1)y</sub> v<sub>(n-1)z</sub></i></b>

* **Normals**. Normals are assumed normalized &#124;n&#124; = 1.
<br><br>NORMALS <b><i>dataName dataType<br>
n<sub>0x</sub> n<sub>0y</sub> n<sub>0z</sub><br>
n<sub>1x</sub> n<sub>1y</sub> n<sub>1z</sub><br>
...<br>
n<sub>(n-1)x</sub> n<sub>(n-1)y</sub> n<sub>(n-1)z</sub></i></b>


* **Texture Coordinates**. Texture coordinates of 1, 2, and 3 dimensions are supported.
<br><br>TEXTURE_COORDINATES <b><i>dataName dim dataType<br>
t<sub>00</sub> t<sub>01</sub> ... t<sub>0(dim-1)</sub><br>
t<sub>10</sub> t<sub>11</sub> ... t<sub>1(dim-1)</sub><br>
...<br>
t<sub>(n-1)0</sub> t<sub>(n-1)1</sub> ... t<sub>(n-1)(dim-1)</sub></i></b>

* **Tensors**. Currently only real-valued, symmetric tensors are supported.
<br><br>TENSORS <b><i>dataName dataType<br>
t<sup>0</sup><sub>00</sub> t<sup>0</sup><sub>01</sub> t<sup>0</sup><sub>02</sub><br>
t<sup>0</sup><sub>10</sub> t<sup>0</sup><sub>11</sub> t<sup>0</sup><sub>12</sub><br>
t<sup>0</sup><sub>20</sub> t<sup>0</sup><sub>21</sub> t<sup>0</sup><sub>22</sub><br>
<br>
t<sup>1</sup><sub>00</sub> t<sup>1</sup><sub>01</sub> t<sup>1</sup><sub>02</sub><br>
t<sup>1</sup><sub>10</sub> t<sup>1</sup><sub>11</sub> t<sup>1</sup><sub>12</sub><br>
t<sup>1</sup><sub>20</sub> t<sup>1</sup><sub>21</sub> t<sup>1</sup><sub>22</sub><br>
...
<br>
t<sup>n - 1</sup><sub>00</sub> t<sup>n - 1</sup><sub>01</sub> t<sup>n - 1</sup><sub>02</sub><br>
t<sup>n - 1</sup><sub>10</sub> t<sup>n - 1</sup><sub>11</sub> t<sup>n - 1</sup><sub>12</sub><br>
t<sup>n - 1</sup><sub>20</sub> t<sup>n - 1</sup><sub>21</sub> t<sup>n - 1</sup><sub>22</sub></i></b>

* **Field Data**. Field data is essentially an array of data arrays. Defining field data means giving a name to the field and specifying the number of arrays it contains. Then, for each array, the name of the array arrayName(i), the number of components of the array, numComponents, the number of tuples in the array, numTuples, and the data type, dataType, are defined.
<br><br>FIELD <b><i>dataName numArrays<br>
arrayName0 numComponents numTuples dataType<br>
f<sub>00</sub> f<sub>01</sub> ... f<sub>0(numComponents-1)</sub><br>
f<sub>10</sub> f<sub>11</sub> ... f<sub>1(numComponents-1)</sub><br>
...<br>
f<sub>(numTuples-1)0</sub> f<sub>(numTuples-1)1</sub> ... f<sub>(numTuples-1)(numComponents-1)</sub><br>
arrayName1 numComponents numTuples dataType<br>
f<sub>00</sub> f<sub>01</sub> ... f<sub>0(numComponents-1)</sub><br>
f<sub>10</sub> f<sub>11</sub> ... f<sub>1(numComponents-1)</sub><br>
...<br>
f<sub>(numTuples-1)0</sub> f<sub>(numTuples-1)1</sub> ... f<sub>(numTuples-1)(numComponents-1)</sub><br>
...<br>
arrayName(numArrays-1) numComponents numTuples dataType<br>
f<sub>00</sub> f<sub>01</sub> ... f<sub>0(numComponents-1)</sub><br>
f<sub>10</sub> f<sub>11</sub> ... f<sub>1(numComponents-1)</sub><br>
...<br>
f<sub>(numTuples-1)0</sub> f<sub>(numTuples-1)1</sub> ... f<sub>(numTuples-1)(numComponents-1)</sub></i></b>

## Legacy File Examples
The first example is a cube represented by six polygonal faces. We define a single-component scalar, normals, and field data on the six faces. There are scalar data associated with the eight vertices. A lookup table of eight colors, associated with the point scalars, is also defined.

```
# vtk DataFile Version 2.0
Cube example
ASCII
DATASET POLYDATA
POINTS 8 float
0.0 0.0 0.0
1.0 0.0 0.0
1.0 1.0 0.0
0.0 1.0 0.0
0.0 0.0 1.0
1.0 0.0 1.0
1.0 1.0 1.0
0.0 1.0 1.0
POLYGONS 6 30
4 0 1 2 3
4 4 5 6 7
4 0 1 5 4
4 2 3 7 6
4 0 4 7 3
4 1 2 6 5
CELL_DATA 6
SCALARS cell_scalars int 1
LOOKUP_TABLE default
0
1
2
3
4
5
NORMALS cell_normals float
0 0 -1
0 0 1
0 -1 0
0 1 0
-1 0 0
1 0 0
FIELD FieldData 2
cellIds 1 6 int
0 1 2 3 4 5
faceAttributes 2 6 float
0.0 1.0 1.0 2.0 2.0 3.0 3.0 4.0 4.0 5.0 5.0 6.0
POINT_DATA 8
SCALARS sample_scalars float 1
LOOKUP_TABLE my_table
0.0
1.0
2.0
3.0
4.0
5.0
6.0
7.0
LOOKUP_TABLE my_table 8
0.0 0.0 0.0 1.0
1.0 0.0 0.0 1.0
0.0 1.0 0.0 1.0
1.0 1.0 0.0 1.0
0.0 0.0 1.0 1.0
1.0 0.0 1.0 1.0
0.0 1.0 1.0 1.0
1.0 1.0 1.0 1.0
```

The next example is a volume of dimension 3 by 4 by 6. Since no lookup table is defined, either the user must create one in VTK, or the default lookup table will be used.

```
# vtk DataFile Version 2.0
Volume example
ASCII
DATASET STRUCTURED_POINTS
DIMENSIONS 3 4 6
ASPECT_RATIO 1 1 1
ORIGIN 0 0 0
POINT_DATA 72
SCALARS volume_scalars char 1
LOOKUP_TABLE default
0 0 0 0 0 0 0 0 0 0 0 0
0 5 10 15 20 25 25 20 15 10 5 0
0 10 20 30 40 50 50 40 30 20 10 0
0 10 20 30 40 50 50 40 30 20 10 0
0 5 10 15 20 25 25 20 15 10 5 0
0 0 0 0 0 0 0 0 0 0 0 0
```

The third example is an unstructured grid containing twelve of the nineteen VTK cell types (see Figure 2 and Figure 3).
Figure 2 shows all 16 of the linear cell types and was generated with the [LinearCellDemo](https://kitware.github.io/vtk-examples/site/Cxx/GeometricObjects/LinearCellDemo).
<figure>
  <img src="https://github.com/Kitware/vtk-examples/blob/gh-pages/src/Testing/Baseline/Cxx/GeometricObjects/TestLinearCellDemo.png?raw=true" width="640" alt="LinearCellDemo">
  <figcaption>Figure 2. - Linear cell types found in VTK. Use the include file vtkCellType.h to manipulate cell types..</figcaption>
</figure>

Figure 3 shows 16 of the non-linear cells and was generated with the [IsoparametricCellsDemo](https://kitware.github.io/vtk-examples/site/Cxx/GeometricObjects/IsoparametricCellsDemo).

<figure>
  <img src="https://github.com/Kitware/vtk-examples/blob/gh-pages/src/Testing/Baseline/Cxx/GeometricObjects/TestIsoparametricCellsDemo.png?raw=true" width="640" alt="LinearCellDemo">
  <figcaption>Figure 3. - Non-linear cell types found in VTK.</figcaption>
</figure>

The file contains scalar and vector data.
Figure 4 shows a presentation of this file generated by [ReadLegacyUnstructuredGrid](https://kitware.github.io/vtk-examples/site/Cxx/IO/ReadLegacyUnstructuredGrid/).
<figure>
  <img src="https://github.com/Kitware/vtk-examples/blob/gh-pages/src/Testing/Baseline/Cxx/IO/TestReadLegacyUnstructuredGrid.png?raw=true" width="640" alt="ReadLegacyReadLegacyUnstructuredGrid">
  <figcaption>Figure 4. - UnstructuredGrid example.</figcaption>
</figure>

```
# vtk DataFile Version 2.0
Unstructured Grid Example
ASCII
DATASET UNSTRUCTURED_GRID

POINTS 27 float
0 0 0  1 0 0  2 0 0  0 1 0  1 1 0  2 1 0
0 0 1  1 0 1  2 0 1  0 1 1  1 1 1  2 1 1
0 1 2  1 1 2  2 1 2  0 1 3  1 1 3  2 1 3
0 1 4  1 1 4  2 1 4  0 1 5  1 1 5  2 1 5
0 1 6  1 1 6  2 1 6

CELLS 11 60
8 0 1 4 3 6 7 10 9
8 1 2 4 5 7 8 10 11
4 6 10 9 12
4 11 14 10 13
6 15 16 17 14 13 12
6 18 15 19 16 20 17
4 22 23 20 19
3 21 22 18
3 22 19 18
2 26 25
1 24

CELL_TYPES 11
12
11
10
8
7
6
9
5
4
3
1

POINT_DATA 27
SCALARS scalars float 1
LOOKUP_TABLE default
0.0 1.0 2.0 3.0 4.0 5.0
6.0 7.0 8.0 9.0 10.0 11.0
12.0 13.0 14.0 15.0 16.0 17.0
18.0 19.0 20.0 21.0 22.0 23.0
24.0 25.0 26.0

VECTORS vectors float
1 0 0  1 1 0  0 2 0  1 0 0  1 1 0  0 2 0
1 0 0  1 1 0  0 2 0  1 0 0  1 1 0  0 2 0
0 0 1  0 0 1  0 0 1  0 0 1  0 0 1  0 0 1
0 0 1  0 0 1  0 0 1  0 0 1  0 0 1  0 0 1
0 0 1  0 0 1  0 0 1

CELL_DATA 11
SCALARS scalars float 1
LOOKUP_TABLE CellColors
0.0 1.0 2.0 3.0 4.0 5.0
6.0 7.0 8.0 9.0 10.0

LOOKUP_TABLE CellColors 11
.4 .4 1 1
.4 1 .4 1
.4 1 1 1
1 .4 .4 1
1 .4 1 1
1 1 .4 1
1 1 1 1
1 .5 .5 1
.5 1 .5 1
.5 .5 .5 1
1 .5 .4 1

```

The fourth and final example is data represented as a field. You may also wish to see "Working With Field Data" on [page 249](https://www.kitware.com/products/books/VTKUsersGuide.pdf#page=263) to see how to manipulate this data. The data file shown below can be found in its entirety [here](https://raw.githubusercontent.com/Kitware/vtk-examples/gh-pages/src/Testing/Data/financial.vtk).
The example [FinanceFieldData](https://kitware.github.io/vtk-examples/site/Cxx/Modelling/FinanceFieldData) generated Figure 5.
<figure>
  <img src="https://github.com/Kitware/vtk-examples/blob/gh-pages/src/Testing/Baseline/Cxx/Modelling/TestFinanceFieldData.png?raw=true" width="640" alt="FinanceFieldData">
  <figcaption>Figure 5. - Visualizing financial field data.</figcaption>
</figure>

```
# vtk DataFile Version 2.0
Financial data in vtk field format
ASCII
FIELD financialData 6
TIME_LATE 1 3188 float
29.14 0.00 0.00 11.71 0.00 0.00 0.00 0.00
...(more stuff — 3188 total values)...
MONTHLY_PAYMENT 1 3188 float
7.26 5.27 8.01 16.84 8.21 15.75 10.62 15.47
...(more stuff)...
UNPAID_PRINCIPLE 1 3188 float
430.70 380.88 516.22 1351.23 629.66 1181.97 888.91 1437.83
...(more stuff)...
LOAN_AMOUNT 1 3188 float
441.50 391.00 530.00 1400.00 650.00 1224.00 920.00 1496.00
...(more stuff)...
INTEREST_RATE 1 3188 float
13.875 13.875 13.750 11.250 11.875 12.875 10.625 10.500
...(more stuff)...
MONTHLY_INCOME 1 3188 unsigned_short
39 51 51 38 35 49 45 56
...(more stuff)...
```

In this example, a field is represented using six arrays. Each array has a single component and 3,188 tuples. Five of the six arrays are of type float, while the last array is of type unsigned_short. Additional examples are available in the data directory.
