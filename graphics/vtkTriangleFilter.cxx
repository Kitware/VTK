/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangleFilter.cxx
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
#include "vtkTriangleFilter.h"
#include "vtkPolygon.h"
#include "vtkTriangleStrip.h"

void vtkTriangleFilter::Execute()
{
  vtkPolyData *input=(vtkPolyData *)this->Input;
  vtkCellArray *inPolys=input->GetPolys();
  vtkCellArray *inStrips=input->GetStrips();;
  int npts, *pts;
  vtkCellArray *newPolys;
  int numCells;
  vtkPolygon poly;
  int i, j;
  vtkIdList outVerts(3*VTK_CELL_SIZE);
  vtkPoints *inPoints=input->GetPoints();
  vtkPointData *pd;
  vtkPolyData *output=(vtkPolyData *)this->Output;

  vtkDebugMacro(<<"Executing triangle filter");

  newPolys = vtkCellArray::New();
  // approximation
  numCells = input->GetNumberOfPolys() + input->GetNumberOfStrips();
  newPolys->Allocate(newPolys->EstimateSize(numCells,3),3*numCells);

  // pass through triangles; triangulate polygons if necessary
  for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
    {
    if ( npts == 3 )
      {
      newPolys->InsertNextCell(npts,pts);
      }
    else if ( npts > 3 ) // triangulate poly
      {
      poly.Initialize(npts,pts,inPoints);
      poly.Triangulate(outVerts);
      for (i=0; i<outVerts.GetNumberOfIds()/3; i++)
        {
        newPolys->InsertNextCell(3);
        for (j=0; j<3; j++)
          newPolys->InsertCellPoint(outVerts.GetId(3*i+j));
        }
      }
    }

  if ( inStrips->GetNumberOfCells() > 0 )
    {
    vtkTriangleStrip strip;
    strip.DecomposeStrips(inStrips,newPolys);
    }
//
// Update ourselves
//
  newPolys->Squeeze();
  output->SetPolys(newPolys);
  newPolys->Delete();

  // pass through points and point data
  output->SetPoints(input->GetPoints());
  pd = input->GetPointData();
  output->GetPointData()->PassData(input->GetPointData());

  // pass through other stuff if requested
  if ( this->PassVerts ) output->SetVerts(input->GetVerts());
  if ( this->PassLines ) output->SetLines(input->GetLines());

  vtkDebugMacro(<<"Converted " << inPolys->GetNumberOfCells() <<
               " polygons and " << inStrips->GetNumberOfCells() <<
               " strips to " << newPolys->GetNumberOfCells() <<
               " triangles");
}

void vtkTriangleFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyToPolyFilter::PrintSelf(os,indent);

  os << indent << "Pass Verts: " << (this->PassVerts ? "On\n" : "Off\n");
  os << indent << "Pass Lines: " << (this->PassLines ? "On\n" : "Off\n");

}

