/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRayCaster.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRayCaster - obsolete class

// .SECTION Description
// vtkRayCaster is an obsolete class. All functionality has moved in
// vtkVolumeRayCastMapper.

#ifndef __vtkRayCaster_h
#define __vtkRayCaster_h

#include "vtkObject.h"
#include "vtkObjectFactory.h"

class VTK_RENDERING_EXPORT vtkRayCaster : public vtkObject
{
public:
  static vtkRayCaster *New();
  vtkTypeRevisionMacro(vtkRayCaster,vtkObject);
  
#ifndef VTK_REMOVE_LEGACY_CODE
  
  float *GetPerspectiveViewRays()
    {VTK_LEGACY_METHOD(GetPersepctiveViewRays,"4.0"); return NULL;};
  
  void GetViewRaysSize( int* vtkNotUsed(size) ) // It is actually int size[2]
    {VTK_LEGACY_METHOD(GetViewRaysSize,"4.0");};


  float *GetParallelStartPosition( void )
    {VTK_LEGACY_METHOD(GetParallelStartPosition,"4.0"); return NULL;};

  float *GetParallelIncrements( void )
    {VTK_LEGACY_METHOD(GetParallelIncrements,"4.0"); return NULL;};

  
  void SetImageScale(int vtkNotUsed(level), float vtkNotUsed(scale))
    {VTK_LEGACY_METHOD(SetImageScale,"4.0");};

  float GetImageScale(int vtkNotUsed(level)) 
    {VTK_LEGACY_METHOD(GetImageScale,"4.0"); return 0.0;};

  int GetImageScaleCount( void ) 
    {VTK_LEGACY_METHOD(GetImageScaleCount,"4.0"); return 0;};

  void SetSelectedImageScaleIndex(int vtkNotUsed(level), 
                                  float vtkNotUsed(scale))
    {VTK_LEGACY_METHOD(SetSelectedImageScaleIndex,"4.0");};
  
  float GetSelectedImageScaleIndex(int vtkNotUsed(level))
    {VTK_LEGACY_METHOD(GetSelectedImageScaleIndex,"4.0"); return 0.0;};
  
  void SetViewRaysStepSize(int vtkNotUsed(level), float vtkNotUsed(scale))
    {VTK_LEGACY_METHOD(SetViewRaysStepSize,"4.0");};
  
  float GetViewRaysStepSize(int vtkNotUsed(level))
    {VTK_LEGACY_METHOD(GetViewRaysStepSize,"4.0");return 0.0;};

  int GetAutomaticScaleAdjustment() 
    {VTK_LEGACY_METHOD(GetAutomaticScaleAdjustment,"4.0");return 0;};
  
  void AutomaticScaleAdjustmentOn( void )
    {VTK_LEGACY_METHOD(AutomaticScaleAdjustmentOn,"4.0");};
  
  void AutomaticScaleAdjustmentOff( void )
    {VTK_LEGACY_METHOD(AutomaticScaleAdjustmentOff,"4.0");};

  void SetAutomaticScaleLowerLimit(float vtkNotUsed(scale))
    {VTK_LEGACY_METHOD(SetAutomaticScaleLowerLimit,"4.0");};

  float GetAutomaticScaleLowerLimit()
    {VTK_LEGACY_METHOD(GetAutomaticScaleLowerLimit,"4.0"); return 0.0;};

  void SetBilinearImageZoom(int vtkNotUsed(val))
    {VTK_LEGACY_METHOD(SetBilinearImageZoom,"4.0");};

  int GetBilinearImageZoom()
    {VTK_LEGACY_METHOD(GetBilinearImageZoom,"4.0"); return 0;};

  void BilinearImageZoomOn()
    {VTK_LEGACY_METHOD(BilinearImageZoomOn,"4.0");};

  void BilinearImageZoomOff()
    {VTK_LEGACY_METHOD(BilinearImageZoomOff,"4.0");};

  float GetTotalRenderTime()
    {VTK_LEGACY_METHOD(GetTotalRenderTime,"4.0"); return 0.0;};

  void SetNumberOfThreads(int vtkNotUsed(val))
    {VTK_LEGACY_METHOD(SetNumberOfThreads,"4.0");};

  int GetNumberOfThreads()
    {VTK_LEGACY_METHOD(GetNumberOfThreads,"4.0"); return 0;};

  int GetNumberOfSamplesTaken()
    {VTK_LEGACY_METHOD(GetNumberOfSamplesTaken,"4.0"); return 0;};
#endif
  
protected:
  vtkRayCaster() {};
  ~vtkRayCaster() {};

private:
  vtkRayCaster(const vtkRayCaster&);  // Not implemented.
  void operator=(const vtkRayCaster&);  // Not implemented.
};
#endif


