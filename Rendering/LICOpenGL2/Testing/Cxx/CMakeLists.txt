# basic tests on the legacy filters
# excercises line integral convolution 2d
vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestImageDataLIC2D.cxx
  TestStructuredGridLIC2DXSlice.cxx
  TestStructuredGridLIC2DYSlice.cxx
  TestStructuredGridLIC2DZSlice.cxx
  )

# surface lic painter tests
# get decent coverage by calling
# varying arguments to the following
# test exec. to get unique test names
# avoid vtk_add_test_cxx and add exec
# and it's test cases manually
list(APPEND tests
  TestSurfaceLIC
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests
  RENDERING_FACTORY
  vtkStructuredGridLIC2DTestDriver.cxx
  vtkSurfaceLICTestDriver.cxx
  )
# vtkSurfaceLICTestDriver needs OpenGL
include(vtkOpenGL)
vtk_opengl_link(${vtk-module}CxxTests)

# surface lic on a curved surface
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-SurfaceLICCurvedDefaults
  COMMAND ${vtk-module}CxxTests TestSurfaceLIC
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V "DATA{${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline/TestSurfaceLICCurvedDefaults.png,:}"
    --step-size=0.4
    --num-steps=40
    --camera-config=1
    --data=${VTK_TEST_DATA_DIR}/Data/disk_out_ref_surface.vtp
    --vectors=V
    )
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-SurfaceLICCurvedContrastEnhancedBlended
  COMMAND ${vtk-module}CxxTests TestSurfaceLIC
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V "DATA{${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline/TestSurfaceLICCurvedContrastEnhancedBlended.png,:}"
    --step-size=0.4
    --num-steps=40
    --camera-config=1
    --data=${VTK_TEST_DATA_DIR}/Data/disk_out_ref_surface.vtp
    --vectors=V
    --enhance-contrast=1
    --lic-intensity=0.8
    )
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-SurfaceLICCurvedContrastEnhancedMapped
  COMMAND ${vtk-module}CxxTests TestSurfaceLIC
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V "DATA{${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline/TestSurfaceLICCurvedContrastEnhancedMapped.png,:}"
    --step-size=0.4
    --num-steps=40
    --camera-config=1
    --data=${VTK_TEST_DATA_DIR}/Data/disk_out_ref_surface.vtp
    --vectors=V
    --enhance-contrast=4
    --color-mode=1
    )
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-SurfaceLICCurvedEnhancedVectorNormalizeOff
  COMMAND ${vtk-module}CxxTests TestSurfaceLIC
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V "DATA{${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline/TestSurfaceLICCurvedEnhancedVectorNormalizeOff.png,:}"
    --data=${VTK_TEST_DATA_DIR}/Data/disk_out_ref_surface.vtp
    --camera-config=1
    --vectors=V
    --normalize-vectors=0
    --step-size=0.5
    --num-steps=800
    --enhance-contrast=1
    --color-mode=1
    )
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-SurfaceLICCurvedContrastEnhancedBlendedSmallGrain
  COMMAND ${vtk-module}CxxTests TestSurfaceLIC
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V "DATA{${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline/TestSurfaceLICCurvedContrastEnhancedBlendedSmallGrain.png,:}"
    --camera-config=1
    --data=${VTK_TEST_DATA_DIR}/Data/disk_out_ref_surface.vtp
    --vectors=V
    --step-size=0.4
    --num-steps=40
    --generate-noise-texture=1
    --noise-texture-size=200
    --noise-grain-size=1
    --number-of-noise-levels=1024
    --noise-gen-seed=1
    --enhance-contrast=1
    --lic-intensity=0.8
    )
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-SurfaceLICCurvedContrastEnhancedMappedSmallGrain
  COMMAND ${vtk-module}CxxTests TestSurfaceLIC
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V "DATA{${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline/TestSurfaceLICCurvedContrastEnhancedMappedSmallGrain.png,:}"
    --camera-config=1
    --data=${VTK_TEST_DATA_DIR}/Data/disk_out_ref_surface.vtp
    --vectors=V
    --step-size=0.4
    --num-steps=40
    --generate-noise-texture=1
    --noise-texture-size=200
    --noise-grain-size=1
    --number-of-noise-levels=1024
    --enhance-contrast=4
    --color-mode=1
    )
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-SurfaceLICCurvedContrastEnhancedMappedSmallVectorNormalizeOff
  COMMAND ${vtk-module}CxxTests TestSurfaceLIC
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V "DATA{${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline/TestSurfaceLICCurvedContrastEnhancedMappedSmallVectorNormalizeOff.png,:}"
    --data=${VTK_TEST_DATA_DIR}/Data/disk_out_ref_surface.vtp
    --camera-config=1
    --vectors=V
    --normalize-vectors=0
    --step-size=0.5
    --num-steps=800
    --generate-noise-texture=1
    --noise-texture-size=200
    --noise-grain-size=1
    --number-of-noise-levels=1024
    --enhance-contrast=1
    --low-lic-contrast-enhancement-factor=0.05
    --color-mode=1
    )
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-SurfaceLICCurvedDefaultsColor
  COMMAND ${vtk-module}CxxTests TestSurfaceLIC
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V "DATA{${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline/TestSurfaceLICCurvedDefaultsColor.png,:}"
    --step-size=0.4
    --num-steps=40
    --camera-config=1
    --data=${VTK_TEST_DATA_DIR}/Data/disk_out_ref_surface.vtp
    --vectors=V
    --color-by-mag=1
    )
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-SurfaceLICCurvedContrastEnhancedColorBlendedSmallGrain
  COMMAND ${vtk-module}CxxTests TestSurfaceLIC
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V "DATA{${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline/TestSurfaceLICCurvedContrastEnhancedColorBlendedSmallGrain.png,:}"
    --camera-config=1
    --data=${VTK_TEST_DATA_DIR}/Data/disk_out_ref_surface.vtp
    --vectors=V
    --step-size=0.4
    --num-steps=40
    --generate-noise-texture=1
    --noise-texture-size=200
    --noise-grain-size=1
    --number-of-noise-levels=1024
    --noise-gen-seed=1
    --enhance-contrast=1
    --color-by-mag=1
    --lic-intensity=0.6
    )
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-SurfaceLICCurvedContrastEnhancedColorMappedSmallGrain
  COMMAND ${vtk-module}CxxTests TestSurfaceLIC
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V "DATA{${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline/TestSurfaceLICCurvedContrastEnhancedColorMappedSmallGrain.png,:}"
    --camera-config=1
    --data=${VTK_TEST_DATA_DIR}/Data/disk_out_ref_surface.vtp
    --vectors=V
    --step-size=0.4
    --num-steps=40
    --generate-noise-texture=1
    --noise-texture-size=200
    --noise-grain-size=1
    --number-of-noise-levels=1024
    --enhance-contrast=4
    --color-by-mag=1
    --color-mode=1
    )
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-SurfaceLICCurvedContrastEnhancedColorBlendedSmallGrainMask
  COMMAND ${vtk-module}CxxTests TestSurfaceLIC
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V "DATA{${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline/TestSurfaceLICCurvedContrastEnhancedColorBlendedSmallGrainMask.png,:}"
    --camera-config=1
    --data=${VTK_TEST_DATA_DIR}/Data/disk_out_ref_surface.vtp
    --vectors=V
    --step-size=0.4
    --num-steps=40
    --generate-noise-texture=1
    --noise-texture-size=200
    --noise-grain-size=1
    --number-of-noise-levels=1024
    --noise-gen-seed=1
    --enhance-contrast=1
    --color-by-mag=1
    --lic-intensity=0.6
    --mask-intensity=0.2
    --mask-color 1.0 1.0 1.0
    )
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-SurfaceLICCurvedContrastEnhancedColorMappedSmallGrainMask
  COMMAND ${vtk-module}CxxTests TestSurfaceLIC
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V "DATA{${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline/TestSurfaceLICCurvedContrastEnhancedColorMappedSmallGrainMask.png,:}"
    --camera-config=1
    --data=${VTK_TEST_DATA_DIR}/Data/disk_out_ref_surface.vtp
    --vectors=V
    --step-size=0.4
    --num-steps=40
    --generate-noise-texture=1
    --noise-texture-size=200
    --noise-grain-size=1
    --number-of-noise-levels=1024
    --enhance-contrast=1
    --high-lic-contrast-enhancement-factor=0.1
    --color-by-mag=1
    --color-mode=1
    --map-mode-bias=0.05
    --mask-intensity=0.2
    --mask-color 1.0 1.0 1.0
    )
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-SurfaceLICCurvedContrastEnhancedSmallGrainMask
  COMMAND ${vtk-module}CxxTests TestSurfaceLIC
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V "DATA{${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline/TestSurfaceLICCurvedContrastEnhancedSmallGrainMask.png,:}"
    --camera-config=1
    --data=${VTK_TEST_DATA_DIR}/Data/disk_out_ref_surface.vtp
    --vectors=V
    --normalize-vectors=0
    --step-size=0.5
    --num-steps=800
    --generate-noise-texture=1
    --noise-texture-size=200
    --noise-grain-size=1
    --number-of-noise-levels=1024
    --enhance-contrast=1
    --low-lic-contrast-enhancement-factor=0.05
    --color-mode=1
    --map-mode-bias=0.05
    --mask-intensity=0.2
    )
# surface lic on a slice
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-SurfaceLICPlanarDefaults
  COMMAND ${vtk-module}CxxTests TestSurfaceLIC
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V "DATA{${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline/TestSurfaceLICPlanarDefaults.png,:}"
    --data=${VTK_TEST_DATA_DIR}/Data/SurfaceVectors.vtk
    --camera-config=3
    --vectors=V
    --step-size=1
    --num-steps=40
    --lic-intensity=0.8
    )
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-SurfaceLICPlanarContrastEnhanced
  COMMAND ${vtk-module}CxxTests TestSurfaceLIC
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V "DATA{${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline/TestSurfaceLICPlanarContrastEnhanced.png,:}"
    --data=${VTK_TEST_DATA_DIR}/Data/SurfaceVectors.vtk
    --camera-config=3
    --vectors=V
    --step-size=1
    --num-steps=40
    --enhance-contrast=1
    --color-mode=1
    )
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-SurfaceLICPlanarVectorNormalizeOff
  COMMAND ${vtk-module}CxxTests TestSurfaceLIC
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V "DATA{${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline/TestSurfaceLICPlanarVectorNormalizeOff.png,:}"
    --data=${VTK_TEST_DATA_DIR}/Data/SurfaceVectors.vtk
    --camera-config=3
    --vectors=V
    --normalize-vectors=0
    --step-size=0.5
    --num-steps=2000
    --generate-noise-texture=1
    --noise-texture-size=128
    --noise-grain-size=2
    --number-of-noise-levels=256
    --noise-gen-seed=12367
    --enhance-contrast=1
    --color-by-mag=1
    --color-mode=1
    )
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-SurfaceLICPlanarVectorNormalizeOffMediumGrainUniform
  COMMAND ${vtk-module}CxxTests TestSurfaceLIC
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V "DATA{${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline/TestSurfaceLICPlanarVectorNormalizeOffMediumGrainUniform.png,:}"
    --data=${VTK_TEST_DATA_DIR}/Data/SurfaceVectors.vtk
    --camera-config=3
    --vectors=V
    --enhanced-lic=0
    --step-size=0.25
    --num-steps=40
    --generate-noise-texture=1
    --noise-type=1
    --noise-texture-size=128
    --noise-grain-size=2
    --impulse-noise-prob=0.04
    --enhance-contrast=4
    --high-color-contrast-enhancement-factor=0.2
    --color-by-mag=1
    --color-mode=1
    )
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-SurfaceLICPlanarVectorNormalizeOffMediumGrainPerlin
  COMMAND ${vtk-module}CxxTests TestSurfaceLIC
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V "DATA{${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline/TestSurfaceLICPlanarVectorNormalizeOffMediumGrainPerlin.png,:}"
    --data=${VTK_TEST_DATA_DIR}/Data/SurfaceVectors.vtk
    --camera-config=3
    --vectors=V
    --normalize-vectors=0
    --step-size=2
    --num-steps=400
    --generate-noise-texture=1
    --noise-type=2
    --noise-texture-size=256
    --noise-grain-size=8
    --max-noise-value=0.8
    --enhance-contrast=4
    --color-by-mag=1
    --color-mode=1
    --anti-alias=1
    )
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-SurfaceLICMultiBlockContrastEnhancedPerlin
  COMMAND ${vtk-module}CxxTests TestSurfaceLIC
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V "DATA{${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline/TestSurfaceLICMultiBlockContrastEnhancedPerlin.png,:}"
    --data=${VTK_TEST_DATA_DIR}/Data/ex-blow_5.vtm
    --camera-config=4
    --vectors=DISPLACEMENT_
    --step-size=0.1
    --num-steps=50
    --generate-noise-texture=1
    --noise-type=2
    --noise-texture-size=128
    --noise-grain-size=2
    --number-of-noise-levels=1024
    --noise-gen-seed=1
    --enhance-contrast=4
    --color-by-mag=1
    --color-mode=1
    --map-mode-bias=0.1
    --mask-on-surface=1
    --mask-intensity=0.41
    --mask-color 0 0 0
    )
