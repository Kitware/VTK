/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMNITagPointReader.cxx

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

#include "vtkMNITagPointReader.h"

#include "vtkObjectFactory.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkStringArray.h"

#include <cctype>

#include <string>
#include <vector>
#include <vtksys/SystemTools.hxx>

//--------------------------------------------------------------------------
vtkStandardNewMacro(vtkMNITagPointReader);

//-------------------------------------------------------------------------
vtkMNITagPointReader::vtkMNITagPointReader()
{
  this->FileName = 0;
  this->NumberOfVolumes = 1;
  this->LineNumber = 0;
  this->Comments = 0;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(2);
}

//-------------------------------------------------------------------------
vtkMNITagPointReader::~vtkMNITagPointReader()
{
  delete [] this->FileName;
  delete [] this->Comments;
}

//-------------------------------------------------------------------------
void vtkMNITagPointReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FileName: "
     << (this->FileName ? this->FileName : "none") << "\n";
  os << indent << "NumberOfVolumes: "
     << this->NumberOfVolumes << "\n";
  os << indent << "Comments: "
     << (this->Comments ? this->Comments : "none") << "\n";
}

//-------------------------------------------------------------------------
int vtkMNITagPointReader::CanReadFile(const char* fname)
{
  // First make sure the file exists.  This prevents an empty file
  // from being created on older compilers.
  vtksys::SystemTools::Stat_t fs;
  if (vtksys::SystemTools::Stat(fname, &fs) != 0)
  {
    return 0;
  }

  // Try to read the first line of the file.
  int status = 0;

  ifstream infile(fname);

  if (infile.good())
  {
    status = 1;
    char linetext[256];
    infile.getline(linetext, 256);
    if (strncmp(linetext, "MNI Tag Point File", 18) != 0)
    {
      status = 0;
    }

    infile.close();
  }

  return status;
}

//-------------------------------------------------------------------------
// Internal function to read in a line up to 256 characters and then
// skip to the next line in the file.
int vtkMNITagPointReader::ReadLine(
  istream &infile, std::string &linetext, std::string::iterator &pos)
{
  this->LineNumber++;

  std::getline(infile, linetext);
  pos = linetext.begin();

  if (infile.fail())
  {
    if (infile.eof())
    {
      return 0;
    }
  }

  return 1;
}

//-------------------------------------------------------------------------
// Skip all blank lines or comment lines and return the first useful line
int vtkMNITagPointReader::ReadLineAfterComments(
  istream &infile, std::string &linetext, std::string::iterator &pos)
{
  // Skip over any comment lines or blank lines.
  // Comment lines start with '%'
  std::string comments;
  do
  {
    this->ReadLine(infile, linetext, pos);
    while (pos != linetext.end() && isspace(*pos))
    {
      ++pos;
    }
    if (linetext.length() != 0 && linetext[0] == '%')
    {
      if (comments.length() > 0)
      {
        comments.push_back('\n');
      }
      if (linetext.length())
      {
        comments.append(linetext);
      }
    }
    else if (linetext.length() != 0 && pos != linetext.end())
    {
      delete [] this->Comments;
      this->Comments = new char[comments.length() + 1];
      strncpy(this->Comments, comments.c_str(), comments.length());
      this->Comments[comments.length()] = '\0';

      return 1;
    }
  }
  while (infile.good());

  return 0;
}

//-------------------------------------------------------------------------
// Skip all whitespace, reading additional lines if necessary if nl != 0
int vtkMNITagPointReader::SkipWhitespace(
  istream &infile, std::string &linetext, std::string::iterator &pos,
  int nl)
{
  while (infile.good())
  {
    // Skip leading whitespace
    while (pos != linetext.end() && isspace(*pos))
    {
      ++pos;
    }

    if (pos != linetext.end())
    {
      return 1;
    }

    if (nl == 0)
    {
      break;
    }

    this->ReadLine(infile, linetext, pos);
  }

  return 0;
}

//-------------------------------------------------------------------------
// Read the left hand side of a statement, including the equals sign
// and any whitespace following the equals.
int vtkMNITagPointReader::ParseLeftHandSide(
  istream &infile, std::string &linetext, std::string::iterator &pos,
  std::string &identifier)
{
  identifier.clear();

  // Read alphanumeric plus underscore
  if (pos != linetext.end() && !isdigit(*pos))
  {
    while (pos != linetext.end() && (isalnum(*pos) || *pos == '_'))
    {
      identifier.push_back(*pos);
      ++pos;
    }
  }

  // Check for equals
  this->SkipWhitespace(infile, linetext, pos, 1);
  if (pos == linetext.end() || *pos != '=')
  {
    return 0;
  }

  // Eat the equals
  ++pos;

  // Skip ahead to the value part of the statement
  this->SkipWhitespace(infile, linetext, pos, 1);

  return 1;
}

//-------------------------------------------------------------------------
// Read a string value.  The terminating semicolon will be read, but
// won't be included in the output string.  Neither will any
// whitespace occurring before the semicolon. The string may not be
// split across multiple lines.
int vtkMNITagPointReader::ParseStringValue(
  istream &infile, std::string &linetext, std::string::iterator &pos,
  std::string &data)
{
  this->SkipWhitespace(infile, linetext, pos, 0);

  if (pos != linetext.end() && *pos == '\"')
  {
    // eat the opening quote
    ++pos;

    // read the string
    while (pos != linetext.end() && *pos != '\"')
    {
      char c = *pos;
      ++pos;
      if (c == '\\')
      {
        if (pos != linetext.end())
        {
          c = '\0';
          static char ctrltable[] = {
            '\a', 'a', '\b', 'b', '\f', 'f', '\n', 'n', '\r', 'r',
            '\t', 't', '\v', 'v', '\\', '\\', '\"', '\"', '\0', '\0'
            };

          if (*pos >= 0 && *pos <= 9)
          {
            for (int j = 0; j < 3 && pos != linetext.end() &&
                 *pos >= 0 && *pos <= 9; ++j, ++pos)
            {
              c = ((c << 3) | (*pos - '0'));
            }
          }
          else if (*pos == 'x')
          {
            ++pos;
            for (int j = 0; j < 2 && pos != linetext.end() &&
                 isalnum(*pos); ++j, ++pos)
            {
              char x = tolower(*pos);
              if (x >= '0' && x <= '9')
              {
                c = ((c << 4) | (x - '0'));
              }
              else if (x >= 'a' && x <= 'f')
              {
                c = ((c << 4) | (x - 'a' + 10));
              }
            }
          }
          else
          {
            for (int ci = 0; ctrltable[ci] != '\0'; ci += 2)
            {
              if (*pos == ctrltable[ci+1])
              {
                c = ctrltable[ci];
                break;
              }
            }
            if (c == '\0')
            {
              c = *pos;
            }
            ++pos;
          }
        }
      }

      data.push_back(c);
    }
  }

  if (pos == linetext.end())
  {
    vtkErrorMacro("Syntax error " << this->FileName
                  << ":" << this->LineNumber);
    return 0;
  }

  // eat the trailing quote
  ++pos;

  return 1;
}

//-------------------------------------------------------------------------
// Read an int value
int vtkMNITagPointReader::ParseIntValues(
  istream &infile, std::string &linetext, std::string::iterator &pos,
  int *values, int n)
{
  this->SkipWhitespace(infile, linetext, pos, 0);

  int i = 0;
  while (pos != linetext.end() && *pos != ';' && i < n)
  {
    const char *cp = linetext.c_str() + (pos - linetext.begin());
    char *ep = 0;
    long val = strtol(cp, &ep, 10);
    if (ep == cp)
    {
      vtkErrorMacro("Syntax error " << this->FileName
                    << ":" << this->LineNumber);
      return 0;
    }
    pos += (ep - cp);
    values[i++] = static_cast<int>(val);
    this->SkipWhitespace(infile, linetext, pos, 0);
  }

  if (i != n)
  {
    vtkErrorMacro("Not enough values: " << this->FileName
                  << ":" << this->LineNumber);
    return 0;
  }

  return 1;
}


//-------------------------------------------------------------------------
// Read floating-point values into a point triplet.
int vtkMNITagPointReader::ParseFloatValues(
  istream &infile, std::string &linetext, std::string::iterator &pos,
  double *values, int n)
{
  this->SkipWhitespace(infile, linetext, pos, 0);

  int i = 0;
  while (pos != linetext.end() && *pos != ';' && i < n)
  {
    const char *cp = linetext.c_str() + (pos - linetext.begin());
    char *ep = 0;
    double val = strtod(cp, &ep);
    if (ep == cp)
    {
      vtkErrorMacro("Syntax error " << this->FileName
                    << ":" << this->LineNumber);
      return 0;
    }
    pos += (ep - cp);
    values[i++] = val;
    this->SkipWhitespace(infile, linetext, pos, 0);
  }

  if (i != n)
  {
    vtkErrorMacro("Not enough values: " << this->FileName
                  << ":" << this->LineNumber);
    return 0;
  }

  return 1;
}

//-------------------------------------------------------------------------
int vtkMNITagPointReader::ReadFile(
  vtkPolyData *output1, vtkPolyData *output2)
{
  // Check that the file name has been set.
  if (!this->FileName)
  {
    vtkErrorMacro("ReadFile: No file name has been set");
    return 0;
  }

  // Make sure that the file exists.
  vtksys::SystemTools::Stat_t fs;
  if (vtksys::SystemTools::Stat(this->FileName, &fs) != 0)
  {
    vtkErrorMacro("ReadFile: Can't open file " << this->FileName);
    return 0;
  }

  // Make sure that the file is readable.
  ifstream infile(this->FileName);
  std::string linetext;
  std::string::iterator pos = linetext.begin();

  if (infile.fail())
  {
    vtkErrorMacro("ReadFile: Can't read the file " << this->FileName);
    return 0;
  }

  // Read the first line
  this->LineNumber = 0;
  this->ReadLine(infile, linetext, pos);
  if (strncmp(linetext.c_str(), "MNI Tag Point File", 18) != 0)
  {
    vtkErrorMacro("ReadFile: File is not a MNI tag file: " << this->FileName);
    infile.close();
    return 0;
  }

  // Read the number of volumes
  this->ReadLine(infile, linetext, pos);
  this->SkipWhitespace(infile, linetext, pos, 1);
  int numVolumes = 1;
  std::string identifier;
  if (!this->ParseLeftHandSide(infile, linetext, pos, identifier) ||
      strcmp(identifier.c_str(), "Volumes") != 0 ||
      !this->ParseIntValues(infile, linetext, pos, &numVolumes, 1) ||
      (numVolumes != 1 && numVolumes != 2) ||
      !this->SkipWhitespace(infile, linetext, pos, 0) ||
      *pos != ';')
  {
    vtkErrorMacro("ReadFile: Line must be Volumes = 1; or Volumes = 2; "
                  << this->FileName << ":" << this->LineNumber);
    infile.close();
    return 0;
  }

  this->NumberOfVolumes = numVolumes;

  // Read the comments
  this->ReadLineAfterComments(infile, linetext, pos);

  // Rad the tag points
  if (!this->ParseLeftHandSide(infile, linetext, pos, identifier) ||
      strcmp(identifier.c_str(), "Points") != 0)
  {
    vtkErrorMacro("ReadFile: Cannot find Points in file; " << this->FileName);
    infile.close();
    linetext.clear();
    return 0;
  }

  vtkPoints *points[2];
  points[0] = vtkPoints::New();
  points[1] = vtkPoints::New();
  vtkCellArray *verts = vtkCellArray::New();
  vtkStringArray *labels = vtkStringArray::New();
  vtkDoubleArray *weights = vtkDoubleArray::New();
  vtkIntArray *structureIds = vtkIntArray::New();
  vtkIntArray *patientIds = vtkIntArray::New();

  int errorOccurred = 0;
  this->SkipWhitespace(infile, linetext, pos, 1);
  for (vtkIdType count = 0; infile.good() && *pos != ';'; count++)
  {
    for (int i = 0; i < numVolumes; i++)
    {
      double point[3];
      if (!this->ParseFloatValues(infile, linetext, pos, point, 3))
      {
        errorOccurred = 1;
        break;
      }
      points[i]->InsertNextPoint(point);
      verts->InsertNextCell(1);
      verts->InsertCellPoint(count);
    }
    if (errorOccurred)
    {
      break;
    }

    this->SkipWhitespace(infile, linetext, pos, 0);
    if (pos != linetext.end() && *pos != '\"' && *pos != ';')
    {
      double weight;
      int structureId;
      int patientId;
      if (!this->ParseFloatValues(infile, linetext, pos, &weight, 1) ||
          !this->ParseIntValues(infile, linetext, pos, &structureId, 1) ||
          !this->ParseIntValues(infile, linetext, pos, &patientId, 1))
      {
        errorOccurred = 1;
        break;
      }
      vtkIdType lastCount = weights->GetNumberOfTuples();
      weights->InsertValue(count, weight);
      structureIds->InsertValue(count, structureId);
      patientIds->InsertValue(count, patientId);
      for (vtkIdType j = lastCount; j < count; j++)
      {
        weights->SetValue(j, 0.0);
        structureIds->SetValue(j, -1);
        patientIds->SetValue(j, -1);
      }
    }

    this->SkipWhitespace(infile, linetext, pos, 0);
    if (pos != linetext.end() && *pos == '\"')
    {
      vtkStdString stringval;
      if (!this->ParseStringValue(infile, linetext, pos, stringval))
      {
        errorOccurred = 1;
        break;
      }
      labels->InsertValue(count, stringval);
    }

    this->SkipWhitespace(infile, linetext, pos, 1);
  }

  // Close the file
  infile.close();

  if (!errorOccurred)
  {
    output1->SetPoints(points[0]);
    output2->SetPoints(points[1]);

    vtkPolyData *output[2];
    output[0] = output1;
    output[1] = output2;

    weights->SetName("Weights");
    structureIds->SetName("StructureIds");
    patientIds->SetName("PatientIds");
    labels->SetName("LabelText");

    for (int k = 0; k < this->NumberOfVolumes; k++)
    {
      output[k]->SetVerts(verts);

      if (weights->GetNumberOfTuples())
      {
        output[k]->GetPointData()->AddArray(weights);
      }
      if (structureIds->GetNumberOfTuples())
      {
        output[k]->GetPointData()->AddArray(structureIds);
      }
      if (patientIds->GetNumberOfTuples())
      {
        output[k]->GetPointData()->AddArray(patientIds);
      }
      if (labels->GetNumberOfValues())
      {
        output[k]->GetPointData()->AddArray(labels);
      }
    }
  }

  points[0]->Delete();
  points[1]->Delete();

  verts->Delete();
  weights->Delete();
  structureIds->Delete();
  patientIds->Delete();
  labels->Delete();

  return 1;
}

//-------------------------------------------------------------------------
int vtkMNITagPointReader::GetNumberOfVolumes()
{
  this->Update();

  return this->NumberOfVolumes;
}

//-------------------------------------------------------------------------
vtkPoints *vtkMNITagPointReader::GetPoints(int port)
{
  this->Update();

  if (port < 0 || port >= this->NumberOfVolumes)
  {
    return 0;
  }

  vtkPolyData *output = static_cast<vtkPolyData *>(
    this->GetOutputDataObject(port));

  if (output)
  {
    return output->GetPoints();
  }

  return 0;
}

//-------------------------------------------------------------------------
vtkStringArray *vtkMNITagPointReader::GetLabelText()
{
  this->Update();

  vtkPolyData *output = static_cast<vtkPolyData *>(
    this->GetOutputDataObject(0));

  if (output)
  {
    return vtkArrayDownCast<vtkStringArray>(
      output->GetPointData()->GetAbstractArray("LabelText"));
  }

  return 0;
}

//-------------------------------------------------------------------------
vtkDoubleArray *vtkMNITagPointReader::GetWeights()
{
  this->Update();

  vtkPolyData *output = static_cast<vtkPolyData *>(
    this->GetOutputDataObject(0));

  if (output)
  {
    return vtkArrayDownCast<vtkDoubleArray>(
      output->GetPointData()->GetArray("Weights"));
  }

  return 0;
}

//-------------------------------------------------------------------------
vtkIntArray *vtkMNITagPointReader::GetStructureIds()
{
  this->Update();

  vtkPolyData *output = static_cast<vtkPolyData *>(
    this->GetOutputDataObject(0));

  if (output)
  {
    return vtkArrayDownCast<vtkIntArray>(
      output->GetPointData()->GetArray("StructureIds"));
  }

  return 0;
}

//-------------------------------------------------------------------------
vtkIntArray *vtkMNITagPointReader::GetPatientIds()
{
  this->Update();

  vtkPolyData *output = static_cast<vtkPolyData *>(
    this->GetOutputDataObject(0));

  if (output)
  {
    return vtkArrayDownCast<vtkIntArray>(
      output->GetPointData()->GetArray("PatientIds"));
  }

  return 0;
}

//-------------------------------------------------------------------------
const char *vtkMNITagPointReader::GetComments()
{
  this->Update();

  return this->Comments;
}

//-------------------------------------------------------------------------
int vtkMNITagPointReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo1 = outputVector->GetInformationObject(0);
  vtkInformation *outInfo2 = outputVector->GetInformationObject(1);

  // get the ouptut
  vtkPolyData *output1 = vtkPolyData::SafeDownCast(
    outInfo1->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output2 = vtkPolyData::SafeDownCast(
    outInfo2->Get(vtkDataObject::DATA_OBJECT()));

  // all of the data in the first piece.
  if (outInfo1->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER())
      > 0 ||
      outInfo2->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER())
      > 0)
  {
    return 0;
  }

  // read the file
  return this->ReadFile(output1, output2);
}
