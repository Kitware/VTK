/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPicker.cxx
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
#include "vtkPicker.h"
#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkVertex.h"
#include "vtkRenderWindow.h"
#include "vtkAssemblyNode.h"
#include "vtkVolume.h"
#include "vtkObjectFactory.h"
#include "vtkLODProp3D.h"

//-----------------------------------------------------------------------------
vtkPicker* vtkPicker::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPicker");
  if(ret)
    {
    return (vtkPicker*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPicker;
}

// Construct object with initial tolerance of 1/40th of window. There are no
// pick methods and picking is performed from the renderer's actors.
vtkPicker::vtkPicker()
{
  this->Tolerance = 0.025; // 1/40th of the renderer window

  this->MapperPosition[0] = 0.0;
  this->MapperPosition[1] = 0.0;
  this->MapperPosition[2] = 0.0;

  this->Mapper = NULL;
  this->DataSet = NULL;
  this->GlobalTMin = VTK_LARGE_FLOAT;
  this->Actors = vtkActorCollection::New();
  this->Prop3Ds = vtkProp3DCollection::New();
  this->PickedPositions = vtkPoints::New();
  this->Transform = vtkTransform::New();
}

vtkPicker::~vtkPicker()
{
  this->Actors->Delete();
  this->Prop3Ds->Delete();
  this->PickedPositions->Delete();
  this->Transform->Delete();
}

// Update state when prop3D is picked.
void vtkPicker::MarkPicked(vtkAssemblyPath *path, vtkProp3D *prop3D, 
                           vtkAbstractMapper3D *m,
                           float tMin, float mapperPos[3])
{
  int i;
  vtkMapper *mapper;
  vtkVolumeMapper *volumeMapper;

  this->SetPath(path);
  this->GlobalTMin = tMin;

  for (i=0; i < 3; i++) 
    {
    this->MapperPosition[i] = mapperPos[i];
    }
  if ( (mapper=vtkMapper::SafeDownCast(m)) != NULL )
    {
    this->DataSet = mapper->GetInput();
    this->Mapper = mapper;
    }
  else if ( (volumeMapper=vtkVolumeMapper::SafeDownCast(m)) != NULL )
    {
    this->DataSet = volumeMapper->GetInput();
    this->Mapper = volumeMapper;    }
  else
    {
    this->DataSet = NULL;
    }

  // The point has to be transformed back into world coordinates.
  // Note: it is assumed that the transform is in the correct state.
  this->Transform->TransformPoint(mapperPos,this->PickPosition);
  
  // Invoke pick method if one defined - actor goes first
  prop3D->Pick();
  if ( this->PickMethod )
    {
    (*this->PickMethod)(this->PickMethodArg);
    }
}

// Perform pick operation with selection point provided. Normally the 
// first two values for the selection point are x-y pixel coordinate, and
// the third value is =0. Return non-zero if something was successfully picked.
int vtkPicker::Pick(float selectionX, float selectionY, float selectionZ,
                   vtkRenderer *renderer)
{
  int i;
  vtkProp *prop;
  vtkCamera *camera;
  vtkAbstractMapper3D *mapper = NULL;
  float p1World[4], p2World[4], p1Mapper[4], p2Mapper[4];
  int picked=0;
  int *winSize;
  float x, y, t;
  float *viewport;
  float cameraPos[4], cameraFP[4];
  float *displayCoords, *worldCoords;
  double *clipRange;
  float ray[3], rayLength;
  int pickable;
  int LODId;
  float windowLowerLeft[4], windowUpperRight[4];
  float bounds[6], tol;
  float tF, tB;
  float hitPosition[3];
  float cameraDOP[3];
  
  //  Initialize picking process
  this->Initialize();
  this->Renderer = renderer;
  this->SelectionPoint[0] = selectionX;
  this->SelectionPoint[1] = selectionY;
  this->SelectionPoint[2] = selectionZ;

  // Invoke start pick method if defined
  if ( this->StartPickMethod ) 
    {
    (*this->StartPickMethod)(this->StartPickMethodArg);
    }

  if ( renderer == NULL )
    {
    vtkErrorMacro(<<"Must specify renderer!");
    return 0;
    }

  // Get camera focal point and position. Convert to display (screen) 
  // coordinates. We need a depth value for z-buffer.
  //
  camera = renderer->GetActiveCamera();
  camera->GetPosition((float *)cameraPos); cameraPos[3] = 1.0;
  camera->GetFocalPoint((float *)cameraFP); cameraFP[3] = 1.0;

  renderer->SetWorldPoint(cameraFP);
  renderer->WorldToDisplay();
  displayCoords = renderer->GetDisplayPoint();
  selectionZ = displayCoords[2];

  // Convert the selection point into world coordinates.
  //
  renderer->SetDisplayPoint(selectionX, selectionY, selectionZ);
  renderer->DisplayToWorld();
  worldCoords = renderer->GetWorldPoint();
  if ( worldCoords[3] == 0.0 )
    {
    vtkErrorMacro(<<"Bad homogeneous coordinates");
    return 0;
    }
  for (i=0; i < 3; i++) 
    {
    this->PickPosition[i] = worldCoords[i] / worldCoords[3];
    }
  
  //  Compute the ray endpoints.  The ray is along the line running from
  //  the camera position to the selection point, starting where this line
  //  intersects the front clipping plane, and terminating where this
  //  line intersects the back clipping plane.
  for (i=0; i<3; i++)
    {
    ray[i] = this->PickPosition[i] - cameraPos[i];
    }
  for (i=0; i<3; i++)
    {
    cameraDOP[i] = cameraFP[i] - cameraPos[i];
    }

  vtkMath::Normalize(cameraDOP);

  if (( rayLength = vtkMath::Dot(cameraDOP,ray)) == 0.0 ) 
    {
    vtkWarningMacro("Cannot process points");
    return 0;
    } 

  clipRange = camera->GetClippingRange();

  if ( camera->GetParallelProjection() )
    {
    tF = clipRange[0] - rayLength;
    tB = clipRange[1] - rayLength;
    for (i=0; i<3; i++) 
      {
      p1World[i] = this->PickPosition[i] + tF*cameraDOP[i];
      p2World[i] = this->PickPosition[i] + tB*cameraDOP[i];
      }
    }
  else
    {
    tF = clipRange[0] / rayLength;
    tB = clipRange[1] / rayLength;
    for (i=0; i<3; i++) 
      {
      p1World[i] = cameraPos[i] + tF*ray[i];
      p2World[i] = cameraPos[i] + tB*ray[i];
      }
    }
  p1World[3] = p2World[3] = 1.0;

  // Compute the tolerance in world coordinates.  Do this by
  // determining the world coordinates of the diagonal points of the
  // window, computing the width of the window in world coordinates, and 
  // multiplying by the tolerance.
  //
  viewport = renderer->GetViewport();
  winSize = renderer->GetRenderWindow()->GetSize();
  x = winSize[0] * viewport[0];
  y = winSize[1] * viewport[1];
  renderer->SetDisplayPoint(x, y, selectionZ);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(windowLowerLeft);

  x = winSize[0] * viewport[2];
  y = winSize[1] * viewport[3];
  renderer->SetDisplayPoint(x, y, selectionZ);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(windowUpperRight);

  for (tol=0.0,i=0; i<3; i++) 
    {
    tol += (windowUpperRight[i] - windowLowerLeft[i]) *
              (windowUpperRight[i] - windowLowerLeft[i]);
    }
  
  tol = sqrt (tol) * this->Tolerance;

  //  Loop over all props.  Transform ray (defined from position of
  //  camera to selection point) into coordinates of mapper (not
  //  transformed to actors coordinates!  Reduces overall computation!!!).
  //  Note that only vtkProp3D's can be picked by vtkPicker.
  //
  vtkPropCollection *props;
  vtkProp *propCandidate;
  if ( this->PickFromList ) 
    {
    props = this->GetPickList();
    }
  else 
    {
    props = renderer->GetProps();
    }

  vtkActor *actor;
  vtkLODProp3D *prop3D;
  vtkVolume *volume;
  vtkAssemblyPath *path;
  vtkProperty *tempProperty;
  this->Transform->PostMultiply();
  for ( props->InitTraversal(); (prop=props->GetNextProp()); )
    {
    for ( prop->InitPathTraversal(); (path=prop->GetNextPath()); )
      {
      pickable = 0;
      actor = NULL;
      propCandidate = path->GetLastNode()->GetProp();
      if ( propCandidate->GetPickable() && propCandidate->GetVisibility() )
        {
        pickable = 1;
        if ( (actor=vtkActor::SafeDownCast(propCandidate)) != NULL )
          {
          mapper = actor->GetMapper();
          if ( actor->GetProperty()->GetOpacity() <= 0.0 )
            {
            pickable = 0;
            }
          }
        else if ( (prop3D=vtkLODProp3D::SafeDownCast(propCandidate)) != NULL )
          {
		  LODId = prop3D->GetPickLODID();
          mapper = prop3D->GetLODMapper(LODId);

          // if the mapper is a vtkMapper (as opposed to a vtkVolumeMapper), then
          // check the transparency to see if the object is pickable
		  if ( vtkMapper::SafeDownCast(mapper) != NULL)
		    {
		    prop3D->GetLODProperty(LODId, &tempProperty);
            if ( tempProperty->GetOpacity() <= 0.0 )
			  {
              pickable = 0;
              }
            }
          }
        else if ( (volume=vtkVolume::SafeDownCast(propCandidate)) != NULL )
          {
          mapper = volume->GetMapper();
          }
        else
          {
          pickable = 0; //only vtkProp3D's (actors and volumes) can be picked
          }
        }
      //  If actor can be picked, get its composite matrix, invert it, and
      //  use the inverted matrix to transform the ray points into mapper
      //  coordinates. 
      if ( pickable  &&  mapper != NULL )
        {
        vtkMatrix4x4 *LastMatrix = path->GetLastNode()->GetMatrix();
        if (LastMatrix == NULL)
          {
          vtkErrorMacro (<< "Pick: Null matrix.");
          return 0;
          }
        this->Transform->SetMatrix(LastMatrix);
        this->Transform->Push();
        this->Transform->Inverse();

        this->Transform->TransformPoint(p1World,p1Mapper);
        this->Transform->TransformPoint(p2World,p2Mapper);

        for (i=0; i<3; i++) 
          {
          ray[i] = p2Mapper[i] - p1Mapper[i];
          }

        this->Transform->Pop();

        //  Have the ray endpoints in mapper space, now need to compare this
        //  with the mapper bounds to see whether intersection is possible.
        //
        //  Get the bounding box of the modeller.  Note that the tolerance is
        //  added to the bounding box to make sure things on the edge of the
        //  bounding box are picked correctly.
        mapper->GetBounds(bounds);
        bounds[0] -= tol; bounds[1] += tol; 
        bounds[2] -= tol; bounds[3] += tol; 
        bounds[4] -= tol; bounds[5] += tol; 
        if ( vtkCell::HitBBox(bounds, (float *)p1Mapper, ray, hitPosition, t) )
          {
          t = this->IntersectWithLine((float *)p1Mapper, 
                                      (float *)p2Mapper, tol, path, 
                                      (vtkProp3D *)propCandidate, mapper);
          if ( t < VTK_LARGE_FLOAT )
            {
            picked = 1;
            this->Prop3Ds->AddItem((vtkProp3D *)prop);
            this->PickedPositions->InsertNextPoint
              ((1.0 - t)*p1World[0] + t*p2World[0],
               (1.0 - t)*p1World[1] + t*p2World[1],
               (1.0 - t)*p1World[2] + t*p2World[2]);

            // backwards compatibility: also add to this->Actors
            if (actor)
              {
              this->Actors->AddItem(actor);
              }
            }
          }

        }//if visible and pickable not transparent and has mapper
      }//for all parts
    }//for all actors

  // Invoke end pick method if defined
  if ( this->EndPickMethod )
    {
    (*this->EndPickMethod)(this->EndPickMethodArg);
    }

  return picked;
}

// Intersect data with specified ray.
float vtkPicker::IntersectWithLine(float p1[3], float p2[3], 
                                   float vtkNotUsed(tol), 
                                   vtkAssemblyPath *path, 
                                   vtkProp3D *prop3D, 
                                   vtkAbstractMapper3D *mapper)
{
  int i;
  float *center, t, ray[3], rayFactor;

  // Get the data from the modeler
  //
  center = mapper->GetCenter();

  for (i=0; i<3; i++)
    {
    ray[i] = p2[i] - p1[i];
    }
  if (( rayFactor = vtkMath::Dot(ray,ray)) == 0.0 )
    {
    return 2.0;
    }
  //
  // Project the center point onto the ray and determine its parametric value
  //
  t = (ray[0]*(center[0]-p1[0]) + ray[1]*(center[1]-p1[1])
          + ray[2]*(center[2]-p1[2])) / rayFactor;

  if ( t >= 0.0 && t <= 1.0 && t < this->GlobalTMin ) 
    {
    this->MarkPicked(path, prop3D, mapper, t, center);
    }
  return t;
}

// Initialize the picking process.
void vtkPicker::Initialize()
{
  this->vtkAbstractPropPicker::Initialize();

  this->Actors->RemoveAllItems();
  this->Prop3Ds->RemoveAllItems();
  this->PickedPositions->Reset();
  
  this->MapperPosition[0] = 0.0;
  this->MapperPosition[1] = 0.0;
  this->MapperPosition[2] = 0.0;

  this->Mapper = NULL;
  this->GlobalTMin = VTK_LARGE_FLOAT;
}

void vtkPicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkAbstractPropPicker::PrintSelf(os,indent);

  if ( this->DataSet )
    {
    os << indent << "DataSet: " << this->DataSet << "\n";
    }
  else
    {
    os << indent << "DataSet: (none)";
    }

  os << indent << "Mapper: " << this->Mapper << "\n";

  os << indent << "Tolerance: " << this->Tolerance << "\n";

  os << indent << "Mapper Position: (" <<  this->MapperPosition[0] << ","
     << this->MapperPosition[1] << ","
     << this->MapperPosition[2] << ")\n";
}
