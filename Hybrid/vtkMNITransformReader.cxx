/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMNITransformReader.cxx

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

#include "vtkMNITransformReader.h"

#include "vtkObjectFactory.h"

#include "vtkImageData.h"
#include "vtkMINCImageReader.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkCollection.h"
#include "vtkTransform.h"
#include "vtkGeneralTransform.h"
#include "vtkThinPlateSplineTransform.h"
#include "vtkGridTransform.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPoints.h"

#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "vtkstd/string"
#include "vtkstd/vector"
#include "vtksys/SystemTools.hxx"

//--------------------------------------------------------------------------
vtkStandardNewMacro(vtkMNITransformReader);

//-------------------------------------------------------------------------
vtkMNITransformReader::vtkMNITransformReader()
{
  this->FileName = 0;
  this->Transform = 0;
  this->Transforms = vtkCollection::New();
  this->LineNumber = 0;
  this->Comments = 0;
}

//-------------------------------------------------------------------------
vtkMNITransformReader::~vtkMNITransformReader()
{
  if (this->Transforms)
    {
    this->Transforms->Delete();
    }
  if (this->Transform)
    {
    this->Transform->Delete();
    }
  if (this->FileName)
    {
    delete [] this->FileName;
    }
  if (this->Comments)
    {
    delete [] this->Comments;
    }
}

//-------------------------------------------------------------------------
void vtkMNITransformReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FileName: "
     << (this->FileName ? this->FileName : "none") << "\n";
  os << indent << "Transform: " << this->Transform << "\n";
  if (this->Transform)
    {
    this->Transform->PrintSelf(os, indent.GetNextIndent());
    }
  os << indent << "NumberOfTransforms: "
     << this->Transforms->GetNumberOfItems() << "\n";
  os << indent << "Comments: "
     << (this->Comments ? this->Comments : "none") << "\n";
}

//-------------------------------------------------------------------------
int vtkMNITransformReader::CanReadFile(const char* fname)
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
    status = 1;
    char linetext[256];
    infile.getline(linetext, 256);
    if (strncmp(linetext, "MNI Transform File", 18) != 0)
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
int vtkMNITransformReader::ReadLine(
  istream &infile, char result[256])
{
  this->LineNumber++;

  infile.getline(result,256);
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
// Skip all blank lines or comment lines and return the first useful line
int vtkMNITransformReader::ReadLineAfterComments(
  istream &infile, char result[256])
{
  // Skip over any comment lines or blank lines.
  // Comment lines start with '%'
  vtkstd::string comments;
  do
    {
    this->ReadLine(infile, result);
    const char *cp = result;
    while (*cp && isspace(*cp))
      {
      cp++;
      }
    if (result[0] == '%')
      {
      if (comments.length() > 0)
        {
        comments.append("\n");
        }
      comments.append(result);
      }
    else if (*cp != '\0')
      {
      if (this->Comments)
        {
        delete [] this->Comments;
        }
      this->Comments = new char[comments.length() + 1];
      strcpy(this->Comments, comments.c_str());
      return 1;
      }
    }
  while (infile.good());

  return 0;
}

//-------------------------------------------------------------------------
// Skip all whitespace, reading additional lines if necessary
int vtkMNITransformReader::SkipWhitespace(
  istream &infile, char linetext[256], char **cpp)
{
  char *cp = *cpp;

  while (infile.good())
    {
    // Skip leading whitespace
    while (isspace(*cp))
      {
      cp++;
      }

    if (*cp != '\0')
      {
      *cpp = cp;
      return 1;
      }

    this->ReadLine(infile, linetext);
    cp = linetext;
    }

  return 0;
}

//-------------------------------------------------------------------------
// Read the left hand side of a statement, including the equals sign
// and any whitespace following the equals.
int vtkMNITransformReader::ParseLeftHandSide(
  istream &infile, char linetext[256], char **cpp, char identifier[256])
{
  int i = 0;
  char *cp = *cpp;

  // Read alphanumeric plus underscore
  if (!isdigit(*cp))
    {
    while ((isalnum(*cp) || *cp == '_') && i < 255)
      {
      identifier[i++] = *cp++;
      }
    }
  identifier[i] = '\0';

  // Skip trailing whitespace
  while (isspace(*cp))
    {
    cp++;
    }

  // Check for equals
  this->SkipWhitespace(infile, linetext, &cp);
  if (*cp != '=')
    {
    vtkErrorMacro("Missing \'=\' " << this->FileName
                  << ":" << this->LineNumber);
    return 0;
    }
  cp++;

  // Skip ahead to the value part of the statement
  this->SkipWhitespace(infile, linetext, &cp);

  *cpp = cp;

  return 1;
}

//-------------------------------------------------------------------------
// Read a string value.  The terminating semicolon will be read, but
// won't be included in the output string.  Neither will any
// whitespace occurring before the semicolon. The string may not be
// split across multiple lines.
int vtkMNITransformReader::ParseStringValue(
  istream &infile, char linetext[256], char **cpp, char data[256])
{
  int i = 0;
  char *cp = *cpp;

  this->SkipWhitespace(infile, linetext, &cp);

  // Read until end of the line or semicolon
  while (*cp && *cp != ';' && i < 255)
    {
    data[i++] = *cp++;
    }

  // Remove trailing whitespace
  while (i > 0 && isspace(data[i-1]))
    {
    i--;
    }

  data[i] = '\0';

  this->SkipWhitespace(infile, linetext, &cp);
  if (*cp != ';')
    {
    vtkErrorMacro("Missing semicolon " << this->FileName
                  << ":" << this->LineNumber);
    return 0;
    }
  cp++;

  *cpp = cp;

  return 1;
}

//-------------------------------------------------------------------------
// Read floating-point values into a vtkDoubleArray until a semicolon
// is reached.  The semicolon is also read.
int vtkMNITransformReader::ParseFloatValues(
  istream &infile, char linetext[256], char **cpp, vtkDoubleArray *array)
{
  char *cp = *cpp;

  this->SkipWhitespace(infile, linetext, &cp);
  while (infile.good() && *cp != ';')
    {
    char *tmp = cp;
    double val = strtod(cp, &cp);
    if (cp == tmp)
      {
      vtkErrorMacro("Syntax error " << this->FileName
                    << ":" << this->LineNumber);
      return 0;
      }
    array->InsertNextValue(val);
    this->SkipWhitespace(infile, linetext, &cp);
    }

  if (*cp != ';')
    {
    vtkErrorMacro("Missing semicolon " << this->FileName
                  << ":" << this->LineNumber);
    return 0;
    }
  cp++;

  *cpp = cp;

  return 1;
}

//-------------------------------------------------------------------------
int vtkMNITransformReader::ParseInvertFlagValue(
  istream &infile, char linetext[256], char **cpp, int *invertFlag)
{
  char data[256];

  if (!this->ParseStringValue(infile, linetext, cpp, data))
    {
    return 0;
    }
  if (strcmp(data, "False") == 0)
    {
    *invertFlag = 0;
    }
  else if (strcmp(data, "True") == 0)
    {
    *invertFlag = 1;
    }
  else
    {
    vtkErrorMacro("Invert_Flag must be \'True\' or \'False\' "
                  << this->FileName << ":" << this->LineNumber);
    return 0;
    }

  return 1;
}


//-------------------------------------------------------------------------
int vtkMNITransformReader::ReadLinearTransform(
  istream &infile, char linetext[256], char **cpp)
{
  // Read the first variable
  this->SkipWhitespace(infile, linetext, cpp);
  char identifier[256];
  if (!this->ParseLeftHandSide(infile, linetext, cpp, identifier))
    {
    return 0;
    }

  // Check for Invert_Flag
  int invertFlag = 0;
  if (strcmp(identifier, "Invert_Flag") == 0)
    {
    if (!this->ParseInvertFlagValue(infile, linetext, cpp, &invertFlag))
      {
      return 0;
      }

    this->SkipWhitespace(infile, linetext, cpp);
    if (!this->ParseLeftHandSide(infile, linetext, cpp, identifier))
      {
      return 0;
      }
    }

  // Check for Linear_Transform
  if (strcmp(identifier, "Linear_Transform") != 0)
    {
    vtkErrorMacro("Expected \'Linear_Transform\' in "
                  << this->FileName << ":" << this->LineNumber);
    return 0;
    }

  // Read twelve array elements from the file
  vtkDoubleArray *array = vtkDoubleArray::New();
  if (!this->ParseFloatValues(infile, linetext, cpp, array))
    {
    return 0;
    }

  if (array->GetNumberOfTuples() != 12)
    {
    vtkErrorMacro("Linear transform must have exactly 12 elements "
                  << this->FileName << ":" << this->LineNumber);
    array->Delete();
    return 0;
    }

  // Fill in the last row of the 4x4 matrix
  array->InsertNextValue(0.0);
  array->InsertNextValue(0.0);
  array->InsertNextValue(0.0);
  array->InsertNextValue(1.0);

  // Create the transform
  vtkTransform *transform = vtkTransform::New();
  transform->Concatenate(array->GetPointer(0));
  array->Delete();
  if (invertFlag)
    {
    transform->Inverse();
    }

  this->Transforms->AddItem(transform);
  transform->Delete();

  return 1;
}

//-------------------------------------------------------------------------
int vtkMNITransformReader::ReadThinPlateSplineTransform(
  istream &infile, char linetext[256], char **cpp)
{
  // Read the first variable
  this->SkipWhitespace(infile, linetext, cpp);
  char identifier[256];
  if (!this->ParseLeftHandSide(infile, linetext, cpp, identifier))
    {
    return 0;
    }

  // Check for Invert_Flag
  int invertFlag = 0;
  if (strcmp(identifier, "Invert_Flag") == 0)
    {
    if (!this->ParseInvertFlagValue(infile, linetext, cpp, &invertFlag))
      {
      return 0;
      }

    this->SkipWhitespace(infile, linetext, cpp);
    if (!this->ParseLeftHandSide(infile, linetext, cpp, identifier))
      {
      return 0;
      }
    }

  // Number_Dimensions: vtkThinPlateSplineTransform supports 2 and 3
  if (strcmp(identifier, "Number_Dimensions") != 0)
    {
    vtkErrorMacro("Expected \'Number_Dimensions\' in "
                  << this->FileName << ":" << this->LineNumber);
    return 0;
    }

  char data[256];
  if (!this->ParseStringValue(infile, linetext, cpp, data))
    {
    return 0;
    }

  int numDimensions = data[0] - '0';
  if (data[1] != '\0' || numDimensions < 2 || numDimensions > 3)
    {
    vtkErrorMacro("Number_Dimensions must be 2 or 3 in "
                  << this->FileName << ":" << this->LineNumber);
    return 0;
    }

  // Read the points
  this->SkipWhitespace(infile, linetext, cpp);
  if (!this->ParseLeftHandSide(infile, linetext, cpp, identifier))
    {
    return 0;
    }

  if (strcmp(identifier, "Points") != 0)
    {
    vtkErrorMacro("Expected \'Points\' in "
                  << this->FileName << ":" << this->LineNumber);
    return 0;
    }

  vtkDoubleArray *points = vtkDoubleArray::New();
  if (!this->ParseFloatValues(infile, linetext, cpp, points))
    {
    points->Delete();
    return 0;
    }

  if (points->GetNumberOfTuples() % numDimensions != 0)
    {
    points->Delete();
    vtkErrorMacro("Points list not divisible by Number_Dimensions in "
                  << this->FileName << ":" << this->LineNumber);
    return 0;
    }

  // Read the displacements
  this->SkipWhitespace(infile, linetext, cpp);
  if (!this->ParseLeftHandSide(infile, linetext, cpp, identifier))
    {
    points->Delete();
    return 0;
    }

  if (strcmp(identifier, "Displacements") != 0)
    {
    points->Delete();
    vtkErrorMacro("Expected \'Displacements\' in "
                  << this->FileName << ":" << this->LineNumber);
    return 0;
    }

  vtkDoubleArray *displacements = vtkDoubleArray::New();
  if (!this->ParseFloatValues(infile, linetext, cpp, displacements))
    {
    displacements->Delete();
    points->Delete();
    return 0;
    }

  if (displacements->GetNumberOfTuples() !=
      points->GetNumberOfTuples() + numDimensions*(numDimensions + 1))
    {
    displacements->Delete();
    points->Delete();
    vtkErrorMacro("Incorrect nubmer of Displacements in "
                  << this->FileName << ":" << this->LineNumber);
    return 0;
    }

  // The vtkThinPlateSplineTransform expects two sets of points,
  // not a set of points and a set of displacements.  We need
  // to perform a thin-plate spline transform to get the points
  // that we need.

  int numPoints = points->GetNumberOfTuples()/numDimensions;
  int i = 0;
  int j = 0;

  // Convert points and displacements to 3D
  double (*q)[3] = new double[numPoints][3];
  double (*W)[3] = new double[numPoints][3];
  for (i = 0; i < numPoints; i++)
    {
    double *point = q[i];
    double *displacement = W[i];
    point[0] = point[1] = point[2] = 0.0;
    displacement[0] = displacement[1] = displacement[2] = 0.0;
    for (j = 0; j < numDimensions; j++)
      {
      point[j] = points->GetValue(i*numDimensions + j);
      displacement[j] = displacements->GetValue(i*numDimensions + j);
      }
    }

  // Get the translation from the TPS matrix
  double C[3];
  C[0] = C[1] = C[2] = 0.0;
  for (j = 0; j < numDimensions; j++)
    {
    C[j] = displacements->GetValue(numPoints*numDimensions + j);
    }

  // Get the square matrix portion of the TPS matrix
  double A[3][3];
  A[0][1] = A[0][2] = A[1][0] = A[1][2] = A[2][0] = A[2][1] = 0.0;
  A[0][0] = A[1][1] = A[2][2] = 1.0;
  for (i = 0; i < numDimensions; i++)
    {
    for (j = 0; j < numDimensions; j++)
      {
      A[i][j] = displacements->GetValue(
        (numPoints + 1 + i)*numDimensions + j);
      }
    }

  displacements->Delete();
  points->Delete();

  // Create the source and target point lists
  vtkPoints *source = vtkPoints::New();
  vtkPoints *target = vtkPoints::New();
  for (i = 0; i < numPoints; i++)
    {
    double *p = q[i];
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    for (j = 0; j < numPoints; j++)
      {
      double dx = p[0] - q[j][0];
      double dy = p[1] - q[j][1];
      double dz = p[2] - q[j][2];
      double r = sqrt(dx*dx + dy*dy + dz*dz);
      double U = ((numDimensions == 2 && r != 0) ? r*r*log(r) : r);
      x += U*W[j][0];
      y += U*W[j][1];
      z += U*W[j][2];
    }

    x += C[0] + p[0]*A[0][0] + p[1]*A[1][0] + p[2]*A[2][0];
    y += C[1] + p[0]*A[0][1] + p[1]*A[1][1] + p[2]*A[2][1];
    z += C[2] + p[0]*A[0][2] + p[1]*A[1][2] + p[2]*A[2][2];

    source->InsertNextPoint(p);
    target->InsertNextPoint(x, y, z);
    }

  delete [] q;
  delete [] W;

  // Create the thin plate spline transform
  vtkThinPlateSplineTransform *transform = vtkThinPlateSplineTransform::New();
  transform->SetSourceLandmarks(source);
  transform->SetTargetLandmarks(target);
  if (numDimensions == 2)
    {
    transform->SetBasisToR2LogR();
    }
  else
    {
    transform->SetBasisToR();
    }

  if (invertFlag)
    {
    transform->Inverse();
    }

  source->Delete();
  target->Delete();

  this->Transforms->AddItem(transform);
  transform->Delete();

  return 1;
}

//-------------------------------------------------------------------------
int vtkMNITransformReader::ReadGridTransform(
  istream &infile, char linetext[256], char **cpp)
{
  // Read the first variable
  this->SkipWhitespace(infile, linetext, cpp);
  char identifier[256];
  if (!this->ParseLeftHandSide(infile, linetext, cpp, identifier))
    {
    return 0;
    }

  // Check for Invert_Flag
  int invertFlag = 0;
  if (strcmp(identifier, "Invert_Flag") == 0)
    {
    if (!this->ParseInvertFlagValue(infile, linetext, cpp, &invertFlag))
      {
      return 0;
      }

    this->SkipWhitespace(infile, linetext, cpp);
    if (!this->ParseLeftHandSide(infile, linetext, cpp, identifier))
      {
      return 0;
      }
    }

  // Displacement_Volume must be a minc file
  if (strcmp(identifier, "Displacement_Volume") != 0)
    {
    vtkErrorMacro("Expected \'Displacement_Volume\' in "
                  << this->FileName << ":" << this->LineNumber);
    return 0;
    }

  char filename[256];
  if (!this->ParseStringValue(infile, linetext, cpp, filename))
    {
    return 0;
    }

  // Create the minc reader
  vtkMINCImageReader *reader = vtkMINCImageReader::New();

  vtkstd::vector<vtkstd::string> xfmpath;
  vtkstd::vector<vtkstd::string> mincpath;

  vtksys::SystemTools::SplitPath(this->FileName, xfmpath);
  vtksys::SystemTools::SplitPath(filename, mincpath);

  // Join minc filename to this->FileName if filename is relative
  if (mincpath[0] == "")
    {
    xfmpath.pop_back();
    xfmpath.insert(xfmpath.end(), mincpath.begin()+1, mincpath.end());
    reader->SetFileName(vtksys::SystemTools::JoinPath(xfmpath).c_str());
    }
  else
    {
    reader->SetFileName(filename);
    }

  // Read the minc file now, rather than later
  reader->GetOutput()->Update();

  // Create the transform
  vtkGridTransform *transform = vtkGridTransform::New();
  transform->SetDisplacementGrid(reader->GetOutput());
  transform->SetDisplacementShift(reader->GetRescaleIntercept());
  transform->SetDisplacementScale(reader->GetRescaleSlope());
  transform->SetInverseTolerance(0.05);
  transform->SetInterpolationModeToCubic();

  if (invertFlag)
    {
    transform->Inverse();
    }

  reader->Delete();

  this->Transforms->AddItem(transform);
  transform->Delete();

  return 1;
}

//-------------------------------------------------------------------------
int vtkMNITransformReader::ReadNextTransform(istream &infile, char linetext[256])
{
  // Check for errors
  if (infile.eof())
    {
    return 1;
    }
  else if (infile.fail())
    {
    vtkErrorMacro("IO error while reading " << this->FileName);
    return 0;
    }

  // Parse the file
  char *cp = linetext;

  // Check for Transform_Type
  char identifier[256];
  this->SkipWhitespace(infile, linetext, &cp);
  if (!this->ParseLeftHandSide(infile, linetext, &cp, identifier))
    {
    return 0;
    }

  if (strcmp(identifier, "Transform_Type") != 0)
    {
    vtkErrorMacro("Expected Transform_Type in "
                  << this->FileName << ":" << this->LineNumber);
    return 0;
    }

  // Read the transform type
  char transformType[256];
  if (!this->ParseStringValue(infile, linetext, &cp, transformType))
    {
    return 0;
    }

  // Check the transform type
  if (strcmp(transformType, "Linear") == 0)
    {
    return this->ReadLinearTransform(infile, linetext, &cp);
    }
  else if (strcmp(transformType, "Thin_Plate_Spline_Transform") == 0)
    {
    return this->ReadThinPlateSplineTransform(infile, linetext, &cp);
    }
  else if (strcmp(transformType, "Grid_Transform") == 0)
    {
    return this->ReadGridTransform(infile, linetext, &cp);
    }

  vtkErrorMacro("Unrecognized type " << transformType << " in "
                << this->FileName << ":" << this->LineNumber);
  return 0;
}

//-------------------------------------------------------------------------
int vtkMNITransformReader::ReadFile()
{
  this->Transforms->RemoveAllItems();
  this->SetTransform(0);

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
  ifstream infile(this->FileName);

  if (infile.fail())
    {
    vtkErrorMacro("ReadFile: Can't read the file " << this->FileName);
    return 0;
    }

  // Read the first line
  char linetext[256];
  this->LineNumber = 0;
  this->ReadLine(infile, linetext);
  if (strncmp(linetext, "MNI Transform File", 18) != 0)
    {
    vtkErrorMacro("ReadFile: File is not a MNI xfm file: " << this->FileName);
    infile.close();
    return 0;
    }

  // Read the comments
  this->ReadLineAfterComments(infile, linetext);

  // Read the transforms
  while (infile.good())
    {
    if (this->ReadNextTransform(infile, linetext) == 0)
      {
      this->Transforms->RemoveAllItems();
      infile.close();
      return 0;
      }
    this->ReadLine(infile, linetext);
    }

  // Close the file
  infile.close();

  // Create the output transform.
  int n = this->Transforms->GetNumberOfItems();
  if (n == 1)
    {
    this->SetTransform(
      (vtkAbstractTransform *)this->Transforms->GetItemAsObject(0));
    }
  else
    {
    // Determine whether the full transform is linear
    int linear = 1;
    int i = 0;
    for (i = 0; i < n; i++)
      {
      if (!this->Transforms->GetItemAsObject(i)->IsA("vtkLinearTransform"))
        {
        linear = 0;
        break;
        }
      }

    // If linear, use vtkTransform to concatenate,
    // else use vtkGeneralTransform.
    if (linear)
      {
      vtkTransform *transform = vtkTransform::New();
      transform->PostMultiply();
      for (i = 0; i < n; i++)
        {
        vtkLinearTransform *linearTransform =
          (vtkLinearTransform *)this->Transforms->GetItemAsObject(i);
        transform->Concatenate(linearTransform->GetMatrix());
        }
      this->SetTransform(transform);
      transform->Delete();
      }
    else
      {
      vtkGeneralTransform *transform = vtkGeneralTransform::New();
      transform->PostMultiply();
      for (i = 0; i < n; i++)
        {
        vtkAbstractTransform *abstractTransform =
          (vtkAbstractTransform *)this->Transforms->GetItemAsObject(i);
        if (abstractTransform->IsA("vtkLinearTransform"))
          {
          transform->Concatenate(
            ((vtkLinearTransform *)abstractTransform)->GetMatrix());
          }
        else
          {
          transform->Concatenate(abstractTransform);
          }
        }
      this->SetTransform(transform);
      transform->Delete();
      }
    }

  return 1;
}

//-------------------------------------------------------------------------
int vtkMNITransformReader::ProcessRequest(vtkInformation *request,
                                    vtkInformationVector **inputVector,
                                    vtkInformationVector *outputVector)
{
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    return this->ReadFile();
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//-------------------------------------------------------------------------
void vtkMNITransformReader::SetTransform(vtkAbstractTransform *transform)
{
  if (this->Transform != transform)
    {
    if (this->Transform)
      {
      this->Transform->Delete();
      }
    if (transform)
      {
      transform->Register(this);
      }
    this->Transform = transform;
    }
}

//-------------------------------------------------------------------------
vtkAbstractTransform *vtkMNITransformReader::GetTransform()
{
  this->Update();

  return this->Transform;
}

//-------------------------------------------------------------------------
int vtkMNITransformReader::GetNumberOfTransforms()
{
  this->Update();

  return this->Transforms->GetNumberOfItems();
}

//-------------------------------------------------------------------------
vtkAbstractTransform *vtkMNITransformReader::GetNthTransform(int i)
{
  this->Update();

  if (i < 0 || i >= this->Transforms->GetNumberOfItems())
    {
    return 0;
    }

  return (vtkAbstractTransform *)this->Transforms->GetItemAsObject(i);
}

//-------------------------------------------------------------------------
const char *vtkMNITransformReader::GetComments()
{
  this->Update();

  return this->Comments;
}
