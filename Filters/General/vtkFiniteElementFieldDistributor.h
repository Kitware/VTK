/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFiniteElementFieldDistributor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkFiniteElementFieldDistributor.h
 * @brief Distribute cell-centered finite element fields from the input dataset onto vtk cell
 * points.
 *
 */

#ifndef vtkFiniteElementFieldDistributor_h
#define vtkFiniteElementFieldDistributor_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPartitionedDataSetCollectionAlgorithm.h"

#include <memory> // for std::unique_ptr

class VTKFILTERSGENERAL_EXPORT vtkFiniteElementFieldDistributor
  : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkFiniteElementFieldDistributor* New();
  vtkTypeMacro(vtkFiniteElementFieldDistributor, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkFiniteElementFieldDistributor();
  ~vtkFiniteElementFieldDistributor() override;

  // vtkAlgorithm interface
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkFiniteElementFieldDistributor(const vtkFiniteElementFieldDistributor&) = delete;
  void operator=(const vtkFiniteElementFieldDistributor&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
