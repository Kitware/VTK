/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNetworkViews2.cxx

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

#include "vtkDataObjectToTable.h"
#include "vtkDataSetAttributes.h"
#include "vtkGroupLeafVertices.h"
#include "vtkNetworkHierarchy.h"
#include "vtkRegressionTestImage.h"
#include "vtkRemoveIsolatedVertices.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRowQueryToTable.h"
#include "vtkSQLDatabase.h"
#include "vtkSQLDatabaseTableSource.h"
#include "vtkSQLQuery.h"
#include "vtkTableToGraph.h"
#include "vtkTableToTreeFilter.h"
#include "vtkTreeRingView.h"
#include "vtkTreeRingView3.h"
#include "vtkVertexDegree.h"
#include "vtkViewTheme.h"
#include "vtkQtInitialization.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

using vtkstd::string;

int TestNetworkViews2(int argc, char* argv[])
{
  //Pull the table (that represents relationships/edges) from the database
  VTK_CREATE( vtkSQLDatabaseTableSource, databaseToEdgeTable );
  databaseToEdgeTable->SetURL("psql://bnwylie@tlp-ds.sandia.gov:5432/sunburst");
  databaseToEdgeTable->SetQuery("select src, dst, dport from tcpsummary where dport != 80");
  
  //Pull the table (that represents entities/vertices) from the database
  VTK_CREATE( vtkSQLDatabaseTableSource, databaseToVertexTable );
  databaseToVertexTable->SetURL("psql://bnwylie@tlp-ds.sandia.gov:5432/sunburst");
  databaseToVertexTable->SetQuery("select d.ip, d.name, i.country_name,i.region_name,i.city_name,i.latitude, i.longitude from  dnsnames d, ipligence i where ip4(d.ip)<<= ip_range;");

  // # Make a graph
  VTK_CREATE( vtkTableToGraph, graph );
  graph->AddInputConnection(0,databaseToEdgeTable->GetOutputPort());
  graph->AddInputConnection(1,databaseToVertexTable->GetOutputPort());
  graph->AddLinkVertex("src", "ip", false);
  graph->AddLinkVertex("dst", "ip", false);
  graph->AddLinkEdge("src", "dst");

  //# Remove any isolated vertices
  VTK_CREATE( vtkRemoveIsolatedVertices, isolated );
  isolated->AddInputConnection(graph->GetOutputPort());

  //# Make a tree out of ip addresses
  VTK_CREATE( vtkNetworkHierarchy, ip_tree );
  ip_tree->AddInputConnection(isolated->GetOutputPort());
  ip_tree->Update();
  
  //# Cleave off part of the graph
  VTK_CREATE( vtkDataObjectToTable, vertexDataTable );
  vertexDataTable->SetInputConnection(isolated->GetOutputPort());
  vertexDataTable->SetFieldType(3); //# Vertex data

  //# Make a tree out of city/region/country
  VTK_CREATE( vtkTableToTreeFilter, toTree );
  toTree->AddInputConnection(vertexDataTable->GetOutputPort());
  VTK_CREATE( vtkGroupLeafVertices, tree1 );
  tree1->AddInputConnection(toTree->GetOutputPort());
  tree1->SetInputArrayToProcess(0,0, 0, 4, "country_name");
  tree1->SetInputArrayToProcess(1,0, 0, 4, "ip");
  VTK_CREATE( vtkGroupLeafVertices, tree2 );
  tree2->AddInputConnection(tree1->GetOutputPort());
  tree2->SetInputArrayToProcess(0,0, 0, 4, "region_name");
  tree2->SetInputArrayToProcess(1,0, 0, 4, "ip");
  VTK_CREATE( vtkGroupLeafVertices, tree3 );
  tree3->AddInputConnection(tree2->GetOutputPort());
  tree3->SetInputArrayToProcess(0,0, 0, 4, "city_name");
  tree3->SetInputArrayToProcess(1,0, 0, 4, "ip");
  tree3->Update();
  tree3->GetOutput()->GetVertexData()->SetActivePedigreeIds("ip");

  VTK_CREATE( vtkTreeRingView3, dummy );
  
  //# Create a view on city/region/country
  VTK_CREATE(vtkTreeRingView3, view1);
  view1->SetTreeFromInputConnection(tree3->GetOutputPort());
  view1->SetGraphFromInputConnection(isolated->GetOutputPort());
  view1->SetLabelPriorityArrayName("GraphVertexDegree");
//  view1->SetAreaColorArrayName("VertexDegree");
  view1->SetAreaColorArrayName("GraphVertexDegree");
  view1->SetAreaLabelArrayName("ip");
  view1->SetAreaHoverArrayName("ip");
  view1->SetAreaLabelVisibility(true);
  view1->SetEdgeColorArrayName("dport");
  view1->SetColorEdges(true);
  view1->SetAreaLabelFontSize( 8 );
  view1->SetInteriorLogSpacingValue(2.);
  view1->SetBundlingStrength(.7);
  view1->SetShrinkPercentage( 0.04 );

  //Create a view on IP network addresses
  VTK_CREATE( vtkTreeRingView3, view2 );
  view2->SetTreeFromInputConnection(ip_tree->GetOutputPort());
  view2->SetGraphFromInputConnection(isolated->GetOutputPort());
  view2->SetAreaColorArrayName("VertexDegree");
  view2->SetAreaLabelArrayName("ip");
  view2->SetAreaHoverArrayName("ip");
  view2->SetAreaLabelVisibility(true);
  view2->SetEdgeColorArrayName("dport");
  view2->SetColorEdges(true);
  view2->SetAreaLabelFontSize( 8 );
  view2->SetInteriorLogSpacingValue(2.);
  view2->SetBundlingStrength(.0);
  
  // Apply a theme to the views
  vtkViewTheme* const theme = vtkViewTheme::CreateOceanTheme();
  theme->SetCellColor( .2, .2, .6 );
  theme->SetCellOpacity( .25 );
  theme->SetVertexLabelColor( 1, 1, 1 );
  view1->ApplyViewTheme(theme);
  theme->Delete();

  VTK_CREATE( vtkRenderWindow, window1 );
  window1->SetMultiSamples(0);
  window1->SetSize(1024, 1024); 
  VTK_CREATE( vtkRenderWindow, window2 );
  window2->SetSize(1024, 1024);

  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(window1);
  dummy->SetupRenderWindow(window1);
  view1->SetupRenderWindow(window1);
  view2->SetupRenderWindow(window2);
  view2->GetRenderer()->ResetCamera();

  window1->Render();

  int retVal = vtkRegressionTestImage(window1);
//  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Initialize();
    iren->Start();
    
    retVal = vtkRegressionTester::PASSED;
    }
  
  return !retVal;
}
