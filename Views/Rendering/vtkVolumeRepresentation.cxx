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
#include "vtkRenderViewBase.h"
#include "vtkRenderer.h"
#include "vtkScalarBarActor.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkVolumeRepresentation);

//------------------------------------------------------------------------------
vtkVolumeRepresentation::vtkVolumeRepresentation()
{
  this->SetNumberOfInputPorts(1);

  this->ScalarBarVisible = false;
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

  this->ScalarBar->SetNumberOfLabels(5);
  this->ScalarBar->SetVisibility(this->ScalarBarVisible);
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
    this->ScalarBar->SetLookupTable(ctf);
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
bool vtkVolumeRepresentation::AddToView(vtkView* view)
{
  vtkRenderViewBase* rv = vtkRenderViewBase::SafeDownCast(view);
  if (!rv)
  {
    vtkErrorMacro("Can only add vtkVolumeRepresentation to a vtkRenderViewBase subclass.");
    return false;
  }
  rv->GetRenderer()->AddVolume(this->VolumeActor);
  // Always add scalar bar to renderer; visibility controls whether it draws.
  rv->GetRenderer()->AddViewProp(this->ScalarBar);
  return true;
}

//------------------------------------------------------------------------------
bool vtkVolumeRepresentation::RemoveFromView(vtkView* view)
{
  vtkRenderViewBase* rv = vtkRenderViewBase::SafeDownCast(view);
  if (!rv)
  {
    return false;
  }
  rv->GetRenderer()->RemoveVolume(this->VolumeActor);
  rv->GetRenderer()->RemoveViewProp(this->ScalarBar);
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
  mTime = std::max(mTime, this->ScalarBar->GetMTime());
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
  this->ScalarBar->SetLookupTable(ctf);
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
void vtkVolumeRepresentation::SetAmbient(double val)
{
  if (this->GetAmbient() == val)
  {
    return;
  }
  this->VolumeProperty->SetAmbient(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkVolumeRepresentation::GetAmbient()
{
  return this->VolumeProperty->GetAmbient();
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::SetDiffuse(double val)
{
  if (this->GetDiffuse() == val)
  {
    return;
  }
  this->VolumeProperty->SetDiffuse(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkVolumeRepresentation::GetDiffuse()
{
  return this->VolumeProperty->GetDiffuse();
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::SetSpecular(double val)
{
  if (this->GetSpecular() == val)
  {
    return;
  }
  this->VolumeProperty->SetSpecular(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkVolumeRepresentation::GetSpecular()
{
  return this->VolumeProperty->GetSpecular();
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::SetSpecularPower(double val)
{
  if (this->GetSpecularPower() == val)
  {
    return;
  }
  this->VolumeProperty->SetSpecularPower(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkVolumeRepresentation::GetSpecularPower()
{
  return this->VolumeProperty->GetSpecularPower();
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::SetInterpolationType(int val)
{
  if (this->GetInterpolationType() == val)
  {
    return;
  }
  this->VolumeProperty->SetInterpolationType(val);
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkVolumeRepresentation::GetInterpolationType()
{
  return this->VolumeProperty->GetInterpolationType();
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::SetBlendMode(int mode)
{
  if (this->GetBlendMode() == mode)
  {
    return;
  }
  this->VolumeMapper->SetBlendMode(mode);
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkVolumeRepresentation::GetBlendMode()
{
  return this->VolumeMapper->GetBlendMode();
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::SetRequestedRenderMode(int mode)
{
  if (this->GetRequestedRenderMode() == mode)
  {
    return;
  }
  this->VolumeMapper->SetRequestedRenderMode(mode);
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkVolumeRepresentation::GetRequestedRenderMode()
{
  return this->VolumeMapper->GetRequestedRenderMode();
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::SetVisibility(bool val)
{
  if (this->GetVisibility() == val)
  {
    return;
  }
  this->VolumeActor->SetVisibility(val);
  this->ScalarBar->SetVisibility(val && this->ScalarBarVisible);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkVolumeRepresentation::GetVisibility()
{
  return this->VolumeActor->GetVisibility() != 0;
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::SetPosition(double x, double y, double z)
{
  double* current = this->GetPosition();
  if (current[0] == x && current[1] == y && current[2] == z)
  {
    return;
  }
  this->VolumeActor->SetPosition(x, y, z);
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkVolumeRepresentation::GetPosition()
{
  return this->VolumeActor->GetPosition();
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::SetOrientation(double x, double y, double z)
{
  double* current = this->GetOrientation();
  if (current[0] == x && current[1] == y && current[2] == z)
  {
    return;
  }
  this->VolumeActor->SetOrientation(x, y, z);
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkVolumeRepresentation::GetOrientation()
{
  return this->VolumeActor->GetOrientation();
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::SetScale(double x, double y, double z)
{
  double* current = this->GetScale();
  if (current[0] == x && current[1] == y && current[2] == z)
  {
    return;
  }
  this->VolumeActor->SetScale(x, y, z);
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkVolumeRepresentation::GetScale()
{
  return this->VolumeActor->GetScale();
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::SetScalarBarVisibility(bool val)
{
  if (this->ScalarBarVisible == val)
  {
    return;
  }
  this->ScalarBarVisible = val;
  this->ScalarBar->SetVisibility(val && (this->VolumeActor->GetVisibility() != 0));
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkVolumeRepresentation::GetScalarBarVisibility()
{
  return this->ScalarBarVisible;
}

//------------------------------------------------------------------------------
vtkScalarBarActor* vtkVolumeRepresentation::GetScalarBarActor()
{
  return this->ScalarBar;
}

//------------------------------------------------------------------------------
vtkVolume* vtkVolumeRepresentation::GetVolume()
{
  return this->VolumeActor;
}

//------------------------------------------------------------------------------
void vtkVolumeRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ScalarBarVisible: " << this->ScalarBarVisible << "\n";
  os << indent << "VolumeMapper: " << this->VolumeMapper << "\n";
  os << indent << "VolumeActor: " << this->VolumeActor << "\n";
  os << indent << "VolumeProperty: " << this->VolumeProperty << "\n";
}

VTK_ABI_NAMESPACE_END
