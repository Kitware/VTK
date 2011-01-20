/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMNIObjectReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

Copyright (c) 2006 Atamai, Inc.

Use, modification and redistribution of the software, in source or
binary forms, are permitted provided that the following terms and
conditions are met:

1) Redistribution of the source code, in verbatim or modified
   form, must retain the above copyright notice, this license,
   the following disclaimer, and any notices that refer to this
   license and/or the following disclaimer.

2) Redistribution in binary form must include the above copyright
   notice, a copy of this license and the following disclaimer
   in the documentation or with other materials provided with the
   distribution.

3) Modified copies of the source code must be clearly marked as such,
   and must not be misrepresented as verbatim copies of the source code.

THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE SOFTWARE "AS IS"
WITHOUT EXPRESSED OR IMPLIED WARRANTY INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  IN NO EVENT SHALL ANY COPYRIGHT HOLDER OR OTHER PARTY WHO MAY
MODIFY AND/OR REDISTRIBUTE THE SOFTWARE UNDER THE TERMS OF THIS LICENSE
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA OR DATA BECOMING INACCURATE
OR LOSS OF PROFIT OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF
THE USE OR INABILITY TO USE THE SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.

=========================================================================*/

#include "vtkMNIObjectReader.h"

#include "vtkObjectFactory.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkProperty.h"
#include "vtkMath.h"

#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "vtkstd/string"
#include "vtkstd/vector"
#include "vtksys/SystemTools.hxx"

#ifndef VTK_BINARY
#define VTK_ASCII 1
#define VTK_BINARY 2
#endif

//--------------------------------------------------------------------------
vtkStandardNewMacro(vtkMNIObjectReader);

#define VTK_MNIOBJ_LINE_LENGTH 256

//-------------------------------------------------------------------------
vtkMNIObjectReader::vtkMNIObjectReader()
{
  this->SetNumberOfInputPorts(0);

  this->FileName = 0;
  this->Property = vtkProperty::New();

  // Whether file is binary or ASCII
  this->FileType = VTK_ASCII;

  // File line number for error reporting (ASCII only)
  this->LineNumber = 0;

  // State information for reading files
  this->InputStream = 0;
  this->LineText = new char[VTK_MNIOBJ_LINE_LENGTH];
  this->CharPointer = this->LineText;
}

//-------------------------------------------------------------------------
vtkMNIObjectReader::~vtkMNIObjectReader()
{
  if (this->Property)
    {
    this->Property->Delete();
    }
  if (this->FileName)
    {
    delete [] this->FileName;
    }
  if (this->LineText)
    {
    delete [] this->LineText;
    }
}

//-------------------------------------------------------------------------
void vtkMNIObjectReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FileName: "
     << (this->FileName ? this->FileName : "none") << "\n";
  os << indent << "Property: " << this->Property << "\n";
  if (this->Property)
    {
    this->Property->PrintSelf(os, indent.GetNextIndent());
    }
}

//-------------------------------------------------------------------------
int vtkMNIObjectReader::CanReadFile(const char* fname)
{
  // First make sure the file exists.  This prevents an empty file
  // from being created on older compilers.
  struct stat fs;
  if(stat(fname, &fs) != 0)
    {
    return 0;
    }

  // Try to read the first line of the file.
  int status = 0;

  ifstream infile(fname);

  if (infile.good())
    {
    int objType = infile.get();

    if (infile.good())
      {
      objType = toupper(objType);

      if (objType == 'P' || objType != 'L' ||
          objType == 'M' || objType != 'F' ||
          objType == 'X' || objType != 'Q' ||
          objType == 'T')
        {
        status = 1;
        }
      }

    infile.close();
    }

  return status;
}

//-------------------------------------------------------------------------
// Internal function to read in a line up to 256 characters and then
// skip to the next line in the file.
int vtkMNIObjectReader::ReadLine(char *line, unsigned int maxlen)
{
  this->LineNumber++;
  istream &infile = *this->InputStream;

  infile.getline(line, maxlen);
  this->CharPointer = line;

  if (infile.fail())
    {
    if (infile.eof())
      {
      return 0;
      }
    if (infile.gcount() == 255)
      {
      // Read 256 chars; ignoring the rest of the line.
      infile.clear();
      infile.ignore(VTK_INT_MAX, '\n');
      vtkWarningMacro("Overlength line (limit is 255) in "
                      << this->FileName << ":" << this->LineNumber);
      }
    }

  return 1;
}

//-------------------------------------------------------------------------
// Skip all whitespace, reading additional lines if necessary
int vtkMNIObjectReader::SkipWhitespace()
{
  if (this->FileType == VTK_BINARY)
    {
    return 1;
    }

  // Only skip whitespace in ASCII files
  do
    {
    char *cp = this->CharPointer;

    // Skip leading whitespace
    while (isspace(*cp))
      {
      cp++;
      }

    if (*cp != '\0')
      {
      this->CharPointer = cp;
      return 1;
      }
    }
  while (this->ReadLine(this->LineText, VTK_MNIOBJ_LINE_LENGTH));

  return 0;
}

//-------------------------------------------------------------------------
// Read floating-point values into a vtkFloatArray.
int vtkMNIObjectReader::ParseValues(vtkDataArray *array, vtkIdType n)
{
  int dataType = array->GetDataType();
  array->SetNumberOfTuples(n/array->GetNumberOfComponents());

  if (this->FileType == VTK_BINARY)
    {
    // The .obj files use native machine endianness
    this->InputStream->read((char *)array->GetVoidPointer(0),
                            n*array->GetDataTypeSize());

    // Switch ABGR to RGBA colors
    if (dataType == VTK_UNSIGNED_CHAR &&
        array->GetNumberOfComponents() == 4)
      {
      unsigned char *data = (unsigned char *)array->GetVoidPointer(0);
      for (vtkIdType i = 0; i < n; i += 4)
        {
        unsigned char abgr[4];
        abgr[0] = data[0];
        abgr[1] = data[1];
        abgr[2] = data[2];
        abgr[3] = data[3];
        data[0] = abgr[3];
        data[1] = abgr[2];
        data[2] = abgr[1];
        data[3] = abgr[0];

        data += 4;
        }
      }

    return !this->InputStream->fail();
    }

  // The rest of the code is for ASCII files
  for (vtkIdType i = 0; i < n; i++)
    {
    if (!this->SkipWhitespace())
      {
      vtkErrorMacro("Unexpected end of file " << this->FileName
                    << ":" << this->LineNumber);
      return 0;
      }

    char *cp = this->CharPointer;

    switch (dataType)
      {
      case VTK_FLOAT:
        {
        double val = strtod(cp, &cp);
        static_cast<vtkFloatArray *>(array)->SetValue(i, val);
        }
        break;
      case VTK_INT:
        {
        unsigned long lval = strtoul(cp, &cp, 10);
        if (lval > static_cast<unsigned long>(VTK_INT_MAX))
          {
          vtkErrorMacro("Value " << lval << " is too large for int "
                        << this->FileName << ":" << this->LineNumber);
          return 0;
          }
        int val = static_cast<int>(lval);
        static_cast<vtkIntArray *>(array)->SetValue(i, val);
        }
        break;
      case VTK_UNSIGNED_CHAR:
        {
        double dval = strtod(cp, &cp);
        if (dval < 0.0 || dval > 1.0)
          {
          vtkErrorMacro("Color value must be [0..1] "
                        << this->FileName << ":" << this->LineNumber);
          return 0;
          }
        unsigned char val = static_cast<unsigned char>(dval*255.0);
        static_cast<vtkUnsignedCharArray *>(array)->SetValue(i, val);
        }
        break;
      }

    // If nothing was read, there was a syntax error
    if (cp == this->CharPointer)
      {
      vtkErrorMacro("Syntax error " << this->FileName
                    << ":" << this->LineNumber);
      return 0;
      }

    this->CharPointer = cp;
    }

  return 1;
}

//-------------------------------------------------------------------------
// Read an integer value
int vtkMNIObjectReader::ParseIdValue(vtkIdType *value)
{
  if (this->FileType == VTK_BINARY)
    {
    int val;
    this->InputStream->read((char *)(&val), sizeof(int));
    *value = val;

    return !this->InputStream->fail();
    }

  // The rest of the code is for ASCII files
  if (!this->SkipWhitespace())
    {
    vtkErrorMacro("Unexpected end of file " << this->FileName
                  << ":" << this->LineNumber);
    return 0;
    }

  char *cp = this->CharPointer;

  long lval = strtol(cp, &cp, 10);
  if (lval > static_cast<long>(VTK_INT_MAX) ||
      lval < static_cast<long>(VTK_INT_MIN))
    {
    vtkErrorMacro("Value " << lval << " is too large for int "
                  << this->FileName << ":" << this->LineNumber);
    return 0;
    }

  *value = static_cast<int>(lval);

  // If no bytes were read, that means there was a syntax error
  if (cp == this->CharPointer)
    {
    vtkErrorMacro("Syntax error " << this->FileName
                  << ":" << this->LineNumber);
    return 0;
    }

  this->CharPointer = cp;

  return 1;
}

//-------------------------------------------------------------------------
int vtkMNIObjectReader::ReadProperty(vtkProperty *property)
{
  vtkFloatArray *tmpArray = vtkFloatArray::New();

  int status = this->ParseValues(tmpArray, 5);

  if (status != 0)
    {
    property->SetAmbient(tmpArray->GetValue(0));
    property->SetDiffuse(tmpArray->GetValue(1));
    property->SetSpecular(tmpArray->GetValue(2));
    property->SetSpecularPower(tmpArray->GetValue(3));
    property->SetOpacity(tmpArray->GetValue(4));
    }

  tmpArray->Delete();

  return status;
}

//-------------------------------------------------------------------------
int vtkMNIObjectReader::ReadLineThickness(vtkProperty *property)
{
  vtkFloatArray *tmpArray = vtkFloatArray::New();

  int status = this->ParseValues(tmpArray, 1);

  if (status != 0)
    {
    property->SetLineWidth(tmpArray->GetValue(0));
    }

  tmpArray->Delete();

  return status;
}

//-------------------------------------------------------------------------
int vtkMNIObjectReader::ReadNumberOfPoints(vtkIdType *numPoints)
{
  int status = this->ParseIdValue(numPoints);

  if (status != 0)
    {
    if (*numPoints < 0)
      {
      // Don't support "compressed" data yet
      vtkErrorMacro("Bad number of points -> " << *numPoints << " "
                    << this->FileName << ":" << this->LineNumber);
      status = 0;
      }
    else if (*numPoints > VTK_LARGE_ID/4)
      {
      vtkErrorMacro("Too many points -> " << *numPoints << " "
                    << this->FileName << ":" << this->LineNumber);
      status = 0;
      }
    }

  return status;
}
//-------------------------------------------------------------------------
int vtkMNIObjectReader::ReadNumberOfCells(vtkIdType *numCells)
{
  int status = this->ParseIdValue(numCells);

  if (status != 0)
    {
    if (*numCells < 0)
      {
      vtkErrorMacro("Bad number of cells -> " << *numCells << " "
                    << this->FileName << ":" << this->LineNumber);
      status = 0;
      }
    else if (*numCells > VTK_LARGE_ID/4)
      {
      vtkErrorMacro("Too many cells -> " << *numCells << " "
                    << this->FileName << ":" << this->LineNumber);
      status = 0;
      }
    }

  return status;
}

//-------------------------------------------------------------------------
int vtkMNIObjectReader::ReadPoints(vtkPolyData *data, vtkIdType numPoints)
{
  vtkPoints *points = vtkPoints::New();
  int status = this->ParseValues(points->GetData(), 3*numPoints);

  if (status != 0)
    {
    data->SetPoints(points);
    }

  points->Delete();

  return status;
}

//-------------------------------------------------------------------------
int vtkMNIObjectReader::ReadNormals(vtkPolyData *data, vtkIdType numPoints)
{
  vtkFloatArray *normals = vtkFloatArray::New();
  normals->SetNumberOfComponents(3);
  int status = this->ParseValues(normals, 3*numPoints);

  if (status != 0)
    {
    data->GetPointData()->SetNormals(normals);
    }

  normals->Delete();

  return status;
}

//-------------------------------------------------------------------------
int vtkMNIObjectReader::ReadColors(vtkProperty *property,
                                   vtkPolyData *data, vtkIdType numPoints,
                                   vtkIdType numCells)
{
  // Find out what kind of coloring is used
  vtkIdType colorType = 0;
  if (this->ParseIdValue(&colorType) == 0)
    {
    return 0;
    }

  // Set the number of colors
  vtkIdType numColors = 1;
  if (colorType == 1)
    {
    numColors = numCells;
    }
  else if (colorType == 2)
    {
    numColors = numPoints;
    }
  else if (colorType != 0)
    {
    vtkErrorMacro("Color number must be 0, 1 or 2 " << this->FileName
                  << ":" << this->LineNumber);
    return 0;
    }

  // Read the colors
  vtkUnsignedCharArray *colors = vtkUnsignedCharArray::New();
  colors->SetName("Colors");
  colors->SetNumberOfComponents(4);
  int status = this->ParseValues(colors, 4*numColors);

  if (status != 0)
    {
    if (colorType == 0)
      {
      data->GetCellData()->SetScalars(0);
      data->GetPointData()->SetScalars(0);
      property->SetColor(colors->GetValue(0)/255.0,
                               colors->GetValue(1)/255.0,
                               colors->GetValue(2)/255.0);
      }
    else if (colorType == 1)
      {
      data->GetPointData()->SetScalars(0);
      data->GetCellData()->SetScalars(colors);
      property->SetColor(1.0, 1.0, 1.0);
      }
    else if (colorType == 2)
      {
      data->GetCellData()->SetScalars(0);
      data->GetPointData()->SetScalars(colors);
      property->SetColor(1.0, 1.0, 1.0);
      }
    }

  colors->Delete();

  return status;
}

//-------------------------------------------------------------------------
int vtkMNIObjectReader::ReadCells(vtkPolyData *data, vtkIdType numCells,
                                  int cellType)
{
  vtkIntArray *endIndices = vtkIntArray::New();
  vtkIntArray *cellIndices = vtkIntArray::New();
  vtkCellArray *cellArray = vtkCellArray::New();

  // Read the cell end indices
  int status = this->ParseValues(endIndices, numCells);

  // Read the cell point indices
  vtkIdType numIndices = 0;
  if (status != 0)
    {
    if (numCells > 0)
      {
      numIndices = endIndices->GetValue(numCells - 1);
      }
    status = this->ParseValues(cellIndices, numIndices);
    }

  // Create the cell array
  if (status != 0)
    {
    cellArray->GetData()->Allocate(
      numCells + endIndices->GetValue(numCells - 1));

    vtkPoints *points = data->GetPoints();
    vtkIdType numPoints = points->GetNumberOfPoints();
    vtkIdType lastEndIndex = 0;
    for (vtkIdType i = 0; i < numCells; i++)
      {
      vtkIdType endIndex = endIndices->GetValue(i);
      numIndices = endIndex - lastEndIndex;

      cellArray->InsertNextCell(numIndices);

      // Check that the index values are okay and create the cell
      for (vtkIdType j = 0; j < numIndices; j++)
        {
        vtkIdType idx = cellIndices->GetValue(lastEndIndex + j);
        if (idx > numPoints)
          {
          vtkErrorMacro("Index " << idx << " is greater than the"
                        << " total number of points " << numPoints << " "
                        << this->FileName);
          return 0;
          }
        cellArray->InsertCellPoint(idx);
        }

      lastEndIndex = endIndex;
      }

    if (cellType == VTK_POLYGON)
      {
      data->SetPolys(cellArray);
      }
    else if (cellType == VTK_POLY_LINE)
      {
      data->SetLines(cellArray);
      }
    }

  endIndices->Delete();
  cellIndices->Delete();
  cellArray->Delete();

  return status;
}

//-------------------------------------------------------------------------
int vtkMNIObjectReader::ReadPolygonObject(vtkPolyData *output)
{
  // Read the surface property
  if (this->ReadProperty(this->Property) == 0)
    {
    return 0;
    }

  // Read the number of points
  vtkIdType numPoints = 0;
  if (this->ReadNumberOfPoints(&numPoints) == 0)
    {
    return 0;
    }

  // Read the points
  if (this->ReadPoints(output, numPoints) == 0)
    {
    return 0;
    }

  // Read the normals
  if (this->ReadNormals(output, numPoints) == 0)
    {
    return 0;
    }

  // Read the number of items
  vtkIdType numCells = 0;
  if (this->ReadNumberOfCells(&numCells) == 0)
    {
    return 0;
    }

  // Read the colors
  if (this->ReadColors(this->Property, output, numPoints, numCells) == 0)
    {
    return 0;
    }

  // Read the cells
  if (this->ReadCells(output, numCells, VTK_POLYGON) == 0)
    {
    return 0;
    }

  return 1;
}

//-------------------------------------------------------------------------
int vtkMNIObjectReader::ReadLineObject(vtkPolyData *output)
{
  // Read the line thickness
  if (this->ReadLineThickness(this->Property) == 0)
    {
    return 0;
    }

  // Read the number of points
  vtkIdType numPoints = 0;
  if (this->ReadNumberOfPoints(&numPoints) == 0)
    {
    return 0;
    }

  // Read the points
  if (this->ReadPoints(output, numPoints) == 0)
    {
    return 0;
    }

  // Read the number of items
  vtkIdType numCells = 0;
  if (this->ReadNumberOfCells(&numCells) == 0)
    {
    return 0;
    }

  // Read the colors
  if (this->ReadColors(this->Property, output, numPoints, numCells) == 0)
    {
    return 0;
    }

  // Read the cells
  if (this->ReadCells(output, numCells, VTK_POLY_LINE) == 0)
    {
    return 0;
    }

  return 1;
}

//-------------------------------------------------------------------------
int vtkMNIObjectReader::ReadFile(vtkPolyData *output)
{
  // Initialize the property to default values
  vtkProperty *property = vtkProperty::New();
  this->Property->DeepCopy(property);
  property->Delete();

  // Check that the file name has been set.
  if (!this->FileName)
    {
    vtkErrorMacro("ReadFile: No file name has been set");
    return 0;
    }

  // Make sure that the file exists.
  struct stat fs;
  if(stat(this->FileName, &fs) != 0)
    {
    vtkErrorMacro("ReadFile: Can't open file " << this->FileName);
    return 0;
    }

  // Make sure that the file is readable.
  ifstream infile(this->FileName, ios::in);

  if (infile.fail())
    {
    vtkErrorMacro("ReadFile: Can't read the file " << this->FileName);
    return 0;
    }

  // Check object type
  int objType = infile.get();
  int fileType = VTK_ASCII;

  if (infile.fail())
    {
    vtkErrorMacro("ReadFile: I/O error for file " << this->FileName);
    infile.close();
    return 0;
    }

  if (islower(objType))
    {
    objType = toupper(objType);
    fileType = VTK_BINARY;
    }

  if (objType != 'P' && objType != 'L' &&
      objType != 'M' && objType != 'F' &&
      objType != 'X' && objType != 'Q' &&
      objType != 'T' && objType != 'V')
    {
    vtkErrorMacro("ReadFile: File is not a MNI obj file: "
                  << this->FileName);
    infile.close();
    return 0;
    }

#ifdef _WIN32
  // Re-open file as binary (only necessary on Windows)
  if (fileType == VTK_BINARY)
    {
    infile.close();
    infile.open(this->FileName, ios::in | ios::binary);
    infile.get();
    }
#endif

  this->InputStream = &infile;
  this->LineNumber = 0;
  this->FileType = fileType;

  int status = 1;

  if (this->FileType == VTK_ASCII)
    {
    // Read the line, include the type char in line text for
    // use in error reporting
    this->LineText[0] = objType;
    status = this->ReadLine(&this->LineText[1], VTK_MNIOBJ_LINE_LENGTH-1);
    }

  if (status != 0)
    {
    switch (objType)
      {
      case 'P':
        status = this->ReadPolygonObject(output);
        break;
      case 'L':
        status = this->ReadLineObject(output);
        break;
      case 'M':
      case 'F':
      case 'X':
      case 'Q':
      case 'T':
      case 'V':
        {
        vtkErrorMacro("ReadFile: Reading of obj type \"" << (char)objType <<
                      "\" is not supported: " << this->FileName);
        status = 0;
        }
        break;
      }
    }

  if (this->FileType == VTK_BINARY)
    {
    if (infile.fail())
      {
      if (infile.eof())
        {
        vtkErrorMacro("Premature end of binary file " << this->FileName);
        }
      else
        {
        vtkErrorMacro("Error encountered while reading " << this->FileName);
        }
      }
    }

  this->InputStream = 0;
  infile.close();

  return status;
}

//-------------------------------------------------------------------------
int vtkMNIObjectReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // all of the data in the first piece.
  if (outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER())
      > 0)
    {
    return 0;
    }

  // read the file
  return this->ReadFile(output);
}
