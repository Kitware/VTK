##  change of the critical type enum names in `vtkVectorFieldTopology`

The names of the critical types now have underscores to separate words to be in accordance with item 11 in the VTK coding style guide: `https://docs.google.com/document/d/1nzinw-dR5JQRNi_gb8qwLL5PnkGMK2FETlQGLr10tZw/edit`
The header file in question is `Filters/FlowPaths/vtkVectorFieldTopology.h`.
The old names of the field `type` in 2D were
`SOURCE2D 2, SADDLE2D 1, SINK2D 0, (CENTER2D 3)`
and now are
`SOURCE_2D 2, SADDLE_2D 1, SINK_2D 0, (CENTER_2D 3)`
and in 3D were
`SOURCE3D 3, SADDLE23D 2, SADDLE13D 1, SINK3D 0, (CENTER3D 4)`
and now are
`SOURCE_3D 3, SADDLE_2_3D 2 , SADDLE_1_3D 1, SINK_3D 0, (CENTER_3D 4)`
