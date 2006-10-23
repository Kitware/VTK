/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkX3DExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkX3DExporter.h"

#include "vtkActor2D.h"
#include "vtkActor2DCollection.h"
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
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"


//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkX3DExporter, "1.1");
vtkStandardNewMacro(vtkX3DExporter);

//----------------------------------------------------------------------------
vtkX3DExporter::vtkX3DExporter()
{
  this->Speed = 4.0;
  this->FileName = NULL;
}
//----------------------------------------------------------------------------
vtkX3DExporter::~vtkX3DExporter()
{
 if ( this->FileName )
    {
    delete [] this->FileName;
    }
}


//----------------------------------------------------------------------------
void vtkX3DExporter::WriteData()
{
  vtkRenderer *ren;
  vtkActorCollection *ac;
  vtkActor2DCollection *a2Dc;
  vtkActor *anActor, *aPart;
  vtkActor2D *anTextActor2D, *aPart2D;
  vtkLightCollection *lc;
  vtkLight *aLight;
  vtkCamera *cam;
  double *tempd;
  FILE *fp;
  
  // make sure the user specified a FileName or FilePointer
  if (this->FileName == NULL)
    {
    vtkErrorMacro(<< "Please specify FileName to use");
    return;
    }

  // first make sure there is only one renderer in this rendering window
  if (this->RenderWindow->GetRenderers()->GetNumberOfItems() > 1)
    {
    vtkErrorMacro(<< "X3D files only support one renderer per window.");
    return;
    }

  // get the renderer
  ren = this->RenderWindow->GetRenderers()->GetFirstRenderer();
  
  // make sure it has at least one actor
  if (ren->GetActors()->GetNumberOfItems() < 1)
    {
    vtkErrorMacro(<< "no actors found for writing X3D file.");
    return;
    }
    
  // try opening the files
  
    fp = fopen(this->FileName,"w");
    if (!fp)
      {
      vtkErrorMacro(<< "unable to open X3D file " << this->FileName);
      return;
      }
    
  
  //
  //  Write header
  //
  vtkDebugMacro("Writing X3D file");
 
  fprintf(fp,"<?xml version=\"1.0\" encoding =\"UTF-8\"?>\n\n");


  fprintf(fp, "<X3D profile=\"Immersive\" version=\"3.0\">\n");
  fprintf(fp,"  <head>\n");
  fprintf(fp,"    <meta name=\"filename\" content=\"%s\"/>\n",this->FileName);
  fprintf(fp,
    "    <meta name=\"author\" content=\"The Visualization ToolKit\"/>\n");
  fprintf(fp,"    <meta name=\"numberofelements\" content=\"%d\"/>\n",
        ren->GetActors()->GetNumberOfItems());
  fprintf(fp,"  </head>\n\n");

  fprintf(fp,"  <Scene>\n");

  
  // Start write the Background
  double background[3];
  ren->GetBackground(background);
  fprintf(fp,"    <Background  ");
  fprintf(fp,"   skyColor=\" %.3f %.3f %.3f\"/>\n", background[0], 
          background[1], background[2]);
 
  // End of Background
  
  // do the camera
  cam = ren->GetActiveCamera();
  fprintf(fp,"    <Viewpoint  fieldOfView=\"%.3f\"",
          cam->GetViewAngle()*3.1415926/180.0);
  fprintf(fp,"   position=\"%.3f %.3f %.3f\"",cam->GetPosition()[0],
          cam->GetPosition()[1], cam->GetPosition()[2]);
  fprintf(fp,"   description=\"Default View\"");
  tempd = cam->GetOrientationWXYZ();
  fprintf(fp,"   orientation=\"%g %g %g %g\"/>\n", tempd[1], tempd[2], 
          tempd[3], tempd[0]*3.1415926/180.0);

  // do the lights first the ambient then the others
  fprintf(fp,
    "    <NavigationInfo type='\"EXAMINE\" \"FLY\" \"ANY\"' speed=\"%.3f\"",
    this->Speed);
  if (ren->GetLights()->GetNumberOfItems() == 0)
    {
    fprintf(fp,"  headlight=\"TRUE\"/>\n\n");
    }
  else
    {
    fprintf(fp,"  headlight=\"FALSE\"/>\n\n");
    }
  fprintf(fp,"    <DirectionalLight ambientIntensity=\"1\" intensity=\"0\" ");
  fprintf(fp,"  color=\"%.3f %.3f %.3f\"/>\n\n", ren->GetAmbient()[0],
          ren->GetAmbient()[1], ren->GetAmbient()[2]);
  
  
  // label ROOT 
   fprintf(fp,"    <Transform  DEF=\"ROOT\"  translation=\"0.0 0.0 0.0\">\n");

  
  
  
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
  int index=0;
  for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait)); )
    {
    for (anActor->InitPathTraversal(); (apath=anActor->GetNextPath()); )
      {
      if(anActor->GetVisibility()!=0)
      {
        aPart=(vtkActor *)apath->GetLastNode()->GetViewProp();
        this->WriteAnActor(aPart, fp,index);
        index++;
        }
      }
    }
  
  fprintf(fp,"    </Transform>\n");

//////////////////////////////////////////////
// do the 2D actors now
  a2Dc = ren->GetActors2D();

  if(a2Dc->GetNumberOfItems()!=0)
  {
    fprintf(fp,"  <ProximitySensor  DEF=\"PROX_LABEL\" ");
    fprintf(fp," size=\"1000000.0 1000000.0 1000000.0\"/>\n");
 
  //disable collision for the text annotations
    fprintf(fp,"  <Collision  enabled=\"FALSE\">\n");
      
   //add a Label TRANS_LABEL for the text annotations and the sensor
    fprintf(fp,"    <Transform  DEF=\"TRANS_LABEL\" >\n");

    vtkAssemblyPath *apath2D;
    vtkCollectionSimpleIterator ait2D;
    for (a2Dc->InitTraversal(ait2D);
         (anTextActor2D = a2Dc->GetNextActor2D(ait2D)); )
      {
     
      for (anTextActor2D->InitPathTraversal();
           (apath2D=anTextActor2D->GetNextPath()); )
        {
        aPart2D=(vtkActor2D *)apath2D->GetLastNode()->GetViewProp();
        this->WriteanTextActor2D(aPart2D, fp);
        }
      }
       
    fprintf(fp,"    </Transform>\n");
    fprintf(fp,"  </Collision>\n");  
    fprintf(fp,
      "<ROUTE fromNode=\"PROX_LABEL\" fromField=\"position_changed\"");
    fprintf(fp," toNode=\"TRANS_LABEL\" toField=\"translation\"/>\n");

    fprintf(fp,
      "<ROUTE fromNode=\"PROX_LABEL\" fromField=\"orientation_changed\"");
    fprintf(fp," toNode=\"TRANS_LABEL\" toField=\"rotation\"/>\n");
    }   
/////////////////////////////////////////////////  

  fprintf(fp,"  </Scene>\n");
  fprintf(fp,"</X3D>\n");

  fclose(fp);
  
}


//----------------------------------------------------------------------------
void vtkX3DExporter::WriteALight(vtkLight *aLight, FILE *fp)
{
  double *pos, *focus, *color;
  double dir[3];
  
  pos = aLight->GetPosition();
  focus = aLight->GetFocalPoint();
  color = aLight->GetColor();

  dir[0] = focus[0] - pos[0];
  dir[1] = focus[1] - pos[1];
  dir[2] = focus[2] - pos[2];
  vtkMath::Normalize(dir);
    
  if (aLight->GetPositional())
    {
    double *attn;
    
    if (aLight->GetConeAngle() >= 180.0)
      {
      fprintf(fp,"    <PointLight ");
      }
    else
      { 
      fprintf(fp,"    <SpotLight ");
      fprintf(fp,"  direction=\"%.3f %.3f %.3f\"",dir[0], dir[1], dir[2]);
      fprintf(fp,"  cutOffAngle=\"%.3f\"", aLight->GetConeAngle());
      }
    fprintf(fp,"  location=\"%.3f %.3f %.3f\"", pos[0], pos[1], pos[2]);
    attn = aLight->GetAttenuationValues();
    fprintf(fp,"  attenuation=\"%.3f %.3f %.3f\"", attn[0], attn[1], attn[2]);
    }
  else
    {
    fprintf(fp,"    <DirectionalLight");
    fprintf(fp,"      direction=\"%.3f %.3f %.3f\"",dir[0], dir[1], dir[2]);
    }

  fprintf(fp,"  color=\"%.3f %.3f %.3f\"", color[0], color[1], color[2]);
  fprintf(fp,"  intensity=\"%.3f\"", aLight->GetIntensity());
  if (aLight->GetSwitch())
    {
    fprintf(fp,"  on=\"TRUE\"/>\n\n");
    }
  else
    {
    fprintf(fp,"  on=\"FALSE\"/>\n\n");
    }
}

//----------------------------------------------------------------------------
void vtkX3DExporter::WriteAnActor(vtkActor *anActor, FILE *fp,int index)
{
  vtkDataSet *ds;
  vtkPolyData *pd;
  vtkGeometryFilter *gf = NULL;
  vtkPointData *pntData;
  vtkPoints *points;
  vtkDataArray *normals = NULL;
  vtkDataArray *tcoords = NULL;
  int i, i1, i2;
  vtkProperty *prop;
  double *tempd;
  vtkCellArray *cells;
  vtkIdType npts = 0;
  vtkIdType *indx = 0;
  double tempf2;
  int pointDataWritten = 0;
  vtkPolyDataMapper *pm;
  vtkUnsignedCharArray *colors;
  double *p;
  unsigned char *c;
  vtkTransform *trans;
  int totalValues;
  
  // see if the actor has a mapper. it could be an assembly
  if (anActor->GetMapper() == NULL)
    {
    return;
    }

  // first stuff out the transform
  trans = vtkTransform::New();
  trans->SetMatrix(anActor->vtkProp3D::GetMatrix());
  
  fprintf(fp,"      <Transform ");
  tempd = trans->GetPosition();
  fprintf(fp," translation=\"%g %g %g\"", tempd[0], tempd[1], tempd[2]);
  tempd = trans->GetOrientationWXYZ();
  fprintf(fp," rotation=\"%g %g %g %g\"", tempd[1], tempd[2], 
          tempd[3], tempd[0]*3.1415926/180.0);
  tempd = trans->GetScale();
  fprintf(fp," scale=\"%g %g %g\">\n", tempd[0], tempd[1], tempd[2]);
  trans->Delete();
  
  // get the mappers input and matrix
  ds = anActor->GetMapper()->GetInput();
  
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
  
  
  fprintf(fp,"        <Shape>\n");
  
  // write out the material properties to the mat file
  fprintf(fp,"          <Appearance>\n");
  fprintf(fp,"            <Material ");
  prop = anActor->GetProperty();
  fprintf(fp," ambientIntensity=\"%g\"", prop->GetAmbient());
  // if we don't have colors and we have only lines & points
  // use emissive to color them
  if (!(normals || colors || pd->GetNumberOfPolys() || 
        pd->GetNumberOfStrips()))
    {
    tempf2 = prop->GetAmbient();
    tempd = prop->GetAmbientColor();
    fprintf(fp," emissiveColor=\"%g %g %g\"",
            tempd[0]*tempf2, tempd[1]*tempf2, tempd[2]*tempf2);
    }
  else fprintf(fp," emissiveColor=\"0 0 0\"");
  tempf2 = prop->GetDiffuse();
  tempd = prop->GetDiffuseColor();
  fprintf(fp," diffuseColor=\"%g %g %g\"",
          tempd[0]*tempf2, tempd[1]*tempf2, tempd[2]*tempf2);
  tempf2 = prop->GetSpecular();
  tempd = prop->GetSpecularColor();
  fprintf(fp," specularColor=\"%g %g %g\"",
          tempd[0]*tempf2, tempd[1]*tempf2, tempd[2]*tempf2);
  fprintf(fp," shininess=\"%g\"",prop->GetSpecularPower()/128.0);
  fprintf(fp," transparency=\"%g\"",1.0 - prop->GetOpacity());
  fprintf(fp,"/>\n"); // close matrial

  // is there a texture map
  if (anActor->GetTexture())
    {
    vtkTexture *aTexture = anActor->GetTexture();
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

    fprintf(fp,"            <PixelTexture \n");
    bpp = mappedScalars->GetNumberOfComponents();
    fprintf(fp,"              image=\"%i %i %i\"\n", xsize, ysize, bpp);
    txtrData = static_cast<vtkUnsignedCharArray*>(mappedScalars)->
                  GetPointer(0);
    totalValues = xsize*ysize;
    for (i = 0; i < totalValues; i++)
      {
      fprintf(fp,"0x%.2x",*txtrData);
      txtrData++;
      if (bpp > 1) 
        {
        fprintf(fp,"%.2x",*txtrData);
        txtrData++;
        }
      if (bpp > 2) 
        {
        fprintf(fp,"%.2x",*txtrData);
        txtrData++;
        }
      if (bpp > 3) 
        {
        fprintf(fp,"%.2x",*txtrData);
        txtrData++;
        }
      if (i%8 == 0)
        {
        fprintf(fp,"\n");
        }
      else
        {
        fprintf(fp," ");
        }
      }
    if (!(aTexture->GetRepeat()))
      {
      fprintf(fp,"              repeatS=\"FALSE\"\n");
      fprintf(fp,"              repeatT=\"FALSE\"\n");
      }
    fprintf(fp,"              />\n"); // close texture
    }
  fprintf(fp,"            </Appearance>\n"); // close appearance

  // write out polys if any
  if (pd->GetNumberOfPolys() > 0)
    {
    fprintf(fp,"          <IndexedFaceSet \n");
    // two sided lighting ? for now assume it is on
    fprintf(fp,"            solid=\"FALSE\"\n");
   /////////////////////
    fprintf(fp,"            coordIndex  =\"\n");
    
    cells = pd->GetPolys();
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
      {
      fprintf(fp,"              ");
      for (i = 0; i < npts; i++)
        {
        // treating vtkIdType as int
        fprintf(fp,"%i ", (int)indx[i]);
        }
      fprintf(fp,"-1\n");
      }
    fprintf(fp,"            \"\n");
    fprintf(fp,"          >\n");
    
  ///////////////////////////// 
  if (!pointDataWritten)
    {
    this->WritePointData(points, normals, tcoords, colors, fp,index);
    pointDataWritten = 1;
    }
    fprintf(fp,"          </IndexedFaceSet> \n");
  }

  // write out tstrips if any
  if (pd->GetNumberOfStrips() > 0)
    {
    fprintf(fp,"           <IndexedFaceSet \n");
    ///////////
     fprintf(fp,"            coordIndex =\" \n");
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
        fprintf(fp,"              %i %i %i -1,\n", (int)indx[i1], 
                (int)indx[i2], (int)indx[i]);
        }
      }
    fprintf(fp,"            \"\n");
    fprintf(fp,"          >\n");
    
 ///////////////   
    if (!pointDataWritten)
      {
      this->WritePointData(points, normals, tcoords, colors, fp,index);
      pointDataWritten = 1;
      }
    fprintf(fp,"          </IndexedFaceSet>\n");
   }
  
  // write out lines if any
  if (pd->GetNumberOfLines() > 0)
    {
    fprintf(fp,"          <IndexedLineSet \n");
   ////////////
    fprintf(fp,"            coordIndex  =\"\n");
    
    cells = pd->GetLines();
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
      {
      fprintf(fp,"              ");
      for (i = 0; i < npts; i++)
        {
        // treating vtkIdType as int
        fprintf(fp,"%i, ", (int)indx[i]);
        }
      fprintf(fp,"-1,\n");
      }
    fprintf(fp,"            \"\n");
    fprintf(fp,"          >\n");
    
    ///////////////  
    
   if (!pointDataWritten)
      {
       
      this->WritePointData(points, NULL, NULL, colors, fp,index);
      pointDataWritten = 1;
      }
     fprintf(fp,"          </IndexedLineSet> \n");
   
}
  // write out verts if any
  if (pd->GetNumberOfVerts() > 0)
    {
    fprintf(fp,"           <PointSet>\n");
    cells = pd->GetVerts();
    fprintf(fp,"             <Coordinate ");
    fprintf(fp,"              point =\"");
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
      {
      fprintf(fp,"              ");
      for (i = 0; i < npts; i++)
        {
        p = points->GetPoint(indx[i]);
        fprintf (fp,"              %g %g %g,\n", p[0], p[1], p[2]);
        }
      }
    fprintf(fp,"              \"\n");
    fprintf(fp,"            />\n");
    if (colors)
      {
      fprintf(fp,"            <Color ");
      fprintf(fp,"              color =\"");
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
      fprintf(fp,"              \"\n");
      fprintf(fp,"            />\n");
      }
    fprintf(fp,"          </PointSet>\n");
    }
  fprintf(fp,"        </Shape>\n"); // close the  Shape
  fprintf(fp,"      </Transform>\n"); // close the original transform
  
  if (gf)
    {
    gf->Delete();
    }
  pm->Delete();
}

//----------------------------------------------------------------------------
void vtkX3DExporter::WritePointData(vtkPoints *points, vtkDataArray *normals,
                                     vtkDataArray *tcoords, 
                                     vtkUnsignedCharArray *colors,
                                     FILE *fp,int index)
{
  double *p;
  int i;
  unsigned char *c;
  
  // write out the points
  fprintf(fp,"            <Coordinate DEF =\"VTKcoordinates%04d\"  \n",index);
  fprintf(fp,"              point =\"\n");
  for (i = 0; i < points->GetNumberOfPoints(); i++)
    {
    p = points->GetPoint(i);
    fprintf (fp,"              %g %g %g,\n", p[0], p[1], p[2]);
    }
  fprintf(fp,"              \"\n");
  fprintf(fp,"            />\n");
  
  // write out the point data
  if (normals)
    {
    fprintf(fp,"            <Normal DEF =\"VTKnormals%04d\"  \n",index);
    fprintf(fp,"              vector =\"\n");
    for (i = 0; i < normals->GetNumberOfTuples(); i++)
      {
      p = normals->GetTuple(i);
      fprintf (fp,"           %g %g %g,\n", p[0], p[1], p[2]);
      }
    fprintf(fp,"            \"\n");
    fprintf(fp,"          />\n");
    }

  // write out the point data
  if (tcoords)
    {
    fprintf(fp,"            <TextureCoordinate DEF =\"VTKtcoords%04d\"  \n",
            index);
    fprintf(fp,"              point =\"\n");
    for (i = 0; i < tcoords->GetNumberOfTuples(); i++)
      {
      p = tcoords->GetTuple(i);
      fprintf (fp,"           %g %g,\n", p[0], p[1]);
      }
    fprintf(fp,"            \"\n");
    fprintf(fp,"          />\n");
    }

  // write out the point data
  if (colors)
    {
    fprintf(fp,"            <Color DEF =\"VTKcolors%04d\"  \n",index);
    fprintf(fp,"              color=\"\n");
    for (i = 0; i < colors->GetNumberOfTuples(); i++)
      {
      c = colors->GetPointer(4*i);
      fprintf (fp,"           %g %g %g,\n", c[0]/255.0, c[1]/255.0, 
               c[2]/255.0);
      }
    fprintf(fp,"            \"\n");
    fprintf(fp,"          />\n");
    }
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
}




//----------------------------------------------------------------------------
void vtkX3DExporter::WriteanTextActor2D(vtkActor2D *anTextActor2D, FILE *fp)
{
  char *ds;
  double x,y;
  vtkTextMapper *tm;
  
  // see if the actor has a mapper. it could be an assembly
  if ((vtkTextMapper*)(anTextActor2D->GetMapper()) == NULL)
    {
    return;
    }

  // add a sensor with a big size for the text annotations
 
  tm=(vtkTextMapper*)anTextActor2D->GetMapper();
  ds = NULL;
  ds = tm->GetInput();

  if (ds==NULL)
    {
     return;
    }

  x=((anTextActor2D->GetPosition()[0])/(this->RenderWindow->GetSize()[0]));
  x-=0.5;
  y=((anTextActor2D->GetPosition()[1])/(this->RenderWindow->GetSize()[1]));
  y-=0.5;
 
  fprintf(fp,"      <Transform  translation=\"%.3f %.3f -2\" ",x,y);
  fprintf(fp,"scale=\"0.002 0.002 0.002\">\n");
  fprintf(fp,"        <Shape >\n");
  fprintf(fp,"          <Appearance >\n");
  fprintf(fp,"            <Material  diffuseColor=\"0 0 1\" ");
  fprintf(fp," emissiveColor=\"%.3f %.3f %.3f\"/>\n",
  tm->GetTextProperty()->GetColor()[0],
  tm->GetTextProperty()->GetColor()[1],
  tm->GetTextProperty()->GetColor()[2]);
  fprintf(fp,"          </Appearance>\n");
  fprintf(fp,"          <Text  string=\"%s\">\n",ds);
 
  vtkstd::string style;
  
  style = " family=\"";
  switch(tm->GetTextProperty()->GetFontFamily())
  {
    case 0:
    default:
      style+="SANS";
      break;
    case 1:
      style+="TIPEWRITER";
      break;
    case 2:
      style+="SERIF";
      break;
  }
  style += "\" topToBottom=\"";
  switch( tm->GetTextProperty()->GetVerticalJustification())
  {
    case 0:
    default: 
      style += "FALSE\"";
      break;
    case 2:
      style += "TRUE\"";
    break;
  }
  
  style+="  justify='\"";
  
  switch  (tm->GetTextProperty()->GetJustification())
  {
    case 0:
    default:
      style += "BEGIN\"";
    break;
    case 2:
      style += "END\"";
    break;
  }
  
  style += " \"BEGIN\"'";
  
  fprintf(fp,"            <FontStyle  %s",style.c_str());
  fprintf(fp," size=\"%d\"/>\n",tm->GetTextProperty()->GetFontSize());
  fprintf(fp,"          </Text>\n");
  fprintf(fp,"        </Shape>\n");
  fprintf(fp,"      </Transform>\n");
} 
