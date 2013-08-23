/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h" // Need to be first and used for Py_xxx macros
#include "vtkWebUtilities.h"

#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkJavaScriptDataWriter.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSplitColumnComponents.h"
#include "vtkTable.h"

#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkWebUtilities);
//----------------------------------------------------------------------------
vtkWebUtilities::vtkWebUtilities()
{
}

//----------------------------------------------------------------------------
vtkWebUtilities::~vtkWebUtilities()
{
}

//----------------------------------------------------------------------------
std::string vtkWebUtilities::WriteAttributesToJavaScript(
  int field_type, vtkDataSet* dataset)
{
  if (dataset == NULL || (
      field_type != vtkDataObject::POINT &&
      field_type != vtkDataObject::CELL) )
    {
    return "[]";
    }

  vtksys_ios::ostringstream stream;

  vtkNew<vtkDataSetAttributes> clone;
  clone->PassData(dataset->GetAttributes(field_type));
  clone->RemoveArray("vtkValidPointMask");

  vtkNew<vtkTable> table;
  table->SetRowData(clone.GetPointer());

  vtkNew<vtkSplitColumnComponents> splitter;
  splitter->SetInputDataObject(table.GetPointer());
  splitter->Update();

  vtkNew<vtkJavaScriptDataWriter> writer;
  writer->SetOutputStream(&stream);
  writer->SetInputDataObject(splitter->GetOutputDataObject(0));
  writer->SetVariableName(NULL);
  writer->SetIncludeFieldNames(false);
  writer->Write();

  return stream.str();
}

//----------------------------------------------------------------------------
std::string vtkWebUtilities::WriteAttributeHeadersToJavaScript(
  int field_type, vtkDataSet* dataset)
{
  if (dataset == NULL || (
      field_type != vtkDataObject::POINT &&
      field_type != vtkDataObject::CELL) )
    {
    return "[]";
    }

  vtksys_ios::ostringstream stream;
  stream << "[";

  vtkDataSetAttributes* dsa = dataset->GetAttributes(field_type);
  vtkNew<vtkDataSetAttributes> clone;
  clone->CopyAllocate(dsa, 0);
  clone->RemoveArray("vtkValidPointMask");

  vtkNew<vtkTable> table;
  table->SetRowData(clone.GetPointer());

  vtkNew<vtkSplitColumnComponents> splitter;
  splitter->SetInputDataObject(table.GetPointer());
  splitter->Update();

  dsa = vtkTable::SafeDownCast(
    splitter->GetOutputDataObject(0))->GetRowData();

  for (int cc=0; cc < dsa->GetNumberOfArrays(); cc++)
    {
    const char* name = dsa->GetArrayName(cc);
    if (cc != 0)
      {
      stream << ", ";
      }
    stream << "\"" << (name? name : "") << "\"";
    }
  stream << "]";
  return stream.str();
}

//----------------------------------------------------------------------------
void vtkWebUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkWebUtilities::ProcessRMIs()
{
  vtkWebUtilities::ProcessRMIs(1,0);
}

//----------------------------------------------------------------------------
void vtkWebUtilities::ProcessRMIs(int reportError, int dont_loop)
{
  Py_BEGIN_ALLOW_THREADS
  vtkMultiProcessController::GetGlobalController()->ProcessRMIs(reportError, dont_loop);
  Py_END_ALLOW_THREADS
}
