/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIVWriter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    to Jon A. Webb for contributing this class.

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
#include <stdio.h>
#include "vtkIVWriter.h"
#include "vtkPolyMapper.h"

void vtkIVWriter::WriteData()
{
  FILE *fp;
  
  // make sure the user specified a filename
  if ( this->Filename == NULL)
    {
    vtkErrorMacro(<< "Please specify filename to use");
    return;
    }

  // try opening the files
  fp = fopen(this->Filename,"w");
  if (!fp)
    {
    vtkErrorMacro(<< "unable to open OpenInventor file: " << this->Filename);
    return;
    }
  
  //
  //  Write header
  //
  vtkDebugMacro("Writing OpenInventor file");
  fprintf(fp,"#Inventor V2.0 ascii\n");
  fprintf(fp,"# OpenInventor file written by the visualization toolkit\n\n");
  this->WritePolyData((vtkPolyData *)this->Input, fp);
  if (fclose(fp)) 
    {
    vtkErrorMacro(<< this->Filename << " did not close successfully. Check disk space.");
    }
}

void vtkIVWriter::WritePolyData(vtkPolyData *pd, FILE *fp)
{
  vtkPointData *pntData;
  vtkPoints *points = NULL;
  vtkNormals *normals = NULL;
  vtkTCoords *tcoords = NULL;
  int i;
  vtkCellArray *cells;
  int npts, *indx;
  vtkPolyMapper *pm;
  vtkColorScalars *colors;
  
  pm = new vtkPolyMapper;
  pm->SetInput(pd);
  
  points = pd->GetPoints();
  pntData = pd->GetPointData();
  normals = pntData->GetNormals();
  tcoords = pntData->GetTCoords();
  colors  = pm->GetColors();
  
  fprintf(fp,"Separator {\n");
  
  // Point data (coordinates)
  fprintf(fp,"\tCoordinate3 {\n");
  fprintf(fp,"\t\tpoint [\n");
  fprintf(fp,"\t\t\t");
  for (i=0; i<points->GetNumberOfPoints(); i++) {
  float xyz[3];
  points->GetPoint(i, xyz);
  fprintf(fp, "%f %f %f, ", xyz[0], xyz[1], xyz[2]);
  if (!((i+1)%2)) fprintf(fp, "\n\t\t\t");
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
    for (i=0; i<colors->GetNumberOfColors(); i++) 
      {
      unsigned char rgba[4];
      colors->GetColor(i,rgba);
      fprintf(fp, "%f %f %f, ", rgba[0]/255.0f, 
	      rgba[1]/255.0f, rgba[2]/255.0f);
      if (!((i+1)%2)) fprintf(fp, "\n\t\t\t");
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



