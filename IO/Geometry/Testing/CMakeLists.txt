vtk_module_test_data(
  Data/42400-IDGH.stl
  Data/AMR/Enzo/DD0010/,REGEX:.*
  Data/IncOctPntLocData.dat
  Data/IncOctPntLocResult.dat

  Data/jacksonville_15.obj
  Data/jacksonville_15.mtl
  Data/jacksonville_15.png

  Data/OpenFOAM/DimFields/cavity/,REGEX:.*
  Data/OpenFOAM/DimFields/cavity/system/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/DimFields/cavity/constant/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/DimFields/cavity/0/,RECURSE:,REGEX:.*$

  Data/OpenFOAM/cavity/,REGEX:.*
  Data/OpenFOAM/cavity/system/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/cavity/constant/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/cavity/0/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/cavity/0.5/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/cavity/1/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/cavity/1.5/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/cavity/2/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/cavity/2.5/,RECURSE:,REGEX:.*$

  Data/OpenFOAM/precision/case01/,REGEX:.*
  Data/OpenFOAM/precision/case01/system/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/precision/case01/constant/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/precision/case01/0/,RECURSE:,REGEX:.*$

  Data/OpenFOAM/largePolyhedral/,REGEX:.*
  Data/OpenFOAM/largePolyhedral/system/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/largePolyhedral/constant/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/largePolyhedral/2002/,RECURSE:,REGEX:.*$

  Data/OpenFOAM/regex/,REGEX:.*
  Data/OpenFOAM/regex/system/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/regex/constant/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/regex/0/,RECURSE:,REGEX:.*$

  Data/OpenFOAM/simplifiedSiwek-serial/,REGEX:.*$
  Data/OpenFOAM/simplifiedSiwek-serial/system/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/simplifiedSiwek-serial/constant/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/simplifiedSiwek-serial/0/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/simplifiedSiwek-serial/0.005/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/simplifiedSiwek-serial/0.01/,RECURSE:,REGEX:.*$

  Data/OpenFOAM/simplifiedSiwek-uncollated/,REGEX:.*
  Data/OpenFOAM/simplifiedSiwek-uncollated/system/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/simplifiedSiwek-uncollated/constant/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/simplifiedSiwek-uncollated/processor0/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/simplifiedSiwek-uncollated/processor1/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/simplifiedSiwek-uncollated/processor2/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/simplifiedSiwek-uncollated/processor3/,RECURSE:,REGEX:.*$

  Data/OpenFOAM/squareBend/,REGEX:.*
  Data/OpenFOAM/squareBend/system/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/squareBend/constant/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/squareBend/50/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/squareBend/100/,RECURSE:,REGEX:.*$

  Data/OpenFOAM/valuePointPatch/,REGEX:.*
  Data/OpenFOAM/valuePointPatch/system/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/valuePointPatch/constant/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/valuePointPatch/2002/,RECURSE:,REGEX:.*$

  Data/TecPlot/,REGEX:.*
  Data/Viewpoint/cow.obj
  Data/absolute_indices.obj
  Data/cellcentered.tec
  Data/cellsnd.ascii.inp
  Data/cellsnd.bin.inp
  Data/flow.tec
  Data/flow.tec.gz
  Data/obj_double_sphere.obj
  Data/obj_multiline.obj
  Data/obj_multitexture.obj
  Data/obj_multitexture_notexture.obj
  Data/obj_singletexture.obj
  Data/obj_with_materials.obj
  Data/obj_with_normals_and_tcoords.obj
  Data/objGroup_1a.obj
  Data/objGroup_1b.obj
  Data/objGroup_2a.obj
  Data/objGroup_2b.obj
  Data/post.vtk
  Data/prostar.cel
  Data/prostar.vrt
  Data/relative_indices.obj
  Data/uGridEx.vtk)

set(enable_large_data_tests FALSE)
if (VTK_USE_LARGE_DATA)
  if (DEFINED VTK_WORDS_BIGENDIAN AND NOT VTK_WORDS_BIGENDIAN)
    vtk_module_test_data(
      Data/MFIXReader/,REGEX:.*
      Data/WindBladeReader/,REGEX:.*
      Data/WindBladeReader/field/,REGEX:.*
      Data/WindBladeReader/turbine/,REGEX:.*)
    set(enable_large_data_tests TRUE)
  endif ()
endif ()

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/Particles.raw
    Data/PentaHexa.vtk
    Data/bluntfinq.bin
    Data/bluntfinxyz.bin
    Data/brainImageSmooth.vtk
    Data/caffeine.pdb
    Data/caffeine_notrailingspaces.pdb
    Data/motor.g
    Data/multi-ascii.q
    Data/multi-ascii.xyz
    Data/multi-bin-2D.q
    Data/multi-bin-2D.xyz
    Data/multi-bin-C.q
    Data/multi-bin-C.xyz
    Data/multi-bin-oflow.q
    Data/multi-bin.f
    Data/multi-bin.q
    Data/multi-bin.xyz
    Data/multi.p3d
    Data/points.txt
    Data/prism.neu
    Data/room.cas
    Data/room.dat
    Data/test.p3d
    Data/texThres2.vtk
    Data/vwgt.coords
    Data/vwgt.graph
    Data/room.dat.h5
    Data/room.cas.h5
    Data/mesh_3ddp.dat.h5
    Data/mesh_3ddp.cas.h5)

  add_subdirectory(Python)
endif ()
