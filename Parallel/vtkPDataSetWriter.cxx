/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDataSetWriter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPDataSetWriter.h"
#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSource.h"

vtkCxxRevisionMacro(vtkPDataSetWriter, "1.6");
vtkStandardNewMacro(vtkPDataSetWriter);

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
}

//----------------------------------------------------------------------------
vtkPDataSetWriter::~vtkPDataSetWriter()
{
  this->SetFilePattern(NULL);
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
void vtkPDataSetWriter::Write()
{
  int i;
  int length;
  char *fileRoot;
  char *fileName;

  ostream *fptr;
  vtkDataSet *input = this->GetInput();

  if (this->FileName == NULL)
    {
    vtkErrorMacro("No file name.");
    return;
    }

  if (this->StartPiece < 0)
    {
    this->StartPiece = 0;
    }
  if (this->NumberOfPieces < 0 || this->EndPiece < this->StartPiece)
    {
    vtkWarningMacro("No pieces to write.");
    return;
    }

  // Only one piece? The just write one vtk file.
  if (this->StartPiece == 0 && this->NumberOfPieces == 1)
    {
    this->vtkDataSetWriter::Write();
    return;
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

  // Lets write the toplevel file.
  if (this->StartPiece == 0)
    {
    fptr = this->OpenFile();
    if (fptr == NULL)
      {
      delete [] fileRoot;
      delete [] fileName;
      return;
      }
    // Write a tag so that we know this file type.
    *fptr << "<File version=\"pvtk-1.0\"\n";
    if (fptr->fail())
      {
      vtkErrorMacro(<< "Unable to write to file: "<< this->FileName);
      delete [] fileRoot;
      delete [] fileName;
      //fptr->close();
      delete fptr;
      return;
      }

    input->UpdateInformation();
    switch (input->GetDataObjectType())
      {
      case VTK_POLY_DATA:
      case VTK_UNSTRUCTURED_GRID:
        this->WriteUnstructuredMetaData(input, fileRoot, fileName, fptr);
        break;
      case VTK_IMAGE_DATA:
        this->WriteImageMetaData((vtkImageData*)input, fileRoot, fileName, fptr);
        break;
      case VTK_RECTILINEAR_GRID:
        this->WriteRectilinearGridMetaData((vtkRectilinearGrid*)input, 
                                           fileRoot, fileName, fptr);
        break;
      case VTK_STRUCTURED_GRID:
        this->WriteStructuredGridMetaData((vtkStructuredGrid*)input, 
                                          fileRoot, fileName, fptr);
        break;
      }
  
    //fptr->close();
    delete fptr;
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

  // Now write the pieces assigned to this writer.
  vtkDataSetWriter *writer = vtkDataSetWriter::New();
  writer->SetFileTypeToBinary();
  vtkDataObject *copy;
  for (i = this->StartPiece; i <= this->EndPiece; ++i)
    {
    sprintf(fileName, this->FilePattern, fileRoot, i);
    writer->SetFileName(fileName);
    input->SetUpdateExtent(i, this->NumberOfPieces, this->GhostLevel);
    input->Update();
    copy = input->MakeObject();
    copy->ShallowCopy(input);
    // I am putting this in here because shallow copy does not copy the
    // UpdateExtentInitializedFlag, and I do not want to touch ShallowCopy
    // in ParaViews release.
    copy->SetUpdateExtent(input->GetUpdateExtent());
    copy->SetRequestExactExtent(1);
    writer->SetInput(vtkDataSet::SafeDownCast(copy));
    writer->Write();
    copy->Delete();
    copy = NULL;
    }
  writer->Delete();
  writer = NULL;
  delete [] fileName;
  delete [] fileRoot;
}


//----------------------------------------------------------------------------
void vtkPDataSetWriter::WriteUnstructuredMetaData(vtkDataSet *input,
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
}


//----------------------------------------------------------------------------
void vtkPDataSetWriter::WriteImageMetaData(vtkImageData *input, 
                                         char *root, char *str, ostream *fptr)
{
  int i;
  int *pi;
  float *pf;

  // We should indicate the type of data that is being saved.
  *fptr << "      dataType=\"" << input->GetClassName() << "\"" << endl;
  // Image data has a buch of meta data.
  *fptr << "      scalarType=\"" << input->GetScalarType() << "\"" << endl;
  pf = input->GetOrigin();
  *fptr << "      origin=\"" << pf[0] << " " << pf[1] << " " << pf[2] << "\"" << endl;
  pf = input->GetSpacing();
  *fptr << "      spacing=\"" << pf[0] << " " << pf[1] << " " << pf[2] << "\"" << endl;
  pi = input->GetWholeExtent();
  *fptr << "      wholeExtent=\"" << pi[0] << " " << pi[1] << " " << pi[2] << " "
        << pi[3] << " " << pi[4] << " " << pi[5] << "\"" << endl;

  // This is making the assumption that all the files will be written out by
  // some processes.
  *fptr << "      numberOfPieces=\"" << this->NumberOfPieces << "\" >" << endl;

  for (i = 0; i < this->NumberOfPieces; ++i)
    {
    input->SetUpdateExtent(i, this->NumberOfPieces, this->GhostLevel);
    pi = input->GetUpdateExtent();
    sprintf(str, this->FilePattern, root, i);
    *fptr << "  <Piece fileName=\"" << str << "\"" << endl
          << "      extent=\"" << pi[0] << " " << pi[1] << " " << pi[2] << " "
          << pi[3] << " " << pi[4] << " " << pi[5] << "\" />" << endl;
    }
  *fptr << "</File>" << endl;
}

//----------------------------------------------------------------------------
void vtkPDataSetWriter::WriteRectilinearGridMetaData(vtkRectilinearGrid *input,
                                         char *root, char *str, ostream *fptr)
{
  int i;
  int *pi;

  // We should indicate the type of data that is being saved.
  *fptr << "      dataType=\"" << input->GetClassName() << "\"" << endl;

  pi = input->GetWholeExtent();
  *fptr << "      wholeExtent=\"" << pi[0] << " " << pi[1] << " " << pi[2] << " "
        << pi[3] << " " << pi[4] << " " << pi[5] << "\"" << endl;


  // This is making the assumption that all the files will be written out by
  // some processes.
  *fptr << "      numberOfPieces=\"" << this->NumberOfPieces << "\" >" << endl;
  for (i = 0; i < this->NumberOfPieces; ++i)
    {
    input->SetUpdateExtent(i, this->NumberOfPieces, this->GhostLevel);
    pi = input->GetUpdateExtent();
    sprintf(str, this->FilePattern, root, i);
    *fptr << "  <Piece fileName=\"" << str << "\"" << endl
          << "      extent=\"" << pi[0] << " " << pi[1] << " " << pi[2] << " "
          << pi[3] << " " << pi[4] << " " << pi[5] << "\" />" << endl;
    }
  *fptr << "</File>" << endl;
}

//----------------------------------------------------------------------------
void vtkPDataSetWriter::WriteStructuredGridMetaData(vtkStructuredGrid *input,
                                         char *root, char *str, ostream *fptr)
{
  int i;
  int *pi;

  // We should indicate the type of data that is being saved.
  *fptr << "      dataType=\"" << input->GetClassName() << "\"" << endl;

  pi = input->GetWholeExtent();
  *fptr << "      wholeExtent=\"" << pi[0] << " " << pi[1] << " " << pi[2] << " "
        << pi[3] << " " << pi[4] << " " << pi[5] << "\"" << endl;


  // This is making the assumption that all the files will be written out by
  // some processes.
  *fptr << "      numberOfPieces=\"" << this->NumberOfPieces << "\" >" << endl;
  for (i = 0; i < this->NumberOfPieces; ++i)
    {
    input->SetUpdateExtent(i, this->NumberOfPieces, this->GhostLevel);
    pi = input->GetUpdateExtent();
    sprintf(str, this->FilePattern, root, i);
    *fptr << "  <Piece fileName=\"" << str << "\"" << endl
          << "      extent=\"" << pi[0] << " " << pi[1] << " " << pi[2] << " "
          << pi[3] << " " << pi[4] << " " << pi[5] << "\" />" << endl;
    }
  *fptr << "</File>" << endl;
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







