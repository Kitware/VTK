/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridWriter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkUnstructuredGridWriter.h"
#include "vtkByteSwap.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkUnstructuredGridWriter* vtkUnstructuredGridWriter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkUnstructuredGridWriter");
  if(ret)
    {
    return (vtkUnstructuredGridWriter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkUnstructuredGridWriter;
}




//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkUnstructuredGridWriter::SetInput(vtkUnstructuredGrid *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkUnstructuredGrid *vtkUnstructuredGridWriter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkUnstructuredGrid *)(this->Inputs[0]);
}

void vtkUnstructuredGridWriter::WriteData()
{
  ostream *fp;
  vtkUnstructuredGrid *input=this->GetInput();
  int *types, ncells, cellId;

  vtkDebugMacro(<<"Writing vtk unstructured grid data...");

  if ( !(fp=this->OpenVTKFile()) || !this->WriteHeader(fp) )
    {
    return;
    }
  //
  // Write unstructured grid specific stuff
  //
  *fp << "DATASET UNSTRUCTURED_GRID\n"; 
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

  *fp << "CELL_TYPES " << ncells << "\n";
  if ( this->FileType == VTK_ASCII )
    {
    for (cellId=0; cellId<ncells; cellId++)
      {
      *fp << types[cellId] << "\n";
      }
    }
  else
    {
    // swap the bytes if necc
    //vtkByteSwap::SwapWrite4BERange(types,ncells,fp);
    }
  *fp << "\n";
  delete [] types;

  this->WriteCellData(fp, input);
  this->WritePointData(fp, input);

  this->CloseVTKFile(fp);  
}

void vtkUnstructuredGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataWriter::PrintSelf(os,indent);
}
