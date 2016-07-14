/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSurfaceLICHelper -- Support code for surface LIC
// .SECTION Description
// A small collection of noise routines for LIC
#ifndef vtkSurfaceLICHelper_h
#define vtkSurfaceLICHelper_h

#include "vtkRenderingLICOpenGL2Module.h" // for export
#include "vtkPixelExtent.h"
#include "vtkWeakPointer.h"
#include "vtkSmartPointer.h"
#include "vtkOpenGLHelper.h"
#include "vtk_glew.h"
#include "vtkTextureObject.h"

#include <deque> // for methods

class vtkFrameBufferObject2;
class vtkOpenGLRenderWindow;
class vtkPainterCommunicator;
class vtkImageData;
class vtkSurfaceLICComposite;
class vtkLineIntegralConvolution2D;
class vtkRenderer;
class vtkActor;
class vtkDataObject;

class vtkSurfaceLICHelper
{
public:
  vtkSurfaceLICHelper();
  ~vtkSurfaceLICHelper();

  // Description:
  // Check for OpenGL support
  static bool IsSupported(vtkOpenGLRenderWindow *context);

  // Description:
  // Free textures and shader programs we're holding a reference to.
  void ReleaseGraphicsResources(vtkWindow *win);

  // Description:
  // Free textures we're holding a reference to.
  void ClearTextures();

  // Description:
  // Allocate textures.
  void AllocateTextures(
        vtkOpenGLRenderWindow *context,
        int *viewsize);

  // Description:
  // Allocate a size texture, store in the given smart pointer.
  void AllocateTexture(
        vtkOpenGLRenderWindow *context,
        int *viewsize,
        vtkSmartPointer<vtkTextureObject> &tex,
        int filter = vtkTextureObject::Nearest);

  // Description:
  // Allocate a size texture, store in the given smart pointer.
  void AllocateDepthTexture(
        vtkOpenGLRenderWindow *context,
        int *viewsize,
        vtkSmartPointer<vtkTextureObject> &tex);

  // Description:
  // After LIC has been computed reset/clean internal state
  void Updated();

  // Description:
  // Force all stages to re-execute. Necessary if the
  // context or communicator changes.
  void UpdateAll();

  // Description:
  // Convert viewport to texture coordinates
  void ViewportQuadTextureCoords(GLfloat *tcoords)
    {
    tcoords[0] = tcoords[2] = 0.0f;
    tcoords[1] = tcoords[3] = 1.0f;
    }

  // Description:
  // Convert a viewport to a bounding box and it's texture coordinates for a
  // screen size texture.
  void ViewportQuadPoints(const vtkPixelExtent &viewportExt, GLfloat *quadpts)
    {
    viewportExt.GetData(quadpts);
    }

  // Description:
  // Convert a viewport to a bounding box and it's texture coordinates for a
  // screen size texture.
  void ViewportQuadTextureCoords(
        const vtkPixelExtent &viewExt,
        const vtkPixelExtent &viewportExt,
        GLfloat *tcoords);

  // Description:
  // Convert the entire view to a bounding box and it's texture coordinates for
  // a screen size texture.
  void ViewQuadPoints(GLfloat *quadpts)
    {
    quadpts[0] = quadpts[2] = 0.0f;
    quadpts[1] = quadpts[3] = 1.0f;
    }

  // Description:
  // Convert the entire view to a bounding box and it's texture coordinates for
  // a screen size texture.
  void ViewQuadTextureCoords(GLfloat *tcoords)
    {
    tcoords[0] = tcoords[2] = 0.0f;
    tcoords[1] = tcoords[3] = 1.0f;
    }

  // Description:
  // Render a quad (to trigger a shader to run)
  void RenderQuad(
        const vtkPixelExtent &viewExt,
        const vtkPixelExtent &viewportExt,
        vtkOpenGLHelper *cbo);

  // Description:
  // Compute the index into the 4x4 OpenGL ordered matrix.
  inline int idx(int row, int col) { return 4*col+row; }

  // Description:
  // given a axes aligned bounding box in
  // normalized device coordinates test for
  // view frustum visibility.
  // if all points are outside one of the
  // view frustum planes then this box
  // is not visible. we might have false
  // positive where more than one clip
  // plane intersects the box.
  bool VisibilityTest(double ndcBBox[24]);

  // Description:
  // Given world space bounds,
  // compute bounding boxes in clip and normalized device
  // coordinates and perform view frustum visiblity test.
  // return true if the bounds are visible. If so the passed
  // in extent object is initialized with the corresponding
  //screen space extents.
  bool ProjectBounds(
          double PMV[16],
          int viewsize[2],
          double bounds[6],
          vtkPixelExtent &screenExt);

  // Description:
  // Compute screen space extents for each block in the input
  // dataset and for the entire dataset. Only visible blocks
  // are used in the computations.
  int ProjectBounds(
        vtkRenderer *ren,
        vtkActor *actor,
        vtkDataObject *dobj,
        int viewsize[2],
        vtkPixelExtent &dataExt,
        std::deque<vtkPixelExtent> &blockExts);

  // Description:
  // Shrink an extent to tightly bound non-zero values
  void GetPixelBounds(float *rgba, int ni, vtkPixelExtent &ext);

  // Description:
  // Shrink a set of extents to tightly bound non-zero values
  // cull extent if it's empty
  void GetPixelBounds(float *rgba, int ni, std::deque<vtkPixelExtent> &blockExts);

  static void StreamingFindMinMax(
    vtkFrameBufferObject2 *fbo,
    std::deque<vtkPixelExtent> &blockExts,
    float &min, float &max);

  vtkSmartPointer<vtkImageData> Noise;
  vtkSmartPointer<vtkTextureObject> NoiseImage;
  vtkSmartPointer<vtkTextureObject> DepthImage;
  vtkSmartPointer<vtkTextureObject> GeometryImage;
  vtkSmartPointer<vtkTextureObject> VectorImage;
  vtkSmartPointer<vtkTextureObject> CompositeVectorImage;
  vtkSmartPointer<vtkTextureObject> MaskVectorImage;
  vtkSmartPointer<vtkTextureObject> CompositeMaskVectorImage;
  vtkSmartPointer<vtkTextureObject> LICImage;
  vtkSmartPointer<vtkTextureObject> RGBColorImage;
  vtkSmartPointer<vtkTextureObject> HSLColorImage;

  bool HasVectors;
  std::deque<vtkPixelExtent> BlockExts;

  vtkOpenGLHelper *ColorEnhancePass;
  vtkOpenGLHelper *CopyPass;
  vtkOpenGLHelper *ColorPass;

  int Viewsize[2];
  vtkSmartPointer<vtkSurfaceLICComposite> Compositor;
  vtkSmartPointer<vtkFrameBufferObject2> FBO;

  vtkSmartPointer<vtkLineIntegralConvolution2D> LICer;
  vtkPainterCommunicator *Communicator;
  vtkPixelExtent DataSetExt;

  vtkWeakPointer<vtkOpenGLRenderWindow> Context;

  bool ContextNeedsUpdate;
  bool CommunicatorNeedsUpdate;


protected:

};

#endif
// VTK-HeaderTest-Exclude: vtkSurfaceLICHelper.h
