/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRuledSurfaceFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkRuledSurfaceFilter.h"
#include "vtkPolyLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------
vtkRuledSurfaceFilter* vtkRuledSurfaceFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkRuledSurfaceFilter");
  if(ret)
    {
    return (vtkRuledSurfaceFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkRuledSurfaceFilter;
}

vtkRuledSurfaceFilter::vtkRuledSurfaceFilter()
{
  this->DistanceFactor = 3.0;
  this->OnRatio = 1;
  this->Offset = 0;
  this->CloseSurface = 0;
  this->PassLines = 0;
}

void vtkRuledSurfaceFilter::Execute()
{
  vtkPoints *inPts;
  int i, numPts, numLines;
  vtkCellArray *inLines;
  vtkCellArray *newPolys;
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  int npts, *pts;
  int npts2, *pts2;
  int loc, loc2;

  // Check input
  //
  vtkDebugMacro(<<"Creating a ruled surface");

  if ( !(inPts=input->GetPoints()) || 
      (numPts = inPts->GetNumberOfPoints()) < 1 ||
      !(inLines = input->GetLines()) || 
       (numLines=inLines->GetNumberOfCells()) < 2 )
    {
    vtkDebugMacro(<< "No input data!\n");
    return;
    }
  
  // Allocate memory for surface...we're gonna pass the points through
  //
  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(numLines,numPts/numLines));
  output->SetPoints(inPts);
  output->GetPointData()->PassData(input->GetPointData());

  // For each pair of lines (as selected by Offset and OnRatio), create a
  // stripe (a ruled surfac between two lines). 
  //
  inLines->InitTraversal();
  inLines->GetNextCell(npts,pts);
  for (i=0; i<numLines; i++)
    {
    inLines->GetNextCell(npts2,pts2); //get the next edge

    // Determine whether this stripe should be generated
    if ( (i-this->Offset) >= 0 && !((i - this->Offset ) % this->OnRatio) )
      {
      // Walk "edge" along the two lines maintaining closest distance
      // and generating triangles as we go.
      loc = loc2 = 0;
      while ( loc<npts && loc2<npts2 )
        {
        }//while still building the stripe
      }//generate this stripe
    
    //Get the next line for generating the next stripe
    npts = npts2;
    pts = pts2;
    if ( i == (numLines-1) )
      {
      if ( this->CloseSurface )
        {
        inLines->InitTraversal();
        }
      else 
        {
        i++; //will cause the loop to end
        }
      }//add far boundary of surface
    }//for all selected line pairs

  // Update the output
  //
  if ( this->PassLines )
    {
    output->SetLines(inLines);
    }
  newPolys->Squeeze();
  output->SetPolys(newPolys);
  newPolys->Delete();
}

void vtkRuledSurfaceFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Distance Factor: " << this->DistanceFactor << "\n";
  os << indent << "On Ratio: " << this->OnRatio << "\n";
  os << indent << "Offset: " << this->Offset << "\n";
  os << indent << "Close Surface: " << (this->CloseSurface ? "On\n" : "Off\n");
  os << indent << "Pass Lines: " << (this->PassLines ? "On\n" : "Off\n");
}

