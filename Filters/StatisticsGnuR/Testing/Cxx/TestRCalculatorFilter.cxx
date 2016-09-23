/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRCalculatorFilter.cxx

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

#include <vtkRCalculatorFilter.h>
#include <vtkSmartPointer.h>
#include <vtkCylinderSource.h>
#include <vtkDataSet.h>
#include <vtkPointData.h>
#include <vtkDoubleArray.h>
#include <vtkArrayExtents.h>
#include <vtkRRandomTableSource.h>
#include <vtkTable.h>
#include <vtkTableToSparseArray.h>
#include <vtkDenseArray.h>
#include <vtkMutableDirectedGraph.h>
#include <vtkTree.h>
#include <vtkNew.h>
#include <vtkStringArray.h>

#include <iostream>
#include <sstream>
#include <stdexcept>

namespace
{

#define test_expression(expression) \
{ \
  if(!(expression)) \
  { \
    std::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw std::runtime_error(buffer.str()); \
  } \
}

bool integerEquals(int left, int right) {
  return ((left - right) == 0 );
}

bool doubleEquals(double left, double right, double epsilon) {
  return (fabs(left - right) < epsilon);
}

bool stringEquals(const char * left, const char * right) {
  return (strcmp(left, right) == 0);
}

}

int TestRCalculatorFilter(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
  {
    int i;
    vtkCylinderSource* cs = vtkCylinderSource::New();
    vtkRCalculatorFilter* rf = vtkRCalculatorFilter::New();
    vtkRRandomTableSource* rts = vtkRRandomTableSource::New();
    vtkRCalculatorFilter* rf2 = vtkRCalculatorFilter::New();
    vtkRCalculatorFilter* rf3 = vtkRCalculatorFilter::New();
    vtkDataSet* ds;
    vtkPointData* pd;
    vtkDoubleArray* da;
    vtkDoubleArray* rda;

    cs->SetResolution(10);
    rf->SetInputConnection(cs->GetOutputPort());
    rf->SetRoutput(0);
    rf->PutArray("Normals", "Norm");
    rf->PutArray("TCoords", "TCoords");
    rf->GetArray("Normalsnew", "Norm");
    rf->GetArray("TCoordsnew", "TCoords");
    rf->SetRscript("Norm = Norm^2\nTCoords = TCoords + TCoords\n");
    rf->Update();

    ds = vtkDataSet::SafeDownCast(rf->GetOutput());
    pd = ds->GetPointData();
    da = (vtkDoubleArray*) pd->GetArray("Normals");
    rda = (vtkDoubleArray*) pd->GetArray("Normalsnew");

    for(i=0;i<da->GetNumberOfTuples();i++)
    {
      double* itup = da->GetTuple3(i);
      double* rtup = rda->GetTuple3(i);
      test_expression(doubleEquals(rtup[0],pow(itup[0],2),0.0001));
      test_expression(doubleEquals(rtup[1],pow(itup[1],2),0.0001));
      test_expression(doubleEquals(rtup[2],pow(itup[2],2),0.0001));
    }

    da = (vtkDoubleArray*) pd->GetArray("TCoords");
    rda = (vtkDoubleArray*) pd->GetArray("TCoordsnew");

    for(i=0;i<da->GetNumberOfTuples();i++)
    {
      double* itup = da->GetTuple2(i);
      double* rtup = rda->GetTuple2(i);
      test_expression(doubleEquals(rtup[0],itup[0]+itup[0],0.0001));
      test_expression(doubleEquals(rtup[1],itup[1]+itup[1],0.0001));
    }

    rts->SetNumberOfRows(20);
    rts->SetStatisticalDistributionForColumn(vtkRRandomTableSource::NORMAL,0.0,1.0,0.0,"Variable One",0);
    rts->SetStatisticalDistributionForColumn(vtkRRandomTableSource::NORMAL,0.0,1.0,0.0,"Variable Two",1);
    rts->SetStatisticalDistributionForColumn(vtkRRandomTableSource::NORMAL,0.0,1.0,0.0,"Variable Three",2);
    rts->SetStatisticalDistributionForColumn(vtkRRandomTableSource::NORMAL,0.0,1.0,0.0,"Variable Four",3);
    rf2->SetInputConnection(rts->GetOutputPort());
    rf2->SetRoutput(0);
    rf2->PutTable("x");
    rf2->GetTable("z");
    rf2->SetRscript("x\nz = matrix(unlist(x),nrow=length(x[[1]]),ncol=length(x))\n\
                     z[,1] = sample(0:19)\n\
                     z[,2] = sample(0:19)\n\
                     z[,3] = sample(0:19)\n");
    rf2->Update();
    vtkTable* table = vtkTable::SafeDownCast(rf2->GetOutput());

    vtkSmartPointer<vtkTableToSparseArray> source = vtkSmartPointer<vtkTableToSparseArray>::New();
    source->AddInputConnection(rf2->GetOutputPort());
    source->AddCoordinateColumn("0");
    source->AddCoordinateColumn("1");
    source->AddCoordinateColumn("2");
    source->SetValueColumn("3");
    rf->SetInputConnection(source->GetOutputPort());
    rf->RemoveAllPutVariables();
    rf->RemoveAllGetVariables();
    rf->PutArray("0","a");
    rf->GetArray("1","a");
    rf->SetRoutput(0);
    rf->SetRscript("a[,,] = sqrt(a[,,] + 5.0)\n");
    rf->Update();

    vtkDenseArray<double>* const dense_array =
                 vtkDenseArray<double>::SafeDownCast(vtkArrayData::SafeDownCast(rf->GetOutput())->GetArray(1));
    test_expression(dense_array);

    for(i=0;i<table->GetNumberOfColumns();i++)
    {
      int ind0 = table->GetValue(i,0).ToInt();
      int ind1 = table->GetValue(i,1).ToInt();
      int ind2 = table->GetValue(i,2).ToInt();
      double table_val = rts->GetOutput()->GetValue(i,3).ToDouble();
      double dense_val = dense_array->GetValue(vtkArrayCoordinates(ind0,ind1,ind2));
      test_expression(doubleEquals(sqrt(table_val + 5.0),dense_val,0.0001));
    }

    //-----  test PutTree() and GetTree()
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
    weights->SetValue(graph->GetEdgeId(root, internalOne), 0.0f);
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

    rf3->AddInputData(0,itree);
    rf3->AddInputConnection(0,source->GetOutputPort());
    rf3->SetRoutput(0);
    rf3->PutArray("0","a");
    rf3->PutTree("inTree");
    rf3->GetTree("outTree");
    rf3->SetRscript("b<-a\noutTree<-inTree\n");

    rf3->Update();
    vtkTree* outTree = vtkTree::SafeDownCast(rf3->GetOutput());

    test_expression(integerEquals(outTree->GetNumberOfEdges(),5));
    test_expression(integerEquals(outTree->GetNumberOfVertices(),6));

    //check edge data
    double v_weights[5] = {0.0,2.0,3.0,1.0,1.0};
    for (i = 0; i < outTree->GetNumberOfEdges(); i++)
    {
      vtkDoubleArray * t_weights = vtkArrayDownCast<vtkDoubleArray>(outTree->GetEdgeData()->GetArray("weight"));
      test_expression(doubleEquals(t_weights->GetValue(i),double( v_weights[i]), 0.001));
    }

    //check vertex data
    const char *  t_names[] ={"a","b","c","","",""};
    for (i = 0; i < outTree->GetNumberOfVertices(); i++)
    {
      vtkStringArray * v_names = vtkArrayDownCast<vtkStringArray>(outTree->GetVertexData()->GetAbstractArray("node name"));
      test_expression(stringEquals(v_names->GetValue(i).c_str(), t_names[i] ));
    }

    cs->Delete();
    rts->Delete();
    rf->Delete();
    rf2->Delete();
    rf3->Delete();

    return 0;
  }
  catch( std::exception& e )
  {
    cerr << e.what()
         << "\n";
    return 1;
  }
}
