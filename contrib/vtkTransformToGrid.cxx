/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformToGrid.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkTransformToGrid.h"
#include "vtkObjectFactory.h"
#include "vtkIdentityTransform.h"

//----------------------------------------------------------------------------
vtkTransformToGrid* vtkTransformToGrid::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTransformToGrid");
  if(ret)
    {
    return (vtkTransformToGrid*)ret;
    }

  return new vtkTransformToGrid;
}

//----------------------------------------------------------------------------
vtkTransformToGrid::vtkTransformToGrid()
{
  this->Input = NULL;
  
  this->GridScalarType = VTK_FLOAT;

  for (int i = 0; i < 3; i++)
    {
    this->GridExtent[2*i] = this->GridExtent[2*i+1] = 0;
    this->GridOrigin[i] = 0.0f;
    this->GridSpacing[i] = 1.0f;
    }

  this->DisplacementScale = 1.0f;
  this->DisplacementShift = 0.0f;
}

//----------------------------------------------------------------------------
vtkTransformToGrid::~vtkTransformToGrid()
{
  this->SetInput(NULL);
}

//----------------------------------------------------------------------------
void vtkTransformToGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  int i;

  vtkImageSource::PrintSelf(os,indent);

  os << indent << "Input: (" << this->Input << ")\n"; 

  os << indent << "GridSpacing: (" << this->GridSpacing[0];
  for (i = 1; i < 3; ++i)
    {
    os << ", " << this->GridSpacing[i];
    }
  os << ")\n";
  
  os << indent << "GridOrigin: (" << this->GridOrigin[0];
  for (i = 1; i < 3; ++i)
    {
    os << ", " << this->GridOrigin[i];
    }
  os << ")\n";

  os << indent << "GridExtent: (" << this->GridExtent[0];
  for (i = 1; i < 6; ++i)
    {
    os << ", " << this->GridExtent[i];
    }
  os << ")\n";

  os << indent << "GridScalarType: " <<
    vtkImageScalarTypeNameMacro(this->GridScalarType) << "\n";

  this->UpdateShiftScale();

  os << indent << "DisplacementScale: " << this->DisplacementScale << "\n";
  os << indent << "DisplacementShift: " << this->DisplacementShift << "\n";
}

//----------------------------------------------------------------------------
// This method returns the largest data that can be generated.
void vtkTransformToGrid::ExecuteInformation()
{
  if (this->GetInput() == NULL)
    {
    vtkErrorMacro("Missing input");
    return;
    }

  this->Input->Update();

  this->GetOutput()->SetWholeExtent(this->GridExtent);
  this->GetOutput()->SetSpacing(this->GridSpacing);
  this->GetOutput()->SetOrigin(this->GridOrigin);
  this->GetOutput()->SetScalarType(this->GridScalarType);
  this->GetOutput()->SetNumberOfScalarComponents(3);
}

//----------------------------------------------------------------------------
// Return the maximum absolute displacement of the transform over
// the entire grid extent -- this is extremely robust and extremely
// inefficient, it should be possible to do much better than this.
static void vtkTransformToGridMinMax(vtkTransformToGrid *self,
				     int extent[6],
				     float &minDisplacement,
				     float &maxDisplacement)
{
  vtkAbstractTransform *transform = self->GetInput();
  transform->Update();

  if (!transform)
    {
    minDisplacement = -1.0f;
    maxDisplacement = +1.0f;
    return;
    }

  float *spacing = self->GetGridSpacing();
  float *origin = self->GetGridOrigin();

  maxDisplacement = -1e37;
  minDisplacement = +1e37;

  float point[3],newPoint[3],displacement;

  for (int k = extent[4]; k <= extent[5]; k++)
    {
    point[2] = k*spacing[2] + origin[2];
    for (int j = extent[2]; j <= extent[3]; j++)
      {
      point[1] = j*spacing[1] + origin[1];
      for (int i = extent[0]; i <= extent[1]; i++)
	{
	point[0] = i*spacing[0] + origin[0];

	transform->InternalTransformPoint(point,newPoint);

	for (int l = 0; l < 3; l++)
	  {
	  displacement = newPoint[l] - point[l];

	  if (displacement > maxDisplacement)
	    {
	    maxDisplacement = displacement;
	    }

	  if (displacement < minDisplacement)
	    {
	    minDisplacement = displacement;
	    }
	  }
	} 
      }
    }
} 

//----------------------------------------------------------------------------
void vtkTransformToGrid::UpdateShiftScale()
{
  int gridType = this->GridScalarType;

  // nothing to do for float or double
  if (gridType == VTK_FLOAT || gridType == VTK_DOUBLE)
    {
    this->DisplacementShift = 0.0f;
    this->DisplacementScale = 1.0f;
    vtkDebugMacro(<< "displacement (scale, shift) = (" << 
                  this->DisplacementScale << ", " << 
                  this->DisplacementShift << ")");  
    return;
    }

  // check mtime
  if (this->ShiftScaleTime.GetMTime() > this->GetMTime())
    {
    return;
    }

  // get the maximum displacement
  float minDisplacement, maxDisplacement;
  vtkTransformToGridMinMax(this,this->GridExtent,
			   minDisplacement,
			   maxDisplacement);

  vtkDebugMacro(<< "displacement (min, max) = (" << 
                minDisplacement << ", " << maxDisplacement << ")");

  float typeMin,typeMax;

  switch (gridType)
    {
    case VTK_SHORT:
      typeMin = VTK_SHORT_MIN;
      typeMax = VTK_SHORT_MAX;
      break;
    case VTK_UNSIGNED_SHORT:
      typeMin = VTK_UNSIGNED_SHORT_MIN;
      typeMax = VTK_UNSIGNED_SHORT_MAX;
      break;
    case VTK_CHAR:
      typeMin = VTK_CHAR_MIN;
      typeMax = VTK_CHAR_MAX;
      break;
    case VTK_UNSIGNED_CHAR:
      typeMin = VTK_UNSIGNED_CHAR_MIN;
      typeMax = VTK_UNSIGNED_CHAR_MAX;
      break;
    default:
      vtkErrorMacro(<< "UpdateShiftScale: Unknown input ScalarType");
      return;
    }
  
  this->DisplacementScale = ((maxDisplacement - minDisplacement)/
			     (typeMax - typeMin));
  this->DisplacementShift = ((typeMax*minDisplacement-typeMin*maxDisplacement)/
			     (typeMax - typeMin)); 

  if (this->DisplacementScale == 0.0f)
    {
    this->DisplacementScale = 1.0f;
    }

  vtkDebugMacro(<< "displacement (scale, shift) = (" << 
                this->DisplacementScale << ", " << 
                this->DisplacementShift << ")");  

  this->ShiftScaleTime.Modified();
}

//----------------------------------------------------------------------------
// macros to ensure proper round-to-nearest behaviour

static inline void vtkGridRound(float val, unsigned char& rnd)
{
  rnd = (unsigned char)(val+0.5f);
}

static inline void vtkGridRound(float val, char& rnd)
{
  rnd = (char)((val+128.5f)-128);
}

static inline void vtkGridRound(float val, short& rnd)
{
  rnd = (short)((int)(val+32768.5f)-32768);
}

static inline void vtkGridRound(float val, unsigned short& rnd)
{
  rnd = (unsigned short)(val+0.5f);
}

static inline void vtkGridRound(float val, float& rnd)
{
  rnd = (float)(val);
}

//----------------------------------------------------------------------------
template<class T>
static void vtkTransformToGridExecute(vtkTransformToGrid *self,
			       vtkImageData *grid, T *gridPtr, int extent[6], 
			       float shift, float scale,
			       int id)
{
  vtkAbstractTransform *transform = self->GetInput();
  int isIdentity = 0;
  if (transform == 0)
    {
    transform = vtkIdentityTransform::New();
    isIdentity = 1;
    }

  float *spacing = grid->GetSpacing();
  float *origin = grid->GetOrigin();
  int *increments = grid->GetIncrements();

  float invScale = 1.0f/scale;

  float point[3];
  float newPoint[3];

  T *gridPtr0 = gridPtr;

  unsigned long count = 0;
  unsigned long target = (unsigned long)
    ((extent[5]-extent[4]+1)*(extent[3]-extent[2]+1)/50.0);
  target++;

  for (int k = extent[4]; k <= extent[5]; k++)
    {
    point[2] = k*spacing[2] + origin[2];
    T *gridPtr1 = gridPtr0;

    for (int j = extent[2]; j <= extent[3]; j++)
      {

      if (id == 0)
	{
	if (count % target == 0)
	  {
	  self->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}

      point[1] = j*spacing[1] + origin[1];
      gridPtr = gridPtr1;

      for (int i = extent[0]; i <= extent[1]; i++)
	{
	point[0] = i*spacing[0] + origin[0];

	transform->InternalTransformPoint(point,newPoint);
	
	vtkGridRound((newPoint[0] - point[0] - shift)*invScale,*gridPtr++);
	vtkGridRound((newPoint[1] - point[1] - shift)*invScale,*gridPtr++);
	vtkGridRound((newPoint[2] - point[2] - shift)*invScale,*gridPtr++);
	} 

      gridPtr1 += increments[1];
      }

    gridPtr0 += increments[2];
    }

  if (isIdentity)
    {
    transform->Delete();
    }
}       

//----------------------------------------------------------------------------
void vtkTransformToGrid::ExecuteData(vtkDataObject *output)
{
  vtkImageData *grid = this->AllocateOutputData(output);
  int *extent = grid->GetExtent();

  float *gridPtr = (float *)grid->GetScalarPointerForExtent(extent);
  int gridType = grid->GetScalarType();

  this->UpdateShiftScale();

  float scale = this->DisplacementScale;
  float shift = this->DisplacementShift;

  int id = 0;

  switch (gridType)
    {
    case VTK_FLOAT:
      vtkTransformToGridExecute(this, grid, (float *)(gridPtr), extent,
				shift,scale,id);
      break;
    case VTK_SHORT:
      vtkTransformToGridExecute(this, grid, (short *)(gridPtr), extent,
				shift,scale,id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkTransformToGridExecute(this, grid, (unsigned short *)(gridPtr),extent,
				shift,scale,id);
      break;
    case VTK_CHAR:
      vtkTransformToGridExecute(this, grid, (char *)(gridPtr), extent,
				shift,scale,id);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkTransformToGridExecute(this, grid, (unsigned char *)(gridPtr), extent,
				shift,scale,id);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
    }
}

//----------------------------------------------------------------------------
unsigned long vtkTransformToGrid::GetMTime()
{
  unsigned long mtime = this->vtkImageSource::GetMTime();

  if (this->Input)
    {
    unsigned long mtime2 = this->Input->GetMTime();
    if (mtime2 > mtime)
      {
      mtime = mtime2;
      }
    }

  return mtime;
}
