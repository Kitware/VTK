// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOOGLExporter.h"

#include "vtkActorCollection.h"
#include "vtkAssemblyPath.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkGeometryFilter.h"
#include "vtkImageData.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkStringFormatter.h"
#include "vtkTexture.h"
#include "vtkUnsignedCharArray.h"

#include <vtksys/SystemTools.hxx>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOOGLExporter);

vtkOOGLExporter::vtkOOGLExporter()
{
  this->FileName = nullptr;
}

vtkOOGLExporter::~vtkOOGLExporter()
{
  this->SetFileName(nullptr);
}

static char indent[256];
static int indent_now = 0;

#define VTK_INDENT_MORE                                                                            \
  do                                                                                               \
  {                                                                                                \
    indent[indent_now] = ' ';                                                                      \
    indent_now += 4;                                                                               \
    indent[indent_now] = 0;                                                                        \
  } while (false)
#define VTK_INDENT_LESS                                                                            \
  do                                                                                               \
  {                                                                                                \
    indent[indent_now] = ' ';                                                                      \
    indent_now -= 4;                                                                               \
    indent[indent_now] = 0;                                                                        \
  } while (false)

void vtkOOGLExporter::WriteData()
{
  FILE* fp;
  int i, j;
  vtkActorCollection* ac;
  vtkActor *anActor, *aPart;
  vtkLightCollection* lc;
  vtkLight* aLight;
  vtkCamera* cam;
  int count;
  vtkMatrix4x4* mat;

  for (i = 0; i < 256; i++)
  {
    indent[i] = ' ';
  }
  indent[indent_now] = 0;

  // make sure the user specified a filename
  if (this->FileName == nullptr)
  {
    vtkErrorMacro(<< "Please specify FileName to use");
    return;
  }

  vtkRenderer* ren = this->ActiveRenderer;
  if (!ren)
  {
    ren = this->RenderWindow->GetRenderers()->GetFirstRenderer();
  }

  // make sure it has at least one actor
  if (ren->GetActors()->GetNumberOfItems() < 1)
  {
    vtkErrorMacro(<< "no actors found for writing Geomview OOGL file.");
    return;
  }

  // try opening the files
  fp = vtksys::SystemTools::Fopen(this->FileName, "w");
  if (!fp)
  {
    vtkErrorMacro(<< "unable to open Geomview OOGL file " << this->FileName);
    return;
  }

  //
  //  Write header
  //
  vtkDebugMacro("Writing Geomview OOGL file");
  vtk::print(fp, "# Geomview OOGL file written by the visualization toolkit\n\n");
  vtk::print(fp, "{:s}( progn\n", indent);

  VTK_INDENT_MORE;

  //
  // Write out the camera
  //
  cam = ren->GetActiveCamera();

  vtk::print(fp, "{:s}(camera \"Camera\" camera {{\n", indent);

  VTK_INDENT_MORE;

  mat = cam->GetViewTransformMatrix();
  vtk::print(fp, "{:s}worldtocam transform {{\n", indent);

  VTK_INDENT_MORE;

  for (i = 0; i < 4; i++)
  {
    vtk::print(fp, "{:s}", indent);
    for (j = 0; j < 4; j++)
    {
      vtk::print(fp, "{:f} ", mat->GetElement(j, i));
    }
    vtk::print(fp, "\n");
  }

  VTK_INDENT_LESS;
  vtk::print(fp, "{:s}}}\n", indent);

  vtk::print(fp, "{:s}perspective {:d} stereo {:d}\n", indent, !cam->GetParallelProjection(), 0);
  vtk::print(fp, "{:s}fov 40\n", indent);
  vtk::print(fp, "{:s}frameaspect 1\n", indent);
  vtk::print(fp, "{:s}focus {:f}\n", indent, cam->GetDistance());
  vtk::print(fp, "{:s}near {:f}\n", indent, cam->GetClippingRange()[0]);
  vtk::print(fp, "{:s}far  {:f}\n", indent, cam->GetClippingRange()[1]);

  VTK_INDENT_LESS;

  vtk::print(fp, "{:s}}}\n", indent);

  VTK_INDENT_LESS;

  vtk::print(fp, "{:s})\n", indent);

  //
  // Write the background colour
  //

  vtk::print(fp, "( backcolor \"Camera\" {:f} {:f} {:f} )\n", ren->GetBackground()[0],
    ren->GetBackground()[1], ren->GetBackground()[2]);

  //
  // Write out default properties
  //

  vtk::print(fp, "( merge-baseap appearance {{\n");

  VTK_INDENT_MORE;

  vtk::print(fp,
    "{:s}face\n{:s}-edge\n{:s}vect\n{:s}-transparent\n{:s}evert\n"
    "{:s}shading flat\n{:s}-normal\n{:s}normscale 1\n{:s}linewidth 1\n"
    "{:s}patchdice 10 10\n",
    indent, indent, indent, indent, indent, indent, indent, indent, indent, indent);
  vtk::print(fp, "{:s}lighting {{\n", indent);

  VTK_INDENT_MORE;

  vtk::print(fp, "{:s}ambient {:f} {:f} {:f}\n", indent, ren->GetAmbient()[0], ren->GetAmbient()[1],
    ren->GetAmbient()[2]);
  vtk::print(fp,
    "{:s}localviewer 1\n{:s}attenconst 1\n{:s}attenmult 0\n"
    "{:s}#replacelights\n",
    indent, indent, indent, indent);

  // make sure we have a default light
  // if we don't then use a headlight
  lc = ren->GetLights();
  vtkCollectionSimpleIterator sit;
  for (lc->InitTraversal(sit); (aLight = lc->GetNextLight(sit));)
  {
    this->WriteALight(aLight, fp);
  }

  VTK_INDENT_LESS;

  vtk::print(fp, "{:s}}}\n", indent);

  VTK_INDENT_LESS;

  vtk::print(fp, "{:s}}})\n", indent);

  // do the actors now
  ac = ren->GetActors();
  vtkAssemblyPath* apath;
  count = 0;
  vtkCollectionSimpleIterator ait;
  for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait));)
  {
    for (anActor->InitPathTraversal(); (apath = anActor->GetNextPath());)
    {
      count++;
      aPart = static_cast<vtkActor*>(apath->GetLastNode()->GetViewProp());
      this->WriteAnActor(aPart, fp, count);
    }
  }

  fclose(fp);
}

void vtkOOGLExporter::WriteALight(vtkLight* aLight, FILE* fp)
{
  double *pos, *focus, *color;
  float dir[3];

  pos = aLight->GetPosition();
  focus = aLight->GetFocalPoint();
  color = aLight->GetDiffuseColor();

  dir[0] = focus[0] - pos[0];
  dir[1] = focus[1] - pos[1];
  dir[2] = focus[2] - pos[2];
  vtkMath::Normalize(dir);

  vtk::print(fp, "{:s}light {{\n", indent);

  VTK_INDENT_MORE;

  vtk::print(fp, "{:s}ambient 0.00 0.00 0.00\n", indent);
  vtk::print(fp, "{:s}color   {:f} {:f} {:f}\n", indent, color[0], color[1], color[2]);
  vtk::print(fp, "{:s}position {:f} {:f} {:f} {:f}\n", indent, pos[0], pos[1], pos[2], 0.0);

  VTK_INDENT_LESS;

  vtk::print(fp, "{:s}}}\n", indent);
}

void vtkOOGLExporter::WriteAnActor(vtkActor* anActor, FILE* fp, int count)
{
  vtkDataSet* ds;
  vtkPolyData* pd;
  vtkGeometryFilter* gf = nullptr;
  vtkPoints* points;
  int i;
  vtkProperty* prop;
  static double defcolor[3] = { 1.0, 1.0, 1.0 };
  double* tempd = defcolor;
  vtkCellArray* cells;
  vtkIdType npts = 0;
  const vtkIdType* indx = nullptr;
  double tempf2 = 0;
  vtkPolyDataMapper* pm;
  vtkUnsignedCharArray* colors;

  double p[3];
  unsigned char* c;

  // see if the actor has a mapper. it could be an assembly
  if (anActor->GetMapper() == nullptr)
  {
    return;
  }

  vtk::print(fp, "{:s}(new-geometry \"[g{:d}]\"\n", indent, count);

  VTK_INDENT_MORE;

  // get the mappers input and matrix
  ds = anActor->GetMapper()->GetInput();

  vtkAlgorithmOutput* pdProducer = nullptr;
  // we really want polydata
  if (ds->GetDataObjectType() != VTK_POLY_DATA)
  {
    gf = vtkGeometryFilter::New();
    gf->SetInputConnection(anActor->GetMapper()->GetInputConnection(0, 0));
    gf->Update();
    pd = gf->GetOutput();
    pdProducer = gf->GetOutputPort();
  }
  else
  {
    anActor->GetMapper()->GetInputAlgorithm()->Update();
    pd = static_cast<vtkPolyData*>(ds);
    pdProducer = anActor->GetMapper()->GetInputConnection(0, 0);
  }

  pm = vtkPolyDataMapper::New();
  pm->SetInputConnection(pdProducer);
  pm->SetScalarRange(anActor->GetMapper()->GetScalarRange());
  pm->SetScalarVisibility(anActor->GetMapper()->GetScalarVisibility());
  pm->SetLookupTable(anActor->GetMapper()->GetLookupTable());

  points = pd->GetPoints();

  // usage of GetColors() has been deprecated in VTK 4.0
  colors = pm->MapScalars(1.0);

  // Get the material properties
  prop = anActor->GetProperty();

  // start an INST object
  vtk::print(fp, "{:s}{{ INST\n", indent);

  VTK_INDENT_MORE;

  // start a LIST object
  vtk::print(fp, "{:s}geom {{ LIST\n", indent);

  VTK_INDENT_MORE;

  // extract vector information
  if (pd->GetNumberOfLines())
  {
    vtk::print(fp, "{:s}{{ VECT\n", indent);

    VTK_INDENT_MORE;

    // write out the header line
    cells = pd->GetLines();
    i = 0;
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      i += npts;
    }
    vtk::print(fp, "{:s}{:d} {:d} {:d}\n", indent, static_cast<int>(pd->GetNumberOfLines()), i, 1);
    cells = pd->GetLines();
    vtk::print(fp, "{:s}", indent);
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      vtk::print(fp, "{:d} ", static_cast<int>(npts));
    }
    vtk::print(fp, "\n");

    // write out # of color information
    vtk::print(fp, "{:s}1 ", indent);
    for (i = 1; i < pd->GetNumberOfLines(); i++)
    {
      vtk::print(fp, "0 ");
    }
    vtk::print(fp, "\n");

    // write out points
    cells = pd->GetLines();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      vtk::print(fp, "{:s}", indent);
      for (i = 0; i < npts; i++)
      {
        double* pt = points->GetPoint(indx[i]);
        vtk::print(fp, "{:s}{:f} {:f} {:f} ", indent, pt[0], pt[1], pt[2]);
      }
      vtk::print(fp, "\n");
    }

    // write out color indices
    tempd = prop->GetColor();
    vtk::print(fp, "{:f} {:f} {:f} 1\n", tempd[0], tempd[1], tempd[2]);
    vtk::print(fp, "}}\n");

    VTK_INDENT_LESS;
  }

  // extract polygon information (includes triangle strips)

  if (pd->GetNumberOfPolys() || pd->GetNumberOfStrips())
  {
    vtk::print(fp, "{:s}{{ {:s}OFF\n", indent, (colors) ? "C" : "");

    VTK_INDENT_MORE;

    // write header
    if (pd->GetNumberOfPolys())
    {
      vtk::print(fp, "{:s}{:d} {:d} {:d}\n", indent, static_cast<int>(points->GetNumberOfPoints()),
        static_cast<int>(pd->GetNumberOfPolys()), 0);
    }
    else
    {
      // Handle triangle strips
      //
      i = 0;
      cells = pd->GetStrips();
      for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
      {
        i += (npts - 2);
      }
      vtk::print(
        fp, "{:s}{:d} {:d} {:d}\n", indent, static_cast<int>(points->GetNumberOfPoints()), i, 0);
    }

    // write points
    if (colors)
    {
      for (i = 0; i < points->GetNumberOfPoints(); i++)
      {
        double* pt = points->GetPoint(i);
        c = static_cast<unsigned char*>(colors->GetPointer(4 * i));

        vtk::print(fp, "{:s}{:g} {:g} {:g} {:g} {:g} {:g} {:g}\n", indent, pt[0], pt[1], pt[2],
          c[0] / 255., c[1] / 255., c[2] / 255., c[3] / 255.);
      }
    }
    else
    {
      for (i = 0; i < points->GetNumberOfPoints(); i++)
      {
        double* pt = points->GetPoint(i);
        vtk::print(fp, "{:s}{:g} {:g} {:g}\n", indent, pt[0], pt[1], pt[2]);
      }
    }

    // write polys
    if (pd->GetNumberOfPolys())
    {
      cells = pd->GetPolys();
      for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
      {
        vtk::print(fp, "{:s}{:d} ", indent, static_cast<int>(npts));
        for (i = 0; i < npts; i++)
        {
          vtk::print(fp, "{:d} ", indx[i]);
        }
        vtk::print(fp, "\n");
      }
      vtk::print(fp, "{:s}}}\n", indent); // finish of polygon list

      VTK_INDENT_LESS;
    }

    else if (pd->GetNumberOfStrips())
    { // write triangle strips
      cells = pd->GetStrips();

      for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
      {
        int pt1, pt2, pt3;

        pt1 = indx[0];
        pt2 = indx[1];

        for (i = 0; i < (npts - 2); i++)
        {
          pt3 = indx[i + 2];
          if (i % 2)
          {
            vtk::print(fp, "{:s}3 {:d} {:d} {:d}\n", indent, pt2, pt1, pt3);
          }
          else
          {
            vtk::print(fp, "{:s}3 {:d} {:d} {:d}\n", indent, pt1, pt2, pt3);
          }
          pt1 = pt2;
          pt2 = pt3;
        }
      }
      vtk::print(fp, "{:s}}}\n", indent); // Finish off triangle strips

      VTK_INDENT_LESS;
    }
  }

  vtk::print(fp, "{:s}}}\n", indent); // End of list object

  VTK_INDENT_LESS;

  // Get the actor's position
  anActor->GetPosition(p);

  vtk::print(fp, "transform {{1 0 0 0 0 1 0 0 0 0 1 0 {:f} {:f} {:f} 1}}\n", p[0], p[1], p[2]);

  VTK_INDENT_LESS;

  vtk::print(fp, "{:s}}}\n", indent); // Finish off INST command

  VTK_INDENT_LESS;

  vtk::print(fp, "{:s})\n", indent); // Finish off new-geometry command

  // turn off the bounding box, set normalization to none
  vtk::print(fp, "( bbox-draw \"[g{:d}]\" off )\n", count);
  vtk::print(fp, "( normalization \"[g{:d}]\" none )\n", count);

  vtk::print(fp, "( merge-ap \"[g{:d}]\" appearance {{\n", count);

  VTK_INDENT_MORE;

  // Set shading model
  if (prop->GetInterpolation() > 0)
  {
    vtk::print(fp, "{:s}shading smooth\n", indent);
  }

  // Set transparency
  if (prop->GetOpacity() < 1)
  {
    vtk::print(fp, "{:s}+transparent\n", indent);
  }

  // Set representation - no way to render points
  if (prop->GetRepresentation() != 2)
  {
    vtk::print(fp, "{:s}+edge\n{:s}-face\n", indent, indent);
  }

  // Set edge information
  vtk::print(fp, "{:s}linewidth {:d}\n", indent, static_cast<int>(prop->GetLineWidth()));

  // Now the material information
  vtk::print(fp, "{:s}material {{\n", indent);

  VTK_INDENT_MORE;

  // Indicate whether edges are shown or not
  if (prop->GetEdgeVisibility())
  {
    tempd = prop->GetEdgeColor();
  }
  if (prop->GetRepresentation() != 2)
  {
    tempd = prop->GetColor();
  }
  if (prop->GetEdgeVisibility() || (prop->GetRepresentation() != 2))
  {
    vtk::print(fp, "{:s}edgecolor {:f} {:f} {:f}\n", indent, tempd[0], tempd[1], tempd[2]);
  }

  tempf2 = prop->GetAmbient();
  tempd = prop->GetAmbientColor();
  vtk::print(fp, "{:s}ka {:f}\n", indent, tempf2);
  vtk::print(fp, "{:s}ambient {:f} {:f} {:f}\n", indent, tempd[0], tempd[1], tempd[2]);
  tempf2 = prop->GetDiffuse();
  tempd = prop->GetDiffuseColor();
  vtk::print(fp, "{:s}kd {:f}\n", indent, tempf2);
  vtk::print(fp, "{:s}diffuse {:f} {:f} {:f}\n", indent, tempd[0], tempd[1], tempd[2]);
  tempf2 = prop->GetSpecular();
  tempd = prop->GetSpecularColor();
  vtk::print(fp, "{:s}ks {:f}\n", indent, tempf2);
  vtk::print(fp, "{:s}specular {:f} {:f} {:f}\n", indent, tempd[0], tempd[1], tempd[2]);
  if (prop->GetOpacity() < 1)
  {
    vtk::print(fp, "{:s}alpha {:f}\n", indent, prop->GetOpacity());
  }

  vtk::print(fp, "{:s}}}\n", indent);

  VTK_INDENT_LESS;

  vtk::print(fp, "{:s}}}\n", indent);

  VTK_INDENT_LESS;

  vtk::print(fp, ")\n");

  if (gf)
  {
    gf->Delete();
  }
  pm->Delete();
}

void vtkOOGLExporter::PrintSelf(ostream& os, vtkIndent ind)
{
  vtkExporter::PrintSelf(os, ind);

  if (this->FileName)
  {
    os << ind << "FileName: " << this->FileName << "\n";
  }
  else
  {
    os << ind << "FileName: (null)\n";
  }
}
VTK_ABI_NAMESPACE_END
