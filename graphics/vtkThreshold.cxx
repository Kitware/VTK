/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreshold.cxx
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
#include "vtkThreshold.h"
#include "vtkObjectFactory.h"

//---------------------------------------------------------------------------
vtkThreshold* vtkThreshold::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkThreshold");
  if(ret)
    {
    return (vtkThreshold*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkThreshold;
}

// Construct with lower threshold=0, upper threshold=1, and threshold 
// function=upper AllScalars=1.
vtkThreshold::vtkThreshold()
{
  this->LowerThreshold = 0.0;
  this->UpperThreshold = 1.0;
  this->AllScalars = 1;
  this->AttributeMode = VTK_ATTRIBUTE_MODE_USE_POINT_DATA;
  this->ThresholdFunction = &vtkThreshold::Upper;
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
  vtkIdList *newCellPts = vtkIdList::New();
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
  vtkScalars *pointScalars=pd->GetScalars(), *cellScalars=cd->GetScalars();
  int keepCell, usePointScalars;
  
  vtkDebugMacro(<< "Executing threshold filter");

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
	    (this->*(this->ThresholdFunction))(pointScalars->GetScalar(ptId));
	  }
	}
      else
	{
	keepCell = 0;
	for ( i=0; (!keepCell) && (i < numCellPts); i++)
	  {
	  ptId = cellPts->GetId(i);
	  keepCell = 
	    (this->*(this->ThresholdFunction))(pointScalars->GetScalar(ptId));
	  }
	}
      }
    else //use cell scalars
      {
      keepCell = (this->*(this->ThresholdFunction))(cellScalars->GetScalar(cellId));
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
  vtkDataSetToUnstructuredGridFilter::PrintSelf(os,indent);

  os << indent << "Attribute Mode: " << this->GetAttributeModeAsString() << endl;
  os << indent << "All Scalars: " << this->AllScalars << "\n";;
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

  os << indent << "Lower Threshold: " << this->LowerThreshold << "\n";;
  os << indent << "Upper Threshold: " << this->UpperThreshold << "\n";;
}
