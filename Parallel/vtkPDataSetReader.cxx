/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDataSetReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkPDataSetReader.h"
#include "vtkDataSetReader.h"
#include "vtkStructuredPointsReader.h"
#include "vtkAppendPolyData.h"
#include "vtkAppendFilter.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkImageData.h"
#include "vtkStructuredPoints.h"
#include "vtkObjectFactory.h"
#include "vtkSource.h"



//------------------------------------------------------------------------------
vtkPDataSetReader* vtkPDataSetReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPDataSetReader");
  if(ret)
    {
    return (vtkPDataSetReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPDataSetReader;
}


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
}

//----------------------------------------------------------------------------
vtkPDataSetReader::~vtkPDataSetReader()
{
  delete[] this->FileName;
  this->SetNumberOfPieces(0);

  if (this->PieceFileNames)
    {
    for (int i = 0; i < this->NumberOfPieces; ++i)
      {
      if (this->PieceFileNames[i])
        {
        delete [] this->PieceFileNames[i];
        this->PieceFileNames[i] = NULL;
        }
      }
    }
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
  if (this->StructuredFlag)
    {
    this->PieceExtents = new int*[num];
    for (i = 0; i < num; ++i)
      {
      this->PieceExtents[i] = new int[6];
      }
    }


  this->NumberOfPieces = num;
}


//----------------------------------------------------------------------------
void vtkPDataSetReader::SetOutput(vtkDataSet *output)
{
  this->vtkSource::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
vtkDataSet *vtkPDataSetReader::GetOutput()
{
  vtkDataSet *output;

  if (this->FileName == NULL)
    {
    vtkErrorMacro("You need to set the filename before you can get the output");
    return NULL;
    }
  this->ExecuteInformation();

  // Creates an output if necessary.
  this->CheckOutput();
  output = (vtkDataSet *)(this->Outputs[0]);

  return output;
}

//----------------------------------------------------------------------------
void vtkPDataSetReader::Update()
{
  vtkDataSet *output = this->GetOutput();

  if (output)
    {
    output->Update();
    }
}


//----------------------------------------------------------------------------
// This assumes that the DataType is set.
vtkDataSet *vtkPDataSetReader::CheckOutput()
{
  vtkDataSet *output = NULL;
  vtkDataSet *newOutput;

  if (this->Outputs)
    {
    output = (vtkDataSet *)(this->Outputs[0]);
    }

  if (output && output->GetDataObjectType() == this->DataType)
    {
    return output;
    }

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
      return NULL;
    }

  if (output)
    {
    vtkWarningMacro("Creating a new output of type " 
                  << newOutput->GetClassName());
    }
  
  this->SetNthOutput(0, newOutput);
  newOutput->Delete();

  return newOutput;
}


//----------------------------------------------------------------------------
void vtkPDataSetReader::ExecuteInformation()
{
  ifstream *file;
  char str[1024];
  vtkDataSet *output;
  int i;

  file = this->OpenFile();
  if (file == NULL)
    {
    return;
    }
  file->getline(str, 1024);
  str[1023] = '\0';

  if (strncmp(str, "<File version=\"pvtk-1.0\"", 24) != 0)
    {
    // Find out the type from the file.
    if (strncmp(str, "# vtk DataFile Version", 22) != 0)
      {
      vtkErrorMacro("This does not look like a VTK file: " 
                    << this->FileName << ", " << str);
      file->close();
      delete file;
      return;
      }
    // This is a vtk file not a PVTK file.
    this->ReadVTKFileInformation(str, file);
    this->VTKFileFlag = 1;
    file->close();
    delete file;
    return;
    }

  // Read PVTK meta data.
  this->VTKFileFlag = 0;
  file->getline(str, 1024);
  if (strncmp(str, "      dataType=", 15) != 0)
    {
    vtkErrorMacro("Expecting DataType.");
    return;
    }
  // Take the last quote off.
  str[strlen(str)-1] = '\0';
  if (strcmp(str+16, "vtkPolyData") == 0)
    {
    this->DataType = VTK_POLY_DATA;
    this->StructuredFlag = 0;
    }
  else if (strcmp(str+16, "vtkUnstructuredGrid") == 0)
    {
    this->DataType = VTK_UNSTRUCTURED_GRID;
    this->StructuredFlag = 0;
    }
  else if (strcmp(str+16, "vtkStructuredGrid") == 0)
    {
    this->DataType = VTK_STRUCTURED_GRID;
    this->StructuredFlag = 1;
    }
  else if (strcmp(str+16, "vtkRectilinearGrid") == 0)
    {
    this->DataType = VTK_RECTILINEAR_GRID;
    this->StructuredFlag = 1;
    }
  else if (strcmp(str+16, "vtkImageData") == 0)
    {
    this->DataType = VTK_IMAGE_DATA;
    this->StructuredFlag = 1;
    }
  else 
    {
    vtkErrorMacro("Unknown data type " << str+16);
    return;
    }
  // Get output after the DataType is set. 
  output = this->CheckOutput();
  if (output == NULL)
    {
    return;
    }  

  // Read Image specific information
  if (this->DataType == VTK_IMAGE_DATA)
    {
    this->ReadImageInformation(vtkImageData::SafeDownCast(output), str, file);
    }
  // Read whole extent for structured data.
  if (this->StructuredFlag)
    {
    this->ReadWholeExtent(output, str, file);
    }

  // Read the number of pieces.
  file->getline(str, 1024);
  if (strncmp(str, "      numberOfPieces=", 21) != 0)
    {
    vtkErrorMacro("Expecting numberOfPieces.");
    return;
    }
  // Take the last > off.
  str[strlen(str)-3] = '\0';
  this->SetNumberOfPieces(atoi(str+22));
  if (output->IsA("vtkPolyData") || output->IsA("vtkUnstructuredGrid"))
    {
    output->SetMaximumNumberOfPieces(this->NumberOfPieces);
    }

  // Read the filename and extents for each piece.
  for (i = 0; i < this->NumberOfPieces; ++i)
    {
    if (this->StructuredFlag)
      {
      int *pi = this->PieceExtents[i];
      file->getline(str, 512);
      // Take all characters after the quote off.
      str[strlen(str)-1] = '\0';
      strcpy(this->PieceFileNames[i], str+19);
      // Now read the extent.
      file->getline(str, 512);
      // Take all characters after the quote off.
      str[strlen(str)-4] = '\0';
      sscanf(str+14, "%d %d %d %d %d %d", pi, pi+1, pi+2, pi+3, pi+4, pi+5);
      }
    else
      {
      file->getline(str, 512);
      // Take all characters after the quote off.
      str[strlen(str)-4] = '\0';
      strcpy(this->PieceFileNames[i], str+19);
      }
    }
}



//----------------------------------------------------------------------------
// Does not read whole extent.
void vtkPDataSetReader::ReadImageInformation(vtkImageData *output, 
                                             char *str, ifstream *fp)
{
  float origin[3];
  float spacing[3];

  if (output == NULL)
    {
    vtkErrorMacro("Expecting and image.");
    return;
    }

  //    scalarType="10"
  //    origin="-1.75 -1.25 0"
  //    spacing="0.1 0.1 0.1"

  // Read the scalar type.
  fp->getline(str, 1024);
  if (strncmp(str, "      scalarType=", 17) != 0)
    {
    vtkErrorMacro("Expecting scalarType.");
    return;
    }
  // Take the quotes off.
  str[strlen(str)-1] = '\0';
  output->SetScalarType(atoi(str+18));

  // Read the origin.
  fp->getline(str, 1024);
  if (strncmp(str, "      origin=", 13) != 0)
    {
    vtkErrorMacro("Expecting origin.");
    return;
    }
  // Take the quotes off.
  str[strlen(str)-1] = '\0';
  sscanf(str+14, "%f %f %f", origin, origin+1, origin+2);
  output->SetOrigin(origin);

  // Read the spacing.
  fp->getline(str, 1024);
  if (strncmp(str, "      spacing=", 14) != 0)
    {
    vtkErrorMacro("Expecting spacing.");
    return;
    }
  // Take the quotes off.
  str[strlen(str)-1] = '\0';
  sscanf(str+15, "%f %f %f", spacing, spacing+1, spacing+2);
  output->SetSpacing(spacing);
}

//----------------------------------------------------------------------------
void vtkPDataSetReader::ReadWholeExtent(vtkDataSet *output, 
                                        char *str, ifstream *fp)
{
  int ext[6];

  if (output == NULL)
    {
    vtkErrorMacro("Expecting a data set.");
    return;
    }

  //      wholeExtent="0 25 0 25 0 10"

  // Read the wholeExtent.
  fp->getline(str, 1024);
  if (strncmp(str, "      wholeExtent=", 18) != 0)
    {
    vtkErrorMacro("Expecting wholeExtent.");
    return;
    }
  // Take the quotes off.
  str[strlen(str)-1] = '\0';
  sscanf(str+19, "%d %d %d %d %d %d", ext, ext+1, ext+2, ext+3, ext+4, ext+5);
  output->SetWholeExtent(ext);
}

//----------------------------------------------------------------------------
void vtkPDataSetReader::ReadVTKFileInformation(char *str, ifstream *file)
{
  int i;
  int dx, dy, dz;
  float x, y, z;
  vtkDataSet *output;

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
    vtkStructuredGrid *grid = (vtkStructuredGrid*)(this->CheckOutput());
    file->getline(str, 1024, ' ');
    if (strncmp(str, "DIMENSIONS", 10) != 0)
      {
      vtkErrorMacro("Expecting 'DIMENSIONS' insted of: " << str);
      return;
      }
    *file >> dx;
    *file >> dy;
    *file >> dz;
    grid->SetWholeExtent(0, dx-1, 0, dy-1, 0, dz-1);
    }
  else if (strncmp(str, "DATASET RECTILINEAR_GRID", 24) == 0)
    {
    this->DataType = VTK_RECTILINEAR_GRID;
    vtkRectilinearGrid *grid = (vtkRectilinearGrid*)(this->CheckOutput());
    file->getline(str, 1024, ' ');
    if (strncmp(str, "DIMENSIONS", 10) != 0)
      {
      vtkErrorMacro("Expecting 'DIMENSIONS' insted of: " << str);
      return;
      }
    *file >> dx;
    *file >> dy;
    *file >> dz;
    grid->SetWholeExtent(0, dx-1, 0, dy-1, 0, dz-1);
    }
  else if (strncmp(str, "DATASET STRUCTURED_POINTS", 25) == 0)
    {
    this->DataType = VTK_IMAGE_DATA;
    vtkImageData *image = (vtkImageData*)(this->CheckOutput());
    file->getline(str, 1024, ' ');
    // hack to stop reading.
    while (strlen(str) > 5)
      {
      if (strncmp(str, "DIMENSIONS", 10) == 0)
        {
        *file >> dx;
        *file >> dy;
        *file >> dz;
        image->SetWholeExtent(0, dx-1, 0, dy-1, 0, dz-1);
        }
      if (strncmp(str, "SPACING", 7) == 0 || strncmp(str, "ASPECT_RATIO", 12) == 0)
        {
        *file >> x;
        *file >> y;
        *file >> z;
        image->SetSpacing(x, y, z);
        }
      if (strncmp(str, "ORIGIN", 6) == 0)
        {
        *file >> x;
        *file >> y;
        *file >> z;
        image->SetOrigin(x, y, z);
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
  output = this->CheckOutput();
  if (output->IsA("vtkPolyData") || output->IsA("vtkUnstructuredGrid"))
    {
    output->SetMaximumNumberOfPieces(1);
    }
}

//----------------------------------------------------------------------------
ifstream *vtkPDataSetReader::OpenFile()
{
  ifstream *file;

  if (!this->FileName)
    {
    vtkErrorMacro(<<"A FileName must be specified.");
    return NULL;
    }
  
  // Open the new file
  file = new ifstream(this->FileName, ios::in);

  if (! file || file->fail())
    {
    vtkErrorMacro(<< "Initialize: Could not open file " << this->FileName);
    return NULL;
    }

  return file;
}

//----------------------------------------------------------------------------
void vtkPDataSetReader::Execute()
{
  if (this->VTKFileFlag)
    {
    vtkDataSet *output;
    vtkDataSetReader *reader = vtkDataSetReader::New();
    reader->SetFileName(this->FileName);
    reader->Update();
    vtkDataSet *data = reader->GetOutput();
    // Structured points giving me a pain.
    //this->DataType = data->GetDataObjectType();
     
    output = this->CheckOutput();
    if (output == NULL)
      {
      return;
      }
    output->CopyStructure(data);
    output->GetCellData()->PassData(data->GetCellData());
    output->GetPointData()->PassData(data->GetPointData());
    this->SetNumberOfPieces(0);

    reader->Delete();
    return;
    }
    
  switch (this->DataType)
    {
    case VTK_POLY_DATA:
      this->PolyDataExecute();
      break;
    case VTK_UNSTRUCTURED_GRID:
      this->UnstructuredGridExecute();
      break;
    case VTK_IMAGE_DATA:
      this->ImageDataExecute();
      break;
    default:
      vtkErrorMacro("We do not handle vtkStructuredGrid yet.");
      return;
    }
}



//----------------------------------------------------------------------------
void vtkPDataSetReader::PolyDataExecute()
{
  // Use out internal method to get the output because GetOutput calls
  // UpdateInformation.
  vtkPolyData *output = vtkPolyData::SafeDownCast(this->CheckOutput());
  int updatePiece, updateNumberOfPieces;
  int startPiece, endPiece;
  int idx;

  if (output == NULL)
    {
    vtkErrorMacro("Could not create output.");
    return;
    }

  updatePiece = output->GetUpdatePiece();
  updateNumberOfPieces = output->GetUpdateNumberOfPieces();

  // Only the first N pieces have anything in them.
  if (updateNumberOfPieces > this->NumberOfPieces)
    {
    updateNumberOfPieces = this->NumberOfPieces;
    }
  if (updatePiece >= updateNumberOfPieces)
    { // This duplicates functionality of the pipeline super classes ...
    return;
    }

  startPiece = updatePiece * this->NumberOfPieces / updateNumberOfPieces;
  endPiece = ((updatePiece+1) * this->NumberOfPieces / updateNumberOfPieces) - 1;

  if (endPiece < startPiece)
    {
    return;
    }

  vtkDataSetReader *reader;
  vtkAppendPolyData *append = vtkAppendPolyData::New();
  for (idx = startPiece; idx <= endPiece; ++idx)
    {
    vtkPolyData *tmp;
    reader = vtkDataSetReader::New();
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
  output->GetCellData()->PassData(append->GetOutput()->GetCellData());
  output->GetPointData()->PassData(append->GetOutput()->GetPointData());

  append->Delete();
}


//----------------------------------------------------------------------------
void vtkPDataSetReader::UnstructuredGridExecute()
{
  // Use out internal method to get the output because GetOutput calls
  // UpdateInformation.
  vtkUnstructuredGrid *output;
  int updatePiece, updateNumberOfPieces;
  int startPiece, endPiece;
  int idx;

  output = vtkUnstructuredGrid::SafeDownCast(this->CheckOutput());
  if (output == NULL)
    {
    vtkErrorMacro("Could not create output.");
    return;
    }

  updatePiece = output->GetUpdatePiece();
  updateNumberOfPieces = output->GetUpdateNumberOfPieces();

  // Only the first N pieces have anything in them.
  if (updateNumberOfPieces > this->NumberOfPieces)
    {
    updateNumberOfPieces = this->NumberOfPieces;
    }
  if (updatePiece >= updateNumberOfPieces)
    { // This duplicates functionality of the pipeline super classes ...
    return;
    }
  startPiece = updatePiece * updateNumberOfPieces / this->NumberOfPieces;
  endPiece = ((updatePiece+1) * updateNumberOfPieces / this->NumberOfPieces) - 1;

  vtkDataSetReader *reader;
  vtkAppendFilter *append = vtkAppendFilter::New();
  for (idx = startPiece; idx <= endPiece; ++idx)
    {
    reader = vtkDataSetReader::New();
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
  output->GetCellData()->PassData(append->GetOutput()->GetCellData());
  output->GetPointData()->PassData(append->GetOutput()->GetPointData());

  append->Delete();
}


//----------------------------------------------------------------------------
// Structured data is trickier.  Which files to load?
void vtkPDataSetReader::ImageDataExecute()
{
  vtkStructuredPointsReader *reader;
  vtkImageData *output;
  int uExt[6];
  int ext[6];
  int *pieceMask;
  int i, j;

  // Use out internal method to get the output because GetOutput calls
  // UpdateInformation.
  output = vtkImageData::SafeDownCast(this->CheckOutput());
  if (output == NULL)
    {
    vtkErrorMacro("Could not create output.");
    return;
    }

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
        }
      }
    }

  delete [] pieceMask;
  reader->Delete();
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
      area *= cExt[j*2+1] - cExt[j*2];
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
  vtkSource::PrintSelf(os,indent);

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







