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

vtkCxxRevisionMacro(vtkPolyDataWriter, "1.18");
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
    return;
    }
  //
  // Write polygonal data specific stuff
  //
  *fp << "DATASET POLYDATA\n"; 
  
  //
  // Write data owned by the dataset
  this->WriteDataSetData(fp, input);

  this->WritePoints(fp, input->GetPoints());

  if (input->GetVerts())
    {
    this->WriteCells(fp, input->GetVerts(),"VERTICES");
    }
  if (input->GetLines())
    {
    this->WriteCells(fp, input->GetLines(),"LINES");
    }
  if (input->GetPolys())
    {
    this->WriteCells(fp, input->GetPolys(),"POLYGONS");
    }
  if (input->GetStrips())
    {
    this->WriteCells(fp, input->GetStrips(),"TRIANGLE_STRIPS");
    }

  this->WriteCellData(fp, input);
  this->WritePointData(fp, input);

  this->CloseVTKFile(fp);
}

void vtkPolyDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
