/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDataSetWriter.cxx
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

#include "vtkPDataSetWriter.h"
#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSource.h"



//------------------------------------------------------------------------------
vtkPDataSetWriter* vtkPDataSetWriter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPDataSetWriter");
  if(ret)
    {
    return (vtkPDataSetWriter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPDataSetWriter;
}


//----------------------------------------------------------------------------
vtkPDataSetWriter::vtkPDataSetWriter()
{
  this->StartPiece = 0;
  this->EndPiece = 0;
  this->NumberOfPieces = 1;
  this->GhostLevel = 0;

  this->FilePattern = NULL;
  this->SetFilePattern("%s.%d.vtk");
}

//----------------------------------------------------------------------------
vtkPDataSetWriter::~vtkPDataSetWriter()
{
  this->SetFilePattern(NULL);
}


//----------------------------------------------------------------------------
void vtkPDataSetWriter::Write()
{
  int i;
  int length;
  char *fileRoot;
  char *fileName;
  char *savedFileName;

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
  length = strlen(this->FileName);
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

  // Now write the pieces assigned to this writer.
  savedFileName = this->FileName;
  for (i = this->StartPiece; i <= this->EndPiece; ++i)
    {
    sprintf(fileName, this->FilePattern, fileRoot, i);
    input->SetUpdateExtent(i, this->NumberOfPieces, this->GhostLevel);
    this->FileName = fileName;
    this->vtkDataSetWriter::Write();
    }
  this->FileName = savedFileName;
  savedFileName = NULL;
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
  vtkDataSetWriter::PrintSelf(os,indent);


  os << indent << "StartPiece: " << this->StartPiece << endl;
  os << indent << "EndPiece: " << this->EndPiece << endl;
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << endl;
  os << indent << "GhostLevel: " << this->GhostLevel << endl;
  os << indent << "FilePattern: " << this->FilePattern << endl;
}







