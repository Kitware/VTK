set(Module_SRCS
  vtkAdaptiveSubdivisionFilter.cxx
  vtkBandedPolyDataContourFilter.cxx
  vtkButterflySubdivisionFilter.cxx
  vtkContourLoopExtraction.cxx
  vtkCookieCutter.cxx
  vtkDijkstraGraphGeodesicPath.cxx
  vtkDijkstraImageGeodesicPath.cxx
  vtkFillHolesFilter.cxx
  vtkGeodesicPath.cxx
  vtkGraphGeodesicPath.cxx
  vtkLinearExtrusionFilter.cxx
  vtkLinearSubdivisionFilter.cxx
  vtkLoopSubdivisionFilter.cxx
  vtkPolyDataPointSampler.cxx
  vtkProjectedTexture.cxx
  vtkQuadRotationalExtrusionFilter.cxx
  vtkRibbonFilter.cxx
  vtkRotationalExtrusionFilter.cxx
  vtkRuledSurfaceFilter.cxx
  vtkSelectEnclosedPoints.cxx
  vtkSelectPolyData.cxx
  vtkSpherePuzzleArrows.cxx
  vtkSpherePuzzle.cxx
  vtkSubdivideTetra.cxx
  vtkOutlineFilter.cxx
  vtkSectorSource.cxx
  vtkVolumeOfRevolutionFilter.cxx
  )

set_source_files_properties(
  vtkGeodesicPath
  vtkGraphGeodesicPath
  ABSTRACT
  )

vtk_module_library(vtkFiltersModeling ${Module_SRCS})
