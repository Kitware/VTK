/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractPolyDataGeometry.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkExtractPolyDataGeometry.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkExtractPolyDataGeometry* vtkExtractPolyDataGeometry::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkExtractPolyDataGeometry");
  if(ret)
    {
    return (vtkExtractPolyDataGeometry*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkExtractPolyDataGeometry;
}




// Construct object with ExtractInside turned on.
vtkExtractPolyDataGeometry::vtkExtractPolyDataGeometry(vtkImplicitFunction *f)
{
  this->ImplicitFunction = f;
  if (this->ImplicitFunction)
    {
    this->ImplicitFunction->Register(this);
    }
    
  this->ExtractInside = 1;
}

vtkExtractPolyDataGeometry::~vtkExtractPolyDataGeometry()
{
  this->SetImplicitFunction(NULL);
}

// Overload standard modified time function. If implicit function is modified,
// then this object is modified as well.
unsigned long vtkExtractPolyDataGeometry::GetMTime()
{
  unsigned long mTime=this->MTime.GetMTime();
  unsigned long impFuncMTime;

  if ( this->ImplicitFunction != NULL )
    {
    impFuncMTime = this->ImplicitFunction->GetMTime();
    mTime = ( impFuncMTime > mTime ? impFuncMTime : mTime );
    }

  return mTime;
}

void vtkExtractPolyDataGeometry::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  int ptId, numPts, numCells, i, cellId;
  int npts, allInside;
  int newId, *pointMap, updateInterval;
  float multiplier, x[3];
  vtkPoints *newPts;

  vtkDebugMacro(<< "Extracting poly data geometry");

  if ( ! this->ImplicitFunction )
    {
    vtkErrorMacro(<<"No implicit function specified");
    return;
    }

  if ( this->ExtractInside )
    {
    multiplier = 1.0;
    }
  else 
    {
    multiplier = -1.0;
    }

  // Loop over all points determining whether they are inside implicit function
  // Copy if they are.
  //
  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();
  pointMap = new int[numPts]; // maps old point ids into new
  for (i=0; i < numPts; i++)
    {
    pointMap[i] = -1;
    }

  newPts = vtkPoints::New();
  newPts->Allocate(numPts/4,numPts);
  outputPD->CopyAllocate(pd);
  
  for ( allInside=1, ptId=0; ptId < numPts; ptId++ )
    {
    input->GetPoint(ptId, x);
    if ( (this->ImplicitFunction->FunctionValue(x)*multiplier) <= 0.0 )
      {
      newId = newPts->InsertNextPoint(x);
      pointMap[ptId] = newId;
      outputPD->CopyData(pd,ptId,newId);
      }
    else
      {
      allInside = 0;
      }
    }

  if ( allInside ) //we'll just pass the data through
    {
    output->CopyStructure(input);
    outputCD->PassData(cd);
    newPts->Delete();
    return;
    }

  // Now loop over all cells to see whether they are inside the implicit
  // function. Copy if they are.
  //
  vtkGenericCell *cell=vtkGenericCell::New();
  vtkIdList *ptIds=vtkIdList::New();
  
  output->Allocate(numCells);
  outputCD->CopyAllocate(cd);

  updateInterval = numCells / 1000;
  if (updateInterval < 1)
    {
    updateInterval = 1;
    }

  // Loop over all cells inserting those that are "in"
  for (cellId=0; cellId < numCells; cellId++)
    {

    //manage progress reports / early abort
    if ( ! (cellId % updateInterval) ) 
      {
      this->UpdateProgress ((float)cellId / numCells);
      if ( this->GetAbortExecute() ) 
        {
        break;
        }
      }

    //check to see whether points are inside
    input->GetCell(cellId, cell);
    npts = cell->PointIds->GetNumberOfIds();
    ptIds->SetNumberOfIds(npts);
    for (i=0; i<npts; i++)
      {
      newId = cell->PointIds->GetId(i);
      if ( pointMap[newId] < 0 )
        {
        break;
        }
      ptIds->SetId(i,pointMap[newId]);
      }
    if ( i >= npts )
      {
      newId = output->InsertNextCell(cell->GetCellType(), ptIds);
      outputCD->CopyData(cd, cellId, newId);
      }

    }//for all cells

  // Update ourselves and release memory
  //
  ptIds->Delete();
  delete [] pointMap;
  cell->Delete();
  output->SetPoints(newPts);
  newPts->Delete();

  output->Squeeze();
}

void vtkExtractPolyDataGeometry::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  if (this->ImplicitFunction)
    {
    os << indent << "Implicit Function: " 
       << (void *)this->ImplicitFunction << "\n";
    }
  else
    {
    os << indent << "Implicit Function: (null)\n";      
    }
  os << indent << "Extract Inside: " 
     << (this->ExtractInside ? "On\n" : "Off\n");
}
