/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalars.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkScalars.h"
#include "vtkLookupTable.h"

vtkScalars::vtkScalars(int dataType, int dim) : vtkAttributeData(dataType)
{
  dim = (dim < 1 ? 1 : (dim > 4 ? 4 : dim));
  this->Data->SetNumberOfComponents(dim);

  this->Range[0] = this->Range[2] = this->Range[4] = this->Range[6] = 0.0;
  this->Range[1] = this->Range[3] = this->Range[5] = this->Range[7] = 1.0;

  this->LookupTable = NULL;
  this->ActiveComponent = 0;
  
  this->CurrentAlpha = 1.0;
  this->CurrentLookupTable = NULL;
  this->CurrentColorFunction = NULL;
}

vtkScalars::~vtkScalars()
{
  if ( this->LookupTable ) this->LookupTable->Delete();
}

// Description:
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

// Description:
// Given a list of point ids, return an array of scalar values.
void vtkScalars::GetScalars(vtkIdList& ptId, vtkScalars& fs)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    fs.InsertScalar(i,this->GetScalar(ptId.GetId(i)));
    }
}

// Description:
// Given a range of point ids [p1,p2], return an array of scalar values.
// Make sure enough space has been allocated in the vtkScalars object
// to hold all the values.
void vtkScalars::GetScalars(int p1, int p2, vtkScalars& fs)
{
  int i, id;

  for (i=0, id=p1; id <= p2; id++, i++)
    {
    fs.SetScalar(i,this->GetScalar(id));
    }
}

// Description:
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
      if ( s < this->Range[0] ) this->Range[0] = s;
      if ( s > this->Range[1] ) this->Range[1] = s;
      }

    this->ComputeTime.Modified();
    }
}

// Description:
// Return the range of scalar values. Data returned as pointer to float array
// of length 2.
float *vtkScalars::GetRange()
{
  this->ComputeRange();
  return this->Range;
}

// Description:
// Return the range of scalar values. Range copied into array provided.
void vtkScalars::GetRange(float range[2])
{
  this->ComputeRange();
  range[0] = this->Range[0];
  range[1] = this->Range[1];
}

void vtkScalars::CreateDefaultLookupTable()
{
  if ( this->LookupTable ) this->LookupTable->UnRegister(this);
  this->LookupTable = vtkLookupTable::New();
  // make sure it is built 
  // otherwise problems with InsertScalar trying to map through 
  // non built lut
  this->LookupTable->Build();
  this->LookupTable->Register(this);
}

void vtkScalars::SetLookupTable(vtkLookupTable *lut)
{
  if ( this->LookupTable != lut ) 
    {
    if ( this->LookupTable ) this->LookupTable->UnRegister(this);
    this->LookupTable = lut;
    this->LookupTable->Register(this);
    this->Modified();
    }
}

void vtkScalars::GetDataTypeRange(float range[2])
{
  range[0] = this->GetDataTypeMin();
  range[1] = this->GetDataTypeMax();
}

float vtkScalars::GetDataTypeMin()
{
  int dataType=this->Data->GetDataType();
  switch (dataType)
    {
    case VTK_BIT:            return (float)VTK_BIT_MIN;
    case VTK_UNSIGNED_CHAR:  return (float)VTK_UNSIGNED_CHAR_MIN;
    case VTK_CHAR:           return (float)VTK_CHAR_MIN;
    case VTK_UNSIGNED_SHORT: return (float)VTK_UNSIGNED_SHORT_MIN;
    case VTK_SHORT:          return (float)VTK_SHORT_MIN;
    case VTK_UNSIGNED_INT:   return (float)VTK_UNSIGNED_INT_MIN;
    case VTK_INT:            return (float)VTK_INT_MIN;
    case VTK_UNSIGNED_LONG:  return (float)VTK_UNSIGNED_LONG_MIN;
    case VTK_LONG:           return (float)VTK_LONG_MIN;
    case VTK_FLOAT:          return (float)VTK_FLOAT_MIN;
    case VTK_DOUBLE:         return (float)VTK_DOUBLE_MIN;
    default: return 0;
    }
}

float vtkScalars::GetDataTypeMax()
{
  int dataType=this->Data->GetDataType();
  switch (dataType)
    {
    case VTK_BIT:            return (float)VTK_BIT_MAX;
    case VTK_UNSIGNED_CHAR:  return (float)VTK_UNSIGNED_CHAR_MAX;
    case VTK_CHAR:           return (float)VTK_CHAR_MAX;
    case VTK_UNSIGNED_SHORT: return (float)VTK_UNSIGNED_SHORT_MAX;
    case VTK_SHORT:          return (float)VTK_SHORT_MAX;
    case VTK_UNSIGNED_INT:   return (float)VTK_UNSIGNED_INT_MAX;
    case VTK_INT:            return (float)VTK_INT_MAX;
    case VTK_UNSIGNED_LONG:  return (float)VTK_UNSIGNED_LONG_MAX;
    case VTK_LONG:           return (float)VTK_LONG_MAX;
    case VTK_FLOAT:          return (float)VTK_FLOAT_MAX;
    case VTK_DOUBLE:         return (float)VTK_DOUBLE_MAX;
    default: return 1;
    }
}

int vtkScalars::InitColorTraversal(float alpha, vtkLookupTable *lut,
                                   int colorMode)
{
  int numComp=this->GetNumberOfComponents();
  int blend=0;

  this->CurrentAlpha = alpha;
  this->RGBA[3] = alpha * 255.0;
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
  this->RGBA[3]  = *rgba * this->CurrentAlpha;
  return this->RGBA;
}

unsigned char *vtkScalars::CompositeIA(int id)
{
  unsigned char *rgba=this->Colors->GetPointer(2*id);
  this->RGBA[0] = *rgba;
  this->RGBA[1] = *rgba;
  this->RGBA[2] = *rgba++;
  this->RGBA[3]  = *rgba * this->CurrentAlpha;
  return this->RGBA;
}

unsigned char *vtkScalars::CompositeMapThroughLookupTable(int id)
{
  unsigned char *rgba = this->CurrentLookupTable->MapValue(this->GetScalar(id));
  this->RGBA[0] = *rgba++;
  this->RGBA[1] = *rgba++;
  this->RGBA[2] = *rgba++;
  this->RGBA[3]  = *rgba * this->CurrentAlpha;
  return this->RGBA;
}

unsigned char *vtkScalars::MapThroughLookupTable(int id)
{
  return this->CurrentLookupTable->MapValue(this->GetScalar(id));
}

unsigned char *vtkScalars::Luminance(int id)
{
  unsigned char *rgba = this->CompositeMapThroughLookupTable(id);
  this->RGBA[0] = 0.30*rgba[0] + 0.59*rgba[1] + 0.11*rgba[2];
  this->RGBA[1] = this->RGBA[2] = this->RGBA[0];
  this->RGBA[3] = rgba[3];
  
  return this->RGBA;
}

void vtkScalars::PrintSelf(ostream& os, vtkIndent indent)
{
  float *range;

  vtkReferenceCount::PrintSelf(os,indent);

  os << indent << "Number Of Scalars: " << this->GetNumberOfScalars() << "\n";
  range = this->GetRange();
  os << indent << "Range: (" << range[0] << ", " << range[1] << ")\n";
  if ( this->LookupTable )
    {
    os << indent << "Lookup Table:\n";
    this->LookupTable->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "LookupTable: (none)\n";
    }

  os << indent << "Number Of Components: " << this->GetNumberOfComponents() << "\n";
  os << indent << "Active Component: " << this->ActiveComponent << "\n";
}
