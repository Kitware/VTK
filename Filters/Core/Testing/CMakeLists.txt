vtk_module_test_data(
  Data/CuspySurface.vtp
  Data/Delaunay/,REGEX:.*
  Data/beach.jpg
  Data/can.ex2
  Data/cow.vtp
  Data/delaunay3d.vtu
  Data/disk_out_ref_surface.vtp
  Data/explicitStructuredGrid.vtu
  Data/explicitStructuredGridEmpty.vtu
  Data/sliding_multi_block_end.vtm
  Data/sliding_multi_block_end/sliding_multi_block_end_0_0.vtu
  Data/sliding_multi_block_end/sliding_multi_block_end_1_0.vtu
  Data/sliding_multi_block_end/sliding_multi_block_end_2_0.vtu
  Data/sliding_multi_block_end/sliding_multi_block_end_3_0.vtu
  Data/sliding_multi_block_start.vtm
  Data/sliding_multi_block_start/sliding_multi_block_start_0_0.vtu
  Data/sliding_multi_block_start/sliding_multi_block_start_1_0.vtu
  Data/sliding_multi_block_start/sliding_multi_block_start_2_0.vtu
  Data/sliding_multi_block_start/sliding_multi_block_start_3_0.vtu
  Data/slightlyRotated.vtu
  Data/tex_debug.jpg
  Data/wavelet300Arrays.vti)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/3GQP.pdb
    Data/AMR/Enzo/DD0010/,REGEX:.*
    Data/B.pgm
    Data/RectGrid2.vtk
    Data/Viewpoint/cow.g
    Data/Viewpoint/cow.obj
    Data/beach.tif
    Data/blow.vtk
    Data/combq.bin
    Data/combxyz.bin
    Data/cth.vtr
    Data/cuttertest.vtu
    Data/disk_out_ref.ex2
    Data/financial.txt
    Data/financial.vtk
    Data/fran_cut.png
    Data/fran_cut.vtk
    Data/fullhead15.png
    Data/headsq/,REGEX:.*
    Data/masonry.bmp
    Data/plate.vtk
    Data/polyEx.vtk
    Data/quadTetEdgeTest.vtk
    Data/teapot.g
    Data/tensors.vtk)

  add_subdirectory(Python)
endif ()
