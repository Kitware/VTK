/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPiecewiseFunctionItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBrush.h"
#include "vtkCallbackCommand.h"
#include "vtkContext2D.h"
#include "vtkImageData.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPiecewiseFunctionItem.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPointData.h"
#include "vtkPoints2D.h"

#include <cassert>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPiecewiseFunctionItem);

//-----------------------------------------------------------------------------
vtkPiecewiseFunctionItem::vtkPiecewiseFunctionItem()
{
  this->PiecewiseFunction = 0;
  this->Color[0] = 255; this->Color[1] = 255; this->Color[2] = 255;
  this->MaskAboveCurve = false;
}

//-----------------------------------------------------------------------------
vtkPiecewiseFunctionItem::~vtkPiecewiseFunctionItem()
{
  if (this->PiecewiseFunction)
    {
    this->PiecewiseFunction->Delete();
    this->PiecewiseFunction = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkPiecewiseFunctionItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PiecewiseFunction: ";
  if (this->PiecewiseFunction)
    {
    os << endl;
    this->PiecewiseFunction->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}

//-----------------------------------------------------------------------------
void vtkPiecewiseFunctionItem::SetPiecewiseFunction(vtkPiecewiseFunction* t)
{
  vtkSetObjectBodyMacro(PiecewiseFunction, vtkPiecewiseFunction, t);
  if (t)
    {
    t->AddObserver(vtkCommand::ModifiedEvent, this->Callback);
    }
}

//-----------------------------------------------------------------------------
void vtkPiecewiseFunctionItem::SetMaskAboveCurve(bool mask)
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
void vtkPiecewiseFunctionItem::ComputeTexture()
{

  if (this->Texture == 0)
    {
    this->Texture = vtkImageData::New();
    }

  double bounds[4];
  this->GetBounds(bounds);
   if (bounds[0] == bounds[1])
    {
    vtkWarningMacro(<< "The piecewise function seems empty");
    return;
    }

  const int dimension = 256;
  double* values = new double[dimension];
  // should depends on the true size on screen
  this->Texture->SetExtent(0, dimension-1,
                           0, 0,
                           0, 0);
  this->Texture->SetNumberOfScalarComponents(4);
  this->Texture->SetScalarTypeToUnsignedChar();
  this->Texture->AllocateScalars();

  this->PiecewiseFunction->GetTable(bounds[0], bounds[1], dimension,  values);
  unsigned char* ptr =
    reinterpret_cast<unsigned char*>(this->Texture->GetScalarPointer(0,0,0));
  if (MaskAboveCurve)
    {
    this->Shape->SetNumberOfPoints(dimension + 2 );
    this->Shape->SetPoint(0, 0.f, 0.f);
    this->Shape->SetPoint(dimension + 1, 1.f, 0.f);
    for (int i = 0; i < dimension; ++i)
      {
      ptr[0] = this->Color[0];
      ptr[1] = this->Color[1];
      ptr[2] = this->Color[2];
      ptr[3] = static_cast<unsigned char>(values[i] * this->Opacity * 255 + 0.5);
      assert(values[i] <= 1. && values[i] >= 0.);
      this->Shape->SetPoint(i + 1, static_cast<float>(i) * 1.f / (dimension-1),
                            values[i] * 1.f);
      ptr+=4;
      }
    }
  else
    {
    for (int i = 0; i < dimension; ++i)
      {
      ptr[0] = this->Color[0];
      ptr[1] = this->Color[1];
      ptr[2] = this->Color[2];
      ptr[3] = static_cast<unsigned char>(values[i] * this->Opacity * 255 + 0.5);
      assert(values[i] <= 1. && values[i] >= 0.);
      ptr+=4;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPiecewiseFunctionItem::ScalarsToColorsModified(vtkObject* object,
                                                     unsigned long eid,
                                                     void* calldata)
{
  if (object != this->PiecewiseFunction)
    {
    vtkErrorMacro("The callback sender object is not the lookup table object ");
    return;
    }
  //Update shape based on the potentially new range.
  double* range = this->PiecewiseFunction->GetRange();
  double bounds[4];
  this->GetBounds(bounds);
  if (bounds[0] != range[0] || bounds[1] != range[1])
    {
    this->Shape->SetNumberOfPoints(4);
    this->Shape->SetPoint(0, range[0], 0.);
    this->Shape->SetPoint(1, range[0], 1.);
    this->Shape->SetPoint(2, range[1], 1.);
    this->Shape->SetPoint(3, range[1], 0.);
    }
  // Internally calls modified to ask for a refresh of the item
  this->Superclass::ScalarsToColorsModified(object, eid, calldata);
}
