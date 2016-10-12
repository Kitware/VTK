/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetGradient.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
/**
 * @class   vtkDataSetGradient
 * @brief   computes scalar field gradient
 *
 *
 * vtkDataSetGradient Computes per cell gradient of point scalar field
 * or per point gradient of cell scalar field.
 *
 * @par Thanks:
 * This file is part of the generalized Youngs material interface reconstruction algorithm contributed by
 * CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br>
 * BP12, F-91297 Arpajon, France. <br>
 * Implementation by Thierry Carrard (CEA)
*/

#ifndef vtkDataSetGradient_h
#define vtkDataSetGradient_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"


class VTKFILTERSGENERAL_EXPORT vtkDataSetGradient : public vtkDataSetAlgorithm
{
 public:
  static vtkDataSetGradient* New();
  vtkTypeMacro(vtkDataSetGradient,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the name of computed vector array.
   */
  vtkSetStringMacro(ResultArrayName);
  vtkGetStringMacro(ResultArrayName);
  //@}

 protected:
  vtkDataSetGradient ();
  ~vtkDataSetGradient() VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  char* ResultArrayName;

 private:
  vtkDataSetGradient(const vtkDataSetGradient&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDataSetGradient&) VTK_DELETE_FUNCTION;
};

#endif /* VTK_DATA_SET_GRADIENT_H */

