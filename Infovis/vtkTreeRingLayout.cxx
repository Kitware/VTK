/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeRingLayout.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkTreeRingLayout.h"

#include "vtkAdjacentVertexIterator.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTree.h"
#include "vtkTreeDFSIterator.h"
#include "vtkTreeRingLayoutStrategy.h"

vtkCxxRevisionMacro(vtkTreeRingLayout, "1.7");
vtkStandardNewMacro(vtkTreeRingLayout);

vtkTreeRingLayout::vtkTreeRingLayout()
{
  this->SectorsFieldName = 0;
  this->LayoutStrategy = 0;
  this->SetSectorsFieldName("sectors");
}

vtkTreeRingLayout::~vtkTreeRingLayout()
{
  this->SetSectorsFieldName(0);
  if (this->LayoutStrategy)
    {
    this->LayoutStrategy->Delete();
    }
}

vtkCxxSetObjectMacro(vtkTreeRingLayout, LayoutStrategy, vtkTreeRingLayoutStrategy);

int vtkTreeRingLayout::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  if (this->LayoutStrategy == NULL)
    {
    vtkErrorMacro(<< "Layout strategy must be non-null.");
    return 0;
    }
  if (this->SectorsFieldName == NULL)
    {
    vtkErrorMacro(<< "Sector field name must be non-null.");
    return 0;
    }
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
    
  // Storing the inputTree and outputTree handles
  vtkTree *inputTree = vtkTree::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkTree *outputTree = vtkTree::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Copy the input into the output
  outputTree->ShallowCopy(inputTree);
  
  // Add the 4-tuple array that will store the min,max xy coords
  vtkFloatArray *coordsArray = vtkFloatArray::New();
  coordsArray->SetName(this->SectorsFieldName);
  coordsArray->SetNumberOfComponents(4);
  coordsArray->SetNumberOfTuples(outputTree->GetNumberOfVertices());
  vtkDataSetAttributes *data = outputTree->GetVertexData(); 
  data->AddArray(coordsArray);
  coordsArray->Delete();
  
  // Okay now layout the tree :)
  this->LayoutStrategy->Layout(outputTree, coordsArray);
   
  return 1;
}

void vtkTreeRingLayout::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SectorsFieldName: " << (this->SectorsFieldName ? this->SectorsFieldName : "(none)") << endl;
  os << indent << "LayoutStrategy: " << (this->LayoutStrategy ? "" : "(none)") << endl;
  if (this->LayoutStrategy)
    {
    this->LayoutStrategy->PrintSelf(os, indent.GetNextIndent());
    }
}

vtkIdType vtkTreeRingLayout::FindVertexRectangular(float pnt[2])
{
  // Do we have an output?
  vtkTree* otree = this->GetOutput();
  if (!otree) 
    {
    vtkErrorMacro(<< "Could not get output tree.");
    return -1;
    }

  //Get the four tuple array for the points
  vtkDataArray *array = otree->GetVertexData()->
    GetArray(this->SectorsFieldName);
  if (!array)
    {
    // vtkErrorMacro(<< "Output Tree does not have box information.");
    return -1;
    }
  
  float blimits[4];
  vtkIdType vertex = otree->GetRoot();
  vtkFloatArray *boundsInfo = vtkFloatArray::SafeDownCast(array);

  // Now try to find the vertex that contains the point
  boundsInfo->GetTupleValue(vertex, blimits); // Get the extents of the root
  if( ((pnt[1] > blimits[2]) && (pnt[1] < blimits[3])) &&
      ((pnt[0] > blimits[0]) && (pnt[0] < blimits[1])) )
  {
      //Point is at the root vertex.
      // but we don't want the root to be pickable, so return -1; 
//    return -1;
    return vertex;
  }
  
  // Now traverse the children to try and find 
  // the vertex that contains the point  
  vtkIdType child;
  vtkTreeDFSIterator *it = vtkTreeDFSIterator::New();
  it->SetTree( otree );
  it->SetStartVertex( vertex );

  while (it->HasNext()) 
    {
    child = it->Next();
    boundsInfo->GetTupleValue(child, blimits); // Get the extents of the child
    bool beyond_radial_bounds = false;
    bool beyond_angle_bounds = false;
    if( (pnt[1] < blimits[2]) || (pnt[1] > blimits[3]))
        beyond_radial_bounds = true;
    if( (pnt[0] < blimits[0]) || (pnt[0] > blimits[1]))
        beyond_angle_bounds = true;
    
    if( beyond_radial_bounds || beyond_angle_bounds )
    {
      continue;
    }
      // If we are here then the point is contained by the child
    it->Delete();
    return child;
    }
  it->Delete();
  return -1;
}

vtkIdType vtkTreeRingLayout::FindVertex(float pnt[2])
{
  float polar_location[2];
  polar_location[0] = sqrt( ( pnt[0] * pnt[0] ) + ( pnt[1] * pnt[1] ) );
  polar_location[1] = vtkMath::DegreesFromRadians( atan2( pnt[1], pnt[0] ) );
  if( polar_location[1] < 0 )
      polar_location[1] += 360.;

  // Do we have an output?
  vtkTree* otree = this->GetOutput();
  if (!otree) 
    {
    vtkErrorMacro(<< "Could not get output tree.");
    return -1;
    }

  //Get the four tuple array for the points
  vtkDataArray *array = otree->GetVertexData()->
    GetArray(this->SectorsFieldName);
  if (!array)
    {
    // vtkErrorMacro(<< "Output Tree does not have box information.");
    return -1;
    }
  
  float blimits[4];
  vtkIdType vertex = otree->GetRoot();
  vtkFloatArray *boundsInfo = vtkFloatArray::SafeDownCast(array);

  // Now try to find the vertex that contains the point
  boundsInfo->GetTupleValue(vertex, blimits); // Get the extents of the root
  if( ((polar_location[0] > blimits[2]) && (polar_location[0] < blimits[3])) &&
      ((polar_location[1] > blimits[0]) && (polar_location[1] < blimits[1])) )
  {
      //Point is at the root vertex.
      // but we don't want the root to be pickable, so return -1; 
    return -1;
//    return vertex;
  }
  
  // Now traverse the children to try and find 
  // the vertex that contains the point  
  vtkIdType child;
  vtkTreeDFSIterator *it = vtkTreeDFSIterator::New();
  it->SetTree( otree );
  it->SetStartVertex( vertex );

  while (it->HasNext()) 
    {
    child = it->Next();
    boundsInfo->GetTupleValue(child, blimits); // Get the extents of the child
    bool beyond_radial_bounds = false;
    bool beyond_angle_bounds = false;
    if( (polar_location[0] < blimits[2]) || (polar_location[0] > blimits[3]))
        beyond_radial_bounds = true;
    if( (polar_location[1] < blimits[0]) || (polar_location[1] > blimits[1]))
        beyond_angle_bounds = true;
    
    if( beyond_radial_bounds || beyond_angle_bounds )
    {
      continue;
    }
      // If we are here then the point is contained by the child
    it->Delete();
    return child;
    }
  it->Delete();
  return -1;
}
 
void vtkTreeRingLayout::GetBoundingSector(vtkIdType id, float *sinfo)
{
  // Do we have an output?
  vtkTree* otree = this->GetOutput();
  if (!otree) 
    {
    vtkErrorMacro(<< "Could not get output tree.");
    return;
    }

  //Get the four tuple array for the points
  vtkDataArray *array = otree->GetVertexData()->
    GetArray(this->SectorsFieldName);
  if (!array)
    {
    // vtkErrorMacro(<< "Output Tree does not have box information.");
    return;
    }

  vtkFloatArray *sectorInfo = vtkFloatArray::SafeDownCast(array);
  sectorInfo->GetTupleValue(id, sinfo);
}

unsigned long vtkTreeRingLayout::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();
  unsigned long time;

  if (this->LayoutStrategy != NULL)
    {
    time = this->LayoutStrategy->GetMTime();
    mTime = (time > mTime ? time : mTime);
    }
  return mTime;
}


