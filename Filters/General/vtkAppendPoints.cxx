/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAppendPoints.h"

#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include <set>
#include <vector>

vtkStandardNewMacro(vtkAppendPoints);

//----------------------------------------------------------------------------
vtkAppendPoints::vtkAppendPoints()
{
  this->InputIdArrayName = 0;
}

//----------------------------------------------------------------------------
vtkAppendPoints::~vtkAppendPoints()
{
  this->SetInputIdArrayName(0);
}

//----------------------------------------------------------------------------
// This method is much too long, and has to be broken up!
// Append data sets into single polygonal data set.
int vtkAppendPoints::RequestData(vtkInformation *vtkNotUsed(request),
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Find common array names.
  vtkIdType totalPoints = 0;
  int numInputs = this->GetNumberOfInputConnections(0);
  bool first = true;
  std::set<std::string> arrayNames;
  for (int idx = 0; idx < numInputs; ++idx)
    {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(idx);
    vtkPolyData* input = vtkPolyData::SafeDownCast(
      inInfo->Get(vtkDataObject::DATA_OBJECT()));
    if (input && input->GetNumberOfPoints() > 0)
      {
      totalPoints += input->GetNumberOfPoints();
      if (first)
        {
        int numArrays = input->GetPointData()->GetNumberOfArrays();
        for (int a = 0; a < numArrays; ++a)
          {
          arrayNames.insert(input->GetPointData()->GetAbstractArray(a)->GetName());
          }
        first = false;
        }
      else
        {
        std::set<std::string> toErase;
        std::set<std::string>::iterator it, itEnd;
        itEnd = arrayNames.end();
        for (it = arrayNames.begin(); it != itEnd; ++it)
          {
          if (!input->GetPointData()->GetAbstractArray(it->c_str()))
            {
            toErase.insert(*it);
            }
          }
        itEnd = toErase.end();
        for (it = toErase.begin(); it != itEnd; ++it)
          {
          arrayNames.erase(*it);
          }
        }
      }
    }

  std::vector<vtkSmartPointer<vtkPolyData> > inputs;
  for (int idx = 0; idx < numInputs; ++idx)
    {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(idx);
    vtkPolyData* input = vtkPolyData::SafeDownCast(
      inInfo->Get(vtkDataObject::DATA_OBJECT()));
    if (input && input->GetNumberOfPoints() > 0)
      {
      vtkSmartPointer<vtkPolyData> copy =
        vtkSmartPointer<vtkPolyData>::New();
      copy->SetPoints(input->GetPoints());
      std::set<std::string>::iterator it, itEnd;
      itEnd = arrayNames.end();
      for (it = arrayNames.begin(); it != itEnd; ++it)
        {
        copy->GetPointData()->AddArray(
          input->GetPointData()->GetAbstractArray(it->c_str()));
        }
      inputs.push_back(copy);
      }
    else
      {
      inputs.push_back(0);
      }
    }

  vtkPointData* pd = 0;
  vtkIdType index = 0;
  vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
  pts->SetNumberOfPoints(totalPoints);
  vtkSmartPointer<vtkIntArray> idArr;
  if (this->InputIdArrayName)
    {
    idArr = vtkSmartPointer<vtkIntArray>::New();
    idArr->SetName(this->InputIdArrayName);
    idArr->SetNumberOfTuples(totalPoints);
    }
  for (size_t idx = 0; idx < inputs.size(); ++idx)
    {
    vtkPolyData* input = inputs[idx];
    if (input)
      {
      vtkPointData* ipd = input->GetPointData();
      if (!pd)
        {
        pd = output->GetPointData();
        pd->CopyAllocate(ipd, totalPoints);
        }
      vtkIdType numPoints = input->GetNumberOfPoints();
      for (vtkIdType i = 0; i < numPoints; ++i)
        {
        pd->CopyData(ipd, i, index);
        pts->InsertPoint(index, input->GetPoint(i));
        if (this->InputIdArrayName)
          {
          idArr->InsertValue(index, static_cast<int>(idx));
          }
        ++index;
        }
      }
    }
  output->SetPoints(pts);
  if (this->InputIdArrayName)
    {
    output->GetPointData()->AddArray(idArr);
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkAppendPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputIdArrayName: "
     << (this->InputIdArrayName ? this->InputIdArrayName : "(none)") << endl;
}

//----------------------------------------------------------------------------
int vtkAppendPoints::FillInputPortInformation(int port, vtkInformation *info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}
