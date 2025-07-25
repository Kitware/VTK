
# VTKHDF format specification

## General Specification

VTK HDF files start with a group called `VTKHDF` with two attributes:
`Version`, an array of two integers and `Type`, a string showing the
VTK dataset type stored in the file. Additional attributes can follow
depending on the dataset type. Currently, `Version`
is the array [2, 2] and `Type` can be `ImageData`, `PolyData`,
`UnstructuredGrid`, `OverlappingAMR`,  `PartitionedDataSetCollection` or
`MultiBlockDataSet`.

Top-level groups outside of `/VTKHDF` do not contain any information related
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

## Image data

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

```{graphviz}
digraph G {
    graph [bgcolor=transparent, fontname="Helvetica"];
    node [style=filled, fillcolor=white, fontname="Helvetica"];
    edge [color=gray, fontname="Helvetica"];

    VTKHDF [label="VTKHDF\n Version, Type, WholeExtent, Origin, Spacing, Direction", shape=Mrecord, fillcolor=lightblue];

    FieldData [label="FieldData", shape=Mrecord, fillcolor=lightblue];
    PointData [label="PointData\nScalars, Vectors, ...", shape=Mrecord, fillcolor=lightblue];
    CellData [label="CellData\nScalars, Vectors, ...", shape=Mrecord, fillcolor=lightblue];

    FieldArrayName [label="FieldArrayName", shape=Mrecord, fillcolor=lightgrey];
    PointArrayName [label="PointArrayName", shape=Mrecord, fillcolor=lightgrey];
    CellArrayName [label="CellArrayName", shape=Mrecord, fillcolor=lightgrey];

    FieldArrayNameEtc [label="...", shape=Mrecord, fillcolor=lightgrey];
    PointArrayNameEtc [label="...", shape=Mrecord, fillcolor=lightgrey];
    CellArrayNameEtc [label="...", shape=Mrecord, fillcolor=lightgrey];

    VTKHDF -> FieldData;
    VTKHDF -> PointData;
    VTKHDF -> CellData;
    FieldData -> FieldArrayName;
    PointData -> PointArrayName;
    CellData -> CellArrayName;
    FieldData -> FieldArrayNameEtc;
    PointData -> PointArrayNameEtc;
    CellData -> CellArrayNameEtc;
}

```

<div align="center">
Figure 1. - Image Data VTKHDF File Format
</div>


## Unstructured grid

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

```{graphviz}
digraph G {
    rankdir=LR;
    graph [bgcolor=transparent, fontname="Helvetica"];
    node [style=filled, fillcolor=white, fontname="Helvetica"];
    edge [color=gray, fontname="Helvetica"];

    VTKHDF [label="VTKHDF\n Version, Type", shape=Mrecord, fillcolor=lightblue];

    NumberOfConnectivityIds [label="NumberOfConnectivityIds", shape=Mrecord, fillcolor=lightgrey];
    NumberOfPoints [label="NumberOfPoints", shape=Mrecord, fillcolor=lightgrey];
    NumberOfCells [label="NumberOfCells", shape=Mrecord, fillcolor=lightgrey];
    Points [label="Points", shape=Mrecord, fillcolor=lightgrey];
    Types [label="Types", shape=Mrecord, fillcolor=lightgrey];
    Connectivity [label="Connectivity", shape=Mrecord, fillcolor=lightgrey];
    Offsets [label="Offsets", shape=Mrecord, fillcolor=lightgrey];

    FieldData [label="FieldData", shape=Mrecord, fillcolor=lightblue];
    PointData [label="PointData\nScalars, Vectors, ...", shape=Mrecord, fillcolor=lightblue];
    CellData [label="CellData\nScalars, Vectors, ...", shape=Mrecord, fillcolor=lightblue];

    FieldArrayName [label="FieldArrayName", shape=Mrecord, fillcolor=lightgrey];
    PointArrayName [label="PointArrayName", shape=Mrecord, fillcolor=lightgrey];
    CellArrayName [label="CellArrayName", shape=Mrecord, fillcolor=lightgrey];

    FieldArrayNameEtc [label="...", shape=Mrecord, fillcolor=lightgrey];
    PointArrayNameEtc [label="...", shape=Mrecord, fillcolor=lightgrey];
    CellArrayNameEtc [label="...", shape=Mrecord, fillcolor=lightgrey];

    VTKHDF -> NumberOfConnectivityIds;
    VTKHDF -> NumberOfPoints;
    VTKHDF -> NumberOfCells;
    VTKHDF -> Points;
    VTKHDF -> Types;
    VTKHDF -> Connectivity;
    VTKHDF -> Offsets;
    VTKHDF -> FieldData;
    VTKHDF -> PointData;
    VTKHDF -> CellData;
    FieldData -> FieldArrayName;
    PointData -> PointArrayName;
    CellData -> CellArrayName;
    FieldData -> FieldArrayNameEtc;
    PointData -> PointArrayNameEtc;
    CellData -> CellArrayNameEtc;
}

```

<div align="center">
Figure 2. - Unstructured Grid VTKHDF File Format
</div>

To read the data for its rank a node reads the information about all
partitions, compute the correct offset and then read data from that
offset.

## Poly data

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

```{graphviz}
digraph G {
    rankdir=LR;
    graph [bgcolor=transparent, fontname="Helvetica"];
    node [style=filled, fillcolor=white, fontname="Helvetica"];
    edge [color=gray, fontname="Helvetica"];

    VTKHDF [label="VTKHDF\n Version, Type", shape=Mrecord, fillcolor=lightblue];

    NumberOfPoints [label="NumberOfPoints", shape=Mrecord, fillcolor=lightgrey];
    Points [label="Points", shape=Mrecord, fillcolor=lightgrey];

    Vertices [label="Vertices", shape=Mrecord, fillcolor=lightblue];
    Lines [label="Lines", shape=Mrecord, fillcolor=lightblue];
    Polygons [label="Polygons", shape=Mrecord, fillcolor=lightblue];
    Strips [label="Strips", shape=Mrecord, fillcolor=lightblue];

    FieldData [label="FieldData", shape=Mrecord, fillcolor=lightblue];
    PointData [label="PointData\nScalars, ...", shape=Mrecord, fillcolor=lightblue];
    CellData [label="CellData\nScalars, ...", shape=Mrecord, fillcolor=lightblue];

    NumberOfConnectivityIds [label="NumberOfConnectivityIds", shape=Mrecord, fillcolor=lightgrey];
    Connectivity [label="Connectivity", shape=Mrecord, fillcolor=lightgrey];
    Offsets [label="Offsets", shape=Mrecord, fillcolor=lightgrey];
    NumberOfCells [label="NumberOfCells", shape=Mrecord, fillcolor=lightgrey];

    FieldArrayName [label="FieldArrayName", shape=Mrecord, fillcolor=lightgrey];
    PointArrayName [label="PointArrayName", shape=Mrecord, fillcolor=lightgrey];
    CellArrayName [label="CellArrayName", shape=Mrecord, fillcolor=lightgrey];

    LinesEtc [label="...", shape=Mrecord, fillcolor=lightgrey];
    PolygonsEtc [label="...", shape=Mrecord, fillcolor=lightgrey];
    StripsEtc [label="...", shape=Mrecord, fillcolor=lightgrey];

    VTKHDF -> NumberOfPoints;
    VTKHDF -> Points;
    VTKHDF -> Vertices;
    VTKHDF -> Lines;
    VTKHDF -> Polygons;
    VTKHDF -> Strips;
    VTKHDF -> FieldData;
    VTKHDF -> PointData;
    VTKHDF -> CellData;
    FieldData -> FieldArrayName;
    PointData -> PointArrayName;
    CellData -> CellArrayName;
    Lines -> LinesEtc;
    Polygons -> PolygonsEtc;
    Strips -> StripsEtc;
    Vertices -> NumberOfConnectivityIds;
    Vertices -> Connectivity;
    Vertices -> NumberOfCells;
    Vertices -> Offsets;
}

```

<div align="center">
Figure 3. - Poly Data VTKHDF File Format
</div>

To read the data for its rank a node reads the information about all
partitions, compute the correct offset and then read data from that
offset.

## Overlapping AMR

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

```{graphviz}
digraph G {
    graph [bgcolor=transparent, fontname="Helvetica"];
    node [style=filled, fillcolor=white, fontname="Helvetica"];
    edge [color=gray, fontname="Helvetica"];

    VTKHDF [label="VTKHDF\n Version, Type, GridDescription, Origin", shape=Mrecord, fillcolor=lightblue];

    Level0 [label="Level0\n Spacing", shape=Mrecord, fillcolor=lightblue];
    LevelEtc [label="...", shape=Mrecord, fillcolor=lightblue];

    AMRBox [label="AMRBox", shape=Mrecord, fillcolor=lightgrey];
    FieldData [label="FieldData", shape=Mrecord, fillcolor=lightblue];
    PointData [label="PointData\nScalars, ...", shape=Mrecord, fillcolor=lightblue];
    CellData [label="CellData\nScalars, ...", shape=Mrecord, fillcolor=lightblue];

    FieldArrayName [label="FieldArrayName", shape=Mrecord, fillcolor=lightgrey];
    PointArrayName [label="PointArrayName", shape=Mrecord, fillcolor=lightgrey];
    CellArrayName [label="CellArrayName", shape=Mrecord, fillcolor=lightgrey];

    FieldArrayEtc [label="...", shape=Mrecord, fillcolor=lightgrey];
    PointArrayEtc [label="...", shape=Mrecord, fillcolor=lightgrey];
    CellArrayEtc [label="...", shape=Mrecord, fillcolor=lightgrey];

    VTKHDF -> Level0;
    VTKHDF -> LevelEtc;
    Level0 -> AMRBox;
    Level0 -> PointData;
    Level0 -> CellData;
    Level0 -> FieldData;
    FieldData -> FieldArrayName;
    PointData -> PointArrayName;
    CellData -> CellArrayName;
    FieldData -> FieldArrayEtc;
    PointData -> PointArrayEtc;
    CellData -> CellArrayEtc;
}

```

<div align="center">
Figure 4. Overlapping AMR VTKHDF File Format
</div>

## HyperTreeGrid

The schema for the tree-based AMR HyperTreeGrid VTKHDF specification is shown in Figure 5.
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

```{graphviz}
digraph G {
    rankdir=LR;
    graph [bgcolor=transparent, fontname="Helvetica"];
    node [style=filled, fillcolor=white, fontname="Helvetica"];
    edge [color=gray, fontname="Helvetica"];

    VTKHDF [label="VTKHDF\n Version, Type, BranchFactor, Dimensions,\nTransposedRootIndexing, InterfaceInterceptsName,\nInterfaceNormalsName", shape=Mrecord, fillcolor=lightblue];

    Steps [label="Steps\n NSteps", shape=Mrecord, fillcolor=lightblue];
    XCoordinates [label="XCoordinates", shape=Mrecord, fillcolor=lightgrey];
    YCoordinates [label="YCoordinates", shape=Mrecord, fillcolor=lightgrey];
    ZCoordinates [label="ZCoordinates", shape=Mrecord, fillcolor=lightgrey];
    Descriptors [label="Descriptors", shape=Mrecord, fillcolor=lightgrey];
    DescriptorsSize [label="DescriptorsSize", shape=Mrecord, fillcolor=lightgrey];
    TreeIds [label="TreeIds", shape=Mrecord, fillcolor=lightgrey];
    DepthPerTree [label="DepthPerTree", shape=Mrecord, fillcolor=lightgrey];
    NumberOfTrees [label="NumberOfTrees", shape=Mrecord, fillcolor=lightgrey];
    NumberOfDepths [label="NumberOfDepths", shape=Mrecord, fillcolor=lightgrey];
    Mask [label="Mask", shape=Mrecord, fillcolor=lightgrey];

    Values [label="Values", shape=Mrecord, fillcolor=lightgrey];
    XCoordinatesOffsets [label="XCoordinatesOffsets", shape=Mrecord, fillcolor=lightgrey];
    YCoordinatesOffsets [label="YCoordinatesOffsets", shape=Mrecord, fillcolor=lightgrey];
    ZCoordinatesOffsets [label="ZCoordinatesOffsets", shape=Mrecord, fillcolor=lightgrey];
    DescriptorsOffsets [label="DescriptorsOffsets", shape=Mrecord, fillcolor=lightgrey];
    TreeIdsOffsets [label="TreeIdsOffsets", shape=Mrecord, fillcolor=lightgrey];
    MaskOffsets [label="MaskOffsets", shape=Mrecord, fillcolor=lightgrey];
    PartOffsets [label="PartOffsets", shape=Mrecord, fillcolor=lightgrey];
    NumberOfCellsPerTreeDepthOffsets [label="NumberOfCellsPerTreeDepthOffsets", shape=Mrecord, fillcolor=lightgrey];

    CellDataOffsets [label="CellDataOffsets", shape=Mrecord, fillcolor=lightblue];
    CellDataEtc [label="...", shape=Mrecord, fillcolor=lightgrey];

    VTKHDF -> Steps;
    VTKHDF -> XCoordinates;
    VTKHDF -> YCoordinates;
    VTKHDF -> ZCoordinates;
    VTKHDF -> Descriptors;
    VTKHDF -> DescriptorsSize;
    VTKHDF -> TreeIds;
    VTKHDF -> DepthPerTree;
    VTKHDF -> NumberOfTrees;
    VTKHDF -> NumberOfDepths;
    VTKHDF -> Mask;
    Steps -> Values;
    Steps -> XCoordinatesOffsets;
    Steps -> YCoordinatesOffsets;
    Steps -> ZCoordinatesOffsets;
    Steps -> DescriptorsOffsets;
    Steps -> TreeIdsOffsets;
    Steps -> MaskOffsets;
    Steps -> PartOffsets;
    Steps -> NumberOfCellsPerTreeDepthOffsets;
    Steps -> CellDataOffsets;
    CellDataOffsets -> CellDataEtc;
}

```

<div align="center">
Figure 5. - HyperTreeGrid VTKHDF File Format
</div>

## PartitionedDataSetCollection and MultiBlockDataSet

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

```{graphviz}
digraph G {
    graph [bgcolor=transparent, fontname="Helvetica"];
    node [style=filled, fillcolor=white, fontname="Helvetica"];
    edge [color=gray, fontname="Helvetica"];

    VTKHDF [label="VTKHDF\n Version, Type", shape=Mrecord, fillcolor=lightblue];

    Assembly [label="Assembly", shape=Mrecord, fillcolor=lightblue];
    BlockName0 [label="BlockName0\n Index, Version, Type", shape=Mrecord, fillcolor=lightblue];
    BlockNameEtc [label="...", shape=Mrecord, fillcolor=lightblue];
    BlockNameN [label="BlockNameN\n Index, Version, Type", shape=Mrecord, fillcolor=lightblue];

    GroupName0 [label="GroupName0", shape=Mrecord, fillcolor=lightblue];
    GroupNameEtc [label="...", shape=Mrecord, fillcolor=lightblue];
    AssemblyBlockName0 [label="BlockName0", shape=Mrecord, fillcolor=plum3];
    AssemblyBlockNameN [label="BlockNameN", shape=Mrecord, fillcolor=plum3];
    GroupName0Etc [label="...", shape=Mrecord, fillcolor=plum3];

    VTKHDF -> Assembly;
    VTKHDF -> BlockName0;
    VTKHDF -> BlockNameEtc;
    VTKHDF -> BlockNameN;
    Assembly -> GroupName0;
    Assembly -> GroupNameEtc;
    Assembly -> AssemblyBlockName0;
    GroupName0 -> GroupName0Etc;
    GroupName0 -> AssemblyBlockNameN;
    AssemblyBlockName0 -> BlockName0 [color=plum3, style=dotted, constraint=false];
    AssemblyBlockNameN -> BlockNameN [color=plum3, style=dotted, constraint=false];
}

```

<div align="center">
Figure 6. - PartitionedDataSetCollection/MultiBlockDataset VTKHDF File Format
</div>

:::{hint}
Each block should describe a valid VTKHDF root node for a supported data types. Composite data types are, and will, not be supported.
:::

## Temporal Data

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

```{graphviz}
digraph G {
    rankdir=LR;
    graph [bgcolor=transparent, fontname="Helvetica"];
    node [style=filled, fillcolor=white, fontname="Helvetica"];
    edge [color=gray, fontname="Helvetica"];

    VTKHDF [label="VTKHDF\n Version, Type", shape=Mrecord, fillcolor=lightblue];

    Steps [label="Steps\nNSteps", shape=Mrecord, fillcolor=lightblue];
    PointDataOffsets [label="PointDataOffsets", shape=Mrecord, fillcolor=lightblue];
    CellDataOffsets [label="CellDataOffsets", shape=Mrecord, fillcolor=lightblue];
    FieldDataOffsets [label="FieldDataOffsets", shape=Mrecord, fillcolor=lightblue];
    FieldDataSizes [label="FieldDataSizes", shape=Mrecord, fillcolor=lightblue];

    PointDataArrayName [label="ArrayName", shape=Mrecord, fillcolor=lightgrey];
    CellDataArrayName [label="ArrayName", shape=Mrecord, fillcolor=lightgrey];
    FieldDataArrayName [label="ArrayName", shape=Mrecord, fillcolor=lightgrey];
    FieldDataSizeArrayName [label="ArrayName", shape=Mrecord, fillcolor=lightgrey];

    Values [label="Values", shape=Mrecord, fillcolor=lightgrey];
    PartOffsets [label="PartOffsets", shape=Mrecord, fillcolor=lightgrey];
    NumberOfParts [label="NumberOfParts", shape=Mrecord, fillcolor=lightgrey];
    ConnectivityIdOffsets [label="ConnectivityIdOffsets", shape=Mrecord, fillcolor=lightgrey];
    CellOffsets [label="CellOffsets", shape=Mrecord, fillcolor=lightgrey];
    PointOffsets [label="PointOffsets", shape=Mrecord, fillcolor=lightgrey];

    VTKHDF -> Steps;
    Steps -> Values;
    Steps -> PartOffsets;
    Steps -> NumberOfParts;
    Steps -> ConnectivityIdOffsets;
    Steps -> CellOffsets;
    Steps -> PointOffsets;
    Steps -> PointDataOffsets;
    Steps -> CellDataOffsets;
    Steps -> FieldDataOffsets;
    Steps -> FieldDataSizes;
    PointDataOffsets -> PointDataArrayName;
    CellDataOffsets -> CellDataArrayName;
    FieldDataOffsets -> FieldDataArrayName;
    FieldDataSizes -> FieldDataSizeArrayName;
}

```

<div align="center">
Figure 7. - Temporal Data VTKHDF File Format
</div>

:::{hint}
`VTKHDF` group should look exactly as it does for no time steps except that the main dimensions of the datasets incorporate the potentially evolving time data as well. Individual time steps can be accessed in these flattened arrays through the offset information in the `Steps` group by slicing the data. Offset value can be repeated for static data.
:::

Writing incrementally to `VTKHDF` temporal datasets is relatively straightforward using the
appending functionality of `HDF5` chunked data sets
([Chunking in HDF5](https://davis.lbl.gov/Manuals/HDF5-1.8.7/Advanced/Chunking/index.html)).

### Particularity regarding ImageData

A particularity of temporal `Image Data` in the format is that the reader expects an additional
prepended dimension considering the time to be the first dimension in the multidimensional arrays.
As such, arrays described in temporal `Image Data` should have dimensions ordered as
`(time, z, y, x)`.

### Particularity regarding OverlappingAMR

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

## Limitations

This specification and the reader available in VTK currently only
supports ImageData, UnstructuredGrid, PolyData, Overlapping AMR, MultiBlockDataSet and Partitioned
DataSet Collection. Other dataset types may be added later depending on interest and funding.

Unlike XML formats, VTKHDF does not support field names containing `/` and `.` characters,
because of a limitation in the HDF5 format specification.
