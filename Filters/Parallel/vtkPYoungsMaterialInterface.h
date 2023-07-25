// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPYoungsMaterialInterface
 * @brief   parallel reconstruction of material interfaces
 *
 *
 * This is a subclass of vtkYoungsMaterialInterface, implementing the reconstruction
 * of material interfaces, for parallel data sets
 *
 * @par Thanks:
 * This file is part of the generalized Youngs material interface reconstruction algorithm
 * contributed by <br> CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br>
 * BP12, F-91297 Arpajon, France. <br>
 * Implementation by Thierry Carrard and Philippe Pebay
 *
 * @sa
 * vtkYoungsMaterialInterface
 */

#ifndef vtkPYoungsMaterialInterface_h
#define vtkPYoungsMaterialInterface_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkYoungsMaterialInterface.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkPYoungsMaterialInterface : public vtkYoungsMaterialInterface
{
public:
  static vtkPYoungsMaterialInterface* New();
  vtkTypeMacro(vtkPYoungsMaterialInterface, vtkYoungsMaterialInterface);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Parallel implementation of the material aggregation.
   */
  void Aggregate(int, int*) override;

  ///@{
  /**
   * Get/Set the multiprocess controller. If no controller is set,
   * single process is assumed.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

protected:
  vtkPYoungsMaterialInterface();
  ~vtkPYoungsMaterialInterface() override;

  vtkMultiProcessController* Controller;

private:
  vtkPYoungsMaterialInterface(const vtkPYoungsMaterialInterface&) = delete;
  void operator=(const vtkPYoungsMaterialInterface&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* VTK_PYOUNGS_MATERIAL_INTERFACE_H */
