/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOBJExporter.cxx
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
#include "vtkOBJExporter.h"
#include "vtkGeometryFilter.h"
#include "vtkAssemblyNode.h"
#include "vtkObjectFactory.h"

//-------------------------------------------------------------------------
vtkOBJExporter* vtkOBJExporter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkOBJExporter");
  if(ret)
    {
    return (vtkOBJExporter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkOBJExporter;
}

vtkOBJExporter::vtkOBJExporter()
{
  this->FilePrefix = NULL;
}

vtkOBJExporter::~vtkOBJExporter()
{
  if ( this->FilePrefix )
    {
    delete [] this->FilePrefix;
    }
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
  if (this->RenderWindow->GetRenderers()->GetNumberOfItems() > 1)
    {
    vtkErrorMacro(<< "obj files only support on renderer per window.");
    return;
    }

  // get the renderer
  this->RenderWindow->GetRenderers()->InitTraversal();
  ren = this->RenderWindow->GetRenderers()->GetNextItem();
  
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
  vtkAssemblyPath *apath;
  for (ac->InitTraversal(); (anActor = ac->GetNextActor()); )
    {
    for (anActor->InitPathTraversal(); (apath=anActor->GetNextPath()); )
      {
      aPart=(vtkActor *)apath->GetLastNode()->GetProp();
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
  vtkPoints *points = NULL;
  vtkNormals *normals = NULL;
  vtkTCoords *tcoords = NULL;
  int i, i1, i2, idNext;
  vtkProperty *prop;
  float *tempf, *p;
  vtkCellArray *cells;
  vtkTransform *trans = vtkTransform::New();
  vtkIdType npts;
  vtkIdType *indx;
  
  // see if the actor has a mapper. it could be an assembly
  if (anActor->GetMapper() == NULL)
    {
    return;
    }

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
  ds->Update();
  trans->SetMatrix(anActor->vtkProp3D::GetMatrix());
    
  // we really want polydata
  if ( ds->GetDataObjectType() != VTK_POLY_DATA )
    {
    gf = vtkGeometryFilter::New();
    gf->SetInput(ds);
    gf->Update();
    pd = gf->GetOutput();
    }
  else
    {
    pd = (vtkPolyData *)ds;
    }

  // write out the points
  points = vtkPoints::New();
  trans->TransformPoints(pd->GetPoints(),points);
  for (i = 0; i < points->GetNumberOfPoints(); i++)
    {
    p = points->GetPoint(i);
    fprintf (fpObj, "v %g %g %g\n", p[0], p[1], p[2]);
    }
  idNext = idStart + (int)(points->GetNumberOfPoints());
  points->Delete();
  
  // write out the point data
  pntData = pd->GetPointData();
  if (pntData->GetNormals())
    {
    normals = vtkNormals::New();
    trans->TransformNormals(pntData->GetNormals(),normals);
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
      fprintf (fpObj, "vt %g %g\n", p[0], p[1]);
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
        // treating vtkIdType as int
	fprintf(fpObj,"%i ", ((int)indx[i])+idStart);
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
          // treating vtkIdType as int
	  fprintf(fpObj,"%i/%i ",((int)indx[i])+idStart,
                  ((int)indx[i]) + idStart);
	  }
	}
      else
	{
	for (i = 0; i < npts; i++)
	  {
          // treating vtkIdType as int
	  fprintf(fpObj,"%i ", ((int)indx[i])+idStart);
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
            // treating vtkIdType as int
	    fprintf(fpObj,"%i/%i/%i ", ((int)indx[i])+idStart, 
		    ((int)indx[i]) + idStart, ((int)indx[i]) + idStart);
	    }
	  else
	    {
            // treating vtkIdType as int
	    fprintf(fpObj,"%i//%i ",((int)indx[i])+idStart,
		    ((int)indx[i]) + idStart);
	    }
	  }
	else
	  {
	  if (tcoords)
	    {
            // treating vtkIdType as int
	    fprintf(fpObj,"%i/%i ", ((int)indx[i])+idStart, 
		    ((int)indx[i]) + idStart);
	    }
	  else
	    {
            // treating vtkIdType as int
	    fprintf(fpObj,"%i ", ((int)indx[i])+idStart);
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
            // treating vtkIdType as int
	    fprintf(fpObj,"f %i/%i/%i ", ((int)indx[i1]) + idStart, 
		    ((int)indx[i1]) + idStart, ((int)indx[i1]) + idStart);
	    fprintf(fpObj,"%i/%i/%i ", ((int)indx[i2])+ idStart, 
		    ((int)indx[i2]) + idStart, ((int)indx[i2]) + idStart);
	    fprintf(fpObj,"%i/%i/%i\n", ((int)indx[i]) + idStart, 
		    ((int)indx[i]) + idStart, ((int)indx[i]) + idStart);
	    }
	  else
	    {
            // treating vtkIdType as int
	    fprintf(fpObj,"f %i//%i ", ((int)indx[i1]) + idStart,
                    ((int)indx[i1]) + idStart);
	    fprintf(fpObj,"%i//%i ", ((int)indx[i2]) + idStart,
                    ((int)indx[i2]) + idStart);
	    fprintf(fpObj,"%i//%i\n",((int)indx[i]) + idStart,
                    ((int)indx[i]) + idStart);
	    }
	  }
	else
	  {
	  if (tcoords)
	    {
            // treating vtkIdType as int
	    fprintf(fpObj,"f %i/%i ", ((int)indx[i1]) + idStart,
                    ((int)indx[i1]) + idStart);
	    fprintf(fpObj,"%i/%i ", ((int)indx[i2]) + idStart,
                    ((int)indx[i2]) + idStart);
	    fprintf(fpObj,"%i/%i\n", ((int)indx[i]) + idStart,
                    ((int)indx[i]) + idStart);
	    }
	  else
	    {
            // treating vtkIdType as int
	    fprintf(fpObj,"f %i %i %i\n", ((int)indx[i1]) + idStart, 
		    ((int)indx[i2]) + idStart, ((int)indx[i]) + idStart);
	    }
	  }
	}
      }
    }
  
  idStart = idNext;
  trans->Delete();
  if (normals)
    {
    normals->Delete();
    }
  if (gf)
    {
    gf->Delete();
    }
}



void vtkOBJExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkExporter::PrintSelf(os,indent);
 
  if (this->FilePrefix)
    {
    os << indent << "FilePrefix: " << this->FilePrefix << "\n";
    }
  else
    {
    os << indent << "FilePrefix: (null)\n";      
    }
}

