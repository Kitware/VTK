/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractPolyDataGeometry.cxx
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
#include "vtkExtractPolyDataGeometry.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkExtractPolyDataGeometry, "1.16");
vtkStandardNewMacro(vtkExtractPolyDataGeometry);

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
  vtkIdType numPts, i, cellId = -1, newId;
  float multiplier;
  vtkCellArray *inVerts=NULL, *inLines=NULL, *inPolys=NULL, *inStrips=NULL;
  vtkCellArray *newVerts=NULL, *newLines=NULL, *newPolys=NULL, *newStrips=NULL;

  vtkDebugMacro(<< "Extracting poly data geometry");

  if ( ! this->ImplicitFunction )
    {
    vtkErrorMacro(<<"No implicit function specified");
    return;
    }

  numPts = input->GetNumberOfPoints();

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
  int numIn;
  vtkIdType npts = 0;
  vtkIdType *pts = 0;
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
  this->Superclass::PrintSelf(os,indent);

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
