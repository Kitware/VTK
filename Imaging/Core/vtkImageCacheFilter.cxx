/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCacheFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageCacheFilter.h"

#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkCachedStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageCacheFilter);

//----------------------------------------------------------------------------
vtkImageCacheFilter::vtkImageCacheFilter()
{
  vtkExecutive *exec = this->CreateDefaultExecutive();
  this->SetExecutive(exec);
  exec->Delete();

  this->SetCacheSize(10);
}

//----------------------------------------------------------------------------
vtkImageCacheFilter::~vtkImageCacheFilter()
{
}

//----------------------------------------------------------------------------
vtkExecutive* vtkImageCacheFilter::CreateDefaultExecutive()
{
  return vtkCachedStreamingDemandDrivenPipeline::New();
}

//----------------------------------------------------------------------------
void vtkImageCacheFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CacheSize: " << this->GetCacheSize() << endl;
}

//----------------------------------------------------------------------------
void vtkImageCacheFilter::SetCacheSize(int size)
{
  vtkCachedStreamingDemandDrivenPipeline *csddp =
    vtkCachedStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (csddp)
    {
    csddp->SetCacheSize(size);
    }
}

//----------------------------------------------------------------------------
int vtkImageCacheFilter::GetCacheSize()
{
  vtkCachedStreamingDemandDrivenPipeline *csddp =
    vtkCachedStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (csddp)
    {
    return csddp->GetCacheSize();
    }
  return 0;
}

//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
void vtkImageCacheFilter::ExecuteData(vtkDataObject *)
{
  // do nothing just override superclass to prevent warning
}
