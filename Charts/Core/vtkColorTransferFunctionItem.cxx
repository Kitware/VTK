/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorTransferFunctionItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkColorTransferFunctionItem.h"

#include "vtkAxis.h"
#include "vtkBrush.h"
#include "vtkCallbackCommand.h"
#include "vtkColorTransferFunction.h"
#include "vtkContext2D.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPlotBar.h"
#include "vtkPointData.h"
#include "vtkPoints2D.h"

#include <cassert>
#include <cmath>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkColorTransferFunctionItem);

//-----------------------------------------------------------------------------
vtkColorTransferFunctionItem::vtkColorTransferFunctionItem()
{
  this->ColorTransferFunction = nullptr;
}

//-----------------------------------------------------------------------------
vtkColorTransferFunctionItem::~vtkColorTransferFunctionItem()
{
  if (this->ColorTransferFunction)
  {
    this->ColorTransferFunction->RemoveObserver(this->Callback);
    this->ColorTransferFunction->Delete();
    this->ColorTransferFunction = nullptr;
  }
}

//-----------------------------------------------------------------------------
void vtkColorTransferFunctionItem::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ColorTransferFunction: ";
  if (this->ColorTransferFunction)
  {
    os << endl;
    this->ColorTransferFunction->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
}

//-----------------------------------------------------------------------------
void vtkColorTransferFunctionItem::ComputeBounds(double* bounds)
{
  this->Superclass::ComputeBounds(bounds);
  if (this->ColorTransferFunction)
  {
    double unused;
    double* range = this->ColorTransferFunction->GetRange();
    this->TransformDataToScreen(range[0], 1, bounds[0], unused);
    this->TransformDataToScreen(range[1], 1, bounds[1], unused);
  }
}

//-----------------------------------------------------------------------------
void vtkColorTransferFunctionItem::SetColorTransferFunction(vtkColorTransferFunction* t)
{
  if (t == this->ColorTransferFunction)
  {
    return;
  }
  if (this->ColorTransferFunction)
  {
    this->ColorTransferFunction->RemoveObserver(this->Callback);
  }
  vtkSetObjectBodyMacro(ColorTransferFunction, vtkColorTransferFunction, t);
  if (t)
  {
    t->AddObserver(vtkCommand::ModifiedEvent, this->Callback);
  }
  this->ScalarsToColorsModified(t, vtkCommand::ModifiedEvent, nullptr);
}

//-----------------------------------------------------------------------------
void vtkColorTransferFunctionItem::ComputeTexture()
{
  double screenBounds[4];
  this->GetBounds(screenBounds);
  if (screenBounds[0] == screenBounds[1] || !this->ColorTransferFunction)
  {
    return;
  }
  if (this->Texture == nullptr)
  {
    this->Texture = vtkImageData::New();
  }

  double dataBounds[4];
  this->TransformScreenToData(screenBounds[0], screenBounds[2], dataBounds[0], dataBounds[2]);
  this->TransformScreenToData(screenBounds[1], screenBounds[3], dataBounds[1], dataBounds[3]);

  // Could depend of the screen resolution
  const int dimension = this->GetTextureWidth();
  double* values = new double[dimension];
  // Texture 1D
  this->Texture->SetExtent(0, dimension - 1, 0, 0, 0, 0);
  this->Texture->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
  for (int i = 0; i < dimension; ++i)
  {
    values[i] = dataBounds[0] + i * (dataBounds[1] - dataBounds[0]) / (dimension - 1);
  }
  unsigned char* ptr = reinterpret_cast<unsigned char*>(this->Texture->GetScalarPointer(0, 0, 0));
  this->ColorTransferFunction->MapScalarsThroughTable2(
    values, ptr, VTK_DOUBLE, dimension, VTK_LUMINANCE, VTK_RGBA);
  if (this->Opacity != 1.0)
  {
    for (int i = 0; i < dimension; ++i)
    {
      ptr[3] = static_cast<unsigned char>(this->Opacity * ptr[3]);
      ptr += 4;
    }
  }
  delete[] values;
}

//-----------------------------------------------------------------------------
bool vtkColorTransferFunctionItem::ConfigurePlotBar()
{
  bool ret = this->Superclass::ConfigurePlotBar();
  if (ret)
  {
    this->PlotBar->SetLookupTable(this->ColorTransferFunction);
    this->PlotBar->Update();
  }
  return ret;
}
