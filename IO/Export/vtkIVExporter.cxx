// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkIVExporter.h"

#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkGeometryFilter.h"
#include "vtkImageData.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkStringFormatter.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkUnsignedCharArray.h"

#include <vtksys/SystemTools.hxx>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkIVExporter);

vtkIVExporter::vtkIVExporter()
{
  this->FileName = nullptr;
}

vtkIVExporter::~vtkIVExporter()
{
  delete[] this->FileName;
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

void vtkIVExporter::WriteData()
{
  FILE* fp;
  vtkActorCollection* ac;
  vtkActor *anActor, *aPart;
  vtkLightCollection* lc;
  vtkLight* aLight;
  vtkCamera* cam;
  double* tempd;

  for (int i = 0; i < 256; i++)
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

  // get the renderer
  vtkRenderer* ren = this->ActiveRenderer;
  if (!ren)
  {
    ren = this->RenderWindow->GetRenderers()->GetFirstRenderer();
  }

  // make sure it has at least one actor
  if (ren->GetActors()->GetNumberOfItems() < 1)
  {
    vtkErrorMacro(<< "no actors found for writing OpenInventor file.");
    return;
  }

  // try opening the files
  fp = vtksys::SystemTools::Fopen(this->FileName, "w");
  if (!fp)
  {
    vtkErrorMacro(<< "unable to open OpenInventor file " << this->FileName);
    return;
  }

  //
  //  Write header
  //
  vtkDebugMacro("Writing OpenInventor file");
  vtk::print(fp, "#Inventor V2.0 ascii\n");
  vtk::print(fp, "# OpenInventor file written by the visualization toolkit\n\n");

  vtk::print(fp, "Separator {{\n");
  VTK_INDENT_MORE;

  // do the camera
  cam = ren->GetActiveCamera();
  if (cam->GetParallelProjection())
  {
    vtk::print(fp, "{:s}OrthographicCamera\n{:s}{{\n", indent, indent);
  }
  else
  {
    // this assumes the aspect ratio is 1
    vtk::print(fp, "{:s}PerspectiveCamera\n{:s}{{\n{:s}    heightAngle {:f}\n", indent, indent,
      indent, cam->GetViewAngle() * vtkMath::Pi() / 180.0);
  }
  VTK_INDENT_MORE;
  vtk::print(fp, "{:s}nearDistance {:f}\n", indent, cam->GetClippingRange()[0]);
  vtk::print(fp, "{:s}farDistance {:f}\n", indent, cam->GetClippingRange()[1]);
  vtk::print(fp, "{:s}focalDistance {:f}\n", indent, cam->GetDistance());
  vtk::print(fp, "{:s}position {:f} {:f} {:f}\n", indent, cam->GetPosition()[0],
    cam->GetPosition()[1], cam->GetPosition()[2]);
  tempd = cam->GetOrientationWXYZ();
  vtk::print(fp, "{:s}orientation {:g} {:g} {:g} {:g}\n{:s}}}\n", indent, tempd[1], tempd[2],
    tempd[3], tempd[0] * vtkMath::Pi() / 180.0, indent);
  VTK_INDENT_LESS;

  // do the lights first the ambient then the others
  vtk::print(fp, "# The following environment information is disabled\n");
  vtk::print(fp, "# because a popular viewer (Template Graphics Software SceneViewer) has\n");
  vtk::print(fp, "# trouble (access violations under Windows NT) with it.\n");
  vtk::print(fp, "#{:s}Environment {{\n", indent);
  // couldn't figure out a way to do headlight -- seems to be a property of the
  // viewer not the model
  VTK_INDENT_MORE;
  vtk::print(fp, "#{:s}ambientIntensity 1.0 # ambient light\n", indent);
  vtk::print(fp, "#{:s}ambientColor {:f} {:f} {:f} }}\n\n", indent, ren->GetAmbient()[0],
    ren->GetAmbient()[1], ren->GetAmbient()[2]);
  VTK_INDENT_LESS;

  // make sure we have a default light
  // if we don't then use a headlight
  lc = ren->GetLights();
  vtkCollectionSimpleIterator lsit;
  for (lc->InitTraversal(lsit); (aLight = lc->GetNextLight(lsit));)
  {
    this->WriteALight(aLight, fp);
  }

  // do the actors now
  ac = ren->GetActors();
  vtkAssemblyPath* apath;
  vtkCollectionSimpleIterator ait;
  for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait));)
  {
    for (anActor->InitPathTraversal(); (apath = anActor->GetNextPath());)
    {
      aPart = static_cast<vtkActor*>(apath->GetLastNode()->GetViewProp());
      this->WriteAnActor(aPart, fp);
    }
  }

  VTK_INDENT_LESS;
  vtk::print(fp, "}}\n"); // close Separator

  fclose(fp);
}

void vtkIVExporter::WriteALight(vtkLight* aLight, FILE* fp)
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

  if (aLight->GetPositional())
  {
    double* attn;

    if (aLight->GetConeAngle() >= 90.0)
    {
      vtk::print(fp, "{:s}PointLight {{\n", indent);
      VTK_INDENT_MORE;
    }
    else
    {
      vtk::print(fp, "{:s}SpotLight {{\n", indent);
      VTK_INDENT_MORE;
      vtk::print(fp, "{:s}direction {:f} {:f} {:f}\n", indent, dir[0], dir[1], dir[2]);
      vtk::print(fp, "{:s}cutOffAngle {:f}\n", indent, aLight->GetConeAngle());
      // the following ignores linear and quadratic attenuation values
      attn = aLight->GetAttenuationValues();
      vtk::print(fp, "{:s}dropOffRate {:f}\n", indent, attn[0]);
    }
    vtk::print(fp, "{:s}location {:f} {:f} {:f}\n", indent, pos[0], pos[1], pos[2]);
  }
  else
  {
    vtk::print(fp, "{:s}DirectionalLight {{\n", indent);
    VTK_INDENT_MORE;
    vtk::print(fp, "{:s}direction {:f} {:f} {:f}\n", indent, dir[0], dir[1], dir[2]);
  }

  vtk::print(fp, "{:s}color {:f} {:f} {:f}\n", indent, color[0], color[1], color[2]);
  vtk::print(fp, "{:s}intensity {:f}\n", indent, aLight->GetIntensity());
  if (aLight->GetSwitch())
  {
    vtk::print(fp, "{:s}on TRUE\n{:s}}}\n", indent, indent);
  }
  else
  {
    vtk::print(fp, "{:s}on FALSE\n{:s}}}\n", indent, indent);
  }
  VTK_INDENT_LESS;
}

void vtkIVExporter::WriteAnActor(vtkActor* anActor, FILE* fp)
{
  vtkDataSet* ds;
  vtkPolyData* pd;
  vtkGeometryFilter* gf = nullptr;
  vtkPointData* pntData;
  vtkPoints* points;
  vtkDataArray* normals = nullptr;
  vtkDataArray* tcoords = nullptr;
  int i;
  vtkProperty* prop;
  double* tempd;
  vtkCellArray* cells;
  vtkIdType npts = 0;
  const vtkIdType* indx = nullptr;
  float tempf2;
  vtkPolyDataMapper* pm;
  vtkUnsignedCharArray* colors;
  double* p;
  unsigned char* c;
  vtkTransform* trans;

  // see if the actor has a mapper. it could be an assembly
  if (anActor->GetMapper() == nullptr)
  {
    return;
  }

  vtk::print(fp, "{:s}Separator {{\n", indent);
  VTK_INDENT_MORE;

  // first stuff out the transform
  trans = vtkTransform::New();
  trans->SetMatrix(anActor->vtkProp3D::GetMatrix());

  vtk::print(fp, "{:s}Transform {{\n", indent);
  VTK_INDENT_MORE;
  tempd = trans->GetPosition();
  vtk::print(fp, "{:s}translation {:g} {:g} {:g}\n", indent, tempd[0], tempd[1], tempd[2]);
  tempd = trans->GetOrientationWXYZ();
  vtk::print(fp, "{:s}rotation {:g} {:g} {:g} {:g}\n", indent, tempd[1], tempd[2], tempd[3],
    tempd[0] * vtkMath::Pi() / 180.0);
  tempd = trans->GetScale();
  vtk::print(fp, "{:s}scaleFactor {:g} {:g} {:g}\n", indent, tempd[0], tempd[1], tempd[2]);
  vtk::print(fp, "{:s}}}\n", indent);
  VTK_INDENT_LESS;
  trans->Delete();

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
  pntData = pd->GetPointData();
  normals = pntData->GetNormals();
  tcoords = pntData->GetTCoords();
  colors = pm->MapScalars(1.0);

  vtk::print(fp, "{:s}Material {{\n", indent);
  VTK_INDENT_MORE;

  // write out the material properties to the mat file
  prop = anActor->GetProperty();
  // the following is based on a guess about how VTK's GetAmbient
  // property corresponds to SoMaterial's ambientColor
  tempf2 = prop->GetAmbient();
  tempd = prop->GetAmbientColor();
  vtk::print(fp, "{:s}ambientColor {:g} {:g} {:g}\n", indent, tempd[0] * tempf2, tempd[1] * tempf2,
    tempd[2] * tempf2);
  tempf2 = prop->GetDiffuse();
  tempd = prop->GetDiffuseColor();
  vtk::print(fp, "{:s}diffuseColor {:g} {:g} {:g}\n", indent, tempd[0] * tempf2, tempd[1] * tempf2,
    tempd[2] * tempf2);
  tempf2 = prop->GetSpecular();
  tempd = prop->GetSpecularColor();
  vtk::print(fp, "{:s}specularColor {:g} {:g} {:g}\n", indent, tempd[0] * tempf2, tempd[1] * tempf2,
    tempd[2] * tempf2);
  vtk::print(fp, "{:s}shininess {:g}\n", indent, prop->GetSpecularPower() / 128.0);
  vtk::print(fp, "{:s}transparency {:g}\n", indent, 1.0 - prop->GetOpacity());
  vtk::print(fp, "{:s}}}\n", indent); // close material
  VTK_INDENT_LESS;

  // is there a texture map
  if (anActor->GetTexture())
  {
    vtkTexture* aTexture = anActor->GetTexture();
    int *size, xsize, ysize, bpp;
    vtkDataArray* scalars;
    vtkUnsignedCharArray* mappedScalars;
    unsigned char* txtrData;
    int totalValues;

    // make sure it is updated and then get some info
    if (aTexture->GetInput() == nullptr)
    {
      vtkErrorMacro(<< "texture has no input!\n");
      return;
    }
    aTexture->GetInputAlgorithm()->Update();
    size = aTexture->GetInput()->GetDimensions();
    scalars = aTexture->GetInput()->GetPointData()->GetScalars();

    // make sure scalars are non null
    if (!scalars)
    {
      vtkErrorMacro(<< "No scalar values found for texture input!\n");
      return;
    }

    // make sure using unsigned char data of color scalars type
    if (aTexture->GetColorMode() == VTK_COLOR_MODE_MAP_SCALARS ||
      (scalars->GetDataType() != VTK_UNSIGNED_CHAR))
    {
      mappedScalars = aTexture->GetMappedScalars();
    }
    else
    {
      mappedScalars = static_cast<vtkUnsignedCharArray*>(scalars);
    }

    // we only support 2d texture maps right now
    // so one of the three sizes must be 1, but it
    // could be any of them, so lets find it
    if (size[0] == 1)
    {
      xsize = size[1];
      ysize = size[2];
    }
    else
    {
      xsize = size[0];
      if (size[1] == 1)
      {
        ysize = size[2];
      }
      else
      {
        ysize = size[1];
        if (size[2] != 1)
        {
          vtkErrorMacro(<< "3D texture maps currently are not supported!\n");
          return;
        }
      }
    }

    vtk::print(fp, "{:s}Texture2 {{\n", indent);
    VTK_INDENT_MORE;
    bpp = mappedScalars->GetNumberOfComponents();
    vtk::print(fp, "{:s}image {:d} {:d} {:d}\n", indent, xsize, ysize, bpp);
    VTK_INDENT_MORE;
    txtrData = mappedScalars->GetPointer(0);
    totalValues = xsize * ysize;
    vtk::print(fp, "{:s}", indent);
    for (i = 0; i < totalValues; i++)
    {
      vtk::print(fp, "{:02x}", *txtrData);
      txtrData++;
      if (bpp > 1)
      {
        vtk::print(fp, "{:02x}", *txtrData);
        txtrData++;
      }
      if (bpp > 2)
      {
        vtk::print(fp, "{:02x}", *txtrData);
        txtrData++;
      }
      if (bpp > 3)
      {
        vtk::print(fp, "{:02x}", *txtrData);
        txtrData++;
      }
      if (i % 8 == 0)
      {
        vtk::print(fp, "\n{:s}    ", indent);
      }
      else
      {
        vtk::print(fp, " ");
      }
    }
    VTK_INDENT_LESS;
    vtk::print(fp, "{:s}}}\n", indent);
    VTK_INDENT_LESS;
  }

  // write out point data if any
  this->WritePointData(points, normals, tcoords, colors, fp);

  // write out polys if any
  if (pd->GetNumberOfPolys() > 0)
  {
    vtk::print(fp, "{:s}IndexedFaceSet {{\n", indent);
    VTK_INDENT_MORE;
    vtk::print(fp, "{:s}coordIndex  [\n", indent);
    VTK_INDENT_MORE;

    cells = pd->GetPolys();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      vtk::print(fp, "{:s}", indent);
      for (i = 0; i < npts; i++)
      {
        // treating vtkIdType as int
        vtk::print(fp, "{:d}, ", static_cast<int>(indx[i]));
        if (((i + 1) % 10) == 0)
        {
          vtk::print(fp, "\n{:s}    ", indent);
        }
      }
      vtk::print(fp, "-1,\n");
    }
    vtk::print(fp, "{:s}]\n", indent);
    VTK_INDENT_LESS;
    vtk::print(fp, "{:s}}}\n", indent);
    VTK_INDENT_LESS;
  }

  // write out tstrips if any
  if (pd->GetNumberOfStrips() > 0)
  {
    vtk::print(fp, "{:s}IndexedTriangleStripSet {{\n", indent);
    VTK_INDENT_MORE;
    vtk::print(fp, "{:s}coordIndex  [\n", indent);
    VTK_INDENT_MORE;
    cells = pd->GetStrips();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      vtk::print(fp, "{:s}", indent);
      for (i = 0; i < npts; i++)
      {
        // treating vtkIdType as int
        vtk::print(fp, "{:d}, ", static_cast<int>(indx[i]));
        if (((i + 1) % 10) == 0)
        {
          vtk::print(fp, "\n{:s}    ", indent);
        }
      }
      vtk::print(fp, "-1,\n");
    }
    vtk::print(fp, "{:s}]\n", indent);
    VTK_INDENT_LESS;
    vtk::print(fp, "{:s}}}\n", indent);
    VTK_INDENT_LESS;
  }

  // write out lines if any
  if (pd->GetNumberOfLines() > 0)
  {
    vtk::print(fp, "{:s}IndexedLineSet {{\n", indent);
    VTK_INDENT_MORE;
    vtk::print(fp, "{:s}coordIndex  [\n", indent);
    VTK_INDENT_MORE;
    cells = pd->GetLines();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      vtk::print(fp, "{:s}", indent);
      for (i = 0; i < npts; i++)
      {
        // treating vtkIdType as int
        vtk::print(fp, "{:d}, ", static_cast<int>(indx[i]));
        if (((i + 1) % 10) == 0)
        {
          vtk::print(fp, "\n{:s}    ", indent);
        }
      }
      vtk::print(fp, "-1,\n");
    }
    vtk::print(fp, "{:s}]\n", indent);
    VTK_INDENT_LESS;
    vtk::print(fp, "{:s}}}\n", indent);
    VTK_INDENT_LESS;
  }

  // write out verts if any
  // (more complex because there is no IndexedPointSet)
  if (pd->GetNumberOfVerts() > 0)
  {
    vtk::print(fp, "{:s}Separator {{\n", indent);
    VTK_INDENT_MORE;
    vtk::print(fp, "{:s}Coordinate3 {{\n", indent);
    VTK_INDENT_MORE;
    vtk::print(fp, "{:s}point [", indent);
    VTK_INDENT_MORE;
    cells = pd->GetVerts();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      for (i = 0; i < npts; i++)
      {
        p = points->GetPoint(indx[i]);
        vtk::print(fp, "{:s}{:g} {:g} {:g},\n", indent, p[0], p[1], p[2]);
      }
    }
    vtk::print(fp, "{:s}]\n", indent);
    VTK_INDENT_LESS;
    vtk::print(fp, "{:s}}}\n", indent);
    VTK_INDENT_LESS;
    if (colors)
    {
      vtk::print(fp, "{:s}PackedColor {{", indent);
      VTK_INDENT_MORE;
      vtk::print(fp, "{:s}rgba [\n", indent);
      VTK_INDENT_MORE;
      for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
      {
        vtk::print(fp, "{:s}", indent);
        for (i = 0; i < npts; i++)
        {
          c = colors->GetPointer(4 * indx[i]);
          vtk::print(fp, "{:#x}, ",
            (static_cast<unsigned long>(c[3]) << 24) | (static_cast<unsigned long>(c[2]) << 16) |
              (static_cast<unsigned long>(c[1]) << 8) | static_cast<unsigned long>(c[0]));

          if (((i + 1) % 5) == 0)
          {
            vtk::print(fp, "\n{:s}", indent);
          }
        }
      }
      vtk::print(fp, "\n{:s}]\n", indent);
      VTK_INDENT_LESS;
      vtk::print(fp, "{:s}}}\n", indent);
      VTK_INDENT_LESS;
      vtk::print(fp, "{:s}MaterialBinding {{ value PER_VERTEX_INDEXED }}\n", indent);
    }

    vtk::print(fp, "{:s}PointSet {{\n", indent);
    VTK_INDENT_MORE;
    // treating vtkIdType as int
    vtk::print(fp, "{:s}numPoints {:d}\n", indent, static_cast<int>(npts));
    VTK_INDENT_MORE;
    vtk::print(fp, "{:s}}}\n", indent);
    VTK_INDENT_LESS;
    vtk::print(fp, "{:s}}}\n", indent); // close the Separator
    VTK_INDENT_LESS;
  }
  vtk::print(fp, "{:s}}}\n", indent);
  VTK_INDENT_LESS;
  if (gf)
  {
    gf->Delete();
  }
  pm->Delete();
}

void vtkIVExporter::WritePointData(vtkPoints* points, vtkDataArray* normals, vtkDataArray* tcoords,
  vtkUnsignedCharArray* colors, FILE* fp)
{
  double* p;
  int i;
  unsigned char* c;

  // write out the points
  vtk::print(fp, "{:s}Coordinate3 {{\n", indent);
  VTK_INDENT_MORE;
  vtk::print(fp, "{:s}point [\n", indent);
  VTK_INDENT_MORE;
  for (i = 0; i < points->GetNumberOfPoints(); i++)
  {
    p = points->GetPoint(i);
    vtk::print(fp, "{:s}{:g} {:g} {:g},\n", indent, p[0], p[1], p[2]);
  }
  vtk::print(fp, "{:s}]\n", indent);
  VTK_INDENT_LESS;
  vtk::print(fp, "{:s}}}\n", indent);
  VTK_INDENT_LESS;

  // write out the point data
  if (normals)
  {
    vtk::print(fp, "{:s}Normal {{\n", indent);
    VTK_INDENT_MORE;
    vtk::print(fp, "{:s}vector [\n", indent);
    VTK_INDENT_MORE;
    for (i = 0; i < normals->GetNumberOfTuples(); i++)
    {
      p = normals->GetTuple(i);
      vtk::print(fp, "{:s}{:g} {:g} {:g},\n", indent, p[0], p[1], p[2]);
    }
    vtk::print(fp, "{:s}]\n", indent);
    VTK_INDENT_LESS;
    vtk::print(fp, "{:s}}}\n", indent);
    VTK_INDENT_LESS;
  }

  // write out the point data
  if (tcoords)
  {
    vtk::print(fp, "{:s}TextureCoordinateBinding  {{\n", indent);
    VTK_INDENT_MORE;
    vtk::print(fp, "{:s}value PER_VERTEX_INDEXED\n", indent);
    VTK_INDENT_LESS;
    vtk::print(fp, "{:s}}}\n", indent);
    vtk::print(fp, "{:s}TextureCoordinate2 {{\n", indent);
    VTK_INDENT_MORE;
    vtk::print(fp, "{:s}point [\n", indent);
    VTK_INDENT_MORE;
    for (i = 0; i < tcoords->GetNumberOfTuples(); i++)
    {
      p = tcoords->GetTuple(i);
      vtk::print(fp, "{:s}{:g} {:g},\n", indent, p[0], p[1]);
    }
    vtk::print(fp, "{:s}]\n", indent);
    VTK_INDENT_LESS;
    vtk::print(fp, "{:s}}}\n", indent);
    VTK_INDENT_LESS;
  }

  // write out the point data
  if (colors)
  {
    vtk::print(fp, "{:s}PackedColor {{\n", indent);
    VTK_INDENT_MORE;
    vtk::print(fp, "{:s}rgba [\n", indent);
    VTK_INDENT_MORE;
    vtk::print(fp, "{:s}", indent);
    for (i = 0; i < colors->GetNumberOfTuples(); i++)
    {
      c = colors->GetPointer(4 * i);
      vtk::print(fp, "{:#x}, ",
        (static_cast<unsigned long>(c[3]) << 24) | (static_cast<unsigned long>(c[2]) << 16) |
          (static_cast<unsigned long>(c[1]) << 8) | static_cast<unsigned long>(c[0]));

      if (((i + 1) % 5) == 0)
      {
        vtk::print(fp, "\n{:s}", indent);
      }
    }
    vtk::print(fp, "\n{:s}]\n", indent);
    VTK_INDENT_LESS;
    vtk::print(fp, "{:s}}}\n", indent);
    VTK_INDENT_LESS;
    vtk::print(fp, "{:s}MaterialBinding {{ value PER_VERTEX_INDEXED }}\n", indent);
  }
}

void vtkIVExporter::PrintSelf(ostream& os, vtkIndent ind)
{
  this->Superclass::PrintSelf(os, ind);

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
