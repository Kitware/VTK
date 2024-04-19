// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2007, Los Alamos National Security, LLC
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-LANL-USGov
#include "vtkPOVExporter.h"

#include "vtkAssemblyPath.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkCompositeDataSet.h"
#include "vtkFloatArray.h"
#include "vtkGeometryFilter.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMapper.h"
#include "vtkMatrix4x4.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkSmartPointer.h"
#include "vtkTexture.h"
#include "vtkTypeTraits.h"
#include "vtkUnsignedCharArray.h"
#include <vtksys/SystemTools.hxx>

#include <sstream>

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPOVExporter);

// Can't use printf("%d", a_vtkIdType) because vtkIdType is not always int.
// This internal class holds format strings vtkPOVExporter can use instead.
class vtkPOVInternals
{
public:
  vtkPOVInternals()
  {
    strcpy(this->CountFormat, "\t\t");
    strcat(this->CountFormat, vtkTypeTraits<vtkIdType>::ParseFormat());
    strcat(this->CountFormat, ",\n");

    char triFormat[100]; //"%d, %d, %d"
    strcpy(triFormat, vtkTypeTraits<vtkIdType>::ParseFormat());
    strcat(triFormat, ", ");
    strcat(triFormat, vtkTypeTraits<vtkIdType>::ParseFormat());
    strcat(triFormat, ", ");
    strcat(triFormat, vtkTypeTraits<vtkIdType>::ParseFormat());

    strcpy(this->TriangleFormat1, "\t\t<");
    strcat(this->TriangleFormat1, triFormat);
    strcat(this->TriangleFormat1, ">,");

    strcpy(this->TriangleFormat2, " ");
    strcat(this->TriangleFormat2, triFormat);
    strcat(this->TriangleFormat2, ",\n");
  }

  ~vtkPOVInternals() = default;

  char CountFormat[100];
  char TriangleFormat1[100];
  char TriangleFormat2[100];
};

#define VTKPOV_CNTFMT this->Internals->CountFormat
#define VTKPOV_TRIFMT1 this->Internals->TriangleFormat1
#define VTKPOV_TRIFMT2 this->Internals->TriangleFormat2

//============================================================================
vtkPOVExporter::vtkPOVExporter()
{
  this->FileName = nullptr;
  this->FilePtr = nullptr;
  this->Internals = new vtkPOVInternals;
}

vtkPOVExporter::~vtkPOVExporter()
{
  delete[] this->FileName;
  delete this->Internals;
}

void vtkPOVExporter::WriteData()
{
  // make sure user specified a filename
  if (this->FileName == nullptr)
  {
    vtkErrorMacro(<< "Please specify file name to create");
    return;
  }

  // get the renderer
  vtkRenderer* renderer = this->ActiveRenderer;
  if (!renderer)
  {
    renderer = this->RenderWindow->GetRenderers()->GetFirstRenderer();
  }

  // make sure it has at least one actor
  if (renderer->GetActors()->GetNumberOfItems() < 1)
  {
    vtkErrorMacro(<< "no actors found for writing .pov file.");
    return;
  }

  // try opening the file
  this->FilePtr = vtksys::SystemTools::Fopen(this->FileName, "w");
  if (this->FilePtr == nullptr)
  {
    vtkErrorMacro(<< "Cannot open " << this->FileName);
    return;
  }

  // write header
  this->WriteHeader(renderer);

  // write camera
  this->WriteCamera(renderer->GetActiveCamera());

  // write lights
  vtkLightCollection* lc = renderer->GetLights();
  vtkCollectionSimpleIterator sit;
  lc->InitTraversal(sit);
  if (lc->GetNextLight(sit) == nullptr)
  {
    vtkWarningMacro(<< "No light defined, creating one at camera position");
    renderer->CreateLight();
  }
  vtkLight* light;
  for (lc->InitTraversal(sit); (light = lc->GetNextLight(sit));)
  {
    if (light->GetSwitch())
    {
      this->WriteLight(light);
    }
  }

  // write actors
  vtkActorCollection* ac = renderer->GetActors();
  vtkAssemblyPath* apath;
  vtkCollectionSimpleIterator ait;
  vtkActor *anActor, *aPart;
  for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait));)
  {
    for (anActor->InitPathTraversal(); (apath = anActor->GetNextPath());)
    {
      aPart = static_cast<vtkActor*>(apath->GetLastNode()->GetViewProp());
      this->WriteActor(aPart);
    }
  }

  fclose(this->FilePtr);
}

void vtkPOVExporter::WriteHeader(vtkRenderer* renderer)
{
  fprintf(this->FilePtr, "// POVRay file exported by vtkPOVExporter\n");
  fprintf(this->FilePtr, "//\n");

  // width and height of output image,
  // and other default command line args to POVRay
  const int* size = renderer->GetSize();
  fprintf(this->FilePtr, "// +W%d +H%d\n\n", size[0], size[1]);

  // global settings
  fprintf(this->FilePtr, "global_settings {\n");
  fprintf(this->FilePtr, "\tambient_light color rgb <1.0, 1.0, 1.0>\n");
  fprintf(this->FilePtr, "\tassumed_gamma 2\n");
  fprintf(this->FilePtr, "}\n\n");

  // background
  double* color = renderer->GetBackground();
  fprintf(this->FilePtr, "background { color rgb <%f, %f, %f>}\n\n", color[0], color[1], color[2]);
}

void vtkPOVExporter::WriteCamera(vtkCamera* camera)
{
  fprintf(this->FilePtr, "camera {\n");
  if (camera->GetParallelProjection())
  {
    fprintf(this->FilePtr, "\torthographic\n");
  }
  else
  {
    fprintf(this->FilePtr, "\tperspective\n");
  }

  double* position = camera->GetPosition();
  fprintf(this->FilePtr, "\tlocation <%f, %f, %f>\n", position[0], position[1], position[2]);

  double* up = camera->GetViewUp();
  // the camera up vector is called "sky" in POVRay
  fprintf(this->FilePtr, "\tsky <%f, %f, %f>\n", up[0], up[1], up[2]);

  // make POVRay to use left handed system to right handed
  // TODO: aspect ratio
  fprintf(this->FilePtr, "\tright <-1, 0, 0>\n");
  // fprintf(this->FilePtr, "\tup <-1, 0, 0>\n");

  fprintf(this->FilePtr, "\tangle %f\n", camera->GetViewAngle());

  double* focal = camera->GetFocalPoint();
  fprintf(this->FilePtr, "\tlook_at <%f, %f, %f>\n", focal[0], focal[1], focal[2]);

  fprintf(this->FilePtr, "}\n\n");
}

void vtkPOVExporter::WriteLight(vtkLight* light)
{
  fprintf(this->FilePtr, "light_source {\n");

  double* position = light->GetPosition();
  fprintf(this->FilePtr, "\t<%f, %f, %f>\n", position[0], position[1], position[2]);

  double* color = light->GetDiffuseColor();
  fprintf(this->FilePtr, "\tcolor <%f, %f, %f>*%f\n", color[0], color[1], color[2],
    light->GetIntensity());

  if (light->GetPositional())
  {
    fprintf(this->FilePtr, "\tspotlight\n");
    fprintf(this->FilePtr, "\tradius %f\n", light->GetConeAngle());
    fprintf(this->FilePtr, "\tfalloff %f\n", light->GetExponent());
  }
  else
  {
    fprintf(this->FilePtr, "\tparallel\n");
  }
  double* focal = light->GetFocalPoint();
  fprintf(this->FilePtr, "\tpoint_at <%f, %f, %f>\n", focal[0], focal[1], focal[2]);

  fprintf(this->FilePtr, "}\n\n");
}

void vtkPOVExporter::WriteActor(vtkActor* actor)
{
  if (actor->GetMapper() == nullptr)
  {
    return;
  }
  if (actor->GetVisibility() == 0)
  {
    return;
  }

  // write geometry, first ask the pipeline to update data
  vtkDataSet* dataset = nullptr;
  vtkSmartPointer<vtkDataSet> tempDS;

  vtkDataObject* dObj = actor->GetMapper()->GetInputDataObject(0, 0);
  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(dObj);
  if (cd)
  {
    vtkCompositeDataGeometryFilter* gf = vtkCompositeDataGeometryFilter::New();
    gf->SetInputConnection(actor->GetMapper()->GetInputConnection(0, 0));
    gf->Update();
    tempDS = gf->GetOutput();
    gf->Delete();

    dataset = tempDS;
  }
  else
  {
    dataset = actor->GetMapper()->GetInput();
  }

  if (dataset == nullptr)
  {
    return;
  }
  actor->GetMapper()->GetInputAlgorithm()->Update();

  // convert non polygon data to polygon data if needed
  vtkGeometryFilter* geometryFilter = nullptr;
  vtkPolyData* polys = nullptr;
  if (dataset->GetDataObjectType() != VTK_POLY_DATA)
  {
    geometryFilter = vtkGeometryFilter::New();
    geometryFilter->SetInputConnection(actor->GetMapper()->GetInputConnection(0, 0));
    geometryFilter->Update();
    polys = geometryFilter->GetOutput();
  }
  else
  {
    polys = static_cast<vtkPolyData*>(dataset);
  }

  // we only export Polygons and Triangle Strips
  if ((polys->GetNumberOfPolys() == 0) && (polys->GetNumberOfStrips() == 0))
  {
    return;
  }

  // write point coordinates
  vtkPoints* points = polys->GetPoints();

  // we use mesh2 since it maps better to how VTK stores
  // polygons/triangle strips
  fprintf(this->FilePtr, "mesh2 {\n");

  fprintf(this->FilePtr, "\tvertex_vectors {\n");
  fprintf(this->FilePtr, VTKPOV_CNTFMT, points->GetNumberOfPoints());
  for (vtkIdType i = 0; i < points->GetNumberOfPoints(); i++)
  {
    double* pos = points->GetPoint(i);
    fprintf(this->FilePtr, "\t\t<%f, %f, %f>,\n", pos[0], pos[1], pos[2]);
  }
  fprintf(this->FilePtr, "\t}\n");

  // write vertex normal
  vtkPointData* pointData = polys->GetPointData();
  if (pointData->GetNormals())
  {
    vtkDataArray* normals = pointData->GetNormals();
    fprintf(this->FilePtr, "\tnormal_vectors {\n");
    fprintf(this->FilePtr, VTKPOV_CNTFMT, normals->GetNumberOfTuples());
    for (vtkIdType i = 0; i < normals->GetNumberOfTuples(); i++)
    {
      double* normal = normals->GetTuple(i);
      fprintf(this->FilePtr, "\t\t<%f, %f, %f>,\n", normal[0], normal[1], normal[2]);
    }
    fprintf(this->FilePtr, "\t}\n");
  }

  // TODO: write texture coordinates (uv vectors)

  // write vertex texture, ask mapper to generate color for each vertex if
  // the scalar data visibility is on
  bool scalar_visible = false;
  if (actor->GetMapper()->GetScalarVisibility())
  {
    vtkUnsignedCharArray* color_array = actor->GetMapper()->MapScalars(1.0);
    if (color_array != nullptr)
    {
      scalar_visible = true;
      fprintf(this->FilePtr, "\ttexture_list {\n");
      fprintf(this->FilePtr, VTKPOV_CNTFMT, color_array->GetNumberOfTuples());
      for (vtkIdType i = 0; i < color_array->GetNumberOfTuples(); i++)
      {
        unsigned char* color = color_array->GetPointer(4 * i);
        fprintf(this->FilePtr, "\t\ttexture { pigment {color rgbf <%f, %f, %f, %f> } },\n",
          color[0] / 255.0, color[1] / 255.0, color[2] / 255.0, 1.0 - color[3] / 255.0);
      }
      fprintf(this->FilePtr, "\t}\n");
    }
  }

  // write polygons
  if (polys->GetNumberOfPolys() > 0)
  {
    this->WritePolygons(polys, scalar_visible);
  }

  // write triangle strips
  if (polys->GetNumberOfStrips() > 0)
  {
    this->WriteTriangleStrips(polys, scalar_visible);
  }

  // write transformation for the actor, it is column major and looks like transposed
  vtkMatrix4x4* matrix = actor->GetMatrix();
  fprintf(this->FilePtr, "\tmatrix < %f, %f, %f,\n", matrix->GetElement(0, 0),
    matrix->GetElement(1, 0), matrix->GetElement(2, 0));
  fprintf(this->FilePtr, "\t\t %f, %f, %f,\n", matrix->GetElement(0, 1), matrix->GetElement(1, 1),
    matrix->GetElement(2, 1));
  fprintf(this->FilePtr, "\t\t %f, %f, %f,\n", matrix->GetElement(0, 2), matrix->GetElement(1, 2),
    matrix->GetElement(2, 2));
  fprintf(this->FilePtr, "\t\t %f, %f, %f >\n", matrix->GetElement(0, 3), matrix->GetElement(1, 3),
    matrix->GetElement(2, 3));

  // write property
  this->WriteProperty(actor->GetProperty());

  // done with this actor
  fprintf(this->FilePtr, "}\n\n");

  if (geometryFilter)
  {
    geometryFilter->Delete();
  }
}

void vtkPOVExporter::WritePolygons(vtkPolyData* polys, bool scalar_visible)
{
  // write polygons with on the fly triangulation,
  // assuming polygon are simple and can be triangulated into "fans"
  vtkIdType numtriangles = 0;
  vtkCellArray* cells = polys->GetPolys();
  vtkIdType npts = 0;
  const vtkIdType* pts = nullptr;

  // first pass,
  // calculate how many triangles there will be after triangulation
  for (cells->InitTraversal(); cells->GetNextCell(npts, pts);)
  {
    // the number of triangles for each polygon will be # of vertex - 2
    numtriangles += (npts - 2);
  }

  // second pass, triangulate and write face indices
  fprintf(this->FilePtr, "\tface_indices {\n");
  fprintf(this->FilePtr, VTKPOV_CNTFMT, numtriangles);
  for (cells->InitTraversal(); cells->GetNextCell(npts, pts);)
  {
    vtkIdType triangle[3];
    // the first triangle
    triangle[0] = pts[0];
    triangle[1] = pts[1];
    triangle[2] = pts[2];

    fprintf(this->FilePtr, VTKPOV_TRIFMT1, triangle[0], triangle[1], triangle[2]);
    if (scalar_visible)
    {
      fprintf(this->FilePtr, VTKPOV_TRIFMT2, triangle[0], triangle[1], triangle[2]);
    }
    else
    {
      fprintf(this->FilePtr, "\n");
    }

    // the rest of triangles
    for (vtkIdType i = 3; i < npts; i++)
    {
      triangle[1] = triangle[2];
      triangle[2] = pts[i];
      fprintf(this->FilePtr, VTKPOV_TRIFMT1, triangle[0], triangle[1], triangle[2]);
      if (scalar_visible)
      {
        fprintf(this->FilePtr, VTKPOV_TRIFMT2, triangle[0], triangle[1], triangle[2]);
      }
      else
      {
        fprintf(this->FilePtr, "\n");
      }
    }
  }
  fprintf(this->FilePtr, "\t}\n");

  // third pass, the same thing as 2nd pass but for normal_indices
  if (polys->GetPointData()->GetNormals())
  {
    fprintf(this->FilePtr, "\tnormal_indices {\n");
    fprintf(this->FilePtr, VTKPOV_CNTFMT, numtriangles);
    for (cells->InitTraversal(); cells->GetNextCell(npts, pts);)
    {
      vtkIdType triangle[3];
      // the first triangle
      triangle[0] = pts[0];
      triangle[1] = pts[1];
      triangle[2] = pts[2];

      fprintf(this->FilePtr, VTKPOV_TRIFMT1, triangle[0], triangle[1], triangle[2]);
      fprintf(this->FilePtr, "\n");

      // the rest of triangles
      for (vtkIdType i = 3; i < npts; i++)
      {
        triangle[1] = triangle[2];
        triangle[2] = pts[i];
        fprintf(this->FilePtr, VTKPOV_TRIFMT1, triangle[0], triangle[1], triangle[2]);
        fprintf(this->FilePtr, "\n");
      }
    }
    fprintf(this->FilePtr, "\t}\n");
  }

  // TODO: 4th pass, texture indices
}

void vtkPOVExporter::WriteTriangleStrips(vtkPolyData* polys, bool scalar_visible)
{
  // convert triangle strips into triangles
  vtkIdType numtriangles = 0;
  vtkCellArray* cells = polys->GetStrips();
  vtkIdType npts = 0;
  const vtkIdType* pts = nullptr;

  // first pass, calculate how many triangles there will be after conversion
  for (cells->InitTraversal(); cells->GetNextCell(npts, pts);)
  {
    // the number of triangles for each polygon will be # of vertex - 2
    numtriangles += (npts - 2);
  }

  // second pass, convert to triangles and write face indices
  fprintf(this->FilePtr, "\tface_indices {\n");
  fprintf(this->FilePtr, VTKPOV_CNTFMT, numtriangles);
  for (cells->InitTraversal(); cells->GetNextCell(npts, pts);)
  {
    vtkIdType triangle[3];
    // the first triangle
    triangle[0] = pts[0];
    triangle[1] = pts[1];
    triangle[2] = pts[2];

    fprintf(this->FilePtr, VTKPOV_TRIFMT1, triangle[0], triangle[1], triangle[2]);
    if (scalar_visible)
    {
      fprintf(this->FilePtr, VTKPOV_TRIFMT2, triangle[0], triangle[1], triangle[2]);
    }
    else
    {
      fprintf(this->FilePtr, "\n");
    }

    // the rest of triangles
    for (vtkIdType i = 3; i < npts; i++)
    {
      triangle[0] = triangle[1];
      triangle[1] = triangle[2];
      triangle[2] = pts[i];
      fprintf(this->FilePtr, VTKPOV_TRIFMT1, triangle[0], triangle[1], triangle[2]);
      if (scalar_visible)
      {
        fprintf(this->FilePtr, VTKPOV_TRIFMT2, triangle[0], triangle[1], triangle[2]);
      }
      else
      {
        fprintf(this->FilePtr, "\n");
      }
    }
  }
  fprintf(this->FilePtr, "\t}\n");

  // third pass, the same thing as 2nd pass but for normal_indices
  if (polys->GetPointData()->GetNormals())
  {
    fprintf(this->FilePtr, "\tnormal_indices {\n");
    fprintf(this->FilePtr, VTKPOV_CNTFMT, numtriangles);
    for (cells->InitTraversal(); cells->GetNextCell(npts, pts);)
    {
      vtkIdType triangle[3];
      // the first triangle
      triangle[0] = pts[0];
      triangle[1] = pts[1];
      triangle[2] = pts[2];

      fprintf(this->FilePtr, VTKPOV_TRIFMT1, triangle[0], triangle[1], triangle[2]);
      fprintf(this->FilePtr, "\n");

      // the rest of triangles
      for (vtkIdType i = 3; i < npts; i++)
      {
        triangle[0] = triangle[1];
        triangle[1] = triangle[2];
        triangle[2] = pts[i];
        fprintf(this->FilePtr, VTKPOV_TRIFMT1, triangle[0], triangle[1], triangle[2]);
        fprintf(this->FilePtr, "\n");
      }
    }
    fprintf(this->FilePtr, "\t}\n");
  }

  // TODO: 4th pass, texture indices
}

void vtkPOVExporter::WriteProperty(vtkProperty* property)
{
  fprintf(this->FilePtr, "\ttexture {\n");

  /* write color */
  fprintf(this->FilePtr, "\t\tpigment {\n");
  double* color = property->GetColor();
  fprintf(this->FilePtr, "\t\t\tcolor rgbf <%f, %f, %f %f>\n", color[0], color[1], color[2],
    1.0 - property->GetOpacity());
  fprintf(this->FilePtr, "\t\t}\n");

  /* write ambient, diffuse and specular coefficients */
  fprintf(this->FilePtr, "\t\tfinish {\n\t\t\t");
  fprintf(this->FilePtr, "ambient %f  ", property->GetAmbient());
  fprintf(this->FilePtr, "diffuse %f  ", property->GetDiffuse());
  fprintf(this->FilePtr, "phong %f  ", property->GetSpecular());
  fprintf(this->FilePtr, "phong_size %f  ", property->GetSpecularPower());
  fprintf(this->FilePtr, "\n\t\t}\n");

  fprintf(this->FilePtr, "\t}\n");
}

void vtkPOVExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->FileName)
  {
    os << indent << "FileName: " << this->FileName << "\n";
  }
  else
  {
    os << indent << "FileName: (null)\n";
  }
}
VTK_ABI_NAMESPACE_END
