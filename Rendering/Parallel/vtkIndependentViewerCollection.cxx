// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkIndependentViewerCollection.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

#include <cassert>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
struct Viewer
{
  vtkNew<vtkMatrix4x4> EyeTransform;
  double EyeSeparation = 0.0;
};
};

class vtkIndependentViewerCollection::vtkInternals
{
public:
  vtkInternals() = default;
  ~vtkInternals() = default;

  Viewer& GetIndependentViewer(int i)
  {
    assert(i >= 0);
    size_t idx = static_cast<size_t>(i);

    if (idx >= this->IndependentViewers.size())
    {
      this->IndependentViewers.resize(idx + 1);
    }

    return this->IndependentViewers[idx];
  }

  std::vector<Viewer> IndependentViewers;
};

//----------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkIndependentViewerCollection);

//----------------------------------------------------------------------------
vtkIndependentViewerCollection::vtkIndependentViewerCollection()
  : Internals(new vtkIndependentViewerCollection::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkIndependentViewerCollection::~vtkIndependentViewerCollection() = default;

//----------------------------------------------------------------------------
void vtkIndependentViewerCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfIndependentViewers: " << this->Internals->IndependentViewers.size()
     << endl;
  for (size_t idx = 0; idx < this->Internals->IndependentViewers.size(); ++idx)
  {
    Viewer& viewer = this->Internals->GetIndependentViewer(static_cast<int>(idx));
    os << indent << "  EyeSeparation[" << idx << "]: " << viewer.EyeSeparation << endl;
    os << indent << "  ";
    viewer.EyeTransform->PrintSelf(os, indent);
  }
}

//----------------------------------------------------------------------------
int vtkIndependentViewerCollection::GetNumberOfIndependentViewers()
{
  return static_cast<int>(this->Internals->IndependentViewers.size());
}

//----------------------------------------------------------------------------
void vtkIndependentViewerCollection::SetNumberOfIndependentViewers(int n)
{
  assert(n >= 0);
  this->Internals->IndependentViewers.resize(n);
}

//----------------------------------------------------------------------------
int vtkIndependentViewerCollection::GetNumberOfEyeTransforms()
{
  return static_cast<int>(this->Internals->IndependentViewers.size());
}

//----------------------------------------------------------------------------
void vtkIndependentViewerCollection::SetNumberOfEyeTransforms(int n)
{
  assert(n >= 0);
  this->Internals->IndependentViewers.resize(n);
}

//----------------------------------------------------------------------------
void vtkIndependentViewerCollection::SetEyeTransform(int i, const std::vector<double>& vals)
{
  Viewer& viewer = this->Internals->GetIndependentViewer(i);
  size_t idx = 0;
  for (int row = 0; row < 4; ++row)
  {
    for (int col = 0; col < 4; ++col)
    {
      viewer.EyeTransform->SetElement(row, col, vals[idx++]);
    }
  }
}

//----------------------------------------------------------------------------
void vtkIndependentViewerCollection::SetEyeTransform(int i, const double* vals)
{
  Viewer& viewer = this->Internals->GetIndependentViewer(i);
  size_t idx = 0;
  for (int row = 0; row < 4; ++row)
  {
    for (int col = 0; col < 4; ++col)
    {
      viewer.EyeTransform->SetElement(row, col, vals[idx++]);
    }
  }
}

//----------------------------------------------------------------------------
void vtkIndependentViewerCollection::GetEyeTransform(int i, std::vector<double>& vals)
{
  Viewer& viewer = this->Internals->GetIndependentViewer(i);
  vals.resize(16);

  size_t idx = 0;
  for (int row = 0; row < 4; ++row)
  {
    for (int col = 0; col < 4; ++col)
    {
      vals[idx++] = viewer.EyeTransform->GetElement(row, col);
    }
  }
}

//----------------------------------------------------------------------------
int vtkIndependentViewerCollection::GetNumberOfEyeSeparations()
{
  return static_cast<int>(this->Internals->IndependentViewers.size());
}

//----------------------------------------------------------------------------
void vtkIndependentViewerCollection::SetNumberOfEyeSeparations(int n)
{
  assert(n >= 0);
  this->Internals->IndependentViewers.resize(n);
}

//----------------------------------------------------------------------------
void vtkIndependentViewerCollection::SetEyeSeparation(int i, double separation)
{
  Viewer& viewer = this->Internals->GetIndependentViewer(i);
  viewer.EyeSeparation = separation;
}

//----------------------------------------------------------------------------
double vtkIndependentViewerCollection::GetEyeSeparation(int i)
{
  Viewer& viewer = this->Internals->GetIndependentViewer(i);
  return viewer.EyeSeparation;
}

VTK_ABI_NAMESPACE_END
