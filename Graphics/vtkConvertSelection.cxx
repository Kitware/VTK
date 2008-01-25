/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConvertSelection.cxx

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
#include "vtkConvertSelection.h"

#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataArrayTemplate.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkExtractSelectedThresholds.h"
#include "vtkExtractSelection.h"
#include "vtkFieldData.h"
#include "vtkGraph.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSignedCharArray.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtksys/stl/algorithm>
#include <vtksys/stl/iterator>
#include <vtksys/stl/set>

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkCxxSetObjectMacro(vtkConvertSelection, ArrayNames, vtkStringArray);

vtkCxxRevisionMacro(vtkConvertSelection, "1.8");
vtkStandardNewMacro(vtkConvertSelection);
//----------------------------------------------------------------------------
vtkConvertSelection::vtkConvertSelection()
{
  this->SetNumberOfInputPorts(2);
  this->OutputType = vtkSelection::INDICES;
  this->ArrayNames = 0;
}

//----------------------------------------------------------------------------
vtkConvertSelection::~vtkConvertSelection()
{
  this->SetArrayNames(0);
}

//----------------------------------------------------------------------------
void vtkConvertSelection::SetArrayName(const char* name)
{
  if (!this->ArrayNames)
    {
    this->ArrayNames = vtkStringArray::New();
    }
  this->ArrayNames->Initialize();
  this->ArrayNames->InsertNextValue(name);
}

//----------------------------------------------------------------------------
const char* vtkConvertSelection::GetArrayName()
{
  if (this->ArrayNames && this->ArrayNames->GetNumberOfValues() > 0)
    {
    return this->ArrayNames->GetValue(0);
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkConvertSelection::SelectTableFromTable(
  vtkTable* selTable, 
  vtkTable* dataTable, 
  vtkIdTypeArray* indices)
{
  vtksys_stl::set<vtkIdType> matching;
  VTK_CREATE(vtkIdList, list);
  for (vtkIdType row = 0; row < selTable->GetNumberOfRows(); row++)
    {
    matching.clear();
    bool initialized = false;
    for (vtkIdType col = 0; col < selTable->GetNumberOfColumns(); col++)
      {
      vtkAbstractArray* from = selTable->GetColumn(col);
      vtkAbstractArray* to = dataTable->GetColumnByName(from->GetName());
      if (to)
        {
        to->LookupValue(selTable->GetValue(row, col), list);
        if (!initialized)
          {
          matching.insert(
            list->GetPointer(0), 
            list->GetPointer(0) + list->GetNumberOfIds());
          initialized = true;
          }
        else
          {
          vtksys_stl::set<vtkIdType> intersection;
          vtksys_stl::sort(list->GetPointer(0), list->GetPointer(0) + list->GetNumberOfIds());
          vtksys_stl::set_intersection(
            matching.begin(), matching.end(), 
            list->GetPointer(0), list->GetPointer(0) + list->GetNumberOfIds(), 
            vtksys_stl::inserter(intersection, intersection.begin()));
          matching = intersection;
          }
        }
      }
    vtksys_stl::set<vtkIdType>::iterator it, itEnd = matching.end();
    for (it = matching.begin(); it != itEnd; ++it)
      {
      indices->InsertNextValue(*it);
      }
    if (row % 100 == 0)
      {
      double progress = 0.8 * row / selTable->GetNumberOfRows();
      this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkConvertSelection::ConvertToIndexSelection(
  vtkSelection* input, 
  vtkDataSet* data,
  vtkSelection* output)
{
  // Change the input to preserve topology
  vtkSelection* selTemp = vtkSelection::New();
  selTemp->ShallowCopy(input);
  selTemp->GetProperties()->Set(vtkSelection::PRESERVE_TOPOLOGY(), true);
  
  // Use the extraction filter to create an insidedness array.
  vtkExtractSelection* const extract = vtkExtractSelection::New();
  extract->SetInput(0, data);
  extract->SetInput(1, selTemp);
  extract->Update();
  vtkDataSet* const extracted = extract->GetOutput();
  selTemp->Delete();
  
  output->GetProperties()->Set(vtkSelection::CONTENT_TYPE(), vtkSelection::INDICES);
  int type = input->GetProperties()->Get(vtkSelection::FIELD_TYPE());
  output->GetProperties()->Set(vtkSelection::FIELD_TYPE(), type);
  vtkSignedCharArray* insidedness = 0;
  if (type == vtkSelection::CELL)
    {
    insidedness = vtkSignedCharArray::SafeDownCast(
      extracted->GetCellData()->GetAbstractArray("vtkInsidedness"));
    }
  else if (type == vtkSelection::POINT)
    {
    insidedness = vtkSignedCharArray::SafeDownCast(
      extracted->GetPointData()->GetAbstractArray("vtkInsidedness"));
    }
  else
    {
    vtkErrorMacro("Unknown field type");
    extract->Delete();
    return 0;
    }
  
  if (!insidedness)
    {
    vtkErrorMacro("Did not find expected vtkInsidedness array.");
    extract->Delete();
    return 0;
    }
  
  // Convert the insidedness array into an index input.
  vtkIdTypeArray* indexArray = vtkIdTypeArray::New();
  for (vtkIdType i = 0; i < insidedness->GetNumberOfTuples(); i++)
    {
    if (insidedness->GetValue(i) == 1)
      {
      indexArray->InsertNextValue(i);
      }
    }
  output->SetSelectionList(indexArray);
  indexArray->Delete();
  extract->Delete();
  return 1;
}

//----------------------------------------------------------------------------
template <class T>
void vtkConvertSelectionLookup(
  T* selArr, 
  T* dataArr, 
  vtkIdTypeArray* indices)
{
  vtkIdType numTuples = selArr->GetNumberOfTuples();
  VTK_CREATE(vtkIdList, list);
  for (vtkIdType i = 0; i < numTuples; i++)
    {
    dataArr->LookupValue(selArr->GetValue(i), list);
    vtkIdType numIds = list->GetNumberOfIds();
    for (vtkIdType j = 0; j < numIds; j++)
      {
      indices->InsertNextValue(list->GetId(j));
      }
    }
}

//----------------------------------------------------------------------------
int vtkConvertSelection::Convert(
  vtkSelection* input,
  vtkDataObject* data,
  vtkSelection* output)
{
  // If it is an internal node, recurse
  if (input->GetContentType() == vtkSelection::SELECTIONS)
    {
    output->SetContentType(vtkSelection::SELECTIONS);
    for (unsigned int i = 0; i < input->GetNumberOfChildren(); ++i)
      {
      vtkSelection* inputChild = input->GetChild(i);
      VTK_CREATE(vtkSelection, outputChild);
      if (!this->Convert(inputChild, data, outputChild))
        {
        return 0;
        }
      output->AddChild(outputChild);
      }
    return 1;
    }
  
  // Start by shallow copying the selection and
  // setting the output content type.
  output->ShallowCopy(input);
  output->SetContentType(this->OutputType);
  
  // If it is the same type, we are done
  if (input->GetContentType() != vtkSelection::VALUES &&
      input->GetContentType() != vtkSelection::THRESHOLDS &&
      input->GetContentType() == this->OutputType)
    {
    return 1;
    }
  
  // If the input is a values or thresholds selection, we need array names
  // on the selection arrays to perform the selection.
  if (input->GetContentType() == vtkSelection::VALUES ||
      input->GetContentType() == vtkSelection::THRESHOLDS)
    {
    vtkFieldData* selData = input->GetSelectionData();
    for (int i = 0; i < selData->GetNumberOfArrays(); i++)
      {
      if (!selData->GetAbstractArray(i)->GetName())
        {
        vtkErrorMacro("Array name must be specified for values or thresholds selection.");
        return 0;
        }
      }
    }
  
  // If the output is a threshold selection, we need exactly one array name.
  if (this->OutputType == vtkSelection::THRESHOLDS &&
      (this->ArrayNames == 0 || this->ArrayNames->GetNumberOfValues() != 1))
    {
    vtkErrorMacro("One array name must be specified for thresholds selection.");
    return 0;
    }
  
  // If the output is a values selection, we need at lease one array name.
  if (this->OutputType == vtkSelection::VALUES &&
      (this->ArrayNames == 0 || this->ArrayNames->GetNumberOfValues() == 0))
    {
    vtkErrorMacro("At least one array name must be specified for values selection.");
    return 0;
    }
  
  // If we are converting a thresholds or values selection to
  // a selection on the same arrays, we are done.
  if ((input->GetContentType() == vtkSelection::VALUES ||
      input->GetContentType() == vtkSelection::THRESHOLDS) &&
      this->OutputType == input->GetContentType() &&
      this->ArrayNames->GetNumberOfValues() == input->GetSelectionData()->GetNumberOfArrays())
    {
    bool same = true;
    vtkFieldData* selData = input->GetSelectionData();
    for (int i = 0; i < selData->GetNumberOfArrays(); i++)
      {
      if (strcmp(selData->GetAbstractArray(i)->GetName(), this->ArrayNames->GetValue(i)))
        {
        same = false;
        break;
        }
      }
    if (same)
      {
      return 1;
      }
    }

  // Check whether we can do the conversion
  if (this->OutputType != vtkSelection::VALUES &&
      this->OutputType != vtkSelection::GLOBALIDS &&
      this->OutputType != vtkSelection::PEDIGREEIDS &&
      this->OutputType != vtkSelection::INDICES)
    {
    vtkErrorMacro("Cannot convert to type " << this->OutputType 
      << " unless input type matches.");
    return 0;
    }
  
  // Get the correct field data
  vtkFieldData* fd = 0;
  vtkDataSetAttributes* dsa = 0;
  if (vtkDataSet::SafeDownCast(data))
    {
    if (!input->GetProperties()->Has(vtkSelection::FIELD_TYPE()) ||
        input->GetFieldType() == vtkSelection::CELL)
      {
      dsa = vtkDataSet::SafeDownCast(data)->GetCellData();
      }
    else if (input->GetFieldType() == vtkSelection::POINT)
      {
      dsa = vtkDataSet::SafeDownCast(data)->GetPointData();
      }
    else if (input->GetFieldType() == vtkSelection::FIELD)
      {
      fd = data->GetFieldData();
      }
    else
      {
      vtkErrorMacro("Inappropriate selection type for a vtkDataSet");
      return 0;
      }
    }
  else if (vtkGraph::SafeDownCast(data))
    {
    if (!input->GetProperties()->Has(vtkSelection::FIELD_TYPE()) ||
        input->GetFieldType() == vtkSelection::EDGE)
      {
      dsa = vtkGraph::SafeDownCast(data)->GetEdgeData();
      }
    else if (input->GetFieldType() == vtkSelection::VERTEX)
      {
      dsa = vtkGraph::SafeDownCast(data)->GetVertexData();
      }
    else if (input->GetFieldType() == vtkSelection::FIELD)
      {
      fd = data->GetFieldData();
      }
    else
      {
      vtkErrorMacro("Inappropriate selection type for a vtkGraph");
      return 0;
      }
    }
  else
    {
    if (!input->GetProperties()->Has(vtkSelection::FIELD_TYPE()) ||
        input->GetFieldType() == vtkSelection::FIELD)
      {
      fd = data->GetFieldData();
      }
    else
      {
      vtkErrorMacro("Inappropriate selection type for a non-dataset, non-graph");
      return 0;
      }
    }
  
  //
  // First, convert the selection to a list of indices
  //
  
  VTK_CREATE(vtkIdTypeArray, indices);
  
  if (input->GetContentType() == vtkSelection::FRUSTUM || 
      input->GetContentType() == vtkSelection::LOCATIONS)
    {
    if (!vtkDataSet::SafeDownCast(data))
      {
      vtkErrorMacro("Can only convert from frustum or locations if the input is a vtkDataSet");
      return 0;
      }
    // Use the extract selection filter to create an index selection
    VTK_CREATE(vtkSelection, indexSelection);
    this->ConvertToIndexSelection(input, vtkDataSet::SafeDownCast(data), indexSelection);
    // TODO: We should shallow copy this, but the method is not defined.
    indices->DeepCopy(indexSelection->GetSelectionList());
    }
  else if (input->GetContentType() == vtkSelection::THRESHOLDS)
    {
    vtkDoubleArray *lims = vtkDoubleArray::SafeDownCast(input->GetSelectionList());
    if (!lims)
      {
      vtkErrorMacro("Thresholds selection requires vtkDoubleArray selection list.");
      return 0;
      }
    vtkDataArray *dataArr = 0;
    if (dsa)
      {
      dataArr = vtkDataArray::SafeDownCast(dsa->GetAbstractArray(lims->GetName()));
      }
    else if (fd)
      {
      dataArr = vtkDataArray::SafeDownCast(fd->GetAbstractArray(lims->GetName()));
      }
    if (!dataArr)
      {
      vtkErrorMacro("Could not find vtkDataArray for thresholds selection.");
      return 0;
      }
    int inverse = 0;
    if (input->GetProperties()->Has(vtkSelection::INVERSE()))
      {
      inverse = input->GetProperties()->Get(vtkSelection::INVERSE());
      }
    for (vtkIdType id = 0; id < dataArr->GetNumberOfTuples(); id++)
      {
      int keepPoint = vtkExtractSelectedThresholds::EvaluateValue(dataArr, id, lims);
      if (keepPoint ^ inverse)
        {
        indices->InsertNextValue(id);
        }
      }
    }
  else if (input->GetContentType() == vtkSelection::INDICES)
    {
    // TODO: We should shallow copy this, but the method is not defined.
    indices->DeepCopy(input->GetSelectionList());
    }
  else if (input->GetContentType() == vtkSelection::VALUES)
    {
    vtkFieldData* selData = input->GetSelectionData();
    VTK_CREATE(vtkTable, selTable);
    selTable->SetFieldData(selData);
    VTK_CREATE(vtkTable, dataTable);
    for (vtkIdType col = 0; col < selTable->GetNumberOfColumns(); col++)
      {
      vtkAbstractArray* dataArr = 0;
      if (dsa)
        {
        dataArr = dsa->GetAbstractArray(selTable->GetColumn(col)->GetName());
        }
      else if (fd)
        {
        dataArr = fd->GetAbstractArray(selTable->GetColumn(col)->GetName());
        }
      if (dataArr)
        {
        dataTable->AddColumn(dataArr);
        }
      }
    // Select rows matching selTable from the input dataTable
    // and put the matches in the index array.
    this->SelectTableFromTable(selTable, dataTable, indices);
    }
  else if (input->GetContentType() == vtkSelection::PEDIGREEIDS ||
           input->GetContentType() == vtkSelection::GLOBALIDS)
    {
    // Get the appropriate array
    vtkAbstractArray* selArr = input->GetSelectionList();
    vtkAbstractArray* dataArr = 0;
    if (dsa && input->GetContentType() == vtkSelection::PEDIGREEIDS)
      {
      dataArr = dsa->GetPedigreeIds();
      }
    else if (dsa && input->GetContentType() == vtkSelection::GLOBALIDS)
      {
      dataArr = dsa->GetGlobalIds();
      }
    else if (fd && selArr->GetName())
      {
      // Since data objects only have field data which does not have attributes,
      // use the array name to try to match the incoming selection's array.
      dataArr = fd->GetAbstractArray(selArr->GetName());
      }
    else
      {
      vtkErrorMacro("Tried to use array name to match global or pedigree ids on data object,"
        << "but name not set on selection array.");
      return 0;
      }
    
    // Check array compatibility
    if (!dataArr)
      {
      vtkErrorMacro("Selection array does not exist in input dataset.");
      return 0;
      }
    if (dataArr->GetDataType() != selArr->GetDataType())
      {
      vtkErrorMacro("Selection array type does not match input dataset array type.");
      return 0;
      }
    
    // Perform the lookup
    switch (selArr->GetDataType())
      {
      vtkTemplateMacro(vtkConvertSelectionLookup(
        static_cast<vtkDataArrayTemplate<VTK_TT>*>(selArr), 
        static_cast<vtkDataArrayTemplate<VTK_TT>*>(dataArr),
        indices));
      case VTK_STRING:
        vtkConvertSelectionLookup(
          vtkStringArray::SafeDownCast(selArr), 
          vtkStringArray::SafeDownCast(dataArr), indices);
        break;
      case VTK_VARIANT:
        vtkConvertSelectionLookup(
          vtkVariantArray::SafeDownCast(selArr), 
          vtkVariantArray::SafeDownCast(dataArr), indices);
        break;
      default:
        vtkErrorMacro("Unsupported data type " << selArr->GetDataType());
        return 0;
      }
    }
  
  double progress = 0.8;
  this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
  
  //
  // Now that we have the list of indices, convert the selection by indexing
  // values in another array.
  //
  
  // If it is an index selection, we are done.
  if (this->OutputType == vtkSelection::INDICES)
    {
    output->SetSelectionList(indices);
    return 1;
    }
  
  vtkIdType numOutputArrays = 1;
  if (this->OutputType == vtkSelection::VALUES)
    {
    numOutputArrays = this->ArrayNames->GetNumberOfValues();
    }
  
  VTK_CREATE(vtkFieldData, outputData);
  for (vtkIdType ind = 0; ind < numOutputArrays; ind++)
    {  
    // Find the output array where to get the output selection values.
    vtkAbstractArray* outputDataArr = 0;
    if (dsa && this->OutputType == vtkSelection::VALUES)
      {
      outputDataArr = dsa->GetAbstractArray(this->ArrayNames->GetValue(ind));
      }
    else if (fd && this->OutputType == vtkSelection::VALUES)
      {
      outputDataArr = fd->GetAbstractArray(this->ArrayNames->GetValue(ind));
      }
    else if (dsa && this->OutputType == vtkSelection::PEDIGREEIDS)
      {
      outputDataArr = dsa->GetPedigreeIds();
      }
    else if (dsa && this->OutputType == vtkSelection::GLOBALIDS)
      {
      outputDataArr = dsa->GetGlobalIds();
      }
    else
      {
      // TODO: Make this error go away.
      vtkErrorMacro("BUG: Currently you can only specify pedigree and global ids on a vtkDataSet.");
      return 0;
      }
    
    // Check array existence.
    if (!outputDataArr)
      {
      vtkErrorMacro("Output selection array does not exist in input dataset.");
      return 0;
      }
    
    // Put the array's values into the selection.
    vtkAbstractArray* outputArr = vtkAbstractArray::CreateArray(outputDataArr->GetDataType());
    if (this->OutputType == vtkSelection::VALUES)
      {
      outputArr->SetName(outputDataArr->GetName());
      }
    vtkIdType numTuples = outputDataArr->GetNumberOfTuples();
    vtkIdType numIndices = indices->GetNumberOfTuples();
    for (vtkIdType i = 0; i < numIndices; ++i)
      {
      vtkIdType index = indices->GetValue(i);
      if (index < numTuples)
        {
        outputArr->InsertNextTuple(index, outputDataArr);
        }
      else
        {
        // TODO: Make this a silent warning when we are done debugging
        vtkWarningMacro("Attempting to select an index outside the array range.");
        }
      if (i % 1000 == 0)
        {
        progress = 0.8 + (0.2 * (ind*numIndices + i)) / (numOutputArrays*numIndices);
        this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
        }
      }
    outputData->AddArray(outputArr);
    outputArr->Delete();
    }
  output->SetSelectionData(outputData);
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkConvertSelection::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkSelection* input = vtkSelection::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  vtkInformation* dataInfo = inputVector[1]->GetInformationObject(0);
  vtkDataObject* data = dataInfo->Get(vtkDataObject::DATA_OBJECT());
  
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkSelection* output = vtkSelection::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  return this->Convert(input, data, output);
}

//----------------------------------------------------------------------------
int vtkConvertSelection::FillInputPortInformation(
  int port, vtkInformation* info)
{
  // now add our info
  if (port == 0)
    {
    info->Set(vtkConvertSelection::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    }
  else if (port == 1)
    {
    info->Set(vtkConvertSelection::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
    }
  return 1;
}

//----------------------------------------------------------------------------
vtkSelection* vtkConvertSelection::ToIndexSelection(
  vtkSelection* input, 
  vtkDataObject* data)
{
  return vtkConvertSelection::ToSelectionType(input, data, vtkSelection::INDICES);
}

//----------------------------------------------------------------------------
vtkSelection* vtkConvertSelection::ToGlobalIdSelection(
  vtkSelection* input, 
  vtkDataObject* data)
{
  return vtkConvertSelection::ToSelectionType(input, data, vtkSelection::GLOBALIDS);
}

//----------------------------------------------------------------------------
vtkSelection* vtkConvertSelection::ToPedigreeIdSelection(
  vtkSelection* input, 
  vtkDataObject* data)
{
  return vtkConvertSelection::ToSelectionType(input, data, vtkSelection::PEDIGREEIDS);
}

//----------------------------------------------------------------------------
vtkSelection* vtkConvertSelection::ToValueSelection(
  vtkSelection* input, 
  vtkDataObject* data, 
  const char* arrayName)
{
  VTK_CREATE(vtkStringArray, names);
  names->InsertNextValue(arrayName);
  return vtkConvertSelection::ToSelectionType(input, data, vtkSelection::VALUES, names);
}

//----------------------------------------------------------------------------
vtkSelection* vtkConvertSelection::ToValueSelection(
  vtkSelection* input, 
  vtkDataObject* data, 
  vtkStringArray* arrayNames)
{
  return vtkConvertSelection::ToSelectionType(input, data, vtkSelection::VALUES, arrayNames);
}

//----------------------------------------------------------------------------
vtkSelection* vtkConvertSelection::ToSelectionType(
  vtkSelection* input, 
  vtkDataObject* data, 
  int type, 
  vtkStringArray* arrayNames)
{
  VTK_CREATE(vtkConvertSelection, convert);
  convert->SetInput(0, input);
  convert->SetInput(1, data);
  convert->SetOutputType(type);
  convert->SetArrayNames(arrayNames);
  convert->Update();
  vtkSelection* output = convert->GetOutput();
  output->Register(0);
  return output;
}

//----------------------------------------------------------------------------
void vtkConvertSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "OutputType: " << this->OutputType << endl;
  os << indent << "ArrayNames: " << (this->ArrayNames ? "" : "(null)") << endl;
  if (this->ArrayNames)
    {
    this->ArrayNames->PrintSelf(os, indent.GetNextIndent());
    }
}
