/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMaskPolyData.cxx
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
#include "vtkMaskPolyData.h"
#include "vtkObjectFactory.h"

//--------------------------------------------------------------------------
vtkMaskPolyData* vtkMaskPolyData::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMaskPolyData");
  if(ret)
    {
    return (vtkMaskPolyData*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMaskPolyData;
}

vtkMaskPolyData::vtkMaskPolyData()
{
  this->OnRatio = 11;
  this->Offset = 0;
}

// Down sample polygonal data.  Don't down sample points (that is, use the
// original points, since usually not worth it.
//
void vtkMaskPolyData::Execute()
{
  vtkIdType numVerts, numLines, numPolys, numStrips;
  vtkCellArray *inVerts,*inLines,*inPolys,*inStrips;
  vtkIdType numNewVerts, numNewLines, numNewPolys, numNewStrips;
  vtkCellArray *newVerts=NULL, *newLines=NULL;
  vtkCellArray *newPolys=NULL, *newStrips=NULL;
  vtkIdType id, interval;
  vtkPointData *pd;
  vtkIdType numCells;
  vtkIdType *pts, npts;
  vtkPolyData *input= this->GetInput();
  vtkPolyData *output = this->GetOutput();
  
  // Check input / pass data through
  //
  inVerts = input->GetVerts();
  numVerts = inVerts->GetNumberOfCells();
  numNewVerts = numVerts / this->OnRatio;

  inLines = input->GetLines();
  numLines = inLines->GetNumberOfCells();
  numNewLines = numLines / this->OnRatio;

  inPolys = input->GetPolys();
  numPolys = inPolys->GetNumberOfCells();
  numNewPolys = numPolys / this->OnRatio;

  inStrips = input->GetStrips();
  numStrips = inStrips->GetNumberOfCells();
  numNewStrips = numStrips / this->OnRatio;

  numCells = numVerts + numLines + numPolys + numStrips;

  if ( numCells < 1 )
    {
    vtkErrorMacro (<<"No PolyData to mask!");
    return;
    }

  // Allocate space
  //
  if ( numNewVerts )
    {
    newVerts = vtkCellArray::New();
    newVerts->Allocate(numNewVerts);
    }

  if ( numNewLines )
    {
    newLines = vtkCellArray::New();
    newLines->Allocate(newLines->EstimateSize(numNewLines,2));
    }

  if ( numNewPolys )
    {
    newPolys = vtkCellArray::New();
    newPolys->Allocate(newPolys->EstimateSize(numNewPolys,4));
    }

  if ( numNewStrips )
    {
    newStrips = vtkCellArray::New();
    newStrips->Allocate(newStrips->EstimateSize(numNewStrips,6));
    }

  // Traverse topological lists and traverse
  //
  interval = this->Offset + this->OnRatio;
  if ( newVerts )
    {
    for (id=0, inVerts->InitTraversal(); inVerts->GetNextCell(npts,pts); id++)
      {
      if ( ! (id % interval) )
        {
        newVerts->InsertNextCell(npts,pts);
        }
      }
    this->UpdateProgress((float)numVerts/numCells);
    }

  if ( newLines )
    {
    for (id=0, inLines->InitTraversal(); inLines->GetNextCell(npts,pts); id++)
      {
      if ( ! (id % interval) )
        {
        newLines->InsertNextCell(npts,pts);
        }
      }
    this->UpdateProgress((float)(numVerts+numLines)/numCells);
    }

  if ( newPolys )
    {
    for (id=0, inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); id++)
      {
      if ( ! (id % interval) )
        {
        newPolys->InsertNextCell(npts,pts);
        }
      }
    this->UpdateProgress((float)(numVerts+numLines+numPolys)/numCells);
    }

  if ( newStrips )
    {
    for (id=0, inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts);
         id++)
      {
      if ( ! (id % interval) )
        {
        newStrips->InsertNextCell(npts,pts);
        }
      }
    }

  // Update ourselves and release memory
  //
  output->SetPoints(input->GetPoints());
  pd = input->GetPointData();
  output->GetPointData()->PassData(pd);

  if (newVerts)
    {
    output->SetVerts(newVerts);
    newVerts->Delete();
    }

  if (newLines)
    {
    output->SetLines(newLines);
    newLines->Delete();
    } 

  if (newPolys)
    {
    output->SetPolys(newPolys);
    newPolys->Delete();
    }

  if (newStrips)
    {
    output->SetStrips(newStrips);
    newStrips->Delete();
    }

  output->Squeeze();
}

void vtkMaskPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "On Ratio: " << this->OnRatio << "\n";
  os << indent << "Offset: " << this->Offset << "\n";
}
