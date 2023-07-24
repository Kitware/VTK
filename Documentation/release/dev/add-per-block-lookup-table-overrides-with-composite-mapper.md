## Use different lookup tables for different meshes with vtkCompositePolyDataMapper

`vtkCompositePolyDataMapper` can now use separate lookup tables and interpolation
modes for different blocks in a composite dataset.

You can override lookup table and other related attributes like scalar interpolation
and scalar ranges. Refer to `vtkMapper` documentation. Here's a summary:

1. **ScalarVisibility**: True/False
2. **UseLookupTableScalarRange**: when true, the mapper shall import the range from the lookup table.
3. **InterpolateScalarsBeforeMapping**: Applies when mesh is colored using point scalars. This flag decides whether point colors are sampled using texture maps instead of
 interpolating colors on the GPU after scalars are mapped to colors.
4. **ColorMode**: Specifies whether to map scalars to colors or directly use the scalars as RGB(A) values.
5. **ScalarRange**:  Specifies a range of scalars for color mapping.
6. **LookupTable**: Specifies a lookup table.
