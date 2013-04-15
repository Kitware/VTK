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

#include "vtkCommand.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"

#include <vector>

vtkStandardNewMacro(vtkDiscretizableColorTransferFunction);
//-----------------------------------------------------------------------------
vtkDiscretizableColorTransferFunction::vtkDiscretizableColorTransferFunction()
{
  this->LookupTable = vtkLookupTable::New();

  this->Discretize = 0;
  this->NumberOfValues = 256;

  this->Data = 0;
  this->UseLogScale = 0;

  this->ScalarOpacityFunction = 0;
  this->ScalarOpacityFunctionObserverId = 0;
  this->EnableOpacityMapping = false;
}

//-----------------------------------------------------------------------------
vtkDiscretizableColorTransferFunction::~vtkDiscretizableColorTransferFunction()
{
  // this removes any observer we may have setup for the
  // ScalarOpacityFunction.
  this->SetScalarOpacityFunction(NULL);
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
int vtkDiscretizableColorTransferFunction::IsOpaque()
{
  return !this->EnableOpacityMapping;
}

//-----------------------------------------------------------------------------
void vtkDiscretizableColorTransferFunction::Build()
{
  this->Superclass::Build();

  this->LookupTable->SetVectorMode(this->VectorMode);
  this->LookupTable->SetVectorComponent(this->VectorComponent);
  this->LookupTable->SetIndexedLookup(this->IndexedLookup);
  this->LookupTable->SetAnnotations( this->AnnotatedValues, this->Annotations );

  if ( this->IndexedLookup )
    {
    int nv = this->GetSize();
    this->LookupTable->SetNumberOfTableValues( nv );
    double nodeVal[6];
    for ( int i = 0; i < nv; ++ i )
      {
      this->GetNodeValue( i, nodeVal );
      nodeVal[4] = 1.;
      this->LookupTable->SetTableValue( i, &nodeVal[1] );
      }
    return;
    }

  if (this->Discretize && (this->GetMTime() > this->BuildTime ||
      (this->ScalarOpacityFunction.GetPointer() &&
       this->ScalarOpacityFunction->GetMTime() > this->BuildTime)))
    {
    // Do not omit the LookupTable->SetNumberOfTableValues call:
    // WritePointer does not update the NumberOfColors ivar.
    this->LookupTable->SetNumberOfTableValues(this->NumberOfValues);
    unsigned char* lut_ptr = this->LookupTable->WritePointer(0,
      this->NumberOfValues * 3);
    double* table = new double[this->NumberOfValues * 3];
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
void vtkDiscretizableColorTransferFunction::SetNanColor(
                                                   double r, double g, double b)
{
  this->LookupTable->SetNanColor(r, g, b, 1.0);
  this->Superclass::SetNanColor(r, g, b);
}

//-----------------------------------------------------------------------------
unsigned char* vtkDiscretizableColorTransferFunction::MapValue(double v)
{
  this->Build();
  if ( this->IndexedLookup )
    {
    vtkIdType idx = this->GetAnnotatedValueIndex( v );
    if ( idx < 0 || this->GetSize() == 0 )
      {
      return this->Superclass::MapValue( vtkMath::Nan() );
      }
    double nodeValue[6];
    this->GetNodeValue( idx % this->GetSize(), nodeValue );
    this->UnsignedCharRGBAValue[0] =
      static_cast<unsigned char>(255.0*nodeValue[1] + 0.5);
    this->UnsignedCharRGBAValue[1] =
      static_cast<unsigned char>(255.0*nodeValue[2] + 0.5);
    this->UnsignedCharRGBAValue[2] =
      static_cast<unsigned char>(255.0*nodeValue[3] + 0.5);
    this->UnsignedCharRGBAValue[3] = 255;
    return this->UnsignedCharRGBAValue;
    }

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
double vtkDiscretizableColorTransferFunction::GetOpacity(double v)
{
  if (
    this->IndexedLookup ||
    !this->EnableOpacityMapping ||
    !this->ScalarOpacityFunction)
    {
    return this->Superclass::GetOpacity(v);
    }
  return this->ScalarOpacityFunction->GetValue(v);
}

//-----------------------------------------------------------------------------
vtkUnsignedCharArray* vtkDiscretizableColorTransferFunction::MapScalars(vtkDataArray *scalars,
  int colorMode, int component)
{
  if ( this->IndexedLookup )
    {
    return this->Superclass::MapScalars( scalars, colorMode, component );
    }

  this->Build();

  bool scalars_are_mapped = !(colorMode == VTK_COLOR_MODE_DEFAULT) &&
                             vtkUnsignedCharArray::SafeDownCast(scalars);

  vtkUnsignedCharArray *colors = this->Discretize ?
    this->LookupTable->MapScalars(scalars, colorMode, component):
    this->Superclass::MapScalars(scalars, colorMode, component);

  // calculate alpha values
  if(colors &&
     colors->GetNumberOfComponents() == 4 &&
     !scalars_are_mapped && !this->IndexedLookup &&
     this->EnableOpacityMapping &&
     this->ScalarOpacityFunction.GetPointer())
    {
    for(vtkIdType i = 0; i < scalars->GetNumberOfTuples(); i++)
      {
      double value = scalars->GetTuple1(i);
      double alpha = this->ScalarOpacityFunction->GetValue(value);
      colors->SetValue(4*i+3, static_cast<unsigned char>(alpha * 255.0 + 0.5));
      }
    }

  return colors;
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

//----------------------------------------------------------------------------
vtkIdType vtkDiscretizableColorTransferFunction::GetNumberOfAvailableColors()
{
  if(this->Discretize == false)
    {
    return 16777216; // 2^24
    }
  return this->NumberOfValues;
}

//----------------------------------------------------------------------------
void vtkDiscretizableColorTransferFunction::SetScalarOpacityFunction(vtkPiecewiseFunction *function)
{
  if(this->ScalarOpacityFunction != function)
    {
    if (this->ScalarOpacityFunction &&
      this->ScalarOpacityFunctionObserverId > 0)
      {
      this->ScalarOpacityFunction->RemoveObserver(this->ScalarOpacityFunctionObserverId);
      this->ScalarOpacityFunctionObserverId = 0;
      }
    this->ScalarOpacityFunction = function;
    if (function)
      {
      this->ScalarOpacityFunctionObserverId =
        function->AddObserver(vtkCommand::ModifiedEvent,
          this,
          &vtkDiscretizableColorTransferFunction::ScalarOpacityFunctionModified);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkPiecewiseFunction* vtkDiscretizableColorTransferFunction::GetScalarOpacityFunction() const
{
  return this->ScalarOpacityFunction;
}

//-----------------------------------------------------------------------------
void vtkDiscretizableColorTransferFunction::ScalarOpacityFunctionModified()
{
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkDiscretizableColorTransferFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Discretize: " << this->Discretize << endl;
  os << indent << "NumberOfValues: " << this->NumberOfValues << endl;
  os << indent << "UseLogScale: " << this->UseLogScale << endl;
  os << indent << "EnableOpacityMapping: " << this->EnableOpacityMapping << endl;
  os << indent << "ScalarOpacityFunction: " << this->ScalarOpacityFunction << endl;
}
