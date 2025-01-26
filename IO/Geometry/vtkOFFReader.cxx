// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOFFReader.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFileResourceStream.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkResourceParser.h"
#include "vtkStringArray.h"

#include <iostream>
#include <sstream>
#include <string>

namespace
{
// helper function to trim strings of whitespaces
inline void trim(std::string& str)
{
  std::size_t p = str.find_first_not_of(" \a\b\f\n\r\t\v");
  if (p == str.npos)
    str.clear();
  else
  {
    std::size_t q = str.find_last_not_of(" \a\b\f\n\r\t\v");
    if (++q < str.length())
      str.erase(q);
    if (p > std::size_t(0))
      str.erase(0u, p);
  }
}
}

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOFFReader);

//------------------------------------------------------------------------------
vtkSmartPointer<vtkResourceStream> vtkOFFReader::Open()
{
  if (this->Stream)
  {
    if (this->Stream->SupportSeek())
    {
      this->Stream->Seek(0, vtkResourceStream::SeekDirection::Begin);
    }

    return this->Stream;
  }

  auto fileStream = vtkSmartPointer<vtkFileResourceStream>::New();
  if (!this->FileName || !fileStream->Open(this->FileName))
  {
    vtkErrorMacro(<< "Failed to open file: "
                  << (this->FileName ? this->FileName : "No file name set"));
    return nullptr;
  }

  return fileStream;
}

//------------------------------------------------------------------------------
int vtkOFFReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkSmartPointer<vtkResourceStream> stream = this->Open();
  if (!stream)
  {
    vtkErrorMacro(<< "Failed to open stream");
    return 0;
  }

  vtkNew<vtkResourceParser> parser;
  parser->SetStream(stream);
  parser->StopOnNewLineOn();

  int lineNumber = 0;

  // get the first line; it must be equal to "OFF"
  std::string sline;
  if (parser->ReadLine(sline) != vtkParseResult::EndOfLine)
  {
    vtkErrorMacro(<< "Failed to read first line of file!");
    return 0;
  }
  if (sline != "OFF")
  {
    vtkErrorMacro(
      << "File does not seem to be a valid OFF file; expected file to begin with \"OFF\\n\"");
    return 0;
  }
  ++lineNumber;

  // try to parse counts from next line; we'll do this in a loop
  // since there may be empty or comment lines before the counts
  vtkIdType numPoints = 0, numPolys = 0;
  while (true)
  {
    // read next line and trim it
    ++lineNumber;
    if (parser->ReadLine(sline) != vtkParseResult::EndOfLine)
    {
      vtkErrorMacro(<< "Failed to read line " << lineNumber);
      return 0;
    }
    ::trim(sline);

    // skip empty lines and comments
    if (sline.empty() || (sline.front() == '#'))
    {
      continue;
    }

    // try to parse number of points and polygons
    std::istringstream iss(sline);
    if ((iss >> numPoints).fail())
    {
      vtkErrorMacro(<< "Failed to read number of points in line " << lineNumber);
      return 0;
    }
    if (numPoints <= 0)
    {
      vtkErrorMacro(<< "File contains 0 points according to  line " << lineNumber);
      return 0;
    }
    if ((iss >> numPolys).fail())
    {
      vtkErrorMacro(<< "Failed to read number of polygons in line " << lineNumber);
      return 0;
    }
    if (numPolys <= 0)
    {
      vtkErrorMacro(<< "File contains 0 polygons according to line " << lineNumber);
      return 0;
    }

    // we have successfully parsed the number of points and polygons,
    // so we can exit this loop now
    break;
  }

  // allocate points
  auto points = vtkSmartPointer<vtkPoints>::New();
  points->Allocate(numPoints);
  points->SetDataTypeToDouble();

  // allocate polygons
  auto polys = vtkSmartPointer<vtkCellArray>::New();
  polys->Allocate(numPolys);

  // scaling factor for the progress bar
  double progressScale = 1.0 / double(numPoints + numPoints);

  // now let's try to parse the point coordinates
  vtkIdType donePoints = 0;
  while (donePoints < numPoints)
  {
    // read next line and trim it
    ++lineNumber;
    if (parser->ReadLine(sline) != vtkParseResult::EndOfLine)
    {
      vtkErrorMacro(<< "Failed to read line " << lineNumber);
      return 0;
    }
    ::trim(sline);

    // skip empty lines and comments
    if (sline.empty() || (sline.front() == '#'))
    {
      continue;
    }

    // try to parse point coordinates
    std::istringstream iss(sline);
    double v[3];
    if ((iss >> v[0] >> v[1] >> v[2]).fail())
    {
      vtkErrorMacro(<< "Failed to parse point coordinates at line " << lineNumber);
      return 0;
    }

    // point parsed, so add it to the list
    points->InsertNextPoint(v);
    ++donePoints;
    this->UpdateProgress(double(donePoints) * progressScale);
  }

  // now let's try to parse the polygons point indices
  vtkIdType donePolys = 0;
  std::vector<vtkIdType> vidx(100u); // max. 100 points per face
  while (donePolys < numPolys)
  {
    // read next line and trim it
    ++lineNumber;
    if (parser->ReadLine(sline) != vtkParseResult::EndOfLine)
    {
      vtkErrorMacro(<< "Failed to read line " << lineNumber);
      return 0;
    }
    ::trim(sline);

    // skip empty lines and comments
    if (sline.empty() || (sline.front() == '#'))
    {
      continue;
    }

    // try to parse point index count
    std::istringstream iss(sline);
    vtkIdType numIdx = 0;
    if ((iss >> numIdx).fail())
    {
      vtkErrorMacro(<< "Failed to parse face point count at line " << lineNumber);
      return 0;
    }
    // not sure whether 1 or 2 make sense at all...
    if (numIdx < 1)
    {
      vtkErrorMacro(<< "Failed to parse face point count at line " << lineNumber);
      return 0;
    }
    // if the parsed count is > 100, then the file is probably corrupt...
    if (numIdx > vtkIdType(vidx.size()))
    {
      vtkErrorMacro(<< "Face point count at line " << lineNumber
                    << " exceeds maximum allowed count of " << vidx.size());
      return 0;
    }

    // try to parse all points indices
    for (vtkIdType i = 0; i < numIdx; ++i)
    {
      if ((iss >> vidx[i]).fail())
      {
        vtkErrorMacro(<< "Failed to parse " << i << "th point index at line " << lineNumber);
        return 0;
      }
      // check for out-of-bounds
      if ((vidx[i] < 0) || (vidx[i] >= numPoints))
      {
        vtkErrorMacro(<< "Invalid point index " << vidx[i] << " at line " << lineNumber);
        return 0;
      }
    }

    // polygon parsed, so add it to the list
    polys->InsertNextCell(numIdx, vidx.data());
    ++donePolys;
    this->UpdateProgress(double(donePoints + donePolys) * progressScale);
  }

  // Fill output
  output->SetPoints(points);
  output->SetPolys(polys);
  output->Squeeze();

  return 1;
}

VTK_ABI_NAMESPACE_END
