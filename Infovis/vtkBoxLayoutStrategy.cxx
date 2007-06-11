/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoxLayoutStrategy.cxx

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

#include "vtkBoxLayoutStrategy.h"
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

vtkCxxRevisionMacro(vtkBoxLayoutStrategy, "1.3");
vtkStandardNewMacro(vtkBoxLayoutStrategy);

vtkBoxLayoutStrategy::vtkBoxLayoutStrategy()
{
}

vtkBoxLayoutStrategy::~vtkBoxLayoutStrategy()
{
}

void vtkBoxLayoutStrategy::Layout(vtkTree *inputTree, 
  vtkDataArray *coordsArray)
{
  vtkTreeDFSIterator* dfs = vtkTreeDFSIterator::New();
  dfs->SetTree(inputTree);
  float coords[4];
  vtkIdType nchildren;
  const vtkIdType* children;
  while (dfs->HasNext())
    {
    vtkIdType vertex = dfs->Next();
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
    if (nchildren > 0)
      {
      // Divide the available space with simple algo
      int xDivisions = (int)(sqrt((float)nchildren)+1); // Ceiling
      int yDivisions = xDivisions;
       
      // Okay try shrinking the bounds
      if ((xDivisions-1)*yDivisions >= nchildren)
        --xDivisions;
      if (xDivisions*(yDivisions-1) >= nchildren)
        --yDivisions;
        
      // Now break up the space evenly and pack
      float xDelta = xSpace / xDivisions;
      float yDelta = ySpace / yDivisions;
      int childIndex = 0;
      for (int i = 0; i < yDivisions; i++)
        {
        for (int j = 0; j < xDivisions; j++)
          {
          // Check to see if we have more children
          if (childIndex >= nchildren)
            {
            break;
            }
          
          // Give children their positions
          coords[0] = 
            parentMinX + xDelta * j;// minX
          coords[1] = 
            parentMinX + xDelta * (j + 1.0);// maxX
          coords[2] = 
            parentMinY + ySpace - yDelta * (i + 1.0);// minY
          coords[3] = 
            parentMinY + ySpace - yDelta*i;// maxY
          
          int id = children[childIndex];
          coordsArray->SetTuple(id, coords);
        
          // Increment child count
          ++childIndex;
          }
        }
      }
    }
  dfs->Delete();
}

void vtkBoxLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
