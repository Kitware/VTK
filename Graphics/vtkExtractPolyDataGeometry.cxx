/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractPolyDataGeometry.cxx
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
#include "vtkExtractPolyDataGeometry.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"

//-------------------------------------------------------------------------
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
  this->ExtractBoundaryCells = 0;
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
  vtkPoints *inPts=input->GetPoints();
  vtkIdType numPts, numCells, i, cellId, newId;
  float multiplier;
  vtkCellArray *inVerts, *inLines, *inPolys, *inStrips;
  vtkCellArray *newVerts=NULL, *newLines=NULL, *newPolys=NULL, *newStrips=NULL;

  vtkDebugMacro(<< "Extracting poly data geometry");

  if ( ! this->ImplicitFunction )
    {
    vtkErrorMacro(<<"No implicit function specified");
    return;
    }

  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();

  if ( this->ExtractInside )
    {
    multiplier = 1.0;
    }
  else 
    {
    multiplier = -1.0;
    }

  // Use a templated function to access the points. The points are
  // passed through, but scalar values are generated.
  vtkFloatArray *newScalars = vtkFloatArray::New();
  newScalars->SetNumberOfValues(numPts);

  for (int ptId=0; ptId < numPts; ptId++ )
    {
    newScalars->SetValue(ptId, this->ImplicitFunction->
                         FunctionValue(inPts->GetPoint(ptId))*multiplier);
    }

  output->SetPoints(inPts);
  outputPD->PassData(pd);

  // Now loop over all cells to see whether they are inside the implicit
  // function. Copy if they are. Note: there is an awful hack here, that
  // can result in bugs. The cellId is assumed to be arranged starting
  // with the verts, then lines, then polys, then strips.
  //
  int abort=0;
  int numIn;
  vtkIdType npts, *pts;
  if ( input->GetNumberOfVerts() )
    {
    inVerts = input->GetVerts();
    newVerts = vtkCellArray::New();
    newVerts->Allocate(inVerts->GetSize());
    }
  if ( input->GetNumberOfLines() )
    {
    inLines = input->GetLines();
    newLines = vtkCellArray::New();
    newLines->Allocate(inLines->GetSize());
    }
  if ( input->GetNumberOfPolys() )
    {
    inPolys = input->GetPolys();
    newPolys = vtkCellArray::New();
    newPolys->Allocate(inPolys->GetSize());
    }
  if ( input->GetNumberOfStrips() )
    {
    inStrips = input->GetStrips();
    newStrips = vtkCellArray::New();
    newStrips->Allocate(inStrips->GetSize());
    }
  
  // verts
  if ( newVerts && !this->GetAbortExecute() )
    {
    for (inVerts->InitTraversal(); inVerts->GetNextCell(npts,pts); )
      {
      for (numIn=0, i=0; i<npts; i++)
        {
        if ( newScalars->GetValue(pts[i]) <= 0.0 )
          {
          numIn++;
          }
        }
      if ( (numIn == npts) || (this->ExtractBoundaryCells && numIn > 0) )
        {
        newId = newVerts->InsertNextCell(npts,pts);
        outputCD->CopyData(cd, cellId, newId);
        }
      cellId++;
      }
    }
  this->UpdateProgress (0.6);

  // lines
  if ( newLines && !this->GetAbortExecute() )
    {
    for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); )
      {
      for (numIn=0, i=0; i<npts; i++)
        {
        if ( newScalars->GetValue(pts[i]) <= 0.0 )
          {
          numIn++;
          }
        }
      if ( (numIn == npts) || (this->ExtractBoundaryCells && numIn > 0) )
        {
        newId = newLines->InsertNextCell(npts,pts);
        outputCD->CopyData(cd, cellId, newId);
        }
      cellId++;
      }
    }
  this->UpdateProgress (0.75);
  
  // polys
  if ( newPolys && !this->GetAbortExecute() )
    {
    for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
      {
      for (numIn=0, i=0; i<npts; i++)
        {
        if ( newScalars->GetValue(pts[i]) <= 0.0 )
          {
          numIn++;
          }
        }
      if ( (numIn == npts) || (this->ExtractBoundaryCells && numIn > 0) )
        {
        newId = newPolys->InsertNextCell(npts,pts);
        outputCD->CopyData(cd, cellId, newId);
        }
      cellId++;
      }
    }
  this->UpdateProgress (0.90);
  
  // strips
  if ( newStrips && !this->GetAbortExecute() )
    {
    for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
      {
      for (numIn=0, i=0; i<npts; i++)
        {
        if ( newScalars->GetValue(pts[i]) <= 0.0 )
          {
          numIn++;
          }
        }
      if ( (numIn == npts) || (this->ExtractBoundaryCells && numIn > 0) )
        {
        newId = newStrips->InsertNextCell(npts,pts);
        outputCD->CopyData(cd, cellId, newId);
        }
      cellId++;
      }
    }
  this->UpdateProgress (1.0);
  
  // Update ourselves and release memory
  //
  newScalars->Delete();

  if ( newVerts )
    {
    output->SetVerts(newVerts);
    newVerts->Delete();
    }
  if ( newLines )
    {
    output->SetLines(newLines);
    newLines->Delete();
    }
  if ( newPolys )
    {
    output->SetPolys(newPolys);
    newPolys->Delete();
    }
  if ( newStrips )
    {
    output->SetStrips(newStrips);
    newStrips->Delete();
    }
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
  os << indent << "Extract Boundary Cells: " 
     << (this->ExtractBoundaryCells ? "On\n" : "Off\n");
}
