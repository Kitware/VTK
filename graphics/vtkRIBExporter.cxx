/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRIBExporter.cxx
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
#include "vtkRIBExporter.h"
#include "vtkRIBProperty.h"
#include "vtkGeometryFilter.h"
#include "vtkMath.h"
#include "vtkPolygon.h"
#include "vtkTIFFWriter.h"

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
  if ( this->FilePrefix ) delete [] this->FilePrefix;
  if ( this->TexturePrefix ) delete [] this->TexturePrefix;
}

void vtkRIBExporter::WriteData()
{
  vtkRenderer *ren;
  vtkActorCollection *ac;
  vtkLightCollection *lc;
  vtkActor *anActor, *aPart;
  vtkCollection *textures = new vtkCollection;
  vtkLight *aLight;
  vtkTexture *aTexture;
  int *size;
  
  // make sure the user specified a FilePrefix
  if ( this->FilePrefix == NULL)
    {
    vtkErrorMacro(<< "Please specify file name for the rib file");
    return;
    }

  // first make sure there is only one renderer in this rendering window
  if (this->Input->GetRenderers()->GetNumberOfItems() > 1)
    {
    vtkErrorMacro(<< "RIB files only support one renderer per window.");
    return;
    }

  // get the renderer
  this->Input->GetRenderers()->InitTraversal();
  ren = this->Input->GetRenderers()->GetNextItem();
  
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
    delete ribFileName;
    return;
    }

  delete ribFileName;
	
  //
  //  Write Header
  //
  this->WriteHeader (ren);
  
  //
  //  All textures must be made first
  //
  ac = ren->GetActors();
  for ( ac->InitTraversal (); (anActor = ac->GetNextItem()); )
    {
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
  if (this->Size[0] == -1 && this->Size[1] == -1)
    {
    size = this->Input->GetSize();
    }
  else
    {
    size = this->GetSize ();
    }
  this->WriteViewport (ren, size);

  //
  // Write camera
  //
  this->WriteCamera (ren->GetActiveCamera ());

//  RiWorldBegin ();
    fprintf (this->FilePtr, "WorldBegin\n");
  //
  // Write all lights
  //
  lc = ren->GetLights();
  // Create an ambient light
  this->WriteAmbientLight (1);
  int lightCount = 2;
  for (lc->InitTraversal(); (aLight = lc->GetNextItem()); )
    {
    if (aLight->GetSwitch ()) this->WriteLight(aLight, lightCount++);
    }

  //
  // Write all actors
  //
  ac = ren->GetActors();
  for (ac->InitTraversal(); (anActor = ac->GetNextItem()); )
    {
    for (anActor->InitPartTraversal();(aPart=anActor->GetNextPart()); )
      {
      if ( anActor->GetVisibility () ) this->WriteActor(aPart);
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

  delete textures;
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
	
  delete imageFileName;

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
  float	dx, dy, dz;
  float	color[4];
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
  dx = FocalPoint[0] - Position[0];
  dy = FocalPoint[1] - Position[1];
  dz = FocalPoint[2] - Position[2];

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

static void PlaceCamera (FILE *filePtr, RtPoint, RtPoint, float);
static void AimZ (FILE *filePtr, RtPoint);
static vtkMath math;

void vtkRIBExporter::WriteCamera (vtkCamera *aCamera)
{
  RtPoint direction;
  float position[3], focalPoint[3];
  vtkMatrix4x4 matrix;

  aCamera->GetPosition (position);
  aCamera->GetFocalPoint (focalPoint);

  direction[0] = focalPoint[0] - position[0];
  direction[1] = focalPoint[1] - position[1];
  direction[2] = focalPoint[2] - position[2];
  math.Normalize (direction);
  

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

static float matrix[4][4] = {
  {-1, 0, 0, 0},
  { 0, 1, 0, 0},
  { 0, 0, 1, 0},
  { 0, 0, 0, 1}
};

void PlaceCamera(FILE *filePtr, RtPoint position, RtPoint direction, float roll)
{
  fprintf (filePtr, "Identity\n");    
  fprintf (filePtr, "Transform [%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f ]\n",
	matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3],
	matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3],
	matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3],
	matrix[3][0], matrix[3][1], matrix[3][2], matrix[3][3]);

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

#define PI 3.14159265359
static void 
AimZ(FILE *filePtr, RtPoint direction)
{
    double xzlen, yzlen, yrot, xrot;

    if (direction[0]==0 && direction[1]==0 && direction[2]==0)
        return;
    /*
     * The initial rotation about the y axis is given by the projection of
     * the direction vector onto the x,z plane: the x and z components
     * of the direction. 
     */
    xzlen = sqrt(direction[0]*direction[0]+direction[2]*direction[2]);
    if (xzlen == 0)
        yrot = (direction[1] < 0) ? 180 : 0;
    else
        yrot = 180*acos(direction[2]/xzlen)/PI;
    /*
     * The second rotation, about the x axis, is given by the projection on
     * the y,z plane of the y-rotated direction vector: the original y
     * component, and the rotated x,z vector from above. 
    */
    yzlen = sqrt(direction[1]*direction[1]+xzlen*xzlen);
    xrot = 180*acos(xzlen/yzlen)/PI;       /* yzlen should never be 0 */

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
  static vtkMatrix4x4 matrix;
  
  fprintf (this->FilePtr, "AttributeBegin\n");

  fprintf (this->FilePtr, "TransformBegin\n");

  // write out the property
  this->WriteProperty (anActor->GetProperty (), anActor->GetTexture ());
  
  // get the mappers input and matrix
  aDataSet = anActor->GetMapper()->GetInput();
  anActor->GetMatrix (matrix);
  matrix.Transpose();

  // insert model transformation 
  fprintf (this->FilePtr, "ConcatTransform [%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f ]\n",
	matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3],
	matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3],
	matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3],
	matrix[3][0], matrix[3][1], matrix[3][2], matrix[3][3]);

  // we really want polydata
  if (strcmp(aDataSet->GetClassName(),"vtkPolyData"))
    {
    geometryFilter = new vtkGeometryFilter;
    geometryFilter->SetInput(aDataSet);
    geometryFilter->Update();
    polyData = geometryFilter->GetOutput();
    }
  else
    {
    polyData = (vtkPolyData *)aDataSet;
    }

  if (polyData->GetNumberOfPolys ()) this->WritePolygons (polyData, anActor->GetMapper()->GetColors (), anActor->GetProperty ());
  if (polyData->GetNumberOfStrips ()) this->WriteStrips (polyData, anActor->GetMapper()->GetColors (), anActor->GetProperty ());
  fprintf (this->FilePtr, "TransformEnd\n");
  fprintf (this->FilePtr, "AttributeEnd\n");
  if (geometryFilter) geometryFilter->Delete();
}

void vtkRIBExporter::WritePolygons (vtkPolyData *polyData, vtkColorScalars *c, vtkProperty *aProperty)
{
  float vertexColors[100][3];
  RtFloat *TCoords;
  RtFloat *normals;
  RtFloat *points;
  RtPoint vertexNormals[100];
  RtPoint vertexPoints[100];
  float poly_norm[3];
  float vertexTCoords[100][2];
  int *pts;
  int k, kk;
  int npts, rep, j, interpolation;
  int tDim;
  unsigned char *colors;
  vtkCellArray *polys;
  vtkNormals *n = NULL;
  vtkPoints *p;
  vtkPolygon polygon;
  vtkTCoords *t;

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
  polys = polyData->GetPolys();

  t = polyData->GetPointData()->GetTCoords();
  if ( t ) 
    {
    tDim = t->GetDimension();
    if (tDim != 2)
      {
      vtkDebugMacro(<< "Currently only 2d textures are supported.\n");
      t = NULL;
      }
    }

  if ( interpolation == VTK_FLAT || !(polyData->GetPointData()) || 
  !(n=polyData->GetPointData()->GetNormals()) ) n = 0;

  for (polys->InitTraversal(); polys->GetNextCell(npts,pts); )

    { 
    if (!n)
      polygon.ComputeNormal(p,npts,pts,poly_norm);
    
    for (j = 0; j < npts; j++) 
      {
      k = j;
      if (c) 
        {
	colors = c->GetColor (pts[k]);
	vertexColors[k][0] = colors[0] / 255.0;
	vertexColors[k][1] = colors[1] / 255.0;
	vertexColors[k][2] = colors[2] / 255.0;
        }
      if (t)
	{
	TCoords = t->GetTCoord (pts[k]);
	vertexTCoords[k][0] = TCoords[0];
	// Renderman Textures have origin at upper left
	vertexTCoords[k][1] = 1.0 - TCoords[1];
	}
      if (n) 
        {
	normals = n->GetNormal (pts[k]);
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
}

void vtkRIBExporter::WriteStrips (vtkPolyData *polyData, vtkColorScalars *c, vtkProperty *aProperty)
{
  float vertexColors[100][3];
  RtFloat *TCoords;
  RtFloat *normals;
  RtFloat *points;
  RtPoint vertexNormals[100];
  RtPoint vertexPoints[100];
  float poly_norm[3];
  float vertexTCoords[100][2];
  int *pts;
  int p1, p2, p3;
  int k, kk;
  int npts, rep, j, interpolation;
  int tDim;
  unsigned char *colors;
  vtkCellArray *strips;
  vtkNormals *n = NULL;
  vtkPoints *p;
  vtkTCoords *t;
  vtkPolygon polygon;
  int idx[3];

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

  t = polyData->GetPointData()->GetTCoords();
  if ( t ) 
    {
    tDim = t->GetDimension();
    if (tDim != 2)
      {
      vtkDebugMacro(<< "Currently only 2d textures are supported.\n");
      t = NULL;
      }
    }

  if ( interpolation == VTK_FLAT || !(polyData->GetPointData()) || 
  !(n=polyData->GetPointData()->GetNormals()) ) n = 0;


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
	  polygon.ComputeNormal (p, 3, idx, poly_norm);
    
	// build colors, texture coordinates and normals for the triangle
	for (k = 0; k < 3; k++)
	  {
	    if (c) 
	      {
		colors = c->GetColor (idx[k]);
		vertexColors[k][0] = colors[0] / 255.0;
		vertexColors[k][1] = colors[1] / 255.0;
		vertexColors[k][2] = colors[2] / 255.0;
	      }
	    if (t)
	      {
		TCoords = t->GetTCoord (idx[k]);
		vertexTCoords[k][0] = TCoords[0];
		// Renderman Textures have origin at upper left
		vertexTCoords[k][1] = 1.0 - TCoords[1];
	      }
	    if (n) 
	      {
		normals = n->GetNormal (idx[k]);
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
      p3 = pts[3+j];
      }
  }
}

void vtkRIBExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkExporter::PrintSelf(os,indent);
 
  os << indent << "FilePrefix: " << this->FilePrefix << "\n";
  os << indent << "TexturePrefix: " << this->TexturePrefix << "\n";
  os << indent << "Background: " << (this->Background ? "On\n" : "Off\n");
  os << indent << "Size: " << this->Size[0] << " " << this->Size[1] << "\n";
  os << indent << "PixelSamples: " << this->PixelSamples[0] << " " << this->PixelSamples[1] << "\n";
}

void vtkRIBExporter::WriteTexture (vtkTexture *aTexture)
{
    vtkScalars *scalars;
    vtkColorScalars *mappedScalars;
    int *size;
    int xsize, ysize;
    unsigned short xs,ys;

//    RtToken wrap = aTexture->GetRepeat () ? RI_PERIODIC : RI_CLAMP;
//    RiMakeTexture (this->GetTIFFName (aTexture),
//		   this->GetTextureName (aTexture),
//                   wrap, wrap,
//                   RiBoxFilter,
//                   1, 1,
//                   RI_NULL);
    char *wrap = aTexture->GetRepeat () ? "periodic" : "clamp";
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
    scalars = (aTexture->GetInput()->GetPointData())->GetScalars();

    // make sure scalars are non null
    if (!scalars) 
      {
      vtkErrorMacro(<< "No scalar values found for texture input!\n");
      return;
      }

    // make sure using unsigned char data of color scalars type
    if (strcmp(scalars->GetDataType(),"unsigned char") ||
        strcmp(scalars->GetScalarType(),"ColorScalar") )
      {
      mappedScalars = aTexture->GetMappedScalars ();
      }
    else
      {
      mappedScalars = (vtkColorScalars *) scalars;
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

    vtkTIFFWriter *aWriter = new vtkTIFFWriter;
    vtkStructuredPoints *anImage = new vtkStructuredPoints;
      anImage->SetDimensions (xsize, ysize, 1);
      anImage->GetPointData()->SetScalars (mappedScalars);
      aWriter->SetInput (anImage);
      aWriter->SetFileName (GetTIFFName (aTexture));
      aWriter->Write ();
    delete aWriter;
    delete anImage;
}

static char tiffName[4096];
static char textureName[4096];

char *vtkRIBExporter::GetTIFFName (vtkTexture *aTexture)
{
    sprintf (tiffName, "%s_%x_%d.tif", this->TexturePrefix, (int) aTexture, (int) aTexture->GetMTime ());
    return tiffName;
}

char *vtkRIBExporter::GetTextureName (vtkTexture *aTexture)
{
    sprintf (textureName, "%s_%x_%d.txt", this->TexturePrefix, (int) aTexture, (int) aTexture->GetMTime ());
    return textureName;
}

