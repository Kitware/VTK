/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOBJExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOBJExporter.h"

#include "vtkActorCollection.h"
#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkCellArray.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkGeometryFilter.h"
#include "vtkMapper.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkTransform.h"
#include "vtksys/SystemTools.hxx"

vtkStandardNewMacro(vtkOBJExporter);

vtkOBJExporter::vtkOBJExporter()
{
  this->FilePrefix = nullptr;
  this->OBJFileComment = nullptr;
  this->MTLFileComment = nullptr;

  this->SetOBJFileComment("wavefront obj file written by the visualization toolkit");
  this->SetMTLFileComment("wavefront mtl file written by the visualization toolkit");
}

vtkOBJExporter::~vtkOBJExporter()
{
  delete [] this->OBJFileComment;
  delete [] this->MTLFileComment;
  delete [] this->FilePrefix;
}

void vtkOBJExporter::WriteData()
{
  // make sure the user specified a filename
  if ( this->FilePrefix == nullptr)
  {
    vtkErrorMacro(<< "Please specify file prefix to use");
    return;
  }

  vtkRenderer *ren = this->ActiveRenderer;
  if (!ren)
  {
    ren = this->RenderWindow->GetRenderers()->GetFirstRenderer();
  }

  // make sure it has at least one actor
  if (ren->GetActors()->GetNumberOfItems() < 1)
  {
    vtkErrorMacro(<< "no actors found for writing .obj file.");
    return;
  }

  // try opening the files
  std::string objFilePath = std::string(this->FilePrefix) + ".obj";
  FILE *fpObj = fopen(objFilePath.c_str(), "w");
  if (!fpObj)
  {
    vtkErrorMacro(<< "unable to open .obj files ");
    return;
  }
  std::string mtlFilePath = std::string(this->FilePrefix) + ".mtl";
  FILE *fpMtl = fopen(mtlFilePath.c_str(), "w");
  if (!fpMtl)
  {
    fclose(fpObj);
    vtkErrorMacro(<< "unable to open .mtl files ");
    return;
  }

  //
  //  Write header
  //
  vtkDebugMacro("Writing wavefront files");
  if (this->GetOBJFileComment())
  {
    fprintf(fpObj, "# %s\n\n", this->GetOBJFileComment());
  }

  std::string mtlFileName = vtksys::SystemTools::GetFilenameName(mtlFilePath);
  fprintf(fpObj, "mtllib %s\n\n", mtlFileName.c_str());

  if (this->GetMTLFileComment())
  {
    fprintf(fpMtl, "# %s\n\n", this->GetMTLFileComment());
  }


  vtkActorCollection *allActors = ren->GetActors();
  vtkCollectionSimpleIterator actorsIt;
  vtkActor *anActor;
  int idStart = 1;
  for (allActors->InitTraversal(actorsIt); (anActor = allActors->GetNextActor(actorsIt)); )
  {
    vtkAssemblyPath *aPath;
    for (anActor->InitPathTraversal(); (aPath = anActor->GetNextPath()); )
    {
      vtkActor *aPart = vtkActor::SafeDownCast(aPath->GetLastNode()->GetViewProp());
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
  vtkNew<vtkPolyData> pd;
  vtkPointData *pntData;
  vtkPoints *points;
  vtkDataArray *tcoords;
  int i, i1, i2, idNext;
  vtkProperty *prop;
  double *tempd;
  double *p;
  vtkCellArray *cells;
  vtkNew<vtkTransform> trans;
  vtkIdType npts = 0;
  vtkIdType *indx = nullptr;

  // see if the actor has a mapper. it could be an assembly
  if (anActor->GetMapper() == nullptr)
  {
    return;
  }
  // write out the material properties to the mat file
  prop = anActor->GetProperty();
  if (anActor->GetVisibility() == 0)
  {
    return;
  }
  fprintf(fpMtl,"newmtl mtl%i\n",idStart);
  tempd = prop->GetAmbientColor();
  fprintf(fpMtl,"Ka %g %g %g\n",tempd[0], tempd[1], tempd[2]);
  tempd = prop->GetDiffuseColor();
  fprintf(fpMtl,"Kd %g %g %g\n",tempd[0], tempd[1], tempd[2]);
  tempd = prop->GetSpecularColor();
  fprintf(fpMtl,"Ks %g %g %g\n",tempd[0], tempd[1], tempd[2]);
  fprintf(fpMtl,"Ns %g\n",prop->GetSpecularPower());
  fprintf(fpMtl,"Tr %g ", prop->GetOpacity());
  fprintf(fpMtl,"illum 3\n\n");

  // get the mappers input and matrix
  ds = anActor->GetMapper()->GetInput();
  // see if the mapper has an input.
  if (ds == nullptr)
  {
    return;
  }
  anActor->GetMapper()->GetInputAlgorithm()->Update();
  trans->SetMatrix(anActor->vtkProp3D::GetMatrix());

  // we really want polydata
  if ( ds->GetDataObjectType() != VTK_POLY_DATA )
  {
    vtkNew<vtkGeometryFilter> gf;
    gf->SetInputConnection(
      anActor->GetMapper()->GetInputConnection(0, 0));
    gf->Update();
    pd->DeepCopy(gf->GetOutput());
  }
  else
  {
    pd->DeepCopy(ds);
  }

  // write out the points
  points = vtkPoints::New();
  trans->TransformPoints(pd->GetPoints(),points);
  for (i = 0; i < points->GetNumberOfPoints(); i++)
  {
    p = points->GetPoint(i);
    fprintf (fpObj, "v %g %g %g\n", p[0], p[1], p[2]);
  }
  idNext = idStart + static_cast<int>(points->GetNumberOfPoints());
  points->Delete();

  // write out the point data
  pntData = pd->GetPointData();
  if (pntData->GetNormals())
  {
    vtkNew<vtkFloatArray> normals;
    normals->SetNumberOfComponents(3);
    trans->TransformNormals(pntData->GetNormals(),normals);
    for (i = 0; i < normals->GetNumberOfTuples(); i++)
    {
      p = normals->GetTuple(i);
      fprintf (fpObj, "vn %g %g %g\n", p[0], p[1], p[2]);
    }
  }

  tcoords = pntData->GetTCoords();
  if (tcoords)
  {
    for (i = 0; i < tcoords->GetNumberOfTuples(); i++)
    {
      p = tcoords->GetTuple(i);
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
        fprintf(fpObj,"%i ", static_cast<int>(indx[i])+idStart);
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
          fprintf(fpObj,"%i/%i ",static_cast<int>(indx[i])+idStart,
                  static_cast<int>(indx[i]) + idStart);
        }
      }
      else
      {
        for (i = 0; i < npts; i++)
        {
          // treating vtkIdType as int
          fprintf(fpObj,"%i ", static_cast<int>(indx[i])+idStart);
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
        if (pntData->GetNormals())
        {
          if (tcoords)
          {
            // treating vtkIdType as int
            fprintf(fpObj,"%i/%i/%i ", static_cast<int>(indx[i])+idStart,
                    static_cast<int>(indx[i])+ idStart,
                    static_cast<int>(indx[i]) + idStart);
          }
          else
          {
            // treating vtkIdType as int
            fprintf(fpObj,"%i//%i ",static_cast<int>(indx[i])+idStart,
                    static_cast<int>(indx[i]) + idStart);
          }
        }
        else
        {
          if (tcoords)
          {
            // treating vtkIdType as int
            fprintf(fpObj,"%i/%i ", static_cast<int>(indx[i])+idStart,
                    static_cast<int>(indx[i]) + idStart);
          }
          else
          {
            // treating vtkIdType as int
            fprintf(fpObj,"%i ", static_cast<int>(indx[i])+idStart);
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
        if (i%2 == 0)
        {
          i1 = i - 2;
          i2 = i - 1;
        }
        else
        {
          i1 = i - 1;
          i2 = i - 2;
        }
        if (pntData->GetNormals())
        {
          if (tcoords)
          {
            // treating vtkIdType as int
            fprintf(fpObj,"f %i/%i/%i ", static_cast<int>(indx[i1]) + idStart,
                    static_cast<int>(indx[i1]) + idStart, static_cast<int>(indx[i1]) + idStart);
            fprintf(fpObj,"%i/%i/%i ", static_cast<int>(indx[i2])+ idStart,
                    static_cast<int>(indx[i2]) + idStart, static_cast<int>(indx[i2]) + idStart);
            fprintf(fpObj,"%i/%i/%i\n", static_cast<int>(indx[i]) + idStart,
                    static_cast<int>(indx[i]) + idStart, static_cast<int>(indx[i]) + idStart);
          }
          else
          {
            // treating vtkIdType as int
            fprintf(fpObj,"f %i//%i ", static_cast<int>(indx[i1]) + idStart,
                    static_cast<int>(indx[i1]) + idStart);
            fprintf(fpObj,"%i//%i ", static_cast<int>(indx[i2]) + idStart,
                    static_cast<int>(indx[i2]) + idStart);
            fprintf(fpObj,"%i//%i\n",static_cast<int>(indx[i]) + idStart,
                    static_cast<int>(indx[i]) + idStart);
          }
        }
        else
        {
          if (tcoords)
          {
            // treating vtkIdType as int
            fprintf(fpObj,"f %i/%i ", static_cast<int>(indx[i1]) + idStart,
                    static_cast<int>(indx[i1]) + idStart);
            fprintf(fpObj,"%i/%i ", static_cast<int>(indx[i2]) + idStart,
                    static_cast<int>(indx[i2]) + idStart);
            fprintf(fpObj,"%i/%i\n", static_cast<int>(indx[i]) + idStart,
                    static_cast<int>(indx[i]) + idStart);
          }
          else
          {
            // treating vtkIdType as int
            fprintf(fpObj,"f %i %i %i\n", static_cast<int>(indx[i1]) + idStart,
                    static_cast<int>(indx[i2]) + idStart, static_cast<int>(indx[i]) + idStart);
          }
        }
      }
    }
  }

  idStart = idNext;
}



void vtkOBJExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FilePrefix: " << (this->FilePrefix ? this->FilePrefix : "(null)") << "\n";
  os << indent << "OBJFileComment: " << (this->OBJFileComment ? this->OBJFileComment : "(null)") << "\n";
  os << indent << "MTLFileComment: " << (this->MTLFileComment ? this->MTLFileComment : "(null)") << "\n";
}
