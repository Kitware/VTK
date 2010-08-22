/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeTransferFunctionItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkImageData.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkCompositeTransferFunctionItem.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints2D.h"

#include <cassert>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkCompositeTransferFunctionItem);

//-----------------------------------------------------------------------------
vtkCompositeTransferFunctionItem::vtkCompositeTransferFunctionItem()
{
  this->OpacityFunction = 0;
  this->MaskAboveCurve = false;
}

//-----------------------------------------------------------------------------
vtkCompositeTransferFunctionItem::~vtkCompositeTransferFunctionItem()
{
  if (this->OpacityFunction)
    {
    this->OpacityFunction->Delete();
    this->OpacityFunction = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkCompositeTransferFunctionItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CompositeTransferFunction: ";
  if (this->OpacityFunction)
    {
    os << endl;
    this->OpacityFunction->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}

//-----------------------------------------------------------------------------
void vtkCompositeTransferFunctionItem::SetOpacityFunction(vtkPiecewiseFunction* opacity)
{
  vtkSetObjectBodyMacro(OpacityFunction, vtkPiecewiseFunction, opacity);
  if (opacity)
    {
    opacity->AddObserver(vtkCommand::ModifiedEvent, this->Callback);
    }
}

//-----------------------------------------------------------------------------
void vtkCompositeTransferFunctionItem::SetMaskAboveCurve(bool mask)
{
  if (mask == this->MaskAboveCurve)
    {
    return;
    }
  if (mask == false)
    {
    this->Shape->SetNumberOfPoints(4);
    this->Shape->SetPoint(0, 0.f, 0.f);
    this->Shape->SetPoint(1, 1.f, 0.f);
    this->Shape->SetPoint(2, 1.f, 1.f);
    this->Shape->SetPoint(3, 0.f, 1.f);
    }
  this->MaskAboveCurve = mask;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkCompositeTransferFunctionItem::ComputeTexture()
{
  this->Superclass::ComputeTexture();
  double bounds[4];
  this->GetBounds(bounds);
  if (bounds[0] == bounds[1])
    {
    vtkWarningMacro(<< "The piecewise function seems empty");
    return;
    }
  const int dimension = this->Texture->GetExtent()[1] + 1;
  double* values = new double[dimension];
  this->OpacityFunction->GetTable(bounds[0], bounds[1], dimension, values);
  unsigned char* ptr =
    reinterpret_cast<unsigned char*>(this->Texture->GetScalarPointer(0,0,0));
  // TBD: maybe the shape should be defined somewhere else...
  if (MaskAboveCurve)
    {
    this->Shape->SetNumberOfPoints(dimension+2);
    this->Shape->SetPoint(0, 0.f, 0.f);
    this->Shape->SetPoint(dimension + 1, 1.f, 0.f);
    for (int i = 0; i < dimension; ++i)
      {
      ptr[3] = static_cast<unsigned char>(values[i] * this->Opacity * 255);
      assert(values[i] <= 1. && values[i] >= 0.);
      this->Shape->SetPoint(i+1,
                            static_cast<float>(i) * 1.f / (dimension - 1),
                            values[i] * 1.f);
      ptr+=4;
      }
    }
  else
    {
    for (int i = 0; i < dimension; ++i)
      {
      ptr[3] = static_cast<unsigned char>(values[i] * this->Opacity * 255);
      assert(values[i] <= 1. && values[i] >= 0.);
      ptr+=4;
      }
    }
  delete [] values;
}

//-----------------------------------------------------------------------------
void vtkCompositeTransferFunctionItem::ScalarsToColorsModified(vtkObject* object,
                                                     unsigned long eid,
                                                     void* calldata)
{
  if (object != this->ColorTransferFunction &&
      object != this->OpacityFunction)
    {
    vtkErrorMacro("The callback sender object is not the color transfer "
                  "function nor the opacity function");
    return;
    }
  //Update shape based on the potentially new range.
  double* range = this->ColorTransferFunction->GetRange();
  double* opacityRange = this->ColorTransferFunction->GetRange();
  double bounds[4];
  this->GetBounds(bounds);
  double newRange[2];
  newRange[0] = range[0] < opacityRange[0] ? range[0] : opacityRange[0];
  newRange[1] = range[1] < opacityRange[1] ? range[1] : opacityRange[1];
  if (bounds[0] != newRange[0] || bounds[1] != newRange[1])
    {
    this->Shape->SetNumberOfPoints(4);
    this->Shape->SetPoint(0, newRange[0], 0.);
    this->Shape->SetPoint(1, newRange[0], 1.);
    this->Shape->SetPoint(2, newRange[1], 1.);
    this->Shape->SetPoint(3, newRange[1], 0.);
    }
  // Internally calls modified to ask for a refresh of the item
  this->vtkScalarsToColorsItem::ScalarsToColorsModified(object, eid, calldata);
}
