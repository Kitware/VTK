/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUGrdW.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vtkUGrdW.hh"

// Description:
// Specify the input data or filter.
void vtkUnstructuredGridWriter::SetInput(vtkUnstructuredGrid *input)
{
  if ( this->Input != input )
    {
    vtkDebugMacro(<<" setting Input to " << (void *)input);
    this->Input = (vtkDataSet *) input;
    this->Modified();
    }
}

void vtkUnstructuredGridWriter::WriteData()
{
  FILE *fp;
  vtkUnstructuredGrid *input=(vtkUnstructuredGrid *)this->Input;
  int *types, ncells, cellId;

  vtkDebugMacro(<<"Writing vtk unstructured grid data...");

  if ( !(fp=this->OpenVTKFile()) || !this->WriteHeader(fp) )
      return;
//
// Write unstructured grid specific stuff
//
  fprintf(fp,"DATASET UNSTRUCTURED_GRID\n");
  this->WritePoints(fp, input->GetPoints());
  this->WriteCells(fp, input->GetCells(),"CELLS");
//
// Cell types are a little more work
//
  ncells = input->GetCells()->GetNumberOfCells();
  types = new int[ncells];
  for (cellId=0; cellId < ncells; cellId++)
    {
    types[cellId] = input->GetCellType(cellId);
    }

  fprintf (fp, "CELL_TYPES %d\n", ncells);
  if ( this->FileType == ASCII )
    {
    for (cellId=0; cellId<ncells; cellId++)
      {
      fprintf (fp, "%d\n", types[cellId]);
      }
    }
  else
    {
    fwrite (types,sizeof(int),ncells,fp);
    }
  fprintf (fp,"\n");
    
  delete [] types;
  this->WritePointData(fp, input);

  this->CloseVTKFile(fp);  
}

void vtkUnstructuredGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataWriter::PrintSelf(os,indent);
}
