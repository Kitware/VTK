/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRIBExporter.cxx
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
#include "vtkRIBExporter.h"
#include "vtkRIBProperty.h"
#include "vtkRIBLight.h"
#include "vtkGeometryFilter.h"
#include "vtkMath.h"
#include "vtkPolygon.h"
#include "vtkTIFFWriter.h"
#include "vtkImageConstantPad.h"
#include "vtkImageExtractComponents.h"
#include "vtkImageAppendComponents.h"
#include "vtkAssemblyPath.h"
#include "vtkAssemblyNode.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkRIBExporter* vtkRIBExporter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkRIBExporter");
  if(ret)
    {
    return (vtkRIBExporter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkRIBExporter;
}




typedef float RtColor[3];
typedef float RtPoint[3];
typedef char   *RtPointer;
typedef float RtFloat;

vtkRIBExporter::vtkRIBExporter()
{
  this->FilePrefix = NULL;
  this->FilePtr = NULL;
  this->TexturePrefix = NULL;
  this->Size[0] = this->Size[1] = -1;
  this->PixelSamples[0] = this->PixelSamples[1] = 2;
  this->Background = 0;
}

vtkRIBExporter::~vtkRIBExporter()
{
  if ( this->FilePrefix )
    {
    delete [] this->FilePrefix;
    }
  if ( this->TexturePrefix )
    {
    delete [] this->TexturePrefix;
    }
}

void vtkRIBExporter::WriteData()
{
  vtkRenderer *ren;
  vtkActorCollection *ac;
  vtkLightCollection *lc;
  vtkActor *anActor;
  vtkCollection *textures = vtkCollection::New();
  vtkLight *aLight;
  vtkTexture *aTexture;
  
  // make sure the user specified a FilePrefix
  if ( this->FilePrefix == NULL)
    {
    vtkErrorMacro(<< "Please specify file name for the rib file");
    return;
    }

  // first make sure there is only one renderer in this rendering window
  if (this->RenderWindow->GetRenderers()->GetNumberOfItems() > 1)
    {
    vtkErrorMacro(<< "RIB files only support one renderer per window.");
    return;
    }

  // get the renderer
  this->RenderWindow->GetRenderers()->InitTraversal();
  ren = this->RenderWindow->GetRenderers()->GetNextItem();
  
  // make sure it has at least one actor
  if (ren->GetActors()->GetNumberOfItems() < 1)
    {
    vtkErrorMacro(<< "no actors found for writing .RIB file.");
    return;
    }
    
  char *ribFileName = new char [strlen (this->FilePrefix) + strlen (".rib") + 1];
  sprintf (ribFileName, "%s%s", this->FilePrefix, ".rib");

  this->FilePtr = fopen (ribFileName, "w");
  if (this->FilePtr == NULL)
    {
    vtkErrorMacro (<< "Cannot open " << ribFileName);
    delete [] ribFileName;
    return;
    }

  delete [] ribFileName;
        
  //
  //  Write Header
  //
  this->WriteHeader (ren);
  
  //
  //  All textures must be made first
  //
  ac = ren->GetActors();
  for ( ac->InitTraversal (); (anActor = ac->GetNextActor()); )
    {
    // see if the actor has a mapper. it could be an assembly
    if (anActor->GetMapper() == NULL)
      {
      continue;
      }
    // if it's invisible, don't make the texture
    if ( anActor->GetVisibility () )
      {
        aTexture = anActor->GetTexture ();
        if (aTexture &&
            textures->IsItemPresent (aTexture) == 0) {
          this->WriteTexture (aTexture);
          textures->AddItem (aTexture);
        }
      }
    }

  //
  // Write viewport
  //
  this->WriteViewport (ren, this->Size);


  //
  // Write camera
  //
  this->WriteCamera (ren->GetActiveCamera ());

  fprintf (this->FilePtr, "WorldBegin\n");

  //
  // Write all lights
  //
  lc = ren->GetLights();

  //
  // If there is no light defined, create one
  //
  lc->InitTraversal();
  if (lc->GetNextItem() == NULL)
    {
    vtkWarningMacro(<< "No light defined, creating one at camera position");
    ren->CreateLight();
    }

  // Create an ambient light
  this->WriteAmbientLight (1);
  int lightCount = 2;
  for (lc->InitTraversal(); (aLight = lc->GetNextItem()); )
    {
    if (aLight->GetSwitch ())
      {
      this->WriteLight(aLight, lightCount++);
      }
    }

  //
  // Write all actors
  //
  vtkAssemblyNode *node;
  ac = ren->GetActors();
  vtkAssemblyPath *apath;
  for (ac->InitTraversal(); (anActor = ac->GetNextActor()); )
    {
    for (anActor->InitPathTraversal(); (apath=anActor->GetNextPath()); )
      {
      node = apath->GetLastNode();
      if ( node->GetProp()->GetVisibility () )
        {
        if ( node->GetProp()->IsA("vtkActor") )
          {
          this->WriteActor((vtkActor *)(node->GetProp()));
          }
        }
      }
    }

  //  RiWorldEnd ();
  fprintf (this->FilePtr, "WorldEnd\n");
  //
  // Write trailer
  //
  this->WriteTrailer ();
  
  //  RiEnd ();
  fclose (this->FilePtr);

  textures->Delete();
}

void vtkRIBExporter::WriteHeader (vtkRenderer *aRen)
{

  // create a FileName to hold the renderered image
  char *imageFileName = new char [strlen (this->FilePrefix) + strlen (".tif") + 1];
  sprintf (imageFileName, "%s%s", this->FilePrefix, ".tif");

  fprintf (this->FilePtr, "FrameBegin %d\n", 1);
  fprintf (this->FilePtr, "Display \"%s\" \"file\" \"rgba\"\n", imageFileName);
  fprintf (this->FilePtr, "Declare \"bgcolor\" \"uniform color\"\n");
  if (this->Background)
  {
    float *color = aRen->GetBackground ();
    fprintf (this->FilePtr, "Imager \"background\" \"bgcolor\" [%f %f %f]\n",
        color[0], color[1], color[2]);
  }
  fprintf (this->FilePtr, "PixelSamples %d %d\n",
                this->PixelSamples[0],
                this->PixelSamples[1]);
        
  delete [] imageFileName;

}

void vtkRIBExporter::WriteTrailer ()
{
  fprintf (this->FilePtr, "FrameEnd\n");
}

void vtkRIBExporter::WriteProperty (vtkProperty *aProperty, vtkTexture *aTexture)
{
  char *mapName;
  RtFloat Ambient, Diffuse, Specular;
  RtFloat Opacity;
  RtFloat *DiffuseColor, *SpecularColor;
  RtFloat Roughness;
  RtColor opacity;
  Opacity = aProperty->GetOpacity();

  // set the opacity
  opacity[0] = Opacity;
  opacity[1] = Opacity;
  opacity[2] = Opacity;
  fprintf (this->FilePtr, "Opacity [%f %f %f]\n",
        opacity[0], opacity[1], opacity[2]);

  // set the color of the surface
  DiffuseColor = aProperty->GetDiffuseColor();
  fprintf (this->FilePtr, "Color [%f %f %f]\n",
        DiffuseColor[0], DiffuseColor[1], DiffuseColor[2]);

  // set the shader parameters
  Ambient = aProperty->GetAmbient();
  Diffuse = aProperty->GetDiffuse();
  Specular = aProperty->GetSpecular();

  SpecularColor = aProperty->GetSpecularColor();
  Roughness = (RtFloat) (1.0 / aProperty->GetSpecularPower ());

//
// if there is a texture map we need to declare it
//
  mapName = (char *) NULL;
  if (aTexture)
    {
    mapName = this->GetTextureName(aTexture);
    if (mapName)
        {
        fprintf (this->FilePtr, "Declare \"mapname\" \"uniform string\"\n");
        }
    }
//
// Now we need to check to see if an RIBProperty has been specified
//
  if (strcmp ("vtkRIBProperty", aProperty->GetClassName ()) == 0)
    {
    vtkRIBProperty *aRIBProperty = (vtkRIBProperty *) aProperty;
    if (aRIBProperty->GetDeclarations ())
      {
      fprintf (this->FilePtr, "%s", aRIBProperty->GetDeclarations ());
      }
    if (aRIBProperty->GetSurfaceShader ())
      {
      fprintf (this->FilePtr, "%s \"%s\" ", "Surface", aRIBProperty->GetSurfaceShader ());
      fprintf (this->FilePtr, "\"Ka\" [%f] ", Ambient);
      fprintf (this->FilePtr, "\"Kd\" [%f] ", Diffuse);
      fprintf (this->FilePtr, "\"Ks\" [%f] ", Specular);
      fprintf (this->FilePtr, "\"roughness\" [%f] ", Roughness);
      fprintf (this->FilePtr, "\"specularcolor\" [%f %f %f]",
        SpecularColor[0], SpecularColor[1], SpecularColor[2]);
      if (mapName)
       {
       fprintf (this->FilePtr, " \"mapname\" [\"%s\"]", mapName);
       }
      }      
    if (aRIBProperty->GetParameters ())
      {
      fprintf (this->FilePtr, "%s", aRIBProperty->GetParameters ());
      }      
      fprintf (this->FilePtr, "\n");
    if (aRIBProperty->GetDisplacementShader ())
      {
      fprintf (this->FilePtr, "%s \"%s\" ", "Displacement", aRIBProperty->GetDisplacementShader ());
      fprintf (this->FilePtr, "\"Ka\" [%f] ", Ambient);
      fprintf (this->FilePtr, "\"Kd\" [%f] ", Diffuse);
      fprintf (this->FilePtr, "\"Ks\" [%f] ", Specular);
      fprintf (this->FilePtr, "\"roughness\" [%f] ", Roughness);
      fprintf (this->FilePtr, "\"specularcolor\" [%f %f %f]",
        SpecularColor[0], SpecularColor[1], SpecularColor[2]);
      if (mapName)
       {
       fprintf (this->FilePtr, " \"mapname\" [\"%s\"]", mapName);
       }
      if (aRIBProperty->GetParameters ())
        {
        fprintf (this->FilePtr, "%s", aRIBProperty->GetParameters ());
        }      
      fprintf (this->FilePtr, "\n");
      }      
    }
// Normal Property
  else
    {
    fprintf (this->FilePtr, "Surface \"%s\" ", mapName ? "txtplastic" : "plastic");
    fprintf (this->FilePtr, "\"Ka\" [%f] ", Ambient);
    fprintf (this->FilePtr, "\"Kd\" [%f] ", Diffuse);
    fprintf (this->FilePtr, "\"Ks\" [%f] ", Specular);
    fprintf (this->FilePtr, "\"roughness\" [%f] ", Roughness);
    fprintf (this->FilePtr, "\"specularcolor\" [%f %f %f] ",
        SpecularColor[0], SpecularColor[1], SpecularColor[2]);
    if (mapName)
     {
     fprintf (this->FilePtr, " \"mapname\" [\"%s\"]", mapName);
     }
    fprintf (this->FilePtr, "\n");
    }
}


void vtkRIBExporter::WriteLight (vtkLight *aLight, int count)
{
  float color[4];
  float *Color, *Position, *FocalPoint;
  float Intensity;

  // get required info from light
  Intensity = aLight->GetIntensity();
  Color = aLight->GetColor();
  color[0] = Intensity * Color[0];
  color[1] = Intensity * Color[1];
  color[2] = Intensity * Color[2];
  color[3] = 1.0;

  FocalPoint = aLight->GetFocalPoint();
  Position   = aLight->GetPosition();

//
// Now we need to check to see if an RIBLight has been specified
//
  if (strcmp ("vtkRIBLight", aLight->GetClassName ()) == 0)
    {
    if (((vtkRIBLight *) aLight)->GetShadows())

      {
      fprintf (this->FilePtr, "Attribute \"light\" \"shadows\" \"on\"\n");
      }
    }
  // define the light source
  if (!aLight->GetPositional())
    {
    fprintf (this->FilePtr, "LightSource \"distantlight\" %d ", count);
    fprintf (this->FilePtr, "\"intensity\" [%f] ", Intensity);
    fprintf (this->FilePtr, "\"lightcolor\" [%f %f %f] ",
        color[0], color[1], color[2]);
    fprintf (this->FilePtr, "\"from\" [%f %f %f] ",
        Position[0], Position[1], Position[2]);
    fprintf (this->FilePtr, "\"to\" [%f %f %f]\n",
        FocalPoint[0], FocalPoint[1], FocalPoint[2]);
    }
  else
    {
    float coneAngle = aLight->GetConeAngle ();
    float exponent = aLight->GetExponent ();
    fprintf (this->FilePtr, "LightSource \"spotlight\" %d ", count);
    fprintf (this->FilePtr, "\"intensity\" [%f] ", Intensity);
    fprintf (this->FilePtr, "\"lightcolor\" [%f %f %f] ",
        color[0], color[1], color[2]);
    fprintf (this->FilePtr, "\"from\" [%f %f %f] ",
        Position[0], Position[1], Position[2]);
    fprintf (this->FilePtr, "\"to\" [%f %f %f]\n",
        FocalPoint[0], FocalPoint[1], FocalPoint[2]);
    fprintf (this->FilePtr, "\"coneangle\" [%f]\n", coneAngle);
    fprintf (this->FilePtr, "\"beamdistribution\" [%f]\n", exponent);
    fprintf (this->FilePtr, "\"conedeltaangle\" [%f]\n", 0.0);
    }
  if (strcmp ("vtkRIBLight", aLight->GetClassName ()) == 0)
    {
    if (((vtkRIBLight *) aLight)->GetShadows())
      {
      fprintf (this->FilePtr, "Attribute \"light\" \"shadows\" \"off\"\n");
      }
    }
}

void vtkRIBExporter::WriteAmbientLight (int count)
{
  fprintf (this->FilePtr, "LightSource \"ambientlight\" %d\n", count);
}

void vtkRIBExporter::WriteViewport (vtkRenderer *ren, int size[2])
{
  float aspect[2];
  float *vport;
  int left,right,bottom,top;
  
  if (size[0] != -1 || size[1] != -1)
    {
    vport = ren->GetViewport();

    left = (int)(vport[0]*(size[0] -1));
    right = (int)(vport[2]*(size[0] - 1));

    bottom = (int)(vport[1]*(size[1] -1));
    top = (int)(vport[3]*(size[1] - 1));
  
    fprintf (this->FilePtr, "Format %d %d 1\n", size[0], size[1]);

    fprintf (this->FilePtr, "CropWindow %f %f %f %f\n",
        vport[0], vport[2], vport[1], vport[3]);        
    
    aspect[0] = (float)(right-left+1)/(float)(top-bottom+1);
    aspect[1] = 1.0;
    fprintf (this->FilePtr, "ScreenWindow %f %f %f %f\n",
        -aspect[0], aspect[0], -1.0, 1.0);
    }
}

static void PlaceCamera (FILE *filePtr, RtPoint, RtPoint, float);
static void AimZ (FILE *filePtr, RtPoint);

void vtkRIBExporter::WriteCamera (vtkCamera *aCamera)
{
  RtPoint direction;
  float position[3], focalPoint[3];

  aCamera->GetPosition (position);
  aCamera->GetFocalPoint (focalPoint);

  direction[0] = focalPoint[0] - position[0];
  direction[1] = focalPoint[1] - position[1];
  direction[2] = focalPoint[2] - position[2];
  vtkMath::Normalize (direction);

  RtFloat angle = aCamera->GetViewAngle ();
  fprintf (this->FilePtr, "Projection \"perspective\" \"fov\" [%f]\n", angle);
  PlaceCamera (this->FilePtr, position, direction, aCamera->GetRoll ());

  fprintf (this->FilePtr, "Orientation \"rh\"\n");
}

/* 
 * PlaceCamera(): establish a viewpoint, viewing direction and orientation
 *    for a scene. This routine must be called before RiWorldBegin(). 
 *    position: a point giving the camera position
 *    direction: a point giving the camera direction relative to position
 *    roll: an optional rotation of the camera about its direction axis 
 */

static float cameraMatrix[4][4] = {
  {-1, 0, 0, 0},
  { 0, 1, 0, 0},
  { 0, 0, 1, 0},
  { 0, 0, 0, 1}
};

void PlaceCamera(FILE *filePtr, RtPoint position, RtPoint direction, float roll)
{
  fprintf (filePtr, "Identity\n");    
  fprintf (filePtr, "Transform [%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f ]\n",
        cameraMatrix[0][0], cameraMatrix[0][1], cameraMatrix[0][2], cameraMatrix[0][3],
        cameraMatrix[1][0], cameraMatrix[1][1], cameraMatrix[1][2], cameraMatrix[1][3],
        cameraMatrix[2][0], cameraMatrix[2][1], cameraMatrix[2][2], cameraMatrix[2][3],
        cameraMatrix[3][0], cameraMatrix[3][1], cameraMatrix[3][2], cameraMatrix[3][3]);

  fprintf (filePtr, "Rotate %f %f %f %f\n", -roll, 0.0, 0.0, 1.0);
    AimZ(filePtr, direction);
  fprintf (filePtr, "Translate %f %f %f\n",
        -position[0], -position[1], -position[2]);      
}

/* 
 * AimZ(): rotate the world so the directionvector points in 
 *    positive z by rotating about the y axis, then x. The cosine 
 *    of each rotation is given by components of the normalized 
 *    direction vector. Before the y rotation the direction vector 
 *    might be in negative z, but not afterward.
 */

static void 
AimZ(FILE *filePtr, RtPoint direction)
{
    double xzlen, yzlen, yrot, xrot;

    if (direction[0]==0 && direction[1]==0 && direction[2]==0)
      {
      return;
      }
    /*
     * The initial rotation about the y axis is given by the projection of
     * the direction vector onto the x,z plane: the x and z components
     * of the direction. 
     */
    xzlen = sqrt(direction[0]*direction[0]+direction[2]*direction[2]);
    if (xzlen == 0)
      {
      yrot = (direction[1] < 0) ? 180 : 0;
      }
    else
      {
      yrot = 180*acos(direction[2]/xzlen)/vtkMath::Pi();
      }
    /*
     * The second rotation, about the x axis, is given by the projection on
     * the y,z plane of the y-rotated direction vector: the original y
     * component, and the rotated x,z vector from above. 
    */
    yzlen = sqrt(direction[1]*direction[1]+xzlen*xzlen);
    xrot = 180*acos(xzlen/yzlen)/vtkMath::Pi();       /* yzlen should never be 0 */

    if (direction[1] > 0)
      {
      fprintf (filePtr, "Rotate %f %f %f %f\n", xrot, 1.0, 0.0, 0.0);
      }
    else
      {
      fprintf (filePtr, "Rotate %f %f %f %f\n", -xrot, 1.0, 0.0, 0.0);
      }
    /* The last rotation declared gets performed first */
    if (direction[0] > 0)
      {
      fprintf (filePtr, "Rotate %f %f %f %f\n", -yrot, 0.0, 1.0, 0.0);
      }
    else
      {
      fprintf (filePtr, "Rotate %f %f %f %f\n", yrot, 0.0, 1.0, 0.0);
      }
}

void vtkRIBExporter::WriteActor(vtkActor *anActor)
{
  vtkDataSet *aDataSet;
  vtkPolyData *polyData;
  vtkGeometryFilter *geometryFilter = NULL;
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  
  // see if the actor has a mapper. it could be an assembly
  if (anActor->GetMapper() == NULL)
    {
    return;
    }

  fprintf (this->FilePtr, "AttributeBegin\n");

  fprintf (this->FilePtr, "TransformBegin\n");

  // write out the property
  this->WriteProperty (anActor->GetProperty (), anActor->GetTexture ());
  
  // get the mappers input and matrix
  aDataSet = anActor->GetMapper()->GetInput();
  anActor->GetMatrix (matrix);
  matrix->Transpose();

  // insert model transformation 
  fprintf (this->FilePtr, "ConcatTransform [%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f ]\n",
        matrix->Element[0][0], matrix->Element[0][1], 
        matrix->Element[0][2], matrix->Element[0][3],
        matrix->Element[1][0], matrix->Element[1][1], 
        matrix->Element[1][2], matrix->Element[1][3],
        matrix->Element[2][0], matrix->Element[2][1], 
        matrix->Element[2][2], matrix->Element[2][3],
        matrix->Element[3][0], matrix->Element[3][1], 
        matrix->Element[3][2], matrix->Element[3][3]);

  // we really want polydata
  if ( aDataSet->GetDataObjectType() != VTK_POLY_DATA )
    {
    geometryFilter = vtkGeometryFilter::New();
    geometryFilter->SetInput(aDataSet);
    geometryFilter->Update();
    polyData = geometryFilter->GetOutput();
    }
  else
    {
    polyData = (vtkPolyData *)aDataSet;
    }

  if (polyData->GetNumberOfPolys ())
    {
    this->WritePolygons (polyData, anActor->GetMapper()->MapScalars(1.0), 
                         anActor->GetProperty ());
    }
  if (polyData->GetNumberOfStrips ())
    {
    this->WriteStrips (polyData, anActor->GetMapper()->MapScalars(1.0), 
                       anActor->GetProperty ());
    }
  fprintf (this->FilePtr, "TransformEnd\n");
  fprintf (this->FilePtr, "AttributeEnd\n");
  if (geometryFilter)
    {
    geometryFilter->Delete();
    }
  matrix->Delete();
}

void vtkRIBExporter::WritePolygons (vtkPolyData *polyData, 
                                    vtkUnsignedCharArray *c, 
                                    vtkProperty *aProperty)
{
  float vertexColors[512][3];
  RtFloat *TCoords;
  RtFloat *normals;
  RtFloat *points;
  RtPoint vertexNormals[512];
  RtPoint vertexPoints[512];
  float poly_norm[3];
  float vertexTCoords[512][2];
  vtkIdType *pts;
  vtkIdType npts;
  int k, kk;
  int rep, j, interpolation;
  int tDim;
  unsigned char *colors;
  vtkCellArray *polys;
  vtkDataArray *n = NULL;
  vtkPoints *p;
  vtkPolygon *polygon;
  vtkDataArray *t;

  // get the representation 
  rep = aProperty->GetRepresentation();

  switch (rep) 
    {
    case VTK_SURFACE:
      break;
    default: 
      vtkErrorMacro(<< "Bad representation sent\n");
      break;
    }

  // get the shading interpolation 
  interpolation = aProperty->GetInterpolation();

  // and draw the display list
  polygon = vtkPolygon::New();
  p = polyData->GetPoints();
  polys = polyData->GetPolys();

  t = polyData->GetPointData()->GetTCoords();
  if ( t ) 
    {
    tDim = t->GetNumberOfComponents();
    if (tDim != 2)
      {
      vtkDebugMacro(<< "Currently only 2d textures are supported.\n");
      t = NULL;
      }
    }

  if ( interpolation == VTK_FLAT || !(polyData->GetPointData()) || 
       !(n=polyData->GetPointData()->GetNormals()) )
    {
    n = 0;
    }

  for (polys->InitTraversal(); polys->GetNextCell(npts,pts); )
    { 
    if (!n)
      {
      polygon->ComputeNormal(p,npts,pts,poly_norm);
      }
    
    for (j = 0; j < npts; j++) 
      {
      k = j;
      if (c) 
        {
        colors = c->GetPointer(4*pts[k]);
        vertexColors[k][0] = colors[0] / 255.0;
        vertexColors[k][1] = colors[1] / 255.0;
        vertexColors[k][2] = colors[2] / 255.0;
        }
      if (t)
        {
        TCoords = t->GetTuple (pts[k]);
        vertexTCoords[k][0] = TCoords[0];
        // Renderman Textures have origin at upper left
        vertexTCoords[k][1] = 1.0 - TCoords[1];
        }
      if (n) 
        {
        normals = n->GetTuple (pts[k]);
        vertexNormals[k][0] = normals[0];
        vertexNormals[k][1] = normals[1];
        vertexNormals[k][2] = normals[2];
        }
      else 
        {
        vertexNormals[k][0] = poly_norm[0];
        vertexNormals[k][1] = poly_norm[1];
        vertexNormals[k][2] = poly_norm[2];
        }
      
      points = p->GetPoint(pts[k]);
      vertexPoints[k][0] = points[0];
      vertexPoints[k][1] = points[1]; 
      vertexPoints[k][2] = points[2];
      }
    fprintf (this->FilePtr, "Polygon ");
    fprintf (this->FilePtr, "\"P\" [");
    for (kk = 0; kk < npts; kk++)
      {
      fprintf (this->FilePtr, "%f %f %f ",
               vertexPoints[kk][0], vertexPoints[kk][1], vertexPoints[kk][2]);
      }
    fprintf (this->FilePtr, "] ");

    fprintf (this->FilePtr, "\"N\" [");
    for (kk = 0; kk < npts; kk++)
      {
      fprintf (this->FilePtr, "%f %f %f ",
               vertexNormals[kk][0], vertexNormals[kk][1], vertexNormals[kk][2]);
      }
    fprintf (this->FilePtr, "] ");


    if (c)
      {
      fprintf (this->FilePtr, "\"Cs\" [");
      for (kk = 0; kk < npts; kk++)
        {
        fprintf (this->FilePtr, "%f %f %f ",
                 vertexColors[kk][0], vertexColors[kk][1], vertexColors[kk][2]);
        }
      fprintf (this->FilePtr, "] ");
      }
    if (t)
      {
      fprintf (this->FilePtr, "\"st\" [");
      for (kk = 0; kk < npts; kk++)
        {
        fprintf (this->FilePtr, "%f %f ",
                 vertexTCoords[kk][0], vertexTCoords[kk][1]);
        }
      fprintf (this->FilePtr, "] ");
      }
    fprintf (this->FilePtr, "\n");
    }
  polygon->Delete();
}

void vtkRIBExporter::WriteStrips (vtkPolyData *polyData, 
                                  vtkUnsignedCharArray *c, 
                                  vtkProperty *aProperty)
{
  float vertexColors[512][3];
  RtFloat *TCoords;
  RtFloat *normals;
  RtFloat *points;
  RtPoint vertexNormals[512];
  RtPoint vertexPoints[512];
  float poly_norm[3];
  float vertexTCoords[512][2];
  vtkIdType *pts, npts;
  int p1, p2, p3;
  int k, kk;
  int rep, j, interpolation;
  int tDim;
  unsigned char *colors;
  vtkCellArray *strips;
  vtkDataArray *n = NULL;
  vtkPoints *p;
  vtkDataArray *t;
  vtkPolygon *polygon;
  vtkIdType idx[3];

  // get the representation 
  rep = aProperty->GetRepresentation();

  switch (rep) 
    {
    case VTK_SURFACE:
      break;
    default: 
      vtkErrorMacro(<< "Bad representation sent\n");
      break;
    }

  // get the shading interpolation 
  interpolation = aProperty->GetInterpolation();

  // and draw the display list
  p = polyData->GetPoints();
  strips = polyData->GetStrips();
  polygon = vtkPolygon::New();
  
  t = polyData->GetPointData()->GetTCoords();
  if ( t ) 
    {
    tDim = t->GetNumberOfComponents();
    if (tDim != 2)
      {
      vtkDebugMacro(<< "Currently only 2d textures are supported.\n");
      t = NULL;
      }
    }

  if ( interpolation == VTK_FLAT || !(polyData->GetPointData()) || 
       !(n=polyData->GetPointData()->GetNormals()) )
    {
    n = 0;
    }


  // each iteration returns a triangle strip
  for (strips->InitTraversal(); strips->GetNextCell(npts,pts); )
    { 
    // each triangle strip is converted into a bunch of triangles
    p1 = pts[0];
    p2 = pts[1];
    p3 = pts[2];
    for (j = 0; j < (npts-2); j++) 
      {
      if (j%2)
        {
        idx[0] = p2;
        idx[1] = p1;
        idx[2] = p3;
        }
      else
        {
        idx[0] = p1;
        idx[1] = p2;
        idx[2] = p3;
        }

      if (!n) 
        {
        polygon->ComputeNormal (p, 3, idx, poly_norm);
        }

      // build colors, texture coordinates and normals for the triangle
      for (k = 0; k < 3; k++)
        {
        if (c) 
          {
          colors = c->GetPointer(4*idx[k]);
          vertexColors[k][0] = colors[0] / 255.0;
          vertexColors[k][1] = colors[1] / 255.0;
          vertexColors[k][2] = colors[2] / 255.0;
          }
        if (t)
          {
          TCoords = t->GetTuple (idx[k]);
          vertexTCoords[k][0] = TCoords[0];
          // Renderman Textures have origin at upper left
          vertexTCoords[k][1] = 1.0 - TCoords[1];
          }
        if (n) 
          {
          normals = n->GetTuple (idx[k]);
          vertexNormals[k][0] = normals[0];
          vertexNormals[k][1] = normals[1];
          vertexNormals[k][2] = normals[2];
          }
        else 
          {
          vertexNormals[k][0] = poly_norm[0];
          vertexNormals[k][1] = poly_norm[1];
          vertexNormals[k][2] = poly_norm[2];
          }
        points = p->GetPoint(idx[k]);
        vertexPoints[k][0] = points[0];
        vertexPoints[k][1] = points[1]; 
        vertexPoints[k][2] = points[2];
        }
      fprintf (this->FilePtr, "Polygon ");
      fprintf (this->FilePtr, "\"P\" [");
      for (kk = 0; kk < 3; kk++)
        {
        fprintf (this->FilePtr, "%f %f %f ",
                 vertexPoints[kk][0], vertexPoints[kk][1], vertexPoints[kk][2]);
        }
      fprintf (this->FilePtr, "] ");

      fprintf (this->FilePtr, "\"N\" [");
      for (kk = 0; kk < 3; kk++)
        {
        fprintf (this->FilePtr, "%f %f %f ",
                 vertexNormals[kk][0], vertexNormals[kk][1], vertexNormals[kk][2]);
        }
      fprintf (this->FilePtr, "] ");

      if (c)
        {
        fprintf (this->FilePtr, "\"Cs\" [");
        for (kk = 0; kk < 3; kk++)
          {
          fprintf (this->FilePtr, "%f %f %f ",
                   vertexColors[kk][0], vertexColors[kk][1], vertexColors[kk][2]);
          }
        fprintf (this->FilePtr, "] ");
        }
      if (t)
        {
        fprintf (this->FilePtr, "\"st\" [");
        for (kk = 0; kk < 3; kk++)
          {
          fprintf (this->FilePtr, "%f %f ",
                   vertexTCoords[kk][0], vertexTCoords[kk][1]);
          }
        fprintf (this->FilePtr, "] ");
        }
      fprintf (this->FilePtr, "\n");
      // Get ready for next triangle
      p1 = p2;
      p2 = p3;
      if (j+3 < npts)
        {
        p3 = pts[j+3];
        }
      }
    }
  polygon->Delete();
}

void vtkRIBExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkExporter::PrintSelf(os,indent);
 
  if (this->FilePrefix)
    {
    os << indent << "FilePrefix: " << this->FilePrefix << "\n";
    }
  else
    {
    os << indent << "FilePrefix: (none)\n";
    }
  if (this->TexturePrefix)
    {
    os << indent << "TexturePrefix: " << this->TexturePrefix << "\n";
    }
  else
    {
    os << indent << "TexturePrefix: (none)\n";
    }
  os << indent << "Background: " << (this->Background ? "On\n" : "Off\n");
  os << indent << "Size: " << this->Size[0] << " " << this->Size[1] << "\n";
  os << indent << "PixelSamples: " << this->PixelSamples[0] << " " 
     << this->PixelSamples[1] << "\n";
}

void vtkRIBExporter::WriteTexture (vtkTexture *aTexture)
{
  vtkDataArray *scalars;
  vtkDataArray *mappedScalars;
  int *size;
  int xsize, ysize;
  unsigned short xs,ys;

//    RtToken wrap = aTexture->GetRepeat () ? RI_PERIODIC : RI_CLAMP;
//    RiMakeTexture (this->GetTIFFName (aTexture),
//                 this->GetTextureName (aTexture),
//                   wrap, wrap,
//                   RiBoxFilter,
//                   1, 1,
//                   RI_NULL);
  const char *wrap = aTexture->GetRepeat () ? "periodic" : "clamp";
  fprintf (this->FilePtr, "MakeTexture \"%s\" ", this->GetTIFFName (aTexture));
  fprintf (this->FilePtr, "\"%s\" ", this->GetTextureName (aTexture));
  fprintf (this->FilePtr, "\"%s\" \"%s\" ", wrap, wrap);
  fprintf (this->FilePtr, "\"%s\" 1 1\n", "box");

  // do an Update and get some info
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

  // xsize and ysize must be a power of 2 in OpenGL
  xs = (unsigned short)xsize;
  ys = (unsigned short)ysize;
  while (!(xs & 0x01))
    {
    xs = xs >> 1;
    }
  while (!(ys & 0x01))
    {
    ys = ys >> 1;
    }
  if ((xs > 1)||(ys > 1))
    {
    vtkWarningMacro(<< "Texture map's width and height must be a power of two in RenderMan\n");
    }

  vtkTIFFWriter *aWriter = vtkTIFFWriter::New();
  vtkImageConstantPad *icp = NULL;
  vtkImageExtractComponents *iec = NULL;
  vtkImageAppendComponents *iac1 = NULL;
  vtkImageAppendComponents *iac2 = NULL;

  vtkStructuredPoints *anImage = vtkStructuredPoints::New();
  anImage->SetDimensions (xsize, ysize, 1);
  anImage->SetUpdateExtent(0,xsize-1,0,ysize-1, 0, 0);
  anImage->SetScalarType(mappedScalars->GetDataType());
  anImage->GetPointData()->SetScalars (mappedScalars);
  int bpp = mappedScalars->GetNumberOfComponents();
  anImage->SetNumberOfScalarComponents (bpp);

  // renderman and bmrt seem to require r,g,b and alpha in all their
  // texture maps. So if our tmap doesn't have the right components
  // we add them
   if (bpp == 1) // needs intensity intensity and alpha
    {
    iac1 = vtkImageAppendComponents::New();
    iac2 = vtkImageAppendComponents::New();
    icp = vtkImageConstantPad::New();

    iac1->SetInput(0, anImage);
    iac1->SetInput(1, anImage);
    iac2->SetInput(0, iac1->GetOutput ());
    iac2->SetInput(1, anImage);
    icp->SetInput( iac2->GetOutput ());
    icp->SetConstant(255);
    icp->SetOutputNumberOfScalarComponents(4);

    aWriter->SetInput (icp->GetOutput());
    }
  else if (bpp == 2) // needs intensity intensity
    {
    iec = vtkImageExtractComponents::New();
    iac1 = vtkImageAppendComponents::New();
    iac2 = vtkImageAppendComponents::New();

    iec->SetInput( anImage);
    iec->SetComponents (0);
    iac1->SetInput(0, iec->GetOutput ());
    iac1->SetInput(1, anImage);
    iac2->SetInput(0, iec->GetOutput ());
    iac2->SetInput(1, iac1->GetOutput ());

    aWriter->SetInput (iac2->GetOutput());
    }
  else if (bpp == 3) // needs alpha
    {
    icp = vtkImageConstantPad::New();
    icp->SetInput( anImage);
    icp->SetConstant(255);
    icp->SetOutputNumberOfScalarComponents(4);
    aWriter->SetInput (icp->GetOutput());
    }
  else // needs nothing
    {
    aWriter->SetInput (anImage);
    }
  aWriter->SetFileName (this->GetTIFFName (aTexture));
  aWriter->Write ();

   if (bpp == 1)
    {
    iac1->Delete ();
    iac2->Delete ();
    icp->Delete ();
    }
  else if (bpp == 2)
    {
    iec->Delete ();
    iac1->Delete ();
    iac2->Delete ();
    }
  else if (bpp == 3)
    {
    icp->Delete ();
    }

  aWriter->Delete();
  anImage->Delete();
}

static char tiffName[4096];
static char textureName[4096];

char *vtkRIBExporter::GetTIFFName (vtkTexture *aTexture)
{
    sprintf (tiffName, "%s_%p_%d.tif", this->TexturePrefix, (void *) aTexture, (int) aTexture->GetMTime ());
    return tiffName;
}

char *vtkRIBExporter::GetTextureName (vtkTexture *aTexture)
{
    sprintf (textureName, "%s_%p_%d.txt", this->TexturePrefix, (void *) aTexture, (int) aTexture->GetMTime ());
    return textureName;
}

