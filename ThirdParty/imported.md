# Imported Third Party Projects

This page provides an overview of the imported third-party projects that VTK depends
on, grouped by import method.

The lists below references project directory name found in either the `ThirdParty`
or `Utilities` source sub-directory available in the [VTK GitLab][] repository where additional
details may be found.

[VTK GitLab]: https://gitlab.kitware.com/vtk/vtk

## Using the `update.sh` framework

The following list shows third-party projects that were imported using the `update.sh`
framework described in the [UPDATING](UPDATING.md) document:

  * [cgns](cgns/update.sh)
  * [cli11](cli11/update.sh)
  * [diy2](diy2/update.sh)
  * [doubleconversion](doubleconversion/update.sh)
  * [eigen](eigen/update.sh)
  * [exodusII](exodusII/update.sh)
  * [expat](expat/update.sh)
  * [exprtk](exprtk/update.sh)
  * [fast_float](fast_float/update.sh)
  * [fides](fides/update.sh)
  * [fmt](fmt/update.sh)
  * [freetype](freetype/update.sh)
  * [gl2ps](gl2ps/update.sh)
  * [glew](glew/update.sh)
  * [h5part](h5part/update.sh)
  * [hdf5](hdf5/update.sh)
  * [ioss](ioss/update.sh)
  * [jpeg](jpeg/update.sh)
  * [jsoncpp](jsoncpp/update.sh)
  * [kissfft](kissfft/update.sh)
  * [KWIML](../Utilities/KWIML/update.sh)
  * [KWSys](../Utilities/KWSys/update.sh)
  * [libharu](libharu/update.sh)
  * [libproj](libproj/update.sh)
  * [libxml2](libxml2/update.sh)
  * [loguru](loguru/update.sh)
  * [lz4](lz4/update.sh)
  * [lzma](lzma/update.sh)
  * [MetaIO](../Utilities/MetaIO/update.sh)
  * [mpi4py](mpi4py/update.sh)
  * [netcdf](netcdf/update.sh)
  * [nlohmannjson](nlohmannjson/update.sh)
  * [ogg](ogg/update.sh)
  * [pegtl](pegtl/update.sh)
  * [png](png/update.sh)
  * [pugixml](pugixml/update.sh)
  * [sqlite](sqlite/update.sh)
  * [theora](theora/update.sh)
  * [tiff](tiff/update.sh)
  * [utf8](utf8/update.sh)
  * [verdict](verdict/update.sh)
  * [xdmf3](xdmf3/update.sh)
  * [zfp](zfp/update.sh)
  * [zlib](zlib/update.sh)

<!--
The list above was generated using the following script:

```
cd VTK/ThirdParty
for path in $(ls -d -1 */) $(ls -d -1 ../Utilities/*/); do
  path=${path%/*}  # "dir1/dir2/Dir3/" -> "dir1/dir2/Dir3"
  project=${path##*/}  # "dir1/dir2/Dir3" -> "Dir3"
  if [[ -f "$path/update.sh" ]]; then
    echo "  * [$project]($path/update.sh)"
  fi
done | sort --ignore-case
```
-->

## Using `git submodule`

The following third-party project were imported as git submodules:

  * vtkm

<!--
The list above was generated using the following script:

```
cd VTK/ThirdParty

root_src_dir=$(git rev-parse --show-toplevel)

for submodule in $(git config --file ${root_src_dir}/.gitmodules --get-regexp path | awk '{ print $2 }'); do
  # Ignore submodules not associated with the "ThirdParty" directory
  if ! [[ "$submodule" =~ ^ThirdParty* ]]; then
    continue
  fi
  project=$(echo $submodule | cut -d/ -f2) # "ThirdParty/vtkm/vtkvtkm/vtk-m" -> "vtkm"
  echo "  * $project"
done
```
-->

## Using `copy`

The following third-party projects were imported by copying the files:

  * vpic
  * xdmf2

<!--
The list above was generated using the following script:

```
root_src_dir=$(git rev-parse --show-toplevel)
submodule_paths=$(git config --file ${root_src_dir}/.gitmodules --get-regexp path | awk '{ print $2 }')

for path in $(ls -d -1 */); do
  path=${path%/*}  # "dir1/dir2/Dir3/" -> "dir1/dir2/Dir3"
  project=${path##*/}  # "dir1/dir2/Dir3" -> "Dir3"

  # List project that are neither imported through "update.sh" or git submodule
  if [ ! -f "$path/update.sh" ] && [[ "$submodule_paths" != *"ThirdParty/$project"* ]]; then
    echo "  * $project"
  fi
done | sort --ignore-case
```
-->
