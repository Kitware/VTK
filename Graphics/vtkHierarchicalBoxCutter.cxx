/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalBoxCutter.h"

#include "vtkAppendPolyData.h"
#include "vtkMultiBlockApplyFilterCommand.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataVisitor.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkCutter.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkHierarchicalBoxCutter, "1.2");
vtkStandardNewMacro(vtkHierarchicalBoxCutter);

//----------------------------------------------------------------------------
vtkHierarchicalBoxCutter::vtkHierarchicalBoxCutter()
{
  this->Cutter = vtkCutter::New();
}

//----------------------------------------------------------------------------
vtkHierarchicalBoxCutter::~vtkHierarchicalBoxCutter()
{
  this->Cutter->Delete();
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxCutter::ExecuteData(vtkDataObject* dooutput)
{
  vtkHierarchicalBoxDataSet* input = this->GetInput();

  vtkMultiBlockApplyFilterCommand* comm = vtkMultiBlockApplyFilterCommand::New();

  comm->SetFilter(this->Cutter);

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
// These are forwarded to the actual cutter
void vtkHierarchicalBoxCutter::SetValue(int i, double value)
{
  this->Cutter->SetValue(i, value);
}
double vtkHierarchicalBoxCutter::GetValue(int i)
{
  return this->Cutter->GetValue(i);
}
double* vtkHierarchicalBoxCutter::GetValues()
{
  return this->Cutter->GetValues();
}
void vtkHierarchicalBoxCutter::GetValues(double* contourValues)
{
  this->Cutter->GetValues(contourValues);
}
void vtkHierarchicalBoxCutter::SetNumberOfContours(int number)
{
  this->Cutter->SetNumberOfContours(number);
}
int vtkHierarchicalBoxCutter::GetNumberOfContours()
{
  return this->Cutter->GetNumberOfContours();
}
void vtkHierarchicalBoxCutter::GenerateValues(int numContours, double range[2])
{
  this->Cutter->GenerateValues(numContours, range);
}
void vtkHierarchicalBoxCutter::GenerateValues(
  int numContours, double rangeStart, double rangeEnd)
{
  this->Cutter->GenerateValues(numContours, rangeStart, rangeEnd);
}
void vtkHierarchicalBoxCutter::SetCutFunction(vtkImplicitFunction* func)
{
  this->Cutter->SetCutFunction(func);
}
vtkImplicitFunction* vtkHierarchicalBoxCutter::GetCutFunction()
{
  return this->Cutter->GetCutFunction();
}
unsigned long vtkHierarchicalBoxCutter::GetMTime()
{
  return this->Cutter->GetMTime();
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

