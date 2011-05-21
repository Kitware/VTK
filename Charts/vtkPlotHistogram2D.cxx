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

#include "vtkPlotHistogram2D.h"
#include "vtkImageData.h"
#include "vtkScalarsToColors.h"
#include "vtkContext2D.h"

#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotHistogram2D);

//-----------------------------------------------------------------------------
vtkPlotHistogram2D::vtkPlotHistogram2D()
{
}

//-----------------------------------------------------------------------------
vtkPlotHistogram2D::~vtkPlotHistogram2D()
{
}

void vtkPlotHistogram2D::Update()
{
  this->GenerateHistogram();
}

//-----------------------------------------------------------------------------
bool vtkPlotHistogram2D::Paint(vtkContext2D *painter)
{
  if (this->Output)
    {
    if (this->Input)
      {
      double bounds[4];
      int *extent = this->Input->GetExtent();
      bounds[0] = this->Input->GetOrigin()[0];
      bounds[1] = bounds[0] +
          (extent[1] - extent[0] + 1) * this->Input->GetSpacing()[0];

      bounds[2] = this->Input->GetOrigin()[1];
      bounds[3] = bounds[2] +
          (extent[3] - extent[2] + 1) * this->Input->GetSpacing()[1];
      this->Position = vtkRectf(bounds[0], bounds[2],
                                bounds[1] - bounds[0], bounds[3] - bounds[2]);
      }
    painter->DrawImage(this->Position, this->Output);
    }
  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotHistogram2D::SetInput(vtkImageData *data, vtkIdType)
{
  // FIXME: Store the z too, for slices.
  this->Input = data;
}

//-----------------------------------------------------------------------------
vtkImageData * vtkPlotHistogram2D::GetInputImageData()
{
  return this->Input;
}

//-----------------------------------------------------------------------------
void vtkPlotHistogram2D::SetTransferFunction(vtkScalarsToColors *function)
{
  this->TransferFunction = function;
}

//-----------------------------------------------------------------------------
vtkScalarsToColors * vtkPlotHistogram2D::GetTransferFunction()
{
  return this->TransferFunction;
}

void vtkPlotHistogram2D::GetBounds(double bounds[4])
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
void vtkPlotHistogram2D::SetPosition(const vtkRectf& pos)
{
  this->Position = pos;
}

//-----------------------------------------------------------------------------
vtkRectf vtkPlotHistogram2D::GetPosition()
{
  return this->Position;
}

//-----------------------------------------------------------------------------
void vtkPlotHistogram2D::GenerateHistogram()
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
void vtkPlotHistogram2D::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
