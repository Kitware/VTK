
## Specifications

The `VTKHDF` file format is a file format relying on [HDF5](https://www.hdfgroup.org/solutions/hdf5/).
It is meant to provide good I/O performance as well as robust and flexible parallel I/O capabilities.

It currently supports: PolyData, UnstructuredGrid, ImageData, OverlappingAMR, MultiBlockDataSet and the
PartitionedDataSetCollection.

The current file format version is the **2.3**.

Note: This development is iterative and the format is expected to grow in
its support for more and more use cases.

### Changelog

#### VTKHDF - 2.4

- add specification for `HyperTreeGrid`

#### VTKHDF - 2.3

- fix array names which miss the `s` to be consistent with other temporal dataset in case of the temporal
OverlappingAMR. It concerns these data names: NumberOfBox, AMRBoxOffset, Point/Cell/FieldDataOffset.

#### VTKHDF - 2.2

- add support for temporal `OverlappingAMR`
- add official support for ignored data outside of `VTKHDF`

#### VTKHDF - 2.1

- add specification in the format for `PartitionedDataSetCollection` and `MultiBlockDataSet`

#### VTKHDF - 2.0

- extends the specification for `PolyData`.

- add support for `Temporal` dataset for `PolyData`, `ImageData` and `UnstructuredGrid`.

#### VTKHDF - 1.0

- add specification for these vtk data types:
  - `UnstructuredGrid`
  - `ImageData`
  - `Overlapping AMR`

### Extension

The ` VTKHDF` format generally uses the `.vtkhdf` extension. The `.hdf`
extension is also supported but is not preferred. There are no specific
extensions to differentiate between different types of dataset, serial
vs. distributed data or static vs. temporal data.

### General Specification

VTK HDF files start with a group called `VTKHDF` with two attributes:
`Version`, an array of two integers and `Type`, a string showing the
VTK dataset type stored in the file. Additional attributes can follow
depending on the dataset type. Currently, `Version`
is the array [2, 2] and `Type` can be `ImageData`, `PolyData`,
`UnstructuredGrid`, `OverlappingAMR`,  `PartitionedDataSetCollection` or
`MultiBlockDataSet`.

Top-level groups outside of /VTKHDF do not contain any information related
to VTK data model and are outside of the scope of this specification.
They can be useful to store meta-information that could be read and written
by custom VTKHDF implementations.

The data type for each HDF dataset is part of the dataset and it is
determined at write time. The reader matches the type of the dataset
with a `H5T_NATIVE_` type and creates the VTK array of that
type. Consequently, the type at writing might be different than the
type at reading even on the same machine because for instance `long`
can be the same type as `long long` or `int` can be the same as `long`
on certain platforms. Also, `vtkIdType` is read as the C++ type it
represents (`long` or `long long`). Endianness conversions are done
automatically.

In the diagrams that follow, showing the HDF file structure for VTK
datasets, the rounded blue rectangles are HDF groups and the gray
rectangles are HDF datasets. Each rectangle shows the name of the
group or dataset in bold font and the attributes underneath with
regular font.

### Image data

The format for image data is detailed in the Figure 6 where the `Type`
attribute of the `VTKHDF` group is `ImageData`.  An
ImageData (regular grid) is not split into partitions for parallel
processing. We rely on the writer to chunk the data to optimize
reading for a certain number of MPI ranks. Attribute data is stored in
a PointData or CellData array using hyper slabs. `WholeExtent`,
`Origin`, `Spacing` and `Direction` attributes have the same meaning
as the corresponding attributes for the vtkImageData dataset. `Scalars`,
`Vectors`, ... string attributes for the `PointData` and `CellData`
groups specify the active attributes in the dataset.

<figure>
  <img src="https://raw.githubusercontent.com/Kitware/vtk-examples/gh-pages/src/VTKFileFormats/Figures/vtkhdf-image-data.svg" width="640" alt="Image data VTKHDF File Format">
  <figcaption>Figure 6. - Image data VTKHDF File Format</figcaption>
</figure>

### Unstructured grid

The format for unstructured grid is shown in Figure 7. In this case
the `Type` attribute of the `VTKHDF` group is `UnstructuredGrid`.
The unstructured grid is split into partitions, with a partition for
each MPI rank. This is reflected in the HDF5 file structure. Each HDF
dataset is obtained by concatenating the data for each partition. The
offset O(i) where we store the data for partition i is computed using:

O(i) = S(0) + ... + S(i-1), i > 1 with O(0) = 0.

where S(i) is the size of partition i.

We describe the split into partitions using HDF5 datasets
`NumberOfConnectivityIds`, `NumberOfPoints` and `NumberOfCells`. Let n
be the number of partitions which usually correspond to the number of
the MPI ranks. `NumberOfConnectivityIds` has size n where
NumberOfConnectivityIds[i] represents the size of the `Connectivity`
array for partition i. `NumberOfPoints` and `NumberOfCells` are arrays
of size n, where NumberOfPoints[i] and NumberOfCells[i] are the number
of points and number of cells for partition i. The `Points` array
contains the points of the VTK dataset. `Offsets` is an array of size
∑ (S(i) + 1), where S(i) is the number of cells in partition i, indicating the index in
the `Connectivity` array where each cell's points start.
`Connectivity` stores the lists of point ids for each cell, and
`Types` contain the cell information stored as described in
vtkCellArray documentation. Data for each partition is appended in a
HDF dataset for `Points`, `Connectivity`, `Offsets`, `Types`,
`PointData` and `CellData`. We can compute the size of partition i
using the following formulas:

|   | Size of partition i |
|:--|:--|
| Points  | NumberOfPoints[i] * 3 * sizeof(Points[0][0])  |
| Connectivity  | NumberOfConnectivityIds[i] * sizeof(Connectivity[0]) |
| Offsets  | (NumberOfCells[i] + 1) * sizeof(Offsets[0]) |
| Types  | NumberOfCells[i] * sizeof(Types[i]) |
| PointData  | NumberOfPoints[i] * sizeof(point_array_k[0]) |
| CellData | NumberOfCells[i] * sizeof(cell_array_k[0]) |

<figure>
  <img src="https://raw.githubusercontent.com/Kitware/vtk-examples/gh-pages/src/VTKFileFormats/Figures/vtkhdf-unstructured-grid.svg" width="640" alt="Unstructured Grid VTKHDF File Format">
  <figcaption>Figure 7. - Unstructured grid VTKHDF File Format</figcaption>
</figure>

To read the data for its rank a node reads the information about all
partitions, compute the correct offset and then read data from that
offset.

### Poly data

The format for poly data is shown in Figure 8. In this case
the `Type` attribute of the `VTKHDF` group is `PolyData`.
The poly data is split into partitions, with a partition for
each MPI rank. This is reflected in the HDF5 file structure. Each HDF
dataset is obtained by concatenating the data for each partition. The
offset O(i) where we store the data for partition i is computed using:

O(i) = S(0) + ... + S(i-1), i > 1 with O(0) = 0.

where S(i) is the size of partition i. This is very similar to and
completely inspired by the `UnstructuredGrid` format.

The split into partitions of the point coordinates is exactly the same
as in the `UnstructuredGrid` format above. However, the split into
partitions of each of the category of cells (`Vertices`, `Lines`,
`Polygons` and `Strips`) using HDF5 datasets `NumberOfConnectivityIds`
and `NumberOfCells`. Let n be the number of partitions which usually
correspond to the number of the MPI ranks. `{CellCategory}/NumberOfConnectivityIds` has
size n where NumberOfConnectivityIds[i] represents the size of the `{CellCategory}/Connectivity`
array for partition i. `NumberOfPoints` and `{CellCategory}/NumberOfCells` are arrays
of size n, where NumberOfPoints[i] and {CellCategory}/NumberOfCells[i] are the number
of points and number of cells for partition i. The `Points` array
contains the points of the VTK dataset. `{CellCategory}/Offsets` is an array of size
∑ (S(i) + 1), where S(i) is the number of cells in partition i, indicating the index in
the `{CellCategory}/Connectivity` array where each cell's points start.
`{CellCategory}/Connectivity` stores the lists of point ids for each cell.
Data for each partition is appended in a HDF dataset for `Points`, `Connectivity`, `Offsets`,
`PointData` and `CellData`. We can compute the size of partition i
using the following formulas:

|   | Size of partition i |
|:--|:--|
| Points  | NumberOfPoints[i] * 3 * sizeof(Points[0][0])  |
| {CellCategory}/Connectivity  | {CellCategory}/NumberOfConnectivityIds[i] * sizeof({CellCategory}/Connectivity[0]) |
| {CellCategory}/Offsets  | ({CellCategory}/NumberOfCells[i] + 1) * sizeof({CellCategory}/Offsets[0]) |
| PointData  | NumberOfPoints[i] * sizeof(point_array_k[0]) |
| CellData | (∑j {CellCategory_j}/NumberOfCells[i]) * sizeof(cell_array_k[0]) |


```{figure} vtkhdf_images/poly_data_hdf_schema.png
:width: 640px
:align: center

Figure 8. - Poly Data VTKHDF File Format
```

To read the data for its rank a node reads the information about all
partitions, compute the correct offset and then read data from that
offset.

### Overlapping AMR

The format for Overlapping AMR is shown in Figure 9. In this case
the `Type` attribute of the `VTKHDF` group is `OverlappingAMR`.
The mandatory `Origin` parameter is a double triplet that defines
the global origin of the AMR data set.
Each level in an overlapping AMR file format (and data structure)
consists of a list of uniform grids with the same spacing from the
Spacing attribute. The Spacing attribute is a list a three doubles
describing the spacing in each x/y/z direction.
The AMRBox dataset contains the bounding box
for each of these grids. Each line in this dataset is expected to contain
6 integers describing the indexed bounds in i, j, k space
(imin/imax/jmin/jmax/kmin/kmax).
The points and cell arrays for these grids are
stored serialized in one dimension and stored in a dataset in the
PointData or CellData group.

<figure>
  <img src="https://raw.githubusercontent.com/Kitware/vtk-examples/gh-pages/src/VTKFileFormats/Figures/vtkhdf-overlapping-amr.svg" width="640" alt="Overlapping AMR VTKHDF File Format">
  <figcaption>Figure 9. - Overlapping AMR VTKHDF File Format</figcaption>
</figure>

### HyperTreeGrid

The schema for the tree-based AMR HyperTreeGrid VTKHDF specification is shown below.
This specification is very different from the ones mentioned above, because its topology is defined as a grid of refined trees.

Root attribute `Dimensions` defines the dimension of the grid. For a `N * M * P` grid, there are a total of `(N - 1) * (M - 1) * (P - 1)` trees.
Coordinates arrays `XCoordinates` (size `N`), `YCoordinates` (size `M`) and `ZCoordinates` (size `P`) define the size of trees in each direction.
Their value can change over time.
The `BranchFactor` attribute defines the subdivision factor used for tree decomposition.

HyperTrees are defined from a bit array describing tree decomposition, level by level. For each tree, the `Descriptor` dataset has one bit for each cell in the tree, except for its deepest level: 0 if the cell is not refined, and 1 if it is. The descriptor does not describe its deepest level, because we know that no cell is ever refined.

Each cell can be masked using an optional bit array `Mask`. A decomposed (refined) cell cannot be masked.
The `Descriptors` and `Mask` datasets are packed bit arrays, stored as unsigned chars.

Each new piece is required to start writing descriptors and mask on a new byte, even if the previous byte was not completely used, except if the previous array has a size of 0.
`DescriptorsSize` stores the size (in **bits**) of the descriptors array for each piece.

`DepthPerTree` contains the depth of each tree listed in `TreeIds` the current piece.
The size of both arrays is `NumberOfTrees`, indexed at the piece id.

`NumberOfCellsPerTreeDepth`'s size is the sum of `DepthPerTree`.
For each depth of each tree, it gives the information of the number of cells for this depth.
For a given piece, we store the size of this dataset as `NumberOfDepths`.

The number of cells for the piece is stored as `NumberOfCells`.

The size of the (optional) `Mask` dataset corresponds to the number of cells divided by 8 (because of bit-packed storage),
rounded to the next bigger integer value.

For HyperTreeGrids, edges cannot store information.
This means there can be a `CellData` group containing cell fields, but no `PointData`.

Optionally, `InterfaceNormalsName` and `InterfaceInterceptsName` root attributes can be set to existing cell array names to define HyperTreeGrid interfaces,
used for interpolation at render time.

For temporal HyperTreeGrids, the "Steps" group contains read offsets into the `DepthPerTree`,
`NumberOfCellsPerTreeDepth`, `TreeIds`, `Mask` `Descriptors` and coordinate datasets for each timestep.

If some values do not change over time (for example coordinates),
you can set the offset to the same value as the previous timestep (O),and store data only once.

Note that for `Mask` and `Descriptors`, the offset is in **bytes** (unlike `DescriptorSize` which is in bits),
because each new piece starts on a new byte, except if it does not contain any value.

```{figure} vtkhdf_images/hypertreegrid_schema.jpg
:width: 640px
:align: center

Figure 10. - HyperTreeGrid VTKHDF File Format
```

### PartitionedDataSetCollection and MultiBlockDataSet

VTKHDF supports composite types, made of multiple datasets of simple types, organised as a tree.
The format currently supports vtkPartitionedDataSetCollection (PDC) and vtkMultiBlockDataSet (MB) composite types, as shown in Figure 11.
The `Type` attribute of the `VTKHDF` group for them should be either `PartitionedDataSetCollection` or `MultiBlockDataSet`.

All simple (non composite) datasets are located in the root `VTKHDF` group, with a unique block name.
These blocks can have any `Type` specified above, or be empty blocks when no `Type` is specified.
These top-level data blocks should not be composite themselves : they can only be simple or partitioned (multi-piece) types.
For temporal datasets, all blocks should have the same number of time steps and time values.

Then, dataset tree hierarchy is defined in the `Assembly` group, which is also a direct child of the `VTKHDF` group.
Sub-groups in the `Assembly` group define the dataset(s) they contain using a [HDF5 symbolic link](https://davis.lbl.gov/Manuals/HDF5-1.8.7/UG/09_Groups.html#HardAndSymbolicLinks)
to the top-level datasets. The name of the link in the assembly will be the actual name of the block when read.
Any group can have multiple children that are either links to datasets, or nodes that define datasets deeper in the hierarchy.

The Assembly group and its children need to track creation order to be able to keep subtrees ordered.
For this, you need to set H5G properties `H5P_CRT_ORDER_TRACKED` and `H5P_CRT_ORDER_INDEXED` on each group when writing the Assembly.

While both MB and PDC share a common structure, there is still a slight distinction in the format between them.
For PDC, a group in the assembly that is not a softlink represents a node in the vtkDataAssembly associated to it, and
a softlink represents a dataset index associated to its parent node (similar to what the function `AddDataSetIndex` does in `vtkDataAssembly`).
This way, a single dataset can be used multiple times in the assembly without any additional storage cost.
Top-level datasets need to set an `Index` attribute to specify their index in the PDC flat structure.

On the other hand, MB structures work a little differently. First, they don't need no index for their datasets, and
secondly, an assembly node that is not a softlink represents a nested `vtkMultiBlockDataSet`.
A softlink in the assembly represents a dataset nested in its parent `vtkMultiBlockDataSet`.
Again, this MB format can save space when a block is referenced multiple times.

```{figure} vtkhdf_images/partitioned_dataset_collection_hdf_schema.png
:width: 640px
:align: center

Figure 11. - PartitionedDataSetCollection/MultiBlockDataset VTKHDF File Format
```

### Temporal Data

The generic format for all `VTKHDF` temporal data is shown in Figure 12.
The general idea is to take the static formats described above and use them
as a base to append all the time dependent data. As such, a file holding static
data has a very similar structure to a file holding dynamic data. An additional
`Steps` subgroup is added to the `VTKHDF` main group holding offset information
for each of the time steps as well as the time values. The choice to include offset
information as HDF5 datasets was made to reduce the quantity of meta-data in the
file to improve performance. This `Steps` group has one integer like attribute
`NSteps` indicating the number of steps in the temporal dataset.

The `Steps` group is structured as follows:
* `Values` [dim = (NSteps)]: each entry indicates the time value for the associated
time step.
* `PartOffsets` [dims = (NSteps)]: each entry indicates at which part offset to
start reading the associated time step (relevant for `Unstructured Grid`s and
`Poly Data`).
* `NumberOfParts` [dims = (NSteps)]: each entry indicates how many parts the
associated time step has (relevant for `Unstructured Grid`s and `Poly Data`). This
information is optional if there is a constant number of parts per time steps and the
length of `VTKHDF/NumberOfPoints` is equal to `NumberOfPartsPerTimeStep x NSteps`.
* `PointOffsets` [dims = (NSteps)]: each entry indicates where in the `VTKHDF/Points`
data set to start reading point coordinates for the associated time step (relevant for
`Unstructured Grid` and `Poly Data`).
* `CellOffsets` [dims = (NSteps, NTopologies)]: each entry indicates by how many cells
to offset reading into the connectivity offset structures for the associated time step
(relevant for `Unstructured Grid` and `Poly Data`).
  * `Unstructured Grid`s only have one set of connectivity data and NTopologies = 1.
  *  `Poly Data`, however, have `Vertices`,`Lines`, `Polygons` and `Strips` in that order
     and therefore NTopologies = 4.
* `ConnectivityIdOffsets` [dims = (NSteps, NTopologies)]: each entry indicates by how many
values to offset reading into the connectivity indexing structures for the associated time
step (relevant for `Unstructured Grid` and `Poly Data`).
  * `Unstructured Grid`s only have one set of connectivity data and NTopologies = 1.
  *  `Poly Data`, however, have `Vertices`,`Lines`, `Polygons` and `Strips` in that order
     and therefore NTopologies = 4.
* `{Point,Cell,Field}DataOffsets/{ArrayName}` [dims = (NSteps)]: each entry indicates by how
many values to offset reading into the given array for the associated time step. In the
absence of a data set, the appropriate geometry offsetting for the time step is used in its
place.
* `FieldDataSizes/{ArrayName}` [dims = (NSteps, 2)]: each entry indicates the field data
component and tuple size. In the absence of a data set, the maximum number of components
and one tuple per step are considered.

```{figure} vtkhdf_images/transient_hdf_schema.png
:width: 640px
:align: center

Figure 12. - Temporal Data VTKHDF File Format
```

Writing incrementally to `VTKHDF` temporal datasets is relatively straightforward using the
appending functionality of `HDF5` chunked data sets
([Chunking in HDF5](https://davis.lbl.gov/Manuals/HDF5-1.8.7/Advanced/Chunking/index.html)).

#### Particularity regarding ImageData

A particularity of temporal `Image Data` in the format is that the reader expects an additional
prepended dimension considering the time to be the first dimension in the multidimensional arrays.
As such, arrays described in temporal `Image Data` should have dimensions ordered as
`(time, z, y, x)`.

#### Particularity regarding OverlappingAMR

Currently only `AMRBox` and `Point/Cell/Field data` can be temporal, not the `Spacing`. Due to the
structure of the OverlappingAMR format, the format specify an intermediary group between the `Steps`
group and the `Point/Cell/FieldDataOffsets` group named `LevelX` for each level where `X` is the
number of level. These `Level` groups will also contain 2 other datasets to retrieve the `AMRBox`:

- `AMRBoxOffsets` : each entry indicates by how many AMR box to offset reading into the `AMRBox`.
- `NumberOfAMRBoxes` : the number of boxes contained in the `AMRBox` for each timestep.

```{figure} vtkhdf_images/temporal_overlapping_amr_hdf_schema.png
:width: 640px
:align: center

Figure 12. - Temporal OverlappingAMR VTKHDF File Format
```

### Limitations

This specification and the reader available in VTK currently only
supports ImageData, UnstructuredGrid, PolyData, Overlapping AMR, MultiBlockDataSet and Partitioned
DataSet Collection. Other dataset types may be added later depending on interest and funding.

Unlike XML formats, VTKHDF does not support field names containing `/` and `.` characters,
because of a limitation in the HDF5 format specification.

### Examples

We present three examples of VTK HDF files, shown using h5dump -A one
image file, one unstructured grid and one overlapping AMR.
These files can be examined in the VTK source code, by building VTK
and enabling testing (`VTK_BUILD_TESTING`). The two files are in the build directory
ExternalData at `Testing/Data/mandelbrot-vti.hdf` for the ImageData
and at `Testing/Data/can-pvtu.hdf` for the partitioned UnstructuredGrid
and `Testing/Data/amr_gaussian_pulse.hdf` for the overlapping AMR.

#### ImageData

The image data file is a wavelet source produced in ParaView. Note
that we don't partition image data, so the same format is used for
serial and parallel processing.

```
HDF5 "ExternalData/Testing/Data/mandelbrot-vti.hdf" {
GROUP "/" {
   GROUP "VTKHDF" {
      ATTRIBUTE "Direction" {
         DATATYPE  H5T_IEEE_F64LE
         DATASPACE  SIMPLE { ( 9 ) / ( 9 ) }
         DATA {
         (0): 1, 0, 0, 0, 1, 0, 0, 0, 1
         }
      }
      ATTRIBUTE "Origin" {
         DATATYPE  H5T_IEEE_F64LE
         DATASPACE  SIMPLE { ( 3 ) / ( 3 ) }
         DATA {
         (0): -1.75, -1.25, 0
         }
      }
      ATTRIBUTE "Spacing" {
         DATATYPE  H5T_IEEE_F64LE
         DATASPACE  SIMPLE { ( 3 ) / ( 3 ) }
         DATA {
         (0): 0.131579, 0.125, 0.0952381
         }
      }
      ATTRIBUTE "Type" {
         DATATYPE  H5T_STRING {
            STRSIZE 9;
            STRPAD H5T_STR_NULLPAD;
            CSET H5T_CSET_ASCII;
            CTYPE H5T_C_S1;
         }
         DATASPACE  SCALAR
         DATA {
         (0): "ImageData"
         }
      }
      ATTRIBUTE "Version" {
         DATATYPE  H5T_STD_I64LE
         DATASPACE  SIMPLE { ( 2 ) / ( 2 ) }
         DATA {
         (0): 1, 0
      }
      ATTRIBUTE "WholeExtent" {
         DATATYPE  H5T_STD_I64LE
         DATASPACE  SIMPLE { ( 6 ) / ( 6 ) }
         DATA {
         (0): 0, 19, 0, 20, 0, 21
         }
      }
      GROUP "PointData" {
         ATTRIBUTE "Scalars" {
            DATATYPE  H5T_STRING {
               STRSIZE 18;
               STRPAD H5T_STR_NULLPAD;
               CSET H5T_CSET_ASCII;
               CTYPE H5T_C_S1;
            }
            DATASPACE  SCALAR
            DATA {
            (0): "IterationsGradient"
            }
         }
         DATASET "Iterations" {
            DATATYPE  H5T_IEEE_F32LE
            DATASPACE  SIMPLE { ( 22, 21, 20 ) / ( 22, 21, 20 ) }
         }
         DATASET "IterationsGradient" {
            DATATYPE  H5T_IEEE_F64LE
            DATASPACE  SIMPLE { ( 22, 21, 20, 3 ) / ( 22, 21, 20, 3 ) }
         }
         DATASET "Iterations_double" {
            DATATYPE  H5T_IEEE_F64LE
            DATASPACE  SIMPLE { ( 22, 21, 20 ) / ( 22, 21, 20 ) }
         }
         DATASET "point_index_llong" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 22, 21, 20 ) / ( 22, 21, 20 ) }
         }
         DATASET "xextent_int" {
            DATATYPE  H5T_STD_I32LE
            DATASPACE  SIMPLE { ( 22, 21, 20 ) / ( 22, 21, 20 ) }
         }
         DATASET "xextent_long" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 22, 21, 20 ) / ( 22, 21, 20 ) }
         }
         DATASET "xextent_uint" {
            DATATYPE  H5T_STD_U32LE
            DATASPACE  SIMPLE { ( 22, 21, 20 ) / ( 22, 21, 20 ) }
         }
         DATASET "xextent_ulong" {
            DATATYPE  H5T_STD_U64LE
            DATASPACE  SIMPLE { ( 22, 21, 20 ) / ( 22, 21, 20 ) }
         }
      }
   }
}
}
```

#### UnstructuredGrid

The unstructured grid is the can example (only the can, not the brick) from ParaView, partitioned in three:

```
HDF5 "ExternalData/Testing/Data/can-pvtu.hdf" {
GROUP "/" {
   GROUP "VTKHDF" {
      ATTRIBUTE "Type" {
         DATATYPE  H5T_STRING {
            STRSIZE 16;
            STRPAD H5T_STR_NULLPAD;
            CSET H5T_CSET_ASCII;
            CTYPE H5T_C_S1;
         }
         DATASPACE  SCALAR
         DATA {
         (0): "UnstructuredGrid"
         }
      }
      ATTRIBUTE "Version" {
         DATATYPE  H5T_STD_I64LE
         DATASPACE  SIMPLE { ( 2 ) / ( 2 ) }
         DATA {
         (0): 1, 0
         }
      }
      GROUP "CellData" {
         DATASET "EQPS" {
            DATATYPE  H5T_IEEE_F64LE
            DATASPACE  SIMPLE { ( 5480 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "vtkGhostType" {
            DATATYPE  H5T_STD_U8LE
            DATASPACE  SIMPLE { ( 5480 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "vtkOriginalCellIds" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 5480 ) / ( H5S_UNLIMITED ) }
         }
      }
      DATASET "Connectivity" {
         DATATYPE  H5T_STD_I64LE
         DATASPACE  SIMPLE { ( 43840 ) / ( H5S_UNLIMITED ) }
      }
      GROUP "FieldData" {
         DATASET "ElementBlockIds" {
            DATATYPE  H5T_STD_I32LE
            DATASPACE  SIMPLE { ( 1 ) / ( 1 ) }
         }
         DATASET "Info_Records" {
            DATATYPE  H5T_STD_I8LE
            DATASPACE  SIMPLE { ( 4, 81 ) / ( 4, 81 ) }
         }
         DATASET "KE" {
            DATATYPE  H5T_IEEE_F64LE
            DATASPACE  SIMPLE { ( 44 ) / ( 44 ) }
         }
         DATASET "NSTEPS" {
            DATATYPE  H5T_IEEE_F64LE
            DATASPACE  SIMPLE { ( 44 ) / ( 44 ) }
         }
         DATASET "QA_Records" {
            DATATYPE  H5T_STD_I8LE
            DATASPACE  SIMPLE { ( 24, 33 ) / ( 24, 33 ) }
         }
         DATASET "TMSTEP" {
            DATATYPE  H5T_IEEE_F64LE
            DATASPACE  SIMPLE { ( 44 ) / ( 44 ) }
         }
         DATASET "TimeValue" {
            DATATYPE  H5T_IEEE_F64LE
            DATASPACE  SIMPLE { ( 1 ) / ( 1 ) }
         }
         DATASET "Title" {
            DATATYPE  H5T_STRING {
               STRSIZE H5T_VARIABLE;
               STRPAD H5T_STR_NULLTERM;
               CSET H5T_CSET_ASCII;
               CTYPE H5T_C_S1;
            }
            DATASPACE  SIMPLE { ( 1 ) / ( 1 ) }
         }
         DATASET "XMOM" {
            DATATYPE  H5T_IEEE_F64LE
            DATASPACE  SIMPLE { ( 44 ) / ( 44 ) }
         }
         DATASET "YMOM" {
            DATATYPE  H5T_IEEE_F64LE
            DATASPACE  SIMPLE { ( 44 ) / ( 44 ) }
         }
         DATASET "ZMOM" {
            DATATYPE  H5T_IEEE_F64LE
            DATASPACE  SIMPLE { ( 44 ) / ( 44 ) }
         }
      }
      DATASET "NumberOfCells" {
         DATATYPE  H5T_STD_I64LE
         DATASPACE  SIMPLE { ( 3 ) / ( 3 ) }
      }
      DATASET "NumberOfConnectivityIds" {
         DATATYPE  H5T_STD_I64LE
         DATASPACE  SIMPLE { ( 3 ) / ( 3 ) }
      }
      DATASET "NumberOfPoints" {
         DATATYPE  H5T_STD_I64LE
         DATASPACE  SIMPLE { ( 3 ) / ( 3 ) }
      }
      DATASET "Offsets" {
         DATATYPE  H5T_STD_I64LE
         DATASPACE  SIMPLE { ( 5483 ) / ( H5S_UNLIMITED ) }
      }
      GROUP "PointData" {
         DATASET "ACCL" {
            DATATYPE  H5T_IEEE_F64LE
            DATASPACE  SIMPLE { ( 8076, 3 ) / ( H5S_UNLIMITED, 3 ) }
         }
         DATASET "DISPL" {
            DATATYPE  H5T_IEEE_F64LE
            DATASPACE  SIMPLE { ( 8076, 3 ) / ( H5S_UNLIMITED, 3 ) }
         }
         DATASET "GlobalNodeId" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 8076 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "PedigreeNodeId" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 8076 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "VEL" {
            DATATYPE  H5T_IEEE_F64LE
            DATASPACE  SIMPLE { ( 8076, 3 ) / ( H5S_UNLIMITED, 3 ) }
         }
         DATASET "vtkGhostType" {
            DATATYPE  H5T_STD_U8LE
            DATASPACE  SIMPLE { ( 8076 ) / ( H5S_UNLIMITED ) }
         }
      }
      DATASET "Points" {
         DATATYPE  H5T_IEEE_F64LE
         DATASPACE  SIMPLE { ( 8076, 3 ) / ( H5S_UNLIMITED, 3 ) }
      }
      DATASET "Types" {
         DATATYPE  H5T_STD_U8LE
         DATASPACE  SIMPLE { ( 5480 ) / ( H5S_UNLIMITED ) }
      }
   }
}
}
```

#### PolyData

The poly data is the `test_poly_data.hdf` from the `VTK` testing data:

```
HDF5 "ExternalData/Testing/Data/test_poly_data.hdf" {
GROUP "/" {
   GROUP "VTKHDF" {
      ATTRIBUTE "Type" {
         DATATYPE  H5T_STRING {
            STRSIZE 8;
            STRPAD H5T_STR_NULLPAD;
            CSET H5T_CSET_ASCII;
            CTYPE H5T_C_S1;
         }
         DATASPACE  SCALAR
      }
      ATTRIBUTE "Version" {
         DATATYPE  H5T_STD_I64LE
         DATASPACE  SIMPLE { ( 2 ) / ( 2 ) }
      }
      GROUP "CellData" {
         DATASET "Materials" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 816 ) / ( H5S_UNLIMITED ) }
         }
      }
      GROUP "Lines" {
         DATASET "Connectivity" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 0 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "NumberOfCells" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 2 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "NumberOfConnectivityIds" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 2 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "Offsets" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 2 ) / ( H5S_UNLIMITED ) }
         }
      }
      DATASET "NumberOfPoints" {
         DATATYPE  H5T_STD_I64LE
         DATASPACE  SIMPLE { ( 2 ) / ( H5S_UNLIMITED ) }
      }
      GROUP "PointData" {
         DATASET "Normals" {
            DATATYPE  H5T_IEEE_F32LE
            DATASPACE  SIMPLE { ( 412, 3 ) / ( H5S_UNLIMITED, 3 ) }
         }
         DATASET "Warping" {
            DATATYPE  H5T_IEEE_F32LE
            DATASPACE  SIMPLE { ( 412, 3 ) / ( H5S_UNLIMITED, 3 ) }
         }
      }
      DATASET "Points" {
         DATATYPE  H5T_IEEE_F32LE
         DATASPACE  SIMPLE { ( 412, 3 ) / ( H5S_UNLIMITED, 3 ) }
      }
      GROUP "Polygons" {
         DATASET "Connectivity" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 2448 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "NumberOfCells" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 2 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "NumberOfConnectivityIds" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 2 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "Offsets" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 818 ) / ( H5S_UNLIMITED ) }
         }
      }
      GROUP "Strips" {
         DATASET "Connectivity" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 0 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "NumberOfCells" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 2 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "NumberOfConnectivityIds" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 2 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "Offsets" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 2 ) / ( H5S_UNLIMITED ) }
         }
      }
      GROUP "Vertices" {
         DATASET "Connectivity" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 0 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "NumberOfCells" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 2 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "NumberOfConnectivityIds" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 2 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "Offsets" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 2 ) / ( H5S_UNLIMITED ) }
         }
      }
   }
}
}
```

#### Overlapping AMR

The Overlapping AMR data file is an AMR Guaussian Pulse source with two levels
(0 and 1), describing one Point Data, several Cell Data and a Field Data. Actual
`Data` are not displayed for readability.

```
HDF5 "ExternalData/Testing/Data/amr_gaussian_pulse.hdf" {
GROUP "/" {
   GROUP "VTKHDF" {
      ATTRIBUTE "Origin" {
         DATATYPE  H5T_IEEE_F64LE
         DATASPACE  SIMPLE { ( 3 ) / ( 3 ) }
         DATA {
         (0): -2, -2, 0
         }
      }
      ATTRIBUTE "Type" {
         DATATYPE  H5T_STRING {
            STRSIZE 14;
            STRPAD H5T_STR_NULLPAD;
            CSET H5T_CSET_ASCII;
            CTYPE H5T_C_S1;
         }
         DATASPACE  SCALAR
         DATA {
         (0): "OverlappingAMR"
         }
      }
      ATTRIBUTE "Version" {
         DATATYPE  H5T_STD_I64LE
         DATASPACE  SIMPLE { ( 2 ) / ( 2 ) }
         DATA {
         (0): 1, 0
         }
      }
      GROUP "Level0" {
         ATTRIBUTE "Spacing" {
            DATATYPE  H5T_IEEE_F64LE
            DATASPACE  SIMPLE { ( 3 ) / ( 3 ) }
            DATA {
            (0): 0.5, 0.5, 0.5
            }
         }
         DATASET "AMRBox" {
            DATATYPE  H5T_STD_I32LE
            DATASPACE  SIMPLE { ( 1, 6 ) / ( 1, 6 ) }
            DATA {
            (0,0): 0, 4, 0, 4, 0, 4
            }
         }
         GROUP "CellData" {
            DATASET "Centroid" {
               DATATYPE  H5T_IEEE_F64LE
               DATASPACE  SIMPLE { ( 125, 3 ) / ( 125, 3 ) }
            }
            DATASET "Gaussian-Pulse" {
               DATATYPE  H5T_IEEE_F64LE
               DATASPACE  SIMPLE { ( 125 ) / ( 125 ) }
            }
            DATASET "vtkGhostType" {
               DATATYPE  H5T_STD_U8LE
               DATASPACE  SIMPLE { ( 125 ) / ( 125 ) }
            }
         }
         GROUP "FieldData" {
            DATASET "KE" {
               DATATYPE  H5T_IEEE_F64LE
               DATASPACE  SIMPLE { ( 44 ) / ( 44 ) }
            }
         }
         GROUP "PointData" {
            DATASET "Coord Result" {
               DATATYPE  H5T_IEEE_F64LE
               DATASPACE  SIMPLE { ( 216 ) / ( 216 ) }
            }
         }
      }
      GROUP "Level1" {
         ATTRIBUTE "Spacing" {
            DATATYPE  H5T_IEEE_F64LE
            DATASPACE  SIMPLE { ( 3 ) / ( 3 ) }
            DATA {
            (0): 0.25, 0.25, 0.25
            }
         }
         DATASET "AMRBox" {
            DATATYPE  H5T_STD_I32LE
            DATASPACE  SIMPLE { ( 2, 6 ) / ( 2, 6 ) }
            DATA {
            (0,0): 0, 3, 0, 5, 0, 9,
            (1,0): 6, 9, 4, 9, 0, 9
            }
         }
         GROUP "CellData" {
            DATASET "Centroid" {
               DATATYPE  H5T_IEEE_F64LE
               DATASPACE  SIMPLE { ( 480, 3 ) / ( 480, 3 ) }
            }
            DATASET "Gaussian-Pulse" {
               DATATYPE  H5T_IEEE_F64LE
               DATASPACE  SIMPLE { ( 480 ) / ( 480 ) }
            }
            DATASET "vtkGhostType" {
               DATATYPE  H5T_STD_U8LE
               DATASPACE  SIMPLE { ( 480 ) / ( 480 ) }
            }
         }
         GROUP "FieldData" {
            DATASET "KE" {
               DATATYPE  H5T_IEEE_F64LE
               DATASPACE  SIMPLE { ( 88 ) / ( 88 ) }
            }
         }
         GROUP "PointData" {
            DATASET "Coord Result" {
               DATATYPE  H5T_IEEE_F64LE
               DATASPACE  SIMPLE { ( 770 ) / ( 770 ) }
            }
         }
      }
   }
}
}
```

#### PartitionedDataSetCollection

This partitioned dataset collection has 2 blocks, one unstructured grid (Block1) and one polydata (Block0).
Its assembly has 3 elements and no nesting, referencing one of the 2 blocks using symbolic links

```
HDF5 "composite.hdf" {
GROUP "/" {
   GROUP "VTKHDF" {
      ATTRIBUTE "Type" {
         DATATYPE  H5T_STRING {
            STRSIZE 28;
            STRPAD H5T_STR_NULLTERM;
            CSET H5T_CSET_ASCII;
            CTYPE H5T_C_S1;
         }
         DATASPACE  SCALAR
      }
      ATTRIBUTE "Version" {
         DATATYPE  H5T_STD_I64LE
         DATASPACE  SIMPLE { ( 2 ) / ( 2 ) }
      }
      GROUP "Assembly" {
         GROUP "blockName0" {
            SOFTLINK "Block0" {
               LINKTARGET "/VTKHDF/Block0"
            }
         }
         GROUP "blockName2" {
            SOFTLINK "Block1" {
               LINKTARGET "/VTKHDF/Block1"
            }
         }
         GROUP "groupName0" {
            GROUP "blockName1" {
               SOFTLINK "Block1" {
                  LINKTARGET "/VTKHDF/Block1"
               }
            }
         }
      }
      GROUP "Block0" {
         ATTRIBUTE "Index" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SCALAR
         }
         ATTRIBUTE "Type" {
            DATATYPE  H5T_STRING {
               STRSIZE 8;
               STRPAD H5T_STR_NULLTERM;
               CSET H5T_CSET_ASCII;
               CTYPE H5T_C_S1;
            }
            DATASPACE  SCALAR
         }
         ATTRIBUTE "Version" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 2 ) / ( 2 ) }
         }
         GROUP "CellData" {
            DATASET "Materials" {
               DATATYPE  H5T_STD_I64LE
               DATASPACE  SIMPLE { ( 96 ) / ( 96 ) }
            }
         }
         GROUP "Lines" {
            DATASET "Connectivity" {
               DATATYPE  H5T_STD_I64LE
               DATASPACE  SIMPLE { ( 0 ) / ( 0 ) }
            }
            DATASET "NumberOfCells" {
               DATATYPE  H5T_STD_I64LE
               DATASPACE  SIMPLE { ( 1 ) / ( 1 ) }
            }
            DATASET "NumberOfConnectivityIds" {
               DATATYPE  H5T_STD_I64LE
               DATASPACE  SIMPLE { ( 1 ) / ( 1 ) }
            }
            DATASET "Offsets" {
               DATATYPE  H5T_STD_I64LE
               DATASPACE  SIMPLE { ( 1 ) / ( 1 ) }
            }
         }
         DATASET "NumberOfPoints" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 1 ) / ( 1 ) }
         }
         GROUP "PointData" {
            DATASET "Normals" {
               DATATYPE  H5T_IEEE_F32LE
               DATASPACE  SIMPLE { ( 50, 3 ) / ( 50, 3 ) }
            }
            DATASET "Warping" {
               DATATYPE  H5T_IEEE_F32LE
               DATASPACE  SIMPLE { ( 50, 3 ) / ( 50, 3 ) }
            }
         }
         DATASET "Points" {
            DATATYPE  H5T_IEEE_F32LE
            DATASPACE  SIMPLE { ( 50, 3 ) / ( 50, 3 ) }
         }
         GROUP "Polygons" {
            DATASET "Connectivity" {
               DATATYPE  H5T_STD_I64LE
               DATASPACE  SIMPLE { ( 288 ) / ( 288 ) }
            }
            DATASET "NumberOfCells" {
               DATATYPE  H5T_STD_I64LE
               DATASPACE  SIMPLE { ( 1 ) / ( 1 ) }
            }
            DATASET "NumberOfConnectivityIds" {
               DATATYPE  H5T_STD_I64LE
               DATASPACE  SIMPLE { ( 1 ) / ( 1 ) }
            }
            DATASET "Offsets" {
               DATATYPE  H5T_STD_I64LE
               DATASPACE  SIMPLE { ( 97 ) / ( 97 ) }
            }
         }
         GROUP "Strips" {
            DATASET "Connectivity" {
               DATATYPE  H5T_STD_I64LE
               DATASPACE  SIMPLE { ( 0 ) / ( 0 ) }
            }
            DATASET "NumberOfCells" {
               DATATYPE  H5T_STD_I64LE
               DATASPACE  SIMPLE { ( 1 ) / ( 1 ) }
            }
            DATASET "NumberOfConnectivityIds" {
               DATATYPE  H5T_STD_I64LE
               DATASPACE  SIMPLE { ( 1 ) / ( 1 ) }
            }
            DATASET "Offsets" {
               DATATYPE  H5T_STD_I64LE
               DATASPACE  SIMPLE { ( 1 ) / ( 1 ) }
            }
         }
         GROUP "Vertices" {
            DATASET "Connectivity" {
               DATATYPE  H5T_STD_I64LE
               DATASPACE  SIMPLE { ( 0 ) / ( 0 ) }
            }
            DATASET "NumberOfCells" {
               DATATYPE  H5T_STD_I64LE
               DATASPACE  SIMPLE { ( 1 ) / ( 1 ) }
            }
            DATASET "NumberOfConnectivityIds" {
               DATATYPE  H5T_STD_I64LE
               DATASPACE  SIMPLE { ( 1 ) / ( 1 ) }
            }
            DATASET "Offsets" {
               DATATYPE  H5T_STD_I64LE
               DATASPACE  SIMPLE { ( 1 ) / ( 1 ) }
            }
         }
      }
      GROUP "Block1" {
         ATTRIBUTE "Index" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SCALAR
         }
         ATTRIBUTE "Type" {
            DATATYPE  H5T_STRING {
               STRSIZE 16;
               STRPAD H5T_STR_NULLTERM;
               CSET H5T_CSET_ASCII;
               CTYPE H5T_C_S1;
            }
            DATASPACE  SCALAR
         }
         ATTRIBUTE "Version" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 2 ) / ( 2 ) }
         }
         DATASET "Connectivity" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 8 ) / ( 8 ) }
         }
         DATASET "NumberOfCells" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 1 ) / ( 1 ) }
         }
         DATASET "NumberOfConnectivityIds" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 1 ) / ( 1 ) }
         }
         DATASET "NumberOfPoints" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 1 ) / ( 1 ) }
         }
         DATASET "Offsets" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 2 ) / ( 2 ) }
         }
         DATASET "Points" {
            DATATYPE  H5T_IEEE_F32LE
            DATASPACE  SIMPLE { ( 8, 3 ) / ( 8, 3 ) }
         }
         DATASET "Types" {
            DATATYPE  H5T_STD_U8LE
            DATASPACE  SIMPLE { ( 1 ) / ( 1 ) }
         }
      }
   }
}
}
```

#### Temporal Poly Data

The poly data is the `test_transient_poly_data.hdf` from the `VTK` testing data:

```
HDF5 "ExternalData/Testing/Data/test_transient_poly_data.hdf" {
GROUP "/" {
   GROUP "VTKHDF" {
      ATTRIBUTE "Type" {
         DATATYPE  H5T_STRING {
            STRSIZE 8;
            STRPAD H5T_STR_NULLPAD;
            CSET H5T_CSET_ASCII;
            CTYPE H5T_C_S1;
         }
         DATASPACE  SCALAR
      }
      ATTRIBUTE "Version" {
         DATATYPE  H5T_STD_I64LE
         DATASPACE  SIMPLE { ( 2 ) / ( 2 ) }
      }
      GROUP "CellData" {
         DATASET "Materials" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 8160 ) / ( H5S_UNLIMITED ) }
         }
      }
      GROUP "Lines" {
         DATASET "Connectivity" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 0 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "NumberOfCells" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 10 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "NumberOfConnectivityIds" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 10 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "Offsets" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 10 ) / ( H5S_UNLIMITED ) }
         }
      }
      DATASET "NumberOfPoints" {
         DATATYPE  H5T_STD_I64LE
         DATASPACE  SIMPLE { ( 10 ) / ( H5S_UNLIMITED ) }
      }
      GROUP "PointData" {
         DATASET "Normals" {
            DATATYPE  H5T_IEEE_F32LE
            DATASPACE  SIMPLE { ( 4120, 3 ) / ( H5S_UNLIMITED, 3 ) }
         }
         DATASET "Warping" {
            DATATYPE  H5T_IEEE_F32LE
            DATASPACE  SIMPLE { ( 4120, 3 ) / ( H5S_UNLIMITED, 3 ) }
         }
      }
      DATASET "Points" {
         DATATYPE  H5T_IEEE_F32LE
         DATASPACE  SIMPLE { ( 2060, 3 ) / ( H5S_UNLIMITED, 3 ) }
      }
      GROUP "Polygons" {
         DATASET "Connectivity" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 12240 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "NumberOfCells" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 10 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "NumberOfConnectivityIds" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 10 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "Offsets" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 4090 ) / ( H5S_UNLIMITED ) }
         }
      }
      GROUP "Steps" {
         ATTRIBUTE "NSteps" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SCALAR
         }
         GROUP "CellDataOffsets" {
            DATASET "Materials" {
               DATATYPE  H5T_STD_I64LE
               DATASPACE  SIMPLE { ( 10 ) / ( H5S_UNLIMITED ) }
            }
         }
         DATASET "CellOffsets" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 10, 4 ) / ( H5S_UNLIMITED, 4 ) }
         }
         DATASET "ConnectivityIdOffsets" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 10, 4 ) / ( H5S_UNLIMITED, 4 ) }
         }
         DATASET "NumberOfParts" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 10 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "PartOffsets" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 10 ) / ( H5S_UNLIMITED ) }
         }
         GROUP "PointDataOffsets" {
            DATASET "Normals" {
               DATATYPE  H5T_STD_I64LE
               DATASPACE  SIMPLE { ( 10 ) / ( H5S_UNLIMITED ) }
            }
            DATASET "Warping" {
               DATATYPE  H5T_STD_I64LE
               DATASPACE  SIMPLE { ( 10 ) / ( H5S_UNLIMITED ) }
            }
         }
         DATASET "PointOffsets" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 10 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "Values" {
            DATATYPE  H5T_IEEE_F32LE
            DATASPACE  SIMPLE { ( 10 ) / ( H5S_UNLIMITED ) }
         }
      }
      GROUP "Strips" {
         DATASET "Connectivity" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 0 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "NumberOfCells" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 10 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "NumberOfConnectivityIds" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 10 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "Offsets" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 10 ) / ( H5S_UNLIMITED ) }
         }
      }
      GROUP "Vertices" {
         DATASET "Connectivity" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 0 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "NumberOfCells" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 10 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "NumberOfConnectivityIds" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 10 ) / ( H5S_UNLIMITED ) }
         }
         DATASET "Offsets" {
            DATATYPE  H5T_STD_I64LE
            DATASPACE  SIMPLE { ( 10 ) / ( H5S_UNLIMITED ) }
         }
      }
   }
}
}
```
