/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCosmicTreeLayoutStrategy.cxx

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

#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkGraphLayout.h"
#include "vtkGraphMapper.h"
#include "vtkGraphToPolyData.h"
#include "vtkIdTypeArray.h"
#include "vtkLabeledDataMapper.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkCosmicTreeLayoutStrategy.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTree.h"
#include "vtkTestUtilities.h"
#include "vtkTreeLayoutStrategy.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestCosmicTreeLayoutStrategy(int argc, char* argv[])
{
  const double inputRadius[] =
    {
    0.432801,
    0.343010,
    0.707502,
    0.703797,
    0.072614,
    0.551869,
    0.072092,
    0.354239,
    0.619700,
    0.352652,
    0.578812,
    0.689687,
    0.487843,
    0.099574,
    0.296240,
    0.757327,
    0.103196,
    0.657770,
    0.623855,
    0.485042,
    0.379716,
    0.887008,
    0.400714,
    0.553902,
    0.245740,
    0.715217,
    0.906472,
    0.959179,
    0.561240,
    0.581328
    };
  const vtkIdType inputParents[] =
    {
    -1,
    0,
    0,
    1,
    1,
    1,
    1,
    1,
    2,
    2,
    2,
    2,
    2,
    3,
    3,
    3,
    4,
    5,
    5,
    5,
    6,
    6,
    6,
    7,
    7,
    7,
    7,
    8,
    9,
    9
    };
  int numNodes = sizeof( inputParents ) / sizeof( inputParents[0] );

  VTK_CREATE(vtkMutableDirectedGraph, builder);
  for ( int i = 0; i < numNodes; ++ i )
  {
    if ( inputParents[i] < 0 )
    {
      builder->AddVertex();
    }
    else
    {
      builder->AddChild( inputParents[i] );
    }
  }

  VTK_CREATE(vtkTree, tree);
  tree->ShallowCopy( builder );

  VTK_CREATE(vtkIdTypeArray, idArr);
  VTK_CREATE(vtkDoubleArray, radArr);
  idArr->SetName( "id" );
  radArr->SetName( "inputRadius" );
  for ( vtkIdType i = 0; i < numNodes; ++ i )
  {
    idArr->InsertNextValue( i );
    radArr->InsertNextValue( inputRadius[i] );
  }
  tree->GetVertexData()->AddArray( idArr );
  tree->GetVertexData()->AddArray( radArr );

  VTK_CREATE(vtkCosmicTreeLayoutStrategy, strategy);
  strategy->SizeLeafNodesOnlyOn();
  strategy->SetNodeSizeArrayName( "inputRadius" );
  VTK_CREATE(vtkGraphLayout, layout);
  layout->SetInputData( tree );
  layout->SetLayoutStrategy( strategy );

  VTK_CREATE(vtkGraphToPolyData, poly);
  poly->SetInputConnection( layout->GetOutputPort() );
  VTK_CREATE(vtkLabeledDataMapper, labelMapper);
  labelMapper->SetInputConnection( poly->GetOutputPort() );
  labelMapper->SetLabelModeToLabelFieldData();
  labelMapper->SetInputArrayToProcess( 0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "id" );
  VTK_CREATE(vtkActor2D, labelActor);
  labelActor->SetMapper( labelMapper );

  VTK_CREATE(vtkGraphMapper, mapper);
  mapper->SetInputConnection( layout->GetOutputPort() );
  mapper->SetScalingArrayName( "TreeRadius" );
  mapper->ScaledGlyphsOn();
  mapper->SetVertexColorArrayName( "id" );
  mapper->ColorVerticesOn();
  VTK_CREATE(vtkActor, actor);
  actor->SetMapper( mapper );
  VTK_CREATE(vtkRenderer, ren);
  ren->AddActor( actor );
  ren->AddActor( labelActor );
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  VTK_CREATE(vtkRenderWindow, win);
  win->AddRenderer( ren );
  win->SetInteractor( iren );

  int retVal = vtkRegressionTestImage( win );
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Initialize();
    iren->Start();

    retVal = vtkRegressionTester::PASSED;
  }

  return !retVal;
}
