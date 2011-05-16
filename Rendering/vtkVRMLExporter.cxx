/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVRMLExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVRMLExporter.h"

#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkCompositeDataSet.h"
#include "vtkGeometryFilter.h"
#include "vtkImageData.h"
#include "vtkLightCollection.h"
#include "vtkLight.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkTexture.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkVRMLExporter);

vtkVRMLExporter::vtkVRMLExporter()
{
  this->Speed = 4.0;
  this->FileName = NULL;
  this->FilePointer = NULL;
}

vtkVRMLExporter::~vtkVRMLExporter()
{
  if ( this->FileName )
    {
    delete [] this->FileName;
    }
}

void vtkVRMLExporter::SetFilePointer(FILE *fp)
{
  if (fp != this->FilePointer)
    {
    this->Modified();
    this->FilePointer = fp;
    }
}

  
void vtkVRMLExporter::WriteData()
{
  vtkRenderer *ren;
  vtkActorCollection *ac;
  vtkActor *anActor, *aPart;
  vtkLightCollection *lc;
  vtkLight *aLight;
  vtkCamera *cam;
  double *tempd;
  FILE *fp;
  
  // make sure the user specified a FileName or FilePointer
  if (!this->FilePointer && (this->FileName == NULL))
    {
    vtkErrorMacro(<< "Please specify FileName to use");
    return;
    }

  // Always pick the first renderer
  // first make sure there is only one renderer in this rendering window
  //if (this->RenderWindow->GetRenderers()->GetNumberOfItems() > 1)
  //  {
  //  vtkErrorMacro(<< "VRML files only support one renderer per window.");
  //  return;
  //  }

  // get the renderer
  ren = this->RenderWindow->GetRenderers()->GetFirstRenderer();
  
  // make sure it has at least one actor
  if (ren->GetActors()->GetNumberOfItems() < 1)
    {
    vtkErrorMacro(<< "no actors found for writing VRML file.");
    return;
    }
    
  // try opening the files
  if (!this->FilePointer)
    {
    fp = fopen(this->FileName,"w");
    if (!fp)
      {
      vtkErrorMacro(<< "unable to open VRML file " << this->FileName);
      return;
      }
    }
  else
    {
    fp = this->FilePointer;
    }
  
  //
  //  Write header
  //
  vtkDebugMacro("Writing VRML file");
  fprintf(fp,"#VRML V2.0 utf8\n");
  fprintf(fp,"# VRML file written by the visualization toolkit\n\n");

  // Start write the Background
  double background[3];
  ren->GetBackground(background);
  fprintf(fp,"    Background {\n ");
  fprintf(fp,"   skyColor [%f %f %f, ]\n", background[0], 
          background[1], background[2]);
  fprintf(fp,"    }\n ");
  // End of Background
  
  // do the camera
  cam = ren->GetActiveCamera();
  fprintf(fp,"    Viewpoint\n      {\n      fieldOfView %f\n",
          cam->GetViewAngle()*3.1415926/180.0);
  fprintf(fp,"      position %f %f %f\n",cam->GetPosition()[0],
          cam->GetPosition()[1], cam->GetPosition()[2]);
  fprintf(fp,"      description \"Default View\"\n");
  tempd = cam->GetOrientationWXYZ();
  fprintf(fp,"      orientation %g %g %g %g\n      }\n", tempd[1], tempd[2], 
          tempd[3], tempd[0]*3.1415926/180.0);

  // do the lights first the ambient then the others
  fprintf(fp,
    "    NavigationInfo {\n      type [\"EXAMINE\",\"FLY\"]\n      speed %f\n", 
          this->Speed);
  if (ren->GetLights()->GetNumberOfItems() == 0)
    {
    fprintf(fp,"      headlight TRUE}\n\n");
    }
  else
    {
    fprintf(fp,"      headlight FALSE}\n\n");
    }
  fprintf(fp,
    "    DirectionalLight { ambientIntensity 1 intensity 0 # ambient light\n");
  fprintf(fp,"      color %f %f %f }\n\n", ren->GetAmbient()[0],
          ren->GetAmbient()[1], ren->GetAmbient()[2]);
  
  // make sure we have a default light
  // if we dont then use a headlight
  lc = ren->GetLights();
  vtkCollectionSimpleIterator lsit;
  for (lc->InitTraversal(lsit); (aLight = lc->GetNextLight(lsit)); )
    {
    this->WriteALight(aLight, fp);
    }

  // do the actors now
  ac = ren->GetActors();
  vtkAssemblyPath *apath;
  vtkCollectionSimpleIterator ait;
  for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait)); )
    {
    for (anActor->InitPathTraversal(); (apath=anActor->GetNextPath()); )
      {
      aPart=static_cast<vtkActor *>(apath->GetLastNode()->GetViewProp());
      this->WriteAnActor(aPart, fp);
      }
    }
  if (!this->FilePointer)
    {
    fclose(fp);
    }
}

void vtkVRMLExporter::WriteALight(vtkLight *aLight, FILE *fp)
{
  double *pos, *focus, *color;
  double dir[3];
  
  pos = aLight->GetPosition();
  focus = aLight->GetFocalPoint();
  color = aLight->GetDiffuseColor();

  dir[0] = focus[0] - pos[0];
  dir[1] = focus[1] - pos[1];
  dir[2] = focus[2] - pos[2];
  vtkMath::Normalize(dir);
    
  if (aLight->GetPositional())
    {
    double *attn;
    
    if (aLight->GetConeAngle() >= 180.0)
      {
      fprintf(fp,"    PointLight {\n");
      }
    else
      { 
      fprintf(fp,"    SpotLight {\n");
      fprintf(fp,"      direction %f %f %f\n",dir[0], dir[1], dir[2]);
      fprintf(fp,"      cutOffAngle %f\n", aLight->GetConeAngle());
      }
    fprintf(fp,"      location %f %f %f\n", pos[0], pos[1], pos[2]);
    attn = aLight->GetAttenuationValues();
    fprintf(fp,"      attenuation %f %f %f\n", attn[0], attn[1], attn[2]);
    }
  else
    {
    fprintf(fp,"    DirectionalLight {\n");
    fprintf(fp,"      direction %f %f %f\n",dir[0], dir[1], dir[2]);
    }

  fprintf(fp,"      color %f %f %f\n", color[0], color[1], color[2]);
  fprintf(fp,"      intensity %f\n", aLight->GetIntensity());
  if (aLight->GetSwitch())
    {
    fprintf(fp,"      on TRUE\n      }\n");
    }
  else
    {
    fprintf(fp,"      on FALSE\n      }\n");
    }
}

void vtkVRMLExporter::WriteAnActor(vtkActor *anActor, FILE *fp)
{
  vtkSmartPointer<vtkPolyData> pd;
  vtkPointData *pntData;
  vtkPoints *points;
  vtkDataArray *normals = NULL;
  vtkDataArray *tcoords = NULL;
  int i, i1, i2;
  double *tempd;
  vtkCellArray *cells;
  vtkIdType npts = 0;
  vtkIdType *indx = 0;
  int pointDataWritten = 0;
  vtkPolyDataMapper *pm;
  vtkUnsignedCharArray *colors;
  double *p;
  unsigned char *c;
  vtkTransform *trans;

  // see if the actor has a mapper. it could be an assembly
  if (anActor->GetMapper() == NULL)
    {
    return;
    }
  if (anActor->GetVisibility() == 0)
    {
    return;
    }
  // first stuff out the transform
  trans = vtkTransform::New();
  trans->SetMatrix(anActor->vtkProp3D::GetMatrix());
  
  fprintf(fp,"    Transform {\n");
  tempd = trans->GetPosition();
  fprintf(fp,"      translation %g %g %g\n", tempd[0], tempd[1], tempd[2]);
  tempd = trans->GetOrientationWXYZ();
  fprintf(fp,"      rotation %g %g %g %g\n", tempd[1], tempd[2], 
          tempd[3], tempd[0]*3.1415926/180.0);
  tempd = trans->GetScale();
  fprintf(fp,"      scale %g %g %g\n", tempd[0], tempd[1], tempd[2]);
  fprintf(fp,"      children [\n");
  trans->Delete();

  vtkDataObject* inputDO = anActor->GetMapper()->GetInputDataObject(0, 0);
  if (inputDO == NULL)
    {
    return;
    }
  // we really want polydata
  if (inputDO->IsA("vtkCompositeDataSet"))
    {
    vtkCompositeDataGeometryFilter* gf = vtkCompositeDataGeometryFilter::New();
    gf->SetInput(inputDO);
    gf->Update();
    pd = gf->GetOutput();
    gf->Delete();
    }
  else if (inputDO->GetDataObjectType() != VTK_POLY_DATA)
    {
    vtkGeometryFilter *gf = vtkGeometryFilter::New();
    gf->SetInput(inputDO);
    gf->Update();
    pd = gf->GetOutput();
    gf->Delete();
    }
  else
    {
    pd = static_cast<vtkPolyData *>(inputDO);
    }

  pm = vtkPolyDataMapper::New();
  pm->SetInput(pd);
  pm->SetScalarRange(anActor->GetMapper()->GetScalarRange());
  pm->SetScalarVisibility(anActor->GetMapper()->GetScalarVisibility());
  pm->SetLookupTable(anActor->GetMapper()->GetLookupTable());
  pm->SetScalarMode(anActor->GetMapper()->GetScalarMode());

  if ( pm->GetScalarMode() == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA ||
       pm->GetScalarMode() == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA )
    {
    if ( anActor->GetMapper()->GetArrayAccessMode() == VTK_GET_ARRAY_BY_ID )
      {
      pm->ColorByArrayComponent(anActor->GetMapper()->GetArrayId(),
        anActor->GetMapper()->GetArrayComponent());
      }
    else
      {
      pm->ColorByArrayComponent(anActor->GetMapper()->GetArrayName(),
        anActor->GetMapper()->GetArrayComponent());
      }
    }

  points = pd->GetPoints();
  pntData = pd->GetPointData();
  normals = pntData->GetNormals();
  tcoords = pntData->GetTCoords();
  colors  = pm->MapScalars(1.0);
  
  // write out polys if any
  if (pd->GetNumberOfPolys() > 0)
    {
    WriteShapeBegin(anActor,fp, pd, pntData, colors);
    fprintf(fp,"          geometry IndexedFaceSet {\n");
    // two sided lighting ? for now assume it is on
    fprintf(fp,"            solid FALSE\n");
    if (!pointDataWritten)
      {
      this->WritePointData(points, normals, tcoords, colors, fp);
      pointDataWritten = 1;
      }
    else
      {
      fprintf(fp,"            coord  USE VTKcoordinates\n");
      if (normals)
        {
        fprintf(fp,"            normal  USE VTKnormals\n");
        }
      if (tcoords)
        {
        fprintf(fp,"            texCoord  USE VTKtcoords\n");
        }
      if (colors)
        {
        fprintf(fp,"            color  USE VTKcolors\n");
        }
      }
    
    fprintf(fp,"            coordIndex  [\n");
    
    cells = pd->GetPolys();
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
      {
      fprintf(fp,"              ");
      for (i = 0; i < npts; i++)
        {
        // treating vtkIdType as int
        fprintf(fp,"%i, ", static_cast<int>(indx[i]));
        }
      fprintf(fp,"-1,\n");
      }
    fprintf(fp,"            ]\n");
    fprintf(fp,"          }\n");
    WriteShapeEnd(fp);
    }

  // write out tstrips if any
  if (pd->GetNumberOfStrips() > 0)
    {
    WriteShapeBegin(anActor,fp, pd, pntData, colors);
    fprintf(fp,"          geometry IndexedFaceSet {\n");
    if (!pointDataWritten)
      {
      this->WritePointData(points, normals, tcoords, colors, fp);
      pointDataWritten = 1;
      }
    else
      {
      fprintf(fp,"            coord  USE VTKcoordinates\n");
      if (normals)
        {
        fprintf(fp,"            normal  USE VTKnormals\n");
        }
      if (tcoords)
        {
        fprintf(fp,"            texCoord  USE VTKtcoords\n");
        }
      if (colors)
        {
        fprintf(fp,"            color  USE VTKcolors\n");
        }
      }
    fprintf(fp,"            coordIndex  [\n");
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
          i1 = i - 2;
          i2 = i - 1;
          }
        // treating vtkIdType as int
        fprintf(fp,"              %i, %i, %i, -1,\n",
                static_cast<int>(indx[i1]), 
                static_cast<int>(indx[i2]),static_cast<int>(indx[i]));
        }
      }
    fprintf(fp,"            ]\n");
    fprintf(fp,"          }\n");
    WriteShapeEnd(fp);
    }
  
  // write out lines if any
  if (pd->GetNumberOfLines() > 0)
    {
    WriteShapeBegin(anActor,fp, pd,  pntData, colors);
    fprintf(fp,"          geometry IndexedLineSet {\n");
    if (!pointDataWritten)
      {
      this->WritePointData(points, NULL, NULL, colors, fp);
      pointDataWritten = 1;
      }
    else
      {
      fprintf(fp,"            coord  USE VTKcoordinates\n");
      if (colors)
        {
        fprintf(fp,"            color  USE VTKcolors\n");
        }
      }
    
    fprintf(fp,"            coordIndex  [\n");
    
    cells = pd->GetLines();
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
      {
      fprintf(fp,"              ");
      for (i = 0; i < npts; i++)
        {
        // treating vtkIdType as int
        fprintf(fp,"%i, ", static_cast<int>(indx[i]));
        }
      fprintf(fp,"-1,\n");
      }
    fprintf(fp,"            ]\n");
    fprintf(fp,"          }\n");
    WriteShapeEnd(fp);
    }

  // write out verts if any
  if (pd->GetNumberOfVerts() > 0)
    {
    WriteShapeBegin(anActor,fp, pd, pntData, colors);
    fprintf(fp,"          geometry PointSet {\n");
    cells = pd->GetVerts();
    fprintf(fp,"            coord Coordinate {");
    fprintf(fp,"              point [");
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
      {
      fprintf(fp,"              ");
      for (i = 0; i < npts; i++)
        {
        p = points->GetPoint(indx[i]);
        fprintf (fp,"              %g %g %g,\n", p[0], p[1], p[2]);
        }
      }
    fprintf(fp,"              ]\n");
    fprintf(fp,"            }\n");
    if (colors)
      {
      fprintf(fp,"            color Color {");
      fprintf(fp,"              color [");
      for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
        {
        fprintf(fp,"              ");
        for (i = 0; i < npts; i++)
          {
          c = colors->GetPointer(4*indx[i]);
          fprintf (fp,"           %g %g %g,\n", c[0]/255.0, c[1]/255.0, 
                     c[2]/255.0);
          }
        }
      fprintf(fp,"              ]\n");
      fprintf(fp,"            }\n");
      }
    fprintf(fp,"          }\n");
    WriteShapeEnd(fp);
    }

  fprintf(fp,"      ]\n"); // close the original transforms children
  fprintf(fp,"    }\n"); // close the original transform
  
  pm->Delete();
}

void vtkVRMLExporter::WriteShapeBegin( vtkActor* actor, FILE *fileP,
                                       vtkPolyData* polyData,
                                       vtkPointData* pntData,
                                       vtkUnsignedCharArray* color)
{
  double *tempd;
  double tempf2;
        
  fprintf(fileP,"        Shape {\n");
  vtkProperty* props = 0;
  // write out the material properties to the mat file
  fprintf(fileP,"          appearance Appearance {\n");
  fprintf(fileP,"            material Material {\n");
  props = actor->GetProperty();
  fprintf(fileP,"              ambientIntensity %g\n", props->GetAmbient());
  // if we don't have colors and we have only lines & points
  // use emissive to color them
  if (!(pntData->GetNormals() || color || polyData->GetNumberOfPolys() || 
                  polyData->GetNumberOfStrips()))
    {
    tempf2 = props->GetAmbient();
    tempd = props->GetAmbientColor();
    fprintf(fileP,"              emissiveColor %g %g %g\n",
    tempd[0]*tempf2, tempd[1]*tempf2, tempd[2]*tempf2);
    }
  tempf2 = props->GetDiffuse();
  tempd = props->GetDiffuseColor();
  fprintf(fileP,"              diffuseColor %g %g %g\n",
  tempd[0]*tempf2, tempd[1]*tempf2, tempd[2]*tempf2);
  tempf2 = props->GetSpecular();
  tempd = props->GetSpecularColor();
  fprintf(fileP,"              specularColor %g %g %g\n",
  tempd[0]*tempf2, tempd[1]*tempf2, tempd[2]*tempf2);
  fprintf(fileP,"              shininess %g\n",props->GetSpecularPower()/128.0);
  fprintf(fileP,"              transparency %g\n",1.0-props->GetOpacity());
  fprintf(fileP,"              }\n"); // close matrial
        
  // is there a texture map
  if (actor->GetTexture())
    {
    vtkTexture *aTexture = actor->GetTexture();
    int *size, xsize, ysize, bpp;
    vtkDataArray *scalars;
    vtkDataArray *mappedScalars;
    unsigned char *txtrData;
                
    // make sure it is updated and then get some info
    if (aTexture->GetInput() == NULL)
      {
      vtkErrorMacro(<< "texture has no input!\n");
      return;
      }
    aTexture->GetInput()->Update();
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
                
    fprintf(fileP,"            texture PixelTexture {\n");
    bpp = mappedScalars->GetNumberOfComponents();
    fprintf(fileP,"              image %i %i %i\n", xsize, ysize, bpp);
    txtrData = 
    static_cast<vtkUnsignedCharArray*>(mappedScalars)->GetPointer(0);
    int totalValues = xsize*ysize;
    for (int i = 0; i < totalValues; i++)
      {
      fprintf(fileP,"0x%.2x",*txtrData);
      txtrData++;
      if (bpp > 1) 
        {
        fprintf(fileP,"%.2x",*txtrData);
        txtrData++;
        }
      if (bpp > 2) 
        {
        fprintf(fileP,"%.2x",*txtrData);
        txtrData++;
        }
      if (bpp > 3) 
        {
        fprintf(fileP,"%.2x",*txtrData);
        txtrData++;
        }
      if (i%8 == 0)
        {
        fprintf(fileP,"\n");
        }
      else
        {
        fprintf(fileP," ");
        }
      }
    if (!(aTexture->GetRepeat()))
      {
      fprintf(fileP,"              repeatS FALSE\n");
      fprintf(fileP,"              repeatT FALSE\n");
      }
    fprintf(fileP,"              }\n"); // close texture
    }
  fprintf(fileP,"            }\n"); // close appearance
}

void vtkVRMLExporter::WriteShapeEnd( FILE *fileP )
{
  fprintf(fileP,"        }\n"); // close the  Shape
}

void vtkVRMLExporter::WritePointData(vtkPoints *points, vtkDataArray *normals,
                                     vtkDataArray *tcoords, 
                                     vtkUnsignedCharArray *colors, FILE *fp)
{
  double *p;
  int i;
  unsigned char *c;

  // write out the points
  fprintf(fp,"            coord DEF VTKcoordinates Coordinate {\n");
  fprintf(fp,"              point [\n");
  for (i = 0; i < points->GetNumberOfPoints(); i++)
    {
    p = points->GetPoint(i);
    fprintf (fp,"              %g %g %g,\n", p[0], p[1], p[2]);
    }
  fprintf(fp,"              ]\n");
  fprintf(fp,"            }\n");
  
  // write out the point data
  if (normals)
    {
    fprintf(fp,"            normal DEF VTKnormals Normal {\n");
    fprintf(fp,"              vector [\n");
    for (i = 0; i < normals->GetNumberOfTuples(); i++)
      {
      p = normals->GetTuple(i);
      fprintf (fp,"           %g %g %g,\n", p[0], p[1], p[2]);
      }
    fprintf(fp,"            ]\n");
    fprintf(fp,"          }\n");
    }

  // write out the point data
  if (tcoords)
    {
    fprintf(fp,"            texCoord DEF VTKtcoords TextureCoordinate {\n");
    fprintf(fp,"              point [\n");
    for (i = 0; i < tcoords->GetNumberOfTuples(); i++)
      {
      p = tcoords->GetTuple(i);
      fprintf (fp,"           %g %g,\n", p[0], p[1]);
      }
    fprintf(fp,"            ]\n");
    fprintf(fp,"          }\n");
    }

  // write out the point data
  if (colors)
    {
    fprintf(fp,"            color DEF VTKcolors Color {\n");
    fprintf(fp,"              color [\n");
    for (i = 0; i < colors->GetNumberOfTuples(); i++)
      {
      c = colors->GetPointer(4*i);
      fprintf (fp,"           %g %g %g,\n", c[0]/255.0, c[1]/255.0, 
               c[2]/255.0);
      }
    fprintf(fp,"            ]\n");
    fprintf(fp,"          }\n");
    }
}

void vtkVRMLExporter::PrintSelf(ostream& os, vtkIndent indent)
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
}
