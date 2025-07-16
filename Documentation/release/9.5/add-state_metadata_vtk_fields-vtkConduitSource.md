# vtkConduitSource: Add state/metadata/vtk_fields node

When defining fields in a Conduit tree, it is often useful to include metadata about the fields that can be
interpreted by VTK, and utilized appropriately. To that end, we have added a new node to the Conduit tree
that can be used to store metadata about the fields in the tree. This node is called `state/metadata/vtk_fields`.

Each child of the `state/metadata/vtk_fields` node has a name that corresponds to a field in the tree. The number
of children of the `state/metadata/vtk_fields` node should not match the number of `fields` in the tree since not every
field needs to have metadata.

The current metadata that can be stored for each field is:

1. `attribute_type`: It is a string and represents the attribute type of a field that, so that VTk can use an attribute
   array, such as `GlobalIds`. The values uses are the entries of `vtkDataSetAttributes::AttributeTypes` + `Ghosts`.
   The complete list of values is:
   - `Scalars`
   - `Vectors`
   - `Normals`
   - `TCoords`
   - `Tensors`
   - `GlobalIds`
   - `PedigreeIds`
   - `EdgeFlag`
   - `Tangents`
   - `RationalWeights`
   - `HigherOrderDegrees`
   - `ProcessIds`
   - `Ghosts`
2. `values_to_replace` and `replacement_values`: These two are vectors of values that be used to replace specific values
   in the field. This is useful when the field has some values that are not valid for VTK. For example,
   the `vtkGhostType` array uses specific values that other ghost cell generator algorithms may not use. In this case,
   the `values_to_replace` would be the values that need to be replaced, and the `replacement_values` would be the
   values that should replace them. The two vectors should have the same length.

Example:

```cpp
conduit_cpp::Node mesh;
CreateUniformMesh(3, 3, 3, mesh);

std::vector<int> cellGhosts(8, 0);
cellGhosts[2] = 1;

conduit_cpp::Node resCellFields = mesh["fields/other_ghosts"];
resCellFields["association"] = "element";
resCellFields["topology"] = "mesh";
resCellFields["volume_dependent"] = "false";
resCellFields["values"] = cellGhosts;

std::vector<int> cellGhostValuesToReplace(1, 1);
std::vector<int> cellGhostReplacementValues(1, vtkDataSetAttributes::HIDDENCELL);

std::vector<int> cellGhostsMetaData(1, 1);
conduit_cpp::Node ghostMetaData = mesh["state/metadata/vtk_fields/other_ghosts"];
ghostMetaData["attribute_type"] = "Ghosts";
ghostMetaData["values_to_replace"] = cellGhostValuesToReplace;
ghostMetaData["replacement_values"] = cellGhostReplacementValues;
```

Lastly, the usage of `ascent_ghosts` has been deprecated and should be replaced with the new
`state/metadata/vtk_fields` node, as shown above.
