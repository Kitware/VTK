/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxContour.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalBoxContour.h"

#include "vtkAppendPolyData.h"
#include "vtkMultiBlockApplyFilterCommand.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataVisitor.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkContourFilter.h"
#include "vtkPolyData.h"



vtkCxxRevisionMacro(vtkHierarchicalBoxContour, "1.1");
vtkStandardNewMacro(vtkHierarchicalBoxContour);

//----------------------------------------------------------------------------
vtkHierarchicalBoxContour::vtkHierarchicalBoxContour()
{
  this->Contour = vtkContourFilter::New();
}

//----------------------------------------------------------------------------
vtkHierarchicalBoxContour::~vtkHierarchicalBoxContour()
{
  this->Contour->Delete();
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxContour::ExecuteData(vtkDataObject* dooutput)
{
  vtkHierarchicalBoxDataSet* input = this->GetInput();

  vtkMultiBlockApplyFilterCommand* comm = vtkMultiBlockApplyFilterCommand::New();

  comm->SetFilter(this->Contour);

  vtkCompositeDataVisitor* visitor = input->NewVisitor();
  visitor->SetCommand(comm);

  visitor->Execute();

  vtkMultiBlockDataSet* output = comm->GetOutput();

  vtkAppendPolyData* append = vtkAppendPolyData::New();

  vtkCompositeDataIterator* iter = output->NewIterator();
  iter->GoToFirstItem();
  while (!iter->IsDoneWithTraversal())
    {
    append->AddInput(vtkPolyData::SafeDownCast(iter->GetCurrentDataObject()));
    iter->GoToNextItem();
    }
  iter->Delete();
  
  append->Update();
  vtkPolyData::SafeDownCast(dooutput)->ShallowCopy(append->GetOutput());
  append->Delete();

  comm->Delete();
  visitor->Delete();
}

//----------------------------------------------------------------------------
// Delegate to contour
void vtkHierarchicalBoxContour::SetValue(int i, double value)
{
  this->Contour->SetValue(i, value);
}
double vtkHierarchicalBoxContour::GetValue(int i)
{
  return this->Contour->GetValue(i);
}
double* vtkHierarchicalBoxContour::GetValues()
{
  return this->Contour->GetValues();
}
void vtkHierarchicalBoxContour::GetValues(double *contourValues)
{
  this->Contour->GetValues(contourValues);
}
void vtkHierarchicalBoxContour::SetNumberOfContours(int number)
{
  this->Contour->SetNumberOfContours(number);
}
int vtkHierarchicalBoxContour::GetNumberOfContours()
{
  return this->Contour->GetNumberOfContours();
}
unsigned long vtkHierarchicalBoxContour::GetMTime()
{
  return this->Contour->GetMTime();
}
const char* vtkHierarchicalBoxContour::GetInputScalarsSelection()
{
  return this->Contour->GetInputScalarsSelection();
}
void vtkHierarchicalBoxContour::SelectInputScalars(const char *fieldName) 
{
  this->Contour->SelectInputScalars(fieldName);
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxContour::PrintSelf(
  ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

