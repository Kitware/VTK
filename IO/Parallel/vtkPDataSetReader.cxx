/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDataSetReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPDataSetReader.h"

#include "vtkAppendFilter.h"
#include "vtkAppendPolyData.h"
#include "vtkCellData.h"
#include "vtkDataSetReader.h"
#include "vtkExtentTranslator.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridReader.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsReader.h"
#include "vtkUnstructuredGrid.h"
#include "vtksys/Encoding.hxx"
#include "vtksys/FStream.hxx"

#include <vector>

vtkStandardNewMacro(vtkPDataSetReader);

//----------------------------------------------------------------------------
vtkPDataSetReader::vtkPDataSetReader()
{
  this->FileName = nullptr;
  this->VTKFileFlag = 0;
  this->StructuredFlag = 0;
  this->NumberOfPieces = 0;
  this->DataType = -1;
  this->PieceFileNames = nullptr;
  this->PieceExtents = nullptr;
  this->SetNumberOfOutputPorts(1);
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkPDataSetReader::~vtkPDataSetReader()
{
  delete[] this->FileName;
  this->SetNumberOfPieces(0);
}

//----------------------------------------------------------------------------
void vtkPDataSetReader::SetNumberOfPieces(int num)
{
  int i;

  if (this->NumberOfPieces == num)
  {
    return;
  }

  // Delete the previous file names/extents.
  for (i = 0; i < this->NumberOfPieces; ++i)
  {
    delete[] this->PieceFileNames[i];
    this->PieceFileNames[i] = nullptr;
    if (this->PieceExtents && this->PieceExtents[i])
    {
      delete[] this->PieceExtents[i];
      this->PieceExtents[i] = nullptr;
    }
  }
  delete[] this->PieceFileNames;
  this->PieceFileNames = nullptr;
  delete[] this->PieceExtents;
  this->PieceExtents = nullptr;
  this->NumberOfPieces = 0;

  if (num <= 0)
  {
    return;
  }

  // Allocate new arrays
  this->PieceFileNames = new char*[num];
  for (i = 0; i < num; ++i)
  {
    this->PieceFileNames[i] = new char[512];
  }
  // Allocate piece extents even for unstructured data.
  this->PieceExtents = new int*[num];
  for (i = 0; i < num; ++i)
  {
    this->PieceExtents[i] = new int[6];
  }

  this->NumberOfPieces = num;
}

//----------------------------------------------------------------------------
int vtkPDataSetReader::RequestDataObject(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{

  istream* file;
  char* block;
  char* param;
  char* value;
  int type;

  // Start reading the meta-data pvtk file.
  file = this->OpenFile(this->FileName);
  if (file == nullptr)
  {
    return 0;
  }

  type = this->ReadXML(file, &block, &param, &value);
  if (type == 1 && strcmp(block, "File") == 0)
  {
    this->ReadPVTKFileInformation(file, request, inputVector, outputVector);
    this->VTKFileFlag = 0;
  }
  else if (type == 4 && strncmp(value, "# vtk DataFile Version", 22) == 0)
  {
    // This is a vtk file not a PVTK file.
    this->ReadVTKFileInformation(request, inputVector, outputVector);
    this->VTKFileFlag = 1;
  }
  else
  {
    vtkErrorMacro("This does not look like a VTK file: " << this->FileName);
  }

  delete file;
  file = nullptr;

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataSet* output = vtkDataSet::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));

  if (output && output->GetDataObjectType() == this->DataType)
  {
    return 1;
  }

  vtkDataSet* newOutput = nullptr;
  switch (this->DataType)
  {
    case VTK_POLY_DATA:
      newOutput = vtkPolyData::New();
      break;
    case VTK_UNSTRUCTURED_GRID:
      newOutput = vtkUnstructuredGrid::New();
      break;
    case VTK_STRUCTURED_GRID:
      newOutput = vtkStructuredGrid::New();
      break;
    case VTK_RECTILINEAR_GRID:
      newOutput = vtkRectilinearGrid::New();
      break;
    case VTK_IMAGE_DATA:
      newOutput = vtkImageData::New();
      break;
    case VTK_STRUCTURED_POINTS:
      newOutput = vtkImageData::New();
      break;
    default:
      vtkErrorMacro("Unknown data type.");
      return 0;
  }

  if (output)
  {
    vtkWarningMacro("Creating a new output of type " << newOutput->GetClassName());
  }

  info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
  newOutput->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// Returns 0 for end of file.
// Returns 1 for start block,
// Returns 2 for parameter-value pair (occurs after 1 but before 3).
// Returns 3 for termination of start block.
// Returns 4 for string inside block.  Puts string in retVal. (param = nullptr)
// Returns 5 for end block.
// =======
// The statics should be instance variables ...
int vtkPDataSetReader::ReadXML(istream* file, char** retBlock, char** retParam, char** retVal)
{
  static char str[1024];
  static char* ptr = nullptr;
  static char block[256];
  static char param[256];
  static char value[512];
  // I could keep track of the blocks on a stack, but I don't need to.
  static int inStartBlock = 0;
  char* tmp;

  // Initialize the strings.
  if (ptr == nullptr)
  {
    block[0] = param[0] = value[0] = '\0';
  }

  // Skip white space
  // We could do this with a get char ...
  while (ptr == nullptr || *ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\0')
  {
    if (ptr == nullptr || *ptr == '\0')
    { // At the end of a line.  Read another.
      file->getline(str, 1024);
      if (file->fail())
      {
        *retBlock = nullptr;
        *retParam = nullptr;
        *retVal = nullptr;
        return 0;
      }
      str[1023] = '\0';
      ptr = str;
    }
    else
    {
      ++ptr;
    }
  }

  // Handle normal end block.  </Block>
  if (!inStartBlock && ptr[0] == '<' && ptr[1] == '/')
  { // Assumes no spaces
    ptr += 2;
    // We could check to see if the block name matches the start block...
    // Copy block name into block var.
    tmp = block;
    while (*ptr != '>' && *ptr != ' ' && *ptr != '\0')
    {
      *tmp++ = *ptr++;
    }
    *tmp = '\0';
    // Now scan to the end of the end block.
    while (*ptr != '>' && *ptr != '\0')
    {
      *tmp++ = *ptr++;
    }
    *retBlock = block;
    *retParam = nullptr;
    *retVal = nullptr;
    if (*ptr == '\0')
    {
      vtkErrorMacro("Newline in end block.");
      return 0;
    }
    return 5;
  }

  // Handle start block. <Block>
  if (!inStartBlock && ptr[0] == '<')
  { // Assumes no spaces
    ptr += 1;
    // Copy block name from read string.
    tmp = block;
    while (*ptr != '>' && *ptr != ' ' && *ptr != '\0')
    {
      *tmp++ = *ptr++;
    }
    *tmp = '\0';
    inStartBlock = 1;
    *retBlock = block;
    *retParam = nullptr;
    *retVal = nullptr;
    return 1;
  }

  // Handle the termination of a start block.
  if (inStartBlock && *ptr == '>')
  {
    ++ptr;
    inStartBlock = 0;
    *retBlock = block;
    *retParam = nullptr;
    *retVal = nullptr;
    return 3;
  }

  // Handle short version of end block. <Block    ...  />
  // Now we want to return twice.
  // First for termination of the start block,
  // and second for ending of the block.
  // This implementation uses in start block as a state variable ...
  if (inStartBlock && ptr[0] == '/' && ptr[1] == '>')
  {
    if (inStartBlock == 2)
    { // Second pass: Return end block.
      ptr += 2;
      inStartBlock = 0;
      *retBlock = block;
      *retParam = nullptr;
      *retVal = nullptr;
      return 5;
    }
    // First pass: inStartBlock == 1.  Return Terminate start block.
    // Uses block name saved from start block.
    // Do not skip over the '/>' characters.
    inStartBlock = 2;
    *retBlock = block;
    *retParam = nullptr;
    *retVal = nullptr;
    return 3;
  }

  // If we are not in a start block, we will just return the string verbatim.
  if (!inStartBlock)
  {
    // Copy string to value string.
    tmp = value;
    while (*ptr != '\0')
    {
      *tmp++ = *ptr++;
    }
    *tmp = '\0';
    // We do not return the block because we do not have a block stack,
    // so cannot be sure what the block is.
    *retBlock = nullptr;
    *retParam = nullptr;
    *retVal = value;
    return 4;
  }

  // Must be a parameter
  tmp = param;
  while (*ptr != '=' && *ptr != '\0')
  {
    *tmp++ = *ptr++;
  }
  // Terminate the parameter string.
  *tmp = '\0';
  // Expect an equals sign immediately after parameter string (no spaces).
  if (*ptr != '=')
  {
    vtkErrorMacro("Reached end of line before =");
    return 0;
  }
  // skip over = sign.
  ++ptr;
  if (*ptr != '"')
  {
    vtkErrorMacro("Expecting parameter value to be in quotes.");
    return 0;
  }
  ++ptr;
  tmp = value;
  while (*ptr != '"' && *ptr != '\0')
  {
    *tmp++ = *ptr++;
  }
  // Terminate the value string
  *tmp = '\0';
  if (*ptr != '"')
  {
    vtkErrorMacro("Newline found in parameter string.");
    return 0;
  }
  // Skip over the last quote
  ++ptr;

  *retBlock = block;
  *retParam = param;
  *retVal = value;
  return 2;
}

//----------------------------------------------------------------------------
int vtkPDataSetReader::CanReadFile(const char* filename)
{
  istream* file;
  char* block;
  char* param;
  char* value;
  int type;
  int flag = 0;

  // Start reading the meta-data pvtk file.
  file = this->OpenFile(filename);
  if (file == nullptr)
  {
    return 0;
  }

  type = this->ReadXML(file, &block, &param, &value);
  if (type == 1 && strcmp(block, "File") == 0)
  {
    // We cannot leave the XML parser in a bad state.
    // As a quick fix, read to the end of the file block.
    // A better solution would be to move statics
    // to ivars and initialize them as needed.
    while (this->ReadXML(file, &block, &param, &value) != 5)
    {
    }
    flag = 1;
  }

  if (type == 4 && strncmp(value, "# vtk DataFile Version", 22) == 0)
  {
    // This is a vtk file.
    vtkDataSetReader* tmp = vtkDataSetReader::New();
    tmp->SetFileName(filename);
    type = tmp->ReadOutputType();
    if (type != -1)
    {
      flag = 1;
    }
    tmp->Delete();
  }

  delete file;
  return flag;
}

//----------------------------------------------------------------------------
void vtkPDataSetReader::ReadPVTKFileInformation(
  istream* file, vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  char* block;
  char* param;
  char* val;
  int type;
  int ext[6];
  double vect[3];
  int i;
  char *pfn, *pdir;
  int count, dirLength;
  char dir[512];

  vtkInformation* info = outputVector->GetInformationObject(0);

  // The file block should have a version parameter.
  type = this->ReadXML(file, &block, &param, &val);
  if (type != 2 || strcmp(param, "version"))
  {
    vtkErrorMacro("Could not find file version.");
    return;
  }
  if (strcmp(val, "pvtk-1.0") != 0)
  {
    vtkDebugMacro("Unexpected Version.");
  }

  // Extract the directory form the filename so we can complete relative paths.
  count = dirLength = 0;
  pfn = this->FileName;
  pdir = dir;
  // Copy filename to dir, and keep track of the last slash.
  while (*pfn != '\0' && count < 512)
  {
    *pdir++ = *pfn++;
    ++count;
    if (*pfn == '/' || *pfn == '\\')
    {
      // The extra +1 is to keep the last slash.
      dirLength = count + 1;
    }
  }
  // This trims off every thing after the last slash.
  dir[dirLength] = '\0';

  // We are in the start file block.
  // Read parameters until we terminate the start block.
  while ((type = this->ReadXML(file, &block, &param, &val)) != 3)
  {
    if (type == 0)
    {
      vtkErrorMacro("Early termination of pvtk file.");
      return;
    }
    if (type != 2)
    { // There should be no other possibility. Param will not be nullptr.
      vtkErrorMacro("Expecting a parameter.");
      return;
    }

    // Handle parameter: numberOfPieces.
    if (strcmp(param, "numberOfPieces") == 0)
    {
      this->SetNumberOfPieces(atoi(val));
    }

    // Handle parameter: wholeExtent.
    if (strcmp(param, "wholeExtent") == 0)
    {
      if (!this->StructuredFlag)
      {
        vtkWarningMacro("Extent mismatch.");
      }
      sscanf(val, "%d %d %d %d %d %d", ext, ext + 1, ext + 2, ext + 3, ext + 4, ext + 5);
      info->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext, 6);
    }

    // Handle parameter: scalarType.
    if (strcmp(param, "scalarType") == 0)
    {
      vtkDataObject::SetPointDataActiveScalarInfo(info, atoi(val), -1);
    }

    // Handle parameter: spacing.
    if (strcmp(param, "spacing") == 0)
    {
      sscanf(val, "%lf %lf %lf", vect, vect + 1, vect + 2);
      info->Set(vtkDataObject::SPACING(), vect, 3);
    }

    // Handle parameter: origin.
    if (strcmp(param, "origin") == 0)
    {
      sscanf(val, "%lf %lf %lf", vect, vect + 1, vect + 2);
      info->Set(vtkDataObject::ORIGIN(), vect, 3);
    }

    // Handle parameter: dataType.
    if (strcmp(param, "dataType") == 0)
    {
      if (strcmp(val, "vtkPolyData") == 0)
      {
        this->DataType = VTK_POLY_DATA;
        this->StructuredFlag = 0;
      }
      else if (strcmp(val, "vtkUnstructuredGrid") == 0)
      {
        this->DataType = VTK_UNSTRUCTURED_GRID;
        this->StructuredFlag = 0;
      }
      else if (strcmp(val, "vtkStructuredGrid") == 0)
      {
        this->DataType = VTK_STRUCTURED_GRID;
        this->StructuredFlag = 1;
      }
      else if (strcmp(val, "vtkRectilinearGrid") == 0)
      {
        this->DataType = VTK_RECTILINEAR_GRID;
        this->StructuredFlag = 1;
      }
      else if (strcmp(val, "vtkImageData") == 0 || strcmp(val, "vtkStructuredPoints") == 0)
      {
        this->DataType = VTK_IMAGE_DATA;
        this->StructuredFlag = 1;
      }
      else
      {
        vtkErrorMacro("Unknown data type " << val);
        return;
      }
    }
  }

  // Read the filename and extents for each piece.
  for (i = 0; i < this->NumberOfPieces; ++i)
  {
    int* pi = this->PieceExtents[i];
    // Initialize extent to nothing.
    pi[0] = pi[2] = pi[4] = 0;
    pi[1] = pi[3] = pi[5] = -1;

    // Read the start tag of the Piece block.
    type = this->ReadXML(file, &block, &param, &val);
    if (type != 1 || strcmp(block, "Piece") != 0)
    {
      vtkErrorMacro("Expecting the start of a 'Piece' block");
      return;
    }
    while ((type = this->ReadXML(file, &block, &param, &val)) != 3)
    {
      if (type != 2)
      { // There should be no other possibility. Param will not be nullptr.
        vtkErrorMacro("Expecting a parameter.");
        return;
      }

      // Handle the file name parameter.
      if (strcmp(param, "fileName") == 0)
      {
        // Copy filename (relative path?)
        if (val[0] != '/' && val[1] != ':' && dirLength > 0)
        { // Must be a relative path.
          snprintf(this->PieceFileNames[i], 512, "%s%s", dir, val);
        }
        else
        {
          strcpy(this->PieceFileNames[i], val);
        }
      }

      // Handle the extent parameter.
      if (strcmp(param, "extent") == 0)
      {
        if (!this->StructuredFlag)
        {
          vtkWarningMacro("Found extent parameter for unstructured data.");
        }
        sscanf(val, "%d %d %d %d %d %d", pi, pi + 1, pi + 2, pi + 3, pi + 4, pi + 5);
      }
    }
    // Start termination was consumed by while loop.

    // Now read the ending piece block.
    type = this->ReadXML(file, &block, &param, &val);
    if (type != 5 || strcmp(block, "Piece") != 0)
    {
      vtkErrorMacro("Expecting termination of the Piece block.");
      return;
    }
  }
}

//----------------------------------------------------------------------------
void vtkPDataSetReader::ReadVTKFileInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);

  vtkNew<vtkDataSetReader> reader;
  reader->SetFileName(this->FileName);
  reader->UpdateInformation();
  if (auto dobj = reader->GetOutputDataObject(0))
  {
    this->DataType = dobj->GetDataObjectType();
    using sddp = vtkStreamingDemandDrivenPipeline;
    info->CopyEntry(reader->GetOutputInformation(0), sddp::WHOLE_EXTENT(), 1);
    info->CopyEntry(reader->GetOutputInformation(0), vtkDataObject::SPACING(), 1);
    info->CopyEntry(reader->GetOutputInformation(0), vtkDataObject::ORIGIN(), 1);
  }
  else
  {
    vtkErrorMacro("I can not figure out what type of data set this is");
  }
}

//----------------------------------------------------------------------------
istream* vtkPDataSetReader::OpenFile(const char* filename)
{
  vtksys::ifstream* file;

  if (!filename || filename[0] == '\0')
  {
    vtkDebugMacro(<< "A FileName must be specified.");
    return nullptr;
  }

  file = new vtksys::ifstream(filename);
  if (!file || file->fail())
  {
    delete file;
    vtkErrorMacro(<< "Initialize: Could not open file " << filename);
    return nullptr;
  }

  return file;
}

//----------------------------------------------------------------------------
int vtkPDataSetReader::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkPDataSetReader::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataSet* output = vtkDataSet::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));

  if (this->VTKFileFlag)
  {
    int updatePiece = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    if (updatePiece == 0)
    {
      vtkDataSetReader* reader = vtkDataSetReader::New();
      reader->ReadAllScalarsOn();
      reader->ReadAllVectorsOn();
      reader->ReadAllNormalsOn();
      reader->ReadAllTensorsOn();
      reader->ReadAllColorScalarsOn();
      reader->ReadAllTCoordsOn();
      reader->ReadAllFieldsOn();
      reader->SetFileName(this->FileName);
      reader->Update();
      vtkDataSet* data = reader->GetOutput();

      if (data == nullptr)
      {
        vtkErrorMacro("Could not read file: " << this->FileName);
        return 0;
      }

      if (data->CheckAttributes())
      {
        vtkErrorMacro("Attribute Mismatch.");
        return 0;
      }

      output->CopyStructure(data);
      output->GetFieldData()->PassData(data->GetFieldData());
      output->GetCellData()->PassData(data->GetCellData());
      output->GetPointData()->PassData(data->GetPointData());
      this->SetNumberOfPieces(0);

      reader->Delete();
    }
    return 1;
  }

  switch (this->DataType)
  {
    case VTK_POLY_DATA:
      return this->PolyDataExecute(request, inputVector, outputVector);
    case VTK_UNSTRUCTURED_GRID:
      return this->UnstructuredGridExecute(request, inputVector, outputVector);
    case VTK_IMAGE_DATA:
      return this->ImageDataExecute(request, inputVector, outputVector);
    case VTK_STRUCTURED_GRID:
      return this->StructuredGridExecute(request, inputVector, outputVector);
    default:
      vtkErrorMacro("We do not handle vtkRectilinear yet.");
  }

  return 0;
}

//----------------------------------------------------------------------------
int vtkPDataSetReader::PolyDataExecute(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkPolyData* output = vtkPolyData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
  int updatePiece, updateNumberOfPieces;
  int startPiece, endPiece;
  int idx;

  updatePiece = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  updateNumberOfPieces = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  // Only the first N pieces have anything in them.
  if (updateNumberOfPieces > this->NumberOfPieces)
  {
    updateNumberOfPieces = this->NumberOfPieces;
  }
  if (updatePiece >= updateNumberOfPieces)
  { // This duplicates functionality of the pipeline super classes ...
    return 1;
  }

  startPiece = updatePiece * this->NumberOfPieces / updateNumberOfPieces;
  endPiece = ((updatePiece + 1) * this->NumberOfPieces / updateNumberOfPieces) - 1;

  if (endPiece < startPiece)
  {
    return 1;
  }

  vtkDataSetReader* reader;
  vtkAppendPolyData* append = vtkAppendPolyData::New();
  for (idx = startPiece; idx <= endPiece; ++idx)
  {
    vtkPolyData* tmp;
    reader = vtkDataSetReader::New();
    reader->ReadAllScalarsOn();
    reader->ReadAllVectorsOn();
    reader->ReadAllNormalsOn();
    reader->ReadAllTensorsOn();
    reader->ReadAllColorScalarsOn();
    reader->ReadAllTCoordsOn();
    reader->ReadAllFieldsOn();
    reader->SetFileName(this->PieceFileNames[idx]);
    tmp = reader->GetPolyDataOutput();
    if (tmp && tmp->GetDataObjectType() != VTK_POLY_DATA)
    {
      vtkWarningMacro("Expecting PolyData in file: " << this->PieceFileNames[idx]);
    }
    else
    {
      append->AddInputConnection(reader->GetOutputPort());
    }
    reader->Delete();
  }

  append->Update();
  output->CopyStructure(append->GetOutput());
  output->GetFieldData()->PassData(append->GetOutput()->GetFieldData());
  output->GetCellData()->PassData(append->GetOutput()->GetCellData());
  output->GetPointData()->PassData(append->GetOutput()->GetPointData());

  append->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkPDataSetReader::UnstructuredGridExecute(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));

  int updatePiece, updateNumberOfPieces;
  int startPiece, endPiece;
  int idx;

  updatePiece = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  updateNumberOfPieces = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  // Only the first N pieces have anything in them.
  if (updateNumberOfPieces > this->NumberOfPieces)
  {
    updateNumberOfPieces = this->NumberOfPieces;
  }
  if (updatePiece >= updateNumberOfPieces)
  { // This duplicates functionality of the pipeline super classes ...
    return 1;
  }
  startPiece = updatePiece * this->NumberOfPieces / updateNumberOfPieces;
  endPiece = ((updatePiece + 1) * this->NumberOfPieces / updateNumberOfPieces) - 1;

  vtkDataSetReader* reader;
  vtkAppendFilter* append = vtkAppendFilter::New();
  for (idx = startPiece; idx <= endPiece; ++idx)
  {
    reader = vtkDataSetReader::New();
    reader->ReadAllScalarsOn();
    reader->ReadAllVectorsOn();
    reader->ReadAllNormalsOn();
    reader->ReadAllTensorsOn();
    reader->ReadAllColorScalarsOn();
    reader->ReadAllTCoordsOn();
    reader->ReadAllFieldsOn();
    reader->SetFileName(this->PieceFileNames[idx]);
    reader->Update();
    if (reader->GetOutput()->GetDataObjectType() != VTK_UNSTRUCTURED_GRID)
    {
      vtkErrorMacro("Expecting unstructured grid.");
    }
    else
    {
      append->AddInputConnection(reader->GetOutputPort());
    }
    reader->Delete();
  }

  append->Update();
  output->CopyStructure(append->GetOutput());
  output->GetFieldData()->PassData(append->GetOutput()->GetFieldData());
  output->GetCellData()->PassData(append->GetOutput()->GetCellData());
  output->GetPointData()->PassData(append->GetOutput()->GetPointData());

  append->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// Structured data is trickier.  Which files to load?
int vtkPDataSetReader::ImageDataExecute(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkImageData* output = vtkImageData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));

  vtkStructuredPointsReader* reader;
  int uExt[6];
  int ext[6];
  int i, j;

  // Allocate the data object.
  int wUExt[6];
  info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), wUExt);
  vtkNew<vtkExtentTranslator> et;
  et->SetWholeExtent(wUExt);
  et->SetPiece(info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  et->SetNumberOfPieces(info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  int ghostLevels = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  et->SetGhostLevel(ghostLevels);
  et->PieceToExtent();
  et->GetExtent(uExt);
  output->SetExtent(uExt);
  output->AllocateScalars(info);

  // Get the pieces that will be read.
  std::vector<int> pieceMask(this->NumberOfPieces, 0);
  this->CoverExtent(uExt, pieceMask.data());

  // Now read and append
  reader = vtkStructuredPointsReader::New();
  reader->ReadAllScalarsOn();
  reader->ReadAllVectorsOn();
  reader->ReadAllNormalsOn();
  reader->ReadAllTensorsOn();
  reader->ReadAllColorScalarsOn();
  reader->ReadAllTCoordsOn();
  reader->ReadAllFieldsOn();
  for (i = 0; i < this->NumberOfPieces; ++i)
  {
    if (pieceMask[i])
    {
      reader->SetFileName(this->PieceFileNames[i]);
      reader->Update();
      // Sanity check: extent is correct.  Ignore electric slide.
      reader->GetOutput()->GetExtent(ext);
      if (ext[1] - ext[0] != this->PieceExtents[i][1] - this->PieceExtents[i][0] ||
        ext[3] - ext[2] != this->PieceExtents[i][3] - this->PieceExtents[i][2] ||
        ext[5] - ext[4] != this->PieceExtents[i][5] - this->PieceExtents[i][4])
      {
        vtkErrorMacro("Unexpected extent in VTK file: " << this->PieceFileNames[i]);
      }
      else
      {
        // Reverse the electric slide.
        reader->GetOutput()->SetExtent(this->PieceExtents[i]);
        // Intersect extent and output extent
        reader->GetOutput()->GetExtent(ext);
        for (j = 0; j < 3; ++j)
        {
          if (ext[j * 2] < uExt[j * 2])
          {
            ext[j * 2] = uExt[j * 2];
          }
          if (ext[j * 2 + 1] > uExt[j * 2 + 1])
          {
            ext[j * 2 + 1] = uExt[j * 2 + 1];
          }
        }
        output->CopyAndCastFrom(reader->GetOutput(), ext);
        vtkDataArray* scalars = reader->GetOutput()->GetPointData()->GetScalars();
        if (scalars && scalars->GetName())
        {
          output->GetPointData()->GetScalars()->SetName(scalars->GetName());
        }
      }
    }
  }

  reader->Delete();

  if (ghostLevels > 0)
  {
    et->SetGhostLevel(0);
    et->PieceToExtent();
    int zeroExt[6];
    et->GetExtent(zeroExt);
    output->GenerateGhostArray(zeroExt);
  }

  return 1;
}

//----------------------------------------------------------------------------
// Structured data is trickier.  Which files to load?
int vtkPDataSetReader::StructuredGridExecute(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkStructuredGrid* output =
    vtkStructuredGrid::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));

  vtkStructuredGrid* tmp;
  int count = 0;
  vtkStructuredGridReader* reader;
  vtkPoints* newPts;
  int uExt[6];
  int ext[6];
  int i;
  int pIncY, pIncZ, cIncY, cIncZ;
  int ix, iy, iz;
  double* pt;
  vtkIdType inId, outId;
  vtkIdType numPts, numCells;

  // Get the pieces that will be read.
  std::vector<int> pieceMask(this->NumberOfPieces, 0);
  int wUExt[6];
  info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), wUExt);
  vtkNew<vtkExtentTranslator> et;
  et->SetWholeExtent(wUExt);
  et->SetPiece(info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  et->SetNumberOfPieces(info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  int ghostLevels = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  et->SetGhostLevel(ghostLevels);
  et->PieceToExtent();
  et->GetExtent(uExt);
  this->CoverExtent(uExt, pieceMask.data());

  // Now read the pieces.
  std::vector<vtkSmartPointer<vtkStructuredGrid> > pieces;
  reader = vtkStructuredGridReader::New();
  reader->ReadAllScalarsOn();
  reader->ReadAllVectorsOn();
  reader->ReadAllNormalsOn();
  reader->ReadAllTensorsOn();
  reader->ReadAllColorScalarsOn();
  reader->ReadAllTCoordsOn();
  reader->ReadAllFieldsOn();
  for (i = 0; i < this->NumberOfPieces; ++i)
  {
    if (pieceMask[i])
    {
      reader->SetOutput(nullptr);
      reader->SetFileName(this->PieceFileNames[i]);
      reader->Update();
      tmp = reader->GetOutput();
      if (tmp->GetNumberOfCells() > 0)
      {
        pieces.emplace_back(tmp);
        // Sanity check: extent is correct.  Ignore electric slide.
        tmp->GetExtent(ext);
        if (ext[1] - ext[0] != this->PieceExtents[i][1] - this->PieceExtents[i][0] ||
          ext[3] - ext[2] != this->PieceExtents[i][3] - this->PieceExtents[i][2] ||
          ext[5] - ext[4] != this->PieceExtents[i][5] - this->PieceExtents[i][4])
        {
          vtkErrorMacro("Unexpected extent in VTK file: " << this->PieceFileNames[i]);
        }
        else
        {
          // Reverse the electric slide.
          tmp->SetExtent(this->PieceExtents[i]);
        }
        ++count;
      }
    }
  }

  // Anything could happen with files.
  if (count <= 0)
  {
    reader->Delete();
    return 1;
  }

  // Allocate the points.
  cIncY = uExt[1] - uExt[0];
  pIncY = cIncY + 1;
  cIncZ = cIncY * (uExt[3] - uExt[2]);
  pIncZ = pIncY * (uExt[3] - uExt[2] + 1);
  numPts = pIncZ * (uExt[5] - uExt[4] + 1);
  numCells = cIncY * (uExt[5] - uExt[4]);
  output->SetExtent(uExt);
  newPts = vtkPoints::New();
  newPts->SetNumberOfPoints(numPts);
  // Copy allocate gymnastics.
  vtkDataSetAttributes::FieldList ptList(count);
  vtkDataSetAttributes::FieldList cellList(count);
  ptList.InitializeFieldList(pieces[0]->GetPointData());
  cellList.InitializeFieldList(pieces[0]->GetCellData());
  for (i = 1; i < count; ++i)
  {
    ptList.IntersectFieldList(pieces[i]->GetPointData());
    cellList.IntersectFieldList(pieces[i]->GetCellData());
  }
  output->GetPointData()->CopyAllocate(ptList, numPts);
  output->GetCellData()->CopyAllocate(cellList, numCells);
  // Now append the pieces.
  for (i = 0; i < count; ++i)
  {
    pieces[i]->GetExtent(ext);

    // Copy point data first.
    inId = 0;
    for (iz = ext[4]; iz <= ext[5]; ++iz)
    {
      for (iy = ext[2]; iy <= ext[3]; ++iy)
      {
        for (ix = ext[0]; ix <= ext[1]; ++ix)
        {
          // For clipping.  I know it is bad to have this condition
          // in the inner most loop, but we had to read the data ...
          if (iz <= uExt[5] && iz >= uExt[4] && iy <= uExt[3] && iy >= uExt[2] && ix <= uExt[1] &&
            ix >= uExt[0])
          {
            outId = (ix - uExt[0]) + pIncY * (iy - uExt[2]) + pIncZ * (iz - uExt[4]);
            pt = pieces[i]->GetPoint(inId);
            newPts->SetPoint(outId, pt);
            output->GetPointData()->CopyData(ptList, pieces[i]->GetPointData(), i, inId, outId);
          }
          ++inId;
        }
      }
    }
    // Copy cell data now.
    inId = 0;
    for (iz = ext[4]; iz < ext[5]; ++iz)
    {
      for (iy = ext[2]; iy < ext[3]; ++iy)
      {
        for (ix = ext[0]; ix < ext[1]; ++ix)
        {
          outId = (ix - uExt[0]) + cIncY * (iy - uExt[2]) + cIncZ * (iz - uExt[4]);
          output->GetCellData()->CopyData(cellList, pieces[i]->GetCellData(), i, inId, outId);
          ++inId;
        }
      }
    }
  }
  output->SetPoints(newPts);
  newPts->Delete();

  reader->Delete();

  if (ghostLevels > 0)
  {
    et->SetGhostLevel(0);
    et->PieceToExtent();
    int zeroExt[6];
    et->GetExtent(zeroExt);
    output->GenerateGhostArray(zeroExt);
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPDataSetReader::CoverExtent(int ext[6], int* pieceMask)
{
  int bestArea;
  int area;
  int best;
  int cExt[6]; // Covered
  int rExt[6]; // Remainder piece
  int i, j;

  // Pick the piece with the largest coverage.
  // Greedy search should be good enough.
  best = -1;
  bestArea = 0;
  for (i = 0; i < this->NumberOfPieces; ++i)
  {
    // Compute coverage.
    area = 1;
    for (j = 0; j < 3; ++j)
    { // Intersection of piece and extent to cover.
      cExt[j * 2] = ext[j * 2];
      if (this->PieceExtents[i][j * 2] > ext[j * 2])
      {
        cExt[j * 2] = this->PieceExtents[i][j * 2];
      }
      cExt[j * 2 + 1] = ext[j * 2 + 1];
      if (this->PieceExtents[i][j * 2 + 1] < ext[j * 2 + 1])
      {
        cExt[j * 2 + 1] = this->PieceExtents[i][j * 2 + 1];
      }
      // Compute the area for cells.
      if (cExt[j * 2] >= cExt[j * 2 + 1])
      {
        area = 0;
      }
      else
      {
        area *= (cExt[j * 2 + 1] - cExt[j * 2]);
      }
    }
    if (area > bestArea)
    {
      bestArea = area;
      best = i;
    }
  }

  // It could happen if pieces do not have complete coverage.
  if (bestArea <= 0)
  {
    vtkErrorMacro("Incomplete coverage.");
    return;
  }

  // Mark the chosen piece in the mask.
  pieceMask[best] = 1;

  // Now recompute the coverage for the chosen piece.
  i = best;
  for (j = 0; j < 3; ++j)
  { // Intersection of piece and extent to cover.
    cExt[j * 2] = ext[j * 2];
    if (this->PieceExtents[i][j * 2] > ext[j * 2])
    {
      cExt[j * 2] = this->PieceExtents[i][j * 2];
    }
    cExt[j * 2 + 1] = ext[j * 2 + 1];
    if (this->PieceExtents[i][j * 2 + 1] < ext[j * 2 + 1])
    {
      cExt[j * 2 + 1] = this->PieceExtents[i][j * 2 + 1];
    }
  }

  // Compute and recursively cover remaining pieces.
  for (i = 0; i < 3; ++i)
  {
    if (ext[i * 2] < cExt[i * 2])
    {
      // This extends covered extent to minimum.
      for (j = 0; j < 6; ++j)
      {
        rExt[j] = cExt[j];
      }
      rExt[i * 2 + 1] = rExt[i * 2];
      rExt[i * 2] = ext[i * 2];
      this->CoverExtent(rExt, pieceMask);
      cExt[i * 2] = ext[i * 2];
    }
    if (ext[i * 2 + 1] > cExt[i * 2 + 1])
    {
      // This extends covered extent to maximum.
      for (j = 0; j < 6; ++j)
      {
        rExt[j] = cExt[j];
      }
      rExt[i * 2] = rExt[i * 2 + 1];
      rExt[i * 2 + 1] = ext[i * 2 + 1];
      this->CoverExtent(rExt, pieceMask);
      cExt[i * 2 + 1] = ext[i * 2 + 1];
    }
  }
}

//----------------------------------------------------------------------------
void vtkPDataSetReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->FileName)
  {
    os << indent << "FileName: " << this->FileName << endl;
  }
  else
  {
    os << indent << "FileName: nullptr\n";
  }
  os << indent << "DataType: " << this->DataType << endl;
}
