/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOBJExporter.cxx
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
#include "vtkOBJExporter.h"
#include "vtkGeometryFilter.h"

vtkOBJExporter::vtkOBJExporter()
{
  this->FilePrefix = NULL;
}

vtkOBJExporter::~vtkOBJExporter()
{
  if ( this->FilePrefix ) delete [] this->FilePrefix;
}

void vtkOBJExporter::WriteData()
{
  vtkRenderer *ren;
  FILE *fpObj, *fpMtl;
  vtkActorCollection *ac;
  vtkActor *anActor, *aPart;
  char nameObj[80];
  char nameMtl[80];
  int idStart = 1;
  
  // make sure the user specified a filename
  if ( this->FilePrefix == NULL)
    {
    vtkErrorMacro(<< "Please specify file prefix to use");
    return;
    }

  // first make sure there is only one renderer in this rendering window
  if (this->Input->GetRenderers()->GetNumberOfItems() > 1)
    {
    vtkErrorMacro(<< "obj files only support on renderer per window.");
    return;
    }

  // get the renderer
  this->Input->GetRenderers()->InitTraversal();
  ren = this->Input->GetRenderers()->GetNextItem();
  
  // make sure it has at least one actor
  if (ren->GetActors()->GetNumberOfItems() < 1)
    {
    vtkErrorMacro(<< "no actors found for writing .obj file.");
    return;
    }
    
  // try opening the files
  sprintf(nameObj,"%s.obj",this->FilePrefix);
  sprintf(nameMtl,"%s.mtl",this->FilePrefix);
  fpObj = fopen(nameObj,"w");
  fpMtl = fopen(nameMtl,"w");
  if (!fpObj || !fpMtl)
    {
    vtkErrorMacro(<< "unable to open .obj and .mtl files ");
    return;
    }
  
  //
  //  Write header
  //
  vtkDebugMacro("Writing wavefront files");
  fprintf(fpObj, 
	  "# wavefront obj file written by the visualization toolkit\n\n");
  fprintf(fpObj, "mtllib %s\n\n", nameMtl);
  fprintf(fpMtl, 
	  "# wavefront mtl file written by the visualization toolkit\n\n");
  
  ac = ren->GetActors();
  for (ac->InitTraversal(); (anActor = ac->GetNextItem()); )
    {
    for (anActor->InitPartTraversal();(aPart=anActor->GetNextPart()); )
      {
      this->WriteAnActor(aPart, fpObj, fpMtl, idStart);
      }
    }
  
  fclose(fpObj);
  fclose(fpMtl);
}

void vtkOBJExporter::WriteAnActor(vtkActor *anActor, FILE *fpObj, FILE *fpMtl,
				  int &idStart)
{
  vtkDataSet *ds;
  vtkPolyData *pd;
  vtkGeometryFilter *gf = NULL;
  vtkPointData *pntData;
  vtkFloatPoints *points = NULL;
  vtkFloatNormals *normals = NULL;
  vtkTCoords *tcoords = NULL;
  int i, i1, i2, idNext;
  vtkProperty *prop;
  float *tempf, *p;
  vtkCellArray *cells;
  vtkTransform *trans = new vtkTransform;
  int npts, *indx;
  
  // write out the material properties to the mat file
  prop = anActor->GetProperty();
  fprintf(fpMtl,"newmtl mtl%i\n",idStart);
  tempf = prop->GetAmbientColor();
  fprintf(fpMtl,"Ka %g %g %g\n",tempf[0], tempf[1], tempf[2]);
  tempf = prop->GetDiffuseColor();
  fprintf(fpMtl,"Kd %g %g %g\n",tempf[0], tempf[1], tempf[2]);
  tempf = prop->GetSpecularColor();
  fprintf(fpMtl,"Ks %g %g %g\n",tempf[0], tempf[1], tempf[2]);
  fprintf(fpMtl,"Ns %g\n",prop->GetSpecularPower());
  fprintf(fpMtl,"Tf %g %g %g\n",1.0 - prop->GetOpacity(),
	  1.0 - prop->GetOpacity(),1.0 - prop->GetOpacity());
  fprintf(fpMtl,"illum 3\n\n");

  // get the mappers input and matrix
  ds = anActor->GetMapper()->GetInput();
  trans->SetMatrix(anActor->GetMatrix());
    
  // we really want polydata
  if (strcmp(ds->GetClassName(),"vtkPolyData"))
    {
    gf = new vtkGeometryFilter;
    gf->SetInput(ds);
    gf->Update();
    pd = gf->GetOutput();
    }
  else
    {
    pd = (vtkPolyData *)ds;
    }

  // write out the points
  points = new vtkFloatPoints;
  trans->MultiplyPoints(pd->GetPoints(),points);
  for (i = 0; i < points->GetNumberOfPoints(); i++)
    {
    p = points->GetPoint(i);
    fprintf (fpObj, "v %g %g %g\n", p[0], p[1], p[2]);
    }
  idNext = idStart + points->GetNumberOfPoints();
  points->Delete();
  
  // write out the point data
  pntData = pd->GetPointData();
  if (pntData->GetNormals())
    {
    normals = new vtkFloatNormals;
    trans->MultiplyNormals(pntData->GetNormals(),normals);
    for (i = 0; i < normals->GetNumberOfNormals(); i++)
      {
      p = normals->GetNormal(i);
      fprintf (fpObj, "vn %g %g %g\n", p[0], p[1], p[2]);
      }
    }
  
  tcoords = pntData->GetTCoords();
  if (tcoords)
    {
    for (i = 0; i < tcoords->GetNumberOfTCoords(); i++)
      {
      p = tcoords->GetTCoord(i);
      fprintf (fpObj, "vt %g %g %g\n", p[0], p[1], p[2]);
      }
    }
  
  // write out a group name and material
  fprintf (fpObj, "\ng grp%i\n", idStart);
  fprintf (fpObj, "usemtl mtl%i\n", idStart);
  
  // write out verts if any
  if (pd->GetNumberOfVerts() > 0)
    {
    cells = pd->GetVerts();
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
      {
      fprintf(fpObj,"p ");
      for (i = 0; i < npts; i++)
	{
	fprintf(fpObj,"%i ",indx[i]+idStart);
	}
      fprintf(fpObj,"\n");
      }
    }

  // write out lines if any
  if (pd->GetNumberOfLines() > 0)
    {
    cells = pd->GetLines();
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
      {
      fprintf(fpObj,"l ");
      if (tcoords)
	{
	for (i = 0; i < npts; i++)
	  {
	  fprintf(fpObj,"%i/%i ",indx[i]+idStart, indx[i] + idStart);
	  }
	}
      else
	{
	for (i = 0; i < npts; i++)
	  {
	  fprintf(fpObj,"%i ",indx[i]+idStart);
	  }
	}
      fprintf(fpObj,"\n");
      }
    }

  // write out polys if any
  if (pd->GetNumberOfPolys() > 0)
    {
    cells = pd->GetPolys();
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
      {
      fprintf(fpObj,"f ");
      for (i = 0; i < npts; i++)
	{
	if (normals)
	  {
	  if (tcoords)
	    {
	    fprintf(fpObj,"%i/%i/%i ",indx[i]+idStart, 
		    indx[i] + idStart, indx[i] + idStart);
	    }
	  else
	    {
	    fprintf(fpObj,"%i//%i ",indx[i]+idStart,
		    indx[i] + idStart);
	    }
	  }
	else
	  {
	  if (tcoords)
	    {
	    fprintf(fpObj,"%i/%i ",indx[i]+idStart, 
		    indx[i] + idStart);
	    }
	  else
	    {
	    fprintf(fpObj,"%i ",indx[i]+idStart);
	    }
	  }
	}
      fprintf(fpObj,"\n");
      }
    }

  // write out tstrips if any
  if (pd->GetNumberOfStrips() > 0)
    {
    cells = pd->GetStrips();
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
      {
      for (i = 2; i < npts; i++)
	{
	if (i%2)
	  {
	  i1 = i - 1;
	  i2 = i - 2;
	  }
	else
	  {
	  i1 = i - 1;
	  i2 = i - 2;
	  }
	if (normals)
	  {
	  if (tcoords)
	    {
	    fprintf(fpObj,"f %i/%i/%i ",indx[i1] + idStart, 
		    indx[i1] + idStart, indx[i1] + idStart);
	    fprintf(fpObj,"%i/%i/%i ",indx[i2]+ idStart, 
		    indx[i2] + idStart, indx[i2] + idStart);
	    fprintf(fpObj,"%i/%i/%i\n",indx[i]+ idStart, 
		    indx[i] + idStart, indx[i] + idStart);
	    }
	  else
	    {
	    fprintf(fpObj,"f %i//%i ",indx[i1] + idStart, indx[i1] + idStart);
	    fprintf(fpObj,"%i//%i ",indx[i2]+ idStart, indx[i2] + idStart);
	    fprintf(fpObj,"%i//%i\n",indx[i]+ idStart, indx[i] + idStart);
	    }
	  }
	else
	  {
	  if (tcoords)
	    {
	    fprintf(fpObj,"f %i/%i ",indx[i1] + idStart, indx[i1] + idStart);
	    fprintf(fpObj,"%i/%i ",indx[i2]+ idStart, indx[i2] + idStart);
	    fprintf(fpObj,"%i/%i\n",indx[i]+ idStart, indx[i] + idStart);
	    }
	  else
	    {
	    fprintf(fpObj,"f %i %i %i\n",indx[i1] + idStart, 
		    indx[i2] + idStart, indx[i] + idStart);
	    }
	  }
	}
      }
    }
  
  idStart = idNext;
  trans->Delete();
  if (normals) normals->Delete();
  if (gf) gf->Delete();
}



void vtkOBJExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkExporter::PrintSelf(os,indent);
 
  os << indent << "FilePrefix: " << this->FilePrefix << "\n";
}

