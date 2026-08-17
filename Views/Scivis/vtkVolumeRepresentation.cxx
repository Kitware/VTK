// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkVolumeRepresentation.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkScivisView.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkVolumeRepresentation);

namespace
{

// vtkAbstractMapper::GetScalars() reports where it found the scalars as 0 for
// points, 1 for cells and 2 for field data.
int AssociationForCellFlag(int cellFlag)
{
  switch (cellFlag)
  {
    case 1:
      return vtkDataObject::FIELD_ASSOCIATION_CELLS;
    case 2:
      return vtkDataObject::FIELD_ASSOCIATION_NONE;
    default:
      return vtkDataObject::FIELD_ASSOCIATION_POINTS;
  }
}

}

//------------------------------------------------------------------------------
vtkVolumeRepresentation::vtkVolumeRepresentation()
{
  this->SetNumberOfInputPorts(1);

  this->DefaultTransferFunctionsCreated = false;
  this->UserSetColorTransferFunction = false;
  this->UserSetScalarOpacity = false;

  this->VolumeProperty->SetInterpolationTypeToLinear();
  this->VolumeProperty->ShadeOn();
  this->VolumeProperty->SetAmbient(0.2);
  this->VolumeProperty->SetDiffuse(0.7);
  this->VolumeProperty->SetSpecular(0.2);
  this->VolumeProperty->SetSpecularPower(10.0);
  this->VolumeActor->SetMapper(this->VolumeMapper);
  this->VolumeActor->SetProperty(this->VolumeProperty);

  // The clipping planes belong to the contract, which owns the collection; the
  // mapper follows it from here on.
  this->VolumeMapper->SetClippingPlanes(this->GetClippingPlanes());
}

//------------------------------------------------------------------------------
vtkVolumeRepresentation::~vtkVolumeRepresentation() = default;

//------------------------------------------------------------------------------
int vtkVolumeRepresentation::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkAlgorithmOutput* inputPort = this->GetInternalOutputPort();
  if (inputPort)
  {
    this->VolumeMapper->SetInputConnection(inputPort);

    if (!this->DefaultTransferFunctionsCreated)
    {
      this->CreateDefaultTransferFunctions();
      this->DefaultTransferFunctionsCreated = true;
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::CreateDefaultTransferFunctions()
{
  // Determine the scalar range from the input data, preferring point scalars
  // but falling back to cell scalars.
  double range[2] = { 0.0, 1.0 };
  vtkAlgorithmOutput* inputPort = this->GetInternalOutputPort();
  if (inputPort)
  {
    inputPort->GetProducer()->Update();
    vtkDataSet* ds = vtkDataSet::SafeDownCast(inputPort->GetProducer()->GetOutputDataObject(0));
    if (ds)
    {
      vtkDataArray* scalars = nullptr;
      if (ds->GetPointData() && ds->GetPointData()->GetScalars())
      {
        scalars = ds->GetPointData()->GetScalars();
      }
      else if (ds->GetCellData() && ds->GetCellData()->GetScalars())
      {
        scalars = ds->GetCellData()->GetScalars();
      }
      if (scalars)
      {
        scalars->GetRange(range);
      }
    }
  }

  double lo = range[0];
  double hi = range[1];
  if (lo == hi)
  {
    lo -= 0.5;
    hi += 0.5;
  }
  double span = hi - lo;

  // Color: cool-to-warm diverging colormap.
  if (!this->UserSetColorTransferFunction)
  {
    vtkNew<vtkColorTransferFunction> ctf;
    ctf->AddRGBPoint(lo, 0.23, 0.30, 0.75);
    ctf->AddRGBPoint(lo + 0.25 * span, 0.0, 0.82, 0.77);
    ctf->AddRGBPoint(lo + 0.50 * span, 0.87, 0.87, 0.87);
    ctf->AddRGBPoint(lo + 0.75 * span, 0.99, 0.57, 0.05);
    ctf->AddRGBPoint(hi, 0.71, 0.02, 0.15);
    this->VolumeProperty->SetColor(ctf);
  }

  // Opacity: S-curve ramp keeping low values mostly transparent.
  if (!this->UserSetScalarOpacity)
  {
    vtkNew<vtkPiecewiseFunction> pf;
    pf->AddPoint(lo, 0.0);
    pf->AddPoint(lo + 0.15 * span, 0.001);
    pf->AddPoint(lo + 0.30 * span, 0.05);
    pf->AddPoint(lo + 0.50 * span, 0.15);
    pf->AddPoint(lo + 0.70 * span, 0.4);
    pf->AddPoint(lo + 0.85 * span, 0.7);
    pf->AddPoint(hi, 0.9);
    this->VolumeProperty->SetScalarOpacity(pf);
  }
}

//------------------------------------------------------------------------------
bool vtkVolumeRepresentation::AddToView(vtkScivisView* view)
{
  if (!view)
  {
    return false;
  }
  view->GetRenderer()->AddVolume(this->VolumeActor);
  return true;
}

//------------------------------------------------------------------------------
bool vtkVolumeRepresentation::RemoveFromView(vtkScivisView* view)
{
  if (!view)
  {
    return false;
  }
  view->GetRenderer()->RemoveVolume(this->VolumeActor);
  return true;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkVolumeRepresentation::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  mTime = std::max(mTime, this->VolumeMapper->GetMTime());
  // vtkVolume::GetMTime() already accounts for the volume property, which in
  // turn accounts for the transfer functions.
  mTime = std::max(mTime, this->VolumeActor->GetMTime());
  return mTime;
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::SetColorTransferFunction(vtkColorTransferFunction* ctf)
{
  // Record the user's intent even when the function is unchanged, so a default
  // one is never generated over it.
  this->UserSetColorTransferFunction = true;
  if (this->GetColorTransferFunction() == ctf)
  {
    return;
  }
  this->VolumeProperty->SetColor(ctf);
  this->Modified();
}

//------------------------------------------------------------------------------
vtkColorTransferFunction* vtkVolumeRepresentation::GetColorTransferFunction()
{
  return this->VolumeProperty->GetRGBTransferFunction();
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::SetScalarOpacity(vtkPiecewiseFunction* pf)
{
  // Record the user's intent even when the function is unchanged, so a default
  // one is never generated over it.
  this->UserSetScalarOpacity = true;
  if (this->GetScalarOpacity() == pf)
  {
    return;
  }
  this->VolumeProperty->SetScalarOpacity(pf);
  this->Modified();
}

//------------------------------------------------------------------------------
vtkPiecewiseFunction* vtkVolumeRepresentation::GetScalarOpacity()
{
  return this->VolumeProperty->GetScalarOpacity();
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::ResetColorTransferFunction()
{
  // CreateDefaultTransferFunctions() rebuilds only the functions that are not
  // the user's, so the opacity is left alone whether or not it was set.
  this->UserSetColorTransferFunction = false;
  this->CreateDefaultTransferFunctions();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::ResetScalarOpacity()
{
  this->UserSetScalarOpacity = false;
  this->CreateDefaultTransferFunctions();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::ResetTransferFunctions()
{
  this->UserSetColorTransferFunction = false;
  this->UserSetScalarOpacity = false;
  this->CreateDefaultTransferFunctions();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::SetScalarOpacityUnitDistance(double distance)
{
  if (this->GetScalarOpacityUnitDistance() == distance)
  {
    return;
  }
  this->VolumeProperty->SetScalarOpacityUnitDistance(distance);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkVolumeRepresentation::GetScalarOpacityUnitDistance()
{
  return this->VolumeProperty->GetScalarOpacityUnitDistance();
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::SetShade(bool val)
{
  if (this->GetShade() == val)
  {
    return;
  }
  this->VolumeProperty->SetShade(val ? 1 : 0);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkVolumeRepresentation::GetShade()
{
  return this->VolumeProperty->GetShade() != 0;
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::SetVisibility(bool val)
{
  if (this->GetVisibility() == val)
  {
    return;
  }
  this->VolumeActor->SetVisibility(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkVolumeRepresentation::GetVisibility()
{
  return this->VolumeActor->GetVisibility() != 0;
}

//------------------------------------------------------------------------------
vtkVolume* vtkVolumeRepresentation::GetVolume()
{
  return this->VolumeActor;
}

//------------------------------------------------------------------------------
vtkDataSet* vtkVolumeRepresentation::GetInputDataSet()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }

  // Bring the input up to date rather than this representation: this is also
  // reached from RequestData(), where updating ourselves would be re-entrant.
  this->GetInputAlgorithm(0, 0)->Update();
  return vtkDataSet::SafeDownCast(this->GetInputDataObject(0, 0));
}

//------------------------------------------------------------------------------
vtkDataArray* vtkVolumeRepresentation::GetRenderedScalars(int& fieldAssoc)
{
  vtkDataSet* dataset = this->GetInputDataSet();
  if (!dataset)
  {
    return nullptr;
  }

  // Ask the same question the mapper asks, rather than reproducing its answer:
  // in the default mode this settles on the active point scalars, or the
  // active cell scalars when there are none.
  int cellFlag = 0;
  vtkDataArray* scalars = vtkAbstractMapper::GetScalars(dataset,
    this->VolumeMapper->GetScalarMode(), this->VolumeMapper->GetArrayAccessMode(),
    this->VolumeMapper->GetArrayId(), this->VolumeMapper->GetArrayName(), cellFlag);
  fieldAssoc = ::AssociationForCellFlag(cellFlag);
  return scalars;
}

//------------------------------------------------------------------------------
const char* vtkVolumeRepresentation::GetRenderedArrayName()
{
  int fieldAssoc = vtkDataObject::FIELD_ASSOCIATION_POINTS;
  vtkDataArray* scalars = this->GetRenderedScalars(fieldAssoc);
  return scalars ? scalars->GetName() : nullptr;
}

//------------------------------------------------------------------------------
int vtkVolumeRepresentation::GetRenderedFieldAssociation()
{
  int fieldAssoc = vtkDataObject::FIELD_ASSOCIATION_POINTS;
  this->GetRenderedScalars(fieldAssoc);
  return fieldAssoc;
}

//------------------------------------------------------------------------------
bool vtkVolumeRepresentation::IsColoringBy(const char* arrayName, int scalarMode)
{
  if (this->VolumeMapper->GetScalarMode() != scalarMode ||
    this->VolumeMapper->GetArrayAccessMode() != VTK_GET_ARRAY_BY_NAME)
  {
    return false;
  }
  const char* current = this->VolumeMapper->GetArrayName();
  if (!current || !arrayName)
  {
    return current == arrayName;
  }
  return strcmp(current, arrayName) == 0;
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::ColorByPointArray(const char* arrayName)
{
  if (this->IsColoringBy(arrayName, VTK_SCALAR_MODE_USE_POINT_FIELD_DATA))
  {
    return;
  }
  this->VolumeMapper->SetScalarModeToUsePointFieldData();
  this->VolumeMapper->SelectScalarArray(arrayName);
  // The generated functions span the range of the array being rendered, which
  // is now a different array.
  this->CreateDefaultTransferFunctions();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::ColorByCellArray(const char* arrayName)
{
  if (this->IsColoringBy(arrayName, VTK_SCALAR_MODE_USE_CELL_FIELD_DATA))
  {
    return;
  }
  this->VolumeMapper->SetScalarModeToUseCellFieldData();
  this->VolumeMapper->SelectScalarArray(arrayName);
  this->CreateDefaultTransferFunctions();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::ResetColorArray()
{
  if (this->VolumeMapper->GetScalarMode() == VTK_SCALAR_MODE_DEFAULT &&
    this->VolumeMapper->GetArrayAccessMode() == VTK_GET_ARRAY_BY_ID)
  {
    return;
  }
  this->VolumeMapper->SetScalarModeToDefault();
  this->VolumeMapper->SelectScalarArray(-1);
  this->CreateDefaultTransferFunctions();
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkVolumeRepresentation::GetDataRange(
  const char* arrayName, int fieldAssoc, double range[2], int component)
{
  if (!arrayName || arrayName[0] == '\0')
  {
    return false;
  }

  vtkDataSet* dataset = this->GetInputDataSet();
  if (!dataset)
  {
    return false;
  }

  vtkFieldData* attributes = nullptr;
  switch (fieldAssoc)
  {
    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
      attributes = dataset->GetPointData();
      break;
    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
      attributes = dataset->GetCellData();
      break;
    case vtkDataObject::FIELD_ASSOCIATION_NONE:
      attributes = dataset->GetFieldData();
      break;
    default:
      return false;
  }

  vtkDataArray* array = attributes->GetArray(arrayName);
  if (!array || component >= array->GetNumberOfComponents())
  {
    // vtkDataArray::GetRange() leaves the range untouched for a component the
    // array does not have, so asking would tell us nothing.
    return false;
  }

  array->GetRange(range, component);
  return true;
}

//------------------------------------------------------------------------------
bool vtkVolumeRepresentation::GetBounds(double bounds[6])
{
  if (!this->GetInputDataSet())
  {
    return false;
  }

  // The volume's bounds rather than the data's, so that a volume that has been
  // positioned or transformed reports where it is drawn -- which is what the
  // surface representation reports through its actor.
  const double* volumeBounds = this->VolumeActor->GetBounds();
  if (!volumeBounds || volumeBounds[0] > volumeBounds[1])
  {
    return false;
  }
  std::copy(volumeBounds, volumeBounds + 6, bounds);
  return true;
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::SetColorMap(vtkScalarsToColors* map)
{
  if (auto* ctf = vtkColorTransferFunction::SafeDownCast(map))
  {
    this->SetColorTransferFunction(ctf);
    return;
  }
  // Anything else cannot color a volume.  This is ignored rather than refused
  // because a view offers the same map to everything drawing an array, and a
  // volume turning one down is the expected answer rather than a mistake.
}

//------------------------------------------------------------------------------
vtkScalarsToColors* vtkVolumeRepresentation::GetColorMap()
{
  // Reporting the color transfer function is what gets a scalar bar labelled
  // with the colors actually on screen.
  return this->GetColorTransferFunction();
}

//------------------------------------------------------------------------------
vtkVolumeProperty* vtkVolumeRepresentation::GetVolumeProperty()
{
  return this->VolumeProperty;
}

//------------------------------------------------------------------------------
vtkSmartVolumeMapper* vtkVolumeRepresentation::GetVolumeMapper()
{
  return this->VolumeMapper;
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VolumeMapper: " << this->VolumeMapper << "\n";
  os << indent << "VolumeActor: " << this->VolumeActor << "\n";
  os << indent << "VolumeProperty: " << this->VolumeProperty << "\n";
}

VTK_ABI_NAMESPACE_END
