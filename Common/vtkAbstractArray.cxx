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
#include "vtkIdList.h"
#include "vtkMath.h"

#include "vtkDataArray.h"
#include "vtkBitArray.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkIdTypeArray.h"
#include "vtkLongArray.h"
#include "vtkShortArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"

// we need this for GetSize()
#include <vtkstd/string>
using vtkstd::string;

vtkCxxRevisionMacro(vtkAbstractArray, "1.2");

// Construct object with sane defaults.

vtkAbstractArray::vtkAbstractArray(vtkIdType vtkNotUsed(numComp))
{
  this->Size = 0;
  this->MaxId = -1;

  this->Name = NULL;
  this->DataType = -1;
}

vtkAbstractArray::~vtkAbstractArray()
{
  if (this->Name != NULL)
    {
    delete [] this->Name;
    }
  this->Name = NULL;
}

void vtkAbstractArray::SetName(const char* name)
{
  if (this->Name != NULL)
    {
    delete[] this->Name;
    }

  this->Name = NULL;
  if (name)
    {
    int size = static_cast<int>(strlen(name));
    this->Name = new char[size+1];
    strcpy(this->Name, name);
    }
}

const char* vtkAbstractArray::GetName()
{
  return this->Name;
}


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
}

unsigned long vtkAbstractArray::GetDataTypeSize(int type)
{
  switch (type)
    {
    case VTK_BIT:
      return 1;
      break;

    case VTK_CHAR:
      return sizeof(char);
      break;

    case VTK_UNSIGNED_CHAR:
      return sizeof(unsigned char);
      break;

    case VTK_SHORT:
      return sizeof(short);
      break;

    case VTK_UNSIGNED_SHORT:
      return sizeof(unsigned short);
      break;

    case VTK_INT:
      return sizeof(int);
      break;

    case VTK_UNSIGNED_INT:
      return sizeof(unsigned int);
      break;

    case VTK_LONG:
      return sizeof(long);
      break;

    case VTK_UNSIGNED_LONG:
      return sizeof(unsigned long);
      break;

    case VTK_FLOAT:
      return sizeof(float);
      break;

    case VTK_DOUBLE:
      return sizeof(double);
      break;

    case VTK_ID_TYPE:
      return sizeof(vtkIdType);
      break;

    case VTK_CELL:
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

    case VTK_FLOAT:
      return vtkFloatArray::New();

    case VTK_DOUBLE:
      return vtkDoubleArray::New();

    case VTK_ID_TYPE:
      return vtkIdTypeArray::New();

    default:
      vtkGenericWarningMacro(<<"Unsupported data type! Setting to VTK_DOUBLE");
      return vtkDoubleArray::New();
    }
}
