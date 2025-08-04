# Examples

We present several examples of VTK HDF files, shown using h5dump, an image file, an unstructured grid, a polydata and an overlapping AMR. These files can be examined in the VTK source code, by building VTK and enabling testing (`VTK_BUILD_TESTING`). The files are in the build directory ExternalData at `Testing/Data/mandelbrot-vti.hdf` for the ImageData. Same thing has been made for `UnstructuredGrid`, `Overlapping AMR`, `PolyData` and `PartitionedDataSetCollection`.

## ImageData

The image data file is a wavelet source produced in ParaView. Note
that we don't partition image data, so the same format is used for
serial and parallel processing.

<details>
<summary>mandelbrot-vti.hdf</summary>

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

</details>

## UnstructuredGrid

The unstructured grid is the can example (only the can, not the brick) from ParaView, partitioned in three:

<details>
<summary>can-pvtu.hdf</summary>

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
</details>

## PolyData

The poly data is the `test_poly_data.hdf` from the `VTK` testing data:

<details>
<summary>test_poly_data.hdf</summary>

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
</details>

## Overlapping AMR

The Overlapping AMR data file is an AMR Guaussian Pulse source with two levels
(0 and 1), describing one Point Data, several Cell Data and a Field Data. Actual
`Data` are not displayed for readability.

<details>
<summary>amr_gaussian_pulse.hdf</summary>

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
</details>

## PartitionedDataSetCollection

This partitioned dataset collection has 2 blocks, one unstructured grid (Block1) and one polydata (Block0).
Its assembly has 3 elements and no nesting, referencing one of the 2 blocks using symbolic links

<details>
<summary>composite.hdf</summary>

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
</details>

## Temporal Poly Data

The poly data is the `test_transient_poly_data.hdf` from the `VTK` testing data:

<details>
<summary>test_transient_poly_data.hdf</summary>

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
</details>
