// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImageCacheFilter.h"

#include "vtkCachedStreamingDemandDrivenPipeline.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImageCacheFilter);

//------------------------------------------------------------------------------
vtkImageCacheFilter::vtkImageCacheFilter()
{
  vtkExecutive* exec = this->CreateDefaultExecutive();
  this->SetExecutive(exec);
  exec->Delete();

  this->SetCacheSize(10);
}

//------------------------------------------------------------------------------
vtkImageCacheFilter::~vtkImageCacheFilter() = default;

//------------------------------------------------------------------------------
vtkExecutive* vtkImageCacheFilter::CreateDefaultExecutive()
{
  return vtkCachedStreamingDemandDrivenPipeline::New();
}

//------------------------------------------------------------------------------
void vtkImageCacheFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CacheSize: " << this->GetCacheSize() << endl;
}

//------------------------------------------------------------------------------
void vtkImageCacheFilter::SetCacheSize(int size)
{
  vtkCachedStreamingDemandDrivenPipeline* csddp =
    vtkCachedStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (csddp)
  {
    csddp->SetCacheSize(size);
  }
}

//------------------------------------------------------------------------------
int vtkImageCacheFilter::GetCacheSize()
{
  vtkCachedStreamingDemandDrivenPipeline* csddp =
    vtkCachedStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (csddp)
  {
    return csddp->GetCacheSize();
  }
  return 0;
}

//------------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
void vtkImageCacheFilter::ExecuteData(vtkDataObject*)
{
  // do nothing just override superclass to prevent warning
}
VTK_ABI_NAMESPACE_END
