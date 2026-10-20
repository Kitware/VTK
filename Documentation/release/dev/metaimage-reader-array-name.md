# `vtkMetaImageReader` can optionally use MetaImage `Name` metadata as the scalar-array name

`vtkMetaImageReader` already parses the optional `Name` header field from `.mha` and `.mhd` files, but it previously always assigned the point-data scalar array the hardcoded name `MetaImage`.

The attached patch adds a flag `UseArrayNameInFile` to enable using the existing `Name` value as the scalar-array name when it is nonempty. When the flag is set to `true`, an entry `Name = Temperature` in the MetaFile results in a scalar array named "Temperature". Files with no `Name`, an empty name, or a whitespace-only name retain the existing "MetaImage" default. As with other MetaImage header metadata, `Name` must appear before `ElementDataFile`.

To preserve backwards compatibility, the `UseArrayNameInFile` flag defaults to `false` so that existing VTK pipelines that expect the "MetaImage" array name continue to work as before.
