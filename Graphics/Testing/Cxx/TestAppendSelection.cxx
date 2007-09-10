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
#include "vtkSmartPointer.h"

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int SelectionCompare(
  vtkSelection* a,
  vtkSelection* b)
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
  if (a->GetContentType() == vtkSelection::VALUES)
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
    if (numComps != bnumComps || numTuples != bnumTuples)
      {
      cerr << "ERROR: The number of components and/or tuples in the selection list do not match." << endl;
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
  if (a->GetContentType() == vtkSelection::SELECTIONS)
    {
    if (a->GetNumberOfChildren() != b->GetNumberOfChildren())
      {
      cerr << "ERROR: Number of children does not match." << endl;
      errors++;
      }
    else
      {
      for (unsigned int cc=0; cc < a->GetNumberOfChildren(); cc++)
        {
        errors += SelectionCompare(a->GetChild(cc), b->GetChild(cc));
        }
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
  append->AddInput(input1);
  append->AddInput(input2);
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
  VTK_CREATE(vtkIdTypeArray, sel1Arr);
  sel1->SetContentType(vtkSelection::INDICES);
  sel1->SetFieldType(vtkSelection::CELL);
  sel1->SetSelectionList(sel1Arr);
  sel1Arr->InsertNextValue(0);
  sel1Arr->InsertNextValue(1);
  sel1Arr->InsertNextValue(2);
  VTK_CREATE(vtkSelection, sel2);
  VTK_CREATE(vtkIdTypeArray, sel2Arr);
  sel2->SetContentType(vtkSelection::INDICES);
  sel2->SetFieldType(vtkSelection::CELL);
  sel2->SetSelectionList(sel2Arr);
  sel2Arr->InsertNextValue(3);
  sel2Arr->InsertNextValue(4);
  sel2Arr->InsertNextValue(5);
  VTK_CREATE(vtkSelection, selAppend);
  VTK_CREATE(vtkIdTypeArray, selAppendArr);
  selAppend->SetContentType(vtkSelection::INDICES);
  selAppend->SetFieldType(vtkSelection::CELL);
  selAppend->SetSelectionList(selAppendArr);
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
  VTK_CREATE(vtkIdTypeArray, sel1Arr);
  sel1Arr->SetName("arrayname");
  sel1->SetContentType(vtkSelection::VALUES);
  sel1->SetFieldType(vtkSelection::CELL);
  sel1->SetSelectionList(sel1Arr);
  sel1Arr->InsertNextValue(0);
  sel1Arr->InsertNextValue(1);
  sel1Arr->InsertNextValue(2);
  VTK_CREATE(vtkSelection, sel2);
  VTK_CREATE(vtkIdTypeArray, sel2Arr);
  sel2Arr->SetName("arrayname");
  sel2->SetContentType(vtkSelection::VALUES);
  sel2->SetFieldType(vtkSelection::CELL);
  sel2->SetSelectionList(sel2Arr);
  sel2Arr->InsertNextValue(3);
  sel2Arr->InsertNextValue(4);
  sel2Arr->InsertNextValue(5);
  VTK_CREATE(vtkSelection, selAppend);
  VTK_CREATE(vtkIdTypeArray, selAppendArr);
  selAppendArr->SetName("arrayname");
  selAppend->SetContentType(vtkSelection::VALUES);
  selAppend->SetFieldType(vtkSelection::CELL);
  selAppend->SetSelectionList(selAppendArr);
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
  VTK_CREATE(vtkIdTypeArray, sel1Arr);
  sel1->SetContentType(vtkSelection::INDICES);
  sel1->SetFieldType(vtkSelection::CELL);
  sel1->SetSelectionList(sel1Arr);
  sel1->GetProperties()->Set(vtkSelection::PROCESS_ID(), 0);
  sel1Arr->InsertNextValue(0);
  sel1Arr->InsertNextValue(1);
  sel1Arr->InsertNextValue(2);
  VTK_CREATE(vtkSelection, sel2);
  VTK_CREATE(vtkIdTypeArray, sel2Arr);
  sel2->SetContentType(vtkSelection::INDICES);
  sel2->SetFieldType(vtkSelection::CELL);
  sel2->SetSelectionList(sel2Arr);
  sel2->GetProperties()->Set(vtkSelection::PROCESS_ID(), 1);
  sel2Arr->InsertNextValue(3);
  sel2Arr->InsertNextValue(4);
  sel2Arr->InsertNextValue(5);
  VTK_CREATE(vtkSelection, selAppend);
  VTK_CREATE(vtkSelection, sel1Clone);
  VTK_CREATE(vtkSelection, sel2Clone);
  selAppend->SetContentType(vtkSelection::SELECTIONS);
  sel1Clone->DeepCopy(sel1);
  sel2Clone->DeepCopy(sel2);
  selAppend->AddChild(sel1Clone);
  selAppend->AddChild(sel2Clone);
  errors += TestAppendSelectionCase(sel1, sel2, selAppend);
  cerr << "... done." << endl;
  }

  return errors;
}

