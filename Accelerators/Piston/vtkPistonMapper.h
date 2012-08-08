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
// .NAME vtkPistonMapper - draws vtkPistonDataObjects to the screen
// .SECTION Description
// vtkPistonMapper is comparable to vtkDataSetMapper for vtkPistonDataObjects.
// The important capability it has is to produce images without bringing
// data back to the CPU.

#ifndef __vtkPistonMapper_h
#define __vtkPistonMapper_h

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

  // Description:
  // Manually call this before any cuda filters are created
  // to use direct GPU rendering.
  static void InitCudaGL(vtkRenderWindow *rw);

  // Description:
  // Return true if using cuda interop feature otherwise false.
  inline static bool IsEnabledCudaGL()
    {
    return CudaGLInitted;
    }

  // Description:
  // A convenience method to reduce code duplication that gets
  // the input as the expected type or NULL.
  vtkPistonDataObject *GetPistonDataObjectInput(int port);

  // Description:
  // Make a shallow copy of this mapper.
  void ShallowCopy(vtkAbstractMapper *m);

  // Description:
  // Method initiates the mapping process. Generally sent by the actor
  // as each frame is rendered.
  virtual void Render(vtkRenderer *ren, vtkActor *a);

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *) {};

  // Description:
  // Return bounding box (array of six doubles) of data expressed as
  // (xmin,xmax, ymin,ymax, zmin,zmax).
  virtual double *GetBounds();
  virtual void GetBounds(double bounds[6])
    {this->vtkAbstractMapper3D::GetBounds(bounds);};

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Used by vtkHardwareSelector to determine if the prop supports hardware
  // selection.
  virtual bool GetSupportsSelection()
    { return false; }

  // Description:
  // Bring this algorithm's outputs up-to-date.
  // Overridden to allow specification on streaming piece.
  virtual void Update();

  // Description:
  // If you want only a part of the data, specify by setting the piece.
  vtkSetMacro(Piece, int);
  vtkGetMacro(Piece, int);
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);

  // Description:
  // Set the number of ghost cells to return.
  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);

protected:
  vtkPistonMapper();
  ~vtkPistonMapper();

  // Description:
  // Overridden to say that we take in vtkPistonDataObjects
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // Internal render methods
  void RenderOnCPU();
  void RenderOnGPU();
  void RenderImageDataOutline();

  int Piece;
  int NumberOfPieces;
  int GhostLevel;

private:
  vtkPistonMapper(const vtkPistonMapper&); // Not implemented.
  void operator=(const vtkPistonMapper&);  // Not implemented.

  // Description:
  // Allocates buffers that are shared between CUDA and GL
  void PrepareDirectRenderBuffers(int nPoints);

  static bool CudaGLInitted;

  class InternalInfo;
  InternalInfo *Internal;
};

#endif
