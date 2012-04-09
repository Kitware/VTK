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
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int SelectionCompare(
  vtkSelectionNode* a,
  vtkSelectionNode* b)
{
  int errors = 0;
  vtkIdTypeArray* alist = vtkIdTypeArray::SafeDownCast(
    a->GetSelectionList());
  vtkIdTypeArray* blist = vtkIdTypeArray::SafeDownCast(
    b->GetSelectionList());
  if (a->GetContentType() != b->GetContentType())
    {
    cerr << "ERROR: Content type does not match." << endl;
    errors++;
    }
  if (a->GetContentType() == vtkSelectionNode::VALUES)
    {
    if (!alist->GetName() ||
        !blist->GetName() ||
        strcmp(alist->GetName(), blist->GetName()))
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
      cerr << "ERROR: The number of tuples in the selection list do not match ("
           << numTuples << "!=" << bnumTuples << ")." << endl;
      errors++;
      }
    else if (numComps != bnumComps)
      {
      cerr << "ERROR: The number of components in the selection list do not match ("
           << numComps << "!=" << bnumComps << ")." << endl;
      errors++;
      }
    else
      {
      for (vtkIdType i = 0; i < numComps*numTuples; i++)
        {
        if (alist->GetValue(i) != blist->GetValue(i))
          {
          cerr << "ERROR: Selection lists do not match at sel "
               << i
               << "("
               << alist->GetValue(i)
               << " != "
               << blist->GetValue(i)
               << ")."
               << endl;
          errors++;
          break;
          }
        }
      }
    }
  return errors;
}

int SelectionCompare(
  vtkSelection* a,
  vtkSelection* b)
{
  int errors = 0;
  if (a->GetNumberOfNodes() != b->GetNumberOfNodes())
    {
    cerr << "ERROR: Number of nodes do not match." << endl;
    errors++;
    }
  else
    {
    for (unsigned int cc=0; cc < a->GetNumberOfNodes(); cc++)
      {
      errors += SelectionCompare(a->GetNode(cc), b->GetNode(cc));
      }
    }
  return errors;
}

int TestAppendSelectionCase(
  vtkSelection* input1,
  vtkSelection* input2,
  vtkSelection* correct)
{
  VTK_CREATE(vtkAppendSelection, append);
  append->AddInputData(input1);
  append->AddInputData(input2);
  append->Update();
  vtkSelection* output = append->GetOutput();
  return SelectionCompare(output, correct);
}

int TestAppendSelection(int, char*[])
{
  int errors = 0;

  {
  cerr << "Testing appending sel selections ..." << endl;
  VTK_CREATE(vtkSelection, sel1);
  VTK_CREATE(vtkSelectionNode, sel1Node);
  VTK_CREATE(vtkIdTypeArray, sel1Arr);
  sel1->AddNode(sel1Node);
  sel1Node->SetContentType(vtkSelectionNode::INDICES);
  sel1Node->SetFieldType(vtkSelectionNode::CELL);
  sel1Node->SetSelectionList(sel1Arr);
  sel1Arr->InsertNextValue(0);
  sel1Arr->InsertNextValue(1);
  sel1Arr->InsertNextValue(2);
  VTK_CREATE(vtkSelection, sel2);
  VTK_CREATE(vtkSelectionNode, sel2Node);
  VTK_CREATE(vtkIdTypeArray, sel2Arr);
  sel2->AddNode(sel2Node);
  sel2Node->SetContentType(vtkSelectionNode::INDICES);
  sel2Node->SetFieldType(vtkSelectionNode::CELL);
  sel2Node->SetSelectionList(sel2Arr);
  sel2Arr->InsertNextValue(3);
  sel2Arr->InsertNextValue(4);
  sel2Arr->InsertNextValue(5);
  VTK_CREATE(vtkSelection, selAppend);
  VTK_CREATE(vtkSelectionNode, selAppendNode);
  VTK_CREATE(vtkIdTypeArray, selAppendArr);
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
  cerr << "Testing appending value selections ..." << endl;
  VTK_CREATE(vtkSelection, sel1);
  VTK_CREATE(vtkSelectionNode, sel1Node);
  VTK_CREATE(vtkIdTypeArray, sel1Arr);
  sel1->AddNode(sel1Node);
  sel1Arr->SetName("arrayname");
  sel1Node->SetContentType(vtkSelectionNode::VALUES);
  sel1Node->SetFieldType(vtkSelectionNode::CELL);
  sel1Node->SetSelectionList(sel1Arr);
  sel1Arr->InsertNextValue(0);
  sel1Arr->InsertNextValue(1);
  sel1Arr->InsertNextValue(2);
  VTK_CREATE(vtkSelection, sel2);
  VTK_CREATE(vtkSelectionNode, sel2Node);
  VTK_CREATE(vtkIdTypeArray, sel2Arr);
  sel2->AddNode(sel2Node);
  sel2Arr->SetName("arrayname");
  sel2Node->SetContentType(vtkSelectionNode::VALUES);
  sel2Node->SetFieldType(vtkSelectionNode::CELL);
  sel2Node->SetSelectionList(sel2Arr);
  sel2Arr->InsertNextValue(3);
  sel2Arr->InsertNextValue(4);
  sel2Arr->InsertNextValue(5);
  VTK_CREATE(vtkSelection, selAppend);
  VTK_CREATE(vtkSelectionNode, selAppendNode);
  VTK_CREATE(vtkIdTypeArray, selAppendArr);
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
  cerr << "Testing appending cell selections with different process ids..." << endl;
  VTK_CREATE(vtkSelection, sel1);
  VTK_CREATE(vtkSelectionNode, sel1Node);
  VTK_CREATE(vtkIdTypeArray, sel1Arr);
  sel1->AddNode(sel1Node);
  sel1Node->SetContentType(vtkSelectionNode::INDICES);
  sel1Node->SetFieldType(vtkSelectionNode::CELL);
  sel1Node->SetSelectionList(sel1Arr);
  sel1Node->GetProperties()->Set(vtkSelectionNode::PROCESS_ID(), 0);
  sel1Arr->InsertNextValue(0);
  sel1Arr->InsertNextValue(1);
  sel1Arr->InsertNextValue(2);
  VTK_CREATE(vtkSelection, sel2);
  VTK_CREATE(vtkSelectionNode, sel2Node);
  VTK_CREATE(vtkIdTypeArray, sel2Arr);
  sel2->AddNode(sel2Node);
  sel2Node->SetContentType(vtkSelectionNode::INDICES);
  sel2Node->SetFieldType(vtkSelectionNode::CELL);
  sel2Node->SetSelectionList(sel2Arr);
  sel2Node->GetProperties()->Set(vtkSelectionNode::PROCESS_ID(), 1);
  sel2Arr->InsertNextValue(3);
  sel2Arr->InsertNextValue(4);
  sel2Arr->InsertNextValue(5);
  VTK_CREATE(vtkSelection, selAppend);
  selAppend->AddNode(sel1Node);
  selAppend->AddNode(sel2Node);
  errors += TestAppendSelectionCase(sel1, sel2, selAppend);
  cerr << "... done." << endl;
  }

  return errors;
}

