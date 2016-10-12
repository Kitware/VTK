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
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTree.h"
#include "vtkCirclePackLayoutStrategy.h"
#include "vtkTreeDFSIterator.h"

vtkStandardNewMacro(vtkCirclePackLayout);

vtkCirclePackLayout::vtkCirclePackLayout()
{
    this->CirclesFieldName = 0;
    this->LayoutStrategy = 0;
    this->SetCirclesFieldName("circles");
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


void vtkCirclePackLayout::prepareSizeArray(vtkDoubleArray* mySizeArray,
                                           vtkTree* tree)
{
  vtkTreeDFSIterator* dfs = vtkTreeDFSIterator::New();
  dfs->SetMode(vtkTreeDFSIterator::FINISH);
  dfs->SetTree(tree);

  double currentLeafSize = 0.0;
  while(dfs->HasNext())
  {
    vtkIdType vertex = dfs->Next();

    if(tree->IsLeaf(vertex))
    {
      double size = mySizeArray->GetValue(vertex);
      if(size == 0.0)
      {
        size = 1.0;
        mySizeArray->SetValue(vertex, size);
      }
      currentLeafSize += size;
    }
    else
    {
      mySizeArray->SetValue(vertex, currentLeafSize);
    }
  }

  dfs->Delete();
}

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

    // Check for size array on the input vtkTree.
    vtkDataArray *sizeArray = this->GetInputArrayToProcess(0, inputTree);
    vtkDoubleArray *mySizeArray = vtkDoubleArray::New();
    if(sizeArray)
    {
      mySizeArray->DeepCopy(sizeArray);
    }
    else
    {
      mySizeArray->FillComponent(0,0.0);
      mySizeArray->SetNumberOfTuples(inputTree->GetNumberOfVertices());
    }

    this->prepareSizeArray(mySizeArray, inputTree);

    // Copy the input into the output
    outputTree->ShallowCopy(inputTree);

    // Add the 3-tuple array that will store the Xcenter, Ycenter, and Radius
    vtkDoubleArray *coordsArray = vtkDoubleArray::New();
    coordsArray->SetName(this->CirclesFieldName);
    coordsArray->SetNumberOfComponents(3);
    coordsArray->SetNumberOfTuples(inputTree->GetNumberOfVertices());
    vtkDataSetAttributes *data = outputTree->GetVertexData();
    data->AddArray(coordsArray);
    coordsArray->Delete();

    // Find circle packing layout
    this->LayoutStrategy->Layout(inputTree, coordsArray, mySizeArray);

    mySizeArray->Delete();

    // Copy the coordinates from the layout into the Points field
    vtkPoints* points = outputTree->GetPoints();
    points->SetNumberOfPoints(coordsArray->GetNumberOfTuples());
    for (int i = 0; i < coordsArray->GetNumberOfTuples(); ++i)
    {
      double where[3];
      coordsArray->GetTuple(i, where);
      where[2] = 0;
      points->SetPoint(i, where);
    }
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

vtkIdType vtkCirclePackLayout::FindVertex(double pnt[2], double *cinfo)
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
    double climits[3];

    vtkIdType vertex = otree->GetRoot();
    vtkDoubleArray *circleInfo = vtkArrayDownCast<vtkDoubleArray>(array);
    // Now try to find the vertex that contains the point
    circleInfo->GetTypedTuple(vertex, climits); // Get the extents of the root
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
        circleInfo->GetTypedTuple(child, climits); // Get the extents of the child
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

void vtkCirclePackLayout::GetBoundingCircle(vtkIdType id, double *cinfo)
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

    vtkDoubleArray *boxInfo = vtkArrayDownCast<vtkDoubleArray>(array);
    boxInfo->GetTypedTuple(id, cinfo);
}

vtkMTimeType vtkCirclePackLayout::GetMTime()
{
    vtkMTimeType mTime = this->Superclass::GetMTime();
    vtkMTimeType time;

    if (this->LayoutStrategy != NULL)
    {
        time = this->LayoutStrategy->GetMTime();
        mTime = (time > mTime ? time : mTime);
    }
    return mTime;
}
