/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataWriter.cxx
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
#include "vtkPolyDataWriter.h"

#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#endif

vtkCxxRevisionMacro(vtkPolyDataWriter, "1.21");
vtkStandardNewMacro(vtkPolyDataWriter);

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkPolyDataWriter::SetInput(vtkPolyData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkPolyData *vtkPolyDataWriter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkPolyData *)(this->Inputs[0]);
}

void vtkPolyDataWriter::WriteData()
{
  ostream *fp;
  vtkPolyData *input = this->GetInput();

  vtkDebugMacro(<<"Writing vtk polygonal data...");

  if ( !(fp=this->OpenVTKFile()) || !this->WriteHeader(fp) )
    {
    if (fp)
      {
      vtkErrorMacro("Ran out of disk space; deleting file: "
                    << this->FileName);
      this->CloseVTKFile(fp);
      unlink(this->FileName);
      }
    return;
    }
  //
  // Write polygonal data specific stuff
  //
  *fp << "DATASET POLYDATA\n"; 
  
  //
  // Write data owned by the dataset
  if (!this->WriteDataSetData(fp, input))
    {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
    }

  if (!this->WritePoints(fp, input->GetPoints()))
    {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
    }

  if (input->GetVerts())
    {
    if (!this->WriteCells(fp, input->GetVerts(),"VERTICES"))
      {
      vtkErrorMacro("Ran out of disk space; deleting file: "
                    << this->FileName);
      this->CloseVTKFile(fp);
      unlink(this->FileName);
      return;
      }
    }
  if (input->GetLines())
    {
    if (!this->WriteCells(fp, input->GetLines(),"LINES"))
      {
      vtkErrorMacro("Ran out of disk space; deleting file: "
                    << this->FileName);
      this->CloseVTKFile(fp);
      unlink(this->FileName);
      return;
      }
    }
  if (input->GetPolys())
    {
    if (!this->WriteCells(fp, input->GetPolys(),"POLYGONS"))
      {
      vtkErrorMacro("Ran out of disk space; deleting file: "
                    << this->FileName);
      this->CloseVTKFile(fp);
      unlink(this->FileName);
      return;
      }
    }
  if (input->GetStrips())
    {
    if (!this->WriteCells(fp, input->GetStrips(),"TRIANGLE_STRIPS"))
      {
      vtkErrorMacro("Ran out of disk space; deleting file: "
                    << this->FileName);
      this->CloseVTKFile(fp);
      unlink(this->FileName);
      return;
      }
    }

  if (!this->WriteCellData(fp, input))
    {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
    }
  if (!this->WritePointData(fp, input))
    {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
    }

  this->CloseVTKFile(fp);
}

void vtkPolyDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
