/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIOSSModel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkIOSSModel
 * @brief internal class used by vtkIOSSWriter
 *
 * vtkIOSSModel is a helper class used by vtkIOSSWriter. It helps us construct a
 * data structure more suitable for serializing to IOSS from a
 * vtkPartitionedDataSetCollection.
 */

#ifndef vtkIOSSModel_h
#define vtkIOSSModel_h

#include "vtkObject.h"
#include <memory> // for std::unique_ptr

// Ioss includes
#include <vtk_ioss.h>
// clang-format off
#include VTK_IOSS(Ioss_Region.h)
// clang-format on

VTK_ABI_NAMESPACE_BEGIN
class vtkIOSSWriter;
class vtkPartitionedDataSetCollection;

class vtkIOSSModel
{
public:
  vtkIOSSModel(vtkPartitionedDataSetCollection* pdc, vtkIOSSWriter* self);
  ~vtkIOSSModel();

  void DefineModel(Ioss::Region& region) const;
  void DefineTransient(Ioss::Region& region) const;
  void Model(Ioss::Region& region) const;
  void Transient(Ioss::Region& region, double time) const;

  /**
   * Generates an MD5 sum summarizing the model. This is used to test if the
   * model has changed enough so that it requires a redefinition.
   *
   * This is not perfect, but is a reasonable option for now.
   */
  std::string MD5() const;

  /**
   * Returns true if the global ids have been created for this model,
   * because they were not present in the input data.
   */
  bool GlobalIdsCreated() const;

private:
  vtkIOSSModel(const vtkIOSSModel&) = delete;
  void operator=(const vtkIOSSModel&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkIOSSModel.h
