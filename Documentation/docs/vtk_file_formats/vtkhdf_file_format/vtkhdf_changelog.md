# Changelog

## VTKHDF - 2.8

- add specification for `Table`

## VTKHDF - 2.7

- add specification for `RectilinearGrid` and `StructuredGrid`

## VTKHDF - 2.6

- add specification for dataset attributes

## VTKHDF - 2.5

- add specification for Unstructured Grid polyhedrons

## VTKHDF - 2.4

- add specification for `HyperTreeGrid`

## VTKHDF - 2.3

- fix array names which miss the `s` to be consistent with other temporal dataset in case of the temporal `OverlappingAMR`. It concerns these data names: `NumberOfBox`, `AMRBoxOffset`, `Point/Cell/FieldDataOffset`.

## VTKHDF - 2.2

- add support for temporal `OverlappingAMR`
- add official support for ignored data outside of `VTKHDF`

## VTKHDF - 2.1

- add specification in the format for `PartitionedDataSetCollection` and `MultiBlockDataSet`

## VTKHDF - 2.0

- extends the specification for `PolyData`.

- add support for `Temporal` dataset for `PolyData`, `ImageData` and `UnstructuredGrid`.

## VTKHDF - 1.0

- add specification for these vtk data types:
  - `UnstructuredGrid`
  - `ImageData`
  - `Overlapping AMR`
