# Removal of old deprecated APIs

The following APIs were deprecated in 9.0 or earlier and are now removed:

- `vtkPlot::GetNearestPoint(const vtkVector2f&, const vtkVector2f&, vtkVector2f*)`
- `vtkPlot::LegacyRecursionFlag` (used to help subclasses implement the
  replacement for the prior method)
- The following APIs have been replaced by `vtkOutputWindow::SetDisplayMode()`:
  - `vtkOutputWindow::SetUseStdErrorForAllMessages()`
  - `vtkOutputWindow::GetUseStdErrorForAllMessages()`
  - `vtkOutputWindow::UseStdErrorForAllMessagesOn()`
  - `vtkOutputWindow::UseStdErrorForAllMessagesOff()`
  - `vtkWin32OutputWindow::SetSendToStdErr()`
  - `vtkWin32OutputWindow::GetSendToStdErr()`
  - `vtkWin32OutputWindow::SendToStdErrOn()`
  - `vtkWin32OutputWindow::SendToStdErrOff()`
- `vtkArrayDispatcher`, `vtkDispatcher`, `vtkDoubleDispatcher` have been
  replaced by `vtkArrayDispatch`
- Fetching edge and face points via `int` rather than `vtkIdType`:
  - `vtkConvexPointSet::GetEdgePoints(int, int*&)`
  - `vtkConvexPointSet::GetFacePoints(int, int*&)`
  - `vtkHexagonalPrism::GetEdgePoints(int, int*&)`
  - `vtkHexagonalPrism::GetFacePoints(int, int*&)`
  - `vtkHexahedron::GetEdgePoints(int, int*&)`
  - `vtkHexahedron::GetFacePoints(int, int*&)`
  - `vtkPentagonalPrism::GetEdgePoints(int, int*&)`
  - `vtkPentagonalPrism::GetFacePoints(int, int*&)`
  - `vtkPolyhedron::GetEdgePoints(int, int*&)`
  - `vtkPolyhedron::GetFacePoints(int, int*&)`
  - `vtkPyramid::GetEdgePoints(int, int*&)`
  - `vtkPyramid::GetFacePoints(int, int*&)`
  - `vtkTetra::GetEdgePoints(int, int*&)`
  - `vtkTetra::GetFacePoints(int, int*&)`
  - `vtkVoxel::GetEdgePoints(int, int*&)`
  - `vtkVoxel::GetFacePoints(int, int*&)`
  - `vtkWedge::GetEdgePoints(int, int*&)`
  - `vtkWedge::GetFacePoints(int, int*&)`
- Querying point cells with an `unsigned short` count of cells:
  - `vtkPolyData::GetPointCells(vtkIdType, unsigned short&, vtkIdType*&)`
  - `vtkUnstructuredGrid::GetPointCells(vtkIdType, unsigned short&, vtkIdType*&)`
- `vtkAlgorithm::SetProgress()` has been replaced by
  `vtkAlgorithm::UpdateProgress()`
- The following APIs have been replaced by
  `vtkResourceFileLocator::SetLogVerbosity()`:
  - `vtkResourceFileLocator::SetPrintDebugInformation()`
  - `vtkResourceFileLocator::GetPrintDebugInformation()`
  - `vtkResourceFileLocator::PrintDebugInformationOn()`
  - `vtkResourceFileLocator::PrintDebugInformationOff()`
- `vtkIdFilter::SetIdsArrayName()` has been replaced by
  `vtkIdFilter::SetPointIdsArrayName()` and
  `vtkIdFilter::SetCellIdsArrayName()`
- `vtkExtractTemporalFieldData` has been replaced by
  `vtkExtractExodusGlobalTemporalVariables`
- `vtkTemporalStreamTracer` and `vtkPTemporalStreamTracer` have been replaced
  by `vtkParticleTracerBase`, `vtkParticleTracer`, `vtkParticlePathFilter`, or
  `vtkStreaklineFilter`
- `vtkHyperTreeGridSource::GetMaximumLevel()` and
  `vtkHyperTreeGridSource::SetMaximumLevel()` have been replaced by
  `vtkHyperTreeGridSource::GetMaxDepth()` and
  `vtkHyperTreeGridSource::SetMaxDepth()`
- `QVTKOpenGLNativeWidget`, `QVTKOpenGLStereoWidget`, `QVTKOpenGLWindow`
  methods have been removed:
  - `::SetRenderWindow()` is now `::setRenderWindow()`
  - `::GetRenderWindow()` is now `::renderWindow()`
  - `::GetInteractor()` and `GetInteractorAdaptor()` have been removed
  - `::setQVTKCursor()` is now `QWidget::setCursor()`
  - `::setDefaultQVTKCursor()` is now `QWidget::setDefaultCursor()`
- `QVTKOpenGLWidget` is replaced by `QVTKOpenGLStereoWidget`
- `vtkJSONDataSetWriter::{Get,Set}FileName()` is now
  `vtkJSONDataSetWriter::{Get,Set}ArchiveName()`
- `vtkLineRepresentation::SetRestrictFlag()` has been removed
- The following `vtkRenderWindow` methods have been removed:
  - `GetIsPicking()`
  - `SetIsPicking()`
  - `IsPickingOn()`
  - `IsPickingOff()`
- The following APIs have been replaced by `vtkShaderProperty` methods of the
  same names:
  - `vtkOpenGLPolyDataMapper::AddShaderReplacement()`
  - `vtkOpenGLPolyDataMapper::ClearShaderReplacement()`
  - `vtkOpenGLPolyDataMapper::ClearAllShaderReplacements()`
  - `vtkOpenGLPolyDataMapper::ClearAllShaderReplacements()`
  - `vtkOpenGLPolyDataMapper::SetVertexShaderCode()`
  - `vtkOpenGLPolyDataMapper::GetVertexShaderCode()`
  - `vtkOpenGLPolyDataMapper::SetFragmentShaderCode()`
  - `vtkOpenGLPolyDataMapper::GetFragmentShaderCode()`
  - `vtkOpenGLPolyDataMapper::SetGeometryShaderCode()`
  - `vtkOpenGLPolyDataMapper::GetGeometryShaderCode()`
- The following APIs have been removed (they supported the legacy shader
  replacements):
  - `vtkOpenGLPolyDataMapper::GetLegacyShaderProperty()`
  - `vtkOpenGLPolyDataMapper::LegacyShaderProperty`
- The following APIs have been removed since only `FLOATING_POINT` mode is now
  supported:
  - `vtkValuePass::SetRenderingMode()`
  - `vtkValuePass::GetRenderingMode()`
  - `vtkValuePass::SetInputArrayToProcess()`
  - `vtkValuePass::SetInputComponentToProcess()`
  - `vtkValuePass::SetScalarRange()`
  - `vtkValuePass::IsFloatingPointModeSupported()`
  - `vtkValuePass::ColorToValue()`
- `vtkPythonInterpreter::GetPythonVerboseFlag()` has been replaced by
  `vtkPythonInterpreter::GetLogVerbosity()`
