vtk_module_test_data(
  Data/ANALYZE.IMG.GZ
  Data/ANALYZE.HDR
  Data/avg152T1_RL_nifti.nii.gz
  Data/avg152T1_RL_nifti2.nii.gz
  Data/beach.ascii
  Data/beach.ascii.nhdr
  Data/beach.nrrd
  Data/dicom/collection/,REGEX:.*
  Data/filtered_func_data.nii.gz
  Data/minimal.hdr.gz
  Data/minimal.img.gz
  Data/minimal.nii.gz
  Data/nifti_rgb.nii.gz
  Data/planar_rgb.nii.gz
  Data/small.H
  Data/small.H@)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/SainteHelens.dem
    Data/beach.jpg
    Data/beach.tif
    Data/foot/foot.mha
    Data/foot/foot.raw
    Data/headsq/,REGEX:.*
    Data/masonry.bmp)
  add_subdirectory(Python)
endif ()
