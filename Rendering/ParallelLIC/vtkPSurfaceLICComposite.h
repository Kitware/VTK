/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPSurfaceLICComposite.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPSurfaceLICComposite -- To move data during parallel surface LIC
// .SECTION Description
// This class decomposes the image space and shuffles image space
// data onto the new decomposition with the necessary guard cells
// to prevent artifacts at the decomposition boundaries. After the
// image LIC is computed on the new decomposition this class will
// un-shuffle the computed LIC back onto the original decomposition.

#ifndef __vtkPSurfaceLICComposite_h
#define __vtkPSurfaceLICComposite_h

#include "vtkSurfaceLICComposite.h"
#include "vtkWeakPointer.h" // for ren context
#include "vtkOpenGLRenderWindow.h" // for context
#include "vtkRenderingParallelLICModule.h" // for export macro
#include "vtkPixelExtent.h" // for pixel extent
#include "vtkPPixelTransfer.h" // for pixel transfer
#include <deque> // for deque
#include <vector> // for vector
#include <list> // for list

class vtkFloatArray;
class vtkRenderWindow;
class vtkFrameBufferObject2;
class vtkTextureObject;
class vtkShaderProgram2;
class vtkPainterCommunicator;
class vtkPPainterCommunicator;
class vtkPPixelExtentOps;

class VTKRENDERINGPARALLELLIC_EXPORT vtkPSurfaceLICComposite : public vtkSurfaceLICComposite
{
public:
  static vtkPSurfaceLICComposite *New();
  vtkTypeMacro(vtkPSurfaceLICComposite, vtkSurfaceLICComposite);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Set the rendering context. Must set prior to use. Reference is not
  // held, so caller must ensure the renderer is not destroyed durring
  // use.
  virtual void SetContext(vtkOpenGLRenderWindow *rwin);
  virtual vtkOpenGLRenderWindow *GetContext(){ return this->Context; }

  // Description:
  // Set the communicator for parallel communication. The Default is
  // COMM_NULL.
   virtual void SetCommunicator(vtkPainterCommunicator *comm);

  // Description:
  // Build programs to move data to the new decomp
  // THIS IS A COLLECTIVE OPERATION
  virtual int BuildProgram(float *vectors);

  // Description:
  // Move a single buffer from the geometry decomp to the LIC decomp.
  // THIS IS A COLLECTIVE OPERATION
  virtual int Gather(
        void *pSendPBO,
        int dataType,
        int nComps,
        vtkTextureObject *&newImage);

  // Description:
  // Move a single buffer from the LIC decomp to the geometry decomp
  // THIS IS A COLLECTIVE OPERATION
  virtual int Scatter(
        void *pSendPBO,
        int dataType,
        int nComps,
        vtkTextureObject *&newImage);

protected:
  vtkPSurfaceLICComposite();
  ~vtkPSurfaceLICComposite();

private:
  // Description:
  // Load, compile, and link the shader.
  int InitializeCompositeShader(vtkOpenGLRenderWindow *context);

  // Description:
  // Composite incoming data.
  int ExecuteShader(const vtkPixelExtent &ext, vtkTextureObject *tex);

  // Description:
  // The communication cost to move from one decomposition to another
  // is given by the ratio of pixels to send off rank to the total
  // number of source pixels.
  double EstimateCommunicationCost(
        const std::deque<std::deque<vtkPixelExtent> > &srcExts,
        const std::deque<std::deque<vtkPixelExtent> > &destExts);

  // Description:
  // The efficiency of a decomposition is the ratio of useful pixels
  // to guard pixels. If this factor shrinks bellow 1 there may be
  // an issue.
  double EstimateDecompEfficiency(
        const std::deque< std::deque<vtkPixelExtent> > &exts,
        const std::deque< std::deque<vtkPixelExtent> > &guardExts);

  // Description:
  // Given a window extent, decompose into the requested number of
  // pieces.
  int DecomposeScreenExtent(
        std::deque< std::deque<vtkPixelExtent> >&newExts,
        float *vectors);

  // Description:
  // Given an extent, decompose into the requested number of
  // pieces.
  int DecomposeExtent(
      vtkPixelExtent &in,
      int nPieces,
      std::list<vtkPixelExtent> &out);

  // Description:
  // For parallel run. Make a decomposition disjoint. Sorts extents
  // and processes largest to smallest , repeatedly subtracting smaller
  // remaining blocks from the largest remaining.  Each extent in the
  // new disjoint set is shrunk to tightly bound the vector data,
  // extents with empty vectors are removed. This is a global operation
  // as the vector field is distributed and has not been composited yet.
  int MakeDecompDisjoint(
        const std::deque< std::deque< vtkPixelExtent> > &in,
        std::deque< std::deque< vtkPixelExtent> > &out,
        float *vectors);


  // decomp set of extents
  int MakeDecompLocallyDisjoint(
       const std::deque< std::deque< vtkPixelExtent> > &in,
       std::deque< std::deque< vtkPixelExtent> > &out);

  using vtkSurfaceLICComposite::MakeDecompDisjoint;

  // Description:
  // All gather geometry domain decomposition. The extent of local
  // blocks are passed in, the collection of all blocks is returned
  // along with the dataset extent.
  int AllGatherExtents(
        const std::deque<vtkPixelExtent> &localExts,
        std::deque<std::deque<vtkPixelExtent> > &remoteExts,
        vtkPixelExtent &dataSetExt);

  // Description:
  // All reduce max(|V|) on the new decomposition.
  int AllReduceVectorMax(
        const std::deque<vtkPixelExtent> &originalExts,
        const std::deque<std::deque<vtkPixelExtent> > &newExts,
        float *vectors,
        std::vector<std::vector<float> > &vectorMax);

  // Description:
  // Add guard pixels (Parallel run)
  int AddGuardPixels(
      const std::deque<std::deque<vtkPixelExtent> > &exts,
      std::deque<std::deque<vtkPixelExtent> > &guardExts,
      std::deque<std::deque<vtkPixelExtent> > &disjointGuardExts,
      float *vectors);

private:
  vtkPPainterCommunicator *PainterComm;          // mpi state
  vtkPPixelExtentOps *PixelOps;
  int CommRank;
  int CommSize;

  vtkWeakPointer<vtkOpenGLRenderWindow> Context; // rendering context
  vtkFrameBufferObject2 *FBO;                    // buffer object
  vtkShaderProgram2 *CompositeShader;            // shader program for compositing

  std::deque<vtkPPixelTransfer> GatherProgram;   // ordered steps required to move data to new decomp
  std::deque<vtkPPixelTransfer> ScatterProgram;  // ordered steps required to unmove data from new decomp

  friend
  ostream &operator<<(ostream &os, vtkPSurfaceLICComposite &ss);

  vtkPSurfaceLICComposite(const vtkPSurfaceLICComposite&); // Not implemented
  void operator=(const vtkPSurfaceLICComposite&); // Not implemented
};

ostream &operator<<(ostream &os, vtkPSurfaceLICComposite &ss);

#endif
