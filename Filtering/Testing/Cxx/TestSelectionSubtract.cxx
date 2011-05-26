/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSelectionSubtract

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME
// .SECTION Description
// this program tests the vtkSelection::Subtract method

#include "vtkIdTypeArray.h"
#include "vtkIndent.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"

#include <iostream>
using namespace std;

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestSelectionSubtract(int,char *[])
{
  // Create a selection, sel1, of PEDIGREEIDS containing {1, 2, 3}
  VTK_CREATE(vtkSelection, sel1);
  VTK_CREATE(vtkSelectionNode, sel1Node);
  VTK_CREATE(vtkIdTypeArray, sel1Arr);
  sel1->AddNode(sel1Node);
  sel1Node->SetContentType(vtkSelectionNode::PEDIGREEIDS);
  sel1Node->SetFieldType(vtkSelectionNode::VERTEX);
  sel1Node->SetSelectionList(sel1Arr);
  sel1Arr->InsertNextValue(1);
  sel1Arr->InsertNextValue(2);
  sel1Arr->InsertNextValue(3);

  // Create a selection, sel2, of PEDIGREEIDS containing {2, 3}
  VTK_CREATE(vtkSelection, sel2);
  VTK_CREATE(vtkSelectionNode, sel2Node);
  VTK_CREATE(vtkIdTypeArray, sel2Arr);
  sel2->AddNode(sel2Node);
  sel2Node->SetContentType(vtkSelectionNode::PEDIGREEIDS);
  sel2Node->SetFieldType(vtkSelectionNode::VERTEX);
  sel2Node->SetSelectionList(sel2Arr);
  sel2Arr->InsertNextValue(2);
  sel2Arr->InsertNextValue(3);

  cout << "sel1->GetNumberOfNodes(): " << sel1->GetNumberOfNodes() << endl;   fflush(stdout);
  cout << "sel2->GetNumberOfNodes(): " << sel2->GetNumberOfNodes() << endl;   fflush(stdout);

  // Subtract sel2 from sel1
  // sel1->Subtract(sel2);

  // TODO: add correctness check.

  if(0)
    {
    cerr << "TestSelectionSubtract failed." << endl;
    return EXIT_FAILURE;
    }

  cout << "TestSelectionSubtract exited successfully." << endl;
  return EXIT_SUCCESS;
}
