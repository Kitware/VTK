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
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSource.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridReader.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsReader.h"
#include "vtkUnstructuredGrid.h"
#include "vtkExtentTranslator.h"

vtkStandardNewMacro(vtkPDataSetReader);

//----------------------------------------------------------------------------
vtkPDataSetReader::vtkPDataSetReader()
{
  this->FileName = NULL;
  this->VTKFileFlag = 0;
  this->StructuredFlag = 0;
  this->NumberOfPieces = 0;
  this->DataType = -1;
  this->PieceFileNames = NULL;
  this->PieceExtents = NULL;
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
    if (this->PieceFileNames[i])
      {
      delete [] this->PieceFileNames[i];
      this->PieceFileNames[i] = NULL;
      }
    if (this->PieceExtents && this->PieceExtents[i])
      {
      delete [] this->PieceExtents[i];
      this->PieceExtents[i] = NULL;
      }
    }
  if (this->PieceFileNames)
    {
    delete [] this->PieceFileNames;
    this->PieceFileNames = NULL;
    }
  if (this->PieceExtents)
    {
    delete [] this->PieceExtents;
    this->PieceExtents = NULL;
    }
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
  vtkInformation* request, 
  vtkInformationVector** inputVector , 
  vtkInformationVector* outputVector)
{

  ifstream *file;
  char* block;
  char* param;
  char* value;
  int type;
  
  // Start reading the meta-data pvtk file.
  file = this->OpenFile(this->FileName);
  if (file == NULL)
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
    this->ReadVTKFileInformation(file, request, inputVector, outputVector);
    this->VTKFileFlag = 1;
    }
  else
    {
    vtkErrorMacro("This does not look like a VTK file: " << this->FileName);
    }
  file->close();
  delete file;

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));
    
  if (output && output->GetDataObjectType() == this->DataType)
    {
    return 1;
    }

  vtkDataSet* newOutput=0;
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
    vtkWarningMacro("Creating a new output of type " 
                  << newOutput->GetClassName());
    }
  
  newOutput->SetPipelineInformation(info);
  newOutput->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// Returns 0 for end of file.
// Returns 1 for start block,
// Returns 2 for parameter-value pair (occurs after 1 but before 3).
// Returns 3 for termination of start block. 
// Returns 4 for string inside block.  Puts string in retVal. (param = NULL)
// Returns 5 for end block.
// =======
// The statics should be instance variables ...
int vtkPDataSetReader::ReadXML(ifstream *file, 
                               char **retBlock, char **retParam, char **retVal)
{
  static char str[1024];
  static char* ptr = NULL;
  static char block[256];
  static char param[256];
  static char value[512];
  // I could keep track of the blocks on a stack, but I don't need to.
  static int inStartBlock = 0;
  char *tmp;

  // Initialize the strings.
  if (ptr == NULL)
    {
    block[0] = param[0] = value[0] = '\0';
    }
    
  // Skip white space
  // We could do this with a get char ...
  while (ptr == NULL || *ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\0')
    {
    if (ptr == NULL || *ptr == '\0')
      { // At the end of a line.  Read another.
      file->getline(str, 1024);
      if (file->fail())
        {
        *retBlock = NULL;
        *retParam = NULL;
        *retVal = NULL;
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
    *retParam = NULL;
    *retVal = NULL;
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
    *retParam = NULL;
    *retVal = NULL;
    return 1;
    }

  // Handle the termination of a start block.
  if (inStartBlock && *ptr == '>')
    {
    ++ptr;
    inStartBlock = 0;
    *retBlock = block;
    *retParam = NULL;
    *retVal = NULL;
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
      *retParam = NULL;
      *retVal = NULL;
      return 5;
      }
    // First pass: inStartBlock == 1.  Return Terminate start block.
    // Uses block name saved from start block.
    // Do not skip over the '/>' characters.
    inStartBlock = 2;
    *retBlock = block;
    *retParam = NULL;
    *retVal = NULL;
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
    *retBlock = NULL;
    *retParam = NULL;
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
  // Expect an equals sign imediately after parameter string (no spaces).
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
  ifstream *file;
  char* block;
  char* param;
  char* value;
  int type;
  int flag = 0;
  
  // Start reading the meta-data pvtk file.
  file = this->OpenFile(filename);
  if (file == NULL)
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
    while (this->ReadXML(file, &block, &param, &value) != 5) {}
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


  file->close();
  delete file;
  return flag;
}



//----------------------------------------------------------------------------
void vtkPDataSetReader::ReadPVTKFileInformation(
  ifstream *file,
  vtkInformation*, 
  vtkInformationVector**,
  vtkInformationVector* outputVector)
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
  if (type != 2 || strcmp(param,"version"))
    {
    vtkErrorMacro("Could not find file version.");
    return;
    }
  if (strcmp(val,"pvtk-1.0") != 0)
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
      dirLength = count+1;
      }
    }
  // This trims off every thing after the last slash.
  dir[dirLength] = '\0';

  // We are in the start file block.
  // Read parameters until we terminate the start block.
  while ( (type = this->ReadXML(file, &block, &param, &val)) != 3)
    {
    if (type == 0)
      {
      vtkErrorMacro("Early termination of pvtk file.");
      return;
      }
    if (type != 2)
      { // There should be no other possibility. Param will not be NULL.
      vtkErrorMacro("Expecting a parameter.");
      return;
      }

    // Handle parameter: numberOfPieces.
    if (strcmp(param, "numberOfPieces") == 0)
      {
      this->SetNumberOfPieces(atoi(val));
      if (! this->StructuredFlag)
        {
        info->Set(
          vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), 
          this->NumberOfPieces);
        }
      }

    // Handle parameter: wholeExtent.
    if (strcmp(param, "wholeExtent") == 0)
      {
      if (! this->StructuredFlag)
        {
        vtkWarningMacro("Extent mismatch.");
        }
      sscanf(val, "%d %d %d %d %d %d", ext, ext+1, ext+2, ext+3, ext+4, ext+5);
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
      sscanf(val, "%lf %lf %lf", vect, vect+1, vect+2);
      info->Set(vtkDataObject::SPACING(), vect, 3);
      }

    // Handle parameter: origin.
    if (strcmp(param, "origin") == 0)
      {
      sscanf(val, "%lf %lf %lf", vect, vect+1, vect+2);
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
    int *pi = this->PieceExtents[i];
    // Initialize extent to nothing.
    pi[0] = pi[2] = pi[4] = 0;
    pi[1] = pi[3] = pi[5] = -1;

    // Read the start tag of the Piece block.
    type = this->ReadXML(file, &block, &param, &val);
    if ( type != 1 || strcmp(block,"Piece") != 0)
      {
      vtkErrorMacro("Expecting the start of a 'Piece' block");
      return;
      }
    while ( (type = this->ReadXML(file, &block, &param, &val)) != 3)
      {
      if (type != 2)
        { // There should be no other possibility. Param will not be NULL.
        vtkErrorMacro("Expecting a parameter.");
        return;
        }

      // Handle the file name parameter.
      if (strcmp(param,"fileName") == 0)
        {
        // Copy filename (relative path?)
        if (val[0] != '/' && val[1] != ':' && dirLength > 0)
          { // Must be a relative path.
          sprintf(this->PieceFileNames[i], "%s%s", dir, val);
          }
        else
          {
          strcpy(this->PieceFileNames[i], val); 
          }
        }

      // Handle the extent parameter.
      if (strcmp(param,"extent") == 0)
        {
        if ( ! this->StructuredFlag)
          {
          vtkWarningMacro("Found extent parameter for unstructured data.");
          }
          sscanf(val, "%d %d %d %d %d %d", pi, pi+1, pi+2, pi+3, pi+4, pi+5);
        }
      }
    // Start termination was consumed by while loop.

    // Now read the ending piece block.
    type = this->ReadXML(file, &block, &param, &val);
    if ( type != 5 || strcmp(block,"Piece") != 0)
      {
      vtkErrorMacro("Expecting termination of the Piece block.");
      return;
      }
    }
}


//----------------------------------------------------------------------------
void vtkPDataSetReader::ReadVTKFileInformation(
  ifstream *file,
  vtkInformation*, 
  vtkInformationVector**,
  vtkInformationVector* outputVector)
{
  int i;
  int dx, dy, dz;
  float x, y, z;
  char str[1024];
  
  vtkInformation* info = outputVector->GetInformationObject(0);

  // To avoid UMR in the first string comparison
  strcpy(str, "        "); 

  // Try to find the line that specifies the dataset type.
  i = 0;
  while (strncmp(str, "DATASET", 7) != 0 && i < 6)
    {
    file->getline(str, 1024);
    ++i;
    }

  if (strncmp(str, "DATASET POLYDATA", 16) == 0)
    {
    this->DataType = VTK_POLY_DATA;
    }
  else if (strncmp(str, "DATASET UNSTRUCTURED_GRID", 25) == 0)
    {
    this->DataType = VTK_UNSTRUCTURED_GRID;
    }
  else if (strncmp(str, "DATASET STRUCTURED_GRID", 23) == 0)
    {
    this->DataType = VTK_STRUCTURED_GRID;
    file->getline(str, 1024, ' ');

    if (! strncmp(str, "FIELD", 5))
      {
      this->SkipFieldData(file);
      file->getline(str, 1024, ' ');
      vtkErrorMacro(<< str);
      }
    if (strncmp(str, "DIMENSIONS", 10) != 0)
      {
      vtkErrorMacro("Expecting 'DIMENSIONS' insted of: " << str);
      return;
      }
        
    *file >> dx;
    *file >> dy;
    *file >> dz;
    info->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 
              0, dx-1, 0, dy-1, 0, dz-1);
    }
  else if (strncmp(str, "DATASET RECTILINEAR_GRID", 24) == 0)
    {
    this->DataType = VTK_RECTILINEAR_GRID;
    file->getline(str, 1024, ' ');
    if (strncmp(str, "DIMENSIONS", 10) != 0)
      {
      vtkErrorMacro("Expecting 'DIMENSIONS' insted of: " << str);
      return;
      }
    *file >> dx;
    *file >> dy;
    *file >> dz;
    info->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 
              0, dx-1, 0, dy-1, 0, dz-1);
    }
  else if (strncmp(str, "DATASET STRUCTURED_POINTS", 25) == 0)
    {
    this->DataType = VTK_IMAGE_DATA;
    file->getline(str, 1024, ' ');
    // hack to stop reading.
    while (strlen(str) > 5)
      {
      if (strncmp(str, "DIMENSIONS", 10) == 0)
        {
        *file >> dx;
        *file >> dy;
        *file >> dz;
        info->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 
                  0, dx-1, 0, dy-1, 0, dz-1);
        }
      if (strncmp(str, "SPACING", 7) == 0 || strncmp(str, "ASPECT_RATIO", 12) == 0)
        {
        *file >> x;
        *file >> y;
        *file >> z;
        info->Set(vtkDataObject::SPACING(), x, y, z);
        }
      if (strncmp(str, "ORIGIN", 6) == 0)
        {
        *file >> x;
        *file >> y;
        *file >> z;
        info->Set(vtkDataObject::ORIGIN(), x, y, z);
        }
      file->getline(str, 1024);
      file->getline(str, 1024, ' ');
      }
    }
  else 
    {
    vtkErrorMacro("I can not figure out what type of data set this is: " << str);
    return;
    }
  if (this->DataType == VTK_POLY_DATA || this->DataType == VTK_UNSTRUCTURED_GRID)
    {
    info->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), 1);
    }
}

void vtkPDataSetReader::SkipFieldData(ifstream *file)
{
  int i, numArrays;
  char name[256], type[256];
  int numComp, numTuples;

  file->width(256);
  *file >> name;
  *file >> numArrays;

  if (file->fail())
    {
    vtkErrorMacro("Could not read field.");
    return;
    }
  
  // Read the number of arrays specified
  for (i=0; i<numArrays; i++)
    {
    long length=0;
    char buffer[256];
    *file >> buffer; 
    *file >> numComp;
    *file >> numTuples;
    *file >> type;
    // What a pain.
    if (strcmp(type, "double") == 0)
      {
      length = sizeof(double) * numComp * numTuples;
      }
    if (strcmp(type, "float") == 0)
      {
      length = sizeof(float) * numComp * numTuples;
      }
    if (strcmp(type, "long") == 0)
      {
      length = sizeof(long) * numComp * numTuples;
      }
    if (strcmp(type, "unsigned long") == 0)
      {
      length = sizeof(unsigned long) * numComp * numTuples;
      }
    if (strcmp(type, "int") == 0)
      {
      length = sizeof(int) * numComp * numTuples;
      }
    if (strcmp(type, "unsigned int") == 0)
      {
      length = sizeof(unsigned int) * numComp * numTuples;
      }
    if (strcmp(type, "short") == 0)
      {
      length = sizeof(short) * numComp * numTuples;
      }
    if (strcmp(type, "unsigned short") == 0)
      {
      length = sizeof(unsigned short) * numComp * numTuples;
      }
    if (strcmp(type, "char") == 0)
      {
      length = sizeof(char) * numComp * numTuples;
      }
    if (strcmp(type, "unsigned char") == 0)
      {
      length = sizeof(unsigned char) * numComp * numTuples;
      }

    // suckup new line.
    file->getline(name,256);

    char *buf = new char[length];

    //int t = file->tellg();
    // this seek did not work for some reason.
    // it passed too many characters.
    //file->seekg(length, ios::cur);
    file->read(buf, length);

    delete [] buf;

    // suckup new line.
    file->getline(name,256);
    if (file->fail())
      {
      vtkErrorMacro("Could not seek past field.");
      return;
      }
    }
}

//----------------------------------------------------------------------------
ifstream *vtkPDataSetReader::OpenFile(const char* filename)
{
  ifstream *file;

  if (!filename || filename[0] == '\0')
    {
    vtkDebugMacro(<<"A FileName must be specified.");
    return NULL;
    }
  
  // Open the new file
  file = new ifstream(filename, ios::in);

  if (! file || file->fail())
    {
    if (file)
      {
      delete file;
      }
    vtkErrorMacro(<< "Initialize: Could not open file " << filename);
    return NULL;
    }

  return file;
}

//----------------------------------------------------------------------------
int vtkPDataSetReader::RequestData(vtkInformation* request, 
                                   vtkInformationVector** inputVector , 
                                   vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));

  if (this->VTKFileFlag)
    {
    vtkDataSetReader *reader = vtkDataSetReader::New();
    reader->ReadAllScalarsOn();
    reader->ReadAllVectorsOn();
    reader->ReadAllNormalsOn();
    reader->ReadAllTensorsOn();
    reader->ReadAllColorScalarsOn();
    reader->ReadAllTCoordsOn();
    reader->ReadAllFieldsOn();
    reader->SetFileName(this->FileName);
    reader->Update();
    vtkDataSet *data = reader->GetOutput();

    if (data == NULL)
      {
      vtkErrorMacro("Could not read file: " << this->FileName);
      return 0;
      }
//    data->Update();

    if (data->CheckAttributes())
      {
      vtkErrorMacro("Attribute Mismatch.");
      return 0;
      }

    // Do not copy the ExtentTranslator (hack) 
    // reader should probably set the extent translator
    // not paraview.
    vtkExtentTranslator *tmp = output->GetExtentTranslator();
    tmp->Register(this);
    output->CopyStructure(data);
    output->SetExtentTranslator(tmp);
    tmp->UnRegister(tmp);
    output->GetFieldData()->PassData(data->GetFieldData());
    output->GetCellData()->PassData(data->GetCellData());
    output->GetPointData()->PassData(data->GetPointData());
    this->SetNumberOfPieces(0);

    reader->Delete();
    return 1;
    }
    
  switch (this->DataType)
    {
    case VTK_POLY_DATA:
      return this->PolyDataExecute(request, inputVector, outputVector);
      break;
    case VTK_UNSTRUCTURED_GRID:
      return this->UnstructuredGridExecute(request, inputVector, outputVector);
      break;
    case VTK_IMAGE_DATA:
      return this->ImageDataExecute(request, inputVector, outputVector);
      break;
    case VTK_STRUCTURED_GRID:
      return this->StructuredGridExecute(request, inputVector, outputVector);
      break;
    default:
      vtkErrorMacro("We do not handle vtkRectilinear yet.");
    }

  return 0;
}



//----------------------------------------------------------------------------
int vtkPDataSetReader::PolyDataExecute(vtkInformation*, 
                                       vtkInformationVector**, 
                                       vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));
  int updatePiece, updateNumberOfPieces;
  int startPiece, endPiece;
  int idx;

  updatePiece = output->GetUpdatePiece();
  updateNumberOfPieces = output->GetUpdateNumberOfPieces();

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
  endPiece = ((updatePiece+1) * this->NumberOfPieces / updateNumberOfPieces) - 1;

  if (endPiece < startPiece)
    {
    return 1;
    }

  vtkDataSetReader *reader;
  vtkAppendPolyData *append = vtkAppendPolyData::New();
  for (idx = startPiece; idx <= endPiece; ++idx)
    {
    vtkPolyData *tmp;
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
      append->AddInput(tmp);
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
  vtkInformation*, 
  vtkInformationVector** , 
  vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));

  int updatePiece, updateNumberOfPieces;
  int startPiece, endPiece;
  int idx;

  updatePiece = output->GetUpdatePiece();
  updateNumberOfPieces = output->GetUpdateNumberOfPieces();

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
  endPiece = ((updatePiece+1) * this->NumberOfPieces / updateNumberOfPieces) - 1;

  vtkDataSetReader *reader;
  vtkAppendFilter *append = vtkAppendFilter::New();
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
      append->AddInput(reader->GetUnstructuredGridOutput());
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
  vtkInformation*, 
  vtkInformationVector** , 
  vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkImageData *output = vtkImageData::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));

  vtkStructuredPointsReader *reader;
  int uExt[6];
  int ext[6];
  int *pieceMask;
  int i, j;

  // Allocate the data object.
  output->GetUpdateExtent(uExt);
  output->SetExtent(uExt);
  output->AllocateScalars();

  // Get the pieces that will be read.
  pieceMask = new int[this->NumberOfPieces];
  for (i = 0; i < this->NumberOfPieces; ++i)
    {
    pieceMask[i] = 0;
    }
  this->CoverExtent(uExt, pieceMask);

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
          if (ext[j*2] < uExt[j*2])
            {
            ext[j*2] = uExt[j*2];
            }
          if (ext[j*2+1] > uExt[j*2+1])
            {
            ext[j*2+1] = uExt[j*2+1];
            }
          }
        output->CopyAndCastFrom(reader->GetOutput(), ext);
        vtkDataArray *scalars = reader->GetOutput()->GetPointData()->GetScalars();
        if (scalars && scalars->GetName())
          {
          output->GetPointData()->GetScalars()->SetName(scalars->GetName());
          }
        }
      }
    }

  delete [] pieceMask;
  reader->Delete();

  return 1;
}


//----------------------------------------------------------------------------
// Structured data is trickier.  Which files to load?
int vtkPDataSetReader::StructuredGridExecute(
  vtkInformation*, 
  vtkInformationVector** , 
  vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkStructuredGrid *output = vtkStructuredGrid::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));

  vtkStructuredGrid *tmp;
  vtkStructuredGrid **pieces;
  int count = 0;
  vtkStructuredGridReader *reader;
  vtkPoints *newPts;
  int uExt[6];
  int ext[6];
  int *pieceMask;
  int i;
  int pIncY, pIncZ, cIncY, cIncZ;
  int ix, iy, iz;
  double *pt;
  vtkIdType inId, outId;
  vtkIdType numPts, numCells;

  // Get the pieces that will be read.
  pieceMask = new int[this->NumberOfPieces];
  for (i = 0; i < this->NumberOfPieces; ++i)
    {
    pieceMask[i] = 0;
    }
  output->GetUpdateExtent(uExt);
  this->CoverExtent(uExt, pieceMask);

  // Now read the pieces.
  pieces = new vtkStructuredGrid*[this->NumberOfPieces];
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
      reader->SetOutput(0);
      reader->SetFileName(this->PieceFileNames[i]);
      reader->Update();
      tmp = reader->GetOutput();
      if (tmp->GetNumberOfCells() > 0)
        {
        pieces[count] = tmp;
        tmp->Register(this);
        // Sanity check: extent is correct.  Ignore electric slide.
        tmp->GetExtent(ext);
        if (ext[1] - ext[0] != 
            this->PieceExtents[i][1] - this->PieceExtents[i][0] ||
            ext[3] - ext[2] != 
            this->PieceExtents[i][3] - this->PieceExtents[i][2] ||
            ext[5] - ext[4] != 
            this->PieceExtents[i][5] - this->PieceExtents[i][4])
          {
          vtkErrorMacro("Unexpected extent in VTK file: " << 
                        this->PieceFileNames[i]);
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
    delete [] pieces;
    delete [] pieceMask;
    reader->Delete();
    return 1;
    }

  // Allocate the points.
  cIncY = uExt[1]-uExt[0];
  pIncY = cIncY+1;
  cIncZ = cIncY*(uExt[3]-uExt[2]);
  pIncZ = pIncY*(uExt[3]-uExt[2]+1);
  numPts = pIncZ * (uExt[5]-uExt[4]+1);
  numCells = cIncY * (uExt[5]-uExt[4]);
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
  output->GetPointData()->CopyAllocate(ptList,numPts);
  output->GetCellData()->CopyAllocate(cellList,numCells);
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
          if (iz <= uExt[5] && iz >= uExt[4] && 
              iy <= uExt[3] && iy >= uExt[2] && 
              ix <= uExt[1] && ix >= uExt[0])
            {
            outId = (ix-uExt[0]) + pIncY*(iy-uExt[2]) + pIncZ*(iz-uExt[4]);
            pt = pieces[i]->GetPoint(inId);
            newPts->SetPoint(outId, pt);
            output->GetPointData()->CopyData(ptList, 
                                             pieces[i]->GetPointData(), i, 
                                             inId, outId); 
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
          outId = (ix-uExt[0]) + cIncY*(iy-uExt[2]) + cIncZ*(iz-uExt[4]);
          output->GetCellData()->CopyData(cellList, pieces[i]->GetCellData(), i,
                                          inId, outId); 
          ++inId;
          }
        }
      }
    }
  output->SetPoints(newPts);
  newPts->Delete();

  for (i = 0; i < count; ++i)
    {
    pieces[i]->Delete();
    pieces[i] = NULL;
    }
  delete [] pieces;
  delete [] pieceMask;

  reader->Delete();
  return 1;
}


//----------------------------------------------------------------------------
void vtkPDataSetReader::CoverExtent(int ext[6], int *pieceMask)
{
  int bestArea;
  int area;
  int best;
  int cExt[6];  // Covered
  int rExt[6];  // Remainder piece
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
      cExt[j*2] = ext[j*2];
      if (this->PieceExtents[i][j*2] > ext[j*2])
        {
        cExt[j*2] = this->PieceExtents[i][j*2];
        }
      cExt[j*2+1] = ext[j*2+1];
      if (this->PieceExtents[i][j*2+1] < ext[j*2+1])
        {
        cExt[j*2+1] = this->PieceExtents[i][j*2+1];
        }
      // Compute the area for cells.  
      if (cExt[j*2] >= cExt[j*2+1])
        {
        area = 0;
        }
      else
        {
        area *= (cExt[j*2+1] - cExt[j*2]);
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

  // Mark the choosen piece in the mask.
  pieceMask[best] = 1;

  // Now recompute the coverage for the choosen piece.
  i = best;
  for (j = 0; j < 3; ++j)
    { // Intersection of piece and extent to cover.
    cExt[j*2] = ext[j*2];
    if (this->PieceExtents[i][j*2] > ext[j*2])
      {
      cExt[j*2] = this->PieceExtents[i][j*2];
      }
    cExt[j*2+1] = ext[j*2+1];
    if (this->PieceExtents[i][j*2+1] < ext[j*2+1])
      {
      cExt[j*2+1] = this->PieceExtents[i][j*2+1];
      }
    }
  
  // Compute and recursively cover remaining pieces.
  for (i = 0; i < 3; ++i)
    {
    if (ext[i*2] < cExt[i*2])
      {
      // This extends covered extent to minimum.
      for (j = 0; j < 6; ++j)
        { 
        rExt[j] = cExt[j];
        }
      rExt[i*2+1] = rExt[i*2];
      rExt[i*2] = ext[i*2];
      this->CoverExtent(rExt, pieceMask);
      cExt[i*2] = ext[i*2];
      }
    if (ext[i*2+1] > cExt[i*2+1])
      {
      // This extends covered extent to maximum.
      for (j = 0; j < 6; ++j)
        { 
        rExt[j] = cExt[j];
        }
      rExt[i*2] = rExt[i*2+1];
      rExt[i*2+1] = ext[i*2+1];
      this->CoverExtent(rExt, pieceMask);
      cExt[i*2+1] = ext[i*2+1];
      }
    }
}

//----------------------------------------------------------------------------
void vtkPDataSetReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->FileName)
    {
    os << indent << "FileName: " << this->FileName << endl;
    }
  else
    {
    os << indent << "FileName: NULL\n";
    }
  os << indent << "DataType: " << this->DataType << endl;
}







