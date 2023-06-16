# Modules that should not be wrapped to javascript.
# This list of modules are all built in a wasm build.
# Right now, all of them are wrappable with some exceptions.
# The troublesome header files are skipped towards the end of this file.
list(APPEND vtk_module_wrap_javascript_skip_modules
  "VTK::ParallelCore"
)

# Header files that should not be wrapped to javascript.
list(APPEND vtk_module_wrap_javascript_skip_headers
  # Module: VTK::RenderingCore
  # Reason: destructor is private
  vtkCellGraphicsPrimitiveMap
  # Module: VTK::CommonDataModel
  # Reason: destructor is private
  vtkPolyhedronUtilities
  # Module: VTK::FiltersSMP
  # Reason: destructor is protected and there is no delete function
  vtkSMPMergePolyDataHelper
  # Module: VTK::ImagingCore
  # Reason: destructor is protected and there is no delete function
  vtkImageBSplineInternals
  # Module: VTK::ImagingMathematics
  # Reason: Class provides overloads for SetInputConnection which take precedence for all vtkAlgorithm(s)
  #         Error when calling any filter.SetInputConnection => expected null or instance of vtkImageMathematics, got an instance of vtkObjectBase
  vtkImageMathematics
  # Module: VTK::ImagingMorphological
  # Reason: destructor is protected and there is no delete function
  vtkImageConnector
  # Module: VTK::InteractionWidgets
  # Reason: compiler thinks vtkWidgetEvent is of type vtkWidgetEvent::WidgetEventIds? 
  #         confuses vtkWidgetEvent::Delete() with the enum vtkWidgetEvent::WidgetEventIds::Delete?
  vtkWidgetEvent
  # Module: VTK::RenderingOpenGL2
  # Reason: destructor is protected and there is no delete function
  vtkGLSLModifierFactory
  # Module: VTK::FiltersGeneral
  # Reason: defines vtkPolyDataAlgorithm as vtkTemporalAlgorithm<vtkPolyDataAlgorithm>
  #         confuses other filters in VTK::FiltersCore
  vtkTemporalPathLineFilter
)
