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
#include "vtkObjectFactory.h"
#include "vtkMapper.h"
#include "vtkAbstractVolumeMapper.h"
#include "vtkAbstractMapper3D.h"
#include "vtkProp.h"
#include "vtkLODProp3D.h"
#include "vtkActor.h"
#include "vtkPropCollection.h"
#include "vtkImageActor.h"
#include "vtkProp3DCollection.h"
#include "vtkAssemblyPath.h"
#include "vtkImageData.h"
#include "vtkVolume.h"
#include "vtkRenderer.h"
#include "vtkProperty.h"
#include "vtkCommand.h"
#include "vtkPlanes.h"
#include "vtkPlane.h"
#include "vtkPoints.h"
#include "vtkFrustumExtractor.h"

vtkCxxRevisionMacro(vtkAreaPicker, "1.4");
vtkStandardNewMacro(vtkAreaPicker);

//--------------------------------------------------------------------------
vtkAreaPicker::vtkAreaPicker()
{
  this->FrustumExtractor = vtkFrustumExtractor::New();
  this->Frustum = this->FrustumExtractor->GetFrustum();
  this->Frustum->Register(this);

  this->ClipPoints = this->FrustumExtractor->GetClipPoints();
  this->ClipPoints->Register(this);

  this->Prop3Ds = vtkProp3DCollection::New();
  this->Mapper = NULL;
  this->DataSet = NULL;
}

//--------------------------------------------------------------------------
vtkAreaPicker::~vtkAreaPicker()
{
  this->Prop3Ds->Delete();
  this->ClipPoints->Delete();  
  this->Frustum->Delete();
  this->FrustumExtractor->Delete();
}

//--------------------------------------------------------------------------
// Initialize the picking process.
void vtkAreaPicker::Initialize()
{
  this->vtkAbstractPropPicker::Initialize();
  this->Prop3Ds->RemoveAllItems();
  this->Mapper = NULL;
}

//--------------------------------------------------------------------------
// Does what this class is meant to do.
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

  return this->PickProps(renderer);  
}

//--------------------------------------------------------------------------
//Converts the given screen rectangle into a selection frustum.
//Saves the results in ClipPoints and Frustum.
void vtkAreaPicker::DefineFrustum(double X0, double Y0, double X1, double Y1, 
                                  vtkRenderer *renderer)
{
  double x0 = (X0 < X1) ? X0 : X1;
  double y0 = (Y0 < Y1) ? Y0 : Y1;
  double x1 = (X0 > X1) ? X0 : X1;
  double y1 = (Y0 > Y1) ? Y0 : Y1;

  if (x0 == x1)
    {
    x0 -= 0.5;
    x1 += 0.5;
    }
  if (y0 == y1)
    {
    y0 -= 0.5;
    y1 += 0.5;
    }

  //compute world coordinates of the pick volume 
  double verts[24];
  renderer->SetDisplayPoint(x0, y0, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&verts[0]);

  renderer->SetDisplayPoint(x0, y0, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&verts[3]);

  renderer->SetDisplayPoint(x0, y1, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&verts[6]);

  renderer->SetDisplayPoint(x0, y1, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&verts[9]);

  renderer->SetDisplayPoint(x1, y0, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&verts[12]);

  renderer->SetDisplayPoint(x1, y0, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&verts[15]);

  renderer->SetDisplayPoint(x1, y1, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&verts[18]);
  
  renderer->SetDisplayPoint(x1, y1, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&verts[21]);    
    
  //a pick point is required by vtkAbstractPicker
  //return center for now until a better meaning is desired
  double sum[3] = {0.0,0.0,0.0};
  for (int i = 0; i < 8; i++)
    {
    sum[0] += verts[i*3+0];
    sum[1] += verts[i*3+1];
    sum[2] += verts[i*3+2];
    }
  this->PickPosition[0] = sum[0]/8.0;
  this->PickPosition[1] = sum[1]/8.0;
  this->PickPosition[2] = sum[2]/8.0;

  this->FrustumExtractor->CreateFrustum(verts);
}

//--------------------------------------------------------------------------
//Decides which props are within the frustum.
//Adds each to the prop3d list and fires pick events.
//Remembers the dataset, mapper, and assembly path for the nearest.
int vtkAreaPicker::PickProps(vtkRenderer *renderer)
{
  vtkProp *prop;
  vtkAbstractMapper3D *mapper = NULL;
  int picked=0;
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
        //cerr << "mapper ABFISECT" << endl;
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
        //cerr << "imageA ABFISECT" << endl;
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

//--------------------------------------------------------------------------
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

  //find distance to the corner nearest the near plane for 'closest' prop
  mindist = -VTK_DOUBLE_MAX;
  vtkPlane *plane = this->Frustum->GetPlane(4); //near plane
  int vid;
  for (vid = 0; vid < 8; vid++)
    {
    double dist = plane->EvaluateFunction(verts[vid]);
    if (dist < 0 && dist > mindist)
      {
      mindist = dist;
      }                
    }
  mindist = -mindist;

  //leave the intersection test to the frustum extractor class
  return this->FrustumExtractor->OverallBoundsTest(bounds);
}

//--------------------------------------------------------------------------
void vtkAreaPicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Frustum: " << this->Frustum << "\n";
  os << indent << "ClipPoints: " << this->ClipPoints << "\n";
  os << indent << "Mapper: " << this->Mapper << "\n";
  os << indent << "DataSet: " << this->DataSet << "\n";
}
