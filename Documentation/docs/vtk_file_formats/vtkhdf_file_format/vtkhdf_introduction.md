# Introduction

The `VTKHDF` file format is a file format relying on [HDF5](https://www.hdfgroup.org/solutions/hdf5/).
It is meant to provide good I/O performance as well as robust and flexible parallel I/O capabilities.

It currently supports basic types: `PolyData`, `UnstructuredGrid`, `ImageData`, `HyperTreeGrid` and composite types such as `OverlappingAMR`, `MultiBlockDataSet` and the `PartitionedDataSetCollection`.

The current file format version is **2.5**.

---
## Python examples and tutorial

To help adopters use the VTKHDF File Format, we provide a repository : [vtkhdf-script](https://gitlab.kitware.com/keu-public/vtkhdf/vtkhdf-scripts).

It contains several folders:
- [tutorial](https://gitlab.kitware.com/keu-public/vtkhdf/vtkhdf-scripts/-/tree/main/tutorial): a complete tutorial showing how to create VTKHDF files from scratch, incrementally adding more features.
- [scripts](https://gitlab.kitware.com/keu-public/vtkhdf/vtkhdf-scripts/-/tree/main/scripts): sample python scripts showing how to create VTKHDF files using `h5py` for most of the data types currently supported by the format, including temporal files.
- [vtkxml-to-vtkhdf](https://gitlab.kitware.com/keu-public/vtkhdf/vtkhdf-scripts/-/tree/main/vtkxml-to-vtkhdf) folder which contains script to convert vtkxml file into vtk using the `vtkHDFWriter`.

---
## Valuable documentation from blog posts

- [an overview of the first version and the motivation behind HDF5 in VTK](https://www.kitware.com/vtk-hdf-reader/).

---
## Extension

The ` VTKHDF` format generally uses the `.vtkhdf` extension. The `.hdf`
extension is also supported but is not preferred. There are no specific
extensions to differentiate between different types of dataset, serial
vs. distributed data or static vs. temporal data.

---
## FAQ

### How to stay informed?

We keep a [status](vtkhdf_status.md) page up to date, that tracks current status of the VTKHDF specification and implementation in the `vtkHDFReader` and `vtkHDFWriter`. Bug tracking and proposals are tracked in a [Gitlab issue](https://gitlab.kitware.com/vtk/vtk/-/issues/19243). Additionally, all VTKHDF issues are [tagged appropriately](https://gitlab.kitware.com/vtk/vtk/-/issues/?sort=created_date&state=opened&label_name%5B%5D=area%3AVTKHDF&first_page_size=20) as `area:VTKHDF`.

### Where should we create a proposal for the file format?

The VTKHDF File Format is not considered complete, and lacks support for some VTK data types. This specification is led by the community; please post ideas and design for new data types on the [VTK discourse](https://discourse.vtk.org/). This Discourse will be made into a Gitlab issue once VTKHDF contributors have agreed on a design.

### Who maintains this specification?

Current maintainer of this file format are :
- Lucas Givord (gitlab: @lucas.givord, discourse: @lgivord)
- Louis Gombert (gitlab: @louis.gombert, discourse: @Louis_Gombert)
- Mathieu Westphal (gitlab: @mwestphal, discourse: @mwestphal)

Do not hesitate to reach out on [discourse](https://discourse.vtk.org/) or [gitlab](https://gitlab.kitware.com/vtk/vtk/).

---
## Changelog

### VTKHDF - 2.5

- add specification for Unstructured Grid polyhedrons

### VTKHDF - 2.4

- add specification for `HyperTreeGrid`

### VTKHDF - 2.3

- fix array names which miss the `s` to be consistent with other temporal dataset in case of the temporal `OverlappingAMR`. It concerns these data names: `NumberOfBox`, `AMRBoxOffset`, `Point/Cell/FieldDataOffset`.

### VTKHDF - 2.2

- add support for temporal `OverlappingAMR`
- add official support for ignored data outside of `VTKHDF`

### VTKHDF - 2.1

- add specification in the format for `PartitionedDataSetCollection` and `MultiBlockDataSet`

### VTKHDF - 2.0

- extends the specification for `PolyData`.

- add support for `Temporal` dataset for `PolyData`, `ImageData` and `UnstructuredGrid`.

### VTKHDF - 1.0

- add specification for these vtk data types:
  - `UnstructuredGrid`
  - `ImageData`
  - `Overlapping AMR`
