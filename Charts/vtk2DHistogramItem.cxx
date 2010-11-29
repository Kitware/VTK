/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk2DHistogramItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtk2DHistogramItem.h"
#include "vtkImageData.h"
#include "vtkScalarsToColors.h"
#include "vtkContext2D.h"

#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtk2DHistogramItem);

//-----------------------------------------------------------------------------
vtk2DHistogramItem::vtk2DHistogramItem()
{
}

//-----------------------------------------------------------------------------
vtk2DHistogramItem::~vtk2DHistogramItem()
{
}

//-----------------------------------------------------------------------------
bool vtk2DHistogramItem::Paint(vtkContext2D *painter)
{
  this->GenerateHistogram();
  if (this->Output)
    {
    if (this->Input)
      {
      double bounds[4];
      int *extent = this->Input->GetExtent();
      bounds[0] = this->Input->GetOrigin()[0];
      bounds[1] = bounds[0] +
          (extent[1] - extent[0]) * this->Input->GetSpacing()[0];

      bounds[2] = this->Input->GetOrigin()[1];
      bounds[3] = bounds[2] +
          (extent[3] - extent[2]) * this->Input->GetSpacing()[1];
      this->Position = vtkRectf(bounds[0], bounds[2],
                                bounds[1] - bounds[0], bounds[3] - bounds[2]);
      }
    painter->DrawImage(this->Position, this->Output);
    }
  return true;
}

//-----------------------------------------------------------------------------
void vtk2DHistogramItem::SetInput(vtkImageData *data, vtkIdType z)
{
  this->Input = data;
}

//-----------------------------------------------------------------------------
vtkImageData * vtk2DHistogramItem::GetInput()
{
  return this->Input;
}

//-----------------------------------------------------------------------------
void vtk2DHistogramItem::SetTransferFunction(vtkScalarsToColors *function)
{
  this->TransferFunction = function;
}

//-----------------------------------------------------------------------------
vtkScalarsToColors * vtk2DHistogramItem::GetTransferFunction()
{
  return this->TransferFunction;
}

void vtk2DHistogramItem::GetBounds(double bounds[4])
{
  if (this->Input)
    {
    int *extent = this->Input->GetExtent();
    bounds[0] = this->Input->GetOrigin()[0];
    bounds[1] = bounds[0] +
        (extent[1] - extent[0]) * this->Input->GetSpacing()[0];

    bounds[2] = this->Input->GetOrigin()[1];
    bounds[3] = bounds[2] +
        (extent[3] - extent[2]) * this->Input->GetSpacing()[1];
    }
  else
    {
    bounds[0] = bounds[1] = bounds[2] = bounds[3] = 0.0;
    }
}

//-----------------------------------------------------------------------------
void vtk2DHistogramItem::SetPosition(const vtkRectf& pos)
{
  this->Position = pos;
}

//-----------------------------------------------------------------------------
vtkRectf vtk2DHistogramItem::GetPosition()
{
  return this->Position;
}

//-----------------------------------------------------------------------------
void vtk2DHistogramItem::GenerateHistogram()
{
  if (!this->Output)
    {
    this->Output = vtkSmartPointer<vtkImageData>::New();
    }
  this->Output->SetExtent(this->Input->GetExtent());
  this->Output->SetNumberOfScalarComponents(4);
  this->Output->SetScalarTypeToUnsignedChar();
  this->Output->AllocateScalars();

  int dimension = this->Input->GetDimensions()[0] * this->Input->GetDimensions()[1];
  double *input = reinterpret_cast<double *>(this->Input->GetScalarPointer());
  unsigned char *output =
    reinterpret_cast<unsigned char*>(this->Output->GetScalarPointer(0,0,0));

  if (this->TransferFunction)
    {
    this->TransferFunction->MapScalarsThroughTable2(input, output, VTK_DOUBLE,
                                                    dimension, 1, 4);
    }
}

//-----------------------------------------------------------------------------
void vtk2DHistogramItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
