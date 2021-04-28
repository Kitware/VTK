[![Documentation Status](https://readthedocs.org/projects/fides/badge/?version=latest)](https://fides.readthedocs.io/en/latest/?badge=latest)

## Fides: Adaptable Data Interfaces and Services

Fides is a library for adding metadata to ADIOS2 files so they can be more easily read and understood by VTK-m.

The metadata required to visualize a dataset is often different than the metadata required in other contexts, so despite the fact we want our simulation data to be "self-describing", this statement has different meanings in different contexts, and hence it is almost impossible to realize in practice.

To use Fides, you must first create a `.json` file which has information relevant for processing the file in VTK-m.

## Documentation

Documentation is hosted at [Read The Docs](https://fides.readthedocs.io/en/latest/?badge=latest).

## Dependencies

Fides depends of VTK-m and ADIOS2.

A commit of VTK-m which is known to work is 4df064f3 (from Apr 14, 2021).

Due to recent fixes in ADIOS, use commit 3324c77f95 (from Apr 13, 2020) or later.
Run the unit tests if doubt arises.


## Testing

`Fides` uses `git lfs` to manage its datasets.
This must be initialized after cloning.

```
fides$ git lfs install
fides$ git lfs pull
```

The tests are managed by `ctest`, so in the build directory just run

```
build_fides$ ctest -V
```
