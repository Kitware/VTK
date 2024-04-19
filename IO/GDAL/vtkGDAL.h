// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGDAL
 * @brief   Shared data for GDAL classes
 *
 */

#ifndef vtkGDAL_h
#define vtkGDAL_h

#include "vtkObject.h"
#include <vtkIOGDALModule.h> // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkInformationStringKey;
class vtkInformationIntegerVectorKey;

class VTKIOGDAL_EXPORT vtkGDAL : public vtkObject
{
public:
  vtkTypeMacro(vtkGDAL, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  // Key used to put GDAL map projection string in the output information
  // by readers.
  static vtkInformationStringKey* MAP_PROJECTION();
  static vtkInformationIntegerVectorKey* FLIP_AXIS();

protected:
private:
  vtkGDAL(); // Static class
  ~vtkGDAL() override;
  vtkGDAL(const vtkGDAL&) = delete;
  void operator=(const vtkGDAL&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkGDAL_h
