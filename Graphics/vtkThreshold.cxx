/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreshold.cxx
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
#include "vtkThreshold.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkThreshold, "1.58");
vtkStandardNewMacro(vtkThreshold);

// Construct with lower threshold=0, upper threshold=1, and threshold 
// function=upper AllScalars=1.
vtkThreshold::vtkThreshold()
{
  this->LowerThreshold = 0.0;
  this->UpperThreshold = 1.0;
  this->AllScalars = 1;
  this->AttributeMode = VTK_ATTRIBUTE_MODE_USE_POINT_DATA;
  this->ThresholdFunction = &vtkThreshold::Upper;
  this->InputScalarsSelection = NULL;
}

vtkThreshold::~vtkThreshold()
{
  // This frees the string
  this->SetInputScalarsSelection(NULL);
}

// Criterion is cells whose scalars are less or equal to lower threshold.
void vtkThreshold::ThresholdByLower(float lower) 
{
  if ( this->LowerThreshold != lower || 
       this->ThresholdFunction != &vtkThreshold::Lower)
    {
    this->LowerThreshold = lower; 
    this->ThresholdFunction = &vtkThreshold::Lower;
    this->Modified();
    }
}
                           
// Criterion is cells whose scalars are greater or equal to upper threshold.
void vtkThreshold::ThresholdByUpper(float upper)
{
  if ( this->UpperThreshold != upper ||
       this->ThresholdFunction != &vtkThreshold::Upper)
    {
    this->UpperThreshold = upper; 
    this->ThresholdFunction = &vtkThreshold::Upper;
    this->Modified();
    }
}
                           
// Criterion is cells whose scalars are between lower and upper thresholds.
void vtkThreshold::ThresholdBetween(float lower, float upper)
{
  if ( this->LowerThreshold != lower || this->UpperThreshold != upper ||
       this->ThresholdFunction != &vtkThreshold::Between)
    {
    this->LowerThreshold = lower; 
    this->UpperThreshold = upper;
    this->ThresholdFunction = &vtkThreshold::Between;
    this->Modified();
    }
}
  
void vtkThreshold::Execute()
{
  vtkIdType cellId, newCellId;
  vtkIdList *cellPts, *pointMap;
  vtkIdList *newCellPts;
  vtkCell *cell;
  vtkPoints *newPoints;
  int i, ptId, newId, numPts;
  int numCellPts;
  float *x;
  vtkDataSet *input = this->GetInput();
  
  if (!input)
    {
    vtkErrorMacro(<<"No input, Can't Execute");
    }
  vtkUnstructuredGrid *output = this->GetOutput();
  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *cd=input->GetCellData(), *outCD=output->GetCellData();
  vtkDataArray *pointScalars;
  vtkDataArray *cellScalars;
  int keepCell, usePointScalars;

  vtkDebugMacro(<< "Executing threshold filter");
  
  // I am explicitly checking to avoid a warning if the array is not found.
  if (this->InputScalarsSelection)
    {
    pointScalars=pd->GetArray(this->InputScalarsSelection);
    cellScalars=cd->GetArray(this->InputScalarsSelection);
    }
  else
    {
    pointScalars=pd->GetScalars();
    cellScalars=cd->GetScalars();
    }

  outPD->CopyAllocate(pd);
  outCD->CopyAllocate(cd);

  if ( !(pointScalars || cellScalars) )
    {
    vtkErrorMacro(<<"No scalar data to threshold");
    return;
    }

  numPts = input->GetNumberOfPoints();
  output->Allocate(input->GetNumberOfCells());
  newPoints = vtkPoints::New();
  newPoints->Allocate(numPts);

  pointMap = vtkIdList::New(); //maps old point ids into new
  pointMap->SetNumberOfIds(numPts);
  for (i=0; i < numPts; i++)
    {
    pointMap->SetId(i,-1);
    }

  // Determine which scalar data to use for thresholding
  if ( this->AttributeMode == VTK_ATTRIBUTE_MODE_DEFAULT )
    {
    if ( pointScalars != NULL)
      {
      usePointScalars = 1;
      }
    else
      {
      usePointScalars = 0;
      }
    }
  else if ( this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_POINT_DATA )
    {
    usePointScalars = 1;
    }
  else
    {
    usePointScalars = 0;
    }

  // Check on scalar consistency
  if ( usePointScalars && !pointScalars )
    {
    vtkErrorMacro(<<"Can't use point scalars because there are none");
    return;
    }
  else if ( !usePointScalars && !cellScalars )
    {
    vtkErrorMacro(<<"Can't use cell scalars because there are none");
    return;
    }

  newCellPts = vtkIdList::New();     

  // Check that the scalars of each cell satisfy the threshold criterion
  for (cellId=0; cellId < input->GetNumberOfCells(); cellId++)
    {
    cell = input->GetCell(cellId);
    cellPts = cell->GetPointIds();
    numCellPts = cell->GetNumberOfPoints();
    
    if ( usePointScalars )
      {
      if (this->AllScalars)
        {
        keepCell = 1;
        for ( i=0; keepCell && (i < numCellPts); i++)
          {
          ptId = cellPts->GetId(i);
          keepCell = 
            (this->*(this->ThresholdFunction))(pointScalars->GetComponent(ptId,0));
          }
        }
      else
        {
        keepCell = 0;
        for ( i=0; (!keepCell) && (i < numCellPts); i++)
          {
          ptId = cellPts->GetId(i);
          keepCell = 
            (this->*(this->ThresholdFunction))(pointScalars->GetComponent(ptId,0));
          }
        }
      }
    else //use cell scalars
      {
      keepCell = (this->*(this->ThresholdFunction))(cellScalars->GetComponent(cellId,0));
      }
    
    if ( keepCell ) // satisfied thresholding
      {
      for (i=0; i < numCellPts; i++)
        {
        ptId = cellPts->GetId(i);
        if ( (newId = pointMap->GetId(ptId)) < 0 )
          {
          x = input->GetPoint(ptId);
          newId = newPoints->InsertNextPoint(x);
          pointMap->SetId(ptId,newId);
          outPD->CopyData(pd,ptId,newId);
          }
        newCellPts->InsertId(i,newId);
        }
      newCellId = output->InsertNextCell(cell->GetCellType(),newCellPts);
      outCD->CopyData(cd,cellId,newCellId);
      newCellPts->Reset();
      } // satisfied thresholding
    } // for all cells

  vtkDebugMacro(<< "Extracted " << output->GetNumberOfCells() 
                << " number of cells.");

  // now clean up / update ourselves
  pointMap->Delete();
  newCellPts->Delete();
  
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->Squeeze();
}

// Return the method for manipulating scalar data as a string.
const char *vtkThreshold::GetAttributeModeAsString(void)
{
  if ( this->AttributeMode == VTK_ATTRIBUTE_MODE_DEFAULT )
    {
    return "Default";
    }
  else if ( this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_POINT_DATA )
    {
    return "UsePointData";
    }
  else 
    {
    return "UseCellData";
    }
}

void vtkThreshold::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Attribute Mode: " << this->GetAttributeModeAsString() << endl;
  if (this->InputScalarsSelection)
    {
    os << indent << "InputScalarsSelection: " << this->InputScalarsSelection;
    } 

  os << indent << "All Scalars: " << this->AllScalars << "\n";
  if ( this->ThresholdFunction == &vtkThreshold::Upper )
    {
    os << indent << "Threshold By Upper\n";
    }

  else if ( this->ThresholdFunction == &vtkThreshold::Lower )
    {
    os << indent << "Threshold By Lower\n";
    }

  else if ( this->ThresholdFunction == &vtkThreshold::Between )
    {
    os << indent << "Threshold Between\n";
    }

  os << indent << "Lower Threshold: " << this->LowerThreshold << "\n";
  os << indent << "Upper Threshold: " << this->UpperThreshold << "\n";
}
