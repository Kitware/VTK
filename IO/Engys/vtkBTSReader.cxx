// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2019-2023 Engys Ltd.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBTSReader.h"

#include "vtkByteSwap.h"
#include "vtkCompositeDataSet.h"
#include "vtkErrorCode.h"
#include "vtkFileResourceStream.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtksys/SystemTools.hxx"

#include <set>
#include <sstream>
#include <string>

VTK_ABI_NAMESPACE_BEGIN

static constexpr char BTS_HEADER[] = "ENGYS binary surface format";

vtkStandardNewMacro(vtkBTSReader);

class SolidNames
{
  std::set<std::string> mappedNames;
  std::vector<const std::string*> indexedNames;

public:
  void addReadSolidName(std::string name)
  {
    replaceForbiddenChars(name);
    auto result = mappedNames.insert(name);
    int index = 1;
    while (!result.second)
    {
      result = mappedNames.insert(addIndexToName(name, index));
      index++;
    }
    indexedNames.push_back(result.first.operator->());
  }

  std::string getProcessedSolidName(int index) { return *indexedNames[index]; };

private:
  static void replaceForbiddenChars(std::string& name)
  {
    if (isdigit(name[0]))
    {
      name[0] = '_';
    }
    for (size_t i = 0; i < name.size(); i++)
    {
      switch (name[i])
      {
        case '(':
        case ')':
        case '{':
        case '}':
        case '\\':
        case '/':
        case '#':
        case '$':
        case ';':
        case '<':
        case '>':
        case ',':
        case '@':
        case '!':
        case '%':
        case '^':
        case '*':
        case ' ':
        case '\"':
          name[i] = '_';
          break;
        default:
          break;
      }
    }
  }

  static std::string addIndexToName(const std::string& name, int n)
  {
    std::ostringstream newNameOS;
    newNameOS << name << "_" << n;
    return newNameOS.str();
  }
};

//------------------------------------------------------------------------------
vtkBTSReader::vtkBTSReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
vtkBTSReader::~vtkBTSReader()
{
  delete[] this->FileName;
  this->FileName = nullptr;
}

//------------------------------------------------------------------------------
int vtkBTSReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPartitionedDataSet* output = vtkPartitionedDataSet::GetData(outInfo);

  if (outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
  {
    // we'll only produce data for piece 0, and produce empty datasets on
    // others since splitting a bts is not supported
    return 1;
  }

  if (this->FileName == nullptr)
  {
    vtkErrorMacro(<< "A FileName must be specified.");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return 0;
  }

  bool ownStream = this->Stream == nullptr;
  if (ownStream)
  {
    vtkNew<vtkFileResourceStream> fileStream;
    if (!fileStream->Open(this->FileName))
    {
      vtkErrorMacro(<< "Error opening the file.");
      this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
      return 0;
    }
    this->Stream = fileStream;
  }

  bool success = this->Read(output);

  if (ownStream)
  {
    this->Stream = nullptr;
  }
  this->UpdateProgress(1.0);
  return success;
}

//------------------------------------------------------------------------------
bool vtkBTSReader::Read(vtkPartitionedDataSet* output)
{
  this->InitReadProgress(this->Filesize());

  if (!this->ReadHeader())
  {
    vtkErrorMacro(<< "File type not recognized (invalid header). Exiting.");
    this->SetErrorCode(vtkErrorCode::UnrecognizedFileTypeError);
    return false;
  }

  int numberOfSolids;
  if (!this->ReadArray(&numberOfSolids, 4, 1))
  {
    numberOfSolids = 0;
  }
  vtkByteSwap::Swap4LE(&numberOfSolids);

  SolidNames solidNames;
  if (!this->ReadSolidNames(numberOfSolids, solidNames))
  {
    vtkErrorMacro(
      "Error reading file: " << this->FileName << " Premature EOF while reading solid names.");
    this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
    return false;
  }

  for (int i = 0; i < numberOfSolids; i++)
  {
    vtkNew<vtkPolyData> readPolyData;
    if (this->ReadSolid(readPolyData))
    {
      output->SetPartition(i, readPolyData);
      output->GetMetaData(i)->Set(vtkCompositeDataSet::NAME(), solidNames.getProcessedSolidName(i));
    }
    else
    {
      vtkErrorMacro(
        "Error reading file: " << this->FileName << " Premature EOF while reading solid data.");
      this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkBTSReader::ReadHeader()
{
  char line1[81];
  if (!this->ReadArray(line1, sizeof(char), 80))
  {
    return false;
  }
  return strstr(line1, BTS_HEADER) != nullptr;
}

//------------------------------------------------------------------------------
bool vtkBTSReader::ReadArray(void* ptr, size_t variableSize, size_t nVariables)
{
  size_t bytesToRead = variableSize * nVariables;
  this->UpdateReadProgress(bytesToRead);
  return this->Stream->Read(ptr, bytesToRead) == bytesToRead;
}

//------------------------------------------------------------------------------
long vtkBTSReader::Filesize()
{
  if (Stream->SupportSeek())
  {
    long prevPosition = Stream->Tell();
    this->Stream->Seek(0, vtkResourceStream::SeekDirection::End);
    long endPosition = Stream->Tell();
    this->Stream->Seek(prevPosition, vtkResourceStream::SeekDirection::Begin);
    return endPosition;
  }
  else
  {
    return std::numeric_limits<long>::max();
  }
}

//------------------------------------------------------------------------------
void vtkBTSReader::InitReadProgress(unsigned long fileSize)
{
  this->ReadBytes = 0;
  this->PreviousPercentProgress = 0;
  this->FileSize = fileSize;
}

//------------------------------------------------------------------------------
void vtkBTSReader::UpdateReadProgress(size_t bytes)
{
  this->ReadBytes += bytes;
  int currentPercentProgress = static_cast<int>(this->ReadBytes * 100 / this->FileSize);
  if (currentPercentProgress > this->PreviousPercentProgress)
  {
    this->PreviousPercentProgress = currentPercentProgress;
    double value = static_cast<double>(currentPercentProgress) / 100;
    this->UpdateProgress(value);
  }
}

//------------------------------------------------------------------------------
bool vtkBTSReader::ReadSolidNames(int numberOfSolids, SolidNames& solidNames)
{
  for (int i = 0; i < numberOfSolids; i++)
  {
    static char solidName[161];
    if (!this->ReadArray(solidName, sizeof(char), 160))
    {
      return false;
    }
    solidName[160] = 0;
    solidNames.addReadSolidName(solidName);
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkBTSReader::ReadSolid(vtkPolyData* polyData)
{
  if (!this->ReadPoints(polyData))
  {
    return false;
  }
  if (!this->ReadFaces(polyData))
  {
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkBTSReader::ReadPoints(vtkPolyData* polyData)
{
  uint32_t numberOfPoints = this->ReadUint32Value();

  if (numberOfPoints == UINT32_MAX)
  {
    return false;
  }

  vtkNew<vtkPoints> points;
  points->SetDataTypeToFloat();
  points->Allocate(numberOfPoints);
  points->SetNumberOfPoints(numberOfPoints);

  float coordinates[3];
  for (uint32_t i = 0; i < numberOfPoints; i++)
  {
    if (!this->ReadArray(coordinates, 4, 3))
    {
      return false;
    }

    vtkByteSwap::Swap4LE(coordinates);
    vtkByteSwap::Swap4LE(coordinates + 1);
    vtkByteSwap::Swap4LE(coordinates + 2);

    points->SetPoint(i, coordinates[0], coordinates[1], coordinates[2]);
  }

  polyData->SetPoints(points);

  return true;
}

//------------------------------------------------------------------------------
uint32_t vtkBTSReader::ReadUint32Value()
{
  uint32_t value;
  if (this->Stream->Read(&value, sizeof(value)) != sizeof(value))
  {
    return UINT32_MAX;
  }
  vtkByteSwap::Swap4LE(&value);

  return value;
}

//------------------------------------------------------------------------------
bool vtkBTSReader::ReadFaces(vtkPolyData* polyData)
{
  uint32_t numberOfFaces = this->ReadUint32Value();

  if (numberOfFaces == UINT32_MAX)
  {
    return false;
  }

  vtkNew<vtkCellArray> polys;
  polys->Allocate(numberOfFaces);

  int pointsIds[3];
  vtkIdType vtkPointsIds[3];
  for (uint32_t i = 0; i < numberOfFaces; i++)
  {
    if (!this->ReadArray(pointsIds, 4, 3))
    {
      return false;
    }

    vtkPointsIds[0] = pointsIds[0];
    vtkPointsIds[1] = pointsIds[1];
    vtkPointsIds[2] = pointsIds[2];

    vtkByteSwap::Swap4LE(vtkPointsIds);
    vtkByteSwap::Swap4LE(vtkPointsIds + 1);
    vtkByteSwap::Swap4LE(vtkPointsIds + 2);

    polys->InsertNextCell(3, vtkPointsIds);
  }

  polyData->SetPolys(polys);

  return true;
}

//------------------------------------------------------------------------------
void vtkBTSReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "File Name: " << (this->FileName == nullptr ? "(none)" : this->FileName)
     << std::endl;
  os << indent << "Stream: " << std::endl;
  if (this->Stream)
  {
    this->Stream->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "is nullptr" << std::endl;
  }
}

//------------------------------------------------------------------------------
const char* vtkBTSReader::GetRegistrationName()
{
  this->RegistrationName = vtksys::SystemTools::GetFilenameWithoutExtension(FileName);
  return this->RegistrationName.c_str();
}

//------------------------------------------------------------------------------
void vtkBTSReader::SetStream(vtkResourceStream* stream)
{
  if (this->Stream != stream)
  {
    this->Stream = stream;
    this->Modified();
  }
}

vtkResourceStream* vtkBTSReader::GetStream()
{
  return this->Stream;
}

VTK_ABI_NAMESPACE_END
