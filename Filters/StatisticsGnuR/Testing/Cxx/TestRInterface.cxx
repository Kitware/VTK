/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRInterface.cxx

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkRInterface.h>
#include <vtkSmartPointer.h>
#include <vtkDoubleArray.h>
#include <vtkRRandomTableSource.h>
#include <vtkArray.h>
#include <vtkTable.h>
#include <vtkTree.h>
#include <vtkDenseArray.h>
#include <vtkMutableDirectedGraph.h>
#include <vtkNew.h>
#include <vtkDataSetAttributes.h>
#include <vtkStringArray.h>

#include <vtksys/ios/iostream>
#include <vtksys/ios/sstream>
#include <vtksys/stl/stdexcept>
#include <stdio.h>
#include <string.h>
#include <cassert>

namespace
{

#define test_expression(expression) \
{ \
  if(!(expression)) \
    { \
    vtksys_ios::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw std::runtime_error(buffer.str()); \
    } \
}

bool doubleEquals(double left, double right, double epsilon) {
  return (fabs(left - right) < epsilon);
}

bool integerEquals(int left, int right) {
  return ((left - right) == 0 );
}

bool stringEquals(const char * left, const char * right) {
  return (strcmp(left, right) == 0);
}

}

int TestRInterface(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
    int buf_size = 2000;
    char* out_buffer = new char[buf_size];
    out_buffer[0] = '\0';
    vtkDoubleArray* da = vtkDoubleArray::New();
    vtkDenseArray<double>* dda = vtkDenseArray<double>::New();
    vtkRRandomTableSource* rts = vtkRRandomTableSource::New();
    vtkRInterface* rint = vtkRInterface::New();

    rint->OutputBuffer(out_buffer, buf_size);
    rint->EvalRscript("1:10\n");
    test_expression(strlen(out_buffer) > 10);

    da->SetNumberOfComponents(3);
    for( int cc = 0; cc < 10; cc ++ )
      {
      da->InsertNextTuple3( cc + 0.1, cc + 0.2, cc + 0.3);
      }
    rint->AssignVTKDataArrayToRVariable(da, "d");
    rint->EvalRscript("d[,1] = d[,1] - 0.1\n\
                       d[,2] = d[,2] - 0.2\n\
                       d[,3] = d[,3] - 0.3\n");
    vtkDoubleArray* rda = vtkDoubleArray::SafeDownCast(rint->AssignRVariableToVTKDataArray("d"));
    for(int i = 0;i<rda->GetNumberOfTuples();i++)
      {
      double* iv = da->GetTuple3(i);
      double* rv = rda->GetTuple3(i);
      test_expression(doubleEquals(iv[0] - 0.1,rv[0],0.001));
      test_expression(doubleEquals(iv[1] - 0.2,rv[1],0.001));
      test_expression(doubleEquals(iv[2] - 0.3,rv[2],0.001));
      }

    dda->Resize(vtkArrayExtents(3, 3, 3));
    dda->Fill(64.0);
    rint->AssignVTKArrayToRVariable(dda, "a");
    rint->EvalRscript("a = sqrt(a)\n");
    vtkDenseArray<double>* rdda = vtkDenseArray<double>::SafeDownCast(rint->AssignRVariableToVTKArray("a"));
    assert(rdda->GetExtents().ZeroBased());
    const vtkArrayExtents extents = rdda->GetExtents();
    for(int i = 0; i != extents[0].GetSize(); ++i)
      {
      for(int j = 0; j != extents[1].GetSize(); ++j)
        {
        for(int k = 0; k != extents[2].GetSize(); ++k)
          {
          test_expression(doubleEquals(sqrt(dda->GetValue(vtkArrayCoordinates(i, j, k))),
                                       rdda->GetValue(vtkArrayCoordinates(i, j, k)),
                                       0.001));
          }
        }
      }

    rts->SetNumberOfRows(20);
    rts->SetStatisticalDistributionForColumn(vtkRRandomTableSource::NORMAL,0.0,1.0,0.0,"Variable One",0);
    rts->SetStatisticalDistributionForColumn(vtkRRandomTableSource::NORMAL,0.0,1.0,0.0,"Variable Two",1);
    rts->SetStatisticalDistributionForColumn(vtkRRandomTableSource::NORMAL,0.0,1.0,0.0,"Variable Three",2);
    rts->SetStatisticalDistributionForColumn(vtkRRandomTableSource::NORMAL,0.0,1.0,0.0,"Variable Four",3);
    rts->Update();
    vtkTable* itable = rts->GetOutput();
    rint->AssignVTKTableToRVariable(itable,"t");
    rint->EvalRscript("t = matrix(unlist(t),nrow=length(t[[1]]),ncol=length(t))\n\
                       t = t - t\n");
    vtkTable* table = rint->AssignRVariableToVTKTable("t");
    for(int i=0;i<table->GetNumberOfColumns();i++)
      {
      for(int j=0;j<table->GetNumberOfRows();j++)
        {
        double i_val = itable->GetValue(i,j).ToDouble() - itable->GetValue(i,j).ToDouble();
        double r_val = table->GetValue(i,j).ToDouble();
        test_expression(doubleEquals(i_val,r_val,0.0001));
        }
      }

    //----------------test vtkTree <==> R Tree
    // 1) construct a vtkTree
    vtkNew<vtkMutableDirectedGraph> graph;
    vtkIdType root = graph->AddVertex();
    vtkIdType internalOne = graph->AddChild(root);
    vtkIdType internalTwo = graph->AddChild(internalOne);
    vtkIdType a = graph->AddChild(internalTwo);
    vtkIdType b = graph->AddChild(internalTwo);
    vtkIdType c = graph->AddChild(internalOne);

    vtkNew<vtkDoubleArray> weights;
    weights->SetNumberOfTuples(5);
    weights->SetValue(graph->GetEdgeId(root, internalOne), 1.0f);
    weights->SetValue(graph->GetEdgeId(internalOne, internalTwo), 2.0f);
    weights->SetValue(graph->GetEdgeId(internalTwo, a), 1.0f);
    weights->SetValue(graph->GetEdgeId(internalTwo, b), 1.0f);
    weights->SetValue(graph->GetEdgeId(internalOne, c), 3.0f);

    weights->SetName("weight");
    graph->GetEdgeData()->AddArray(weights.GetPointer());


    vtkNew<vtkStringArray> names;
    names->SetNumberOfTuples(6);
    names->SetValue(root,"");
    names->SetValue(internalOne,"");
    names->SetValue(internalTwo,"");
    names->SetValue(a, "a");
    names->SetValue(b, "b");
    names->SetValue(c, "c");
    names->SetName("node name");
    graph->GetVertexData()->AddArray(names.GetPointer());

    vtkSmartPointer<vtkTree> itree = vtkSmartPointer<vtkTree>::New();
    if ( ! itree->CheckedDeepCopy(graph.GetPointer()))
      {
      std::cout<<"Edges do not create a valid tree."<<std::endl;
      return 1;
      };


    //2) test VTKTree to R
    rint->AssignVTKTreeToRVariable(itree,"r_tr");
    rint->EvalRscript("edge<-r_tr[[1]]\n\
                       Nnode<-r_tr[[2]]\n\
                       tip_label<-r_tr[[3]]\n\
                       edge_length<-r_tr[[4]]\n\
                       node_label<-r_tr[[5]]\n");

    // check edge
    vtkDoubleArray* r_edge = vtkDoubleArray::SafeDownCast(rint->AssignRVariableToVTKDataArray("edge"));
    int EDGE_ARRAY[4][2] = { {4,5},{4,3},{5,1},{5,2}};
    for ( int i = 0; i< r_edge->GetNumberOfTuples(); i++)
      {
      double * a = r_edge->GetTuple(i);
      test_expression(doubleEquals(a[0],double( EDGE_ARRAY[i][0]), 0.001));
      test_expression(doubleEquals(a[1],double( EDGE_ARRAY[i][1]), 0.001));
      }
    //check Nnode
    vtkDoubleArray* r_Nnode= vtkDoubleArray::SafeDownCast(rint->AssignRVariableToVTKDataArray("Nnode"));
    test_expression(doubleEquals(r_Nnode->GetValue(0),double(2), 0.001));

    //check tip_label, node.label
    /* TODO: implement R <=> VTKStringArray, so that the following function can be called:
       vtkStringArray* r_tip_label= vtkStringArray::SafeDownCast(rint->AssignRVariableToVTKStringArray("tip_label"));
     */

    //check edge_length
    vtkDoubleArray* r_edge_length= vtkDoubleArray::SafeDownCast(rint->AssignRVariableToVTKDataArray("edge_length"));
    double e_weights[4] = {2.0,3.0,1.0,1.0};
    for (int i = 0; i < r_edge_length->GetNumberOfTuples(); i++)
      {
      double * r_weights = r_edge_length->GetTuple(i);
      test_expression(doubleEquals(r_weights[0],double( e_weights[i]), 0.001));
      }


    // 3) test R to VTKTree
    vtkTree * vtk_tr = rint->AssignRVariableToVTKTree("r_tr");

    test_expression(integerEquals(vtk_tr->GetNumberOfEdges(),5));
    test_expression(integerEquals(vtk_tr->GetNumberOfVertices(),6));


    //check edge data
    double v_weights[5] = {0.0,2.0,3.0,1.0,1.0};
    for (int i = 0; i < vtk_tr->GetNumberOfEdges(); i++)
      {
      vtkDoubleArray * weights = vtkDoubleArray::SafeDownCast(vtk_tr->GetEdgeData()->GetArray("weight"));
      test_expression(doubleEquals(weights->GetValue(i),double( v_weights[i]), 0.001));
      }

    //check vertex data
    const char *  t_names[] ={"","a","b","c","",""};
    for (int i = 0; i < vtk_tr->GetNumberOfVertices(); i++)
      {
      vtkStringArray * names = vtkStringArray::SafeDownCast(vtk_tr->GetVertexData()->GetAbstractArray("node name"));
      test_expression(stringEquals(names->GetValue(i).c_str(), t_names[i] ));
      }


    delete [] out_buffer;
    rts->Delete();
    dda->Delete();
    da->Delete();
    rint->Delete();
    return 0;
    }
  catch( std::exception& e )
    {
    cerr << e.what()
      << "\n";
    return 1;
    }
}

