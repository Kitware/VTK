/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSliceAndDiceLayoutStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkSliceAndDiceLayoutStrategy.h"
#include "vtkTreeDFSIterator.h"

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

vtkCxxRevisionMacro(vtkSliceAndDiceLayoutStrategy, "1.4");
vtkStandardNewMacro(vtkSliceAndDiceLayoutStrategy);

vtkSliceAndDiceLayoutStrategy::vtkSliceAndDiceLayoutStrategy()
{
  this->SizeFieldName = 0;
  this->SetSizeFieldName("size");
}

vtkSliceAndDiceLayoutStrategy::~vtkSliceAndDiceLayoutStrategy()
{
  this->SetSizeFieldName(0);
}

void vtkSliceAndDiceLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "SizeFieldName: " << (this->SizeFieldName ? this->SizeFieldName : "(none)") << endl;
}

// Alternating tree layout method
void vtkSliceAndDiceLayoutStrategy::Layout(vtkTree *inputTree, 
  vtkDataArray *coordsArray)
{
  // Get the size array
  vtkDataArray* sizeArray = inputTree->GetVertexData()->GetArray(this->SizeFieldName);

  vtkTreeDFSIterator* dfs = vtkTreeDFSIterator::New();
  dfs->SetTree(inputTree);
  float coords[4];
  vtkIdType nchildren;
  const vtkIdType* children;
  while (dfs->HasNext())
    {
    vtkIdType vertex = dfs->Next();
    bool vertical = (inputTree->GetLevel(vertex) % 2) == 1;
    if (vertex == inputTree->GetRoot())
      {
      coords[0] = 0; coords[1] = 1; coords[2] = 0; coords[3] = 1;
      coordsArray->SetTuple(vertex, coords);
      }
    double doubleCoords[4];
    coordsArray->GetTuple(vertex, doubleCoords);
    for (int i = 0; i < 4; i++)
      {
      coords[i] = doubleCoords[i];
      }
    this->AddBorder(coords);
    float parentMinX = coords[0];
    float parentMaxX = coords[1];
    float parentMinY = coords[2];
    float parentMaxY = coords[3];
    float xSpace = parentMaxX - parentMinX;
    float ySpace = parentMaxY - parentMinY;

    inputTree->GetChildren(vertex, nchildren, children);

    float total = 0;
    for (int i = 0; i < nchildren; i++)
      {
      total += static_cast<float>(sizeArray->GetTuple1(children[i]));
      }
    
    // Give children their positions
    float part = 0;
    float oldDelta = 0;
    float delta = 0;
    for (int i = 0; i < nchildren; i++)
      {
      part += static_cast<float>(sizeArray->GetTuple1(children[i]));
      oldDelta = delta;
      if (vertical)
        {
        delta = xSpace * (part / total);
        coords[0] = parentMinX + oldDelta;     // minX
        coords[1] = parentMinX + delta;        // maxX
        coords[2] = parentMinY;                // minY
        coords[3] = parentMaxY;                // maxY
        }
      else
        {
        delta = ySpace * (part / total);
        coords[0] = parentMinX;                // minX
        coords[1] = parentMaxX;                // maxX
        coords[2] = parentMaxY - delta;        // maxY
        coords[3] = parentMaxY - oldDelta;     // minY
        }
      coordsArray->SetTuple(children[i], coords);
      }
    }
  dfs->Delete();
}
