// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkRectilinearGridToPointSet
 * @brief   Converts a vtkRectilinearGrid to a vtkPointSet
 *
 *
 * vtkRectilinearGridToPointSet takes a vtkRectilinearGrid as an image and
 * outputs an equivalent vtkStructuredGrid (which is a subclass of
 * vtkPointSet).
 *
 * @par Thanks:
 * This class was developed by Kenneth Moreland (kmorel@sandia.gov) from
 * Sandia National Laboratories.
 */

#ifndef vtkRectilinearGridToPointSet_h
#define vtkRectilinearGridToPointSet_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkStructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkRectilinearGrid;
class vtkStructuredData;

class VTKFILTERSGENERAL_EXPORT vtkRectilinearGridToPointSet : public vtkStructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkRectilinearGridToPointSet, vtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkRectilinearGridToPointSet* New();

protected:
  vtkRectilinearGridToPointSet();
  ~vtkRectilinearGridToPointSet() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkRectilinearGridToPointSet(const vtkRectilinearGridToPointSet&) = delete;
  void operator=(const vtkRectilinearGridToPointSet&) = delete;

  int CopyStructure(vtkStructuredGrid* outData, vtkRectilinearGrid* inData);
};

VTK_ABI_NAMESPACE_END
#endif // vtkRectilinearGridToPointSet_h
