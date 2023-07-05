// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkViewUpdater.h"

#include "vtkAnnotationLink.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderView.h"
#include "vtkView.h"

#include <algorithm>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkViewUpdater);

class vtkViewUpdater::vtkViewUpdaterInternals : public vtkCommand
{
public:
  void Execute(vtkObject*, unsigned long, void*) override
  {
    for (unsigned int i = 0; i < this->Views.size(); ++i)
    {
      vtkRenderView* rv = vtkRenderView::SafeDownCast(this->Views[i]);
      if (rv)
      {
        rv->Render();
      }
      else
      {
        this->Views[i]->Update();
      }
    }
  }

  std::vector<vtkView*> Views;
};

vtkViewUpdater::vtkViewUpdater()
{
  this->Internals = new vtkViewUpdaterInternals();
}

vtkViewUpdater::~vtkViewUpdater()
{
  this->Internals->Delete();
}

void vtkViewUpdater::AddView(vtkView* view)
{
  this->Internals->Views.push_back(view);
  // view->AddObserver(vtkCommand::SelectionChangedEvent, this->Internals);
}
void vtkViewUpdater::RemoveView(vtkView* view)
{
  std::vector<vtkView*>::iterator p;
  p = std::find(this->Internals->Views.begin(), this->Internals->Views.end(), view);
  if (p == this->Internals->Views.end())
    return;
  this->Internals->Views.erase(p);
}

void vtkViewUpdater::AddAnnotationLink(vtkAnnotationLink* link)
{
  link->AddObserver(vtkCommand::AnnotationChangedEvent, this->Internals);
}

void vtkViewUpdater::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
