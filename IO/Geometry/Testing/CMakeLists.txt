vtk_module_test_data(
  Data/42400-IDGH.stl
  Data/AMR/Enzo/DD0010/,REGEX:.*
  Data/IncOctPntLocData.dat
  Data/IncOctPntLocResult.dat
  Data/OpenFOAM/64BitFloats/,REGEX:.*
  Data/OpenFOAM/64BitFloats/0/,REGEX:.*
  Data/OpenFOAM/64BitFloats/constant/,REGEX:.*
  Data/OpenFOAM/64BitFloats/constant/polyMesh/,REGEX:.*
  Data/OpenFOAM/64BitFloats/system/,REGEX:.*
  Data/OpenFOAM/cavity/,REGEX:.*
  Data/OpenFOAM/cavity/0/,REGEX:.*
  Data/OpenFOAM/cavity/0.5/,REGEX:.*
  Data/OpenFOAM/cavity/0.5/uniform/,REGEX:.*
  Data/OpenFOAM/cavity/1/,REGEX:.*
  Data/OpenFOAM/cavity/1/uniform/,REGEX:.*
  Data/OpenFOAM/cavity/1.5/,REGEX:.*
  Data/OpenFOAM/cavity/1.5/uniform/,REGEX:.*
  Data/OpenFOAM/cavity/2.5/,REGEX:.*
  Data/OpenFOAM/cavity/2.5/uniform/,REGEX:.*
  Data/OpenFOAM/cavity/2/,REGEX:.*
  Data/OpenFOAM/cavity/2/uniform/,REGEX:.*
  Data/OpenFOAM/cavity/constant/,REGEX:.*
  Data/OpenFOAM/cavity/constant/polyMesh/,REGEX:.*
  Data/OpenFOAM/cavity/system/,REGEX:.*
  Data/OpenFOAM/regex/,REGEX:.*
  Data/OpenFOAM/regex/0/,REGEX:.*
  Data/OpenFOAM/regex/constant/,REGEX:.*
  Data/OpenFOAM/regex/constant/polyMesh/,REGEX:.*
  Data/OpenFOAM/regex/system/,REGEX:.*
  Data/TecPlot/,REGEX:.*
  Data/Viewpoint/cow.obj
  Data/absolute_indices.obj
  Data/cellcentered.tec
  Data/cellsnd.ascii.inp
  Data/cellsnd.bin.inp
  Data/flow.tec
  Data/flow.tec.gz
  Data/obj_multitexture.obj
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

add_subdirectory(Cxx)

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
    Data/thio3xx.xyz
    Data/vwgt.coords
    Data/vwgt.graph)

  add_subdirectory(Python)
endif ()
