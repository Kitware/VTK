/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataSetTessellator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericDataSetTessellator.h"

#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPointData.h"
#include "vtkTetra.h"
#include "vtkCellArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkIdTypeArray.h"
#include "vtkDoubleArray.h"
#include "vtkMergePoints.h"
#include "vtkGenericDataSet.h"
#include "vtkGenericCellIterator.h"
#include "vtkGenericAdaptorCell.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkGenericAttribute.h"
#include "vtkCellData.h"
#include "vtkGenericCellTessellator.h"

vtkCxxRevisionMacro(vtkGenericDataSetTessellator, "1.8");
vtkStandardNewMacro(vtkGenericDataSetTessellator);

//----------------------------------------------------------------------------
//
vtkGenericDataSetTessellator::vtkGenericDataSetTessellator()
{
  this->internalPD=vtkPointData::New();
  this->KeepCellIds = 1;
}

//----------------------------------------------------------------------------
vtkGenericDataSetTessellator::~vtkGenericDataSetTessellator()
{
  this->internalPD->Delete();
}

//----------------------------------------------------------------------------
//
void vtkGenericDataSetTessellator::Execute()
{
  vtkDebugMacro(<< "Executing vtkGenericDataSetTessellator...");

  vtkGenericDataSet *input = this->GetInput();
  vtkUnstructuredGrid *output = this->GetOutput();
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  vtkGenericAdaptorCell *cell;
  vtkIdType numInserted=0, numNew, i;
  vtkIdType npts, *pts;
  int abortExecute=0;

  // Copy original points and point data
  vtkPoints *newPts = vtkPoints::New();
  newPts->Allocate(2*numPts,numPts);

  // loop over region
  vtkUnsignedCharArray *types = vtkUnsignedCharArray::New();
  types->Allocate(numCells);
  vtkIdTypeArray *locs = vtkIdTypeArray::New();
  locs->Allocate(numCells);
  vtkCellArray *conn = vtkCellArray::New();
  conn->Allocate(numCells);

  
  // prepare the output attributes
  vtkGenericAttributeCollection *attributes=input->GetAttributes();
  vtkGenericAttribute *attribute;
  vtkDataArray *attributeArray;
  
  int c=attributes->GetNumberOfAttributes();
  vtkDataSetAttributes *dsAttributes;

  int attributeType;
  
  i=0;
  while(i<c)
    {
    attribute=attributes->GetAttribute(i);
    attributeType=attribute->GetType();
    if(attribute->GetCentering()==vtkPointCentered)
      {
      dsAttributes=outputPD;
      
      attributeArray=vtkDataArray::CreateDataArray(attribute->GetComponentType());
      attributeArray->SetNumberOfComponents(attribute->GetNumberOfComponents());
      attributeArray->SetName(attribute->GetName());
      this->internalPD->AddArray(attributeArray);
      attributeArray->Delete();
      if(this->internalPD->GetAttribute(attributeType)==0)
        {
        this->internalPD->SetActiveAttribute(this->internalPD->GetNumberOfArrays()-1,attributeType);
        }
      }
    else // vtkCellCentered
      {
      dsAttributes=outputCD;
      }
    attributeArray=vtkDataArray::CreateDataArray(attribute->GetComponentType());
    attributeArray->SetNumberOfComponents(attribute->GetNumberOfComponents());
    attributeArray->SetName(attribute->GetName());
    dsAttributes->AddArray(attributeArray);
    attributeArray->Delete();
    
    if(dsAttributes->GetAttribute(attributeType)==0)
      {
      dsAttributes->SetActiveAttribute(dsAttributes->GetNumberOfArrays()-1,attributeType);
      }
    ++i;
    }
  
  vtkIdTypeArray *cellIdArray;
  
  if(this->KeepCellIds)
    {
    cellIdArray=vtkIdTypeArray::New();
    cellIdArray->SetName("OriginalIds");
    }
  
  vtkGenericCellIterator *cellIt = input->NewCellIterator();
  vtkIdType updateCount = numCells/20 + 1;  // update roughly every 5%
  vtkIdType count = 0;
  
  input->GetTessellator()->InitErrorMetrics(input);
  
  for(cellIt->Begin(); !cellIt->IsAtEnd() && !abortExecute; cellIt->Next(), count++)
    {
    if ( !(count % updateCount) )
      {
      this->UpdateProgress((double)count / numCells);
      abortExecute = this->GetAbortExecute();
      }
      
    cell = cellIt->GetCell();
    cell->Tessellate(input->GetAttributes(), input->GetTessellator(),
                     newPts, conn, this->internalPD, outputPD, outputCD);    
    numNew = conn->GetNumberOfCells() - numInserted;
    numInserted = conn->GetNumberOfCells();
    
    vtkIdType cellId=cell->GetId();
    
    if(this->KeepCellIds)
      {
      for(i=0;i<numNew;i++)
        {
        cellIdArray->InsertNextValue(cellId);
        }
      }
    
    for (i=0; i < numNew; i++) 
      {
      locs->InsertNextValue(conn->GetTraversalLocation());
      conn->GetNextCell(npts,pts); //side effect updates traversal location
     
      switch (cell->GetDimension())
        {
        case 1:
          types->InsertNextValue(VTK_LINE);
          break;
        case 2:
          types->InsertNextValue(VTK_TRIANGLE);
          break;
        case 3:
          types->InsertNextValue(VTK_TETRA);
          break;
        default:
          vtkErrorMacro(<<"Bad mojo in data set tessellation");
        } //switch
      } //insert each new cell
    } //for all cells
  cellIt->Delete();
  
  // Send to the output
  if(this->KeepCellIds)
    {
    outputCD->AddArray(cellIdArray);
    cellIdArray->Delete();
    }
  
  
  output->SetPoints(newPts);
  output->SetCells(types, locs, conn);

  // Init the active attributes
  
  
  vtkDebugMacro(<<"Subdivided " << numCells << " cells to produce "
                << conn->GetNumberOfCells() << "new cells");

  newPts->Delete();
  types->Delete();
  locs->Delete();
  conn->Delete();

  output->Squeeze();  
}

//----------------------------------------------------------------------------
void vtkGenericDataSetTessellator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "keep cells ids=";
  if(this->KeepCellIds)
    {
    os << "true" << endl;
    }
  else
    {
    os << "false" << endl;
    }
}
