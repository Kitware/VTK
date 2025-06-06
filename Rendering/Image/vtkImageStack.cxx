// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkImageStack.h"
#include "vtkAssemblyPath.h"
#include "vtkAssemblyPaths.h"
#include "vtkImageMapper3D.h"
#include "vtkImageProperty.h"
#include "vtkImageSliceCollection.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImageStack);

//------------------------------------------------------------------------------
vtkImageStack::vtkImageStack()
{
  this->ImageMatrices = nullptr;
  this->ActiveLayer = 0;
}

//------------------------------------------------------------------------------
vtkImageStack::~vtkImageStack()
{
  if (this->Images)
  {
    vtkCollectionSimpleIterator pit;
    this->Images->InitTraversal(pit);
    vtkImageSlice* image = nullptr;
    while ((image = this->Images->GetNextImage(pit)) != nullptr)
    {
      image->RemoveConsumer(this);
    }
  }

  if (this->ImageMatrices)
  {
    this->ImageMatrices->Delete();
  }
}

//------------------------------------------------------------------------------
vtkImageSlice* vtkImageStack::GetActiveImage()
{
  vtkImageSlice* activeImage = nullptr;

  vtkCollectionSimpleIterator pit;
  this->Images->InitTraversal(pit);
  vtkImageSlice* image = nullptr;
  while ((image = this->Images->GetNextImage(pit)) != nullptr)
  {
    vtkImageProperty* p = image->GetProperty();
    if (p->GetLayerNumber() == this->ActiveLayer)
    {
      activeImage = image;
    }
  }

  return activeImage;
}

//------------------------------------------------------------------------------
void vtkImageStack::AddImage(vtkImageSlice* prop)
{
  if (this->Images->IndexOfFirstOccurence(prop) < 0 && !vtkImageStack::SafeDownCast(prop))
  {
    this->Images->AddItem(prop);
    prop->AddConsumer(this);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkImageStack::RemoveImage(vtkImageSlice* prop)
{
  if (this->Images->IndexOfFirstOccurence(prop) >= 0)
  {
    prop->RemoveConsumer(this);
    this->Images->RemoveItem(prop);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
vtkTypeBool vtkImageStack::HasImage(vtkImageSlice* prop)
{
  int index = this->Images->IndexOfFirstOccurence(prop);

  // VTK_DEPRECATED_IN_9_5_0()
  // Remove "#if" block and keep "#else" when removing 9.5.0 deprecations
#if defined(VTK_LEGACY_REMOVE)
  return (index >= 0);
#else
  // VTK_DEPRECATED_IN_9_5_0()
  // Keep "#if" block and remove this "#else" when removing 9.5.0 deprecations

  // The implementation used to call IsItemPresent(), which, despite its name,
  // returned an index, not a boolean.  Preserve the old behaviour.  0 means
  // the item is not found, otherwise return the index + 1.
  return index + 1;
#endif
}

//------------------------------------------------------------------------------
void vtkImageStack::GetImages(vtkPropCollection* vc)
{
  vtkCollectionSimpleIterator pit;
  this->Images->InitTraversal(pit);
  vtkImageSlice* image = nullptr;
  while ((image = this->Images->GetNextImage(pit)) != nullptr)
  {
    image->GetImages(vc);
  }
}

//------------------------------------------------------------------------------
void vtkImageStack::ShallowCopy(vtkProp* prop)
{
  vtkImageStack* v = vtkImageStack::SafeDownCast(prop);

  if (v != nullptr)
  {
    this->Images->RemoveAllItems();
    vtkCollectionSimpleIterator pit;
    v->Images->InitTraversal(pit);
    vtkImageSlice* image = nullptr;
    while ((image = v->Images->GetNextImage(pit)) != nullptr)
    {
      this->Images->AddItem(image);
    }
    this->SetActiveLayer(v->GetActiveLayer());
  }

  // Now do prop superclass (NOT vtkImageSlice)
  // NOLINTNEXTLINE(bugprone-parent-virtual-call)
  this->vtkProp3D::ShallowCopy(prop);
}

//------------------------------------------------------------------------------
void vtkImageStack::SetProperty(vtkImageProperty*)
{
  // do nothing
}

//------------------------------------------------------------------------------
vtkImageProperty* vtkImageStack::GetProperty()
{
  // Get the property with the active layer number
  vtkImageSlice* image = this->GetActiveImage();
  if (image)
  {
    return image->GetProperty();
  }

  // Return a dummy property, can't return nullptr.
  if (this->Property == nullptr)
  {
    this->Property = vtkImageProperty::New();
    this->Property->Register(this);
    this->Property->Delete();
  }
  return this->Property;
}

//------------------------------------------------------------------------------
void vtkImageStack::SetMapper(vtkImageMapper3D*)
{
  // do nothing
}

//------------------------------------------------------------------------------
vtkImageMapper3D* vtkImageStack::GetMapper()
{
  // Get the mapper with the active layer number
  vtkImageSlice* image = this->GetActiveImage();
  if (image)
  {
    return image->GetMapper();
  }

  return nullptr;
}

//------------------------------------------------------------------------------
double* vtkImageStack::GetBounds()
{
  this->UpdatePaths();

  double bounds[6];
  bool nobounds = true;

  bounds[0] = VTK_DOUBLE_MAX;
  bounds[1] = VTK_DOUBLE_MIN;
  bounds[2] = VTK_DOUBLE_MAX;
  bounds[3] = VTK_DOUBLE_MIN;
  bounds[4] = VTK_DOUBLE_MAX;
  bounds[5] = VTK_DOUBLE_MIN;

  if (!this->IsIdentity)
  {
    this->PokeMatrices(this->GetMatrix());
  }

  vtkCollectionSimpleIterator pit;
  this->Images->InitTraversal(pit);
  vtkImageSlice* image = nullptr;
  while ((image = this->Images->GetNextImage(pit)) != nullptr)
  {
    const double* b = image->GetBounds();
    if (b)
    {
      nobounds = false;
      bounds[0] = (bounds[0] < b[0] ? bounds[0] : b[0]);
      bounds[1] = (bounds[1] > b[1] ? bounds[1] : b[1]);
      bounds[2] = (bounds[2] < b[2] ? bounds[2] : b[2]);
      bounds[3] = (bounds[3] > b[3] ? bounds[3] : b[3]);
      bounds[4] = (bounds[4] < b[4] ? bounds[4] : b[4]);
      bounds[5] = (bounds[5] > b[5] ? bounds[5] : b[5]);
    }
  }

  if (!this->IsIdentity)
  {
    this->PokeMatrices(nullptr);
  }

  if (nobounds)
  {
    return nullptr;
  }

  this->Bounds[0] = bounds[0];
  this->Bounds[1] = bounds[1];
  this->Bounds[2] = bounds[2];
  this->Bounds[3] = bounds[3];
  this->Bounds[4] = bounds[4];
  this->Bounds[5] = bounds[5];

  return this->Bounds;
}

//------------------------------------------------------------------------------
// Does this prop have some translucent polygonal geometry?
vtkTypeBool vtkImageStack::HasTranslucentPolygonalGeometry()
{
  vtkCollectionSimpleIterator pit;
  this->Images->InitTraversal(pit);
  vtkImageSlice* image = nullptr;
  while ((image = this->Images->GetNextImage(pit)) != nullptr)
  {
    if (image->HasTranslucentPolygonalGeometry())
    {
      return 1;
    }
  }

  return 0;
}

//------------------------------------------------------------------------------
// Assembly-like behavior
void vtkImageStack::PokeMatrices(vtkMatrix4x4* matrix)
{
  if (this->ImageMatrices == nullptr)
  {
    this->ImageMatrices = vtkCollection::New();
  }

  if (matrix)
  {
    vtkCollectionSimpleIterator pit;
    this->Images->InitTraversal(pit);
    vtkImageSlice* image = nullptr;
    while ((image = this->Images->GetNextImage(pit)) != nullptr)
    {
      vtkMatrix4x4* propMatrix = vtkMatrix4x4::New();
      propMatrix->Multiply4x4(image->GetMatrix(), matrix, propMatrix);
      image->PokeMatrix(propMatrix);
      this->ImageMatrices->AddItem(propMatrix);
      propMatrix->Delete();
    }
  }
  else
  {
    vtkCollectionSimpleIterator pit;
    this->Images->InitTraversal(pit);
    vtkImageSlice* image = nullptr;
    while ((image = this->Images->GetNextImage(pit)) != nullptr)
    {
      image->PokeMatrix(nullptr);
    }
    this->ImageMatrices->RemoveAllItems();
  }
}

//------------------------------------------------------------------------------
int vtkImageStack::RenderOpaqueGeometry(vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkImageStack::RenderOpaqueGeometry");

  // Opaque render is always called first, so sort here
  this->Images->Sort();
  this->UpdatePaths();

  if (!this->IsIdentity)
  {
    this->PokeMatrices(this->GetMatrix());
  }

  int rendered = 0;
  vtkImageSlice* image = nullptr;
  vtkCollectionSimpleIterator pit;
  vtkIdType n = 0;
  this->Images->InitTraversal(pit);
  while ((image = this->Images->GetNextImage(pit)) != nullptr)
  {
    n += (image->GetVisibility() != 0);
  }
  double renderTime = this->AllocatedRenderTime / (n + (n == 0));

  if (n == 1)
  {
    // no multi-pass if only one image
    this->Images->InitTraversal(pit);
    while ((image = this->Images->GetNextImage(pit)) != nullptr)
    {
      if (image->GetVisibility())
      {
        image->SetAllocatedRenderTime(renderTime, viewport);
        rendered = image->RenderOpaqueGeometry(viewport);
      }
    }
  }
  else
  {
    for (int pass = 0; pass < 3; pass++)
    {
      this->Images->InitTraversal(pit);
      while ((image = this->Images->GetNextImage(pit)) != nullptr)
      {
        if (image->GetVisibility())
        {
          image->SetAllocatedRenderTime(renderTime, viewport);
          image->SetStackedImagePass(pass);
          rendered |= image->RenderOpaqueGeometry(viewport);
          image->SetStackedImagePass(-1);
        }
      }
    }
  }

  if (!this->IsIdentity)
  {
    this->PokeMatrices(nullptr);
  }

  return rendered;
}

//------------------------------------------------------------------------------
int vtkImageStack::RenderTranslucentPolygonalGeometry(vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkImageStack::RenderTranslucentPolygonalGeometry");

  if (!this->IsIdentity)
  {
    this->PokeMatrices(this->GetMatrix());
  }

  int rendered = 0;
  vtkImageSlice* image = nullptr;
  vtkCollectionSimpleIterator pit;
  vtkIdType n = 0;
  this->Images->InitTraversal(pit);
  while ((image = this->Images->GetNextImage(pit)) != nullptr)
  {
    n += (image->GetVisibility() != 0);
  }
  double renderTime = this->AllocatedRenderTime / (n + (n == 0));

  if (n == 1)
  {
    // no multi-pass if only one image
    this->Images->InitTraversal(pit);
    while ((image = this->Images->GetNextImage(pit)) != nullptr)
    {
      if (image->GetVisibility())
      {
        image->SetAllocatedRenderTime(renderTime, viewport);
        rendered = image->RenderTranslucentPolygonalGeometry(viewport);
      }
    }
  }
  else
  {
    for (int pass = 1; pass < 3; pass++)
    {
      this->Images->InitTraversal(pit);
      while ((image = this->Images->GetNextImage(pit)) != nullptr)
      {
        if (image->GetVisibility())
        {
          image->SetAllocatedRenderTime(renderTime, viewport);
          image->SetStackedImagePass(pass);
          rendered |= image->RenderTranslucentPolygonalGeometry(viewport);
          image->SetStackedImagePass(-1);
        }
      }
    }
  }

  if (!this->IsIdentity)
  {
    this->PokeMatrices(nullptr);
  }

  return rendered;
}

//------------------------------------------------------------------------------
int vtkImageStack::RenderOverlay(vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkImageStack::RenderOverlay");

  if (!this->IsIdentity)
  {
    this->PokeMatrices(this->GetMatrix());
  }

  int rendered = 0;
  vtkImageSlice* image = nullptr;
  vtkCollectionSimpleIterator pit;
  vtkIdType n = 0;
  this->Images->InitTraversal(pit);
  while ((image = this->Images->GetNextImage(pit)) != nullptr)
  {
    n += (image->GetVisibility() != 0);
  }
  double renderTime = this->AllocatedRenderTime / (n + (n == 0));

  if (n == 1)
  {
    // no multi-pass if only one image
    this->Images->InitTraversal(pit);
    while ((image = this->Images->GetNextImage(pit)) != nullptr)
    {
      if (image->GetVisibility())
      {
        image->SetAllocatedRenderTime(renderTime, viewport);
        rendered = image->RenderOverlay(viewport);
      }
    }
  }
  else
  {
    for (int pass = 1; pass < 3; pass++)
    {
      this->Images->InitTraversal(pit);
      while ((image = this->Images->GetNextImage(pit)) != nullptr)
      {
        if (image->GetVisibility())
        {
          image->SetAllocatedRenderTime(renderTime, viewport);
          image->SetStackedImagePass(pass);
          rendered |= image->RenderOverlay(viewport);
          image->SetStackedImagePass(-1);
        }
      }
    }
  }

  if (!this->IsIdentity)
  {
    this->PokeMatrices(nullptr);
  }

  return rendered;
}

//------------------------------------------------------------------------------
void vtkImageStack::ReleaseGraphicsResources(vtkWindow* win)
{
  vtkCollectionSimpleIterator pit;
  this->Images->InitTraversal(pit);
  vtkImageSlice* image = nullptr;
  while ((image = this->Images->GetNextImage(pit)) != nullptr)
  {
    image->ReleaseGraphicsResources(win);
  }
}

//------------------------------------------------------------------------------
vtkMTimeType vtkImageStack::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType t;

  // Get the max mtime of all the images
  vtkCollectionSimpleIterator pit;
  this->Images->InitTraversal(pit);
  vtkImageSlice* image = nullptr;
  while ((image = this->Images->GetNextImage(pit)) != nullptr)
  {
    t = image->GetMTime();
    mTime = (t < mTime ? mTime : t);
  }

  return mTime;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkImageStack::GetRedrawMTime()
{
  // Just call GetMTime on ourselves, not GetRedrawMTime
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType t;

  // Get the max mtime of all the images
  vtkCollectionSimpleIterator pit;
  this->Images->InitTraversal(pit);
  vtkImageSlice* image = nullptr;
  while ((image = this->Images->GetNextImage(pit)) != nullptr)
  {
    t = image->GetRedrawMTime();
    mTime = (t < mTime ? mTime : t);
  }

  return mTime;
}

//------------------------------------------------------------------------------
void vtkImageStack::InitPathTraversal()
{
  this->UpdatePaths();
  this->Paths->InitTraversal();
}

//------------------------------------------------------------------------------
vtkAssemblyPath* vtkImageStack::GetNextPath()
{
  if (this->Paths)
  {
    return this->Paths->GetNextItem();
  }
  return nullptr;
}

//------------------------------------------------------------------------------
int vtkImageStack::GetNumberOfPaths()
{
  this->UpdatePaths();
  return this->Paths->GetNumberOfItems();
}

//------------------------------------------------------------------------------
void vtkImageStack::UpdatePaths()
{
  if (this->GetMTime() > this->PathTime ||
    (this->Paths && this->Paths->GetMTime() > this->PathTime))
  {
    if (this->Paths)
    {
      this->Paths->Delete();
    }

    // Create the list to hold all the paths
    this->Paths = vtkAssemblyPaths::New();
    vtkAssemblyPath* path = vtkAssemblyPath::New();

    // Add ourselves to the path to start things off
    path->AddNode(this, this->GetMatrix());

    // Add the active image
    vtkImageSlice* image = this->GetActiveImage();

    if (image)
    {
      path->AddNode(image, image->GetMatrix());
      image->BuildPaths(this->Paths, path);
      path->DeleteLastNode();
    }

    path->Delete();
    this->PathTime.Modified();
  }
}

//------------------------------------------------------------------------------
void vtkImageStack::BuildPaths(vtkAssemblyPaths* paths, vtkAssemblyPath* path)
{
  // the path consists only of the active image
  vtkImageSlice* image = this->GetActiveImage();

  if (image)
  {
    path->AddNode(image, image->GetMatrix());
    image->BuildPaths(paths, path);
    path->DeleteLastNode();
  }
}

//------------------------------------------------------------------------------
void vtkImageStack::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Images: " << this->Images << "\n";
  os << indent << "ActiveLayer: " << this->ActiveLayer << "\n";
  os << indent << "ActiveImage: " << this->GetActiveImage() << "\n";
}
VTK_ABI_NAMESPACE_END
