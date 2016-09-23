/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRCalculatorFilterAsymmetrical.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkRCalculatorFilter.h>

#include <vtkDoubleArray.h>
#include <vtkMultiPieceDataSet.h>
#include <vtkNew.h>
#include <vtkTable.h>
#include <vtkTree.h>

int TestRCalculatorFilterAsymmetrical(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  // setup input tables
  vtkNew<vtkTable> inTable1;
  vtkNew<vtkDoubleArray> m1;
  vtkNew<vtkDoubleArray> m2;

  m1->SetNumberOfTuples(2);
  m2->SetNumberOfTuples(2);
  m1->SetName("m1");
  m2->SetName("m2");
  m1->SetValue(0, 1.0f);
  m1->SetValue(1, 3.0f);
  m2->SetValue(0, 2.0f);
  m2->SetValue(1, 2.0f);

  inTable1->AddColumn(m1.GetPointer());
  inTable1->AddColumn(m2.GetPointer());

  vtkNew<vtkTable> inTable2;
  inTable2->DeepCopy(inTable1.GetPointer());

  // test case #1: 1 input table, 1 output tree
  vtkNew<vtkRCalculatorFilter> r1;
  r1->SetRscript("library(ape)\noutput_tree1 <- rtree(3)\n");
  r1->AddInputData(0, inTable1.GetPointer());
  r1->PutTable("input_table1");
  r1->GetTree("output_tree1");
  r1->Update();
  vtkTree *outTree1 = vtkTree::SafeDownCast(r1->GetOutput());
  if (outTree1 == NULL)
  {
    cerr << "case #1 failed." << endl;
    return EXIT_FAILURE;
  }

  // test case #2: 2 input tables, 1 output tree
  vtkNew<vtkMultiPieceDataSet> inComposite1;
  inComposite1->SetNumberOfPieces(2);
  inComposite1->SetPiece(0, inTable1.GetPointer());
  inComposite1->SetPiece(1, inTable2.GetPointer());

  vtkNew<vtkRCalculatorFilter> r2;
  r2->SetRscript("library(ape)\noutput_tree1 <- rtree(3)\n");
  r2->AddInputData(0, inComposite1.GetPointer());
  r2->PutTable("input_table1");
  r2->PutTable("input_table2");
  r2->GetTree("output_tree1");
  r2->Update();

  outTree1 = vtkTree::SafeDownCast(r2->GetOutput());
  if (outTree1 == NULL)
  {
    cerr << "case #2 failed." << endl;
    return EXIT_FAILURE;
  }

  // test case #3: 2 input tables, 2 output trees
  vtkNew<vtkRCalculatorFilter> r3;
  r3->SetRscript("library(ape)\noutput_tree1 <- rtree(3)\noutput_tree2 <- rtree(3)\n");
  r3->AddInputData(0, inComposite1.GetPointer());
  r3->PutTable("input_table1");
  r3->PutTable("input_table2");
  r3->GetTree("output_tree1");
  r3->GetTree("output_tree2");
  r3->Update();

  vtkMultiPieceDataSet * outComposite = vtkMultiPieceDataSet::SafeDownCast(r3->GetOutput());
  if(outComposite == NULL)
  {
    cerr << "case #3 failed because outComposite is NULL." << endl;
    return EXIT_FAILURE;
  }
  outTree1 = vtkTree::SafeDownCast(outComposite->GetPieceAsDataObject(0));
  if (outTree1 == NULL)
  {
    cerr << "case #3 failed because outTree1 is NULL." << endl;
    return EXIT_FAILURE;
  }
  vtkTree *outTree2 = vtkTree::SafeDownCast(outComposite->GetPieceAsDataObject(1));
  if (outTree2 == NULL)
  {
    cerr << "case #3 failed because outTree2 is NULL." << endl;
    return EXIT_FAILURE;
  }

  // test case #4: 1 input table, 2 output trees
  vtkNew<vtkRCalculatorFilter> r4;
  r4->SetRscript("library(ape)\noutput_tree1 <- rtree(3)\noutput_tree2 <- rtree(3)\n");
  r4->AddInputData(0, inTable1.GetPointer());
  r4->PutTable("input_table1");
  r4->GetTree("output_tree1");
  r4->GetTree("output_tree2");
  r4->Update();

  outComposite = vtkMultiPieceDataSet::SafeDownCast(r4->GetOutput());
  if(outComposite == NULL)
  {
    cerr << "case #4 failed because outComposite is NULL." << endl;
    return EXIT_FAILURE;
  }
  outTree1 = vtkTree::SafeDownCast(outComposite->GetPieceAsDataObject(0));
  if (outTree1 == NULL)
  {
    cerr << "case #4 failed because outTree1 is NULL." << endl;
    return EXIT_FAILURE;
  }
  outTree2 = vtkTree::SafeDownCast(outComposite->GetPieceAsDataObject(1));
  if (outTree2 == NULL)
  {
    cerr << "case #4 failed because outTree2 is NULL." << endl;
    return EXIT_FAILURE;
  }

  // test case #5: 1 input table, 1 input tree, 2 output trees
  vtkNew<vtkTree> inTree1;
  vtkNew<vtkMultiPieceDataSet> inComposite2;
  inComposite2->SetNumberOfPieces(2);
  inComposite2->SetPiece(0, inTable1.GetPointer());
  inComposite2->SetPiece(1, inTree1.GetPointer());

  vtkNew<vtkRCalculatorFilter> r5;
  r5->SetRscript("library(ape)\noutput_tree1 <- rtree(3)\noutput_tree2 <- rtree(3)\n");
  r5->AddInputData(0, inTable1.GetPointer());
  r5->PutTable("input_table1");
  r5->PutTree("input_tree1");
  r5->GetTree("output_tree1");
  r5->GetTree("output_tree2");
  r5->Update();

  outComposite = vtkMultiPieceDataSet::SafeDownCast(r5->GetOutput());
  if(outComposite == NULL)
  {
    cerr << "case #4 failed because outComposite is NULL." << endl;
    return EXIT_FAILURE;
  }
  outTree1 = vtkTree::SafeDownCast(outComposite->GetPieceAsDataObject(0));
  if (outTree1 == NULL)
  {
    cerr << "case #4 failed because outTree1 is NULL." << endl;
    return EXIT_FAILURE;
  }
  outTree2 = vtkTree::SafeDownCast(outComposite->GetPieceAsDataObject(1));
  if (outTree2 == NULL)
  {
    cerr << "case #4 failed because outTree2 is NULL." << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
