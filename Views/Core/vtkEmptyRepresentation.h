// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkEmptyRepresentation
 *
 *
 */

#ifndef vtkEmptyRepresentation_h
#define vtkEmptyRepresentation_h

#include "vtkDataRepresentation.h"
#include "vtkSmartPointer.h"    // For SP ivars
#include "vtkViewsCoreModule.h" // For export macro
#include "vtkWrappingHints.h"   // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkConvertSelectionDomain;

class VTKVIEWSCORE_EXPORT VTK_MARSHALAUTO vtkEmptyRepresentation : public vtkDataRepresentation
{
public:
  static vtkEmptyRepresentation* New();
  vtkTypeMacro(vtkEmptyRepresentation, vtkDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Since this representation has no inputs, override superclass
   * implementation with one that ignores "port" and "conn" and still allows it
   * to have an annotation output.
   */
  vtkAlgorithmOutput* GetInternalAnnotationOutputPort() override
  {
    return this->GetInternalAnnotationOutputPort(0);
  }
  vtkAlgorithmOutput* GetInternalAnnotationOutputPort(int port) override
  {
    return this->GetInternalAnnotationOutputPort(port, 0);
  }
  vtkAlgorithmOutput* GetInternalAnnotationOutputPort(int port, int conn) override;

protected:
  vtkEmptyRepresentation();
  ~vtkEmptyRepresentation() override;

private:
  vtkEmptyRepresentation(const vtkEmptyRepresentation&) = delete;
  void operator=(const vtkEmptyRepresentation&) = delete;

  vtkSmartPointer<vtkConvertSelectionDomain> ConvertDomains;
};

VTK_ABI_NAMESPACE_END
#endif
