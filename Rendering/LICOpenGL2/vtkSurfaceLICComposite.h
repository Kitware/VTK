/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSurfaceLICComposite.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSurfaceLICComposite
 *
 * This class decomposes the image space and shuffles image space
 * data onto the new decomposition with the necessary guard cells
 * to prevent artifacts at the decomposition boundaries. After the
 * image LIC is computed on the new decomposition this class will
 * un-shuffle the computed LIC back onto the original decomposition
*/

#ifndef vtkSurfaceLICComposite_h
#define vtkSurfaceLICComposite_h

#include "vtkObject.h"
#include "vtkRenderingLICOpenGL2Module.h" // for export macro
#include "vtkPixelExtent.h" // for pixel extent
#include <deque> // for deque
#include <vector> // for vector

class vtkFloatArray;
class vtkOpenGLRenderWindow;
class vtkTextureObject;
class vtkPainterCommunicator;

class VTKRENDERINGLICOPENGL2_EXPORT vtkSurfaceLICComposite : public vtkObject
{
public:
  static vtkSurfaceLICComposite *New();
  vtkTypeMacro(vtkSurfaceLICComposite, vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  /**
   * Initialize the object based on the following description of the
   * blocks projected onto the render window. wholeExt describes the
   * window size, originalExts describe each block's extent in window
   * coords. stepSize is the window coordiniate integration step size.
   * when inplace is true compositing happens on the original extent.
   */
  void Initialize(
        const vtkPixelExtent &winExt,
        const std::deque<vtkPixelExtent> &blockExts,
        int strategy,
        double stepSize,
        int nSteps,
        int normalizeVectors,
        int enhancedLIC,
        int anitalias);

  /**
   * Control the screen space decomposition. The available modes are:

   * INPLACE
   * use the block decomp. This may result in LIC being computed
   * many times for the same pixels and an excessive amount of
   * IPC during compositing if any of the block extents cover
   * or intersect a number of block extents. The input data
   * needs to be shuffled but not unshuffled since for overlapping
   * regions LIC is computed by all proccesses that overlap.
   * If there is very little overlap between block extents
   * then this method is superior since no unshuffle is needed.

   * INPLACE_DISJOINT
   * use a disjoint version of the block decomp. This will leave
   * non-overlapping data in place, reasigning overlaping regions
   * so that LIC is computed once for each pixel on the screen.
   * An unshuffle step to move data in overlapping region to all
   * processes that overlap.

   * BALANCED
   * move to a new decomp where each rank gets an equal number
   * of pixels. This ensures the best load balancing during LIC
   * and that LIC is computed once for each pixel. In the worst
   * case each pixel will be shuffled and unshuffled.

   * AUTO
   * Use a heuristic to select the mode.
   */
  enum {
    COMPOSITE_INPLACE=0,
    COMPOSITE_INPLACE_DISJOINT,
    COMPOSITE_BALANCED,
    COMPOSITE_AUTO
  };
  void SetStrategy(int val){ this->Strategy = val; }
  int GetStrategy(){ return this->Strategy; }

  /**
   * Get the number of new extents assigned to this rank after
   * the decomposition.
   */
  int GetNumberOfCompositeExtents() const
    { return static_cast<int>(this->CompositeExt.size()); }

  /**
   * Get the extent of the domain over which to compute the LIC. This can
   * be querried only after the Composite takes place.
   */
  const vtkPixelExtent &GetGuardExtent(int i=0) const
    { return this->GuardExt[i]; }

  const std::deque<vtkPixelExtent> &GetGuardExtents() const
    { return this->GuardExt; }

  /**
   * Get the extent of the domain over which to compute the LIC. This can
   * be querried only after the Composite takes place.
   */
  const vtkPixelExtent &GetDisjointGuardExtent(int i=0) const
    { return this->DisjointGuardExt[i]; }

  const std::deque<vtkPixelExtent> &GetDisjointGuardExtents() const
    { return this->GuardExt; }

  /**
   * Get the extent of the domain over which to compute the LIC. This can
   * be querried only after the Composite takes place.
   */
  const vtkPixelExtent &GetCompositeExtent(int i=0) const
    { return this->CompositeExt[i]; }

  const std::deque<vtkPixelExtent> &GetCompositeExtents() const
    { return this->CompositeExt; }

  /**
   * Get the whole dataset extent (all blocks).
   */
  const vtkPixelExtent &GetDataSetExtent() const
    { return this->DataSetExt; }

  /**
   * Get the whole window extent.
   */
  const vtkPixelExtent &GetWindowExtent() const
    { return this->WindowExt; }

  /**
   * Set up for a serial run, makes the decomp disjoint and adds
   * requisite guard pixles.
   */
  int InitializeCompositeExtents(float *vectors);

  /**
   * Set the rendering context. Must set prior to use. Reference is not
   * held, so caller must ensure the renderer is not destroyed durring
   * use.
   */
  virtual void SetContext(vtkOpenGLRenderWindow *){}
  virtual vtkOpenGLRenderWindow *GetContext(){ return NULL; }

  /**
   * Set the communicator for parallel communication. A duplicate
   * is not made. It is up to the caller to manage the life of
   * the communicator such that it is around while this class
   * needs it and is released after.
   */
  virtual void SetCommunicator(vtkPainterCommunicator*){}

  /**
   * Set the communicator to the default communicator
   */
  virtual void RestoreDefaultCommunicator(){}

  /**
   * Build programs to move data to the new decomp
   * In parallel THIS IS A COLLECTIVE OPERATION
   */
  virtual int BuildProgram(float*){ return -1; }

  /**
   * Move a single buffer from the geometry decomp to the LIC decomp.
   * THIS IS A COLLECTIVE OPERATION
   */
  virtual int Gather(void *, int, int, vtkTextureObject *&)
    { return -1; }

  /**
   * Move a single buffer from the LIC decomp to the geometry decomp
   * In parallel THIS IS A COLLECTIVE OPERATION
   */
  virtual int Scatter(void *, int, int, vtkTextureObject *&)
    { return -1; }

  /**
   * Make a decomposition disjoint with respect to itself. Extents are
   * removed from the input array and disjoint extents are appened onto
   * the output array. This is a local operation.
   */
  static
  int MakeDecompDisjoint(
        std::deque<vtkPixelExtent> &in,
        std::deque<vtkPixelExtent> &out);

protected:
  vtkSurfaceLICComposite();
  ~vtkSurfaceLICComposite();

  /**
   * For serial run. Make a decomposition disjoint. Sorts extents and
   * processes largest to smallest , repeatedly subtracting smaller
   * remaining blocks from the largest remaining. Each extent in the
   * new disjoint set is shrunk to tightly bound the vector data,
   * extents with empty vectors are removed. This is a local operation
   * since vector field is local.
   */
  int MakeDecompDisjoint(
        const std::deque<vtkPixelExtent> &in,
        std::deque<vtkPixelExtent> &out,
        float *vectors);

  /**
   * Compute max(V) on the given extent.
   */
  float VectorMax(
        const vtkPixelExtent &ext,
        float *vectors);

  /**
   * Compute max(V) on a set of extents. Neighboring extents are
   * including in the computation.
   */
  int VectorMax(
        const std::deque<vtkPixelExtent> &exts,
        float *vectors,
        std::vector<float> &vMax);

  /**
   * Add guard pixels (Serial run)
   */
  int AddGuardPixels(
      const std::deque<vtkPixelExtent> &exts,
      std::deque<vtkPixelExtent> &guardExts,
      std::deque<vtkPixelExtent> &disjointGuardExts,
      float *vectors);

  /**
   * shrink pixel extent based on non-zero alpha channel values
   */
  void GetPixelBounds(
      float *rgba,
      int ni,
      vtkPixelExtent &ext);

  /**
   * factor for determining extra padding for guard pixels.
   * depends on window aspect ratio because of anisotropic
   * transform to texture space. see note in implementation.
   */
  float GetFudgeFactor(int nx[2]);

protected:
  int Pass;                                    // id for mpi tagging

  vtkPixelExtent WindowExt;                    // screen extent (screen size)
  vtkPixelExtent DataSetExt;                   // screen extent of the dataset
  std::deque<vtkPixelExtent> BlockExts;        // screen extents of blocks

  std::deque<vtkPixelExtent> CompositeExt;     // screen extents after decomp
  std::deque<vtkPixelExtent> GuardExt;         // screen extents w/ guard cells
  std::deque<vtkPixelExtent> DisjointGuardExt; // screen extents w/ guard cells

  int Strategy;                                // control for parallel composite

  double StepSize;                             // window coordinates step size
  int NumberOfSteps;                           // number of integration steps
  int NormalizeVectors;                        // does integrator normailze
  int NumberOfGuardLevels;                     // 1.5 if enhanced LIC 1 otherwise
  int NumberOfEEGuardPixels;                   // 1 if enhanced LIC 0 otherwise
  int NumberOfAAGuardPixels;                   // n antialias passes

private:
  vtkSurfaceLICComposite(const vtkSurfaceLICComposite&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSurfaceLICComposite&) VTK_DELETE_FUNCTION;

  friend
  ostream &operator<<(ostream &os, vtkSurfaceLICComposite &ss);
};

ostream &operator<<(ostream &os, vtkSurfaceLICComposite &ss);

#endif
