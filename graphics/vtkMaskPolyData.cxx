/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMaskPolyData.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkMaskPolyData.h"

vtkMaskPolyData::vtkMaskPolyData()
{
  this->OnRatio = 11;
  this->Offset = 0;
}

//
// Down sample polygonal data.  Don't down sample points (that is, use the
// original points, since usually not worth it.
//
void vtkMaskPolyData::Execute()
{
  int numVerts, numLines, numPolys, numStrips;
  vtkCellArray *inVerts,*inLines,*inPolys,*inStrips;
  int numNewVerts, numNewLines, numNewPolys, numNewStrips;
  vtkCellArray *newVerts=NULL, *newLines=NULL;
  vtkCellArray *newPolys=NULL, *newStrips=NULL;
  int id, interval;
  vtkPointData *pd;
  int npts, *pts;
  vtkPolyData *input=(vtkPolyData *)this->Input;
  vtkPolyData *output = this->GetOutput();
  
  //
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

  if ( numNewVerts < 1 && numNewLines < 1 &&
  numNewPolys < 1 && numNewStrips < 1 )
    {
    vtkErrorMacro (<<"No PolyData to mask!");
    return;
    }
//
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
//
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
    }

  if ( newStrips )
    {
    for (id=0, inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); id++)
      {
      if ( ! (id % interval) )
        {
        newStrips->InsertNextCell(npts,pts);
        }
      }
    }
  //
  // Update ourselves and release memory
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
  vtkPolyToPolyFilter::PrintSelf(os,indent);

  os << indent << "On Ratio: " << this->OnRatio << "\n";
  os << indent << "Offset: " << this->Offset << "\n";
}
