/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAttributeData.cxx
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
#include "vtkAttributeData.h"
#include "vtkBitArray.h"
#include "vtkCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkShortArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkLongArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"

// Description:
// Construct object with an initial data array of type dataType (by default
// dataType is VTK_FLOAT.
vtkAttributeData::vtkAttributeData(int dataType)
{
  this->Data = vtkFloatArray::New();
  this->SetDataType(dataType);
}

vtkAttributeData::~vtkAttributeData()
{
  this->Data->Delete();
}

int vtkAttributeData::Allocate(const int sz, const int ext)
{
  int numComp=this->Data->GetNumberOfComponents();
  return this->Data->Allocate(sz*numComp,ext*numComp);
}

void vtkAttributeData::Initialize()
{
  this->Data->Initialize();
}

int vtkAttributeData::GetDataType()
{
  return this->Data->GetDataType();
}

// Description:
// Specify the underlying data type of the object.
void vtkAttributeData::SetDataType(int dataType)
{
  if ( dataType == this->Data->GetDataType() )
    {
    return;
    }
  // special cases
  if (dataType == VTK_UNSIGNED_INT 
      && this->Data->GetDataType() == VTK_UNSIGNED_SHORT)
    {
    return;
    }
  if (dataType == VTK_LONG 
      && this->Data->GetDataType() == VTK_INT)
    {
    return;
    }
  if (dataType == VTK_UNSIGNED_LONG 
      && this->Data->GetDataType() == VTK_UNSIGNED_SHORT)
    {
    return;
    }
  if (dataType == VTK_UNSIGNED_INT 
      && this->Data->GetDataType() == VTK_UNSIGNED_SHORT)
    {
    return;
    }
  
  this->Modified();
  
  switch (dataType)
    {
    case VTK_BIT:
      this->Data->Delete();
      this->Data = vtkBitArray::New();
      break;

    case VTK_CHAR:
      this->Data->Delete();
      this->Data = vtkCharArray::New();
      break;

    case VTK_UNSIGNED_CHAR:
      this->Data->Delete();
      this->Data = vtkUnsignedCharArray::New();
      break;

    case VTK_SHORT:
      this->Data->Delete();
      this->Data = vtkShortArray::New();
      break;

    case VTK_UNSIGNED_SHORT:
      this->Data->Delete();
      this->Data = vtkUnsignedShortArray::New();
      break;

    case VTK_INT:
      this->Data->Delete();
      this->Data = vtkIntArray::New();
      break;

    case VTK_UNSIGNED_INT:
      this->Data->Delete();
      this->Data = vtkUnsignedShortArray::New();
      break;

    case VTK_LONG:
      this->Data->Delete();
      this->Data = vtkIntArray::New();
      break;

    case VTK_UNSIGNED_LONG:
      this->Data->Delete();
      this->Data = vtkUnsignedShortArray::New();
      break;

    case VTK_FLOAT:
      this->Data->Delete();
      this->Data = vtkFloatArray::New();
      break;

    case VTK_DOUBLE:
      this->Data->Delete();
      this->Data = vtkDoubleArray::New();
      break;

    default:
      vtkErrorMacro(<<"Unsupported data type! Setting to VTK_FLOAT");
      this->SetDataType(VTK_FLOAT);
    }
}

// Description:
// Set the data for this object. The tuple dimension must be consistent with
// the object.
void vtkAttributeData::SetData(vtkDataArray *data)
{
  if ( data != this->Data && data != NULL )
    {
    if (data->GetNumberOfComponents() != this->Data->GetNumberOfComponents() )
      {
      vtkErrorMacro(<<"Number of components is different...can't set data");
      return;
      }
    this->Data->UnRegister(this);
    this->Data = data;
    this->Data->Register(this);
    this->Modified();
    }
}

// Description:
// Deep copy of data. Checks consistency to make sure this operation
// makes sense.
void vtkAttributeData::DeepCopy(vtkAttributeData& da)
{
  if ( da.Data != this->Data && &da.Data != NULL )
    {
    if (da.Data->GetNumberOfComponents() != this->Data->GetNumberOfComponents() )
      {
      vtkErrorMacro(<<"Number of components is different...can't copy");
      return;
      }
    this->Data->DeepCopy(*(da.Data));
    this->Modified();
    }
}

// Description:
// Shallow copy of data (i.e. via reference counting). Checks 
// consistency to make sure this operation makes sense.
void vtkAttributeData::ShallowCopy(vtkAttributeData& da)
{
  this->SetData(da.GetData());
}

void vtkAttributeData::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkReferenceCount::PrintSelf(os,indent);

  os << indent << "Data: " << this->Data << "\n";
}

