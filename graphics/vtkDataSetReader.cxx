/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkDataSetReader.h"
#include "vtkPolyDataReader.h"
#include "vtkStructuredPointsReader.h"
#include "vtkStructuredGridReader.h"
#include "vtkUnstructuredGridReader.h"


vtkDataSetReader::vtkDataSetReader()
{
}

// Description:
// Specify file name of vtk data file to read.
void vtkDataSetReader::SetFileName(char *name) 
{
  this->Reader.SetFileName(name);
}
char *vtkDataSetReader::GetFileName() 
{
  return this->Reader.GetFileName();
}

// Description:
// Get the type of file (VTK_ASCII or VTK_BINARY).
int vtkDataSetReader::GetFileType() 
{
  return this->Reader.GetFileType();
}

// Description:
// Set the name of the scalar data to extract. If not specified, first 
// scalar data encountered is extracted.
void vtkDataSetReader::SetScalarsName(char *name) 
{
  this->Reader.SetScalarsName(name);
}
char *vtkDataSetReader::GetScalarsName() 
{
  return this->Reader.GetScalarsName();
}

// Description:
// Set the name of the vector data to extract. If not specified, first 
// vector data encountered is extracted.
void vtkDataSetReader::SetVectorsName(char *name) 
{
  this->Reader.SetVectorsName(name);
}
char *vtkDataSetReader::GetVectorsName() 
{
  return this->Reader.GetVectorsName();
}

// Description:
// Set the name of the tensor data to extract. If not specified, first 
// tensor data encountered is extracted.
void vtkDataSetReader::SetTensorsName(char *name) 
{
  this->Reader.SetTensorsName(name);
}
char *vtkDataSetReader::GetTensorsName() 
{
  return this->Reader.GetTensorsName();
}

// Description:
// Set the name of the normal data to extract. If not specified, first 
// normal data encountered is extracted.
void vtkDataSetReader::SetNormalsName(char *name) 
{
  this->Reader.SetNormalsName(name);
}
char *vtkDataSetReader::GetNormalsName() 
{
  return this->Reader.GetNormalsName();
}

// Description:
// Set the name of the texture coordinate data to extract. If not specified,
// first texture coordinate data encountered is extracted.
void vtkDataSetReader::SetTCoordsName(char *name) 
{
  this->Reader.SetTCoordsName(name);
}
char *vtkDataSetReader::GetTCoordsName() 
{
  return this->Reader.GetTCoordsName();
}

// Description:
// Set the name of the lookup table data to extract. If not specified, uses 
// lookup table named by scalar. Otherwise, this specification supersedes.
void vtkDataSetReader::SetLookupTableName(char *name) 
{
  this->Reader.SetLookupTableName(name);
}
char *vtkDataSetReader::GetLookupTableName() 
{
  return this->Reader.GetLookupTableName();
}


void vtkDataSetReader::Execute()
{
  char line[256];
  vtkDataSet *output = NULL;
  
  vtkDebugMacro(<<"Reading vtk dataset...");
  if ( this->Debug ) this->Reader.DebugOn();
  else this->Reader.DebugOff();

  if (!this->Reader.OpenVTKFile() || !this->Reader.ReadHeader())
      return;
//
// Determine dataset type
//
  if (!this->Reader.ReadString(line))
    {
    vtkErrorMacro(<< "Premature EOF reading dataset keyword");
    return;
    }

  if ( !strncmp(this->Reader.LowerCase(line),"dataset",(unsigned long)7) )
    {
//
// See if type is recognized.
//
    if (!this->Reader.ReadString(line))
      {
      vtkErrorMacro(<< "Premature EOF reading type");
      return;
      }

    this->Reader.CloseVTKFile();
    if ( ! strncmp(this->Reader.LowerCase(line),"polydata",8) )
      {
      vtkPolyDataReader *preader = vtkPolyDataReader::New();
      preader->SetFileName(this->Reader.GetFileName());
      preader->SetInputString(this->Reader.GetInputString());
      preader->SetReadFromInputString(this->Reader.GetReadFromInputString());
      preader->SetScalarsName(this->Reader.GetScalarsName());
      preader->SetVectorsName(this->Reader.GetVectorsName());
      preader->SetNormalsName(this->Reader.GetNormalsName());
      preader->SetTensorsName(this->Reader.GetTensorsName());
      preader->SetTCoordsName(this->Reader.GetTCoordsName());
      preader->SetLookupTableName(this->Reader.GetLookupTableName());
      preader->Update();
      output = preader->GetOutput();
      }

    else if ( ! strncmp(line,"structured_points",17) )
      {
      vtkStructuredPointsReader *preader = vtkStructuredPointsReader::New();
      preader->SetFileName(this->Reader.GetFileName());
      preader->SetInputString(this->Reader.GetInputString());
      preader->SetReadFromInputString(this->Reader.GetReadFromInputString());
      preader->SetScalarsName(this->Reader.GetScalarsName());
      preader->SetVectorsName(this->Reader.GetVectorsName());
      preader->SetNormalsName(this->Reader.GetNormalsName());
      preader->SetTensorsName(this->Reader.GetTensorsName());
      preader->SetTCoordsName(this->Reader.GetTCoordsName());
      preader->SetLookupTableName(this->Reader.GetLookupTableName());
      preader->Update();
      output = preader->GetOutput();
      }

    else if ( ! strncmp(line,"structured_grid",15) )
      {
      vtkStructuredGridReader *preader = vtkStructuredGridReader::New();
      preader->SetFileName(this->Reader.GetFileName());
      preader->SetInputString(this->Reader.GetInputString());
      preader->SetReadFromInputString(this->Reader.GetReadFromInputString());
      preader->SetScalarsName(this->Reader.GetScalarsName());
      preader->SetVectorsName(this->Reader.GetVectorsName());
      preader->SetNormalsName(this->Reader.GetNormalsName());
      preader->SetTensorsName(this->Reader.GetTensorsName());
      preader->SetTCoordsName(this->Reader.GetTCoordsName());
      preader->SetLookupTableName(this->Reader.GetLookupTableName());
      preader->Update();
      output = preader->GetOutput();
      }

    else if ( ! strncmp(line,"unstructured_grid",17) )
      {
      vtkUnstructuredGridReader *preader = vtkUnstructuredGridReader::New();
      preader->SetFileName(this->Reader.GetFileName());
      preader->SetInputString(this->Reader.GetInputString());
      preader->SetReadFromInputString(this->Reader.GetReadFromInputString());
      preader->SetScalarsName(this->Reader.GetScalarsName());
      preader->SetVectorsName(this->Reader.GetVectorsName());
      preader->SetNormalsName(this->Reader.GetNormalsName());
      preader->SetTensorsName(this->Reader.GetTensorsName());
      preader->SetTCoordsName(this->Reader.GetTCoordsName());
      preader->SetLookupTableName(this->Reader.GetLookupTableName());
      preader->Update();
      output = preader->GetOutput();
      }

    else
      {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      return;
      }
    }
  //
  // Create appropriate dataset
  //
  if ( this->Output ) this->Output->Delete();
  this->Output = output;

  return;
}

void vtkDataSetReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkSource::PrintSelf(os,indent);
  this->Reader.PrintSelf(os,indent);
}
