// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSTLReader.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkErrorCode.h"
#include "vtkFileResourceStream.h"
#include "vtkFloatArray.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergePoints.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkResourceParser.h"
#include "vtkResourceStream.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringScanner.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vtksys/SystemTools.hxx>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
// fixed in STL file format
constexpr int STL_HEADER_SIZE = 80;

// twelve 32-bit-floating point numbers + 2 byte for attribute byte count = 50 bytes.
constexpr vtkTypeInt64 STL_TRI_SIZE = 12 * sizeof(float) + sizeof(uint16_t);
}

vtkStandardNewMacro(vtkSTLReader);

vtkCxxSetObjectMacro(vtkSTLReader, Locator, vtkIncrementalPointLocator);
vtkCxxSetObjectMacro(vtkSTLReader, BinaryHeader, vtkUnsignedCharArray);

//------------------------------------------------------------------------------
vtkSTLReader::vtkSTLReader() = default;

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

  if (!this->Stream && (!this->FileName || *this->FileName == 0))
  {
    vtkErrorMacro(<< "A FileName or stream must be specified.");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return 0;
  }

  vtkResourceStream* stream = this->Stream;
  vtkNew<vtkFileResourceStream> fileStream;
  if (stream)
  {
    stream->Seek(0, vtkResourceStream::SeekDirection::Begin);
  }
  else
  {
    if (!fileStream->Open(this->FileName))
    {
      vtkErrorMacro("Unable to open " << this->FileName << " . Aborting.");
      this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
      return 0;
    }
    stream = fileStream;
  }

  std::string solid;
  vtkNew<vtkResourceParser> asciiTester;
  asciiTester->SetStream(stream);
  asciiTester->ReadLine(solid, 5);
  stream->Seek(0, vtkResourceStream::SeekDirection::Begin);

  vtkNew<vtkPoints> newPts;
  vtkNew<vtkCellArray> newPolys;
  vtkSmartPointer<vtkFloatArray> newScalars;

  if (solid == "solid")
  {
    // First word is "solid", which means the data should be ASCII.
    newPts->Allocate(5000);
    newPolys->AllocateEstimate(10000, 1);
    if (this->ScalarTags)
    {
      newScalars = vtkSmartPointer<vtkFloatArray>::New();
      newScalars->Allocate(5000);
    }

    vtkNew<vtkResourceParser> parser;
    parser->SetStream(stream);
    if (!this->ReadASCIISTL(parser, newPts.Get(), newPolys.Get(), newScalars))
    {
      // In relaxed mode, fallback to try reading as binary (because we have seen malformed STL
      // files in the wild that have the 80 byte header but start with `solid`).
      if (this->GetRelaxedConformance())
      {
        stream->Seek(0, vtkResourceStream::SeekDirection::Begin);
        if (!this->ReadBinarySTL(stream, newPts.Get(), newPolys.Get()))
        {
          vtkErrorMacro("Fallback reading as binary STL failed too. Aborting.");
          return 0;
        }
      }
    }
  }
  else
  {
    if (!this->ReadBinarySTL(stream, newPts.Get(), newPolys.Get()))
    {
      vtkErrorMacro("Error reading a binary STL. Aborting.");
      return 0;
    }
  }

  vtkDebugMacro(<< "Read: " << newPts->GetNumberOfPoints() << " points, "
                << newPolys->GetNumberOfCells() << " triangles");

  // If merging is on, create hash table and merge points/triangles.
  vtkSmartPointer<vtkPoints> mergedPts = newPts;
  vtkSmartPointer<vtkCellArray> mergedPolys = newPolys;
  vtkSmartPointer<vtkFloatArray> mergedScalars = newScalars;
  if (this->Merging)
  {
    mergedPts = vtkSmartPointer<vtkPoints>::New();
    mergedPts->Allocate(newPts->GetNumberOfPoints() / 2);
    mergedPolys = vtkSmartPointer<vtkCellArray>::New();
    mergedPolys->AllocateCopy(newPolys);
    if (newScalars)
    {
      mergedScalars = vtkSmartPointer<vtkFloatArray>::New();
      mergedScalars->Allocate(newPolys->GetNumberOfCells());
    }

    vtkSmartPointer<vtkIncrementalPointLocator> locator = this->Locator;
    if (this->Locator == nullptr)
    {
      locator.TakeReference(this->NewDefaultLocator());
    }
    locator->InitPointInsertion(mergedPts, newPts->GetBounds());

    vtkIdType nextCell = 0;
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

    vtkDebugMacro(<< "Merged to: " << mergedPts->GetNumberOfPoints() << " points, "
                  << mergedPolys->GetNumberOfCells() << " triangles");
  }

  output->SetPoints(mergedPts);
  output->SetPolys(mergedPolys);

  if (mergedScalars)
  {
    mergedScalars->SetName("STLSolidLabeling");
    output->GetCellData()->SetScalars(mergedScalars);
  }

  if (this->Locator)
  {
    this->Locator->Initialize(); // free storage
  }

  output->Squeeze();

  return 1;
}

//------------------------------------------------------------------------------
bool vtkSTLReader::ReadBinaryTrisField(vtkResourceStream* stream, uint32_t& numTrisField)
{
  if (stream->Read(&numTrisField, sizeof(numTrisField)) != sizeof(numTrisField))
  {
    return false;
  }
  vtkByteSwap::Swap4LE(&numTrisField);
  return true;
}

//------------------------------------------------------------------------------
bool vtkSTLReader::ReadBinaryTrisFile(vtkResourceStream* stream, vtkTypeInt64& numTrisFile)
{
  // How many bytes are remaining in the file?
  vtkTypeInt64 current = stream->Tell();
  vtkTypeInt64 ulFileLength = stream->Seek(0, vtkResourceStream::SeekDirection::End);
  stream->Seek(current, vtkResourceStream::SeekDirection::Begin);
  ulFileLength -= ::STL_HEADER_SIZE + sizeof(uint32_t); // 80 byte - header, 4 byte - triangle count
  if (ulFileLength < 0 || ulFileLength % ::STL_TRI_SIZE != 0)
  {
    return false;
  }
  numTrisFile = ulFileLength / ::STL_TRI_SIZE;
  return true;
}

//------------------------------------------------------------------------------
bool vtkSTLReader::ReadBinarySTL(
  vtkResourceStream* stream, vtkPoints* newPts, vtkCellArray* newPolys)
{
  struct facet_t_t
  {
    float n[3], v1[3], v2[3], v3[3];
    uint16_t attrByteCount;
  };
  using facet_t = struct facet_t_t;

  vtkDebugMacro(<< "Reading BINARY STL file");

  //  File is read to obtain raw information as well as bounding box
  //
  if (!this->BinaryHeader)
  {
    vtkNew<vtkUnsignedCharArray> binaryHeader;
    this->SetBinaryHeader(binaryHeader);
  }

  // The 80 byte header need not be a null-terminated string, so allocate +1 byte for null
  // termination.
  this->BinaryHeader->SetNumberOfValues(::STL_HEADER_SIZE + 1);

  // Zero fill everything so that null termination is guaranteed.
  this->BinaryHeader->FillValue(0);

  if (stream->Read(this->BinaryHeader->GetPointer(0), ::STL_HEADER_SIZE) != ::STL_HEADER_SIZE)
  {
    vtkErrorMacro("STLReader error reading file. Premature EOF while reading header.");
    return false;
  }

  // Even though this is a binary file, provide the header as a C string also.
  this->SetHeader(reinterpret_cast<char*>(this->BinaryHeader->GetPointer(0)));

  // Remove the extra NULL termination from the binary header.
  this->BinaryHeader->Resize(::STL_HEADER_SIZE);

  uint32_t numTrisField;
  if (!vtkSTLReader::ReadBinaryTrisField(stream, numTrisField))
  {
    vtkErrorMacro("STLReader error reading file. Premature EOF while reading triangle count.");
    return false;
  }

  vtkTypeInt64 numTrisFile;
  if (!vtkSTLReader::ReadBinaryTrisFile(stream, numTrisFile))
  {
    vtkErrorMacro("STLReader error reading file. Remaining file length bad.");
    return false;
  }

  // Many .stl files contain bogus triangle count. Let's compare to the remaining file size. If
  // we're being strict, they should match.
  if (numTrisFile != numTrisField && !this->GetRelaxedConformance())
  {
    vtkErrorMacro("STLReader error reading file. Triangle count / file size mismatch.");
    return false;
  }

  // now allocate the memory we need for the triangles.
  // note we ignore the triangle count field and read until end of file.
  newPts->Allocate(numTrisFile * 3);
  newPolys->AllocateEstimate(numTrisFile, 3);

  facet_t facet;
  for (size_t i = 0; stream->Read(&facet, ::STL_TRI_SIZE) > 0; ++i)
  {
    vtkByteSwap::Swap4LE(facet.n);
    vtkByteSwap::Swap4LE(facet.n + 1);
    vtkByteSwap::Swap4LE(facet.n + 2);
    if (!std::isfinite(facet.n[0]) || !std::isfinite(facet.n[1]) || !std::isfinite(facet.n[2]))
    {
      vtkErrorMacro("Normal vector non-finite.");
      return false;
    }

    vtkByteSwap::Swap4LE(facet.v1);
    vtkByteSwap::Swap4LE(facet.v1 + 1);
    vtkByteSwap::Swap4LE(facet.v1 + 2);
    if (!std::isfinite(facet.v1[0]) || !std::isfinite(facet.v1[1]) || !std::isfinite(facet.v1[2]))
    {
      vtkErrorMacro("vertex 1 non-finite.");
      return false;
    }

    vtkByteSwap::Swap4LE(facet.v2);
    vtkByteSwap::Swap4LE(facet.v2 + 1);
    vtkByteSwap::Swap4LE(facet.v2 + 2);
    if (!std::isfinite(facet.v2[0]) || !std::isfinite(facet.v2[1]) || !std::isfinite(facet.v2[2]))
    {
      vtkErrorMacro("vertex 2 non-finite.");
      return false;
    }

    vtkByteSwap::Swap4LE(facet.v3);
    vtkByteSwap::Swap4LE(facet.v3 + 1);
    vtkByteSwap::Swap4LE(facet.v3 + 2);
    if (!std::isfinite(facet.v3[0]) || !std::isfinite(facet.v3[1]) || !std::isfinite(facet.v3[2]))
    {
      vtkErrorMacro("vertex 3 non-finite.");
      return false;
    }

    vtkIdType pts[3];
    pts[0] = newPts->InsertNextPoint(facet.v1);
    pts[1] = newPts->InsertNextPoint(facet.v2);
    pts[2] = newPts->InsertNextPoint(facet.v3);

    newPolys->InsertNextCell(3, pts);

    if ((i % 100000) == 0 && i != 0)
    {
      vtkDebugMacro(<< "triangle# " << i);
      this->UpdateProgress(static_cast<double>(i) / numTrisFile);
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
  std::string_view buffer = buf;

  for (int i = 0; i < 3; ++i)
  {
    auto result = vtk::scan_value<float>(buffer);
    if (!result)
    {
      return false;
    }
    vertCoord[i] = result->value();
    buffer = result->range().data();
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
  vtkResourceParser* parser, vtkPoints* newPts, vtkCellArray* newPolys, vtkFloatArray* scalars)
{
  vtkDebugMacro(<< "Reading ASCII STL file");

  this->SetHeader(nullptr);
  this->SetBinaryHeader(nullptr);
  std::string header;

  std::string line;   // line buffer
  float vertCoord[3]; // scratch space when parsing "vertex %f %f %f"
  vtkIdType pts[3];   // point ids for building triangles
  int vertOff = 0;

  int solidId = -1;
  size_t lineNum = 0;

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

  for (StlAsciiScanState state = scanSolid; errorMessage.empty(); /*nil*/)
  {
    vtkParseResult res = parser->ReadLine(line);
    char* cmd = line.data();
    if (res == vtkParseResult::EndOfStream)
    {
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
          if (*arg)
          {
            header += arg;
            // strip end-of-line character from the end
            while (!header.empty() && (header.back() == '\r' || header.back() == '\n'))
            {
              header.pop_back();
            }
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
    vtkDebugMacro("STLReader: unable to read line " << lineNum << ": " << errorMessage);
    return false;
  }

  return true;
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

  os << indent << "RelaxedConformance: " << (this->RelaxedConformance ? "On\n" : "Off\n");
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

//------------------------------------------------------------------------------
bool vtkSTLReader::CanReadFile(const char* filename)
{
  vtkNew<vtkFileResourceStream> stream;
  if (!stream->Open(filename))
  {
    return false;
  }
  return vtkSTLReader::CanReadFile(stream);
}

//------------------------------------------------------------------------------
bool vtkSTLReader::CanReadFile(vtkResourceStream* stream)
{
  if (!stream)
  {
    return false;
  }

  stream->Seek(0, vtkResourceStream::SeekDirection::Begin);
  vtkNew<vtkResourceParser> asciiTester;
  asciiTester->SetStream(stream);

  std::string solid;
  if (asciiTester->ReadLine(solid, 5) != vtkParseResult::Limit)
  {
    return false;
  }

  if (solid != "solid")
  {
    // Skip binary header
    stream->Seek(::STL_HEADER_SIZE, vtkResourceStream::SeekDirection::Begin);

    uint32_t numTrisField;
    if (!vtkSTLReader::ReadBinaryTrisField(stream, numTrisField))
    {
      return false;
    }

    vtkTypeInt64 numTrisFile;
    if (!vtkSTLReader::ReadBinaryTrisFile(stream, numTrisFile))
    {
      return false;
    }

    if (numTrisFile != numTrisField)
    {
      return false;
    }
  }
  return true;
}

VTK_ABI_NAMESPACE_END
