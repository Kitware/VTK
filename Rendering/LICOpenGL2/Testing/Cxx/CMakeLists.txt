# basic tests on the legacy filters
# exercises line integral convolution 2d
vtk_add_test_cxx(vtkRenderingLICOpenGL2CxxTests tests
  TestImageDataLIC2D.cxx
  TestStructuredGridLIC2DXSlice.cxx
  TestStructuredGridLIC2DYSlice.cxx
  TestStructuredGridLIC2DZSlice.cxx
  )

# surface lic on a curved surface
vtk_add_test_cxx(vtkRenderingLICOpenGL2CxxTests tests
    --step-size=0.4
    --num-steps=40
    --camera-config=1
    --data=Data/disk_out_ref_surface.vtp
    --vectors=V
    TestSurfaceLICCurvedDefaults,TestSurfaceLIC.cxx
  )
vtk_add_test_cxx(vtkRenderingLICOpenGL2CxxTests tests
    --step-size=0.4
    --num-steps=40
    --camera-config=1
    --data=Data/disk_out_ref_surface.vtp
    --vectors=V
    --enhance-contrast=1
    --lic-intensity=0.8
    TestSurfaceLICCurvedContrastEnhancedBlended,TestSurfaceLIC.cxx
  )
vtk_add_test_cxx(vtkRenderingLICOpenGL2CxxTests tests
    --step-size=0.4
    --num-steps=40
    --camera-config=1
    --data=Data/disk_out_ref_surface.vtp
    --vectors=V
    --enhance-contrast=4
    --color-mode=1
    TestSurfaceLICCurvedContrastEnhancedMapped,TestSurfaceLIC.cxx
  )
vtk_add_test_cxx(vtkRenderingLICOpenGL2CxxTests tests
    --step-size=0.4
    --data=Data/disk_out_ref_surface.vtp
    --camera-config=1
    --vectors=V
    --normalize-vectors=0
    --step-size=0.5
    --num-steps=800
    --enhance-contrast=1
    --color-mode=1
    TestSurfaceLICCurvedEnhancedVectorNormalizeOff,TestSurfaceLIC.cxx
  )
vtk_add_test_cxx(vtkRenderingLICOpenGL2CxxTests tests
    --camera-config=1
    --data=Data/disk_out_ref_surface.vtp
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
    TestSurfaceLICCurvedContrastEnhancedBlendedSmallGrain,TestSurfaceLIC.cxx
  )
vtk_add_test_cxx(vtkRenderingLICOpenGL2CxxTests tests
    --camera-config=1
    --data=Data/disk_out_ref_surface.vtp
    --vectors=V
    --step-size=0.4
    --num-steps=40
    --generate-noise-texture=1
    --noise-texture-size=200
    --noise-grain-size=1
    --number-of-noise-levels=1024
    --enhance-contrast=4
    --color-mode=1
    TestSurfaceLICCurvedContrastEnhancedMappedSmallGrain,TestSurfaceLIC.cxx
  )
vtk_add_test_cxx(vtkRenderingLICOpenGL2CxxTests tests
    --data=Data/disk_out_ref_surface.vtp
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
    TestSurfaceLICCurvedContrastEnhancedMappedSmallVectorNormalizeOff,TestSurfaceLIC.cxx
  )
vtk_add_test_cxx(vtkRenderingLICOpenGL2CxxTests tests
    --step-size=0.4
    --num-steps=40
    --camera-config=1
    --data=Data/disk_out_ref_surface.vtp
    --vectors=V
    --color-by-mag=1
    TestSurfaceLICCurvedDefaultsColor,TestSurfaceLIC.cxx
  )
vtk_add_test_cxx(vtkRenderingLICOpenGL2CxxTests tests
    --step-size=0.4
    --num-steps=40
    --camera-config=1
    --data=Data/disk_out_ref_surface.vtp
    --vectors=V
    --color-by-mag=1
    --interpolate-scalars-before-mapping=1
    --num-discrete-colors=10
    --lic-intensity=0.3
    TestSurfaceLICCurvedDefaultsColorInterpolateScalarsBeforeMapping,TestSurfaceLIC.cxx
  )
vtk_add_test_cxx(vtkRenderingLICOpenGL2CxxTests tests
    --camera-config=1
    --data=Data/disk_out_ref_surface.vtp
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
    TestSurfaceLICCurvedContrastEnhancedColorBlendedSmallGrain,TestSurfaceLIC.cxx
  )
vtk_add_test_cxx(vtkRenderingLICOpenGL2CxxTests tests
    --camera-config=1
    --data=Data/disk_out_ref_surface.vtp
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
    TestSurfaceLICCurvedContrastEnhancedColorMappedSmallGrain,TestSurfaceLIC.cxx
  )
vtk_add_test_cxx(vtkRenderingLICOpenGL2CxxTests tests
    --camera-config=1
    --data=Data/disk_out_ref_surface.vtp
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
    TestSurfaceLICCurvedContrastEnhancedColorBlendedSmallGrainMask,TestSurfaceLIC.cxx
  )
vtk_add_test_cxx(vtkRenderingLICOpenGL2CxxTests tests
    --camera-config=1
    --data=Data/disk_out_ref_surface.vtp
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
    TestSurfaceLICCurvedContrastEnhancedColorMappedSmallGrainMask,TestSurfaceLIC.cxx
  )
vtk_add_test_cxx(vtkRenderingLICOpenGL2CxxTests tests
    --camera-config=1
    --data=Data/disk_out_ref_surface.vtp
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
    TestSurfaceLICCurvedContrastEnhancedSmallGrainMask,TestSurfaceLIC.cxx
  )
# surface lic on a slice
vtk_add_test_cxx(vtkRenderingLICOpenGL2CxxTests tests
    --data=Data/SurfaceVectors.vtk
    --camera-config=3
    --vectors=V
    --step-size=1
    --num-steps=40
    --lic-intensity=0.8
    TestSurfaceLICPlanarDefaults,TestSurfaceLIC.cxx
  )
vtk_add_test_cxx(vtkRenderingLICOpenGL2CxxTests tests
    --data=Data/SurfaceVectors.vtk
    --camera-config=3
    --vectors=V
    --step-size=1
    --num-steps=40
    --enhance-contrast=1
    --color-mode=1
    TestSurfaceLICPlanarContrastEnhanced,TestSurfaceLIC.cxx
  )
vtk_add_test_cxx(vtkRenderingLICOpenGL2CxxTests tests
    --data=Data/SurfaceVectors.vtk
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
    TestSurfaceLICPlanarVectorNormalizeOff,TestSurfaceLIC.cxx
  )
vtk_add_test_cxx(vtkRenderingLICOpenGL2CxxTests tests
    --data=Data/SurfaceVectors.vtk
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
    TestSurfaceLICPlanarVectorNormalizeOffMediumGrainUniform,TestSurfaceLIC.cxx
  )
vtk_add_test_cxx(vtkRenderingLICOpenGL2CxxTests tests
    --data=Data/SurfaceVectors.vtk
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
    TestSurfaceLICPlanarVectorNormalizeOffMediumGrainPerlin,TestSurfaceLIC.cxx
  )

vtk_add_test_cxx(vtkRenderingLICOpenGL2CxxTests tests
    --data=Data/ex-blow_5.vtm
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
    TestSurfaceLICMultiBlockContrastEnhancedPerlin,TestSurfaceLIC.cxx
  )

vtk_test_cxx_executable(vtkRenderingLICOpenGL2CxxTests tests
  RENDERING_FACTORY
  vtkStructuredGridLIC2DTestDriver.cxx
  vtkSurfaceLICTestDriver.cxx
  )
