/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkX3DExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen, Kristian Sons
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkX3DExporter.h"

#include "vtkActor2DCollection.h"
#include "vtkActor2D.h"
#include "vtkAssemblyPath.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkGeometryFilter.h"
#include "vtkImageData.h"
#include "vtkLightCollection.h"
#include "vtkLight.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkX3DExporterFIWriter.h"
#include "vtkX3DExporterXMLWriter.h"
#include "vtkX3D.h"

#include <sstream>
#include <cassert>

using namespace vtkX3D;

// forward declarations
static bool vtkX3DExporterWriterUsingCellColors(vtkMapper* anActor);
static bool vtkX3DExporterWriterRenderFaceSet(
  int cellType,
  int representation,
  vtkPoints* points,
  vtkIdType cellOffset,
  vtkCellArray* cells,
  vtkUnsignedCharArray* colors, bool cell_colors,
  vtkDataArray* normals, bool cell_normals,
  vtkDataArray* tcoords,
  bool common_data_written, int index, vtkX3DExporterWriter* writer);
static void vtkX3DExporterWriteData(vtkPoints *points,
  vtkDataArray *normals,
  vtkDataArray *tcoords,
  vtkUnsignedCharArray *colors,
  int index, vtkX3DExporterWriter* writer);
static void vtkX3DExporterUseData(bool normals, bool tcoords, bool colors, int index,
  vtkX3DExporterWriter* writer);
static bool vtkX3DExporterWriterRenderVerts(
  vtkPoints* points, vtkCellArray* cells,
  vtkUnsignedCharArray* colors, bool cell_colors,  vtkX3DExporterWriter* writer);
static bool vtkX3DExporterWriterRenderPoints(
  vtkPolyData* pd,
  vtkUnsignedCharArray* colors,
  bool cell_colors,
  vtkX3DExporterWriter* writer);

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkX3DExporter);

//----------------------------------------------------------------------------
vtkX3DExporter::vtkX3DExporter()
{
  this->Speed = 4.0;
  this->FileName = NULL;
  this->Binary = 0;
  this->Fastest = 0;
  this->WriteToOutputString = 0;
  this->OutputString = NULL;
  this->OutputStringLength = 0;
}

//----------------------------------------------------------------------------
vtkX3DExporter::~vtkX3DExporter()
{
  this->SetFileName(0);
  delete [] this->OutputString;
}

//----------------------------------------------------------------------------
void vtkX3DExporter::WriteData()
{
  vtkSmartPointer<vtkX3DExporterWriter> writer;
  vtkRenderer *ren;
  vtkActorCollection *ac;
  vtkActor2DCollection *a2Dc;
  vtkActor *anActor, *aPart;
  vtkActor2D *anTextActor2D, *aPart2D;
  vtkLightCollection *lc;
  vtkLight *aLight;
  vtkCamera *cam;

  // make sure the user specified a FileName or FilePointer
  if (this->FileName == NULL && (!this->WriteToOutputString))
  {
    vtkErrorMacro(<< "Please specify FileName to use");
    return;
  }

  // Let's assume the first renderer is the right one
  // first make sure there is only one renderer in this rendering window
  //if (this->RenderWindow->GetRenderers()->GetNumberOfItems() > 1)
  //  {
  //  vtkErrorMacro(<< "X3D files only support one renderer per window.");
  //  return;
  //  }

  // get the renderer
  ren = this->RenderWindow->GetRenderers()->GetFirstRenderer();

  // make sure it has at least one actor
  if (ren->GetActors()->GetNumberOfItems() < 1)
  {
    vtkErrorMacro(<< "no actors found for writing X3D file.");
    return;
  }

  // try opening the files
  if (this->Binary)
  {
    vtkX3DExporterFIWriter* temp = vtkX3DExporterFIWriter::New();
    temp->SetFastest(this->GetFastest());
    writer.TakeReference(temp);
  }
  else
  {
    writer = vtkSmartPointer<vtkX3DExporterXMLWriter>::New();
  }


  if(this->WriteToOutputString)
  {
    if (!writer->OpenStream())
    {
      vtkErrorMacro(<< "unable to open X3D stream");
      return;
    }
  }
  else
  {
    if (!writer->OpenFile(this->FileName))
    {
      vtkErrorMacro(<< "unable to open X3D file " << this->FileName);
      return;
    }
  }

  //
  //  Write header
  //
  vtkDebugMacro("Writing X3D file");

  writer->StartDocument();

  writer->StartNode(X3D);
  writer->SetField(profile, "Immersive");
  writer->SetField(vtkX3D::version, "3.0");

  writer->StartNode(head);

  writer->StartNode(meta);
  writer->SetField(name, "filename");
  writer->SetField(content, this->FileName ? this->FileName : "Stream");
  writer->EndNode();

  writer->StartNode(meta);
  writer->SetField(name, "generator");
  writer->SetField(content, "Visualization ToolKit X3D exporter v0.9.1");
  writer->EndNode();

  writer->StartNode(meta);
  writer->SetField(name, "numberofelements");
  std::ostringstream ss;
  ss << ren->GetActors()->GetNumberOfItems();
  writer->SetField(content, ss.str().c_str());
  writer->EndNode();

  writer->EndNode(); // head

  writer->StartNode(Scene);

  // Start write the Background
  writer->StartNode(Background);
  writer->SetField(skyColor, SFVEC3F, ren->GetBackground());
  writer->EndNode();
  // End of Background

  // Start write the Camera
  cam = ren->GetActiveCamera();
  writer->StartNode(Viewpoint);
  writer->SetField( fieldOfView,static_cast<float>( vtkMath::RadiansFromDegrees( cam->GetViewAngle() ) ) );
  writer->SetField(position, SFVEC3F, cam->GetPosition());
  writer->SetField(description, "Default View");
  writer->SetField(orientation, SFROTATION, cam->GetOrientationWXYZ());
  writer->SetField(centerOfRotation, SFVEC3F, cam->GetFocalPoint());
  writer->EndNode();
  // End of Camera

  // do the lights first the ambient then the others
  writer->StartNode(NavigationInfo);
  writer->SetField(type, "\"EXAMINE\" \"FLY\" \"ANY\"", true);
  writer->SetField(speed,static_cast<float>(this->Speed));
  writer->SetField(headlight, this->HasHeadLight(ren) ? true : false);
  writer->EndNode();

  writer->StartNode(DirectionalLight);
  writer->SetField(ambientIntensity, 1.0f);
  writer->SetField(intensity, 0.0f);
  writer->SetField(color, SFCOLOR, ren->GetAmbient());
  writer->EndNode();


  // label ROOT
  static double n[] = {0.0, 0.0, 0.0};
  writer->StartNode(Transform);
  writer->SetField(DEF, "ROOT");
  writer->SetField(translation, SFVEC3F, n);

  // make sure we have a default light
  // if we dont then use a headlight
  lc = ren->GetLights();
  vtkCollectionSimpleIterator lsit;
  for (lc->InitTraversal(lsit); (aLight = lc->GetNextLight(lsit)); )
  {
    if (!aLight->LightTypeIsHeadlight())
    {
      this->WriteALight(aLight, writer);
    }
  }

  // do the actors now
  ac = ren->GetActors();
  vtkAssemblyPath *apath;
  vtkCollectionSimpleIterator ait;
  int index=0;
  for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait)); )
  {
    for (anActor->InitPathTraversal(); (apath=anActor->GetNextPath()); )
    {
      if(anActor->GetVisibility()!=0)
      {
        aPart=static_cast<vtkActor *>(apath->GetLastNode()->GetViewProp());
        this->WriteAnActor(aPart, writer, index);
        index++;
      }
    }
  }
  writer->EndNode(); // ROOT Transform


  //////////////////////////////////////////////
  // do the 2D actors now
  a2Dc = ren->GetActors2D();

  if(a2Dc->GetNumberOfItems()!=0)
  {
    static double s[] = {1000000.0, 1000000.0, 1000000.0};
    writer->StartNode(ProximitySensor);
    writer->SetField(DEF, "PROX_LABEL");
    writer->SetField(size, SFVEC3F, s);
    writer->EndNode();

    //disable collision for the text annotations
    writer->StartNode(Collision);
    writer->SetField(enabled, false);

    //add a Label TRANS_LABEL for the text annotations and the sensor
    writer->StartNode(Transform);
    writer->SetField(DEF, "TRANS_LABEL");

    vtkAssemblyPath *apath2D;
    vtkCollectionSimpleIterator ait2D;
    for (a2Dc->InitTraversal(ait2D);
      (anTextActor2D = a2Dc->GetNextActor2D(ait2D)); )
    {

      for (anTextActor2D->InitPathTraversal();
        (apath2D=anTextActor2D->GetNextPath()); )
      {
        aPart2D=
              static_cast<vtkActor2D *>(apath2D->GetLastNode()->GetViewProp());
        this->WriteATextActor2D(aPart2D, writer);
      }
    }
    writer->EndNode(); // Transform
    writer->EndNode(); // Collision

    writer->StartNode(ROUTE);
    writer->SetField(fromNode, "PROX_LABEL");
    writer->SetField(fromField, "position_changed");
    writer->SetField(toNode, "TRANS_LABEL");
    writer->SetField(toField, "set_translation");
    writer->EndNode(); // Route

    writer->StartNode(ROUTE);
    writer->SetField(fromNode, "PROX_LABEL");
    writer->SetField(fromField, "orientation_changed");
    writer->SetField(toNode, "TRANS_LABEL");
    writer->SetField(toField, "set_rotation");
    writer->EndNode(); // Route
  }
  /////////////////////////////////////////////////

  writer->EndNode(); // Scene
  writer->EndNode(); // X3D
  writer->Flush();
  writer->EndDocument();
  writer->CloseFile();

  if(this->WriteToOutputString)
  {
    this->OutputStringLength = writer->GetOutputStringLength();
    this->OutputString = writer->RegisterAndGetOutputString();
  }
}

//----------------------------------------------------------------------------
void vtkX3DExporter::WriteALight(vtkLight *aLight,
  vtkX3DExporterWriter* writer)
{
  double *pos, *focus, *colord;
  double dir[3];

  pos = aLight->GetPosition();
  focus = aLight->GetFocalPoint();
  colord = aLight->GetDiffuseColor();

  dir[0] = focus[0] - pos[0];
  dir[1] = focus[1] - pos[1];
  dir[2] = focus[2] - pos[2];
  vtkMath::Normalize(dir);

  if (aLight->GetPositional())
  {
    if (aLight->GetConeAngle() >= 180.0)
    {
      writer->StartNode(PointLight);
    }
    else
    {
      writer->StartNode(SpotLight);
      writer->SetField(direction, SFVEC3F, dir);
      writer->SetField(cutOffAngle,static_cast<float>(aLight->GetConeAngle()));
    }
    writer->SetField(location, SFVEC3F, pos);
    writer->SetField(attenuation, SFVEC3F, aLight->GetAttenuationValues());

  }
  else
  {
    writer->StartNode(DirectionalLight);
    writer->SetField(direction, SFVEC3F, dir);
  }

  // TODO: Check correct color
  writer->SetField(color, SFCOLOR, colord);
  writer->SetField(intensity, static_cast<float>(aLight->GetIntensity()));
  writer->SetField(on, aLight->GetSwitch() ? true : false);
  writer->EndNode();
  writer->Flush();
}

//----------------------------------------------------------------------------
void vtkX3DExporter::WriteAnActor(vtkActor *anActor,
  vtkX3DExporterWriter* writer, int index)
{
  vtkSmartPointer<vtkDataSet> ds;
  vtkPolyData *pd;
  vtkPointData *pntData;
  vtkCellData *cellData;
  vtkPoints *points;
  vtkDataArray *normals = NULL;
  vtkDataArray *tcoords = NULL;
  vtkProperty *prop;
  vtkUnsignedCharArray *colors;
  vtkSmartPointer<vtkTransform> trans;

  // see if the actor has a mapper. it could be an assembly
  if (anActor->GetMapper() == NULL)
  {
    return;
  }

  vtkDataObject* dObj = anActor->GetMapper()->GetInputDataObject(0, 0);

  // get the mappers input and matrix
  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(dObj);
  if (cd)
  {
    vtkCompositeDataGeometryFilter* gf = vtkCompositeDataGeometryFilter::New();
    gf->SetInputConnection(anActor->GetMapper()->GetInputConnection(0, 0));
    gf->Update();
    ds = gf->GetOutput();
    gf->Delete();
  }
  else
  {
    anActor->GetMapper()->Update();
    ds = anActor->GetMapper()->GetInput();
  }

  if (!ds)
  {
    return;
  }

  // we really want polydata
  if ( ds->GetDataObjectType() != VTK_POLY_DATA )
  {
    vtkSmartPointer<vtkGeometryFilter> gf = vtkSmartPointer<vtkGeometryFilter>::New();
    gf->SetInputData(ds);
    gf->Update();
    pd = gf->GetOutput();
  }
  else
  {
    pd = static_cast<vtkPolyData *>(ds.GetPointer());
  }

  // Create a temporary poly-data mapper that we use.
  vtkSmartPointer<vtkPolyDataMapper> mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();

  mapper->SetInputData(pd);
  mapper->SetScalarRange(anActor->GetMapper()->GetScalarRange());
  mapper->SetScalarVisibility(anActor->GetMapper()->GetScalarVisibility());
  mapper->SetLookupTable(anActor->GetMapper()->GetLookupTable());
  mapper->SetScalarMode(anActor->GetMapper()->GetScalarMode());

  // Essential to turn of interpolate scalars otherwise GetScalars() may return
  // NULL. We restore value before returning.
  mapper->SetInterpolateScalarsBeforeMapping(0);
  if ( mapper->GetScalarMode() == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA ||
    mapper->GetScalarMode() == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA )
  {
    if ( anActor->GetMapper()->GetArrayAccessMode() == VTK_GET_ARRAY_BY_ID )
    {
      mapper->ColorByArrayComponent(anActor->GetMapper()->GetArrayId(),
        anActor->GetMapper()->GetArrayComponent());
    }
    else
    {
      mapper->ColorByArrayComponent(anActor->GetMapper()->GetArrayName(),
        anActor->GetMapper()->GetArrayComponent());
    }
  }

  // first stuff out the transform
  trans = vtkSmartPointer<vtkTransform>::New();
  trans->SetMatrix(anActor->vtkProp3D::GetMatrix());

  writer->StartNode(Transform);
  writer->SetField(translation, SFVEC3F, trans->GetPosition());
  writer->SetField(rotation, SFROTATION, trans->GetOrientationWXYZ());
  writer->SetField(scale, SFVEC3F, trans->GetScale());

  prop = anActor->GetProperty();
  points = pd->GetPoints();
  pntData = pd->GetPointData();
  tcoords = pntData->GetTCoords();
  cellData = pd->GetCellData();

  colors  = mapper->MapScalars(255.0);

  // Are we using cell colors? Pass the temporary mapper we created here since
  // we're assured that that mapper only has vtkPolyData as input and hence
  // don't run into issue when dealing with composite datasets.
  bool cell_colors = vtkX3DExporterWriterUsingCellColors(mapper);

  normals = pntData->GetNormals();

  // Are we using cell normals.
  bool cell_normals = false;
  if (prop->GetInterpolation() == VTK_FLAT || !normals)
  {
    // use cell normals, if any.
    normals = cellData->GetNormals();
    cell_normals = true;
  }


  // if we don't have colors and we have only lines & points
  // use emissive to color them
  bool writeEmissiveColor = !(normals || colors || pd->GetNumberOfPolys() ||
    pd->GetNumberOfStrips());

  // write out the material properties to the mat file
  int representation = prop->GetRepresentation();

  if (representation == VTK_POINTS)
  {
    // If representation is points, then we don't have to render different cell
    // types in separate shapes, since the cells type no longer matter.
    if (true)
    {
      writer->StartNode(Shape);
      this->WriteAnAppearance(anActor, writeEmissiveColor, writer);
      vtkX3DExporterWriterRenderPoints(pd, colors, cell_colors, writer);
      writer->EndNode();
    }
  }
  else
  {
    // When rendering as lines or surface, we need to respect the cell
    // structure. This requires rendering polys, tstrips, lines, verts in
    // separate shapes.
    vtkCellArray* verts = pd->GetVerts();
    vtkCellArray* lines = pd->GetLines();
    vtkCellArray* polys = pd->GetPolys();
    vtkCellArray* tstrips = pd->GetStrips();

    vtkIdType numVerts = verts->GetNumberOfCells();
    vtkIdType numLines = lines->GetNumberOfCells();
    vtkIdType numPolys = polys->GetNumberOfCells();
    vtkIdType numStrips = tstrips->GetNumberOfCells();

    bool common_data_written = false;
    if (numPolys > 0)
    {
      writer->StartNode(Shape);
      // Write Appearance
      this->WriteAnAppearance(anActor, writeEmissiveColor, writer);
      // Write Geometry
      vtkX3DExporterWriterRenderFaceSet(VTK_POLYGON, representation, points,
        (numVerts+numLines), polys,
        colors, cell_colors, normals, cell_normals,
        tcoords, common_data_written, index, writer);
      writer->EndNode();  // close the  Shape
      common_data_written = true;
    }

    if (numStrips > 0)
    {
      writer->StartNode(Shape);
      // Write Appearance
      this->WriteAnAppearance(anActor, writeEmissiveColor, writer);
      // Write Geometry
      vtkX3DExporterWriterRenderFaceSet(VTK_TRIANGLE_STRIP,
        representation, points,
        (numVerts+numLines+numPolys), tstrips,
        colors, cell_colors, normals, cell_normals,
        tcoords, common_data_written, index, writer);
      writer->EndNode();  // close the  Shape
      common_data_written = true;
    }

    if (numLines > 0)
    {
      writer->StartNode(Shape);
      // Write Appearance
      this->WriteAnAppearance(anActor, writeEmissiveColor, writer);
      // Write Geometry
      vtkX3DExporterWriterRenderFaceSet(VTK_POLY_LINE,
        (representation==VTK_SURFACE? VTK_WIREFRAME:representation),
        points, (numVerts), lines,
        colors, cell_colors, normals, cell_normals,
        tcoords, common_data_written, index, writer);
      writer->EndNode();  // close the  Shape
      common_data_written = true;
    }

    if (numVerts > 0)
    {
      writer->StartNode(Shape);
      this->WriteAnAppearance(anActor, writeEmissiveColor, writer);
      vtkX3DExporterWriterRenderVerts(
        points, verts,
        colors, cell_normals, writer);
      writer->EndNode();  // close the  Shape
    }

  }
  writer->EndNode(); // close the original transform
}

//----------------------------------------------------------------------------
void vtkX3DExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->FileName)
  {
    os << indent << "FileName: " << this->FileName << "\n";
  }
  else
  {
    os << indent << "FileName: (null)\n";
  }
  os << indent << "Speed: " << this->Speed << "\n";
  os << indent << "Binary: " << this->Binary << "\n";
  os << indent << "Fastest: " << this->Fastest << endl;

  os << indent << "WriteToOutputString: "
     << (this->WriteToOutputString ? "On" : "Off") << std::endl;
  os << indent << "OutputStringLength: " << this->OutputStringLength << std::endl;
  if (this->OutputString)
  {
    os << indent << "OutputString: " << this->OutputString << std::endl;
  }
}




//----------------------------------------------------------------------------
void vtkX3DExporter::WriteATextActor2D(vtkActor2D *anTextActor2D,
  vtkX3DExporterWriter* writer)
{
  char *ds;
  vtkTextActor *ta;
  vtkTextProperty *tp;

  if (!anTextActor2D->IsA("vtkTextActor"))
  {
    return;
  }

  ta = static_cast<vtkTextActor*>(anTextActor2D);
  tp = ta->GetTextProperty();
  ds = ta->GetInput();

  if (ds==NULL)
  {
    return;
  }

  double temp[3];

  writer->StartNode(Transform);
  temp[0] = ((ta->GetPosition()[0])/(this->RenderWindow->GetSize()[0])) - 0.5;
  temp[1] = ((ta->GetPosition()[1])/(this->RenderWindow->GetSize()[1])) - 0.5;
  temp[2] = -2.0;
  writer->SetField(translation, SFVEC3F, temp);
  temp[0] = temp[1] = temp[2] = 0.002;
  writer->SetField(scale, SFVEC3F, temp);

  writer->StartNode(Shape);

  writer->StartNode(Appearance);

  writer->StartNode(Material);
  temp[0] = 0.0; temp[1] = 0.0; temp[2] = 1.0;
  writer->SetField(diffuseColor, SFCOLOR, temp);
  tp->GetColor(temp);
  writer->SetField(emissiveColor, SFCOLOR, temp);
  writer->EndNode(); // Material

  writer->EndNode(); // Appearance

  writer->StartNode(Text);
  writer->SetField(vtkX3D::string, ds);

  std::string familyStr;
  switch(tp->GetFontFamily())
  {
  case 0:
  default:
    familyStr = "\"SANS\"";
    break;
  case 1:
    familyStr = "\"TYPEWRITER\"";
    break;
  case 2:
    familyStr = "\"SERIF\"";
    break;
  }

  std::string justifyStr;
  switch  (tp->GetJustification())
  {
  case 0:
  default:
    justifyStr += "\"BEGIN\"";
    break;
  case 2:
    justifyStr += "\"END\"";
    break;
  }

  justifyStr += " \"BEGIN\"";

  writer->StartNode(FontStyle);
  writer->SetField(family, familyStr.c_str(), true);
  writer->SetField(topToBottom, tp->GetVerticalJustification() == 2);
  writer->SetField(justify, justifyStr.c_str(), true);
  writer->SetField(size, tp->GetFontSize());
  writer->EndNode(); // FontStyle
  writer->EndNode(); // Text
  writer->EndNode(); // Shape
  writer->EndNode(); // Transform
}

void vtkX3DExporter::WriteAnAppearance(vtkActor *anActor, bool emissive,
  vtkX3DExporterWriter* writer)
{
  double tempd[3];
  double tempf2;

  vtkProperty* prop = anActor->GetProperty();

  writer->StartNode(Appearance);
  writer->StartNode(Material);
  writer->SetField(ambientIntensity,static_cast<float>(prop->GetAmbient()));

  if (emissive)
  {
    tempf2 = prop->GetAmbient();
    prop->GetAmbientColor(tempd);
    tempd[0]*=tempf2;
    tempd[1]*=tempf2;
    tempd[2]*=tempf2;
  }
  else
  {
    tempd[0] = tempd[1] = tempd[2] = 0.0f;
  }
  writer->SetField(emissiveColor, SFCOLOR, tempd);

  // Set diffuse color
  tempf2 = prop->GetDiffuse();
  prop->GetDiffuseColor(tempd);
  tempd[0]*=tempf2;
  tempd[1]*=tempf2;
  tempd[2]*=tempf2;
  writer->SetField(diffuseColor, SFCOLOR, tempd);

  // Set specular color
  tempf2 = prop->GetSpecular();
  prop->GetSpecularColor(tempd);
  tempd[0]*=tempf2;
  tempd[1]*=tempf2;
  tempd[2]*=tempf2;
  writer->SetField(specularColor, SFCOLOR, tempd);

  // Material shininess
  writer->SetField(shininess,static_cast<float>(prop->GetSpecularPower()/128.0));
  // Material transparency
  writer->SetField(transparency,static_cast<float>(1.0 - prop->GetOpacity()));
  writer->EndNode(); // close material

  // is there a texture map
  if (anActor->GetTexture())
  {
    this->WriteATexture(anActor, writer);
  }
  writer->EndNode(); // close appearance
}

void vtkX3DExporter::WriteATexture(vtkActor *anActor,
  vtkX3DExporterWriter* writer)
{
  vtkTexture *aTexture = anActor->GetTexture();
  int *size, xsize, ysize;
  vtkDataArray *scalars;
  vtkDataArray *mappedScalars;
  unsigned char *txtrData;
  int totalValues;


  // make sure it is updated and then get some info
  if (aTexture->GetInput() == NULL)
  {
    vtkErrorMacro(<< "texture has no input!\n");
    return;
  }
  aTexture->Update();
  size = aTexture->GetInput()->GetDimensions();
  scalars = aTexture->GetInput()->GetPointData()->GetScalars();

  // make sure scalars are non null
  if (!scalars)
  {
    vtkErrorMacro(<< "No scalar values found for texture input!\n");
    return;
  }

  // make sure using unsigned char data of color scalars type
  if (aTexture->GetMapColorScalarsThroughLookupTable () ||
    (scalars->GetDataType() != VTK_UNSIGNED_CHAR) )
  {
    mappedScalars = aTexture->GetMappedScalars ();
  }
  else
  {
    mappedScalars = scalars;
  }

  // we only support 2d texture maps right now
  // so one of the three sizes must be 1, but it
  // could be any of them, so lets find it
  if (size[0] == 1)
  {
    xsize = size[1]; ysize = size[2];
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

  std::vector<int> imageDataVec;
  imageDataVec.push_back(xsize);
  imageDataVec.push_back(ysize);
  imageDataVec.push_back(mappedScalars->GetNumberOfComponents());

  totalValues = xsize*ysize;
  txtrData = static_cast<vtkUnsignedCharArray*>(mappedScalars)->
    GetPointer(0);
  for (int i = 0; i < totalValues; i++)
  {
    int result = 0;
    for(int j = 0; j < imageDataVec[2]; j++)
    {
      result = result << 8;
      result += *txtrData;
      txtrData++;
    }
    imageDataVec.push_back(result);
  }




  writer->StartNode(PixelTexture);
  writer->SetField(image, &(imageDataVec.front()), imageDataVec.size(), true);
  if (!(aTexture->GetRepeat()))
  {
    writer->SetField(repeatS, false);
    writer->SetField(repeatT, false);
  }
  writer->EndNode();
}
//----------------------------------------------------------------------------
int vtkX3DExporter::HasHeadLight(vtkRenderer* ren)
{
  // make sure we have a default light
  // if we dont then use a headlight
  vtkLightCollection* lc = ren->GetLights();
  vtkCollectionSimpleIterator lsit;
  vtkLight* aLight=0;
  for (lc->InitTraversal(lsit); (aLight = lc->GetNextLight(lsit)); )
  {
    if (aLight->LightTypeIsHeadlight())
    {
      return 1;
    }
  }
  return 0;
}

// Determine if we're using cell data for scalar coloring. Returns true if
// that's the case.
static bool vtkX3DExporterWriterUsingCellColors(vtkMapper* mapper)
{
  int cellFlag = 0;
  vtkAbstractMapper::GetScalars(
    mapper->GetInput(),
    mapper->GetScalarMode(),
    mapper->GetArrayAccessMode(),
    mapper->GetArrayId(),
    mapper->GetArrayName(), cellFlag);
  return (cellFlag == 1);
}

//----------------------------------------------------------------------------
static bool vtkX3DExporterWriterRenderFaceSet(
  int cellType,
  int representation,
  vtkPoints* points,
  vtkIdType cellOffset,
  vtkCellArray* cells,
  vtkUnsignedCharArray* colors, bool cell_colors,
  vtkDataArray* normals, bool cell_normals,
  vtkDataArray* tcoords,
  bool common_data_written, int index, vtkX3DExporterWriter* writer)
{
  std::vector<int> coordIndexVector;
  std::vector<int> cellIndexVector;

  vtkIdType npts = 0;
  vtkIdType *indx = 0;

  if (cellType == VTK_POLYGON || cellType == VTK_POLY_LINE)
  {
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); cellOffset++)
    {
      for (vtkIdType cc=0; cc < npts; cc++)
      {
        coordIndexVector.push_back(static_cast<int>(indx[cc]));
      }

      if (representation == VTK_WIREFRAME && npts>2 && cellType == VTK_POLYGON)
      {
        // close the polygon.
        coordIndexVector.push_back(static_cast<int>(indx[0]));
      }
      coordIndexVector.push_back(-1);

      vtkIdType cellid = cellOffset;
      cellIndexVector.push_back(cellid);
    }
  }
  else // cellType == VTK_TRIANGLE_STRIP
  {
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); cellOffset++)
    {
      for (vtkIdType cc=2; cc < npts; cc++)
      {
        vtkIdType i1;
        vtkIdType i2;
        if (cc%2)
        {
          i1 = cc - 1;
          i2 = cc - 2;
        }
        else
        {
          i1 = cc - 2;
          i2 = cc - 1;
        }
        coordIndexVector.push_back(static_cast<int>(indx[i1]));
        coordIndexVector.push_back(static_cast<int>(indx[i2]));
        coordIndexVector.push_back(static_cast<int>(indx[cc]));

        if (representation == VTK_WIREFRAME)
        {
          // close the polygon when drawing lines
            coordIndexVector.push_back(static_cast<int>(indx[i1]));
        }
        coordIndexVector.push_back(-1);

        vtkIdType cellid = cellOffset;
        cellIndexVector.push_back(static_cast<int>(cellid));
      }
    }
  }

  if (representation == VTK_SURFACE)
  {
    writer->StartNode(IndexedFaceSet);
    writer->SetField(solid, false);
    writer->SetField(colorPerVertex, !cell_colors);
    writer->SetField(normalPerVertex, !cell_normals);
    writer->SetField(coordIndex, &(coordIndexVector.front()), coordIndexVector.size());
  }
  else
  {
    // don't save normals/tcoords when saving wireframes.
    normals = 0;
    tcoords = 0;

    writer->StartNode(IndexedLineSet);
    writer->SetField(colorPerVertex, !cell_colors);
    writer->SetField(coordIndex, &(coordIndexVector.front()), coordIndexVector.size());
  }

  if (normals && cell_normals && representation == VTK_SURFACE)
  {
    writer->SetField(normalIndex, &(cellIndexVector.front()), cellIndexVector.size());
  }

  if (colors && cell_colors)
  {
    writer->SetField(colorIndex, &(cellIndexVector.front()), cellIndexVector.size());
  }

  // Now save Coordinate, Color, Normal TextureCoordinate nodes.
  // Use DEF/USE to avoid duplicates.
  if (!common_data_written)
  {
    vtkX3DExporterWriteData(points, normals, tcoords, colors, index, writer);
  }
  else
  {
    vtkX3DExporterUseData((normals != NULL), (tcoords != NULL), (colors!= NULL), index, writer);
  }

  writer->EndNode(); // end IndexedFaceSet or IndexedLineSet
  return true;
}

static void vtkX3DExporterWriteData(vtkPoints *points,
  vtkDataArray *normals,
  vtkDataArray *tcoords,
  vtkUnsignedCharArray *colors,
  int index,
  vtkX3DExporterWriter* writer)
{
  char indexString[100];
  sprintf(indexString, "%04d", index);

  // write out the points
  std::string defString = "VTKcoordinates";
  writer->StartNode(Coordinate);
  writer->SetField(DEF, defString.append(indexString).c_str());
  writer->SetField(point, MFVEC3F, points->GetData());
  writer->EndNode();

  // write out the point data
  if (normals)
  {
    defString="VTKnormals";
    writer->StartNode(Normal);
    writer->SetField(DEF, defString.append(indexString).c_str());
    writer->SetField(vtkX3D::vector, MFVEC3F, normals);
    writer->EndNode();
  }

  // write out the point data
  if (tcoords)
  {
    defString="VTKtcoords";
    writer->StartNode(TextureCoordinate);
    writer->SetField(DEF, defString.append(indexString).c_str());
    writer->SetField(point, MFVEC2F, tcoords);
    writer->EndNode();
  }

  // write out the point data
  if (colors)
  {
    defString="VTKcolors";
    writer->StartNode(Color);
    writer->SetField(DEF, defString.append(indexString).c_str());

    std::vector<double> colorVec;
    unsigned char c[4];
    for (int i = 0; i < colors->GetNumberOfTuples(); i++)
    {
      colors->GetTypedTuple(i,c);
      colorVec.push_back(c[0]/255.0);
      colorVec.push_back(c[1]/255.0);
      colorVec.push_back(c[2]/255.0);
    }
    writer->SetField(color, &(colorVec.front()), colorVec.size());
    writer->EndNode();
  }
}

static void vtkX3DExporterUseData(bool normals, bool tcoords, bool colors, int index,
  vtkX3DExporterWriter* writer)
{
  char indexString[100];
  sprintf(indexString, "%04d", index);
  std::string defString = "VTKcoordinates";
  writer->StartNode(Coordinate);
  writer->SetField(USE, defString.append(indexString).c_str());
  writer->EndNode();

  // write out the point data
  if (normals)
  {
    defString = "VTKnormals";
    writer->StartNode(Normal);
    writer->SetField(USE, defString.append(indexString).c_str());
    writer->EndNode();
  }

  // write out the point data
  if (tcoords)
  {
    defString = "VTKtcoords";
    writer->StartNode(TextureCoordinate);
    writer->SetField(USE, defString.append(indexString).c_str());
    writer->EndNode();
  }

  // write out the point data
  if (colors)
  {
    defString = "VTKcolors";
    writer->StartNode(Color);
    writer->SetField(USE, defString.append(indexString).c_str());
    writer->EndNode();
  }
}

static bool vtkX3DExporterWriterRenderVerts(
  vtkPoints* points, vtkCellArray* cells,
  vtkUnsignedCharArray* colors, bool cell_colors,  vtkX3DExporterWriter* writer)
{
  std::vector<double> colorVector;

  if (colors)
  {
    vtkIdType cellId = 0;
    vtkIdType npts = 0;
    vtkIdType *indx = 0;
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); cellId++)
    {
      for (vtkIdType cc=0; cc < npts; cc++)
      {
        unsigned char color[4];
        if (cell_colors)
        {
          colors->GetTypedTuple(cellId, color);
        }
        else
        {
          colors->GetTypedTuple(indx[cc], color);
        }

        colorVector.push_back(color[0]/255.0);
        colorVector.push_back(color[1]/255.0);
        colorVector.push_back(color[2]/255.0);
      }
    }
  }

  writer->StartNode(PointSet);
  writer->StartNode(Coordinate);
  writer->SetField(point, MFVEC3F, points->GetData());
  writer->EndNode();
  if (colors)
  {
    writer->StartNode(Color);
    writer->SetField(point, &(colorVector.front()), colorVector.size());
    writer->EndNode();
  }
  return true;
}

static bool vtkX3DExporterWriterRenderPoints(
  vtkPolyData* pd,
  vtkUnsignedCharArray* colors,
  bool cell_colors
  ,  vtkX3DExporterWriter* writer)
{
  if (pd->GetNumberOfCells() == 0)
  {
    return false;
  }

  std::vector<double> colorVec;
  std::vector<double> coordinateVec;

  vtkPoints* points = pd->GetPoints();

  // We render as cells so that even when coloring with cell data, the points
  // are assigned colors correctly.

  if ( (colors !=0) && cell_colors)
  {
    // Cell colors are used, however PointSet element can only have point
    // colors, hence we use this method. Although here we end up with duplicate
    // points, that's exactly what happens in case of OpenGL rendering, so it's
    // okay.
    vtkIdType numCells = pd->GetNumberOfCells();
    vtkSmartPointer<vtkIdList> pointIds = vtkSmartPointer<vtkIdList>::New();
    for (vtkIdType cid =0; cid < numCells; cid++)
    {
      pointIds->Reset();
      pd->GetCellPoints(cid, pointIds);

      // Get the color for this cell.
      unsigned char color[4];
      colors->GetTypedTuple(cid, color);
      double dcolor[3];
      dcolor[0] = color[0]/255.0;
      dcolor[1] = color[1]/255.0;
      dcolor[2] = color[2]/255.0;


      for (vtkIdType cc=0; cc < pointIds->GetNumberOfIds(); cc++)
      {
        vtkIdType pid = pointIds->GetId(cc);
        double* point = points->GetPoint(pid);
        coordinateVec.push_back(point[0]);
        coordinateVec.push_back(point[1]);
        coordinateVec.push_back(point[2]);
        colorVec.push_back(dcolor[0]);
        colorVec.push_back(dcolor[1]);
        colorVec.push_back(dcolor[2]);
      }
    }
  }
  else
  {
    // Colors are point colors, simply render all the points and corresponding
    // colors.
    vtkIdType numPoints = points->GetNumberOfPoints();
    for (vtkIdType pid=0; pid < numPoints; pid++)
    {
      double* point = points->GetPoint(pid);
      coordinateVec.push_back(point[0]);
      coordinateVec.push_back(point[1]);
      coordinateVec.push_back(point[2]);

      if (colors)
      {
        unsigned char color[4];
        colors->GetTypedTuple(pid, color);
        colorVec.push_back(color[0]/255.0);
        colorVec.push_back(color[1]/255.0);
        colorVec.push_back(color[2]/255.0);
      }
    }
  }

  writer->StartNode(PointSet);
  writer->StartNode(Coordinate);
  writer->SetField(point, &(coordinateVec.front()), coordinateVec.size());
  writer->EndNode(); // Coordinate
  if (colors)
  {
    writer->StartNode(Color);
    writer->SetField(color, &(colorVec.front()), colorVec.size());
    writer->EndNode(); // Color
  }
  writer->EndNode(); // PointSet
  return true;
}
//----------------------------------------------------------------------------
char *vtkX3DExporter::RegisterAndGetOutputString()
{
  char *tmp = this->OutputString;

  this->OutputString = NULL;
  this->OutputStringLength = 0;

  return tmp;
}
