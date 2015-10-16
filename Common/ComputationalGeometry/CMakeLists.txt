set(Module_SRCS
  vtkCardinalSpline.cxx
  vtkKochanekSpline.cxx
  vtkParametricBoy.cxx
  vtkParametricConicSpiral.cxx
  vtkParametricCrossCap.cxx
  vtkParametricDini.cxx
  vtkParametricEllipsoid.cxx
  vtkParametricEnneper.cxx
  vtkParametricFigure8Klein.cxx
  vtkParametricFunction.cxx
  vtkParametricKlein.cxx
  vtkParametricMobius.cxx
  vtkParametricRandomHills.cxx
  vtkParametricRoman.cxx
  vtkParametricSpline.cxx
  vtkParametricSuperEllipsoid.cxx
  vtkParametricSuperToroid.cxx
  vtkParametricTorus.cxx
  vtkParametricKuen.cxx
  vtkParametricPseudosphere.cxx
  vtkParametricBohemianDome.cxx
  vtkParametricHenneberg.cxx
  vtkParametricCatalanMinimal.cxx
  vtkParametricBour.cxx
  vtkParametricPluckerConoid.cxx
  )

set_source_files_properties(
  vtkParametricFunction
  ABSTRACT
)

vtk_module_library(vtkCommonComputationalGeometry ${Module_SRCS})
