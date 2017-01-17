/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPYoungsMaterialInterface.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPYoungsMaterialInterface
 * @brief   parallel reconstruction of material interfaces
 *
 *
 * This is a subclass of vtkYoungsMaterialInterface, implementing the reconstruction
 * of material interfaces, for parallel data sets
 *
 * @par Thanks:
 * This file is part of the generalized Youngs material interface reconstruction algorithm contributed by <br>
 * CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br>
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

class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkPYoungsMaterialInterface : public vtkYoungsMaterialInterface
{
public:
  static vtkPYoungsMaterialInterface* New();
  vtkTypeMacro(vtkPYoungsMaterialInterface,vtkYoungsMaterialInterface);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Parallel implementation of the material aggregation.
   */
  void Aggregate ( int, int* ) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the multiprocess controller. If no controller is set,
   * single process is assumed.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

protected:
  vtkPYoungsMaterialInterface ();
  ~vtkPYoungsMaterialInterface () VTK_OVERRIDE;

  vtkMultiProcessController* Controller;

private:
  vtkPYoungsMaterialInterface(const vtkPYoungsMaterialInterface&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPYoungsMaterialInterface&) VTK_DELETE_FUNCTION;
};

#endif /* VTK_PYOUNGS_MATERIAL_INTERFACE_H */
