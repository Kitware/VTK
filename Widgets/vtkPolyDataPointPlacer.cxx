/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataPointPlacer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyDataPointPlacer.h"

#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkProp.h"
#include "vtkPropCollection.h"
#include "vtkPropPicker.h"
#include "vtkAssemblyPath.h"
#include "vtkAssemblyNode.h"
#include "vtkInteractorObserver.h"

vtkStandardNewMacro(vtkPolyDataPointPlacer);

//----------------------------------------------------------------------
vtkPolyDataPointPlacer::vtkPolyDataPointPlacer()
{
  this->SurfaceProps    = vtkPropCollection::New();
  this->PropPicker      = vtkPropPicker::New();
  this->PropPicker->PickFromListOn();
}

//----------------------------------------------------------------------
vtkPolyDataPointPlacer::~vtkPolyDataPointPlacer()
{
  this->SurfaceProps->Delete();
  this->PropPicker->Delete();
}

//----------------------------------------------------------------------
void vtkPolyDataPointPlacer::AddProp(vtkProp *prop)
{
  this->SurfaceProps->AddItem(prop);
  this->PropPicker->AddPickList(prop);
}

//----------------------------------------------------------------------
void vtkPolyDataPointPlacer::RemoveViewProp(vtkProp *prop)
{
  this->SurfaceProps->RemoveItem( prop );
  this->PropPicker->DeletePickList( prop );
}

//----------------------------------------------------------------------
void vtkPolyDataPointPlacer::RemoveAllProps()
{
  this->SurfaceProps->RemoveAllItems();
  this->PropPicker->InitializePickList(); // clear the pick list.. remove
                                          // old props from it...
}

//----------------------------------------------------------------------
int vtkPolyDataPointPlacer::HasProp(vtkProp *prop)
{
  return this->SurfaceProps->IsItemPresent(prop);
}

//----------------------------------------------------------------------
int vtkPolyDataPointPlacer::GetNumberOfProps()
{
  return this->SurfaceProps->GetNumberOfItems();
}

//----------------------------------------------------------------------
int vtkPolyDataPointPlacer::ComputeWorldPosition( vtkRenderer *ren,
                                        double  displayPos[2],
                                        double *vtkNotUsed(refWorldPos),
                                        double  worldPos[3],
                                        double  worldOrient[9] )
{
  return this->ComputeWorldPosition(ren, displayPos, worldPos, worldOrient);
}

//----------------------------------------------------------------------
int vtkPolyDataPointPlacer::ComputeWorldPosition( vtkRenderer *ren,
                                      double displayPos[2],
                                      double worldPos[3],
                                      double vtkNotUsed(worldOrient)[9] )
{
  if ( this->PropPicker->Pick(displayPos[0], 
                              displayPos[1], 0.0, ren) )
    {
    if (vtkAssemblyPath *path = this->PropPicker->GetPath())
      {

      // We are checking if the prop present in the path is present
      // in the list supplied to us.. If it is, that prop will be picked.
      // If not, no prop will be picked.

      bool found = false;
      vtkAssemblyNode *node = NULL;
      vtkCollectionSimpleIterator sit;
      this->SurfaceProps->InitTraversal(sit);
      
      while (vtkProp *p = this->SurfaceProps->GetNextProp(sit))
        {
        vtkCollectionSimpleIterator psit;
        path->InitTraversal(psit);

        for ( int i = 0; i < path->GetNumberOfItems() && !found ; ++i )
          {
          node = path->GetNextNode(psit);
          found = ( node->GetViewProp() == p );
          }

        if (found)
          {
          this->PropPicker->GetPickPosition(worldPos);
          
          // Raise height by 0.01 ... this should be a method..
          double displyPos[3];
          vtkInteractorObserver::ComputeWorldToDisplay(ren,
              worldPos[0], worldPos[1], worldPos[2], displyPos);
          displyPos[2] -= 0.01;
          double w[4];
          vtkInteractorObserver::ComputeDisplayToWorld(ren,
              displyPos[0], displyPos[1], displyPos[2], w);
          worldPos[0] = w[0];
          worldPos[1] = w[1];
          worldPos[2] = w[2];

          return 1;
          }
        }
      }
    }
    
  return 0;
}

//----------------------------------------------------------------------
int vtkPolyDataPointPlacer::ValidateWorldPosition( double worldPos[3],
                                           double *vtkNotUsed(worldOrient) )
{
  return this->ValidateWorldPosition( worldPos );
}

//----------------------------------------------------------------------
int vtkPolyDataPointPlacer::ValidateWorldPosition( 
                     double vtkNotUsed(worldPos)[3] )
{
  return 1;
}

//----------------------------------------------------------------------
int vtkPolyDataPointPlacer::ValidateDisplayPosition( vtkRenderer *, 
                                      double vtkNotUsed(displayPos)[2] )
{
  // We could check here to ensure that the display point picks one of the
  // terrain props, but the contour representation always calls 
  // ComputeWorldPosition followed by 
  // ValidateDisplayPosition/ValidateWorldPosition when it needs to 
  // update a node... 
  //
  // So that would be wasting CPU cycles to perform
  // the same check twice..  Just return 1 here.

  return 1;
}

//----------------------------------------------------------------------
void vtkPolyDataPointPlacer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PropPicker: " << this->PropPicker << endl;
  if (this->PropPicker)
    {
    this->PropPicker->PrintSelf(os, indent.GetNextIndent());
    }

  os << indent << "SurfaceProps: " << this->SurfaceProps << endl;
  if (this->SurfaceProps)
    {
    this->SurfaceProps->PrintSelf(os, indent.GetNextIndent());
    }
}

