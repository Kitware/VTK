/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSTLReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSTLReader.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkErrorCode.h"
#include "vtkFloatArray.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkSTLReader);

#define VTK_ASCII 0
#define VTK_BINARY 1

vtkCxxSetObjectMacro(vtkSTLReader, Locator, vtkIncrementalPointLocator);

//------------------------------------------------------------------------------
// Construct object with merging set to true.
vtkSTLReader::vtkSTLReader()
{
  this->FileName = NULL;
  this->Merging = 1;
  this->ScalarTags = 0;
  this->Locator = NULL;

  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
vtkSTLReader::~vtkSTLReader()
{
  this->SetFileName(0);
  this->SetLocator(0);
}

//------------------------------------------------------------------------------
// Overload standard modified time function. If locator is modified,
// then this object is modified as well.
vtkMTimeType vtkSTLReader::GetMTime()
{
  vtkMTimeType mTime1 = this->Superclass::GetMTime();

  if (this->Locator)
  {
    vtkMTimeType mTime2 = this->Locator->GetMTime();
    mTime1 = std::max(mTime1, mTime2);
  }

  return mTime1;
}

//------------------------------------------------------------------------------
int vtkSTLReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // All of the data in the first piece.
  if (outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
  {
    return 0;
  }

  if (!this->FileName || *this->FileName == 0)
  {
    vtkErrorMacro(<<"A FileName must be specified.");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return 0;
  }

  // Initialize
  FILE *fp = fopen(this->FileName, "r");
  if (fp == NULL)
  {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    return 0;
  }

  vtkPoints *newPts = vtkPoints::New();
  vtkCellArray *newPolys = vtkCellArray::New();
  vtkFloatArray *newScalars = 0;

  // Depending upon file type, read differently
  if (this->GetSTLFileType(this->FileName) == VTK_ASCII)
  {
    newPts->Allocate(5000);
    newPolys->Allocate(10000);
    if (this->ScalarTags)
    {
      newScalars = vtkFloatArray::New();
      newScalars->Allocate(5000);
    }
    if (!this->ReadASCIISTL(fp, newPts, newPolys, newScalars))
    {
      fclose(fp);
      return 0;
    }
  }
  else
  {
    // Close file and reopen in binary mode.
    fclose(fp);
    fp = fopen(this->FileName, "rb");
    if (fp == NULL)
    {
      vtkErrorMacro(<< "File " << this->FileName << " not found");
      this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
      return 0;
    }

    if (!this->ReadBinarySTL(fp, newPts, newPolys))
    {
      fclose(fp);
      return 0;
    }
  }

  vtkDebugMacro(<< "Read: "
    << newPts->GetNumberOfPoints() << " points, "
    << newPolys->GetNumberOfCells() << " triangles");

  fclose(fp);

  // If merging is on, create hash table and merge points/triangles.
  vtkPoints *mergedPts = newPts;
  vtkCellArray *mergedPolys = newPolys;
  vtkFloatArray *mergedScalars = newScalars;
  if (this->Merging)
  {
    mergedPts = vtkPoints::New();
    mergedPts->Allocate(newPts->GetNumberOfPoints() /2);
    mergedPolys = vtkCellArray::New();
    mergedPolys->Allocate(newPolys->GetSize());
    if (newScalars)
    {
      mergedScalars = vtkFloatArray::New();
      mergedScalars->Allocate(newPolys->GetSize());
    }

    vtkSmartPointer<vtkIncrementalPointLocator> locator = this->Locator;
    if (this->Locator == NULL)
    {
      locator.TakeReference(this->NewDefaultLocator());
    }
    locator->InitPointInsertion(mergedPts, newPts->GetBounds());

    int nextCell = 0;
    vtkIdType *pts = 0;
    vtkIdType npts;
    for (newPolys->InitTraversal(); newPolys->GetNextCell(npts, pts);)
    {
      vtkIdType nodes[3];
      for (int i = 0; i < 3; i++)
      {
        double x[3];
        newPts->GetPoint(pts[i], x);
        locator->InsertUniquePoint(x, nodes[i]);
      }

      if (nodes[0] != nodes[1] &&
        nodes[0] != nodes[2] &&
        nodes[1] != nodes[2])
      {
        mergedPolys->InsertNextCell(3, nodes);
        if (newScalars)
        {
          mergedScalars->InsertNextValue(newScalars->GetValue(nextCell));
        }
      }
      nextCell++;
    }

    newPts->Delete();
    newPolys->Delete();
    if (newScalars)
    {
      newScalars->Delete();
    }

    vtkDebugMacro(<< "Merged to: "
      << mergedPts->GetNumberOfPoints() << " points, "
      << mergedPolys->GetNumberOfCells() << " triangles");
  }

  output->SetPoints(mergedPts);
  mergedPts->Delete();

  output->SetPolys(mergedPolys);
  mergedPolys->Delete();

  if (mergedScalars)
  {
    mergedScalars->SetName("STLSolidLabeling");
    output->GetCellData()->SetScalars(mergedScalars);
    mergedScalars->Delete();
  }

  if (this->Locator)
  {
    this->Locator->Initialize(); //free storage
  }

  output->Squeeze();

  return 1;
}

//------------------------------------------------------------------------------
bool vtkSTLReader::ReadBinarySTL(FILE *fp, vtkPoints *newPts,
                                 vtkCellArray *newPolys)
{
  typedef struct { float  n[3], v1[3], v2[3], v3[3]; } facet_t;

  vtkDebugMacro(<< "Reading BINARY STL file");

  //  File is read to obtain raw information as well as bounding box
  //
  char header[81];
  if (fread(header, 1, 80, fp) != 80)
  {
    vtkErrorMacro("STLReader error reading file: " << this->FileName
      << " Premature EOF while reading header.");
    return false;
  }

  unsigned long ulint;
  if (fread(&ulint, 1, 4, fp) != 4)
  {
    vtkErrorMacro("STLReader error reading file: " << this->FileName
      << " Premature EOF while reading header.");
    return false;
  }
  vtkByteSwap::Swap4LE(&ulint);

  // Many .stl files contain bogus count.  Hence we will ignore and read
  //   until end of file.
  //
  int numTris = static_cast<int>(ulint);
  if (numTris <= 0)
  {
    vtkDebugMacro(<< "Bad binary count: attempting to correct("
      << numTris << ")");
  }

  // Verify the numTris with the length of the file
  unsigned long ulFileLength = vtksys::SystemTools::FileLength(this->FileName);
  ulFileLength -= (80 + 4); // 80 byte - header, 4 byte - tringle count
  ulFileLength /= 50;       // 50 byte - twelve 32-bit-floating point numbers + 2 byte for attribute byte count

  if (numTris < static_cast<int>(ulFileLength))
  {
    numTris = static_cast<int>(ulFileLength);
  }

  // now we can allocate the memory we need for this STL file
  newPts->Allocate(numTris * 3);
  newPolys->Allocate(numTris);

  facet_t facet;
  for (int i = 0; fread(&facet, 48, 1, fp) > 0; i++)
  {
    unsigned short ibuff2;
    if (fread(&ibuff2, 2, 1, fp) != 1) //read extra junk
    {
      vtkErrorMacro("STLReader error reading file: " << this->FileName
        << " Premature EOF while reading extra junk.");
      return false;
    }

    vtkByteSwap::Swap4LE(facet.n);
    vtkByteSwap::Swap4LE(facet.n+1);
    vtkByteSwap::Swap4LE(facet.n+2);

    vtkByteSwap::Swap4LE(facet.v1);
    vtkByteSwap::Swap4LE(facet.v1+1);
    vtkByteSwap::Swap4LE(facet.v1+2);

    vtkByteSwap::Swap4LE(facet.v2);
    vtkByteSwap::Swap4LE(facet.v2+1);
    vtkByteSwap::Swap4LE(facet.v2+2);

    vtkByteSwap::Swap4LE(facet.v3);
    vtkByteSwap::Swap4LE(facet.v3+1);
    vtkByteSwap::Swap4LE(facet.v3+2);

    vtkIdType pts[3];
    pts[0] = newPts->InsertNextPoint(facet.v1);
    pts[1] = newPts->InsertNextPoint(facet.v2);
    pts[2] = newPts->InsertNextPoint(facet.v3);

    newPolys->InsertNextCell(3, pts);

    if ((i % 5000) == 0 && i != 0)
    {
      vtkDebugMacro(<< "triangle# " << i);
      this->UpdateProgress(static_cast<double>(i) / numTris);
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkSTLReader::ReadASCIISTL(FILE *fp, vtkPoints *newPts,
                                vtkCellArray *newPolys, vtkFloatArray *scalars)
{
  vtkDebugMacro(<< "Reading ASCII STL file");

  // Ingest header and junk to get to first vertex
  char line[256];
  if (!fgets(line, 255, fp))
  {
    vtkErrorMacro("STLReader error reading file: " << this->FileName
                   << " Premature EOF while reading header at line 0.");
    return false;
  }

  int lineCount = 1;
  int done = (fgets(line, 255, fp) == 0);
  float x[3];
  lineCount++;
  if (!strcmp(line, "COLOR") || !strcmp(line, "color"))
  {
      // if there is a color field, skip it
      done = (fgets(line, 255, fp) == 0);
      lineCount++;
  }

  try
  {
    int currentSolid = 0;
    // Go into loop, reading facet normal and vertices
    while (!done)
    {
      if (!fgets(line, 255, fp))
      {
        throw std::runtime_error("unable to read outer loop.");
      }
      lineCount++;

      if (fscanf(fp, "%*s %f %f %f\n", x, x+1, x+2) != 3)
      {
        throw std::runtime_error("unable to read point.");
      }
      lineCount++;

      vtkIdType pts[3];
      pts[0] = newPts->InsertNextPoint(x);
      if (fscanf(fp, "%*s %f %f %f\n", x, x+1, x+2) != 3)
      {
        throw std::runtime_error("unable to read point.");
      }
      lineCount++;

      pts[1] = newPts->InsertNextPoint(x);
      if (fscanf(fp, "%*s %f %f %f\n", x, x+1, x+2) != 3)
      {
        throw std::runtime_error("unable to read reading point.");
      }
      lineCount++;

      pts[2] = newPts->InsertNextPoint(x);
      if (!fgets(line, 255, fp)) // end loop
      {
        throw std::runtime_error("unable to read end loop.");
      }
      lineCount++;

      if (!fgets(line, 255, fp)) // end facet
      {
        throw std::runtime_error("unable to read end facet.");
      }
      lineCount++;

      newPolys->InsertNextCell(3, pts);
      if (scalars)
      {
        scalars->InsertNextValue(currentSolid);
      }

      if ((newPolys->GetNumberOfCells() % 5000) == 0)
      {
        this->UpdateProgress((newPolys->GetNumberOfCells()%50000) / 50000.0);
      }
      done = (fscanf(fp,"%s", line) == EOF);
      if (!strcmp(line, "ENDSOLID") || !strcmp(line, "endsolid"))
      {
        currentSolid++;
        if (!fgets(line, 255, fp) && !feof(fp))
        {
          throw std::runtime_error("unable to read end solid.");
        }

        done = feof(fp);
        while (!strstr(line, "SOLID") && !strstr(line, "solid") && !done)
        {
          if (!fgets(line, 255, fp))
          {
            // if fgets() returns an error, it may be due to the fact that the EOF
            // is reached(BUG #13101) hence we test again.
            done = feof(fp);
            if (!done)
            {
              throw std::runtime_error("unable to read solid.");
            }
          }
          lineCount++;
          done = feof(fp);
        }

        done = (fscanf(fp,"%s", line)==EOF);
        if (!strstr(line, "COLOR") || !strstr(line, "color"))
        {
          done = (fgets(line, 255, fp) == 0);
          lineCount++;
          done = done || (fscanf(fp,"%s", line)==EOF);
        }
      }
      else if (!done)
      {
        done = (fgets(line, 255, fp) == 0);
        lineCount++;
      }
    }
  }
  catch (const std::runtime_error &e)
  {
    vtkErrorMacro("STLReader: error while reading file " <<
      this->FileName << " at line " << lineCount << ": " << e.what());
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
int vtkSTLReader::GetSTLFileType(const char *filename)
{
  vtksys::SystemTools::FileTypeEnum ft =
    vtksys::SystemTools::DetectFileType(filename);
  switch (ft)
  {
  case vtksys::SystemTools::FileTypeBinary:
    return VTK_BINARY;
  case vtksys::SystemTools::FileTypeText:
    return VTK_ASCII;
  case vtksys::SystemTools::FileTypeUnknown:
    vtkWarningMacro("File type not recognized; attempting binary");
    return VTK_BINARY;
  default:
    vtkErrorMacro("Case not handled, file type is " << static_cast<int>(ft));
    return VTK_BINARY; // should not happen
  }
}

//------------------------------------------------------------------------------
// Specify a spatial locator for merging points. By
// default an instance of vtkMergePoints is used.
vtkIncrementalPointLocator* vtkSTLReader::NewDefaultLocator()
{
  return vtkMergePoints::New();
}

//------------------------------------------------------------------------------
void vtkSTLReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "File Name: "
     <<(this->FileName ? this->FileName : "(none)") << "\n";

  os << indent << "Merging: " <<(this->Merging ? "On\n" : "Off\n");
  os << indent << "ScalarTags: " <<(this->ScalarTags ? "On\n" : "Off\n");
  os << indent << "Locator: ";
  if (this->Locator)
  {
    this->Locator->PrintSelf(os << endl, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
}
