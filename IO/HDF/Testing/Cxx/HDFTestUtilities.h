// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef HDFTestUtilities_h
#define HDFTestUtilities_h

#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPartitionedDataSetCollectionAlgorithm.h"

namespace HDFTestUtilities
{

/**
 * Simple filter that adds a vtkDataAssembly to a PDC that does not have one.
 * This can be removed when vtkGroupDataSetsFilter will support generating an assembly automatically
 * for PartitionedDataSetCollections (see https://gitlab.kitware.com/vtk/vtk/-/issues/19650)
 */
class vtkAddAssembly : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkAddAssembly* New();
  vtkTypeMacro(vtkAddAssembly, vtkPartitionedDataSetCollectionAlgorithm);

protected:
  int RequestData(
    vtkInformation* request, vtkInformationVector** inVector, vtkInformationVector* ouInfo) override
  {
    this->vtkPartitionedDataSetCollectionAlgorithm::RequestData(request, inVector, ouInfo);
    auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(vtkDataObject::GetData(ouInfo, 0));
    auto input = vtkPartitionedDataSetCollection::SafeDownCast(
      inVector[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));

    vtkNew<vtkDataAssembly> hierarchy;
    vtkDataAssemblyUtilities::GenerateHierarchy(input, hierarchy, pdc);
    return 1;
  }
};
}
#endif
