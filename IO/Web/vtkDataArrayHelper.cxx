/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArrayHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataArrayHelper.h"

#include "vtkDataArray.h"
#include "vtkSmartPointer.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"
#include "vtkTypeUInt32Array.h"
#include "vtkTypeUInt64Array.h"
#include "vtksys/MD5.h"
#include "vtksys/SystemTools.hxx"

#include <fstream>
#include <sstream>
#include <string>

// ----------------------------------------------------------------------------

vtkDataArrayHelper::vtkDataArrayHelper()
{
}

// ----------------------------------------------------------------------------

vtkDataArrayHelper::~vtkDataArrayHelper()
{
}

// ----------------------------------------------------------------------------

void vtkDataArrayHelper::ComputeMD5(const unsigned char* content, int size, std::string& hash)
{
  unsigned char digest[16];
  char md5Hash[33];
  md5Hash[32] = '\0';

  vtksysMD5* md5 = vtksysMD5_New();
  vtksysMD5_Initialize(md5);
  vtksysMD5_Append(md5, content, size);
  vtksysMD5_Finalize(md5, digest);
  vtksysMD5_DigestToHex(digest, md5Hash);
  vtksysMD5_Delete(md5);

  hash = md5Hash;
}

// ----------------------------------------------------------------------------

std::string vtkDataArrayHelper::GetShortType(vtkDataArray* input, bool& needConversion)
{
  needConversion = false;
  std::stringstream ss;
  switch (input->GetDataType())
  {
    case VTK_UNSIGNED_CHAR:
    case VTK_UNSIGNED_SHORT:
    case VTK_UNSIGNED_INT:
    case VTK_UNSIGNED_LONG:
    case VTK_UNSIGNED_LONG_LONG:
      ss << "Uint";
      if (input->GetDataTypeSize() <= 4)
      {
        ss << (input->GetDataTypeSize() * 8);
      }
      else
      {
        needConversion = true;
        ss << "32";
      }

      break;

    case VTK_CHAR:
    case VTK_SIGNED_CHAR:
    case VTK_SHORT:
    case VTK_INT:
    case VTK_LONG:
    case VTK_LONG_LONG:
    case VTK_ID_TYPE:
      ss << "Int";
      if (input->GetDataTypeSize() <= 4)
      {
        ss << (input->GetDataTypeSize() * 8);
      }
      else
      {
        needConversion = true;
        ss << "32";
      }
      break;

    case VTK_FLOAT:
    case VTK_DOUBLE:
      ss << "Float";
      ss << (input->GetDataTypeSize() * 8);
      break;

    case VTK_BIT:
    case VTK_STRING:
    case VTK_UNICODE_STRING:
    case VTK_VARIANT:
    default:
      ss << "xxx";
      break;
  }

  return ss.str();
}

// ----------------------------------------------------------------------------

std::string vtkDataArrayHelper::GetUID(vtkDataArray* input, bool& needConversion)
{
  const unsigned char* content = (const unsigned char*)input->GetVoidPointer(0);
  int size = input->GetNumberOfValues() * input->GetDataTypeSize();
  std::string hash;
  vtkDataArrayHelper::ComputeMD5(content, size, hash);

  std::stringstream ss;
  ss << vtkDataArrayHelper::GetShortType(input, needConversion) << "_" << input->GetNumberOfValues()
     << "-" << hash.c_str();

  return ss.str();
}

// ----------------------------------------------------------------------------

bool vtkDataArrayHelper::WriteArray(vtkDataArray* input, const char* filePath)
{
  if (input->GetDataTypeSize() == 0)
  {
    // Skip BIT arrays
    return false;
  }

  // Make sure parent directory exist
  vtksys::SystemTools::MakeDirectory(vtksys::SystemTools::GetParentDirectory(filePath));

  // Check if we need to convert the (u)int64 to (u)int32
  vtkSmartPointer<vtkDataArray> arrayToWrite = input;
  vtkIdType arraySize = input->GetNumberOfTuples() * input->GetNumberOfComponents();
  switch (input->GetDataType())
  {
    case VTK_UNSIGNED_CHAR:
    case VTK_UNSIGNED_LONG:
    case VTK_UNSIGNED_LONG_LONG:
      if (input->GetDataTypeSize() > 4)
      {
        vtkNew<vtkTypeUInt64Array> srcUInt64;
        srcUInt64->ShallowCopy(input);
        vtkNew<vtkTypeUInt32Array> uint32;
        uint32->SetNumberOfValues(arraySize);
        uint32->SetName(input->GetName());
        for (vtkIdType i = 0; i < arraySize; i++)
        {
          uint32->SetValue(i, srcUInt64->GetValue(i));
        }
        arrayToWrite = uint32;
      }
      break;
    case VTK_LONG:
    case VTK_LONG_LONG:
    case VTK_ID_TYPE:
      if (input->GetDataTypeSize() > 4)
      {
        vtkNew<vtkTypeInt64Array> srcInt64;
        srcInt64->ShallowCopy(input);
        vtkNew<vtkTypeInt32Array> int32;
        int32->SetNumberOfTuples(arraySize);
        int32->SetName(input->GetName());
        for (vtkIdType i = 0; i < arraySize; i++)
        {
          int32->SetValue(i, srcInt64->GetValue(i));
        }
        arrayToWrite = int32;
      }
      break;
  }

  const char* content = (const char*)arrayToWrite->GetVoidPointer(0);
  size_t size = arrayToWrite->GetNumberOfValues() * arrayToWrite->GetDataTypeSize();

  ofstream file;
  file.open(filePath, ios::out | ios::binary);
  file.write(content, size);
  file.close();

  return true;
}
