// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAMRInformation
 * @brief   Meta data that describes the structure of an AMR data seit
 * @deprecated Use vtkOverlappingAMRMetaData instead
 *
 * vtkAMRInformation encapsulates the following meta information for an AMR data set
 * - a list of vtkAMRBox objects
 * - Refinement ratio between AMR levels
 * - Grid spacing for each level
 * - The file block index for each block
 * - parent child information, if requested
 *
 * @sa
 * vtkOverlappingAMR, vtkAMRBox
 */

#ifndef vtkAMRInformation_h
#define vtkAMRInformation_h

#include "vtkAMRBox.h"                //for storing AMR Boxes
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDeprecation.h"           //for VTK_DEPRECATED_IN_9_6_0
#include "vtkOverlappingAMRMetaData.h"
#include "vtkSmartPointer.h" //for ivars

#include <vector> //for storing AMR Boxes

VTK_DEPRECATED_IN_9_6_0("Use `std::vector<vtkAMRBox>` instead.")
typedef std::vector<vtkAMRBox> vtkAMRBoxList;

VTK_ABI_NAMESPACE_BEGIN
class VTK_DEPRECATED_IN_9_6_0("Please use `vtkOverlappingAMRMetaData` or `vtkAMRMetaData` instead.")
  VTKCOMMONDATAMODEL_EXPORT vtkAMRInformation : public vtkOverlappingAMRMetaData
{
public:
  static vtkAMRInformation* New();
  vtkTypeMacro(vtkAMRInformation, vtkOverlappingAMRMetaData);

  bool operator==(const vtkAMRInformation& other) const
  {
    return this->Superclass::operator==(other);
  };

private:
  vtkAMRInformation() = default;
  ~vtkAMRInformation() override = default;
  vtkAMRInformation(const vtkAMRInformation&) = delete;
  void operator=(const vtkAMRInformation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
