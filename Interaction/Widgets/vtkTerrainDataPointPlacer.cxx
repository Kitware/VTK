/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTerrainDataPointPlacer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTerrainDataPointPlacer.h"

#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkProp.h"
#include "vtkPropCollection.h"
#include "vtkPropPicker.h"
#include "vtkAssemblyPath.h"
#include "vtkAssemblyNode.h"

vtkStandardNewMacro(vtkTerrainDataPointPlacer);

//----------------------------------------------------------------------
vtkTerrainDataPointPlacer::vtkTerrainDataPointPlacer()
{
  this->TerrainProps    = vtkPropCollection::New();
  this->PropPicker      = vtkPropPicker::New();
  this->PropPicker->PickFromListOn();

  this->HeightOffset    = 0.0;
}

//----------------------------------------------------------------------
vtkTerrainDataPointPlacer::~vtkTerrainDataPointPlacer()
{
  this->TerrainProps->Delete();
  this->PropPicker->Delete();
}

//----------------------------------------------------------------------
void vtkTerrainDataPointPlacer::AddProp(vtkProp *prop)
{
  this->TerrainProps->AddItem(prop);
  this->PropPicker->AddPickList(prop);
}

//----------------------------------------------------------------------
void vtkTerrainDataPointPlacer::RemoveAllProps()
{
  this->TerrainProps->RemoveAllItems();
  this->PropPicker->InitializePickList(); // clear the pick list.. remove
                                          // old props from it...
}

//----------------------------------------------------------------------
int vtkTerrainDataPointPlacer::ComputeWorldPosition( vtkRenderer *ren,
                                        double  displayPos[2],
                                        double *vtkNotUsed(refWorldPos),
                                        double  worldPos[3],
                                        double  worldOrient[9] )
{
  return this->ComputeWorldPosition(ren, displayPos, worldPos, worldOrient);
}

//----------------------------------------------------------------------
int vtkTerrainDataPointPlacer::ComputeWorldPosition( vtkRenderer *ren,
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
      this->TerrainProps->InitTraversal(sit);

      while (vtkProp *p = this->TerrainProps->GetNextProp(sit))
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
          worldPos[2] += this->HeightOffset;
          return 1;
        }
      }
    }
  }

  return 0;
}

//----------------------------------------------------------------------
int vtkTerrainDataPointPlacer::ValidateWorldPosition( double worldPos[3],
                                           double *vtkNotUsed(worldOrient) )
{
  return this->ValidateWorldPosition( worldPos );
}

//----------------------------------------------------------------------
int vtkTerrainDataPointPlacer::ValidateWorldPosition(
                     double vtkNotUsed(worldPos)[3] )
{
  return 1;
}

//----------------------------------------------------------------------
int vtkTerrainDataPointPlacer::ValidateDisplayPosition( vtkRenderer *,
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
void vtkTerrainDataPointPlacer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PropPicker: " << this->PropPicker << endl;
  if (this->PropPicker)
  {
    this->PropPicker->PrintSelf(os, indent.GetNextIndent());
  }

  os << indent << "TerrainProps: " << this->TerrainProps << endl;
  if (this->TerrainProps)
  {
    this->TerrainProps->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "HeightOffset: " << this->HeightOffset << endl;
}

