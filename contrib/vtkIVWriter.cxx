/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIVWriter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    to Jon A. Webb for contributing this class.

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
#include <stdio.h>
#include "vtkIVWriter.h"
#include "vtkPolyDataMapper.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkIVWriter* vtkIVWriter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkIVWriter");
  if(ret)
    {
    return (vtkIVWriter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkIVWriter;
}




void vtkIVWriter::WriteData()
{
  FILE *fp;
  
  // make sure the user specified a FileName
  if ( this->FileName == NULL)
    {
    vtkErrorMacro(<< "Please specify FileName to use");
    return;
    }

  // try opening the files
  fp = fopen(this->FileName,"w");
  if (!fp)
    {
    vtkErrorMacro(<< "unable to open OpenInventor file: " << this->FileName);
    return;
    }
  
  //
  //  Write header
  //
  vtkDebugMacro("Writing OpenInventor file");
  fprintf(fp,"#Inventor V2.0 ascii\n");
  fprintf(fp,"# OpenInventor file written by the visualization toolkit\n\n");
  this->WritePolyData(this->GetInput(), fp);
  if (fclose(fp)) 
    {
    vtkErrorMacro(<< this->FileName << " did not close successfully. Check disk space.");
    }
}

void vtkIVWriter::WritePolyData(vtkPolyData *pd, FILE *fp)
{
  vtkPoints *points = NULL;
  int i;
  vtkCellArray *cells;
  vtkIdType npts;
  vtkIdType *indx;
  vtkPolyDataMapper *pm;
  vtkScalars *colors;
  
  pm = vtkPolyDataMapper::New();
  pm->SetInput(pd);
  
  points = pd->GetPoints();
  colors  = pm->GetColors();
  
  fprintf(fp,"Separator {\n");
  
  // Point data (coordinates)
  fprintf(fp,"\tCoordinate3 {\n");
  fprintf(fp,"\t\tpoint [\n");
  fprintf(fp,"\t\t\t");
  for (i=0; i<points->GetNumberOfPoints(); i++)
    {
    float xyz[3];
    points->GetPoint(i, xyz);
    fprintf(fp, "%g %g %g, ", xyz[0], xyz[1], xyz[2]);
    if (!((i+1)%2))
      {
      fprintf(fp, "\n\t\t\t");
      }
    }
  fprintf(fp, "\n\t\t]");
  fprintf(fp, "\t}\n");
  
  // Per vertex coloring
  fprintf(fp,"\tMaterialBinding {\n");
  fprintf(fp,"\t\tvalue PER_VERTEX_INDEXED\n");
  fprintf(fp,"\t}\n");
  
  // Colors, if any
  if (colors) 
    {
    fprintf(fp,"\tMaterial {\n");
    fprintf(fp,"\t\tdiffuseColor [\n");
    fprintf(fp, "\t\t\t");
    for (i=0; i<colors->GetNumberOfScalars(); i++) 
      {
      unsigned char *rgba;
      rgba = colors->GetColor(i);
      fprintf(fp, "%g %g %g, ", rgba[0]/255.0f, 
	      rgba[1]/255.0f, rgba[2]/255.0f);
      if (!((i+1)%2))
	{
	fprintf(fp, "\n\t\t\t");
	}
      }
    fprintf(fp, "\n\t\t]\n");
    fprintf(fp,"\t}\n");
    }
  
  // write out polys if any
  if (pd->GetNumberOfPolys() > 0)
    {
    fprintf(fp,"\tIndexedFaceSet {\n");
    fprintf(fp,"\t\tcoordIndex [\n");
    cells = pd->GetPolys();
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
      {
      fprintf(fp, "\t\t\t");
      for (i = 0; i < npts; i++)
	{
	fprintf(fp,"%i, ",indx[i]);
	}
      fprintf(fp,"-1,\n");
      }
    fprintf(fp,"\t\t]\n");
    fprintf(fp,"\t}\n");
    }
  
  // write out lines if any
  if (pd->GetNumberOfLines() > 0)
    {
    fprintf(fp,"\tIndexedLineSet {\n");
    fprintf(fp,"\t\tcoordIndex  [\n");
    
    cells = pd->GetLines();
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
      {
      fprintf(fp,"\t\t\t");
      for (i = 0; i < npts; i++)
	{
	fprintf(fp,"%i, ",indx[i]);
	}
      fprintf(fp,"-1,\n");
      }
    fprintf(fp,"\t\t]\n");
    fprintf(fp,"\t}\n");
    }
  
  // write out verts if any
  if (pd->GetNumberOfVerts() > 0)
    {
    fprintf(fp,"\tIndexdedPointSet {\n");
    fprintf(fp,"\t\tcoordIndex [");
    cells = pd->GetVerts();
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
      {
      fprintf(fp,"\t\t\t");
      for (i = 0; i < npts; i++)
	{
	fprintf(fp,"%i, ",indx[i]);
	}
      fprintf(fp,"-1,\n");
      }
    fprintf(fp,"\t\t]\n");
    fprintf(fp,"\t}\n");
    }
  
  
  // write out tstrips if any
  if (pd->GetNumberOfStrips() > 0)
    {
    
    fprintf(fp,"\tIndexedTriangleStripSet {\n");
    fprintf(fp,"\t\tcoordIndex [\n");
    cells = pd->GetStrips();
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
      {
      fprintf(fp,"\t\t\t");
      for (i = 0; i < npts; i++)
	{
	fprintf(fp,"%i, ",indx[i]);
	}
      fprintf(fp,"-1,\n");
      }
    fprintf(fp,"\t\t]\n");
    fprintf(fp,"\t}\n");
    }
  
  fprintf(fp,"}\n"); // close the  Shape
  
  pm->Delete();
}



