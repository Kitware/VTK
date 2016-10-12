/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCompositer
 * @brief   Super class for composite algorthms.
 *
 *
 * vtkCompositer operates in multiple processes.  Each compositer has
 * a render window.  They use vtkMultiProcessControllers to communicate
 * the color and depth buffer to process 0's render window.
 * It will not handle transparency well.
 *
 * @sa
 * vtkCompositeManager.
*/

#ifndef vtkCompositer_h
#define vtkCompositer_h

#include "vtkRenderingParallelModule.h" // For export macro
#include "vtkObject.h"

class vtkMultiProcessController;
class vtkCompositer;
class vtkDataArray;
class vtkFloatArray;
class vtkUnsignedCharArray;

class VTKRENDERINGPARALLEL_EXPORT vtkCompositer : public vtkObject
{
public:
  static vtkCompositer *New();
  vtkTypeMacro(vtkCompositer,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * This method gets called on every process.  The final image gets
   * put into pBuf and zBuf.
   */
  virtual void CompositeBuffer(vtkDataArray *pBuf, vtkFloatArray *zBuf,
                               vtkDataArray *pTmp, vtkFloatArray *zTmp);

  //@{
  /**
   * Access to the controller.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller,vtkMultiProcessController);
  //@}

  //@{
  /**
   * A hack to get a sub world until I can get communicators working.
   */
  vtkSetMacro(NumberOfProcesses, int);
  vtkGetMacro(NumberOfProcesses, int);
  //@}

  //@{
  /**
   * Methods that allocate and delete memory with special MPIPro calls.
   */
  static void DeleteArray(vtkDataArray* da);
  static void ResizeFloatArray(vtkFloatArray* fa, int numComp,
                               vtkIdType size);
  static void ResizeUnsignedCharArray(vtkUnsignedCharArray* uca,
                                      int numComp, vtkIdType size);
  //@}

protected:
  vtkCompositer();
  ~vtkCompositer();

  vtkMultiProcessController *Controller;
  int NumberOfProcesses;

private:
  vtkCompositer(const vtkCompositer&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCompositer&) VTK_DELETE_FUNCTION;
};

#endif
