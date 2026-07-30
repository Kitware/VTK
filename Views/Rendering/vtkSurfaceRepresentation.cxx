// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSurfaceRepresentation.h"

#include "vtkActor.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSet.h"
#include "vtkExtractSelection.h"
#include "vtkGeometryFilterDispatcher.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderViewBase.h"
#include "vtkRenderer.h"
#include "vtkScalarBarActor.h"
#include "vtkScalarsToColors.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"

#include <algorithm>
#include <cstring>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSurfaceRepresentation);

//------------------------------------------------------------------------------
vtkSurfaceRepresentation::vtkSurfaceRepresentation()
{
  this->SetNumberOfInputPorts(1);

  this->GeometryFilter->SetUseOutline(0);
  // Explicitly enable pass-through of original IDs so that ConvertSelection
  // can map rendered surface cell/point IDs back to the original dataset.
  this->GeometryFilter->SetPassThroughCellIds(1);
  this->GeometryFilter->SetPassThroughPointIds(1);

  this->ScalarBarVisible = false;
  this->RepresentationValue = SURFACE;

  this->Actor->SetMapper(this->Mapper);

  // Scalar bar (always created; visibility controls whether it draws).
  this->ScalarBar->SetLookupTable(this->Mapper->GetLookupTable());
  this->ScalarBar->SetNumberOfLabels(5);
  this->ScalarBar->SetVisibility(this->ScalarBarVisible);

  // Selection visualization pipeline
  this->SelectionGeometryFilter->SetUseOutline(0);
  this->SelectionGeometryFilter->SetInputConnection(this->SelectionExtractor->GetOutputPort());
  this->SelectionMapper->SetInputConnection(this->SelectionGeometryFilter->GetOutputPort());
  this->SelectionMapper->ScalarVisibilityOff();
  this->SelectionMapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->SelectionActor->SetMapper(this->SelectionMapper);
  this->SelectionActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
  this->SelectionActor->GetProperty()->SetLineWidth(2.0);
  this->SelectionActor->GetProperty()->SetPointSize(8.0);
  this->SelectionActor->GetProperty()->SetRenderPointsAsSpheres(true);
  this->SelectionActor->GetProperty()->SetRepresentationToWireframe();
  this->SelectionActor->GetProperty()->LightingOff();
  this->SelectionActor->SetVisibility(false);
  this->SelectionActor->SetPickable(false);
}

//------------------------------------------------------------------------------
vtkSurfaceRepresentation::~vtkSurfaceRepresentation() = default;

//------------------------------------------------------------------------------
int vtkSurfaceRepresentation::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkAlgorithmOutput* inputPort = this->GetInternalOutputPort();
  if (inputPort)
  {
    this->GeometryFilter->SetInputConnection(inputPort);
    this->GeometryFilter->Update();
    this->Mapper->SetInputDataObject(this->GeometryFilter->GetOutputDataObject(0));
  }
  return 1;
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::AddToView(vtkView* view)
{
  vtkRenderViewBase* rv = vtkRenderViewBase::SafeDownCast(view);
  if (!rv)
  {
    vtkErrorMacro("Can only add vtkSurfaceRepresentation to a vtkRenderViewBase subclass.");
    return false;
  }
  rv->GetRenderer()->AddActor(this->Actor);
  rv->GetRenderer()->AddActor(this->SelectionActor);
  // Always add scalar bar to renderer; visibility controls whether it draws.
  rv->GetRenderer()->AddViewProp(this->ScalarBar);
  return true;
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::RemoveFromView(vtkView* view)
{
  vtkRenderViewBase* rv = vtkRenderViewBase::SafeDownCast(view);
  if (!rv)
  {
    return false;
  }
  rv->GetRenderer()->RemoveActor(this->Actor);
  rv->GetRenderer()->RemoveActor(this->SelectionActor);
  rv->GetRenderer()->RemoveViewProp(this->ScalarBar);
  return true;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkSurfaceRepresentation::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  mTime = std::max(mTime, this->GeometryFilter->GetMTime());
  mTime = std::max(mTime, this->Mapper->GetMTime());
  // vtkActor::GetMTime() already accounts for the property.
  mTime = std::max(mTime, this->Actor->GetMTime());
  mTime = std::max(mTime, this->ScalarBar->GetMTime());
  mTime = std::max(mTime, this->SelectionExtractor->GetMTime());
  mTime = std::max(mTime, this->SelectionGeometryFilter->GetMTime());
  mTime = std::max(mTime, this->SelectionMapper->GetMTime());
  mTime = std::max(mTime, this->SelectionActor->GetMTime());
  return mTime;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetRepresentation(int type)
{
  int representation;
  bool edgeVisibility;
  switch (type)
  {
    case POINTS:
      representation = VTK_POINTS;
      edgeVisibility = false;
      break;
    case WIREFRAME:
      representation = VTK_WIREFRAME;
      edgeVisibility = false;
      break;
    case SURFACE:
      representation = VTK_SURFACE;
      edgeVisibility = false;
      break;
    case SURFACE_WITH_EDGES:
      representation = VTK_SURFACE;
      edgeVisibility = true;
      break;
    default:
      vtkWarningMacro("Unknown representation type: " << type);
      return;
  }
  if (this->RepresentationValue == type)
  {
    return;
  }
  this->RepresentationValue = type;
  vtkProperty* prop = this->Actor->GetProperty();
  prop->SetRepresentation(representation);
  prop->SetEdgeVisibility(edgeVisibility);
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkSurfaceRepresentation::GetRepresentation()
{
  return this->RepresentationValue;
}

//------------------------------------------------------------------------------
const char* vtkSurfaceRepresentation::GetRepresentationAsString()
{
  switch (this->RepresentationValue)
  {
    case POINTS:
      return "Points";
    case WIREFRAME:
      return "Wireframe";
    case SURFACE:
      return "Surface";
    case SURFACE_WITH_EDGES:
      return "SurfaceWithEdges";
    default:
      return "Unknown";
  }
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetColor(double r, double g, double b)
{
  double* current = this->GetColor();
  if (current[0] == r && current[1] == g && current[2] == b)
  {
    return;
  }
  this->Actor->GetProperty()->SetColor(r, g, b);
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkSurfaceRepresentation::GetColor()
{
  return this->Actor->GetProperty()->GetColor();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetOpacity(double val)
{
  if (this->GetOpacity() == val)
  {
    return;
  }
  this->Actor->GetProperty()->SetOpacity(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkSurfaceRepresentation::GetOpacity()
{
  return this->Actor->GetProperty()->GetOpacity();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetEdgeColor(double r, double g, double b)
{
  double* current = this->GetEdgeColor();
  if (current[0] == r && current[1] == g && current[2] == b)
  {
    return;
  }
  this->Actor->GetProperty()->SetEdgeColor(r, g, b);
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkSurfaceRepresentation::GetEdgeColor()
{
  return this->Actor->GetProperty()->GetEdgeColor();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetEdgeOpacity(double val)
{
  if (this->GetEdgeOpacity() == val)
  {
    return;
  }
  this->Actor->GetProperty()->SetEdgeOpacity(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkSurfaceRepresentation::GetEdgeOpacity()
{
  return this->Actor->GetProperty()->GetEdgeOpacity();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetAmbient(double val)
{
  if (this->GetAmbient() == val)
  {
    return;
  }
  this->Actor->GetProperty()->SetAmbient(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkSurfaceRepresentation::GetAmbient()
{
  return this->Actor->GetProperty()->GetAmbient();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetDiffuse(double val)
{
  if (this->GetDiffuse() == val)
  {
    return;
  }
  this->Actor->GetProperty()->SetDiffuse(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkSurfaceRepresentation::GetDiffuse()
{
  return this->Actor->GetProperty()->GetDiffuse();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetSpecular(double val)
{
  if (this->GetSpecular() == val)
  {
    return;
  }
  this->Actor->GetProperty()->SetSpecular(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkSurfaceRepresentation::GetSpecular()
{
  return this->Actor->GetProperty()->GetSpecular();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetSpecularPower(double val)
{
  if (this->GetSpecularPower() == val)
  {
    return;
  }
  this->Actor->GetProperty()->SetSpecularPower(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkSurfaceRepresentation::GetSpecularPower()
{
  return this->Actor->GetProperty()->GetSpecularPower();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetLineWidth(double val)
{
  if (this->GetLineWidth() == val)
  {
    return;
  }
  this->Actor->GetProperty()->SetLineWidth(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkSurfaceRepresentation::GetLineWidth()
{
  return this->Actor->GetProperty()->GetLineWidth();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetPointSize(double val)
{
  if (this->GetPointSize() == val)
  {
    return;
  }
  this->Actor->GetProperty()->SetPointSize(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkSurfaceRepresentation::GetPointSize()
{
  return this->Actor->GetProperty()->GetPointSize();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetLighting(bool val)
{
  if (this->GetLighting() == val)
  {
    return;
  }
  this->Actor->GetProperty()->SetLighting(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetLighting()
{
  return this->Actor->GetProperty()->GetLighting();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetInterpolation(int val)
{
  if (this->GetInterpolation() == val)
  {
    return;
  }
  this->Actor->GetProperty()->SetInterpolation(val);
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkSurfaceRepresentation::GetInterpolation()
{
  return this->Actor->GetProperty()->GetInterpolation();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetRenderPointsAsSpheres(bool val)
{
  if (this->GetRenderPointsAsSpheres() == val)
  {
    return;
  }
  this->Actor->GetProperty()->SetRenderPointsAsSpheres(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetRenderPointsAsSpheres()
{
  return this->Actor->GetProperty()->GetRenderPointsAsSpheres();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetRenderLinesAsTubes(bool val)
{
  if (this->GetRenderLinesAsTubes() == val)
  {
    return;
  }
  this->Actor->GetProperty()->SetRenderLinesAsTubes(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetRenderLinesAsTubes()
{
  return this->Actor->GetProperty()->GetRenderLinesAsTubes();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetRoughness(double val)
{
  if (this->GetRoughness() == val)
  {
    return;
  }
  this->Actor->GetProperty()->SetRoughness(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkSurfaceRepresentation::GetRoughness()
{
  return this->Actor->GetProperty()->GetRoughness();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetMetallic(double val)
{
  if (this->GetMetallic() == val)
  {
    return;
  }
  this->Actor->GetProperty()->SetMetallic(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkSurfaceRepresentation::GetMetallic()
{
  return this->Actor->GetProperty()->GetMetallic();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetUseOutline(bool val)
{
  if (this->GetUseOutline() == val)
  {
    return;
  }
  this->GeometryFilter->SetUseOutline(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetUseOutline()
{
  return this->GeometryFilter->GetUseOutline() != 0;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetGenerateFeatureEdges(bool val)
{
  if (this->GetGenerateFeatureEdges() == val)
  {
    return;
  }
  this->GeometryFilter->SetGenerateFeatureEdges(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetGenerateFeatureEdges()
{
  return this->GeometryFilter->GetGenerateFeatureEdges();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetGeneratePointNormals(bool val)
{
  if (this->GetGeneratePointNormals() == val)
  {
    return;
  }
  this->GeometryFilter->SetGeneratePointNormals(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetGeneratePointNormals()
{
  return this->GeometryFilter->GetGeneratePointNormals();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetGenerateCellNormals(bool val)
{
  if (this->GetGenerateCellNormals() == val)
  {
    return;
  }
  this->GeometryFilter->SetGenerateCellNormals(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetGenerateCellNormals()
{
  return this->GeometryFilter->GetGenerateCellNormals() != 0;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetFeatureAngle(double val)
{
  if (this->GetFeatureAngle() == val)
  {
    return;
  }
  this->GeometryFilter->SetFeatureAngle(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkSurfaceRepresentation::GetFeatureAngle()
{
  return this->GeometryFilter->GetFeatureAngle();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetSplitting(bool val)
{
  if (this->GetSplitting() == val)
  {
    return;
  }
  this->GeometryFilter->SetSplitting(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetSplitting()
{
  return this->GeometryFilter->GetSplitting() != 0;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetTriangulate(bool val)
{
  if (this->GetTriangulate() == val)
  {
    return;
  }
  this->GeometryFilter->SetTriangulate(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetTriangulate()
{
  return this->GeometryFilter->GetTriangulate() != 0;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetNonlinearSubdivisionLevel(int val)
{
  if (this->GetNonlinearSubdivisionLevel() == val)
  {
    return;
  }
  this->GeometryFilter->SetNonlinearSubdivisionLevel(val);
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkSurfaceRepresentation::GetNonlinearSubdivisionLevel()
{
  return this->GeometryFilter->GetNonlinearSubdivisionLevel();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetMatchBoundariesIgnoringCellOrder(bool val)
{
  if (this->GetMatchBoundariesIgnoringCellOrder() == val)
  {
    return;
  }
  this->GeometryFilter->SetMatchBoundariesIgnoringCellOrder(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetMatchBoundariesIgnoringCellOrder()
{
  return this->GeometryFilter->GetMatchBoundariesIgnoringCellOrder() != 0;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetPassThroughCellIds(bool val)
{
  if (this->GetPassThroughCellIds() == val)
  {
    return;
  }
  this->GeometryFilter->SetPassThroughCellIds(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetPassThroughCellIds()
{
  return this->GeometryFilter->GetPassThroughCellIds() != 0;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetPassThroughPointIds(bool val)
{
  if (this->GetPassThroughPointIds() == val)
  {
    return;
  }
  this->GeometryFilter->SetPassThroughPointIds(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetPassThroughPointIds()
{
  return this->GeometryFilter->GetPassThroughPointIds() != 0;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetBlockColorsDistinctValues(int val)
{
  if (this->GetBlockColorsDistinctValues() == val)
  {
    return;
  }
  this->GeometryFilter->SetBlockColorsDistinctValues(val);
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkSurfaceRepresentation::GetBlockColorsDistinctValues()
{
  return this->GeometryFilter->GetBlockColorsDistinctValues();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetHideInternalAMRFaces(bool val)
{
  if (this->GetHideInternalAMRFaces() == val)
  {
    return;
  }
  this->GeometryFilter->SetHideInternalAMRFaces(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetHideInternalAMRFaces()
{
  return this->GeometryFilter->GetHideInternalAMRFaces();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetUseNonOverlappingAMRMetaDataForOutlines(bool val)
{
  if (this->GetUseNonOverlappingAMRMetaDataForOutlines() == val)
  {
    return;
  }
  this->GeometryFilter->SetUseNonOverlappingAMRMetaDataForOutlines(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetUseNonOverlappingAMRMetaDataForOutlines()
{
  return this->GeometryFilter->GetUseNonOverlappingAMRMetaDataForOutlines();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetGenerateProcessIds(bool val)
{
  if (this->GetGenerateProcessIds() == val)
  {
    return;
  }
  this->GeometryFilter->SetGenerateProcessIds(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetGenerateProcessIds()
{
  return this->GeometryFilter->GetGenerateProcessIds();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetScalarVisibility(bool val)
{
  if (this->GetScalarVisibility() == val)
  {
    return;
  }
  this->Mapper->SetScalarVisibility(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetScalarVisibility()
{
  return this->Mapper->GetScalarVisibility() != 0;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::ColorByPointArray(const char* arrayName)
{
  if (this->IsColoringBy(arrayName, VTK_SCALAR_MODE_USE_POINT_FIELD_DATA))
  {
    return;
  }
  this->Mapper->SetScalarVisibility(true);
  this->Mapper->SetScalarModeToUsePointFieldData();
  this->Mapper->SelectColorArray(arrayName);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::ColorByPointArray(const char* arrayName, int component)
{
  this->ColorByPointArray(arrayName);
  this->ColorByComponent(component);
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::ColorByCellArray(const char* arrayName)
{
  if (this->IsColoringBy(arrayName, VTK_SCALAR_MODE_USE_CELL_FIELD_DATA))
  {
    return;
  }
  this->Mapper->SetScalarVisibility(true);
  this->Mapper->SetScalarModeToUseCellFieldData();
  this->Mapper->SelectColorArray(arrayName);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::ColorByCellArray(const char* arrayName, int component)
{
  this->ColorByCellArray(arrayName);
  this->ColorByComponent(component);
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::IsColoringBy(const char* arrayName, int scalarMode)
{
  if (!this->Mapper->GetScalarVisibility() || this->Mapper->GetScalarMode() != scalarMode ||
    this->Mapper->GetArrayAccessMode() != VTK_GET_ARRAY_BY_NAME)
  {
    return false;
  }
  const char* current = this->Mapper->GetArrayName();
  if (!current || !arrayName)
  {
    return current == arrayName;
  }
  return strcmp(current, arrayName) == 0;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::ColorByComponent(int component)
{
  vtkScalarsToColors* lut = this->Mapper->GetLookupTable();
  if (this->Mapper->GetColorMode() == VTK_COLOR_MODE_MAP_SCALARS &&
    lut->GetVectorMode() == vtkScalarsToColors::COMPONENT && lut->GetVectorComponent() == component)
  {
    return;
  }
  this->Mapper->SetColorModeToMapScalars();
  lut->SetVectorModeToComponent();
  lut->SetVectorComponent(component);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetLookupTable(vtkScalarsToColors* lut)
{
  if (this->GetLookupTable() == lut)
  {
    return;
  }
  this->Mapper->SetLookupTable(lut);
  this->ScalarBar->SetLookupTable(lut);
  this->Modified();
}

//------------------------------------------------------------------------------
vtkScalarsToColors* vtkSurfaceRepresentation::GetLookupTable()
{
  return this->Mapper->GetLookupTable();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetScalarRange(double min, double max)
{
  double* current = this->GetScalarRange();
  if (current[0] == min && current[1] == max)
  {
    return;
  }
  this->Mapper->SetScalarRange(min, max);
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkSurfaceRepresentation::GetScalarRange()
{
  return this->Mapper->GetScalarRange();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetInterpolateScalarsBeforeMapping(bool val)
{
  if (this->GetInterpolateScalarsBeforeMapping() == val)
  {
    return;
  }
  this->Mapper->SetInterpolateScalarsBeforeMapping(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetInterpolateScalarsBeforeMapping()
{
  return this->Mapper->GetInterpolateScalarsBeforeMapping() != 0;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetVisibility(bool val)
{
  if (this->GetVisibility() == val)
  {
    return;
  }
  this->Actor->SetVisibility(val);
  this->ScalarBar->SetVisibility(val && this->ScalarBarVisible);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetVisibility()
{
  return this->Actor->GetVisibility() != 0;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetPickable(bool val)
{
  if (this->GetPickable() == val)
  {
    return;
  }
  this->Actor->SetPickable(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetPickable()
{
  return this->Actor->GetPickable() != 0;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetPosition(double x, double y, double z)
{
  double* current = this->GetPosition();
  if (current[0] == x && current[1] == y && current[2] == z)
  {
    return;
  }
  this->Actor->SetPosition(x, y, z);
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkSurfaceRepresentation::GetPosition()
{
  return this->Actor->GetPosition();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetOrientation(double x, double y, double z)
{
  double* current = this->GetOrientation();
  if (current[0] == x && current[1] == y && current[2] == z)
  {
    return;
  }
  this->Actor->SetOrientation(x, y, z);
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkSurfaceRepresentation::GetOrientation()
{
  return this->Actor->GetOrientation();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetScale(double x, double y, double z)
{
  double* current = this->GetScale();
  if (current[0] == x && current[1] == y && current[2] == z)
  {
    return;
  }
  this->Actor->SetScale(x, y, z);
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkSurfaceRepresentation::GetScale()
{
  return this->Actor->GetScale();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetScalarBarVisibility(bool val)
{
  if (this->ScalarBarVisible == val)
  {
    return;
  }
  this->ScalarBarVisible = val;
  this->ScalarBar->SetVisibility(val && (this->Actor->GetVisibility() != 0));
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkSurfaceRepresentation::GetScalarBarVisibility()
{
  return this->ScalarBarVisible;
}

//------------------------------------------------------------------------------
vtkScalarBarActor* vtkSurfaceRepresentation::GetScalarBarActor()
{
  return this->ScalarBar;
}

//------------------------------------------------------------------------------
vtkActor* vtkSurfaceRepresentation::GetActor()
{
  return this->Actor;
}

//------------------------------------------------------------------------------
vtkSelection* vtkSurfaceRepresentation::ConvertSelection(
  vtkView* vtkNotUsed(view), vtkSelection* selection)
{
  if (!selection || selection->GetNumberOfNodes() == 0)
  {
    this->SelectionActor->SetVisibility(false);
    // Return an empty selection so the base class UpdateSelection()
    // clears this representation's AnnotationLink.
    return vtkSelection::New();
  }

  vtkAlgorithmOutput* inputPort = this->GetInternalOutputPort();
  if (!inputPort)
  {
    return nullptr;
  }

  // Filter selection nodes to only those relevant to this representation.
  // Hardware-selector results tag each node with PROP identifying the picked actor.
  // Frustum selections have no PROP and apply to all representations.
  //
  // For INDICES selections (hardware pick), the IDs refer to the rendered
  // polydata (geometry filter output), not the original dataset.  Map them
  // back using vtkOriginalCellIds / vtkOriginalPointIds arrays that the
  // geometry filter produces when PassThroughCellIds/PointIds is on.
  //
  // When the geometry filter output is a composite dataset (e.g. from a
  // vtkPartitionedDataSet input), each selection node carries a
  // COMPOSITE_INDEX identifying which leaf it belongs to.  We look up the
  // original-ID arrays from the matching leaf and preserve COMPOSITE_INDEX
  // on the mapped node so that vtkExtractSelection extracts from the
  // correct partition.

  // For non-composite output, get the single surface polydata.
  // For composite output, build parallel leaf maps so we can translate
  // COMPOSITE_INDEX values from the GF output tree to the input tree
  // (they differ because the GF may wrap the output in an extra level).
  vtkDataObject* gfOutput = this->GeometryFilter->GetOutputDataObject(0);
  vtkPolyData* singleSurface = vtkPolyData::SafeDownCast(gfOutput);
  vtkDataObjectTree* compositeOutput = vtkDataObjectTree::SafeDownCast(gfOutput);

  // Build parallel arrays: for each leaf index, store the GF-output leaf
  // polydata and the corresponding input-tree flat index.
  std::vector<vtkPolyData*> gfLeaves;
  std::vector<unsigned int> gfLeafFlatIndices;
  std::vector<unsigned int> inputLeafFlatIndices;

  if (compositeOutput)
  {
    auto* iter = compositeOutput->NewTreeIterator();
    iter->VisitOnlyLeavesOn();
    iter->InitTraversal();
    while (!iter->IsDoneWithTraversal())
    {
      gfLeaves.push_back(vtkPolyData::SafeDownCast(iter->GetCurrentDataObject()));
      gfLeafFlatIndices.push_back(iter->GetCurrentFlatIndex());
      iter->GoToNextItem();
    }
    iter->Delete();

    // Walk the input tree to get corresponding flat indices.
    vtkDataObject* inputObj = inputPort->GetProducer()->GetOutputDataObject(0);
    vtkDataObjectTree* inputTree = vtkDataObjectTree::SafeDownCast(inputObj);
    if (inputTree)
    {
      auto* iiter = inputTree->NewTreeIterator();
      iiter->VisitOnlyLeavesOn();
      iiter->InitTraversal();
      while (!iiter->IsDoneWithTraversal())
      {
        inputLeafFlatIndices.push_back(iiter->GetCurrentFlatIndex());
        iiter->GoToNextItem();
      }
      iiter->Delete();
    }
  }

  vtkSelection* filtered = vtkSelection::New();
  for (unsigned int i = 0; i < selection->GetNumberOfNodes(); ++i)
  {
    vtkSelectionNode* node = selection->GetNode(i);

    // Frustum nodes apply to all representations — pass through as-is.
    if (node->GetContentType() == vtkSelectionNode::FRUSTUM)
    {
      filtered->AddNode(node);
      continue;
    }

    // Filter by PROP to match only this representation's actor.
    vtkInformation* nodeProps = node->GetProperties();
    if (nodeProps->Has(vtkSelectionNode::PROP()))
    {
      vtkObjectBase* obj = nodeProps->Get(vtkSelectionNode::PROP());
      if (obj != static_cast<vtkObjectBase*>(this->Actor.Get()))
      {
        continue;
      }
    }

    // For INDICES nodes, map surface IDs back to original dataset IDs.
    if (node->GetContentType() == vtkSelectionNode::INDICES)
    {
      vtkIdTypeArray* selList = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
      if (!selList || selList->GetNumberOfTuples() == 0)
      {
        continue;
      }

      bool isPoint = (node->GetFieldType() == vtkSelectionNode::POINT);

      // Find the correct surface leaf for this selection node.
      vtkPolyData* leafSurface = singleSurface;
      int inputFlatIndex = -1;

      if (!leafSurface && !gfLeaves.empty() && nodeProps->Has(vtkSelectionNode::COMPOSITE_INDEX()))
      {
        int gfFlatIndex = nodeProps->Get(vtkSelectionNode::COMPOSITE_INDEX());
        // Find which leaf in the GF output matches this flat index.
        for (size_t li = 0; li < gfLeafFlatIndices.size(); ++li)
        {
          if (static_cast<int>(gfLeafFlatIndices[li]) == gfFlatIndex)
          {
            leafSurface = gfLeaves[li];
            // Map to the corresponding input tree flat index.
            if (li < inputLeafFlatIndices.size())
            {
              inputFlatIndex = static_cast<int>(inputLeafFlatIndices[li]);
            }
            break;
          }
        }
      }

      vtkIdTypeArray* origIds = nullptr;
      if (leafSurface)
      {
        origIds = isPoint ? vtkIdTypeArray::SafeDownCast(
                              leafSurface->GetPointData()->GetArray("vtkOriginalPointIds"))
                          : vtkIdTypeArray::SafeDownCast(
                              leafSurface->GetCellData()->GetArray("vtkOriginalCellIds"));
      }

      if (origIds)
      {
        // Map surface IDs to original dataset IDs.
        vtkNew<vtkIdTypeArray> mappedIds;
        mappedIds->SetNumberOfComponents(1);
        for (vtkIdType j = 0; j < selList->GetNumberOfTuples(); ++j)
        {
          vtkIdType surfaceId = selList->GetValue(j);
          if (surfaceId >= 0 && surfaceId < origIds->GetNumberOfTuples())
          {
            mappedIds->InsertNextValue(origIds->GetValue(surfaceId));
          }
        }
        if (mappedIds->GetNumberOfTuples() > 0)
        {
          vtkNew<vtkSelectionNode> mappedNode;
          mappedNode->SetContentType(vtkSelectionNode::INDICES);
          mappedNode->SetFieldType(node->GetFieldType());
          mappedNode->SetSelectionList(mappedIds);
          // Use the INPUT tree's flat index (not the GF output's).
          if (inputFlatIndex >= 0)
          {
            mappedNode->GetProperties()->Set(vtkSelectionNode::COMPOSITE_INDEX(), inputFlatIndex);
          }
          filtered->AddNode(mappedNode);
        }
      }
      else
      {
        // No original ID arrays available — pass through as-is.
        filtered->AddNode(node);
      }
    }
    else
    {
      // Other content types — pass through.
      filtered->AddNode(node);
    }
  }

  if (filtered->GetNumberOfNodes() == 0)
  {
    this->SelectionActor->SetVisibility(false);
    // Return the empty filtered selection so the base class UpdateSelection()
    // clears this representation's AnnotationLink.
    return filtered;
  }

  // Detect if this is a point selection so we can adjust the visual style.
  bool isPointSelection = false;
  for (unsigned int i = 0; i < filtered->GetNumberOfNodes(); ++i)
  {
    vtkSelectionNode* fnode = filtered->GetNode(i);
    if (fnode->GetFieldType() == vtkSelectionNode::POINT)
    {
      isPointSelection = true;
      break;
    }
  }

  // For point selections, render as points; for cell selections, as wireframe.
  if (isPointSelection)
  {
    this->SelectionActor->GetProperty()->SetRepresentationToPoints();
  }
  else
  {
    this->SelectionActor->GetProperty()->SetRepresentationToWireframe();
  }

  this->SelectionExtractor->SetInputConnection(0, inputPort);
  this->SelectionExtractor->SetInputData(1, filtered);
  this->SelectionExtractor->Update();

  // Check if extraction produced any geometry.  For composite outputs,
  // the extraction result is also composite — check total cell count.
  vtkDataObject* extOutput = this->SelectionExtractor->GetOutputDataObject(0);
  vtkDataSet* extractedDS = vtkDataSet::SafeDownCast(extOutput);
  vtkCompositeDataSet* extractedComposite = vtkCompositeDataSet::SafeDownCast(extOutput);
  vtkIdType totalCells = 0;
  vtkIdType totalPoints = 0;
  if (extractedDS)
  {
    totalCells = extractedDS->GetNumberOfCells();
    totalPoints = extractedDS->GetNumberOfPoints();
  }
  else if (extractedComposite)
  {
    auto* iter = extractedComposite->NewIterator();
    iter->InitTraversal();
    while (!iter->IsDoneWithTraversal())
    {
      vtkDataSet* leaf = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (leaf)
      {
        totalCells += leaf->GetNumberOfCells();
        totalPoints += leaf->GetNumberOfPoints();
      }
      iter->GoToNextItem();
    }
    iter->Delete();
  }

  if (totalCells == 0 && totalPoints == 0)
  {
    this->SelectionActor->SetVisibility(false);
    return filtered;
  }

  this->SelectionGeometryFilter->SetInputConnection(this->SelectionExtractor->GetOutputPort());
  this->SelectionGeometryFilter->Update();

  this->SelectionMapper->SetInputDataObject(this->SelectionGeometryFilter->GetOutputDataObject(0));
  this->SelectionActor->SetVisibility(true);

  // Return the filtered selection with mapped IDs — base class will store
  // it in the AnnotationLink and delete it (since filtered != selection).
  return filtered;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetSelectionColor(double r, double g, double b)
{
  double* current = this->GetSelectionColor();
  if (current[0] == r && current[1] == g && current[2] == b)
  {
    return;
  }
  this->SelectionActor->GetProperty()->SetColor(r, g, b);
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkSurfaceRepresentation::GetSelectionColor()
{
  return this->SelectionActor->GetProperty()->GetColor();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetSelectionOpacity(double val)
{
  if (this->GetSelectionOpacity() == val)
  {
    return;
  }
  this->SelectionActor->GetProperty()->SetOpacity(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkSurfaceRepresentation::GetSelectionOpacity()
{
  return this->SelectionActor->GetProperty()->GetOpacity();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetSelectionLineWidth(double val)
{
  if (this->GetSelectionLineWidth() == val)
  {
    return;
  }
  this->SelectionActor->GetProperty()->SetLineWidth(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkSurfaceRepresentation::GetSelectionLineWidth()
{
  return this->SelectionActor->GetProperty()->GetLineWidth();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetSelectionPointSize(double val)
{
  if (this->GetSelectionPointSize() == val)
  {
    return;
  }
  this->SelectionActor->GetProperty()->SetPointSize(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkSurfaceRepresentation::GetSelectionPointSize()
{
  return this->SelectionActor->GetProperty()->GetPointSize();
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetSelectionRepresentation(int type)
{
  int representation;
  bool edgeVisibility;
  switch (type)
  {
    case POINTS:
      representation = VTK_POINTS;
      edgeVisibility = false;
      break;
    case WIREFRAME:
      representation = VTK_WIREFRAME;
      edgeVisibility = false;
      break;
    case SURFACE:
      representation = VTK_SURFACE;
      edgeVisibility = false;
      break;
    case SURFACE_WITH_EDGES:
      representation = VTK_SURFACE;
      edgeVisibility = true;
      break;
    default:
      return;
  }
  vtkProperty* prop = this->SelectionActor->GetProperty();
  if (prop->GetRepresentation() == representation &&
    (prop->GetEdgeVisibility() != 0) == edgeVisibility)
  {
    return;
  }
  prop->SetRepresentation(representation);
  prop->SetEdgeVisibility(edgeVisibility);
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkSurfaceRepresentation::GetSelectionRepresentation()
{
  return this->SelectionActor->GetProperty()->GetRepresentation();
}

//------------------------------------------------------------------------------
vtkActor* vtkSurfaceRepresentation::GetSelectionActor()
{
  return this->SelectionActor;
}

//------------------------------------------------------------------------------
void vtkSurfaceRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RepresentationValue: " << this->RepresentationValue << "\n";
  os << indent << "ScalarBarVisible: " << this->ScalarBarVisible << "\n";
  os << indent << "UseOutline: " << this->GeometryFilter->GetUseOutline() << "\n";
  os << indent << "GenerateFeatureEdges: " << this->GeometryFilter->GetGenerateFeatureEdges()
     << "\n";
  os << indent << "GeneratePointNormals: " << this->GeometryFilter->GetGeneratePointNormals()
     << "\n";
  os << indent << "GenerateCellNormals: " << this->GeometryFilter->GetGenerateCellNormals() << "\n";
  os << indent << "FeatureAngle: " << this->GeometryFilter->GetFeatureAngle() << "\n";
  os << indent << "Triangulate: " << this->GeometryFilter->GetTriangulate() << "\n";
  os << indent
     << "NonlinearSubdivisionLevel: " << this->GeometryFilter->GetNonlinearSubdivisionLevel()
     << "\n";
  os << indent << "MatchBoundariesIgnoringCellOrder: "
     << this->GeometryFilter->GetMatchBoundariesIgnoringCellOrder() << "\n";
  os << indent << "PassThroughCellIds: " << this->GeometryFilter->GetPassThroughCellIds() << "\n";
  os << indent << "PassThroughPointIds: " << this->GeometryFilter->GetPassThroughPointIds() << "\n";
  os << indent
     << "BlockColorsDistinctValues: " << this->GeometryFilter->GetBlockColorsDistinctValues()
     << "\n";
  os << indent << "HideInternalAMRFaces: " << this->GeometryFilter->GetHideInternalAMRFaces()
     << "\n";
  os << indent << "UseNonOverlappingAMRMetaDataForOutlines: "
     << this->GeometryFilter->GetUseNonOverlappingAMRMetaDataForOutlines() << "\n";
  os << indent << "GenerateProcessIds: " << this->GeometryFilter->GetGenerateProcessIds() << "\n";
  os << indent << "Actor: " << this->Actor << "\n";
  os << indent << "Mapper: " << this->Mapper << "\n";
  os << indent << "GeometryFilter: " << this->GeometryFilter << "\n";
  os << indent << "SelectionActor: " << this->SelectionActor << "\n";
}

VTK_ABI_NAMESPACE_END
