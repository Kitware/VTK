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
  
  for (idx = 0; idx < 3; ++idx)
    {
    this->UpdateExtent[idx*2] = -VTK_LARGE_INTEGER;
    this->UpdateExtent[idx*2+1] = VTK_LARGE_INTEGER;
    this->WholeExtent[idx*2] = this->WholeExtent[idx*2+1] = 0;
    this->Spacing[idx] = 1.0;
    this->Origin[idx] = 0.0;
    }

  this->Source = NULL;
  
  // for automatic conversion
  this->ImageToStructuredPoints = NULL;

  // default is to save data,
  // (But caches automatically created by sources set ReleaseDataFlag to 1)
  this->ReleaseDataFlag = 0;
  this->DataReleased = 1;
  
  // Invalid data type
  // This will be changed when the filter gets updated or
  // the ScalarType is set explicitly
  this->ScalarType = VTK_VOID;
  this->NumberOfScalarComponents = 0;
  
  this->MemoryLimit = 500000;   // 500 MB
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
  
  vtkReferenceCount::PrintSelf(os,indent);

  os << indent << "MemoryLimit: " << this->MemoryLimit << endl;
  os << indent << "NumberOfScalarComponents: " << 
    this->NumberOfScalarComponents << endl;

  if ( this->Source )
    {
    os << indent << "Source: (" << this->Source << ").\n";
    }
  else
    {
    os << indent << "Source: (none).\n";
    }

  os << indent << "ReleaseDataFlag: " << this->ReleaseDataFlag << "\n";
  os << indent << "Data Released: " << this->DataReleased << "\n";
  os << indent << "ScalarType: "<<vtkImageScalarTypeNameMacro(this->ScalarType)
     << "\n";

  if ( this->ImageToStructuredPoints )
    {
    os << indent << "ImageToStructuredPoints: (" 
       << this->ImageToStructuredPoints << ")\n";
    }
  else
    {
    os << indent << "ImageToStructuredPoints: (none)\n";
    }
  
  os << indent << "Spacing: (" << this->Spacing[0];
  for (idx = 1; idx < 3; ++idx)
    {
    os << ", " << this->Spacing[idx];
    }
  os << ")\n";
  
  os << indent << "Origin: (" << this->Origin[0];
  for (idx = 1; idx < 3; ++idx)
    {
    os << ", " << this->Origin[idx];
    }
  os << ")\n";
  
  os << indent << "WholeExtent: (" << this->WholeExtent[0];
  for (idx = 1; idx < 6; ++idx)
    {
    os << ", " << this->WholeExtent[idx];
    }
  os << ")\n";

  os << indent << "UpdateExtent: (" << this->UpdateExtent[0];
  for (idx = 1; idx < 6; ++idx)
    {
    os << ", " << this->UpdateExtent[idx];
    }
  os << ")\n";
}
  
    
//----------------------------------------------------------------------------
void vtkImageCache::GetAxisUpdateExtent(int idx, int &min, int &max)
{
  if (idx > 2)
    {
    vtkWarningMacro("illegal axis!");
    return;
    }

  min = this->UpdateExtent[idx*2];
  max = this->UpdateExtent[idx*2+1];
}

//----------------------------------------------------------------------------
void vtkImageCache::SetAxisUpdateExtent(int idx, int min, int max)
{
  int modified = 0;
  
  if (idx > 2)
    {
    vtkWarningMacro("illegal axis!");
    return;
    }
  
  if (this->UpdateExtent[idx*2] != min)
    {
    modified = 1;
    this->UpdateExtent[idx*2] = min;
    }
  if (this->UpdateExtent[idx*2+1] != max)
    {
    modified = 1;
    this->UpdateExtent[idx*2+1] = max;
    }

  if (modified)
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkImageCache::SetUpdateExtent(int extent[6])
{
  int idx, modified = 0;
  
  for (idx = 0; idx < 6; ++idx)
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
				    int zMin, int zMax)
{
  int extent[6];

  extent[0] = xMin; extent[1] = xMax;
  extent[2] = yMin; extent[3] = yMax;
  extent[4] = zMin; extent[5] = zMax;
  
  this->SetUpdateExtent(extent);
}

//----------------------------------------------------------------------------
void vtkImageCache::SetUpdateExtentToWholeExtent()
{
  unsigned long pipelineMTime = this->GetPipelineMTime();
  
  // update if mtime indicates to do so
  if (pipelineMTime > this->ExecuteTime)
    {
    // Make sure image information is upto date
    this->UpdateImageInformation();
    }
  this->SetUpdateExtent(this->WholeExtent);
}


//----------------------------------------------------------------------------
void vtkImageCache::GetUpdateExtent(int extent[6])
{
  int idx;
  
  for (idx = 0; idx < 6; ++idx)
    {
    extent[idx] = this->UpdateExtent[idx];
    }
}
//----------------------------------------------------------------------------
void vtkImageCache::GetUpdateExtent(int &xMin, int &xMax, int &yMin, int &yMax,
				    int &zMin, int &zMax)
{
  xMin = this->UpdateExtent[0]; xMax = this->UpdateExtent[1];
  yMin = this->UpdateExtent[2]; yMax = this->UpdateExtent[3];
  zMin = this->UpdateExtent[4]; zMax = this->UpdateExtent[5];
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
  unsigned long pipelineMTime = this->GetPipelineMTime();
  
  // update if mtime indicates to do so
  if (this->Source && pipelineMTime > this->ExecuteTime)
    {
    this->Source->UpdateImageInformation();
    }
}

//----------------------------------------------------------------------------
// Description:
// Clip updateExtent so it will nopt be larger than WHoleExtent
void vtkImageCache::ClipUpdateExtentWithWholeExtent()
{
  int idx;
  
  // Clip the UpdateExtent with the WholeExtent
  for (idx = 0; idx < 3; ++idx)
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
void vtkImageCache::SetWholeExtent(int extent[6])
{
  int idx, modified = 0;
  
  for (idx = 0; idx < 6; ++idx)
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
void vtkImageCache::SetWholeExtent(int xMin, int xMax,
				   int yMin, int yMax, int zMin, int zMax)
{
  int extent[6];

  extent[0] = xMin; extent[1] = xMax;
  extent[2] = yMin; extent[3] = yMax;
  extent[4] = zMin; extent[5] = zMax;
  this->SetWholeExtent(extent);
}

//----------------------------------------------------------------------------
void vtkImageCache::GetWholeExtent(int extent[6])
{
  int idx;
  
  for (idx = 0; idx < 6; ++idx)
    {
    extent[idx] = this->WholeExtent[idx];
    }
}
//----------------------------------------------------------------------------
void vtkImageCache::GetWholeExtent(int &xMin, int &xMax, int &yMin, int &yMax,
				   int &zMin, int &zMax)
{
  xMin = this->WholeExtent[0]; xMax = this->WholeExtent[1];
  yMin = this->WholeExtent[2]; yMax = this->WholeExtent[3];
  zMin = this->WholeExtent[4]; zMax = this->WholeExtent[5];
}

//----------------------------------------------------------------------------
void vtkImageCache::GetDimensions(int dimensions[3])
{
  int idx;
  
  for (idx = 0; idx < 3; ++idx)
    {
    dimensions[idx] = this->WholeExtent[idx*2+1] - 
      this->WholeExtent[idx*2] + 1;
    }
}
//----------------------------------------------------------------------------
void vtkImageCache::GetDimensions(int &x, int &y, int &z)
{
  x = this->WholeExtent[1] - this->WholeExtent[0] + 1;
  y = this->WholeExtent[3] - this->WholeExtent[2] + 1;
  z = this->WholeExtent[5] - this->WholeExtent[4] + 1;
}

//----------------------------------------------------------------------------
void vtkImageCache::GetCenter(float center[3])
{
  int idx;
  
  for (idx = 0; idx < 3; ++idx)
    {
    center[idx] = this->Origin[idx] + 
      this->Spacing[idx] * (this->WholeExtent[idx*2+1] 
			    - this->WholeExtent[idx*2] + 1)/2.0;
    }
}
//----------------------------------------------------------------------------
void vtkImageCache::GetCenter(float &x, float &y, float &z)
{
  x = this->Origin[0] + 
    this->Spacing[0] * (this->WholeExtent[1] - this->WholeExtent[0] + 1)/2.0;
  y = this->Origin[1] + 
    this->Spacing[1] * (this->WholeExtent[3] - this->WholeExtent[2] + 1)/2.0;
  z = this->Origin[2] + 
    this->Spacing[2] * (this->WholeExtent[5] - this->WholeExtent[4] + 1)/2.0;
}

//----------------------------------------------------------------------------
void vtkImageCache::GetBounds(float bounds[6])
{
  int idx;
  
  for (idx = 0; idx < 3; ++idx)
    {
    bounds[idx*2] = this->Origin[idx] + this->Spacing[idx] * 
      this->WholeExtent[idx*2];
    bounds[idx*2+1] = this->Origin[idx] + this->Spacing[idx] * 
      this->WholeExtent[idx*2+1];
    }
}
//----------------------------------------------------------------------------
void 
vtkImageCache::GetBounds(float &xMin, float &xMax, float &yMin, float &yMax,
			 float &zMin, float &zMax)
{
  xMin = this->Origin[0] + this->Spacing[0] * this->WholeExtent[0];
  xMax = this->Origin[0] + this->Spacing[0] * this->WholeExtent[1];
  yMin = this->Origin[1] + this->Spacing[1] * this->WholeExtent[2];
  yMax = this->Origin[1] + this->Spacing[1] * this->WholeExtent[3];
  zMin = this->Origin[2] + this->Spacing[2] * this->WholeExtent[4];
  zMax = this->Origin[2] + this->Spacing[2] * this->WholeExtent[5];
}

//----------------------------------------------------------------------------
void vtkImageCache::SetGlobalReleaseDataFlag(int val)
{
  vtkDataSet::SetGlobalReleaseDataFlag(val);
}

//----------------------------------------------------------------------------
int  vtkImageCache::GetGlobalReleaseDataFlag()
{
  return vtkDataSet::GetGlobalReleaseDataFlag();
}

//----------------------------------------------------------------------------
// Description:
// Return flag indicating whether data should be released after use  
// by a filter. 
int vtkImageCache::ShouldIReleaseData()
{
  if ( vtkDataSet::GetGlobalReleaseDataFlag() || 
       this->ReleaseDataFlag ) return 1;
  else return 0;
}

//----------------------------------------------------------------------------
// Description:
// This method returns the memory that would be required for scalars on update.
// The returned value is in units KBytes.
// This method is used for determining when to stream.
long vtkImageCache::GetUpdateExtentMemorySize()
{
  long size = this->NumberOfScalarComponents;
  int idx;
  
  // Compute the number of scalars.
  for (idx = 0; idx < 3; ++idx)
    {
    size *= (this->UpdateExtent[idx*2+1] - this->UpdateExtent[idx*2] + 1);
    }
  
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
// Check to see if we own an ImageToStructuredPoints which has registered
// this cache.
void vtkImageCache::UnRegister(vtkObject* o)
{
  // this is the special test. I own ImageToStructuredPoints, but it has registered me.
  if (this->GetReferenceCount() == 2 && this->ImageToStructuredPoints != NULL &&
      this->ImageToStructuredPoints->GetInput() == this)
    {
    vtkImageToStructuredPoints *temp = this->ImageToStructuredPoints;
    this->ImageToStructuredPoints = NULL;    
    temp->Delete();
    }

  this->vtkReferenceCount::UnRegister(o);  
}

//----------------------------------------------------------------------------
float *vtkImageCache::GetScalarRange()
{
  static float range[2];
  this->GetScalarRange(range);
  return range;
}




