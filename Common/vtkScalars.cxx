/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalars.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkScalars.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkScalars* vtkScalars::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkScalars");
  if(ret)
    {
    return (vtkScalars*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkScalars;
}


vtkScalars::vtkScalars() 
{
  this->Range[0] = this->Range[2] = this->Range[4] = this->Range[6] = 0.0;
  this->Range[1] = this->Range[3] = this->Range[5] = this->Range[7] = 1.0;

  this->CurrentAlpha = 1.0;
  this->CurrentLookupTable = NULL;
  this->CurrentColorFunction = &vtkScalars::MapThroughLookupTable;
  this->SetNumberOfComponents(1);
  this->ActiveComponent = 0;
}

vtkScalars *vtkScalars::New(int dataType, int numComp)
{
  vtkScalars *res = vtkScalars::New();
  res->SetDataType(dataType);
  res->SetNumberOfComponents(numComp);
  return res;
}

vtkScalars::~vtkScalars()
{
}


//  // Copy all vtkScalars ivars (except ComputeTime, CurrentLookupTable
//  // Colors)
//  void vtkScalars::CopyObject(vtkScalars* source)
//  {
//    this->SetData(source->Data);
//    memcpy(this->Range, source->Range, 6*sizeof(float));
//    this->ActiveComponent = source->ActiveComponent;
//    this->CurrentAlpha = source->CurrentAlpha;
  
//  }

// Set the data for this object. The tuple dimension must be consistent with
// the object.
void vtkScalars::SetData(vtkDataArray *data)
{
  if ( data != this->Data && data != NULL )
    {
    if (data->GetNumberOfComponents() > 4 )
      {
      vtkErrorMacro(<<"Tuple dimension for scalars must be <= 4");
      return;
      }
    this->Data->UnRegister(this);
    this->Data = data;
    this->Data->Register(this);
    this->Modified();
    }
}

// Given a list of point ids, return an array of scalar values.
void vtkScalars::GetScalars(vtkIdList *ptIds, vtkScalars *s)
{
  int num=ptIds->GetNumberOfIds();

  s->SetNumberOfScalars(num);
  for (int i=0; i<num; i++)
    {
    s->SetScalar(i,this->GetScalar(ptIds->GetId(i)));
    }
}

// Given a range of point ids [p1,p2], return an array of scalar values.
// Make sure enough space has been allocated in the vtkScalars object
// to hold all the values.
void vtkScalars::GetScalars(int p1, int p2, vtkScalars *fs)
{
  int i, id;

  for (i=0, id=p1; id <= p2; id++, i++)
    {
    fs->SetScalar(i,this->GetScalar(id));
    }
}

// Determine (rmin,rmax) range of scalar values.
void vtkScalars::ComputeRange()
{
  int i, numScalars=this->GetNumberOfScalars();
  float s;

  if ( this->GetMTime() > this->ComputeTime )
    {
    this->Range[0] =  VTK_LARGE_FLOAT;
    this->Range[1] =  -VTK_LARGE_FLOAT;
    for (i=0; i<numScalars; i++)
      {
      s = this->GetScalar(i);
      if ( s < this->Range[0] )
	{
	this->Range[0] = s;
	}
      if ( s > this->Range[1] )
	{
	this->Range[1] = s;
	}
      }

    this->ComputeTime.Modified();
    }
}

// Return the range of scalar values. Data returned as pointer to float array
// of length 2.
float *vtkScalars::GetRange()
{
  this->ComputeRange();
  return this->Range;
}

// Return the range of scalar values. Range copied into array provided.
void vtkScalars::GetRange(float range[2])
{
  this->ComputeRange();
  range[0] = this->Range[0];
  range[1] = this->Range[1];
}

void vtkScalars::CreateDefaultLookupTable()
{
  this->Data->CreateDefaultLookupTable();
}

vtkLookupTable* vtkScalars::GetLookupTable()
{
  if (this->Data)
    {
    return this->Data->GetLookupTable();
    }
  else
    {
    return 0;
    }
}
void vtkScalars::SetLookupTable(vtkLookupTable *lut)
{
  if (this->Data)
    {
    this->Data->SetLookupTable(lut);
    }
}

void vtkScalars::GetDataTypeRange(double range[2])
{
  range[0] = this->GetDataTypeMin();
  range[1] = this->GetDataTypeMax();
}

double vtkScalars::GetDataTypeMin()
{
  int dataType=this->Data->GetDataType();
  switch (dataType)
    {
    case VTK_BIT:            return (double)VTK_BIT_MIN;
    case VTK_UNSIGNED_CHAR:  return (double)VTK_UNSIGNED_CHAR_MIN;
    case VTK_CHAR:           return (double)VTK_CHAR_MIN;
    case VTK_UNSIGNED_SHORT: return (double)VTK_UNSIGNED_SHORT_MIN;
    case VTK_SHORT:          return (double)VTK_SHORT_MIN;
    case VTK_UNSIGNED_INT:   return (double)VTK_UNSIGNED_INT_MIN;
    case VTK_INT:            return (double)VTK_INT_MIN;
    case VTK_UNSIGNED_LONG:  return (double)VTK_UNSIGNED_LONG_MIN;
    case VTK_LONG:           return (double)VTK_LONG_MIN;
    case VTK_FLOAT:          return (double)VTK_FLOAT_MIN;
    case VTK_DOUBLE:         return (double)VTK_DOUBLE_MIN;
    default: return 0;
    }
}

double vtkScalars::GetDataTypeMax()
{
  int dataType=this->Data->GetDataType();
  switch (dataType)
    {
    case VTK_BIT:            return (double)VTK_BIT_MAX;
    case VTK_UNSIGNED_CHAR:  return (double)VTK_UNSIGNED_CHAR_MAX;
    case VTK_CHAR:           return (double)VTK_CHAR_MAX;
    case VTK_UNSIGNED_SHORT: return (double)VTK_UNSIGNED_SHORT_MAX;
    case VTK_SHORT:          return (double)VTK_SHORT_MAX;
    case VTK_UNSIGNED_INT:   return (double)VTK_UNSIGNED_INT_MAX;
    case VTK_INT:            return (double)VTK_INT_MAX;
    case VTK_UNSIGNED_LONG:  return (double)VTK_UNSIGNED_LONG_MAX;
    case VTK_LONG:           return (double)VTK_LONG_MAX;
    case VTK_FLOAT:          return (double)VTK_FLOAT_MAX;
    case VTK_DOUBLE:         return (double)VTK_DOUBLE_MAX;
    default: return 1;
    }
}

int vtkScalars::InitColorTraversal(float alpha, vtkScalarsToColors *lut,
                                   int colorMode)
{
  int numComp=this->GetNumberOfComponents();
  int blend=0;

  this->CurrentAlpha = alpha;
  this->RGBA[3] = (unsigned char)(alpha * 255.0);
  this->CurrentLookupTable = lut;
  
  // If unsigned char, assume that we have colors
  if ( this->GetDataType() == VTK_UNSIGNED_CHAR && 
  colorMode == VTK_COLOR_MODE_DEFAULT )
    {
    this->Colors = (vtkUnsignedCharArray *)this->GetData();
    if (numComp == 4) //rgba
      {
      blend = 1;
      if ( this->CurrentAlpha < 1.0 )
        {
        this->CurrentColorFunction = &vtkScalars::CompositeRGBA;
        }
      else
        {
        this->CurrentColorFunction = &vtkScalars::PassRGBA;
        }
      }
    else if (numComp == 3) //rgb
      {
      if ( this->CurrentAlpha < 1.0 )
        {
        blend = 1;
        }
      this->CurrentColorFunction = &vtkScalars::PassRGB;
      }
    else if (numComp == 2)
      {
      blend = 1;
      if ( this->CurrentAlpha < 1.0 )
        {
        this->CurrentColorFunction = &vtkScalars::CompositeIA;
        }
      else
        {
        this->CurrentColorFunction = &vtkScalars::PassIA;
        }
      }
    else
      {
      if ( this->CurrentAlpha < 1.0 )
        {
        blend = 1;
        }
      this->CurrentColorFunction = &vtkScalars::PassI;
      }
    }
  
  else if ( colorMode == VTK_COLOR_MODE_LUMINANCE )
    {
    this->CurrentColorFunction = &vtkScalars::Luminance;
    }
  
  else //have to be going through lookup table
    {
    this->Colors = NULL;
    if ( this->CurrentAlpha < 1.0 )
      {
      blend = 1;
      this->CurrentColorFunction = &vtkScalars::CompositeMapThroughLookupTable;
      }
    else
      {
      this->CurrentColorFunction = &vtkScalars::MapThroughLookupTable;
      }
    }
  
  return blend;
}

unsigned char *vtkScalars::PassRGBA(int id)
{
  return this->Colors->GetPointer(4*id);
}

unsigned char *vtkScalars::PassRGB(int id)
{
  unsigned char *rgba=this->Colors->GetPointer(3*id);
  this->RGBA[0] = *rgba++;
  this->RGBA[1] = *rgba++;
  this->RGBA[2] = *rgba;
  return this->RGBA;
}

unsigned char *vtkScalars::PassIA(int id)
{
  unsigned char *rgba=this->Colors->GetPointer(2*id);
  this->RGBA[0] = *rgba;
  this->RGBA[1] = *rgba;
  this->RGBA[2] = *rgba++;
  this->RGBA[3]  = *rgba;
  return this->RGBA;
}

unsigned char *vtkScalars::PassI(int id)
{
  unsigned char *rgba=this->Colors->GetPointer(id);
  this->RGBA[0] = *rgba;
  this->RGBA[1] = *rgba;
  this->RGBA[2] = *rgba;
  return this->RGBA;
}

unsigned char *vtkScalars::CompositeRGBA(int id)
{
  unsigned char *rgba=this->Colors->GetPointer(4*id);
  this->RGBA[0] = *rgba++;
  this->RGBA[1] = *rgba++;
  this->RGBA[2] = *rgba++;
  this->RGBA[3]  = (unsigned char)(*rgba * this->CurrentAlpha);
  return this->RGBA;
}

unsigned char *vtkScalars::CompositeIA(int id)
{
  unsigned char *rgba=this->Colors->GetPointer(2*id);
  this->RGBA[0] = *rgba;
  this->RGBA[1] = *rgba;
  this->RGBA[2] = *rgba++;
  this->RGBA[3]  = (unsigned char)(*rgba * this->CurrentAlpha);
  return this->RGBA;
}

unsigned char *vtkScalars::CompositeMapThroughLookupTable(int id)
{
  unsigned char *rgba = this->CurrentLookupTable->MapValue(this->GetScalar(id));
  this->RGBA[0] = *rgba++;
  this->RGBA[1] = *rgba++;
  this->RGBA[2] = *rgba++;
  this->RGBA[3]  = (unsigned char)(*rgba * this->CurrentAlpha);
  return this->RGBA;
}

unsigned char *vtkScalars::MapThroughLookupTable(int id)
{
  return this->CurrentLookupTable->MapValue(this->GetScalar(id));
}

unsigned char *vtkScalars::Luminance(int id)
{
  unsigned char *rgba = this->CompositeMapThroughLookupTable(id);
  this->RGBA[0] = (unsigned char)(0.30*rgba[0] + 0.59*rgba[1] + 0.11*rgba[2]);
  this->RGBA[1] = this->RGBA[2] = this->RGBA[0];
  this->RGBA[3] = rgba[3];
  
  return this->RGBA;
}

void vtkScalars::PrintSelf(ostream& os, vtkIndent indent)
{
  float *range;

  this->vtkAttributeData::PrintSelf(os,indent);

  os << indent << "Number Of Scalars: " << this->GetNumberOfScalars() << "\n";
  range = this->GetRange();
  os << indent << "Range: (" << range[0] << ", " << range[1] << ")\n";

  os << indent << "Number Of Components: " << this->GetNumberOfComponents() << "\n";
  os << indent << "Active Component: " << this->ActiveComponent << "\n";
}
