// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNrrdReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkPNrrdReader.h"

#include "vtkCharArray.h"
#include "vtkDummyController.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"

#include <vtkstd/string>
#include <vtkstd/vector>
#include <vtksys/SystemTools.hxx>
#include <istream>
#include <sstream>

#include <math.h>
#include <string.h>

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

//=============================================================================
static vtkstd::string trim(vtkstd::string s)
{
  size_t start = 0;
  while ((start < s.length()) && (s[start] == ' ')) start++;
  size_t end = s.length();
  while ((end > start) && (s[end-1] == ' ')) end--;
  return s.substr(start, end-start);
}

//-----------------------------------------------------------------------------
static vtkstd::vector<vtkstd::string> split(vtkstd::string s)
{
  vtkstd::vector<vtkstd::string> result;
  size_t startValue = 0;
  while (true)
    {
    while ((startValue != s.npos) && (s[startValue] == ' ')) startValue++;
    if (startValue == s.npos) return result;
    size_t endValue = s.find(' ', startValue);
    if (endValue != s.npos)
      {
      result.push_back(s.substr(startValue, endValue-startValue));
      }
    else
      {
      result.push_back(s.substr(startValue));
      }
    startValue = endValue;
    }
}

//-----------------------------------------------------------------------------
static void GetVector(vtkstd::string s, vtkstd::vector<int> &dest)
{
  vtkstd::vector<vtkstd::string> strlist = split(s);
  for (size_t i = 0; i < dest.size(); i++)
    {
    if (i < strlist.size())
      {
      dest[i] = atoi(strlist[i].c_str());
      }
    else
      {
      dest[i] = 0;
      }
    }
}

//-----------------------------------------------------------------------------
static void GetVector(vtkstd::string s, vtkstd::vector<double> &dest)
{
  vtkstd::vector<vtkstd::string> strlist = split(s);
  for (size_t i = 0; i < dest.size(); i++)
    {
    if (i < strlist.size())
      {
      dest[i] = atof(strlist[i].c_str());
      }
    else
      {
      dest[i] = 0.0;
      }
    }
}

//-----------------------------------------------------------------------------
static vtkstd::vector<double> ParseVector(vtkstd::string s)
{
  vtkstd::vector<double> result;

  s = trim(s);
  if ((s[0] != '(') || (s[s.length()-1] != ')')) return result;
  s = s.substr(1, s.length()-2);
  while (true)
    {
    size_t i = s.find(',');
    vtkstd::string value = s.substr(0, i);
    result.push_back(atof(value.c_str()));
    if (i == s.npos) break;
    s = s.substr(i+1);
    }

  return result;
}

//-----------------------------------------------------------------------------
static int NrrdType2VTKType(vtkstd::string nrrdType)
{
  nrrdType = trim(nrrdType);
  if (   (nrrdType == "signed char")
      || (nrrdType == "int8") || (nrrdType == "int8_t") )
    {
    return VTK_CHAR;
    }
  else if (   (nrrdType == "uchar") || (nrrdType == "unsigned char")
           || (nrrdType == "uint8") || (nrrdType == "uint8_t") )
    {
    return VTK_UNSIGNED_CHAR;
    }
  else if (   (nrrdType == "short") || (nrrdType == "short int")
           || (nrrdType == "signed short") || (nrrdType == "signed short int")
           || (nrrdType == "int16") || (nrrdType == "int16_t") )
    {
    return VTK_SHORT;
    }
  else if (   (nrrdType == "ushort") || (nrrdType == "unsigned short")
           || (nrrdType == "unsigned short int")
           || (nrrdType == "uint16") || (nrrdType == "uint16_t") )
    {
    return VTK_UNSIGNED_SHORT;
    }
  else if (   (nrrdType == "int") || (nrrdType == "signed int")
           || (nrrdType == "int32") || (nrrdType == "int32_t") )
    {
    return VTK_INT;
    }
  else if (   (nrrdType == "uint") || (nrrdType == "unsigned int")
           || (nrrdType == "uint32") || (nrrdType == "uint32_t") )
    {
    return VTK_UNSIGNED_INT;
    }
  else if (   (nrrdType == "longlong") || (nrrdType == "long long")
           || (nrrdType == "long long int") || (nrrdType == "signed long long")
           || (nrrdType == "signed long long int") || (nrrdType == "int64")
           || (nrrdType == "int64_t") )
    {
    return VTK_TYPE_INT64;
    }
  else if (   (nrrdType == "ulonglong") || (nrrdType == "unsigned long long")
           || (nrrdType == "unsigned long long int")
           || (nrrdType == "uint64") || (nrrdType == "uint64_t") )
    {
    return VTK_TYPE_UINT64;
    }
  else if (nrrdType == "float")
    {
    return VTK_FLOAT;
    }
  else if (nrrdType == "double")
    {
    return VTK_DOUBLE;
    }
  else if (nrrdType == "block")
    {
    vtkGenericWarningMacro(<< "Reading blocks not supported.");
    return VTK_VOID;
    }
  else
    {
    vtkGenericWarningMacro(<< "Unknown type: " << nrrdType);
    return VTK_VOID;
    }
}

//=============================================================================
vtkStandardNewMacro(vtkPNrrdReader);

//-----------------------------------------------------------------------------
vtkPNrrdReader::vtkPNrrdReader()
{
  this->DataFiles = vtkStringArray::New();
}

vtkPNrrdReader::~vtkPNrrdReader()
{
  this->DataFiles->Delete();
  this->DataFiles = NULL;
}

void vtkPNrrdReader::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkPNrrdReader::CanReadFile(const char *filename)
{
  ifstream file(filename, ios::in | ios::binary);
  vtkstd::string firstLine;
  getline(file, firstLine);
  if (firstLine.substr(0, 4) == "NRRD")
    {
    return 2;
    }
  else
    {
    return 0;
    }
}

//-----------------------------------------------------------------------------
int vtkPNrrdReader::RequestInformation(vtkInformation *request,
                                       vtkInformationVector **inputVector,
                                       vtkInformationVector *outputVector)
{
  if (!this->ReadHeader()) return 0;

  return this->Superclass::RequestInformation(request,
                                              inputVector, outputVector);
}

//-----------------------------------------------------------------------------
int vtkPNrrdReader::ReadHeader()
{
  if (!this->FileName)
    {
    vtkErrorMacro(<< "No filename set.");
    return 0;
    }

  VTK_CREATE(vtkCharArray, headerBuffer);

  // Having a dummy controller means less cases later.
  if (!this->Controller) this->Controller = vtkDummyController::New();

  // Read the header on process 0 and broadcast to everyone else.
  if (this->Controller->GetLocalProcessId() == 0)
    {
    ifstream file(this->FileName, ios::in | ios::binary);
    // Read in 4 MB.  Assuming that the header will be smaller than that.
    headerBuffer->SetNumberOfTuples(0x400000);
    file.read(headerBuffer->GetPointer(0), 0x400000-1);
    vtkIdType buffersize = file.gcount();
    headerBuffer->SetValue(buffersize, '\0');
    headerBuffer->SetNumberOfTuples(buffersize+1);

    // Find a blank line (which signals the end of the header).  Be careful
    // because line endings can be "\n", "\r\n", or both.  It is possible that
    // the entire file is the header (happens with "detatched headers").
    char *bufferStart = headerBuffer->GetPointer(0);
    char *s = bufferStart;
    while ((s = strchr(s+1, '\n')) != NULL)
      {
      // If you find a double line ending, terminate the string and shorten the
      // buffer.
      if (s[1] == '\n')
        {
        s[2] = '\0';
        headerBuffer->SetNumberOfTuples(
                                   static_cast<vtkIdType>(s + 3 - bufferStart));
        break;
        }
      if ((s[1] == '\r') && (s[2] == '\n'))
        {
        s[3] = '\0';
        headerBuffer->SetNumberOfTuples(
                                   static_cast<vtkIdType>(s + 4 - bufferStart));
        break;
        }
      }
    }

  this->Controller->Broadcast(headerBuffer, 0);

  return this->ReadHeader(headerBuffer);
}

//-----------------------------------------------------------------------------
int vtkPNrrdReader::ReadHeader(vtkCharArray *headerBuffer)
{
  this->HeaderSize = headerBuffer->GetNumberOfTuples();

  vtkstd::string headerStringBuffer = headerBuffer->GetPointer(0);
  vtkstd::stringstream header(headerStringBuffer);

  vtkstd::string line;
  getline(header, line);
  if (line.substr(0, 4) != "NRRD")
    {
    vtkErrorMacro(<< this->FileName << " is not a nrrd file.");
    return 0;
    }

  this->DataFiles->Initialize();
  int byteskip = 0;
  int numDimensions = 0;
  int subDimension = -1;
  vtkstd::vector<int> dimSizes;
  vtkstd::vector<double> dimSpacing;
  this->FileLowerLeft = 1;
  while (1)
    {
    getline(header, line);
    if (line.length() < 1) break;

    if (line[0] == '#')
      {
      // Comment.  Ignore.
      continue;
      }

    size_t delm = line.find(": ");
    if (delm != line.npos)
      {
      // A field/description pair.
      vtkstd::string field = line.substr(0, delm);
      vtkstd::string description = line.substr(delm+2);
      if (field == "dimension")
        {
        numDimensions = atoi(description.c_str());
        }
      else if (field == "sizes")
        {
        dimSizes.resize(numDimensions);
        GetVector(description, dimSizes);
        }
      else if (field == "spacings")
        {
        dimSpacing.resize(numDimensions);
        GetVector(description, dimSpacing);
        }
      else if (field == "type")
        {
        this->DataScalarType = NrrdType2VTKType(description);
        if (this->DataScalarType == VTK_VOID) return 0;
        // The superclass does this, but I'm not sure it's necessary.
        this->GetOutput()->SetScalarType(this->DataScalarType);
        }
      else if (field == "encoding")
        {
        if (description != "raw")
          {
          vtkErrorMacro(<< "Unsupported encoding: " << description);
          return 0;
          }
        }
      else if ((field == "data file") || (field == "datafile"))
        {
        vtkstd::vector<vtkstd::string> filepatterninfo = split(description);
        if (filepatterninfo[0] == "LIST")
          {
          // After LIST there is an optional subdimension (see next case below).
          subDimension = (  (filepatterninfo.size() > 1)
                          ? atoi(filepatterninfo[1].c_str()) : numDimensions );
          
          // In this mode files are listed one per line to the end of the file.
          while (1)
            {
            getline(header, line);
            trim(line);
            if (line.length() < 1) break;
            this->DataFiles->InsertNextValue(line);
            }
          break;
          }
        else if (filepatterninfo.size() >=4)
          {
          // description should be "<format> <min> <max> <step> [<subdim>]"
          // where <format> is a string to be processed by sprintf and <min>,
          // <max>, and <step> form the numbers.  <subdim> defines on which
          // dimension the files are split up.
          vtkstd::string format = filepatterninfo[0];
          int min = atoi(filepatterninfo[1].c_str());
          int max = atoi(filepatterninfo[2].c_str());
          int step = atoi(filepatterninfo[3].c_str());
          subDimension = (  (filepatterninfo.size() > 4)
                          ? atoi(filepatterninfo[4].c_str()) : numDimensions );
          char *filename = new char[format.size() + 20];
          for (int i = min; i <= max; i += step)
            {
            sprintf(filename, format.c_str(), i);
            this->DataFiles->InsertNextValue(filename);
            }
          delete[] filename;
          }
        else
          {
          // description is simply a filename
          this->DataFiles->InsertNextValue(description);
          }
        }
      else if (field == "space")
        {
        // All spaces are either 3D or 3D with time.
        if (description.find("time") != description.npos)
          {
          vtkErrorMacro(<< "Time in NRRD array not supported (yet).");
          return 0;
          }
        if (   (description == "left-anterior-superior")
            || (description == "LAS")
            || (description == "3D-left-handed") )
          {
          this->FileLowerLeft = 0;
          }
        numDimensions = 3;
        }
      else if (field == "labels")
        {
        vtkstd::string dataname = description.substr(description.find("\"")+1);
        dataname = dataname.substr(0, dataname.find("\""));
        delete[] this->ScalarArrayName;
        this->ScalarArrayName = new char[dataname.size()+1];
        strcpy(this->ScalarArrayName, dataname.c_str());
        }
      else if (field == "space dimension")
        {
        numDimensions = atoi(description.c_str());
        }
      else if (field == "space origin")
        {
        vtkstd::vector<double> origin = ParseVector(description);
        for (size_t i = 0; (i < 3) && (i < origin.size()); i++)
          {
          this->DataOrigin[i] = origin[i];
          }
        }
      else if (field == "space directions")
        {
        vtkstd::vector<vtkstd::string> directions = split(description);
        dimSpacing.resize(0);
        for (size_t j = 0; j < directions.size(); j++)
          {
          if (directions[j] == "none")
            {
            dimSpacing.push_back(0);
            continue;
            }
          vtkstd::vector<double> dir = ParseVector(directions[j]);
          // We don't support orientation, but we do support spacing.
          double mag = 0.0;
          for (size_t k = 0; k < dir.size(); k++)
            {
            mag += dir[k]*dir[k];
            }
          dimSpacing.push_back(sqrt(mag));
          }
        }
      else if (field == "endian")
        {
#ifdef VTK_WORDS_BIGENDIAN
        this->SwapBytes = static_cast<int>(description == "little");
#else
        this->SwapBytes = static_cast<int>(description == "big");
#endif
        }
      else if ((field == "line skip") || (field == "lineskip"))
        {
        if (atoi(description.c_str()) != 0)
          {
          vtkErrorMacro(<< "line skip not supported");
          return 0;
          }
        }
      else if ((field == "byte skip") || (field == "byteskip"))
        {
        byteskip = atoi(description.c_str());
        }
      else if (   (field == "space units")
               || (field == "sample units") || (field == "sampleunits")
               || (field == "measurement frame")
               || (field == "block size") || (field == "blocksize")
               || (field == "content")
               || (field == "thicknesses")
               || (field == "axis mins") || (field == "axismins")
               || (field == "axis maxs") || (field == "axismaxs")
               || (field == "centers") || (field == "centerings")
               || (field == "units")
               || (field == "kinds")
               || (field == "min") || (field == "max")
               || (field == "old min") || (field == "oldmin")
               || (field == "old max") || (field == "oldmax")
               || (field == "number") )
        {
        // Ignored.
        }
      else
        {
        vtkWarningMacro(<< "Unknown field: '" << field << "'");
        }
      continue;
      }

    delm = line.find(":=");
    if (delm != line.npos)
      {
      // A key/value pair.
      continue;
      }
    }

  // NRRD does not distinguish between vector entries and dimensions.  For
  // example, RGB tuples are represented by adding a dimension of size 3.
  // VTK really needs to know the difference.  Here we are going to guess.
  // If the fastest changing dimension is 9 or less we consider that a tuple.
  // We will also consider any 4th dimension as a tuple.
  if (   (dimSizes.size() > 3)
      || ((dimSizes.size() > 0) && (dimSizes[0] <= 9))
      || ((dimSpacing.size() > 0) && (dimSpacing[0] == 0)) )
    {
    this->NumberOfScalarComponents = dimSizes[0];
    dimSizes.erase(dimSizes.begin());
    if (!dimSpacing.empty()) dimSpacing.erase(dimSpacing.begin());
    subDimension--;
    }
  else
    {
    this->NumberOfScalarComponents = 1;
    }

  // Record the dimensions.
  this->FileDimensionality = static_cast<int>(dimSizes.size());
  for (unsigned int i = 0; i < 3; i++)
    {
    this->DataExtent[i*2+0] = 0;
    this->DataExtent[i*2+1] = (i < dimSizes.size()) ? dimSizes[i]-1 : 0;
    this->DataSpacing[i] = (i < dimSpacing.size()) ? dimSpacing[i] : 1;
    }

  if (this->DataFiles->GetNumberOfValues() > 0)
    {
    if (this->DataFiles->GetNumberOfValues() > 1)
      {
      this->FileDimensionality--;
      if (this->FileDimensionality != 2)
        {
        vtkErrorMacro(<< "Data split into multiple files is only supported"
                      << " when each file is 2D (+ an optional vector"
                      << " dimension).");
        }
      if (subDimension != 3)
        {
        vtkErrorMacro(<< "Data split into multiple files is only supported"
                      << " when each file is 2D (+ an optional vector"
                      << " dimension).  This means the subdim must be on"
                      << " that third (or fourth in the case of a vector)"
                      << " dimension.");
        }
      }
    vtkstd::string parentDir
      = vtksys::SystemTools::GetParentDirectory(this->FileName);
    for (vtkIdType i = 0; i < this->DataFiles->GetNumberOfValues(); i++)
      {
      vtkstd::string relativePath = this->DataFiles->GetValue(i);
      vtkstd::string fullPath
        = vtksys::SystemTools::CollapseFullPath(relativePath.c_str(),
                                                parentDir.c_str());
      this->DataFiles->SetValue(i, fullPath);
      }
    }

  return 1;
}

//=============================================================================
int vtkPNrrdReader::RequestData(vtkInformation *request,
                                vtkInformationVector **inputVector,
                                vtkInformationVector *outputVector)
{
  // Get rid of superclasses FileNames.  We don't support that functionality,
  // but we exploit that of the superclass.
  if (this->FileNames != NULL)
    {
    this->FileNames->Delete();
    this->FileNames = NULL;
    }

  char *saveFileName = this->FileName;

  if (this->DataFiles->GetNumberOfValues() == 1)
    {
    this->FileName = const_cast<char *>(this->DataFiles->GetValue(0).c_str());
    }
  else if (this->DataFiles->GetNumberOfValues() > 1)
    {
    this->FileNames = this->DataFiles;
    }

  this->Superclass::RequestData(request, inputVector, outputVector);

  this->FileName = saveFileName;
  this->FileNames = NULL;

  return 1;
}
