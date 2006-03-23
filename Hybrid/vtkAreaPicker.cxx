/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAreaPicker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAreaPicker.h"

#include "vtkGenericCell.h"
#include "vtkImageData.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkAbstractVolumeMapper.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderer.h"

#include "vtkPlanes.h"
#include "vtkExtractGeometry.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkDoubleArray.h"
#include "vtkLandmarkTransform.h"
#include "vtkUnstructuredGrid.h"
#include "vtkRenderWindow.h"
#include "vtkCamera.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"

#include "vtkCamera.h"
#include "vtkAbstractMapper3D.h"
#include "vtkPropCollection.h"
#include "vtkProp.h"
#include "vtkActor.h"
#include "vtkLODProp3D.h"
#include "vtkVolume.h"
#include "vtkImageActor.h"
#include "vtkAssemblyPath.h"
#include "vtkProperty.h"
#include "vtkCommand.h"
#include "vtkPlane.h"
#include "vtkProp3DCollection.h"

vtkCxxRevisionMacro(vtkAreaPicker, "1.1");
vtkStandardNewMacro(vtkAreaPicker);

vtkAreaPicker::vtkAreaPicker()
{
  this->ClipPoints = vtkPoints::New();
  this->ClipPoints->SetNumberOfPoints(8);

  //invert Z because camera coordinate system is left handed
  this->ClipPoints->SetPoint(0, 0.0, 0.0, 1.0);
  this->ClipPoints->SetPoint(1, 0.0, 0.0, 0.0);
  this->ClipPoints->SetPoint(2, 0.0, 1.0, 1.0);
  this->ClipPoints->SetPoint(3, 0.0, 1.0, 0.0);
  this->ClipPoints->SetPoint(4, 1.0, 0.0, 1.0);
  this->ClipPoints->SetPoint(5, 1.0, 0.0, 0.0);
  this->ClipPoints->SetPoint(6, 1.0, 1.0, 1.0);
  this->ClipPoints->SetPoint(7, 1.0, 1.0, 0.0);

  this->Pts = vtkPoints::New();
  this->Pts->SetNumberOfPoints(6);
  this->Norms = vtkDoubleArray::New();
  this->Norms->SetNumberOfComponents(3);
  this->Norms->SetNumberOfTuples(6);

  this->Planes = vtkPlanes::New();
  //near
  this->ComputePlane(0, 6, 2, 0);
  //far
  this->ComputePlane(1, 1, 3, 7);
  //left
  this->ComputePlane(2, 0, 2, 3);
  //right
  this->ComputePlane(3, 7, 6, 4);
  //bottom
  this->ComputePlane(4, 5, 4, 0);
  //top
  this->ComputePlane(5, 2, 6, 7);
  this->Planes->SetPoints(this->Pts);
  this->Planes->SetNormals(this->Norms);
  this->Planes->Modified();

  this->Prop3Ds = vtkProp3DCollection::New();
  this->Mapper = NULL;
  this->DataSet = NULL;
}

vtkAreaPicker::~vtkAreaPicker()
{
  this->Prop3Ds->Delete();
  this->Planes->Delete();
  this->Norms->Delete();
  this->Pts->Delete();
  this->ClipPoints->Delete();
}

// Initialize the picking process.
void vtkAreaPicker::Initialize()
{
  this->vtkAbstractPropPicker::Initialize();

  this->Prop3Ds->RemoveAllItems();
  this->Mapper = NULL;
}

// Does what this class is for.
int vtkAreaPicker::AreaPick(double x0, double y0, double x1, double y1, 
                            vtkRenderer *renderer)
{
  this->Initialize();
  this->Renderer = renderer;

  this->SelectionPoint[0] = (x0+x1)*0.5;
  this->SelectionPoint[1] = (x0+x1)*0.5;
  this->SelectionPoint[2] = 0.0;

  this->InvokeEvent(vtkCommand::StartPickEvent,NULL);

  if ( renderer == NULL )
    {
    vtkErrorMacro(<<"Must specify renderer!");
    return 0;
    }

  this->DefineFrustum(x0, y0, x1, y1, renderer);

  return this->PickProps(x0, y0, x1, y1, renderer);  
}

//Converts the given screen rectangle into a selection frustum.
//Saves the results in ClipPoints and Planes.
void vtkAreaPicker::DefineFrustum(double x0, double y0, double x1, double y1, 
                                  vtkRenderer *renderer)
{
  //compute world coordinates of the pick volume ----------------------------
  //left/right=0/1,bottom/top=0/1,near/far=0/1
  double wp000[3];
  double wp001[3];
  double wp010[3];
  double wp011[3];
  double wp100[3];
  double wp101[3];
  double wp110[3];
  double wp111[3];

  renderer->SetDisplayPoint(x0, y0, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(wp000);

  renderer->SetDisplayPoint(x0, y0, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(wp001);

  renderer->SetDisplayPoint(x0, y1, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(wp010);

  renderer->SetDisplayPoint(x0, y1, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(wp011);

  renderer->SetDisplayPoint(x1, y0, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(wp100);

  renderer->SetDisplayPoint(x1, y0, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(wp101);

  renderer->SetDisplayPoint(x1, y1, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(wp110);
  
  renderer->SetDisplayPoint(x1, y1, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(wp111);
    
    
  //save the points ----------------------------------------------------------
  this->ClipPoints->SetPoint(0, wp000);
  this->ClipPoints->SetPoint(1, wp001);
  this->ClipPoints->SetPoint(2, wp010);
  this->ClipPoints->SetPoint(3, wp011);
  this->ClipPoints->SetPoint(4, wp100);
  this->ClipPoints->SetPoint(5, wp101);
  this->ClipPoints->SetPoint(6, wp110);
  this->ClipPoints->SetPoint(7, wp111);
  this->ClipPoints->Modified();

  //return a pick point because it is required by vtkAbstractPicker
  //simply return centroid for now unless a better meaning is desired
  double tmp[3];
  double sum[3] = {0.0,0.0,0.0};
  for (int i = 0; i < 8; i++)
    {
    this->ClipPoints->GetPoint(i, tmp);
    sum[0] += tmp[0];
    sum[1] += tmp[1];
    sum[2] += tmp[2];
    }
  this->PickPosition[0] = sum[0]/8.0;
  this->PickPosition[1] = sum[1]/8.0;
  this->PickPosition[2] = sum[2]/8.0;

  //update the implicit function with six planes defined by the points ------
  //planes lie on each side of the frustum and point outward
  //for every plane, evaluating a world point inside the frustum comes out < 0
  //near
  this->ComputePlane(0, 6, 2, 0);
  //far
  this->ComputePlane(1, 1, 3, 7);
  //left
  this->ComputePlane(2, 0, 2, 3);
  //right
  this->ComputePlane(3, 7, 6, 4);
  //bottom
  this->ComputePlane(4, 5, 4, 0);
  //top
  this->ComputePlane(5, 2, 6, 7);
  this->Planes->Modified();
}

//Decides which props are within the frustum.
//Adds each to the prop3d list and fires pick events.
//Remembers the dataset, mapper, and assembly path for the nearest.
int vtkAreaPicker::PickProps(double x0, double y0, double x1, double y1, 
                              vtkRenderer *renderer)
{
  int i;
  vtkProp *prop;
  vtkAbstractMapper3D *mapper = NULL;
  int picked=0;
  double x, y, t;
  int pickable;
  int LODId;
  double bounds[6];
  
  //  Initialize picking process
  this->Initialize();
  this->Renderer = renderer;
 
  // Invoke start pick method if defined
  this->InvokeEvent(vtkCommand::StartPickEvent,NULL);

  if ( renderer == NULL )
    {
    vtkErrorMacro(<<"Must specify renderer!");
    return 0;
    }

  //  Loop over all props.  
  //
  vtkPropCollection *props;
  vtkProp *propCandidate;
  if ( this->PickFromList ) 
    {
    props = this->GetPickList();
    }
  else 
    {
    props = renderer->GetViewProps();
    }

  vtkActor *actor;
  vtkLODProp3D *prop3D;
  vtkVolume *volume;
  vtkImageActor *imageActor = 0;
  vtkAssemblyPath *path;
  vtkProperty *tempProperty;

  double mindist = VTK_DOUBLE_MAX;

  vtkCollectionSimpleIterator pit;
  double scale[3];
  for ( props->InitTraversal(pit); (prop=props->GetNextProp(pit)); )
    {
    for ( prop->InitPathTraversal(); (path=prop->GetNextPath()); )
      {
      pickable = 0;
      actor = NULL;
      propCandidate = path->GetLastNode()->GetViewProp();
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

          // if the mapper is a vtkMapper (as opposed to a vtkVolumeMapper), 
          // then check the transparency to see if the object is pickable
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
        else if ( (imageActor=vtkImageActor::SafeDownCast(propCandidate)) )
          {
          mapper = 0;
          }
        else 
          {
          pickable = 0; //only vtkProp3D's (actors and volumes) can be picked
          }
        }

      //  If actor can be picked, see if it is within the pick frustum.
      if ( pickable  && mapper != NULL )
        {
        mapper->GetBounds(bounds);   
        double dist;
        if (this->ABoxFrustumIsect(bounds, dist))
          {
          picked = 1;
          if ( ! this->Prop3Ds->IsItemPresent(prop) )
            {
            this->Prop3Ds->AddItem((vtkProp3D *)prop);
            if (dist < mindist) //new nearest, remember it
              {
              mindist = dist;
              this->SetPath(path);
              this->Mapper = mapper; 
              vtkMapper *map1;
              vtkAbstractVolumeMapper *vmap;
              if ( (map1=vtkMapper::SafeDownCast(mapper)) != NULL )
                {
                this->DataSet = map1->GetInput();
                this->Mapper = map1;
                }
              else if ( (vmap=vtkAbstractVolumeMapper::SafeDownCast(mapper)) != NULL )
                {
                this->DataSet = vmap->GetDataSetInput();
                this->Mapper = vmap;
                }
              else
                {
                this->DataSet = NULL;
                }              
              }
            ((vtkProp3D *)propCandidate)->Pick();
            this->InvokeEvent(vtkCommand::PickEvent,NULL);
            }
          }
        }
      else if ( pickable && imageActor )
        {
        imageActor->GetBounds(bounds);
        double dist;
        if (this->ABoxFrustumIsect(bounds, dist))
          {
          picked = 1;          
          if ( ! this->Prop3Ds->IsItemPresent(prop) )
            {
            this->Prop3Ds->AddItem(imageActor);
            if (dist < mindist) //new nearest, remember it
              {
              mindist = dist;
              this->SetPath(path);
              this->Mapper = mapper; // mapper is null
              this->DataSet = imageActor->GetInput();
              }
            imageActor->Pick();
            this->InvokeEvent(vtkCommand::PickEvent,NULL);          
            }
          }
        }

      }//for all parts
    }//for all actors

  // Invoke end pick method if defined
  this->InvokeEvent(vtkCommand::EndPickEvent,NULL);

  return picked;
}

//Intersect the bbox represented by the bounds with the clipping frustum.
//Return true if partially inside.
//Also return a distance to the near plane.
int vtkAreaPicker::ABoxFrustumIsect(double *bounds, double &mindist)
{
  if (bounds[0] > bounds[1] ||
      bounds[2] > bounds[3] ||
      bounds[4] > bounds[5]) 
    {
    return 0;
    }
    
  //convert bounds to 8 vertices
  double verts[8][3];
  int x, y, z, id;
  id = 0;
  for (x = 0; x < 2; x++)
    {
    for (y = 0; y < 2; y++)
      {
      for (z = 0; z < 2; z++)
        {
        verts[id][0] = bounds[0+x];
        verts[id][1] = bounds[2+y];
        verts[id][2] = bounds[4+z];
        id++;
        }
      }
    }

  //do box/frustum intersect, reject only if all verts are behind any one plane
  mindist = -VTK_DOUBLE_MAX;
  int pid;
  vtkPlane *plane;
  for (pid = 0; pid < 6; pid++)
    {
    plane = this->Planes->GetPlane(pid);
    int allbehind = true;
    for (id = 0; id < 8; id++)
      {
      double dist = plane->EvaluateFunction(verts[id]);
      if (dist < 0)
        {
        //at least part of the bbox is in front of this plane
        allbehind = false; 
        if (pid == 0)
          {
          //for near plane, evaluate all verts
          //so that we can get a nearest prop ranking out of distance to front
          if (dist > mindist) //closer to 0 = on the plane 
            {
            mindist = dist;
            }                
          }
        else
          {
          //for other planes, if any vert is in front, jump to next plane
          break;
          }
        }
      }
    if (allbehind)
      {
      //the entire bbox is behind this plane, reject the prop
      mindist = -1.0; // a negative distance to signify and error
      return 0;
      }
    }

  //switch from operating behind (inside) plane to a distance from plane
  mindist = -mindist; 

  //this prop is picked
  return 1;
}

//Takes indices to three points in the ClipPoints list, saves the resulting 
//plane in the Plane list.
void vtkAreaPicker::ComputePlane(int idx, int p0, int p1, int p2)
{
  double v0[3];
  this->ClipPoints->GetPoint(p0, v0);
  double v1[3];
  this->ClipPoints->GetPoint(p1, v1);
  double v2[3];  
  this->ClipPoints->GetPoint(p2, v2);

  this->Pts->SetPoint(idx, v0[0], v0[1], v0[2]);

  double e0[3];
  e0[0] = v1[0]-v0[0];
  e0[1] = v1[1]-v0[1];
  e0[2] = v1[2]-v0[2];

  double e1[3];
  e1[0] = v2[0]-v0[0];
  e1[1] = v2[1]-v0[1];
  e1[2] = v2[2]-v0[2];

  double n[3];
  vtkMath::Cross(e0,e1,n);
  vtkMath::Normalize(n);

  this->Norms->SetTuple(idx, n);
}

void vtkAreaPicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Planes: " << this->Planes << "\n";
  os << indent << "ClipPoints: " << this->ClipPoints << "\n";
  os << indent << "Mapper: " << this->Mapper << "\n";
  os << indent << "DataSet: " << this->DataSet << "\n";
}
