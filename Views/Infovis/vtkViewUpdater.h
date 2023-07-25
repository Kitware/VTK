// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkViewUpdater
 * @brief   Updates views automatically
 *
 *
 * vtkViewUpdater registers with annotation change events for a set of
 * annotation links, and updates all views when an annotation link fires an
 * annotation changed event. This is often needed when multiple views share
 * a selection with vtkAnnotationLink.
 */

#ifndef vtkViewUpdater_h
#define vtkViewUpdater_h

#include "vtkObject.h"
#include "vtkViewsInfovisModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkAnnotationLink;
class vtkView;

class VTKVIEWSINFOVIS_EXPORT vtkViewUpdater : public vtkObject
{
public:
  static vtkViewUpdater* New();
  vtkTypeMacro(vtkViewUpdater, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void AddView(vtkView* view);
  void RemoveView(vtkView* view);

  void AddAnnotationLink(vtkAnnotationLink* link);

protected:
  vtkViewUpdater();
  ~vtkViewUpdater() override;

private:
  vtkViewUpdater(const vtkViewUpdater&) = delete;
  void operator=(const vtkViewUpdater&) = delete;

  class vtkViewUpdaterInternals;
  vtkViewUpdaterInternals* Internals;
};

VTK_ABI_NAMESPACE_END
#endif
