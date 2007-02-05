/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeMapLayout.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTreeMapLayout.h"

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
#include "vtkTreeMapLayoutStrategy.h"

vtkCxxRevisionMacro(vtkTreeMapLayout, "1.5");
vtkStandardNewMacro(vtkTreeMapLayout);

vtkTreeMapLayout::vtkTreeMapLayout()
{
  this->RectanglesFieldName = 0;
  this->LayoutStrategy = 0;
  this->SetRectanglesFieldName("rectangles");
}

vtkTreeMapLayout::~vtkTreeMapLayout()
{
  this->SetRectanglesFieldName(0);
  if (this->LayoutStrategy)
    {
    this->LayoutStrategy->Delete();
    }
}

vtkCxxSetObjectMacro(vtkTreeMapLayout, LayoutStrategy, vtkTreeMapLayoutStrategy);

int vtkTreeMapLayout::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  if (this->LayoutStrategy == NULL)
    {
    vtkErrorMacro(<< "Layout strategy must me non-null.");
    return 0;
    }
  if (this->RectanglesFieldName == NULL)
    {
    vtkErrorMacro(<< "Rectangles field name must me non-null.");
    return 0;
    }
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
    
  // Storing the inputTree and outputTree handles
  vtkTree* inputTree = vtkTree::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkTree* outputTree = vtkTree::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Copy the input into the output
  outputTree->ShallowCopy(inputTree);
  
  // Add the 4-tuple array that will store the min,max xy coords
  vtkFloatArray *coordsArray = vtkFloatArray::New();
  coordsArray->SetName(this->RectanglesFieldName);
  coordsArray->SetNumberOfComponents(4);
  coordsArray->SetNumberOfTuples(inputTree->GetNumberOfVertices());
  vtkPointData* data = outputTree->GetPointData(); 
  data->AddArray(coordsArray);
  coordsArray->Delete();
  
  // Okay now layout the tree :)
  this->LayoutStrategy->Layout(inputTree, coordsArray);
   
  return 1;
}

void vtkTreeMapLayout::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RectanglesFieldName: " << (this->RectanglesFieldName ? this->RectanglesFieldName : "(none)") << endl;
  os << indent << "LayoutStrategy: " << (this->LayoutStrategy ? "" : "(none)") << endl;
  if (this->LayoutStrategy)
    {
    this->LayoutStrategy->PrintSelf(os, indent.GetNextIndent());
    }
}

vtkIdType vtkTreeMapLayout::FindVertex(float pnt[2], float *binfo)
{

  // Do we have an output?
  vtkTree* otree = this->GetOutput();
  if (!otree) 
    {
    vtkErrorMacro(<< "Could not get output tree.");
    return -1;
    }

  //Get the four tuple array for the points
  vtkDataArray *array = otree->GetPointData()->
    GetArray(this->RectanglesFieldName);
  if (!array)
    {
    // vtkErrorMacro(<< "Output Tree does not have box information.");
    return -1;
    }
  
  // Check to see that we are in the dataset at all
  float blimits[4];
 
  vtkIdType vertex = otree->GetRoot();
  vtkFloatArray *boxInfo = vtkFloatArray::SafeDownCast(array);
  // Now try to find the vertex that contains the point
  boxInfo->GetTupleValue(vertex, blimits); // Get the extents of the root
  if ((pnt[0] < blimits[0]) || (pnt[0] > blimits[1]) ||
      (pnt[1] < blimits[2]) || (pnt[1] > blimits[3]))
    {
    // Point is not in the tree at all
    return -1;
    }
  
  // Now traverse the children to try and find 
  // the vertex that contains the point  
  vtkIdType child;
  if (binfo) 
    {
    binfo[0] = blimits[0];
    binfo[1] = blimits[1];
    binfo[2] = blimits[2];
    binfo[3] = blimits[3];
    }

  vtkIdType n;
  const vtkIdType* children;
  otree->GetChildren(vertex, n, children);
  for (vtkIdType i = 0; i < n; ++i) 
    {
    child = children[i];
    boxInfo->GetTupleValue(child, blimits); // Get the extents of the child
    if ((pnt[0] < blimits[0]) || (pnt[0] > blimits[1]) ||
            (pnt[1] < blimits[2]) || (pnt[1] > blimits[3]))
      {
      continue;
      }
    // If we are here then the point is contained by the child
    // So recurse down the children of this vertex
    vertex = child;
    otree->GetChildren(vertex, n, children);
    i = -1; // Remember that the for loop is going to increment this
            // Note: This is an odd way to do recursion 
    }

  return vertex;
}
 
void vtkTreeMapLayout::GetBoundingBox(vtkIdType id, float *binfo)
{
  // Do we have an output?
  vtkTree* otree = this->GetOutput();
  if (!otree) 
    {
    vtkErrorMacro(<< "Could not get output tree.");
    return;
    }

  //Get the four tuple array for the points
  vtkDataArray *array = otree->GetPointData()->
    GetArray(this->RectanglesFieldName);
  if (!array)
    {
    // vtkErrorMacro(<< "Output Tree does not have box information.");
    return;
    }

  vtkFloatArray *boxInfo = vtkFloatArray::SafeDownCast(array);
  boxInfo->GetTupleValue(id, binfo);
}

unsigned long vtkTreeMapLayout::GetMTime()
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


