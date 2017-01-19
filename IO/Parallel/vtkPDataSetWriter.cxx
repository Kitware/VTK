/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDataSetWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPDataSetWriter.h"
#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkStructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkErrorCode.h"

vtkStandardNewMacro(vtkPDataSetWriter);

vtkCxxSetObjectMacro(vtkPDataSetWriter,
                     Controller,
                     vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkPDataSetWriter::vtkPDataSetWriter()
{
  this->StartPiece = 0;
  this->EndPiece = 0;
  this->NumberOfPieces = 1;
  this->GhostLevel = 0;

  this->FilePattern = NULL;
  this->SetFilePattern("%s.%d.vtk");
  this->UseRelativeFileNames = 1;

  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkPDataSetWriter::~vtkPDataSetWriter()
{
  this->SetFilePattern(NULL);
  this->SetController(0);
}


//----------------------------------------------------------------------------
void vtkPDataSetWriter::SetNumberOfPieces(int num)
{
  if (num == this->NumberOfPieces)
  {
    return;
  }

  this->Modified();
  this->NumberOfPieces = num;

  // Default behavior is for the single process to stream the pieces.
  this->StartPiece = 0;
  this->EndPiece = num-1;
}

//----------------------------------------------------------------------------
int vtkPDataSetWriter::Write()
{
  int i;
  int length;
  char *fileRoot;
  char *fileName;

  ostream *fptr;
  vtkDataSet *input = this->GetInput();
  int inputAlgPort;
  vtkAlgorithm *inputAlg = this->GetInputAlgorithm(0, 0, inputAlgPort);

  if (this->FileName == NULL)
  {
    vtkErrorMacro("No file name.");
    return 0;
  }

  if (this->StartPiece < 0)
  {
    this->StartPiece = 0;
  }
  if (this->NumberOfPieces < 0 || this->EndPiece < this->StartPiece)
  {
    vtkWarningMacro("No pieces to write.");
    return 1;
  }

  // Only one piece? The just write one vtk file.
  if (this->StartPiece == 0 && this->NumberOfPieces == 1)
  {
    return this->vtkDataSetWriter::Write();
  }

  // Lets compute the file root from the file name supplied by the user.
  length = static_cast<int>(strlen(this->FileName));
  fileRoot = new char [length+1];
  fileName = new char [length+strlen(this->FilePattern)+20];
  strncpy(fileRoot, this->FileName, length);
  fileRoot[length] = '\0';
  // Trim off the pvtk extension.
  if (strncmp(fileRoot+length-5, ".pvtk", 5) == 0)
  {
    fileRoot[length-5] = '\0';
  }
  if (strncmp(fileRoot+length-4, ".vtk", 4) == 0)
  {
    fileRoot[length-4] = '\0';
  }
  // If we are using relative file names, trim off the directory path.
  if (this->UseRelativeFileNames)
  {
    char *tmp, *slash;
    // Find the last / or \ in the file name.
    slash = NULL;
    tmp = fileRoot;
    while (*tmp != '\0')
    {
      if (*tmp == '/' || *tmp == '\\')
      {
        slash = tmp;
      }
      ++tmp;
    }
    // Copy just the filename into root.
    if (slash)
    {
      ++slash;
      tmp = fileRoot;
      while (*slash != '\0')
      {
        *tmp++ = *slash++;
      }
      *tmp = '\0';
    }
  }


  // Restore the fileRoot to the full path.
  strncpy(fileRoot, this->FileName, length);
  fileRoot[length] = '\0';
  // Trim off the pvtk extension.
  if (strncmp(fileRoot+length-5, ".pvtk", 5) == 0)
  {
    fileRoot[length-5] = '\0';
  }
  if (strncmp(fileRoot+length-4, ".vtk", 4) == 0)
  {
    fileRoot[length-4] = '\0';
  }

  this->UpdateInformation();

  // Now write the pieces assigned to this writer.
  vtkDataSetWriter *writer = vtkDataSetWriter::New();
  writer->SetFileTypeToBinary();
  vtkDataObject *copy;
  for (i = this->StartPiece; i <= this->EndPiece; ++i)
  {
    sprintf(fileName, this->FilePattern, fileRoot, i);
    writer->SetFileName(fileName);
    inputAlg->UpdatePiece(i, this->NumberOfPieces, this->GhostLevel);

    // Store the extent of this piece in Extents. This is later used
    // to write the extents in the pvtk file.
    vtkInformation* info = input->GetInformation();
    int* ext = 0;
    if (info->Has(vtkDataObject::DATA_EXTENT()))
    {
      ext = input->GetInformation()->Get(vtkDataObject::DATA_EXTENT());
    }
    if (ext)
    {
      this->Extents[i] = std::vector<int>(ext, ext+6);
    }

    copy = input->NewInstance();
    copy->ShallowCopy(input);
    // I am putting this in here because shallow copy does not copy the
    // UpdateExtentInitializedFlag, and I do not want to touch ShallowCopy
    // in ParaViews release.
    // copy->Crop(vtkStreamingDemandDrivenPipeline::GetUpdateExtent(
    //              this->GetInputInformation()));
    writer->SetInputData(vtkDataSet::SafeDownCast(copy));
    writer->Write();
    copy->Delete();
    copy = NULL;
    if (writer->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
    {
      this->DeleteFiles();
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      break;
    }
  }
  writer->Delete();
  writer = NULL;

    // Lets write the toplevel file.
  if (this->StartPiece == 0 &&
      (!this->Controller || this->Controller->GetLocalProcessId() == 0))
  {
    fptr = this->OpenFile();
    if (fptr == NULL)
    {
      delete [] fileRoot;
      delete [] fileName;
      return 0;
    }
    // Write a tag so that we know this file type.
    *fptr << "<File version=\"pvtk-1.0\"\n";
    fptr->flush();
    if (fptr->fail())
    {
      vtkErrorMacro(<< "Unable to write to file: "<< this->FileName);
      this->CloseVTKFile(fptr);
      remove(this->FileName);
      delete [] fileRoot;
      delete [] fileName;
      delete fptr;
      this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
      return 0;
    }

    switch (input->GetDataObjectType())
    {
      case VTK_POLY_DATA:
      case VTK_UNSTRUCTURED_GRID:
        if (!this->WriteUnstructuredMetaData(input, fileRoot, fileName, fptr))
        {
          this->CloseVTKFile(fptr);
          remove(this->FileName);
          delete [] fileRoot;
          delete [] fileName;
          delete fptr;
          this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
          return 0;
        }
        break;
      case VTK_IMAGE_DATA:
      case VTK_STRUCTURED_POINTS:
        if (!this->WriteImageMetaData((vtkImageData*)input, fileRoot,
                                      fileName, fptr))
        {
          this->CloseVTKFile(fptr);
          remove(this->FileName);
          delete [] fileRoot;
          delete [] fileName;
          delete fptr;
          this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
          return 0;
        }
        break;
      case VTK_RECTILINEAR_GRID:
        if (!this->WriteRectilinearGridMetaData((vtkRectilinearGrid*)input,
                                                fileRoot, fileName, fptr))
        {
          this->CloseVTKFile(fptr);
          remove(this->FileName);
          delete [] fileRoot;
          delete [] fileName;
          delete fptr;
          this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
          return 0;
        }
        break;
      case VTK_STRUCTURED_GRID:
        if (!this->WriteStructuredGridMetaData((vtkStructuredGrid*)input,
                                               fileRoot, fileName, fptr))
        {
          this->CloseVTKFile(fptr);
          remove(this->FileName);
          delete [] fileRoot;
          delete [] fileName;
          delete fptr;
          this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
          return 0;
        }
        break;
    }

    //fptr->close();
    delete fptr;
  }

  delete [] fileName;
  delete [] fileRoot;

  return 1;
}

//----------------------------------------------------------------------------
int vtkPDataSetWriter::WriteUnstructuredMetaData(vtkDataSet *input,
                                         char *root, char *str, ostream *fptr)
{
  int i;

  // We should indicate the type of data that is being saved.
  *fptr << "      dataType=\"" << input->GetClassName() << "\"" << endl;
  // This is making the assumption that all the files will be written out by
  // some processes.
  *fptr << "      numberOfPieces=\"" << this->NumberOfPieces << "\" >" << endl;
  for (i = 0; i < this->NumberOfPieces; ++i)
  {
    sprintf(str, this->FilePattern, root, i);
    *fptr << "  <Piece fileName=\"" << str << "\" />" << endl;
  }
  *fptr << "</File>" << endl;
  fptr->flush();
  if (fptr->fail())
  {
    return 0;
  }
  return 1;
}


//----------------------------------------------------------------------------
int vtkPDataSetWriter::WriteImageMetaData(vtkImageData * input,
                                          char *root, char *str, ostream *fptr)
{
  int *pi;
  double *pf;
  vtkInformation* inInfo = this->GetInputInformation();

  // We should indicate the type of data that is being saved.
  *fptr << "      dataType=\"" << input->GetClassName() << "\"" << endl;
  // Image data has a buch of meta data.
  *fptr << "      scalarType=\"" << input->GetScalarType() << "\"" << endl;
  pf = inInfo->Get(vtkDataObject::ORIGIN());
  *fptr << "      origin=\""
        << pf[0] << " " << pf[1] << " " << pf[2] << "\"" << endl;
  pf = inInfo->Get(vtkDataObject::SPACING());
  *fptr << "      spacing=\""
        << pf[0] << " " << pf[1] << " " << pf[2] << "\"" << endl;
  pi = vtkStreamingDemandDrivenPipeline::GetWholeExtent(inInfo);
  *fptr << "      wholeExtent=\""
        << pi[0] << " " << pi[1] << " " << pi[2] << " "
        << pi[3] << " " << pi[4] << " " << pi[5] << "\"" << endl;

  // This is making the assumption that all the files will be written out by
  // some processes.
  *fptr << "      numberOfPieces=\"" << this->NumberOfPieces << "\" >" << endl;

  // The code below gathers extens from all processes to write in the
  // meta-file. Note that the extent of each piece was already stored by
  // each writer. This is gathering it all to root node.
  if (this->Controller)
  {
    // Even though the logic is pretty straightforward, we need to
    // do a fair amount of work to use GatherV. Each rank simply
    // serializes its extents to 7 int blocks - piece number and 6
    // extent values. Then we gather this all to root.
    int rank = this->Controller->GetLocalProcessId();
    int nRanks = this->Controller->GetNumberOfProcesses();

    int nPiecesTotal = 0;
    vtkIdType nPieces = static_cast<vtkIdType>(this->Extents.size());

    vtkIdType* offsets = 0;
    vtkIdType* nPiecesAll = 0;
    vtkIdType* recvLengths = 0;
    if (rank == 0)
    {
      nPiecesAll = new vtkIdType[nRanks];
      recvLengths = new vtkIdType[nRanks];
      offsets = new vtkIdType[nRanks];
    }
    this->Controller->Gather(&nPieces, nPiecesAll, 1, 0);
    if (rank == 0)
    {
      for (int i=0; i<nRanks; i++)
      {
        offsets[i] = nPiecesTotal*7;
        nPiecesTotal += nPiecesAll[i];
        recvLengths[i] = nPiecesAll[i]*7;
      }
    }
    int* sendBuffer = 0;
    int sendSize = nPieces*7;
    if (nPieces > 0)
    {
      sendBuffer = new int[sendSize];
      ExtentsType::iterator iter = this->Extents.begin();
      for (int count = 0; iter != this->Extents.end(); ++iter, ++count)
      {
        sendBuffer[count*7] = iter->first;
        memcpy(&sendBuffer[count*7+1], &iter->second[0], 6*sizeof(int));
      }
    }
    int* recvBuffer = 0;
    if (rank == 0)
    {
      recvBuffer = new int[nPiecesTotal*7];
    }
    this->Controller->GatherV(sendBuffer, recvBuffer, sendSize,
      recvLengths, offsets, 0);

    if (rank == 0)
    {
      // Add all received values to Extents.
      // These are later written in WritePPieceAttributes()
      for (int i=1; i<nRanks; i++)
      {
        for (int j=0; j<nPiecesAll[i]; j++)
        {
          int* buffer = recvBuffer + offsets[i] + j*7;
          this->Extents[*buffer] =
            std::vector<int>(buffer+1, buffer+7);
        }
      }
    }

    delete[] nPiecesAll;
    delete[] recvBuffer;
    delete[] offsets;
    delete[] recvLengths;
    delete[] sendBuffer;
  }

  for (int i = 0; i < this->NumberOfPieces; ++i)
  {
    pi = &this->Extents[i][0];
    sprintf(str, this->FilePattern, root, i);
    *fptr << "  <Piece fileName=\"" << str << "\"" << endl
          << "      extent=\"" << pi[0] << " " << pi[1] << " " << pi[2] << " "
          << pi[3] << " " << pi[4] << " " << pi[5] << "\" />" << endl;
  }
  *fptr << "</File>" << endl;
  fptr->flush();
  if (fptr->fail())
  {
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPDataSetWriter::WriteRectilinearGridMetaData(vtkRectilinearGrid *input,
                                         char *root, char *str, ostream *fptr)
{
  int i;
  int *pi;

  // We should indicate the type of data that is being saved.
  *fptr << "      dataType=\"" << input->GetClassName() << "\"" << endl;

  pi = vtkStreamingDemandDrivenPipeline::GetWholeExtent(
    this->GetInputInformation());
  *fptr << "      wholeExtent=\"" << pi[0] << " " << pi[1] << " " << pi[2] << " "
        << pi[3] << " " << pi[4] << " " << pi[5] << "\"" << endl;


  // This is making the assumption that all the files will be written out by
  // some processes.
  *fptr << "      numberOfPieces=\"" << this->NumberOfPieces << "\" >" << endl;
  for (i = 0; i < this->NumberOfPieces; ++i)
  {
    pi = &this->Extents[i][0];
    sprintf(str, this->FilePattern, root, i);
    *fptr << "  <Piece fileName=\"" << str << "\"" << endl
          << "      extent=\"" << pi[0] << " " << pi[1] << " " << pi[2] << " "
          << pi[3] << " " << pi[4] << " " << pi[5] << "\" />" << endl;
  }
  *fptr << "</File>" << endl;

  fptr->flush();
  if (fptr->fail())
  {
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPDataSetWriter::WriteStructuredGridMetaData(vtkStructuredGrid *input,
                                         char *root, char *str, ostream *fptr)
{
  int i;
  int *pi;

  // We should indicate the type of data that is being saved.
  *fptr << "      dataType=\"" << input->GetClassName() << "\"" << endl;

  pi = vtkStreamingDemandDrivenPipeline::GetWholeExtent(
    this->GetInputInformation());
  *fptr << "      wholeExtent=\"" << pi[0] << " " << pi[1] << " " << pi[2] << " "
        << pi[3] << " " << pi[4] << " " << pi[5] << "\"" << endl;


  // This is making the assumption that all the files will be written out by
  // some processes.
  *fptr << "      numberOfPieces=\"" << this->NumberOfPieces << "\" >" << endl;
  for (i = 0; i < this->NumberOfPieces; ++i)
  {
    pi = &this->Extents[i][0];
    sprintf(str, this->FilePattern, root, i);
    *fptr << "  <Piece fileName=\"" << str << "\"" << endl
          << "      extent=\"" << pi[0] << " " << pi[1] << " " << pi[2] << " "
          << pi[3] << " " << pi[4] << " " << pi[5] << "\" />" << endl;
  }
  *fptr << "</File>" << endl;

  fptr->flush();
  if (fptr->fail())
  {
    return 0;
  }
  return 1;
}


//----------------------------------------------------------------------------
// Open a vtk data file. Returns NULL if error.
ostream *vtkPDataSetWriter::OpenFile()
{
  ostream *fptr;

  fptr = new ofstream(this->FileName, ios::out);

  if (fptr->fail())
  {
    vtkErrorMacro(<< "Unable to open file: "<< this->FileName);
    delete fptr;
    return NULL;
  }

  return fptr;
}

void vtkPDataSetWriter::DeleteFiles()
{
  int i;
  int length = static_cast<int>(strlen(this->FileName));
  char *fileRoot = new char[length+1];
  char *fileName = new char[length+strlen(this->FilePattern)+20];

  strncpy(fileRoot, this->FileName, length);
  fileRoot[length] = '\0';
  // Trim off the pvtk extension.
  if (strncmp(fileRoot+length-5, ".pvtk", 5) == 0)
  {
    fileRoot[length-5] = '\0';
  }
  if (strncmp(fileRoot+length-4, ".vtk", 4) == 0)
  {
    fileRoot[length-4] = '\0';
  }
  // If we are using relative file names, trim off the directory path.
  if (this->UseRelativeFileNames)
  {
    char *tmp, *slash;
    // Find the last / or \ in the file name.
    slash = NULL;
    tmp = fileRoot;
    while (*tmp != '\0')
    {
      if (*tmp == '/' || *tmp == '\\')
      {
        slash = tmp;
      }
      ++tmp;
    }
    // Copy just the filename into root.
    if (slash)
    {
      ++slash;
      tmp = fileRoot;
      while (*slash != '\0')
      {
        *tmp++ = *slash++;
      }
      *tmp = '\0';
    }
  }

  for (i = this->StartPiece; i <= this->EndPiece; i++)
  {
    sprintf(fileName, this->FilePattern, fileRoot, i);
    remove(fileName);
  }

  remove(this->FileName);

  delete [] fileName;
  delete [] fileRoot;
}

//----------------------------------------------------------------------------
void vtkPDataSetWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);


  os << indent << "StartPiece: " << this->StartPiece << endl;
  os << indent << "EndPiece: " << this->EndPiece << endl;
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << endl;
  os << indent << "GhostLevel: " << this->GhostLevel << endl;
  os << indent << "FilePattern: " << this->FilePattern << endl;
  os << indent << "UseRelativeFileNames: " << this->UseRelativeFileNames << endl;
}
