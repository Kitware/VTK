/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeRenderManager.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCompositeRenderManager
 * @brief   An object to control sort-last parallel rendering.
 *
 *
 * vtkCompositeRenderManager is a subclass of vtkParallelRenderManager that
 * uses compositing to do parallel rendering.  This class has
 * replaced vtkCompositeManager.
 *
*/

#ifndef vtkCompositeRenderManager_h
#define vtkCompositeRenderManager_h

#include "vtkRenderingParallelModule.h" // For export macro
#include "vtkParallelRenderManager.h"

class vtkCompositer;
class vtkFloatArray;

class VTKRENDERINGPARALLEL_EXPORT vtkCompositeRenderManager : public vtkParallelRenderManager
{
public:
  vtkTypeMacro(vtkCompositeRenderManager, vtkParallelRenderManager);
  static vtkCompositeRenderManager *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  //@{
  /**
   * Set/Get the composite algorithm.
   */
  void SetCompositer(vtkCompositer *c);
  vtkGetObjectMacro(Compositer, vtkCompositer);
  //@}

  //@{
  /**
   * Get rendering metrics.
   */
  vtkGetMacro(ImageProcessingTime, double);
  //@}

protected:
  vtkCompositeRenderManager();
  ~vtkCompositeRenderManager();

  vtkCompositer *Compositer;

  virtual void PreRenderProcessing();
  virtual void PostRenderProcessing();

  vtkFloatArray *DepthData;
  vtkUnsignedCharArray *TmpPixelData;
  vtkFloatArray *TmpDepthData;

  int SavedMultiSamplesSetting;

private:
  vtkCompositeRenderManager(const vtkCompositeRenderManager &) VTK_DELETE_FUNCTION;
  void operator=(const vtkCompositeRenderManager &) VTK_DELETE_FUNCTION;
};

#endif //vtkCompositeRenderManager_h
