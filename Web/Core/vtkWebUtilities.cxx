// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebUtilities.h"
#include "vtkPython.h" // Need to be first and used for Py_xxx macros

#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkJavaScriptDataWriter.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSplitColumnComponents.h"
#include "vtkTable.h"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkWebUtilities);
//------------------------------------------------------------------------------
vtkWebUtilities::vtkWebUtilities() = default;

//------------------------------------------------------------------------------
vtkWebUtilities::~vtkWebUtilities() = default;

//------------------------------------------------------------------------------
std::string vtkWebUtilities::WriteAttributesToJavaScript(int field_type, vtkDataSet* dataset)
{
  if (dataset == nullptr ||
    (field_type != vtkDataObject::POINT && field_type != vtkDataObject::CELL))
  {
    return "[]";
  }

  std::ostringstream stream;

  vtkNew<vtkDataSetAttributes> clone;
  clone->PassData(dataset->GetAttributes(field_type));
  clone->RemoveArray("vtkValidPointMask");

  vtkNew<vtkTable> table;
  table->SetRowData(clone);

  vtkNew<vtkSplitColumnComponents> splitter;
  splitter->SetInputDataObject(table);
  splitter->Update();

  vtkNew<vtkJavaScriptDataWriter> writer;
  writer->SetOutputStream(&stream);
  writer->SetInputDataObject(splitter->GetOutputDataObject(0));
  writer->SetVariableName(nullptr);
  writer->SetIncludeFieldNames(false);
  writer->Write();

  return stream.str();
}

//------------------------------------------------------------------------------
std::string vtkWebUtilities::WriteAttributeHeadersToJavaScript(int field_type, vtkDataSet* dataset)
{
  if (dataset == nullptr ||
    (field_type != vtkDataObject::POINT && field_type != vtkDataObject::CELL))
  {
    return "[]";
  }

  std::ostringstream stream;
  stream << "[";

  vtkDataSetAttributes* dsa = dataset->GetAttributes(field_type);
  vtkNew<vtkDataSetAttributes> clone;
  clone->CopyAllocate(dsa, 0);
  clone->RemoveArray("vtkValidPointMask");

  vtkNew<vtkTable> table;
  table->SetRowData(clone);

  vtkNew<vtkSplitColumnComponents> splitter;
  splitter->SetInputDataObject(table);
  splitter->Update();

  dsa = vtkTable::SafeDownCast(splitter->GetOutputDataObject(0))->GetRowData();

  for (int cc = 0; cc < dsa->GetNumberOfArrays(); cc++)
  {
    const char* name = dsa->GetArrayName(cc);
    if (cc != 0)
    {
      stream << ", ";
    }
    stream << "\"" << (name ? name : "") << "\"";
  }
  stream << "]";
  return stream.str();
}

//------------------------------------------------------------------------------
void vtkWebUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkWebUtilities::ProcessRMIs()
{
  vtkWebUtilities::ProcessRMIs(1, 0);
}

//------------------------------------------------------------------------------
void vtkWebUtilities::ProcessRMIs(int reportError, int dont_loop)
{
  Py_BEGIN_ALLOW_THREADS

  vtkMultiProcessController::GetGlobalController()
    ->ProcessRMIs(reportError, dont_loop);

  Py_END_ALLOW_THREADS
}
VTK_ABI_NAMESPACE_END
