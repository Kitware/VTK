/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSquarifyLayoutStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSquarifyLayoutStrategy.h"

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkMath.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkFloatArray.h>
#include <vtkDataArray.h>

#include "vtkTree.h"

vtkCxxRevisionMacro(vtkSquarifyLayoutStrategy, "1.3");
vtkStandardNewMacro(vtkSquarifyLayoutStrategy);

vtkSquarifyLayoutStrategy::vtkSquarifyLayoutStrategy()
{
  this->SizeFieldName = 0;
  this->SetSizeFieldName("size");
}

vtkSquarifyLayoutStrategy::~vtkSquarifyLayoutStrategy()
{
  this->SetSizeFieldName(0);
}

void vtkSquarifyLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "SizeFieldName: " << (this->SizeFieldName ? this->SizeFieldName : "(none)") << endl;
}

void vtkSquarifyLayoutStrategy::Layout(vtkTree *inputTree, 
  vtkDataArray *coordsArray)
{
  // Get the size array
  vtkDataArray* sizeArray = 
    inputTree->GetVertexData()->GetArray(this->SizeFieldName);

  // Get the root vertex and set it to 0,1,0,1
  vtkIdType rootId = inputTree->GetRoot();
  float coords[] = {0,1,0,1};
  coordsArray->SetTuple(rootId, coords);
  
  // Now layout the children vertices
  vtkIdType nchildren;
  const vtkIdType* children;
  inputTree->GetChildren(rootId, nchildren, children);
  this->AddBorder(coords);
  this->LayoutChildren(inputTree, coordsArray, sizeArray, nchildren, children, 0,
                       coords[0], coords[1], coords[2], coords[3]);
}

void vtkSquarifyLayoutStrategy::LayoutChildren(
  vtkTree *tree, 
  vtkDataArray *coordsArray,
  vtkDataArray *sizeArray,
  vtkIdType nchildren,
  const vtkIdType* children,
  vtkIdType begin, 
  float minX, float maxX, 
  float minY, float maxY)
{
  float sizeX = maxX - minX;
  float sizeY = maxY - minY;
  if ((sizeX == 0.0) || (sizeY == 0.0)) 
    {
    vtkErrorMacro(<< "Invalid Box Sizes for Vertex: "
                  << children[begin] << " ("
                  << sizeX << ", " << sizeY << ")");
    return;
    }
  bool vertical = (sizeX < sizeY);
  float total = 0;
  for (vtkIdType i = begin; i < nchildren; i++)
    {
    total += static_cast<float>(sizeArray->GetTuple1(children[i]));
    }
  float factor = (sizeX * sizeY) / total;
  
  vtkIdType cur = begin;
  float oldRowError = VTK_FLOAT_MAX;
  float rowError = VTK_FLOAT_MAX;
  float oldCurTotal = 0;
  float curTotal = 0;
  while (rowError <= oldRowError && cur < nchildren)
    {
    oldCurTotal = curTotal;
    curTotal += factor * static_cast<float>(sizeArray->GetTuple1(children[cur]));

    oldRowError = rowError;
    // Compute the new row error
    rowError = 0;
    float width = vertical ? (curTotal / sizeX) : (curTotal / sizeY);
    for (vtkIdType i = begin; i <= cur; i++)
      {
      float curHeight = factor * static_cast<float>(sizeArray->GetTuple1(children[i])) / width;
      float ratio1 = curHeight / width;
      float ratio2 = width / curHeight;
      float curError = (ratio1 > ratio2) ? ratio1 : ratio2;
      if (curError > rowError)
        {
        rowError = curError;
        }
      }
    cur++;
    }
  /*
  if (rowError <= oldRowError)
    {
    cur = children->GetNumberOfIds();
    }
    */
  if (rowError > oldRowError)
  //else
    {
    cur--;
    curTotal = oldCurTotal;
    }

  float coords[4];
  float rowMinX = minX;
  float rowMinY = minY;
  float rowMaxX;
  float rowMaxY = maxY;
  if (vertical)
    {
    rowMaxX = maxX;
    rowMinY = rowMaxY - curTotal / sizeX;
    }
  else
    {
    rowMaxX = rowMinX + curTotal / sizeY;
    rowMinY = minY;
    }
  float part = 0;
  float oldPosition = 0;
  float position = 0;
  for (vtkIdType j = begin; j < cur; j++)
    {
    int id = children[j];
    part += factor * static_cast<float>(sizeArray->GetTuple1(id));
    oldPosition = position;
    // Give children their positions
    if (vertical)
      {
      if (curTotal == 0)
        {
        position = 0;
        }
      else
        {
        position = sizeX * (part / curTotal);
        }
      coords[0] = rowMinX + oldPosition;     // minX
      coords[1] = rowMinX + position;        // maxX
      coords[2] = rowMinY;                   // minY
      coords[3] = rowMaxY;                   // maxY
      }
    else
      {
      if (curTotal == 0)
        {
        position = 0;
        }
      else
        {
        position = sizeY * (part / curTotal);
        }
      coords[0] = rowMinX;                   // minX
      coords[1] = rowMaxX;                   // maxX
      coords[2] = rowMaxY - position;     // minY
      coords[3] = rowMaxY - oldPosition;        // maxY
      }

    coordsArray->SetTuple(id, coords);
    vtkIdType numNewChildren;
    const vtkIdType* newChildren;
    tree->GetChildren(id, numNewChildren, newChildren);
    if (numNewChildren > 0)
      {
      this->AddBorder(coords);
      this->LayoutChildren(tree, coordsArray, sizeArray, numNewChildren, newChildren, 0,
        coords[0], coords[1], coords[2], coords[3]);
      }
    }

  if (cur < nchildren)
    {
    float restMinX;
    float restMaxX;
    float restMinY;
    float restMaxY;
    if (vertical)
      {
      restMinY = minY;
      restMaxY = rowMinY;
      restMinX = rowMinX;
      restMaxX = rowMaxX;
      }
    else
      {
      restMinX = rowMaxX;
      restMaxX = maxX;
      restMinY = rowMinY;
      restMaxY = rowMaxY;
      }
    this->LayoutChildren(tree, coordsArray, sizeArray, nchildren, children, cur,
      restMinX, restMaxX, restMinY, restMaxY);
    }
}
