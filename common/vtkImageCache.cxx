 /*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCache.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkImageToStructuredPoints.h"
#include "vtkImageRegion.h"
#include "vtkImageSource.h"
#include "vtkImageCache.h"

//----------------------------------------------------------------------------
// Description:
// Constructor:  By default caches ReleaseDataFlags are turned off. However,
// the vtkImageSource method CheckCache, which create a default cache, 
// turns this flag on.  If a cache is created and set explicitely, by 
// default it saves its data between generates.  But if the cache is created
// automatically by the vtkImageSource, it does not.
vtkImageCache::vtkImageCache()
{
  int idx;
  
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->UpdateExtent[idx*2] = -VTK_LARGE_INTEGER;
    this->UpdateExtent[idx*2+1] = VTK_LARGE_INTEGER;
    this->Spacing[idx] = 1.0;
    this->Origin[idx] = 0.0;
    this->ExecuteExtent[idx*2] = this->ExecuteExtent[idx*2+1] = 0;
    this->WholeExtent[idx*2] = this->WholeExtent[idx*2+1] = 0;
    this->Dimensions[idx] = 1;
    this->Center[idx] = 0.0;
    this->Bounds[idx*2] = this->Bounds[idx*2+1] = 0.0;
    }

  this->NumberOfScalarComponents = 1;
  this->NumberOfVectorComponents = 1;
  
  this->Source = NULL;
  
  // for automatic conversion
  this->ImageToStructuredPoints = NULL;

  // default is to save data,
  // (But caches automatically created by sources set ReleaseDataFlag to 1)
  this->ReleaseDataFlag = 0;
  this->DataReleased = 1;
  
  // Invalid data type
  // This will be changed when the filter gets an input or
  // the ScalarType is set explicitly
  this->ScalarType = VTK_VOID;

  this->ScalarData = NULL;
  this->VectorData = NULL;
}


//----------------------------------------------------------------------------
vtkImageCache::~vtkImageCache()
{

  if (this->ImageToStructuredPoints)
    {
    this->ImageToStructuredPoints->Delete();
    }
  this->ReleaseData();
}

//----------------------------------------------------------------------------
void vtkImageCache::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  vtkObject::PrintSelf(os,indent);

  os << indent << "Source: (" << this->Source << ").\n";
  os << indent << "ReleaseDataFlag: " << this->ReleaseDataFlag << "\n";
  os << indent << "ScalarType: "<<vtkImageScalarTypeNameMacro(this->ScalarType)
     << "\n";
  os << indent << "ImageToStructuredPoints: (" 
     << this->ImageToStructuredPoints << ")\n";
  
  os << indent << "Spacing: (" << this->Spacing[0];
  for (idx = 1; idx < 4; ++idx)
    {
    os << ", " << this->Spacing[idx];
    }
  os << ")\n";
  
  os << indent << "Origin: (" << this->Origin[0];
  for (idx = 1; idx < 4; ++idx)
    {
    os << ", " << this->Origin[idx];
    }
  os << ")\n";
  
  os << indent << "Center: (" << this->Center[0];
  for (idx = 1; idx < 4; ++idx)
    {
    os << ", " << this->Center[idx];
    }
  os << ")\n";
  
  os << indent << "WholeExtent: (" << this->WholeExtent[0];
  for (idx = 1; idx < 8; ++idx)
    {
    os << ", " << this->WholeExtent[idx];
    }
  os << ")\n";

  os << indent << "UpdateExtent: (" << this->UpdateExtent[0];
  for (idx = 1; idx < 8; ++idx)
    {
    os << ", " << this->UpdateExtent[idx];
    }
  os << ")\n";

  os << indent << "ExecuteTime: " << this->ExecuteTime.GetMTime() << "\n";
  
  os << indent << "Bounds: (" << this->Bounds[0];
  for (idx = 1; idx < 8; ++idx)
    {
    os << ", " << this->Bounds[idx];
    }
  os << ")\n";

  os << indent << "ScalarData: (" << this->ScalarData << ")\n";
  os << indent << "VectorData: (" << this->VectorData << ")\n";
  
}
  
    

//----------------------------------------------------------------------------
void vtkImageCache::SetUpdateExtent(int extent[8])
{
  int idx, modified = 0;
  
  for (idx = 0; idx < 8; ++idx)
    {
    if (this->UpdateExtent[idx] != extent[idx])
      {
      modified = 1;
      this->UpdateExtent[idx] = extent[idx];
      }
    }
  if (modified)
    {
    this->Modified();
    }
}
//----------------------------------------------------------------------------
void vtkImageCache::SetUpdateExtent(int xMin, int xMax, int yMin, int yMax,
				    int zMin, int zMax, int tMin, int tMax)
{
  int extent[8];

  extent[0] = xMin; extent[1] = xMax;
  extent[2] = yMin; extent[3] = yMax;
  extent[4] = zMin; extent[5] = zMax;
  extent[6] = tMin; extent[7] = tMax;
}
//----------------------------------------------------------------------------
void vtkImageCache::SetAxesUpdateExtent(int num, int *axes, int *extent)
{
  int idx, axis, modified = 0;
  
  if (num > 4)
    {
    vtkWarningMacro("SetAxesUpdateExtent: " << num 
		    << " is too many axes");
    num = 4;
    }
  for (idx = 0; idx < num; ++idx)
    {
    axis = axes[idx];
    if (this->UpdateExtent[axis*2] != extent[idx*2])
      {
      modified = 1;
      this->UpdateExtent[axis*2] = extent[idx*2];
      }
    if (this->UpdateExtent[axis*2+1] != extent[idx*2+1])
      {
      modified = 1;
      this->UpdateExtent[axis*2+1] = extent[idx*2+1];
      }
    }
  if (modified)
    {
    this->Modified();
    }
}
//----------------------------------------------------------------------------
void vtkImageCache::SetUpdateExtentToWholeExtent()
{
  this->UpdateImageInformation();
  this->SetUpdateExtent(this->WholeExtent);
}


//----------------------------------------------------------------------------
void vtkImageCache::GetUpdateExtent(int extent[8])
{
  int idx;
  
  for (idx = 0; idx < 8; ++idx)
    {
    extent[idx] = this->UpdateExtent[idx];
    }
}
//----------------------------------------------------------------------------
void vtkImageCache::GetUpdateExtent(int &xMin, int &xMax, int &yMin, int &yMax,
				    int &zMin, int &zMax, int &tMin, int &tMax)
{
  xMin = this->UpdateExtent[0]; xMax = this->UpdateExtent[1];
  yMin = this->UpdateExtent[2]; yMax = this->UpdateExtent[2];
  zMin = this->UpdateExtent[4]; zMax = this->UpdateExtent[4];
  tMin = this->UpdateExtent[6]; tMax = this->UpdateExtent[6];
}
//----------------------------------------------------------------------------
void vtkImageCache::GetAxesUpdateExtent(int num, int *axes, int *extent)
{
  int idx, axis;
  
  if (num > 4)
    {
    vtkWarningMacro("GetAxesUpdateExtent: " << num 
		    << " is too many axes");
    num = 4;
    }
  for (idx = 0; idx < num; ++idx)
    {
    axis = axes[idx];
    extent[idx*2] = this->UpdateExtent[axis*2];
    extent[idx*2+1] = this->UpdateExtent[axis*2+1];
    }
}

//----------------------------------------------------------------------------
// Description:
// This method updates the region specified by "UpdateExtent".  
void vtkImageCache::Update()
{
  int updateExtentSave[8];
  unsigned long pipelineMTime = this->GetPipelineMTime();

  // Make sure image information is upto date
  this->UpdateImageInformation(pipelineMTime);
  this->ClipUpdateExtentWithWholeExtent();
  
  // Let the cache modify the update extent, but save the old one.
  this->GetUpdateExtent(updateExtentSave);
  this->Source->InterceptCacheUpdate(this);
  
  // if cache doesn't have the necessary data.
  if (pipelineMTime > this->ExecuteTime || this->DataReleased ||
      (this->ExecuteExtent[0] > this->UpdateExtent[0] ||
       this->ExecuteExtent[1] < this->UpdateExtent[1] ||
       this->ExecuteExtent[2] > this->UpdateExtent[2] ||
       this->ExecuteExtent[3] < this->UpdateExtent[3] ||
       this->ExecuteExtent[4] > this->UpdateExtent[4] ||
       this->ExecuteExtent[5] < this->UpdateExtent[5] ||
       this->ExecuteExtent[6] > this->UpdateExtent[6] ||
       this->ExecuteExtent[7] < this->UpdateExtent[7]))
    {
    if (this->Source)
      {
      vtkDebugMacro("Update: We have to update the source.");
      this->Source->Update();
      // save the time and extent of the update for test "cache has data?"
      this->ExecuteTime.Modified();
      this->GetUpdateExtent(this->ExecuteExtent);
      this->DataReleased = 0;
      }
    }
  else
    {
    vtkDebugMacro("Update: UpdateRegion already in cache.");
    }
  
  // Restore the old update extent
  this->SetUpdateExtent(updateExtentSave);
}


//----------------------------------------------------------------------------
// Description:
// This method updates the instance variables "WholeExtent", "Spacing", 
// "Origin", "Bounds" etc.
// It needs to be separate from "Update" because the image information
// may be needed to compute the required UpdateExtent of the input
// (see "vtkImageFilter").
void vtkImageCache::UpdateImageInformation()
{
  this->UpdateImageInformation(this->GetPipelineMTime());
}

//----------------------------------------------------------------------------
// Description:
// Make this a separate method to avoid another GetPipelineMTime call.
void vtkImageCache::UpdateImageInformation(unsigned long pipelineMTime)
{
  if ((pipelineMTime > this->ExecuteImageInformationTime) && this->Source)
    {
    this->Source->UpdateImageInformation();
    this->ExecuteImageInformationTime.Modified();
    }

}

//----------------------------------------------------------------------------
// Description:
// Clip updateExtent so it will nopt be larger than WHoleExtent
void vtkImageCache::ClipUpdateExtentWithWholeExtent()
{
  int idx;
  
  // Clip the UpdateExtent with the WholeExtent
  for (idx = 0; idx < 4; ++idx)
    {
    // min
    if (this->UpdateExtent[idx*2] < this->WholeExtent[idx*2])
      {
      this->UpdateExtent[idx*2] = this->WholeExtent[idx*2];
      }
    if (this->UpdateExtent[idx*2] > this->WholeExtent[idx*2+1])
      {
      this->UpdateExtent[idx*2] = this->WholeExtent[idx*2+1];
      }
    // max
    if (this->UpdateExtent[idx*2+1] < this->WholeExtent[idx*2])
      {
      this->UpdateExtent[idx*2+1] = this->WholeExtent[idx*2];
      }
    if (this->UpdateExtent[idx*2+1] > this->WholeExtent[idx*2+1])
      {
      this->UpdateExtent[idx*2+1] = this->WholeExtent[idx*2+1];
      }
    }
}

//----------------------------------------------------------------------------
// Description:
// Make this a separate method to avoid another GetPipelineMTime call.
unsigned long vtkImageCache::GetPipelineMTime()
{
  if (this->Source)
    {
    // We do not tak this objects MTime into consideration, 
    // but maybe we should.
    return this->Source->GetPipelineMTime();
    }
  return this->GetMTime();
}

//----------------------------------------------------------------------------
// Description:
// This method returns a "vtkImageRegion" that holds the scalar data
// of the cache.  Source must delete the region when finished with it.
// It assumes that the UpdateExtent has already been cliped.
// (i.e. This method is called after an update)
vtkImageRegion *vtkImageCache::GetScalarRegion()
{
  vtkImageRegion *region;
  
  if ( this->ScalarData)
    {
    int min, max;
    int *dataExtent, *extent;
    // If data extent is large enough, we should reuse the data.
    dataExtent = this->ScalarData->GetExtent();
    this->ScalarData->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, min, max);
    extent = this->UpdateExtent;
    if (extent[0] < dataExtent[0] || extent[1] > dataExtent[1] ||
	extent[2] < dataExtent[2] || extent[3] > dataExtent[3] ||
	extent[4] < dataExtent[4] || extent[5] > dataExtent[5] ||
	extent[6] < dataExtent[6] || extent[7] > dataExtent[7] ||
	this->NumberOfScalarComponents > (max-min+1))
      {
      // Data is not valid. Get rid of it.
      this->ScalarData->Delete();
      this->ScalarData = NULL;
      }
    }
  
  if ( ! this->ScalarData)
    {
    this->ScalarData = vtkImageData::New();
    this->ScalarData->SetExtent(4, this->UpdateExtent);
    this->ScalarData->SetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, 
				    0, this->NumberOfScalarComponents-1);
    this->ScalarData->SetScalarType(this->ScalarType);
    }
  
  region = vtkImageRegion::New();
  region->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS, VTK_IMAGE_Z_AXIS,
		  VTK_IMAGE_TIME_AXIS);
  region->SetExtent(4, this->UpdateExtent);
  region->SetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, 
			0, this->NumberOfScalarComponents - 1);
  region->SetWholeExtent(4, this->WholeExtent);
  region->SetAxisWholeExtent(VTK_IMAGE_COMPONENT_AXIS, 
			     0, this->NumberOfScalarComponents - 1);
  region->SetOrigin(4, this->Origin);
  region->SetSpacing(4, this->Spacing);  
  region->SetData(this->ScalarData);
  
  return region;
}



//----------------------------------------------------------------------------
// Description:
// This method is here for vtkImageToStructuredPoints.
// It allows the converter to stream.  It fixes the update extent,
// and then updates with tiled smaller extents. The cache allocates
// one big extent, and sub extents update into the larger piece of memory.
void vtkImageCache::SetWholeUpdateExtent(int *extent)
{
  this->SetUpdateExtent(extent);
  this->UpdateImageInformation();
  this->ClipUpdateExtentWithWholeExtent();
  extent = this->UpdateExtent;
  
  // similar logic to GetScalarData.
  // Because it creates a scalar data.
  if ( this->ScalarData)
    {
    int min, max;
    int *dataExtent;
    // If data extent is large enough, we should reuse the data.
    dataExtent = this->ScalarData->GetExtent();
    this->ScalarData->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, min, max);
    if (extent[0] < dataExtent[0] || extent[1] > dataExtent[1] ||
	extent[2] < dataExtent[2] || extent[3] > dataExtent[3] ||
	extent[4] < dataExtent[4] || extent[5] > dataExtent[5] ||
	extent[6] < dataExtent[6] || extent[7] > dataExtent[7] ||
	this->NumberOfScalarComponents > (max-min+1))
      {
      // Data is not valid. Get rid of it.
      this->ScalarData->Delete();
      this->ScalarData = NULL;
      }
    }
  
  if ( ! this->ScalarData)
    {
    this->ScalarData = vtkImageData::New();
    this->ScalarData->SetExtent(4, this->UpdateExtent);
    this->ScalarData->SetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, 
				    0, this->NumberOfScalarComponents-1);
    this->ScalarData->SetScalarType(this->ScalarType);
    }
}



//----------------------------------------------------------------------------
// Description:
// This method returns a "vtkImageRegion" that holds the vector data
// of the cache.  Source must delete the region when finished with it.
vtkImageRegion *vtkImageCache::GetVectorRegion()
{
  vtkImageRegion *region;
  
  if ( this->VectorData)
    {
    int min, max;
    int *dataExtent, *extent;
    // If data extent is large enough, we should reuse the data.
    dataExtent = this->VectorData->GetExtent();
    this->VectorData->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, min, max);
    extent = this->UpdateExtent;
    if (extent[0] < dataExtent[0] || extent[1] > dataExtent[1] ||
	extent[2] < dataExtent[2] || extent[3] > dataExtent[3] ||
	extent[4] < dataExtent[4] || extent[5] > dataExtent[5] ||
	extent[6] < dataExtent[6] || extent[7] > dataExtent[7] ||
	this->NumberOfVectorComponents > (max-min+1))
      {
      // Data is not valid. Get rid of it.
      this->VectorData->Delete();
      this->VectorData = NULL;
      }
    }
  
  if ( ! this->VectorData)
    {
    this->VectorData = vtkImageData::New();
    this->VectorData->SetExtent(4, this->UpdateExtent);
    this->VectorData->SetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, 
				    0, this->NumberOfVectorComponents-1);
    this->VectorData->SetScalarType(this->ScalarType);
    }
  
  region = vtkImageRegion::New();
  region->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS, VTK_IMAGE_Z_AXIS,
		  VTK_IMAGE_TIME_AXIS);
  region->SetExtent(4, this->UpdateExtent);
  region->SetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, 
			0, this->NumberOfVectorComponents - 1);
  region->SetWholeExtent(4, this->WholeExtent);
  region->SetAxisWholeExtent(VTK_IMAGE_COMPONENT_AXIS, 
			     0, this->NumberOfVectorComponents - 1);
  region->SetOrigin(4, this->Origin);
  region->SetSpacing(4, this->Spacing);  
  region->SetData(this->ScalarData);
  
  return region;
}




//----------------------------------------------------------------------------
void vtkImageCache::SetSpacing(float spacing[4])
{
  int idx, modified = 0;
  
  for (idx = 0; idx < 4; ++idx)
    {
    if (this->Spacing[idx] != spacing[idx])
      {
      modified = 1;
      this->Spacing[idx] = spacing[idx];
      }
    }
  if (modified)
    {
    this->Modified();
    }
}
//----------------------------------------------------------------------------
void vtkImageCache::SetSpacing(float x, float y, float z, float t)
{
  float spacing[4];

  spacing[0] = x;
  spacing[1] = y;
  spacing[2] = z;
  spacing[3] = t;
  this->SetSpacing(spacing);
}
//----------------------------------------------------------------------------
void vtkImageCache::SetAxesSpacing(int num, int *axes, float *spacing)
{
  int idx, axis, modified = 0;
  
  if (num > 4)
    {
    vtkWarningMacro("SetAxesSpacing: " << num 
		    << " is too many axes");
    num = 4;
    }
  for (idx = 0; idx < num; ++idx)
    {
    axis = axes[idx];
    if (this->Spacing[axis] != spacing[idx])
      {
      modified = 1;
      this->Spacing[axis] = spacing[idx];
      }
    }
  if (modified)
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkImageCache::GetSpacing(float spacing[4])
{
  int idx;
  
  for (idx = 0; idx < 4; ++idx)
    {
    spacing[idx] = this->Spacing[idx];
    }
}
//----------------------------------------------------------------------------
void vtkImageCache::GetSpacing(float &x, float &y, float &z, float &t)
{
  x = this->Spacing[0];
  y = this->Spacing[1];
  z = this->Spacing[2];
  t = this->Spacing[3];
}
//----------------------------------------------------------------------------
void vtkImageCache::GetAxesSpacing(int num, int *axes, float *spacing)
{
  int idx, axis;
  
  if (num > 4)
    {
    vtkWarningMacro("GetAxesSpacing: " << num 
		    << " is too many axes");
    num = 4;
    }
  for (idx = 0; idx < num; ++idx)
    {
    axis = axes[idx];
    if (axis < 0 || axis > 3)
      {
      vtkErrorMacro("GetAxesSpacing: Axis " << axis << " is invalid");
      return;
      }
    spacing[idx] = this->Spacing[axis];
    }
}


//----------------------------------------------------------------------------
void vtkImageCache::SetOrigin(float origin[4])
{
  int idx, modified = 0;
  
  for (idx = 0; idx < 4; ++idx)
    {
    if (this->Origin[idx] != origin[idx])
      {
      modified = 1;
      this->Origin[idx] = origin[idx];
      }
    }
  if (modified)
    {
    this->Modified();
    }
}
//----------------------------------------------------------------------------
void vtkImageCache::SetOrigin(float x, float y, float z, float t)
{
  float origin[4];

  origin[0] = x;
  origin[1] = y;
  origin[2] = z;
  origin[3] = t;
  this->SetOrigin(origin);
}
//----------------------------------------------------------------------------
void vtkImageCache::SetAxesOrigin(int num, int *axes, float *origin)
{
  int idx, axis, modified = 0;
  
  if (num > 4)
    {
    vtkWarningMacro("SetAxesOrigin: " << num 
		    << " is too many axes");
    num = 4;
    }
  for (idx = 0; idx < num; ++idx)
    {
    axis = axes[idx];
    if (this->Origin[axis] != origin[idx])
      {
      modified = 1;
      this->Origin[axis] = origin[idx];
      }
    }
  if (modified)
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkImageCache::GetOrigin(float origin[4])
{
  int idx;
  
  for (idx = 0; idx < 4; ++idx)
    {
    origin[idx] = this->Origin[idx];
    }
}
//----------------------------------------------------------------------------
void vtkImageCache::GetOrigin(float &x, float &y, float &z, float &t)
{
  x = this->Origin[0];
  y = this->Origin[1];
  z = this->Origin[2];
  t = this->Origin[3];
}
//----------------------------------------------------------------------------
void vtkImageCache::GetAxesOrigin(int num, int *axes, float *origin)
{
  int idx, axis;
  
  if (num > 4)
    {
    vtkWarningMacro("GetAxesOrigin: " << num 
		    << " is too many axes");
    num = 4;
    }
  for (idx = 0; idx < num; ++idx)
    {
    axis = axes[idx];
    origin[idx] = this->Origin[axis];
    }
}


//----------------------------------------------------------------------------
void vtkImageCache::SetWholeExtent(int extent[8])
{
  int idx, modified = 0;
  
  for (idx = 0; idx < 8; ++idx)
    {
    if (this->WholeExtent[idx] != extent[idx])
      {
      modified = 1;
      this->WholeExtent[idx] = extent[idx];
      }
    }
  if (modified)
    {
    this->Modified();
    }
}
//----------------------------------------------------------------------------
void vtkImageCache::SetWholeExtent(int xMin, int xMax, int yMin, int yMax,
				   int zMin, int zMax, int tMin, int tMax)
{
  int extent[8];

  extent[0] = xMin; extent[1] = xMax;
  extent[2] = yMin; extent[3] = yMax;
  extent[4] = zMin; extent[5] = zMax;
  extent[6] = tMin; extent[7] = tMax;
  this->SetWholeExtent(extent);
}
//----------------------------------------------------------------------------
void vtkImageCache::SetAxesWholeExtent(int num, int *axes, int *extent)
{
  int idx, axis, modified = 0;
  
  if (num > 4)
    {
    vtkWarningMacro("SetAxesWholeExtent: " << num 
		    << " is too many axes");
    num = 4;
    }
  for (idx = 0; idx < num; ++idx)
    {
    axis = axes[idx];
    if (this->WholeExtent[axis*2] != extent[idx*2])
      {
      modified = 1;
      this->WholeExtent[axis*2] = extent[idx*2];
      }
    if (this->WholeExtent[axis*2+1] != extent[idx*2+1])
      {
      modified = 1;
      this->WholeExtent[axis*2+1] = extent[idx*2+1];
      }
    }
  if (modified)
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkImageCache::GetWholeExtent(int extent[8])
{
  int idx;
  
  for (idx = 0; idx < 8; ++idx)
    {
    extent[idx] = this->WholeExtent[idx];
    }
}
//----------------------------------------------------------------------------
void vtkImageCache::GetWholeExtent(int &xMin, int &xMax, int &yMin, int &yMax,
				   int &zMin, int &zMax, int &tMin, int &tMax)
{
  xMin = this->WholeExtent[0]; xMax = this->WholeExtent[1];
  yMin = this->WholeExtent[2]; yMax = this->WholeExtent[2];
  zMin = this->WholeExtent[4]; zMax = this->WholeExtent[4];
  tMin = this->WholeExtent[6]; tMax = this->WholeExtent[6];
}
//----------------------------------------------------------------------------
void vtkImageCache::GetAxesWholeExtent(int num, int *axes, int *extent)
{
  int idx, axis;
  
  if (num > 4)
    {
    vtkWarningMacro("GetAxesWholeExtent: " << num 
		    << " is too many axes");
    num = 4;
    }
  for (idx = 0; idx < num; ++idx)
    {
    axis = axes[idx];
    extent[idx*2] = this->WholeExtent[axis*2];
    extent[idx*2+1] = this->WholeExtent[axis*2+1];
    }
}



//----------------------------------------------------------------------------
void vtkImageCache::GetDimensions(int dimensions[4])
{
  int idx;
  
  for (idx = 0; idx < 4; ++idx)
    {
    dimensions[idx] = this->Dimensions[idx];
    }
}
//----------------------------------------------------------------------------
void vtkImageCache::GetDimensions(int &x, int &y, int &z, int &t)
{
  this->ComputeBounds();
  x = this->Dimensions[0];
  y = this->Dimensions[1];
  z = this->Dimensions[2];
  t = this->Dimensions[3];
}
//----------------------------------------------------------------------------
void vtkImageCache::GetAxesDimensions(int num, int *axes, int *dimensions)
{
  int idx, axis;
  
  this->ComputeBounds();
  if (num > 4)
    {
    vtkWarningMacro("GetAxesDimensions: " << num 
		    << " is too many axes");
    num = 4;
    }
  for (idx = 0; idx < num; ++idx)
    {
    axis = axes[idx];
    dimensions[idx] = this->Dimensions[axis];
    }
}


//----------------------------------------------------------------------------
void vtkImageCache::GetCenter(float center[4])
{
  int idx;
  
  for (idx = 0; idx < 4; ++idx)
    {
    center[idx] = this->Center[idx];
    }
}
//----------------------------------------------------------------------------
void vtkImageCache::GetCenter(float &x, float &y, float &z, float &t)
{
  this->ComputeBounds();
  x = this->Center[0];
  y = this->Center[1];
  z = this->Center[2];
  t = this->Center[3];
}
//----------------------------------------------------------------------------
void vtkImageCache::GetAxesCenter(int num, int *axes, float *center)
{
  int idx, axis;
  
  this->ComputeBounds();
  if (num > 4)
    {
    vtkWarningMacro("GetAxesCenter: " << num 
		    << " is too many axes");
    num = 4;
    }
  for (idx = 0; idx < num; ++idx)
    {
    axis = axes[idx];
    center[idx] = this->Center[axis];
    }
}


//----------------------------------------------------------------------------
void vtkImageCache::GetBounds(float bounds[8])
{
  int idx;
  
  for (idx = 0; idx < 8; ++idx)
    {
    bounds[idx] = this->Bounds[idx];
    }
}
//----------------------------------------------------------------------------
void 
vtkImageCache::GetBounds(float &xMin, float &xMax, float &yMin, float &yMax,
			 float &zMin, float &zMax, float &tMin, float &tMax)
{
  xMin = this->Bounds[0]; xMax = this->Bounds[1];
  yMin = this->Bounds[2]; yMax = this->Bounds[2];
  zMin = this->Bounds[4]; zMax = this->Bounds[4];
  tMin = this->Bounds[6]; tMax = this->Bounds[6];
}
//----------------------------------------------------------------------------
void vtkImageCache::GetAxesBounds(int num, int *axes, float *bounds)
{
  int idx, axis;
  
  if (num > 4)
    {
    vtkWarningMacro("GetAxesBounds: " << num 
		    << " is too many axes");
    num = 4;
    }
  for (idx = 0; idx < num; ++idx)
    {
    axis = axes[idx];
    bounds[idx*2] = this->Bounds[axis*2];
    bounds[idx*2+1] = this->Bounds[axis*2+1];
    }
}



//----------------------------------------------------------------------------
// Description:
// This Method sets the value of "ReleaseDataFlag" which turns cache on or off.
// When cache is off, memory will be freed by the consumer.
void vtkImageCache::SetReleaseDataFlag(int value)
{
  if (value == this->ReleaseDataFlag)
    {
    return;
    }
  
  this->Modified();
  this->ReleaseDataFlag = value;

  if ( value == 1)
    {
    // Tell the subclass to delete data it has cached.
    this->ReleaseData();
    }
}


//----------------------------------------------------------------------------
void vtkImageCache::SetGlobalReleaseDataFlag(int val)
{
  // not implemented yet ...
}

//----------------------------------------------------------------------------
int  vtkImageCache::GetGlobalReleaseDataFlag()
{
  // not implemented yet ...
  return 0;
}



//----------------------------------------------------------------------------
// Description:  
// This method causes the cache to release its data, however it 
// does not affect image infomation.
void vtkImageCache::ReleaseData()
{
  if (this->ScalarData)
    {
    this->ScalarData->Delete();
    this->ScalarData = NULL;
    }
  if (this->VectorData)
    {
    this->VectorData->Delete();
    this->VectorData = NULL;
    }
  this->DataReleased = 1;
}


//----------------------------------------------------------------------------
// Description:
// Return flag indicating whether data should be released after use  
// by a filter.  For now, it does not look at the global release data flag.
int vtkImageCache::ShouldIReleaseData()
{
  if (this->ReleaseDataFlag ) 
    {
    return 1;
    }
  else 
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
// Description:
// This method returns the memory that would be required for scalars on update.
// The returned value is in units KBytes.
// This method is used for determining when to stream.
long vtkImageCache::GetUpdateExtentMemorySize()
{
  long size = 1;
  int idx;
  
  // Compute the number of scalars.
  for (idx = 0; idx < 4; ++idx)
    {
    size *= (this->UpdateExtent[idx*2+1] - this->UpdateExtent[idx*2] + 1);
    }
  // Consider components
  size *= this->NumberOfScalarComponents;
  
  // Consider the size of each scalar.
  switch (this->ScalarType)
    {
    case VTK_FLOAT:
      size *= sizeof(float);
      break;
    case VTK_INT:
      size *= sizeof(int);
      break;
    case VTK_SHORT:
      size *= sizeof(short);
      break;
    case VTK_UNSIGNED_SHORT:
      size *= sizeof(unsigned short);
      break;
    case VTK_UNSIGNED_CHAR:
      size *= sizeof(unsigned char);
      break;
    default:
      vtkWarningMacro(<< "GetExtentMemorySize: "
        << "Cannot determine input scalar type");
    }  

  // In case the extent is set improperly
  if (size < 0)
    {
    vtkErrorMacro("GetExtentMemorySize: Computed value negative: " << size);
    return 0;
    }
  
  return size / 1000;
}


//----------------------------------------------------------------------------
// Description:  
// This method is used translparently by the "SetInput(vtkImageCache *)"
// method to connect the image pipeline to the visualization pipeline.
vtkImageToStructuredPoints *vtkImageCache::GetImageToStructuredPoints()
{
  if ( ! this->ImageToStructuredPoints)
    {
    this->ImageToStructuredPoints = vtkImageToStructuredPoints::New();
    this->ImageToStructuredPoints->SetInput(this);
    }
  
  return this->ImageToStructuredPoints;
}

//----------------------------------------------------------------------------
// Description:  
// This method uses "Spacing" and "WholeExtent" to compute Dimesions,
// Center, and Bounds.
void vtkImageCache::ComputeBounds()
{
  if (this->ComputeBoundsTime < this->GetMTime())
    {
    int idx;
    for (idx = 0; idx < 4; ++idx)
      {
      this->Dimensions[idx] = 
	this->WholeExtent[idx*2+1] - this->WholeExtent[idx*2] + 1;
      this->Bounds[idx*2] = this->Origin[idx] + 
	(float)(this->WholeExtent[idx*2]) * this->Spacing[idx];
      this->Bounds[idx*2+1] = this->Origin[idx] + 
	(float)(this->WholeExtent[idx*2+1]) * this->Spacing[idx];
      this->Center[idx] = 0.5 * (this->Bounds[idx*2] + this->Bounds[idx*2+1]);
      }
    this->ComputeBoundsTime.Modified();
    }
}

  







