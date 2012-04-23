/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkCirclePackLayout.cxx

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

#include "vtkCirclePackLayout.h"

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
#include "vtkCirclePackLayoutStrategy.h"

vtkStandardNewMacro(vtkCirclePackLayout);

vtkCirclePackLayout::vtkCirclePackLayout()
{
    this->CirclesFieldName = 0;
    this->LayoutStrategy = 0;
    this->SetCirclesFieldName("circle");
    this->SetSizeArrayName("size");
}

vtkCirclePackLayout::~vtkCirclePackLayout()
{
    this->SetCirclesFieldName(0);
    if (this->LayoutStrategy)
      {
        this->LayoutStrategy->Delete();
      }
}

vtkCxxSetObjectMacro(vtkCirclePackLayout, LayoutStrategy, vtkCirclePackLayoutStrategy);

int vtkCirclePackLayout::RequestData(
                                  vtkInformation *vtkNotUsed(request),
                                  vtkInformationVector **inputVector,
                                  vtkInformationVector *outputVector)
{
    if (this->LayoutStrategy == NULL)
      {
        vtkErrorMacro(<< "Layout strategy must be non-null.");
        return 0;
      }
    if (this->CirclesFieldName == NULL)
      {
        vtkErrorMacro(<< "Circles field name must be non-null.");
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

    // Add the 3-tuple array that will store the Xcenter, Ycenter, and Radius
    vtkFloatArray *coordsArray = vtkFloatArray::New();
    coordsArray->SetName(this->CirclesFieldName);
    coordsArray->SetNumberOfComponents(3);
    coordsArray->SetNumberOfTuples(inputTree->GetNumberOfVertices());
    vtkDataSetAttributes *data = outputTree->GetVertexData();
    data->AddArray(coordsArray);
    coordsArray->Delete();

    // Check for size array on the input vtkTree.
    vtkDataArray *sizeArray = this->GetInputArrayToProcess(0, inputTree);
    if (!sizeArray)
      {
        vtkErrorMacro("Size array not found.");
        return 0;
      }

    // Find circle packing layout
    this->LayoutStrategy->Layout(inputTree, coordsArray, sizeArray);

    return 1;
}

void vtkCirclePackLayout::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os, indent);
    os << indent << "CirclesFieldName: " << (this->CirclesFieldName ? this->CirclesFieldName : "(none)") << endl;
    os << indent << "LayoutStrategy: " << (this->LayoutStrategy ? "" : "(none)") << endl;
    if (this->LayoutStrategy)
      {
        this->LayoutStrategy->PrintSelf(os, indent.GetNextIndent());
      }
}

vtkIdType vtkCirclePackLayout::FindVertex(float pnt[2], float *cinfo)
{
    // Do we have an output?
    vtkTree* otree = this->GetOutput();
    if (!otree)
      {
        vtkErrorMacro(<< "Could not get output tree.");
        return -1;
      }

    //Get the three tuple array for the points
    vtkDataArray *array = otree->GetVertexData()->
    GetArray(this->CirclesFieldName);
    if (!array)
      {
        vtkErrorMacro(<< "Output Tree does not contain circle packing information.");
        return -1;
      }

    // Check to see that we are in the dataset at all
    float climits[3];

    vtkIdType vertex = otree->GetRoot();
    vtkFloatArray *circleInfo = vtkFloatArray::SafeDownCast(array);
    // Now try to find the vertex that contains the point
    circleInfo->GetTupleValue(vertex, climits); // Get the extents of the root
    if (pow((pnt[0]- climits[0]),2) + pow((pnt[1] - climits[1]),2) > pow(climits[2],2))
      {
        // Point is not in the tree at all
        return -1;
      }

    // Now traverse the children to try and find
    // the vertex that contains the point
    vtkIdType child;
    if (cinfo)
      {
        cinfo[0] = climits[0];
        cinfo[1] = climits[1];
        cinfo[2] = climits[2];
      }

    vtkAdjacentVertexIterator *it = vtkAdjacentVertexIterator::New();
    otree->GetAdjacentVertices(vertex, it);
    while (it->HasNext())
      {
        child = it->Next();
        circleInfo->GetTupleValue(child, climits); // Get the extents of the child
        if (pow((pnt[0]- climits[0]),2) + pow((pnt[1] - climits[1]),2) > pow(climits[2],2))
          {
            continue;
          }
        // If we are here then the point is contained by the child
        // So recurse down the children of this vertex
        vertex = child;
        if (cinfo)
          {
          cinfo[0] = climits[0];
          cinfo[1] = climits[1];
          cinfo[2] = climits[2];
          }
        otree->GetAdjacentVertices(vertex, it);
      }
    it->Delete();

    return vertex;
}

void vtkCirclePackLayout::GetBoundingCircle(vtkIdType id, float *cinfo)
{
    // Do we have an output?
    vtkTree* otree = this->GetOutput();
    if (!otree)
      {
        vtkErrorMacro(<< "Could not get output tree.");
        return;
      }

    if(!cinfo)
      {
      vtkErrorMacro(<< "cinfo is NULL");
      return;
      }

    //Get the three tuple array for the circle
    vtkDataArray *array = otree->GetVertexData()->
    GetArray(this->CirclesFieldName);
    if (!array)
      {
        vtkErrorMacro(<< "Output Tree does not contain circle packing information.");
        return;
      }

    vtkFloatArray *boxInfo = vtkFloatArray::SafeDownCast(array);
    boxInfo->GetTupleValue(id, cinfo);
}

unsigned long vtkCirclePackLayout::GetMTime()
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
