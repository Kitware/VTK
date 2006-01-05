/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAbstractArray.h"

#include "vtkBitArray.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIntArray.h"
#include "vtkIdTypeArray.h"
#include "vtkLongArray.h"
#include "vtkShortArray.h"
#include "vtkSignedCharArray.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"

#if defined(VTK_TYPE_USE_LONG_LONG)
# include "vtkLongLongArray.h"
# include "vtkUnsignedLongLongArray.h"
#endif

#if defined(VTK_TYPE_USE___INT64)
# include "vtk__Int64Array.h"
# if defined(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
#  include "vtkUnsigned__Int64Array.h"
# endif
#endif
vtkCxxRevisionMacro(vtkAbstractArray, "1.6");
//----------------------------------------------------------------------------
// Construct object with sane defaults.
vtkAbstractArray::vtkAbstractArray(vtkIdType vtkNotUsed(numComp))
{
  this->Size = 0;
  this->MaxId = -1;
  this->NumberOfComponents = 1;
  this->Name = NULL;
}

//----------------------------------------------------------------------------
vtkAbstractArray::~vtkAbstractArray()
{
  this->SetName(NULL);
}

//----------------------------------------------------------------------------
void vtkAbstractArray::GetTuples(vtkIdList* ptIds, vtkAbstractArray* aa)
{
  if (aa->GetNumberOfComponents() != this->GetNumberOfComponents())
    {
    vtkWarningMacro("Number of components for input and output do not match.");
    return;
    }
  // Here we give the slowest implementation. Subclasses can override
  // to use the knowledge about the data.
  vtkIdType num = ptIds->GetNumberOfIds();
  for (vtkIdType i = 0; i < num; i++)
    {
    aa->SetTuple(i, ptIds->GetId(i), this);
    }
}

//----------------------------------------------------------------------------
void vtkAbstractArray::GetTuples(vtkIdType p1, vtkIdType p2, 
  vtkAbstractArray* aa)
{
  if (aa->GetNumberOfComponents() != this->GetNumberOfComponents())
    {
    vtkWarningMacro("Number of components for input and output do not match.");
    return;
    }
  
  // Here we give the slowest implementation. Subclasses can override
  // to use the knowledge about the data.
  vtkIdType num = p2 - p1 + 1;
  for (vtkIdType i = 0; i < num; i++)
    {
    aa->SetTuple(i, (p1+i), this);
    }
}


//----------------------------------------------------------------------------
template <class T>
unsigned long vtkAbstractArrayGetDataTypeSize(T*)
{
  return sizeof(T);
}

unsigned long vtkAbstractArray::GetDataTypeSize(int type)
{
  switch (type)
    {
    vtkTemplateMacro(
      return vtkAbstractArrayGetDataTypeSize(static_cast<VTK_TT*>(0))
      );
      
    case VTK_BIT:
      return 0;
      break;

    case VTK_STRING:
      return 0;
      break;

    default:
      vtkGenericWarningMacro(<<"Unsupported data type!");
    }
  
  return 1;
}

// ----------------------------------------------------------------------
vtkAbstractArray* vtkAbstractArray::CreateArray(int dataType)
{
  switch (dataType)
    {
    case VTK_BIT:
      return vtkBitArray::New();

    case VTK_CHAR:
      return vtkCharArray::New();

    case VTK_SIGNED_CHAR:
      return vtkSignedCharArray::New();

    case VTK_UNSIGNED_CHAR:
      return vtkUnsignedCharArray::New();

    case VTK_SHORT:
      return vtkShortArray::New();

    case VTK_UNSIGNED_SHORT:
      return vtkUnsignedShortArray::New();

    case VTK_INT:
      return vtkIntArray::New();

    case VTK_UNSIGNED_INT:
      return vtkUnsignedIntArray::New();

    case VTK_LONG:
      return vtkLongArray::New();

    case VTK_UNSIGNED_LONG:
      return vtkUnsignedLongArray::New();

#if defined(VTK_TYPE_USE_LONG_LONG)
    case VTK_LONG_LONG:
      return vtkLongLongArray::New();

    case VTK_UNSIGNED_LONG_LONG:
      return vtkUnsignedLongLongArray::New();
#endif

#if defined(VTK_TYPE_USE___INT64)
    case VTK___INT64:
      return vtk__Int64Array::New();
      break;

# if defined(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
    case VTK_UNSIGNED___INT64:
      return vtkUnsigned__Int64Array::New();
      break;
# endif
#endif

    case VTK_FLOAT:
      return vtkFloatArray::New();

    case VTK_DOUBLE:
      return vtkDoubleArray::New();

    case VTK_ID_TYPE:
      return vtkIdTypeArray::New();
   
    case VTK_STRING:
      return vtkStringArray::New();

    default:
      vtkGenericWarningMacro("Unsupported data type " << dataType
                             << "! Setting to VTK_DOUBLE");
      return vtkDoubleArray::New();
    }
}

//----------------------------------------------------------------------------
void vtkAbstractArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  const char* name = this->GetName();
  if (name)
    {
    os << indent << "Name: " << name << "\n";
    }
  else
    {
    os << indent << "Name: (none)\n";
    }
  os << indent << "Data type: " << this->GetDataTypeAsString();
  os << indent << "Size: " << this->Size << "\n";
  os << indent << "MaxId: " << this->MaxId << "\n";
  os << indent << "NumberOfComponents: " << this->NumberOfComponents << endl;
}
