/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDiscretizableColorTransferFunction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDiscretizableColorTransferFunction.h"

#include "vtkObjectFactory.h"
#include "vtkLookupTable.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkDiscretizableColorTransferFunction);
//-----------------------------------------------------------------------------
vtkDiscretizableColorTransferFunction::vtkDiscretizableColorTransferFunction()
{
  this->LookupTable = vtkLookupTable::New();

  this->Discretize = 0;
  this->NumberOfValues = 256;

  this->Data = 0;
  this->UseLogScale = 0;
}

//-----------------------------------------------------------------------------
vtkDiscretizableColorTransferFunction::~vtkDiscretizableColorTransferFunction()
{
  this->LookupTable->Delete();
  delete [] this->Data;
}

//-----------------------------------------------------------------------------
struct vtkDiscretizableColorTransferFunctionNode
{
  double Value[6];
};

//-----------------------------------------------------------------------------
void vtkDiscretizableColorTransferFunction::SetUseLogScale(int useLogScale)
{
  if(this->UseLogScale != useLogScale)
    {
    this->UseLogScale = useLogScale;
    if(this->UseLogScale)
      {
      this->LookupTable->SetScaleToLog10();
      this->SetScaleToLog10();
      }
    else
      {
      this->LookupTable->SetScaleToLinear();
      this->SetScaleToLinear();
      }

    this->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkDiscretizableColorTransferFunction::SetNumberOfValues(vtkIdType number)
{
  this->NumberOfValues = number;
  this->LookupTable->SetNumberOfTableValues(number);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkDiscretizableColorTransferFunction::Build()
{
  this->Superclass::Build();

  this->LookupTable->SetVectorMode(this->VectorMode);
  this->LookupTable->SetVectorComponent(this->VectorComponent);

  if (this->Discretize && (this->GetMTime() > this->BuildTime ||
    this->GetMTime() > this->BuildTime))
    {
    unsigned char* lut_ptr = this->LookupTable->WritePointer(0,
      this->NumberOfValues);
    double* table = new double[this->NumberOfValues*3];
    double range[2];
    this->GetRange(range);
    bool logRangeValid = true;
    if(this->UseLogScale)
      {
      logRangeValid = range[0] > 0.0 || range[1] < 0.0;
      if(!logRangeValid && this->LookupTable->GetScale() == VTK_SCALE_LOG10)
        {
        this->LookupTable->SetScaleToLinear();
        }
      }

    this->LookupTable->SetRange(range);
    if(this->UseLogScale && logRangeValid &&
        this->LookupTable->GetScale() == VTK_SCALE_LINEAR)
      {
      this->LookupTable->SetScaleToLog10();
      }

    this->GetTable(range[0], range[1], this->NumberOfValues, table);
    // Now, convert double to unsigned chars and fill the LUT.
    for (int cc=0; cc < this->NumberOfValues; cc++)
      {
      lut_ptr[4*cc]   = (unsigned char)(255.0*table[3*cc] + 0.5);
      lut_ptr[4*cc+1] = (unsigned char)(255.0*table[3*cc+1] + 0.5);
      lut_ptr[4*cc+2] = (unsigned char)(255.0*table[3*cc+2] + 0.5);
      lut_ptr[4*cc+3] = 255;
      }
    delete [] table;

    this->BuildTime.Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkDiscretizableColorTransferFunction::SetAlpha(double alpha)
{
  this->LookupTable->SetAlpha(alpha);
  this->Superclass::SetAlpha(alpha);
}

//-----------------------------------------------------------------------------
unsigned char* vtkDiscretizableColorTransferFunction::MapValue(double v)
{
  this->Build();
  if (this->Discretize)
    {
    return this->LookupTable->MapValue(v);
    }

  return this->Superclass::MapValue(v);
}

//-----------------------------------------------------------------------------
void vtkDiscretizableColorTransferFunction::GetColor(double v, double rgb[3])
{
  this->Build();
  if (this->Discretize)
    {
    this->LookupTable->GetColor(v, rgb);
    return;
    }

  this->Superclass::GetColor(v, rgb);
}

//-----------------------------------------------------------------------------
vtkUnsignedCharArray* vtkDiscretizableColorTransferFunction::MapScalars(vtkDataArray *scalars, 
  int colorMode, int component)
{
  this->Build();
  if (this->Discretize)
    {
    return this->LookupTable->MapScalars(scalars, colorMode, component);
    }

  return this->Superclass::MapScalars(scalars, colorMode, component);
}

//-----------------------------------------------------------------------------
double* vtkDiscretizableColorTransferFunction::GetRGBPoints()
{
  delete [] this->Data;
  this->Data = 0;

  int num_points = this->GetSize();
  if (num_points > 0)
    {
    this->Data = new double[num_points*4];
    for (int cc=0; cc < num_points; cc++)
      {
      double values[6];
      this->GetNodeValue(cc, values);
      this->Data[4*cc] = values[0];
      this->Data[4*cc+1] = values[0];
      this->Data[4*cc+2] = values[1];
      this->Data[4*cc+3] = values[2];
      }
    }
  return this->Data;
}

//-----------------------------------------------------------------------------
void vtkDiscretizableColorTransferFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Discretize: " << this->Discretize << endl;
  os << indent << "NumberOfValues: " << this->NumberOfValues << endl;
  os << indent << "UseLogScale: " << this->UseLogScale << endl;
}
