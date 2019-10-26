/*=========================================================================
  Copyright (c) GeometryFactory
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSEPReader.h"

#include "vtkInformation.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include <iostream>
#include <sstream>
#include <string>

vtkStandardNewMacro(vtkSEPReader);

namespace
{
//----------------------------------------------------------------------------
void TrimString(std::string& s)
{
  // trim trailing spaces
  std::size_t pos = s.find_last_not_of(" \t");
  if (pos != std::string::npos)
  {
    s = s.substr(0, pos + 1);
  }
  // trim leading spaces
  pos = s.find_first_not_of(" \t");
  if (pos != std::string::npos)
  {
    s = s.substr(pos);
  }
}
}

//----------------------------------------------------------------------------
vtkSEPReader::vtkSEPReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetFileLowerLeft(1);
}

//-----------------------------------------------------------------------------
int vtkSEPReader::CanReadFile(const char* filename)
{
  std::string extension = vtksys::SystemTools::GetFilenameLastExtension(filename);
  return (extension == ".H") ? 1 : 0;
}

//-----------------------------------------------------------------------------
int vtkSEPReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->ReadHeader())
  {
    return 0;
  }

  return this->Superclass::RequestInformation(request, inputVector, outputVector);
}

//-----------------------------------------------------------------------------
void vtkSEPReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "DataFile: " << this->DataFile << std::endl;
}

//-----------------------------------------------------------------------------
int vtkSEPReader::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Replace the filename with the data file and delegate the reading of this
  // raw data to the underlying vtkImageReader
  char* const fileName = this->FileName;

  // This `const_cast` is valid because the `RequestData` of
  // `Superclass` does not try to modify the string pointed by
  // `FileName`. (It does not modify the pointer `FileName` as well,
  // but that is not why the `const_cast` is valid.)
  this->FileName = const_cast<char*>(this->DataFile.c_str());

  int res = this->Superclass::RequestData(request, inputVector, outputVector);

  // Restore the user provided filename (the header file)
  this->FileName = fileName;
  return res;
}

//-----------------------------------------------------------------------------
int vtkSEPReader::ReadHeader()
{
  if (!this->FileName)
  {
    vtkErrorMacro(<< "A FileName must be specified.");
    return 0;
  }

  // Open the new file
  if (!vtksys::SystemTools::FileExists(this->FileName))
  {
    vtkErrorMacro(<< "Could not find file " << this->FileName);
    return 0;
  }

  vtksys::ifstream file(this->FileName, ios::in | ios::binary);
  if (file.fail())
  {
    vtkErrorMacro(<< "Could not open file " << this->FileName);
    return 0;
  }

  std::string line;
  while (vtksys::SystemTools::GetLineFromStream(file, line))
  {
    auto splittedLine = vtksys::SystemTools::SplitString(line.c_str(), '=');
    if (splittedLine.size() == 2)
    {
      std::string key = splittedLine[0];
      std::string value = splittedLine[1];
      std::istringstream iss(value);
      iss.imbue(std::locale::classic());
      double d_value;
      iss >> d_value;

      ::TrimString(key);
      ::TrimString(value);
      if (key.length() == 2 && key[0] == 'n')
      {
        this->DataExtent[2 * (key[1] - '0' - 1)] = 0;
        this->DataExtent[2 * (key[1] - '0' - 1) + 1] = atoi(value.c_str()) - 1;
      }
      else if (key.length() == 2 && key[0] == 'd')
      {
        this->DataSpacing[key[1] - '0' - 1] = d_value;
      }
      else if (key.length() == 2 && key[0] == 'o')
      {
        this->DataOrigin[key[1] - '0' - 1] = d_value;
      }
      else if (key == "data_format")
      {
        vtksys::SystemTools::ReplaceString(value, "\"", "");
        if (value == "xdr_float" || value == "native_float")
        {
          this->HeaderSize = 0;
          this->DataScalarType = VTK_FLOAT;
        }
        else
        {
          vtkErrorMacro("Only xdr_float data format is currently supported!");
          return 0;
        }
        if (value.substr(0, 3) != "xdr")
        {
#ifdef VTK_WORDS_BIGENDIAN
          this->SwapBytes = 1;
#else
          this->SwapBytes = 0;
#endif
        }
        else
        {
#ifdef VTK_WORDS_BIGENDIAN
          this->SwapBytes = 0;
#else
          this->SwapBytes = 1;
#endif
        }
      }
      else if (key == "in")
      {
        std::string path = vtksys::SystemTools::GetFilenamePath(this->FileName);
        if (path.empty())
        {
          this->DataFile = value;
        }
        else
        {
          vtksys::SystemTools::LocateFileInDir(value.c_str(),
            vtksys::SystemTools::GetParentDirectory(this->FileName).c_str(), this->DataFile);
        }
      }
    }
  }

  if (!vtksys::SystemTools::PathExists(this->DataFile))
  {
    vtkErrorMacro("Unable to find the raw data file " << this->DataFile);
    return 0;
  }

  int dim = 0;
  for (int i = 0; i < 3; ++i)
  {
    if (this->DataExtent[2 * i + 1] == 0)
    {
      this->DataExtent[2 * i + 1] = 1;
    }
    else
    {
      ++dim;
    }
  }
  this->FileDimensionality = dim;

  return 1;
}
