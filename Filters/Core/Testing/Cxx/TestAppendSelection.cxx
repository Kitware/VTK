/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAppendSelection.cxx

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

#include "vtkAppendSelection.h"
#include "vtkExtractSelection.h"
#include "vtkIOSSReader.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSelectionSource.h"
#include "vtkTestUtilities.h"

int SelectionCompare(vtkSelectionNode* a, vtkSelectionNode* b)
{
  int errors = 0;
  vtkIdTypeArray* alist = vtkArrayDownCast<vtkIdTypeArray>(a->GetSelectionList());
  vtkIdTypeArray* blist = vtkArrayDownCast<vtkIdTypeArray>(b->GetSelectionList());
  if (a->GetContentType() != b->GetContentType())
  {
    cerr << "ERROR: Content type does not match." << endl;
    errors++;
  }
  if (a->GetContentType() == vtkSelectionNode::VALUES)
  {
    if (!alist->GetName() || !blist->GetName() || strcmp(alist->GetName(), blist->GetName()) != 0)
    {
      cerr << "ERROR: The array names do not match." << endl;
    }
  }
  if (a->GetFieldType() != b->GetFieldType())
  {
    cerr << "ERROR: Field type does not match." << endl;
    errors++;
  }
  if ((alist && !blist) || (!alist && blist))
  {
    cerr << "ERROR: One has a selection list while the other does not." << endl;
    errors++;
  }
  if (alist && blist)
  {
    int numComps = alist->GetNumberOfComponents();
    vtkIdType numTuples = alist->GetNumberOfTuples();
    int bnumComps = blist->GetNumberOfComponents();
    vtkIdType bnumTuples = blist->GetNumberOfTuples();
    if (numTuples != bnumTuples)
    {
      cerr << "ERROR: The number of tuples in the selection list do not match (" << numTuples
           << "!=" << bnumTuples << ")." << endl;
      errors++;
    }
    else if (numComps != bnumComps)
    {
      cerr << "ERROR: The number of components in the selection list do not match (" << numComps
           << "!=" << bnumComps << ")." << endl;
      errors++;
    }
    else
    {
      for (vtkIdType i = 0; i < numComps * numTuples; i++)
      {
        if (alist->GetValue(i) != blist->GetValue(i))
        {
          cerr << "ERROR: Selection lists do not match at sel " << i << "(" << alist->GetValue(i)
               << " != " << blist->GetValue(i) << ")." << endl;
          errors++;
          break;
        }
      }
    }
  }
  return errors;
}

int SelectionCompare(vtkSelection* a, vtkSelection* b)
{
  int errors = 0;
  if (a->GetNumberOfNodes() != b->GetNumberOfNodes())
  {
    cerr << "ERROR: Number of nodes do not match." << endl;
    errors++;
  }
  else
  {
    for (unsigned int cc = 0; cc < a->GetNumberOfNodes(); cc++)
    {
      errors += SelectionCompare(a->GetNode(cc), b->GetNode(cc));
    }
  }
  return errors;
}

int TestAppendSelectionCase(vtkSelection* input1, vtkSelection* input2, vtkSelection* correct)
{
  vtkNew<vtkAppendSelection> append;
  append->AddInputData(input1);
  append->AddInputData(input2);
  append->Update();
  vtkSelection* output = append->GetOutput();
  return SelectionCompare(output, correct);
}

int TestAppendSelection(int argc, char* argv[])
{
  int errors = 0;

  {
    cerr << "Testing appending cell-indices selections ..." << endl;
    vtkNew<vtkSelection> sel1;
    vtkNew<vtkSelectionNode> sel1Node;
    vtkNew<vtkIdTypeArray> sel1Arr;
    sel1->AddNode(sel1Node);
    sel1Node->SetContentType(vtkSelectionNode::INDICES);
    sel1Node->SetFieldType(vtkSelectionNode::CELL);
    sel1Node->SetSelectionList(sel1Arr);
    sel1Arr->InsertNextValue(0);
    sel1Arr->InsertNextValue(1);
    sel1Arr->InsertNextValue(2);
    vtkNew<vtkSelection> sel2;
    vtkNew<vtkSelectionNode> sel2Node;
    vtkNew<vtkIdTypeArray> sel2Arr;
    sel2->AddNode(sel2Node);
    sel2Node->SetContentType(vtkSelectionNode::INDICES);
    sel2Node->SetFieldType(vtkSelectionNode::CELL);
    sel2Node->SetSelectionList(sel2Arr);
    sel2Arr->InsertNextValue(3);
    sel2Arr->InsertNextValue(4);
    sel2Arr->InsertNextValue(5);
    vtkNew<vtkSelection> selAppend;
    vtkNew<vtkSelectionNode> selAppendNode;
    vtkNew<vtkIdTypeArray> selAppendArr;
    selAppend->AddNode(selAppendNode);
    selAppendNode->SetContentType(vtkSelectionNode::INDICES);
    selAppendNode->SetFieldType(vtkSelectionNode::CELL);
    selAppendNode->SetSelectionList(selAppendArr);
    selAppendArr->InsertNextValue(0);
    selAppendArr->InsertNextValue(1);
    selAppendArr->InsertNextValue(2);
    selAppendArr->InsertNextValue(3);
    selAppendArr->InsertNextValue(4);
    selAppendArr->InsertNextValue(5);
    errors += TestAppendSelectionCase(sel1, sel2, selAppend);
    cerr << "... done." << endl;
  }

  {
    cerr << "Testing appending cell-values selections ..." << endl;
    vtkNew<vtkSelection> sel1;
    vtkNew<vtkSelectionNode> sel1Node;
    vtkNew<vtkIdTypeArray> sel1Arr;
    sel1->AddNode(sel1Node);
    sel1Arr->SetName("arrayname");
    sel1Node->SetContentType(vtkSelectionNode::VALUES);
    sel1Node->SetFieldType(vtkSelectionNode::CELL);
    sel1Node->SetSelectionList(sel1Arr);
    sel1Arr->InsertNextValue(0);
    sel1Arr->InsertNextValue(1);
    sel1Arr->InsertNextValue(2);
    vtkNew<vtkSelection> sel2;
    vtkNew<vtkSelectionNode> sel2Node;
    vtkNew<vtkIdTypeArray> sel2Arr;
    sel2->AddNode(sel2Node);
    sel2Arr->SetName("arrayname");
    sel2Node->SetContentType(vtkSelectionNode::VALUES);
    sel2Node->SetFieldType(vtkSelectionNode::CELL);
    sel2Node->SetSelectionList(sel2Arr);
    sel2Arr->InsertNextValue(3);
    sel2Arr->InsertNextValue(4);
    sel2Arr->InsertNextValue(5);
    vtkNew<vtkSelection> selAppend;
    vtkNew<vtkSelectionNode> selAppendNode;
    vtkNew<vtkIdTypeArray> selAppendArr;
    selAppend->AddNode(selAppendNode);
    selAppendArr->SetName("arrayname");
    selAppendNode->SetContentType(vtkSelectionNode::VALUES);
    selAppendNode->SetFieldType(vtkSelectionNode::CELL);
    selAppendNode->SetSelectionList(selAppendArr);
    selAppendArr->InsertNextValue(0);
    selAppendArr->InsertNextValue(1);
    selAppendArr->InsertNextValue(2);
    selAppendArr->InsertNextValue(3);
    selAppendArr->InsertNextValue(4);
    selAppendArr->InsertNextValue(5);
    errors += TestAppendSelectionCase(sel1, sel2, selAppend);
    cerr << "... done." << endl;
  }

  {
    cerr << "Testing appending cell-indices selections with different process ids..." << endl;
    vtkNew<vtkSelection> sel1;
    vtkNew<vtkSelectionNode> sel1Node;
    vtkNew<vtkIdTypeArray> sel1Arr;
    sel1->AddNode(sel1Node);
    sel1Node->SetContentType(vtkSelectionNode::INDICES);
    sel1Node->SetFieldType(vtkSelectionNode::CELL);
    sel1Node->SetSelectionList(sel1Arr);
    sel1Node->GetProperties()->Set(vtkSelectionNode::PROCESS_ID(), 0);
    sel1Arr->InsertNextValue(0);
    sel1Arr->InsertNextValue(1);
    sel1Arr->InsertNextValue(2);
    vtkNew<vtkSelection> sel2;
    vtkNew<vtkSelectionNode> sel2Node;
    vtkNew<vtkIdTypeArray> sel2Arr;
    sel2->AddNode(sel2Node);
    sel2Node->SetContentType(vtkSelectionNode::INDICES);
    sel2Node->SetFieldType(vtkSelectionNode::CELL);
    sel2Node->SetSelectionList(sel2Arr);
    sel2Node->GetProperties()->Set(vtkSelectionNode::PROCESS_ID(), 1);
    sel2Arr->InsertNextValue(3);
    sel2Arr->InsertNextValue(4);
    sel2Arr->InsertNextValue(5);
    vtkNew<vtkSelection> selAppend;
    selAppend->AddNode(sel1Node);
    selAppend->AddNode(sel2Node);
    errors += TestAppendSelectionCase(sel1, sel2, selAppend);
    cerr << "... done." << endl;
  }

  {
    cerr << "Testing appending cell-indices selections with expression..." << endl;
    // create first selection
    vtkNew<vtkSelectionSource> sel1;
    sel1->SetNumberOfNodes(2);
    sel1->SetFieldType(vtkSelectionNode::CELL);
    // create first node of first selection
    sel1->SetNodeName(0, "node0");
    sel1->SetContentType(0, vtkSelectionNode::INDICES);
    sel1->SetCompositeIndex(0, 2);
    sel1->AddID(0, -1, 0);
    sel1->AddID(0, -1, 1);
    sel1->AddID(0, -1, 2);
    sel1->AddID(0, -1, 5);
    sel1->AddID(0, -1, 7);
    // create second node of first selection
    sel1->SetNodeName(1, "node1");
    sel1->SetFieldType(vtkSelectionNode::CELL);
    sel1->SetContentType(1, vtkSelectionNode::INDICES);
    sel1->SetCompositeIndex(1, 2);
    sel1->AddID(1, -1, 0);
    sel1->AddID(1, -1, 1);
    sel1->AddID(1, -1, 2);
    sel1->AddID(1, -1, 3);
    sel1->AddID(1, -1, 4);
    sel1->AddID(1, -1, 6);
    // this expression should generate only 3 ids (the common ids = 0, 1, 2)
    sel1->SetExpression("node0&node1");

    // create first selection
    vtkNew<vtkSelectionSource> sel2;
    sel2->SetNumberOfNodes(1);
    sel2->SetFieldType(vtkSelectionNode::CELL);
    // create first node of second selection
    sel2->SetNodeName(0, "node0");
    sel2->SetContentType(0, vtkSelectionNode::INDICES);
    sel2->SetCompositeIndex(0, 2);
    sel2->AddID(0, -1, 10);
    sel2->AddID(0, -1, 11);
    sel2->AddID(0, -1, 12);
    // this selection should generate 3 ids (10, 11, 12)
    sel2->SetExpression("node0");

    vtkNew<vtkAppendSelection> append;
    append->AppendByUnionOff();
    append->AddInputConnection(sel1->GetOutputPort());
    append->SetInputName(0, "S0");
    append->AddInputConnection(sel2->GetOutputPort());
    append->SetInputName(1, "S1");
    // this selection should generate 6 cell ids (0, 1, 2, 10, 11, 12)
    append->SetExpression("S0|S1");

    char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/can.ex2");
    vtkNew<vtkIOSSReader> reader;
    reader->AddFileName(fileNameC);
    delete[] fileNameC;
    reader->Update();

    vtkNew<vtkExtractSelection> extract;
    extract->SetInputConnection(0, reader->GetOutputPort());
    extract->SetInputConnection(1, append->GetOutputPort());
    extract->Update();
    auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(extract->GetOutput());
    errors += pdc->GetNumberOfCells() != 6;
    cerr << "... done." << endl;
  }

  return errors;
}
