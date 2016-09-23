/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPistonMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPistonMapper
 * @brief   draws vtkPistonDataObjects to the screen
 *
 * vtkPistonMapper is comparable to vtkDataSetMapper for vtkPistonDataObjects.
 * The important capability it has is to produce images without bringing
 * data back to the CPU.
*/

#ifndef vtkPistonMapper_h
#define vtkPistonMapper_h

#include "vtkAcceleratorsPistonModule.h" // For export macro
#include "vtkMapper.h"

class vtkActor;
class vtkRenderer;
class vtkPistonDataObject;
class vtkPistonScalarsColors;
class vtkRenderWindow;
class vtkWindow;

class VTKACCELERATORSPISTON_EXPORT vtkPistonMapper : public vtkMapper
{
public:
  static vtkPistonMapper *New();
  vtkTypeMacro(vtkPistonMapper,vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Manually call this before any cuda filters are created
   * to use direct GPU rendering.
   */
  static void InitCudaGL(vtkRenderWindow *rw);

  /**
   * Return true if using cuda interop feature otherwise false.
   */
  inline static bool IsEnabledCudaGL()
    {
    return CudaGLInitted;
    }

  /**
   * A convenience method to reduce code duplication that gets
   * the input as the expected type or NULL.
   */
  vtkPistonDataObject *GetPistonDataObjectInput(int port);

  /**
   * Make a shallow copy of this mapper.
   */
  void ShallowCopy(vtkAbstractMapper *m);

  /**
   * Method initiates the mapping process. Generally sent by the actor
   * as each frame is rendered.
   */
  virtual void Render(vtkRenderer *ren, vtkActor *a);

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow *) {}

  //@{
  /**
   * Return bounding box (array of six doubles) of data expressed as
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   */
  virtual double *GetBounds();
  virtual void GetBounds(double bounds[6])
    {this->vtkAbstractMapper3D::GetBounds(bounds);};
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Used by vtkHardwareSelector to determine if the prop supports hardware
   * selection.
   */
  virtual bool GetSupportsSelection()
    { return false; }

  //@{
  /**
   * Bring this algorithm's outputs up-to-date.
   * Overridden to allow specification on streaming piece.
   */
  virtual void Update();
  // Use the other overloads of Update.
  using vtkAlgorithm::Update;
  //@}

  //@{
  /**
   * If you want only a part of the data, specify by setting the piece.
   */
  vtkSetMacro(Piece, int);
  vtkGetMacro(Piece, int);
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);
  //@}

  //@{
  /**
   * Set the number of ghost cells to return.
   */
  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);
  //@}

protected:
  vtkPistonMapper();
  ~vtkPistonMapper();

  /**
   * Overridden to say that we take in vtkPistonDataObjects
   */
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  //@{
  /**
   * Internal render methods
   */
  void RenderOnCPU();
  void RenderOnGPU();
  void RenderImageDataOutline();
  //@}

  int Piece;
  int NumberOfPieces;
  int GhostLevel;

private:
  vtkPistonMapper(const vtkPistonMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPistonMapper&) VTK_DELETE_FUNCTION;

  /**
   * Allocates buffers that are shared between CUDA and GL
   */
  void PrepareDirectRenderBuffers(int nPoints);

  static bool CudaGLInitted;

  class InternalInfo;
  InternalInfo *Internal;
};

#endif
