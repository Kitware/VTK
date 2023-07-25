// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLevelIdScalars
 * @brief   Empty class for backwards compatibility.
 */

#ifndef vtkLevelIdScalars_h
#define vtkLevelIdScalars_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkOverlappingAMRLevelIdScalars.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkLevelIdScalars : public vtkOverlappingAMRLevelIdScalars
{
public:
  static vtkLevelIdScalars* New();
  vtkTypeMacro(vtkLevelIdScalars, vtkOverlappingAMRLevelIdScalars);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkLevelIdScalars();
  ~vtkLevelIdScalars() override;

private:
  vtkLevelIdScalars(const vtkLevelIdScalars&) = delete;
  void operator=(const vtkLevelIdScalars&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* VTKLEVELIDSCALARS_H_ */
