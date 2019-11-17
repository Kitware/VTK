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
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkSTLReader);

#define VTK_ASCII 0
#define VTK_BINARY 1

vtkCxxSetObjectMacro(vtkSTLReader, Locator, vtkIncrementalPointLocator);
vtkCxxSetObjectMacro(vtkSTLReader, BinaryHeader, vtkUnsignedCharArray);

//------------------------------------------------------------------------------
// Construct object with merging set to true.
vtkSTLReader::vtkSTLReader()
{
  this->Merging = 1;
  this->ScalarTags = 0;
  this->Locator = nullptr;
  this->Header = nullptr;
  this->BinaryHeader = nullptr;
}

//------------------------------------------------------------------------------
vtkSTLReader::~vtkSTLReader()
{
  this->SetLocator(nullptr);
  this->SetHeader(nullptr);
  this->SetBinaryHeader(nullptr);
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
int vtkSTLReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // All of the data in the first piece.
  if (outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
  {
    return 0;
  }

  if (!this->FileName || *this->FileName == 0)
  {
    vtkErrorMacro(<< "A FileName must be specified.");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return 0;
  }

  // Initialize
  FILE* fp = vtksys::SystemTools::Fopen(this->FileName, "r");
  if (fp == nullptr)
  {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    return 0;
  }

  vtkNew<vtkPoints> newPts;
  vtkNew<vtkCellArray> newPolys;
  vtkFloatArray* newScalars = nullptr;

  // Depending upon file type, read differently
  if (this->GetSTLFileType(this->FileName) == VTK_ASCII)
  {
    newPts->Allocate(5000);
    newPolys->AllocateEstimate(10000, 1);
    if (this->ScalarTags)
    {
      newScalars = vtkFloatArray::New();
      newScalars->Allocate(5000);
    }
    if (!this->ReadASCIISTL(fp, newPts.Get(), newPolys.Get(), newScalars))
    {
      fclose(fp);
      if (newScalars)
      {
        newScalars->Delete();
      }
      return 0;
    }
  }
  else
  {
    // Close file and reopen in binary mode.
    fclose(fp);
    fp = vtksys::SystemTools::Fopen(this->FileName, "rb");
    if (fp == nullptr)
    {
      vtkErrorMacro(<< "File " << this->FileName << " not found");
      this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
      if (newScalars)
      {
        newScalars->Delete();
      }
      return 0;
    }

    if (!this->ReadBinarySTL(fp, newPts.Get(), newPolys.Get()))
    {
      fclose(fp);
      if (newScalars)
      {
        newScalars->Delete();
      }
      return 0;
    }
  }

  vtkDebugMacro(<< "Read: " << newPts->GetNumberOfPoints() << " points, "
                << newPolys->GetNumberOfCells() << " triangles");

  fclose(fp);

  // If merging is on, create hash table and merge points/triangles.
  vtkPoints* mergedPts = newPts.Get();
  vtkCellArray* mergedPolys = newPolys.Get();
  vtkFloatArray* mergedScalars = newScalars;
  if (this->Merging)
  {
    mergedPts = vtkPoints::New();
    mergedPts->Allocate(newPts->GetNumberOfPoints() / 2);
    mergedPolys = vtkCellArray::New();
    mergedPolys->AllocateCopy(newPolys);
    if (newScalars)
    {
      mergedScalars = vtkFloatArray::New();
      mergedScalars->Allocate(newPolys->GetNumberOfCells());
    }

    vtkSmartPointer<vtkIncrementalPointLocator> locator = this->Locator;
    if (this->Locator == nullptr)
    {
      locator.TakeReference(this->NewDefaultLocator());
    }
    locator->InitPointInsertion(mergedPts, newPts->GetBounds());

    int nextCell = 0;
    const vtkIdType* pts = nullptr;
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

      if (nodes[0] != nodes[1] && nodes[0] != nodes[2] && nodes[1] != nodes[2])
      {
        mergedPolys->InsertNextCell(3, nodes);
        if (newScalars)
        {
          mergedScalars->InsertNextValue(newScalars->GetValue(nextCell));
        }
      }
      nextCell++;
    }

    if (newScalars)
    {
      newScalars->Delete();
    }

    vtkDebugMacro(<< "Merged to: " << mergedPts->GetNumberOfPoints() << " points, "
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
    this->Locator->Initialize(); // free storage
  }

  output->Squeeze();

  return 1;
}

//------------------------------------------------------------------------------
bool vtkSTLReader::ReadBinarySTL(FILE* fp, vtkPoints* newPts, vtkCellArray* newPolys)
{
  typedef struct
  {
    float n[3], v1[3], v2[3], v3[3];
  } facet_t;

  vtkDebugMacro(<< "Reading BINARY STL file");

  //  File is read to obtain raw information as well as bounding box
  //
  if (!this->BinaryHeader)
  {
    vtkNew<vtkUnsignedCharArray> binaryHeader;
    this->SetBinaryHeader(binaryHeader);
  }
  const int headerSize = 80;                             // fixed in STL file format
  this->BinaryHeader->SetNumberOfValues(headerSize + 1); // allocate +1 byte for zero termination)
  this->BinaryHeader->FillValue(0);
  if (fread(this->BinaryHeader->GetVoidPointer(0), 1, headerSize, fp) != headerSize)
  {
    vtkErrorMacro(
      "STLReader error reading file: " << this->FileName << " Premature EOF while reading header.");
    return false;
  }
  this->SetHeader(static_cast<char*>(this->BinaryHeader->GetVoidPointer(0)));
  // Remove extra zero termination from binary header
  this->BinaryHeader->Resize(headerSize);

  unsigned long ulint;
  if (fread(&ulint, 1, 4, fp) != 4)
  {
    vtkErrorMacro(
      "STLReader error reading file: " << this->FileName << " Premature EOF while reading header.");
    return false;
  }
  vtkByteSwap::Swap4LE(&ulint);

  // Many .stl files contain bogus count.  Hence we will ignore and read
  //   until end of file.
  //
  int numTris = static_cast<int>(ulint);
  if (numTris <= 0)
  {
    vtkDebugMacro(<< "Bad binary count: attempting to correct(" << numTris << ")");
  }

  // Verify the numTris with the length of the file
  unsigned long ulFileLength = vtksys::SystemTools::FileLength(this->FileName);
  ulFileLength -= (80 + 4); // 80 byte - header, 4 byte - tringle count
  ulFileLength /=
    50; // 50 byte - twelve 32-bit-floating point numbers + 2 byte for attribute byte count

  if (numTris < static_cast<int>(ulFileLength))
  {
    numTris = static_cast<int>(ulFileLength);
  }

  // now we can allocate the memory we need for this STL file
  newPts->Allocate(numTris * 3);
  newPolys->AllocateEstimate(numTris, 3);

  facet_t facet;
  for (int i = 0; fread(&facet, 48, 1, fp) > 0; i++)
  {
    unsigned short ibuff2;
    if (fread(&ibuff2, 2, 1, fp) != 1) // read extra junk
    {
      vtkErrorMacro("STLReader error reading file: " << this->FileName
                                                     << " Premature EOF while reading extra junk.");
      return false;
    }

    vtkByteSwap::Swap4LE(facet.n);
    vtkByteSwap::Swap4LE(facet.n + 1);
    vtkByteSwap::Swap4LE(facet.n + 2);

    vtkByteSwap::Swap4LE(facet.v1);
    vtkByteSwap::Swap4LE(facet.v1 + 1);
    vtkByteSwap::Swap4LE(facet.v1 + 2);

    vtkByteSwap::Swap4LE(facet.v2);
    vtkByteSwap::Swap4LE(facet.v2 + 1);
    vtkByteSwap::Swap4LE(facet.v2 + 2);

    vtkByteSwap::Swap4LE(facet.v3);
    vtkByteSwap::Swap4LE(facet.v3 + 1);
    vtkByteSwap::Swap4LE(facet.v3 + 2);

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

// Local Functions
namespace
{
inline std::string stlParseEof(const std::string& expected)
{
  return "Premature EOF while reading '" + expected + "'";
}

inline std::string stlParseExpected(const std::string& expected, const std::string& found)
{
  return "Parse error. Expecting '" + expected + "' found '" + found + "'";
}

// Get three space-delimited floats from string.
bool stlReadVertex(char* buf, float vertCoord[3])
{
  char* begptr = buf;
  char* endptr = nullptr;

  for (int i = 0; i < 3; ++i)
  {
    // We really should use: vertCoord[i] = std::strtof(begptr, &endptr);
    // instead of strtod below but Apple Clang 9.0.0.9000039 doesn't
    // recognize strtof as part of the C++11 standard
    vertCoord[i] = static_cast<float>(std::strtod(begptr, &endptr));
    if (begptr == endptr)
    {
      return false;
    }

    begptr = endptr;
  }

  return true;
}

} // end of anonymous namespace

// https://en.wikipedia.org/wiki/STL_%28file_format%29#ASCII_STL
//
// Format
//
// solid [name]
//
// * where name is an optional string.
// * The file continues with any number of triangles,
//   each represented as follows:
//
// [color ...]
// facet normal ni nj nk
//     outer loop
//         vertex v1x v1y v1z
//         vertex v2x v2y v2z
//         vertex v3x v3y v3z
//     endloop
// endfacet
//
// * where each n or v is a floating-point number.
// * The file concludes with
//
// endsolid [name]

bool vtkSTLReader::ReadASCIISTL(
  FILE* fp, vtkPoints* newPts, vtkCellArray* newPolys, vtkFloatArray* scalars)
{
  vtkDebugMacro(<< "Reading ASCII STL file");

  this->SetHeader(nullptr);
  this->SetBinaryHeader(nullptr);
  std::string header;

  char line[256];     // line buffer
  float vertCoord[3]; // scratch space when parsing "vertex %f %f %f"
  vtkIdType pts[3];   // point ids for building triangles
  int vertOff = 0;

  int solidId = -1;
  int lineNum = 0;

  enum StlAsciiScanState
  {
    scanSolid = 0,
    scanFacet,
    scanLoop,
    scanVerts,
    scanEndLoop,
    scanEndFacet,
    scanEndSolid
  };

  std::string errorMessage;

  for (StlAsciiScanState state = scanSolid; errorMessage.empty() == true; /*nil*/)
  {
    char* cmd = fgets(line, 255, fp);

    if (!cmd)
    {
      // fgets() failed (eg EOF).
      // If scanning for the next "solid" this is a valid way to exit,
      // but is an error if scanning for the initial "solid" or any other token

      switch (state)
      {
        case scanSolid:
        {
          // Emit error if EOF encountered without having read anything
          if (solidId < 0)
            errorMessage = stlParseEof("solid");
          break;
        }
        case scanFacet:
        {
          errorMessage = stlParseEof("facet");
          break;
        }
        case scanLoop:
        {
          errorMessage = stlParseEof("outer loop");
          break;
        }
        case scanVerts:
        {
          errorMessage = stlParseEof("vertex");
          break;
        }
        case scanEndLoop:
        {
          errorMessage = stlParseEof("endloop");
          break;
        }
        case scanEndFacet:
        {
          errorMessage = stlParseEof("endfacet");
          break;
        }
        case scanEndSolid:
        {
          errorMessage = stlParseEof("endsolid");
          break;
        }
      }

      // Terminate the parsing loop
      break;
    }

    // Cue to the first non-space.
    while (isspace(*cmd))
    {
      ++cmd;
    }

    // An empty line - try again
    if (!*cmd)
    {
      // Increment line-number, but not while still in the header
      if (lineNum)
        ++lineNum;
      continue;
    }

    // Ensure consistent case on the first token and separate from
    // subsequent arguments

    char* arg = cmd;
    while (*arg && !isspace(*arg))
    {
      *arg = tolower(*arg);
      ++arg;
    }

    // Terminate first token (cmd)
    if (*arg)
    {
      *arg = '\0';
      ++arg;

      while (isspace(*arg))
      {
        ++arg;
      }
    }

    ++lineNum;

    // Handle all expected parsed elements
    switch (state)
    {
      case scanSolid:
      {
        if (!strcmp(cmd, "solid"))
        {
          ++solidId;
          state = scanFacet; // Next state
          if (!header.empty())
          {
            header += "\n";
          }
          header += arg;
          // strip end-of-line character from the end
          while (!header.empty() && (header.back() == '\r' || header.back() == '\n'))
          {
            header.pop_back();
          }
        }
        else
        {
          errorMessage = stlParseExpected("solid", cmd);
        }
        break;
      }
      case scanFacet:
      {
        if (!strcmp(cmd, "color"))
        {
          // Optional 'color' entry (after solid) - continue looking for 'facet'
          continue;
        }

        if (!strcmp(cmd, "facet"))
        {
          state = scanLoop; // Next state
        }
        else if (!strcmp(cmd, "endsolid"))
        {
          // Finished with 'endsolid' - find next solid
          state = scanSolid;
        }
        else
        {
          errorMessage = stlParseExpected("facet", cmd);
        }
        break;
      }
      case scanLoop:
      {
        if (!strcmp(cmd, "outer")) // More pedantic => && !strcmp(arg, "loop")
        {
          state = scanVerts; // Next state
        }
        else
        {
          errorMessage = stlParseExpected("outer loop", cmd);
        }
        break;
      }
      case scanVerts:
      {
        if (!strcmp(cmd, "vertex"))
        {
          if (stlReadVertex(arg, vertCoord))
          {
            pts[vertOff] = newPts->InsertNextPoint(vertCoord);
            ++vertOff; // Next vertex

            if (vertOff >= 3)
            {
              // Finished this triangle.
              vertOff = 0;
              state = scanEndLoop; // Next state

              // Save as cell
              newPolys->InsertNextCell(3, pts);
              if (scalars)
              {
                scalars->InsertNextValue(solidId);
              }

              if ((newPolys->GetNumberOfCells() % 5000) == 0)
              {
                this->UpdateProgress((newPolys->GetNumberOfCells() % 50000) / 50000.0);
              }
            }
          }
          else
          {
            errorMessage = "Parse error reading STL vertex";
          }
        }
        else
        {
          errorMessage = stlParseExpected("vertex", cmd);
        }
        break;
      }
      case scanEndLoop:
      {
        if (!strcmp(cmd, "endloop"))
        {
          state = scanEndFacet; // Next state
        }
        else
        {
          errorMessage = stlParseExpected("endloop", cmd);
        }
        break;
      }
      case scanEndFacet:
      {
        if (!strcmp(cmd, "endfacet"))
        {
          state = scanFacet; // Next facet, or endsolid
        }
        else
        {
          errorMessage = stlParseExpected("endfacet", cmd);
        }
        break;
      }
      case scanEndSolid:
      {
        if (!strcmp(cmd, "endsolid"))
        {
          state = scanSolid; // Start over again
        }
        else
        {
          errorMessage = stlParseExpected("endsolid", cmd);
        }
        break;
      }
    }
  }

  this->SetHeader(header.c_str());

  if (!errorMessage.empty())
  {
    vtkErrorMacro("STLReader: error while reading file " << this->FileName << " at line " << lineNum
                                                         << ": " << errorMessage);
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
int vtkSTLReader::GetSTLFileType(const char* filename)
{
  vtksys::SystemTools::FileTypeEnum ft = vtksys::SystemTools::DetectFileType(filename);
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

  os << indent << "Merging: " << (this->Merging ? "On\n" : "Off\n");
  os << indent << "ScalarTags: " << (this->ScalarTags ? "On\n" : "Off\n");
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
