/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRayCaster.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
// .NAME vtkRayCaster - obsolete class

// .SECTION Description
// vtkRayCaster is an obsolete class. All functionality has moved in
// vtkVolumeRayCastMapper.

#ifndef __vtkRayCaster_h
#define __vtkRayCaster_h

#include "vtkObject.h"

class VTK_EXPORT vtkRayCaster : public vtkObject
{
public:
  static vtkRayCaster *New() {return new vtkRayCaster;};
  vtkTypeMacro(vtkRayCaster,vtkObject);
  
#ifndef VTK_REMOVE_LEGACY_CODE
  
  float *GetPerspectiveViewRays()
    {VTK_LEGACY_METHOD(GetPersepctiveViewRays,"4.0"); return NULL;};
  
  void GetViewRaysSize( int size[2] )
    {VTK_LEGACY_METHOD(GetViewRaysSize,"4.0");};


  float *GetParallelStartPosition( void )
    {VTK_LEGACY_METHOD(GetParallelStartPosition,"4.0"); return NULL;};

  float *GetParallelIncrements( void )
    {VTK_LEGACY_METHOD(GetParallelIncrements,"4.0"); return NULL;};

  
  void SetImageScale(int level, float scale)
    {VTK_LEGACY_METHOD(SetImageScale,"4.0");};

  float GetImageScale(int level) 
    {VTK_LEGACY_METHOD(GetImageScale,"4.0"); return 0.0;};

  int GetImageScaleCount( void ) 
    {VTK_LEGACY_METHOD(GetImageScaleCount,"4.0"); return 0;};

  void SetSelectedImageScaleIndex(int level, float scale)
    {VTK_LEGACY_METHOD(SetSelectedImageScaleIndex,"4.0");};
  
  float GetSelectedImageScaleIndex(int level)
    {VTK_LEGACY_METHOD(GetSelectedImageScaleIndex,"4.0"); return 0.0;};
  
  void SetViewRaysStepSize(int level, float scale)
    {VTK_LEGACY_METHOD(SetViewRaysStepSize,"4.0");};
  
  float GetViewRaysStepSize(int level)
    {VTK_LEGACY_METHOD(GetViewRaysStepSize,"4.0");return 0.0;};

  int GetAutomaticScaleAdjustment() 
    {VTK_LEGACY_METHOD(GetAutomaticScaleAdjustment,"4.0");return 0;};
  
  void AutomaticScaleAdjustmentOn( void )
    {VTK_LEGACY_METHOD(AutomaticScaleAdjustmentOn,"4.0");};
  
  void AutomaticScaleAdjustmentOff( void )
    {VTK_LEGACY_METHOD(AutomaticScaleAdjustmentOff,"4.0");};

  void SetAutomaticScaleLowerLimit(float scale)
    {VTK_LEGACY_METHOD(SetAutomaticScaleLowerLimit,"4.0");};

  float GetAutomaticScaleLowerLimit()
    {VTK_LEGACY_METHOD(GetAutomaticScaleLowerLimit,"4.0"); return 0.0;};

  void SetBilinearImageZoom(int val)
    {VTK_LEGACY_METHOD(SetBilinearImageZoom,"4.0");};

  int GetBilinearImageZoom()
    {VTK_LEGACY_METHOD(GetBilinearImageZoom,"4.0"); return 0;};

  void BilinearImageZoomOn()
    {VTK_LEGACY_METHOD(BilinearImageZoomOn,"4.0");};

  void BilinearImageZoomOff()
    {VTK_LEGACY_METHOD(BilinearImageZoomOff,"4.0");};

  float GetTotalRenderTime()
    {VTK_LEGACY_METHOD(GetTotalRenderTime,"4.0"); return 0.0;};

  void SetNumberOfThreads(int val)
    {VTK_LEGACY_METHOD(SetNumberOfThreads,"4.0");};

  int GetNumberOfThreads()
    {VTK_LEGACY_METHOD(GetNumberOfThreads,"4.0"); return 0;};

  int GetNumberOfSamplesTaken()
    {VTK_LEGACY_METHOD(GetNumberOfSamplesTaken,"4.0"); return 0;};
#endif
  
protected:
  vtkRayCaster() {};
  ~vtkRayCaster() {};
  vtkRayCaster(const vtkRayCaster&);
  void operator=(const vtkRayCaster&);

};

#endif


