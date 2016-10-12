/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSurfaceLICHelper.h"

#include "vtk_glew.h"
#include "vtkFrameBufferObject2.h"
#include "vtkPixelBufferObject.h"
#include "vtkPainterCommunicator.h"
#include "vtkLineIntegralConvolution2D.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkBoundingBox.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkMatrix4x4.h"
#include "vtkOpenGLCamera.h"
#include "vtkRenderer.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLError.h"
#include "vtkDataSet.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkSurfaceLICComposite.h"

#include <vector>

// Description
// find min/max of unmasked fragments across all regions
// download each search each region individually
void vtkSurfaceLICHelper::StreamingFindMinMax(
      vtkFrameBufferObject2 *fbo,
      std::deque<vtkPixelExtent> &blockExts,
      float &min,
      float &max)
{
  size_t nBlocks = blockExts.size();
  // initiate download
  fbo->ActivateReadBuffer(1U);
  vtkStaticCheckFrameBufferStatusMacro(GL_FRAMEBUFFER);
  std::vector<vtkPixelBufferObject*> pbos(nBlocks, NULL);
  for (size_t e=0; e<nBlocks; ++e)
  {
    pbos[e] = fbo->Download(
          blockExts[e].GetData(),
          VTK_FLOAT,
          4,
          GL_FLOAT,
          GL_RGBA);
  }
  fbo->RemoveTexColorAttachment(GL_DRAW_FRAMEBUFFER, 0U);
  fbo->RemoveTexColorAttachment(GL_DRAW_FRAMEBUFFER, 1U);
  fbo->DeactivateDrawBuffers();
  fbo->DeactivateReadBuffer();
  // map search and release each region
  for (size_t e=0; e<nBlocks; ++e)
  {
    vtkPixelBufferObject *&pbo = pbos[e];
    float *pColors = (float*)pbo->MapPackedBuffer();

    size_t n = blockExts[e].Size();
    for (size_t i = 0; i<n; ++i)
    {
      if (pColors[4*i+3] != 0.0f)
      {
        float L = pColors[4*i+2];
        min = min > L ? L : min;
        max = max < L ? L : max;
      }
    }
    pbo->UnmapPackedBuffer();
    pbo->Delete();
    pbo = NULL;
  }
  #if vtkSurfaceLICMapperDEBUG >= 1
  cerr << "min=" << min << " max=" << max << endl;
  #endif
}

// Description:
// Constructor
vtkSurfaceLICHelper::vtkSurfaceLICHelper()
{
  this->Viewsize[0] = this->Viewsize[1] = 0;

  this->ContextNeedsUpdate = true;
  this->CommunicatorNeedsUpdate = true;

  this->Communicator = new vtkPainterCommunicator;

  this->HasVectors = false;

  this->ColorPass = NULL;
  this->ColorEnhancePass = NULL;
  this->CopyPass = NULL;
}

// Description:
// Destructor
vtkSurfaceLICHelper::~vtkSurfaceLICHelper()
{
  this->ReleaseGraphicsResources(NULL);

  if (this->ColorPass)
  {
    delete this->ColorPass;
  }
  if (this->ColorEnhancePass)
  {
    delete this->ColorEnhancePass;
  }
  if (this->CopyPass)
  {
    delete this->CopyPass;
  }
  this->ColorPass = NULL;
  this->ColorEnhancePass = NULL;
  this->CopyPass = NULL;

  delete this->Communicator;
}

// Description:
// Check for OpenGL support
bool vtkSurfaceLICHelper::IsSupported(vtkOpenGLRenderWindow *context)
{
  if (context == NULL)
  {
    vtkGenericWarningMacro("OpenGL render window required");
    return false;
  }

  bool lic2d = vtkLineIntegralConvolution2D::IsSupported(context);

  bool floatFormats
    = vtkTextureObject::IsSupported(context, true, true, false);

  bool support = lic2d && floatFormats;

  if (!support)
  {
    vtkGenericWarningMacro(
      << "SurfaceLIC is not supported" << endl
      << context->GetClassName() << endl
      << "LIC support = " << lic2d << endl
      << "floating point texture formats = " << floatFormats);
    return false;
  }
  return true;
}

// Description:
// Free textures and shader programs we're holding a reference to.
void vtkSurfaceLICHelper::ReleaseGraphicsResources(vtkWindow *win)
{
  if (this->ColorEnhancePass)
  {
    this->ColorEnhancePass->ReleaseGraphicsResources(win);
  }
  if (this->ColorPass)
  {
    this->ColorPass->ReleaseGraphicsResources(win);
  }
  if (this->CopyPass)
  {
    this->CopyPass->ReleaseGraphicsResources(win);
  }

  this->ClearTextures();

  this->Compositor = NULL;
  this->LICer = NULL;
  this->FBO = NULL;
}

// Description:
// Free textures we're holding a reference to.
void vtkSurfaceLICHelper::ClearTextures()
{
  this->DepthImage = NULL;
  this->GeometryImage = NULL;
  this->VectorImage = NULL;
  this->MaskVectorImage = NULL;
  this->CompositeVectorImage = NULL;
  this->CompositeMaskVectorImage = NULL;
  this->NoiseImage = NULL;
  this->LICImage = NULL;
  this->RGBColorImage = NULL;
  this->HSLColorImage = NULL;
}

// Description:
// Allocate textures.
void vtkSurfaceLICHelper::AllocateTextures(
      vtkOpenGLRenderWindow *context,
      int *viewsize)
{
  this->AllocateDepthTexture(context, viewsize, this->DepthImage);
  this->AllocateTexture(context, viewsize, this->GeometryImage, vtkTextureObject::Nearest);
  this->AllocateTexture(context, viewsize, this->VectorImage, vtkTextureObject::Linear);
  this->AllocateTexture(context, viewsize, this->MaskVectorImage, vtkTextureObject::Linear);
  this->AllocateTexture(context, viewsize, this->CompositeVectorImage, vtkTextureObject::Linear);
  this->AllocateTexture(context, viewsize, this->CompositeMaskVectorImage, vtkTextureObject::Linear);
  this->AllocateTexture(context, viewsize, this->LICImage, vtkTextureObject::Nearest);
  this->AllocateTexture(context, viewsize, this->RGBColorImage, vtkTextureObject::Nearest);
  this->AllocateTexture(context, viewsize, this->HSLColorImage, vtkTextureObject::Nearest);
}

// Description:
// Allocate a size texture, store in the given smart pointer.
void vtkSurfaceLICHelper::AllocateTexture(
      vtkOpenGLRenderWindow *context,
      int *viewsize,
      vtkSmartPointer<vtkTextureObject> &tex,
      int filter)
{
  if ( !tex )
  {
    vtkTextureObject * newTex = vtkTextureObject::New();
    newTex->SetContext(context);
    newTex->SetBaseLevel(0);
    newTex->SetMaxLevel(0);
    newTex->SetWrapS(vtkTextureObject::ClampToEdge);
    newTex->SetWrapT(vtkTextureObject::ClampToEdge);
    newTex->SetMinificationFilter(filter);
    newTex->SetMagnificationFilter(filter);
    newTex->SetBorderColor(0.0f, 0.0f, 0.0f, 0.0f);
    newTex->Create2D(viewsize[0], viewsize[1], 4, VTK_FLOAT, false);
    newTex->SetAutoParameters(0);
    tex = newTex;
    newTex->Delete();
  }
}

// Description:
// Allocate a size texture, store in the given smart pointer.
void vtkSurfaceLICHelper::AllocateDepthTexture(
      vtkOpenGLRenderWindow *context,
      int *viewsize,
      vtkSmartPointer<vtkTextureObject> &tex)
{
  if ( !tex )
  {
    vtkTextureObject * newTex = vtkTextureObject::New();
    newTex->SetContext(context);
    newTex->AllocateDepth(viewsize[0], viewsize[1], vtkTextureObject::Float32);
    newTex->SetAutoParameters(0);
    tex = newTex;
    newTex->Delete();
  }
}

// Description:
// After LIC has been computed reset/clean internal state
void vtkSurfaceLICHelper::Updated()
{
  this->ContextNeedsUpdate = false;
  this->CommunicatorNeedsUpdate = false;
}

// Description:
// Force all stages to re-execute. Necessary if the
// context or communicator changes.
void vtkSurfaceLICHelper::UpdateAll()
{
  this->ContextNeedsUpdate = true;
  this->CommunicatorNeedsUpdate= true;
}

// Description:
// Convert a viewport to a bounding box and it's texture coordinates for a
// screen size texture.
void vtkSurfaceLICHelper::ViewportQuadTextureCoords(
      const vtkPixelExtent &viewExt,
      const vtkPixelExtent &viewportExt,
      GLfloat *tcoords)
{
  GLfloat viewsize[2];
  viewExt.Size(viewsize);

  // cell to node
  vtkPixelExtent next(viewportExt);
  next.CellToNode();
  next.GetData(tcoords);

  tcoords[0] = tcoords[0]/viewsize[0];
  tcoords[1] = tcoords[1]/viewsize[0];
  tcoords[2] = tcoords[2]/viewsize[1];
  tcoords[3] = tcoords[3]/viewsize[1];
}

// Description:
// Render a quad (to trigger a shader to run)
void vtkSurfaceLICHelper::RenderQuad(
      const vtkPixelExtent &viewExt,
      const vtkPixelExtent &viewportExt,
      vtkOpenGLHelper *cbo)
{
  vtkOpenGLStaticCheckErrorMacro("failed at RenderQuad");

  // cell to node
  vtkPixelExtent next(viewportExt);
  next.CellToNode();

  GLfloat quadPts[4];
  next.GetData(quadPts);

  GLfloat quadTCoords[4];
  this->ViewportQuadTextureCoords(viewExt, viewportExt, quadTCoords);

  float tcoords[] = {
    quadTCoords[0], quadTCoords[2],
    quadTCoords[1], quadTCoords[2],
    quadTCoords[1], quadTCoords[3],
    quadTCoords[0], quadTCoords[3]};

  float verts[] = {
    quadTCoords[0]*2.0f-1.0f, quadTCoords[2]*2.0f-1.0f, 0.0f,
    quadTCoords[1]*2.0f-1.0f, quadTCoords[2]*2.0f-1.0f, 0.0f,
    quadTCoords[1]*2.0f-1.0f, quadTCoords[3]*2.0f-1.0f, 0.0f,
    quadTCoords[0]*2.0f-1.0f, quadTCoords[3]*2.0f-1.0f, 0.0f};

  vtkOpenGLRenderUtilities::RenderQuad(verts, tcoords,
    cbo->Program, cbo->VAO);
  vtkOpenGLStaticCheckErrorMacro("failed at RenderQuad");
}

// Description:
// given a axes aligned bounding box in
// normalized device coordinates test for
// view frustum visibility.
// if all points are outside one of the
// view frustum planes then this box
// is not visible. we might have false
// positive where more than one clip
// plane intersects the box.
bool vtkSurfaceLICHelper::VisibilityTest(double ndcBBox[24])
{
  // check all points in the direction d
  // at the same time.
  for (int d=0; d<3; ++d)
  {
    if (((ndcBBox[     d] < -1.0)
      && (ndcBBox[3  + d] < -1.0)
      && (ndcBBox[6  + d] < -1.0)
      && (ndcBBox[9  + d] < -1.0)
      && (ndcBBox[12 + d] < -1.0)
      && (ndcBBox[15 + d] < -1.0)
      && (ndcBBox[18 + d] < -1.0)
      && (ndcBBox[21 + d] < -1.0))
      ||((ndcBBox[     d] > 1.0)
      && (ndcBBox[3  + d] > 1.0)
      && (ndcBBox[6  + d] > 1.0)
      && (ndcBBox[9  + d] > 1.0)
      && (ndcBBox[12 + d] > 1.0)
      && (ndcBBox[15 + d] > 1.0)
      && (ndcBBox[18 + d] > 1.0)
      && (ndcBBox[21 + d] > 1.0)) )
    {
      return false;
    }
  }
  return true;
}

// Description:
// Given world space bounds,
// compute bounding boxes in clip and normalized device
// coordinates and perform view frustum visiblity test.
// return true if the bounds are visible. If so the passed
// in extent object is initialized with the corresponding
//screen space extents.
bool vtkSurfaceLICHelper::ProjectBounds(
        double PMV[16],
        int viewsize[2],
        double bounds[6],
        vtkPixelExtent &screenExt)
{
  // this is how to get the 8 corners of a bounding
  // box from the VTK bounds
  int bbIds[24] = {
        0,2,4,
        1,2,4,
        1,3,4,
        0,3,4,
        0,2,5,
        1,2,5,
        1,3,5,
        0,3,5
        };

  // normalized device coordinate bounding box
  double ndcBBox[24];
  for (int q = 0; q<8; ++q)
  {
    int qq = 3*q;
    // bounding box corner
    double wx = bounds[bbIds[qq  ]];
    double wy = bounds[bbIds[qq+1]];
    double wz = bounds[bbIds[qq+2]];
    // to clip coordinates
    ndcBBox[qq  ] = wx * PMV[idx(0,0)] + wy * PMV[idx(0,1)] + wz * PMV[idx(0,2)] + PMV[idx(0,3)];
    ndcBBox[qq+1] = wx * PMV[idx(1,0)] + wy * PMV[idx(1,1)] + wz * PMV[idx(1,2)] + PMV[idx(1,3)];
    ndcBBox[qq+2] = wx * PMV[idx(2,0)] + wy * PMV[idx(2,1)] + wz * PMV[idx(2,2)] + PMV[idx(2,3)];
    double ndcw   = wx * PMV[idx(3,0)] + wy * PMV[idx(3,1)] + wz * PMV[idx(3,2)] + PMV[idx(3,3)];

    // TODO
    // if the point is past the near clipping plane
    // we need to do something more robust. this ensures
    // the correct result but its inefficient
    if (ndcw < 0.0)
    {
      screenExt = vtkPixelExtent(viewsize[0], viewsize[1]);
      //cerr << "W<0!!!!!!!!!!!!!" << endl;
      return true;
    }

    // to normalized device coordinates
    ndcw = (ndcw == 0.0 ? 1.0 : 1.0/ndcw);
    ndcBBox[qq  ] *= ndcw;
    ndcBBox[qq+1] *= ndcw;
    ndcBBox[qq+2] *= ndcw;
  }

  // compute screen extent only if the object
  // is inside the view frustum.
  if (VisibilityTest(ndcBBox))
  {
    // these bounds are visible. compute screen
    // space exents
    double vx  = viewsize[0] - 1.0;
    double vy  = viewsize[1] - 1.0;
    double vx2 = viewsize[0] * 0.5;
    double vy2 = viewsize[1] * 0.5;
    vtkBoundingBox box;
    for (int q=0; q<8; ++q)
    {
      int qq = 3*q;
      double sx = (ndcBBox[qq  ] + 1.0) * vx2;
      double sy = (ndcBBox[qq+1] + 1.0) * vy2;
      box.AddPoint(
        vtkMath::ClampValue(sx, 0.0, vx),
        vtkMath::ClampValue(sy, 0.0, vy),
        0.0);
    }
    // to screen extent
    const double *s0 = box.GetMinPoint();
    const double *s1 = box.GetMaxPoint();
    screenExt[0] = static_cast<int>(s0[0]);
    screenExt[1] = static_cast<int>(s1[0]);
    screenExt[2] = static_cast<int>(s0[1]);
    screenExt[3] = static_cast<int>(s1[1]);
    return true;
  }

  // these bounds aren't visible
  return false;
}

// Description:
// Compute screen space extents for each block in the input
// dataset and for the entire dataset. Only visible blocks
// are used in the computations.
int vtkSurfaceLICHelper::ProjectBounds(
      vtkRenderer *ren,
      vtkActor *actor,
      vtkDataObject *dobj,
      int viewsize[2],
      vtkPixelExtent &dataExt,
      std::deque<vtkPixelExtent> &blockExts)
{
  // get the modelview projection matrix
  vtkNew<vtkMatrix4x4> tmpMatrix;

  vtkOpenGLCamera *oglCam =
    vtkOpenGLCamera::SafeDownCast(ren->GetActiveCamera());
  vtkMatrix4x4 *wcdc;
  vtkMatrix4x4 *wcvc;
  vtkMatrix3x3 *norms;
  vtkMatrix4x4 *vcdc;
  oglCam->GetKeyMatrices(ren,wcvc,norms,vcdc,wcdc);

  if (!actor->GetIsIdentity())
  {
    vtkMatrix4x4 *mcwc;
    vtkMatrix3x3 *anorms;
    ((vtkOpenGLActor *)actor)->GetKeyMatrices(mcwc,anorms);
    vtkMatrix4x4::Multiply4x4(mcwc, wcdc, tmpMatrix.GetPointer());
  }
  else
  {
    tmpMatrix->DeepCopy(wcdc);
  }
/*
  for ( int c = 0; c < 4; c ++ )
    {
    for ( int r = 0; r < 4; r ++ )
      {
      PMV[c*4+r]
        = P[idx(r,0)] * MV[idx(0,c)]
        + P[idx(r,1)] * MV[idx(1,c)]
        + P[idx(r,2)] * MV[idx(2,c)]
        + P[idx(r,3)] * MV[idx(3,c)];
      }
    }
*/
  // dataset case
  vtkDataSet* ds = dynamic_cast<vtkDataSet*>(dobj);
  if (ds && ds->GetNumberOfCells())
  {
    double bounds[6];
    ds->GetBounds(bounds);
    if ( vtkBoundingBox::IsValid(bounds)
      && this->ProjectBounds(tmpMatrix->Element[0], viewsize, bounds, dataExt) )
    {
      // the dataset is visible
      // add its extent
      blockExts.push_back(dataExt);
      return 1;
    }
    //cerr << "ds " << ds << " not visible " << endl;
    return 0;
  }
  // composite dataset case
  vtkCompositeDataSet* cd = dynamic_cast<vtkCompositeDataSet*>(dobj);
  if (cd)
  {
    // process each block's bounds
    vtkBoundingBox bbox;
    vtkCompositeDataIterator* iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      ds = dynamic_cast<vtkDataSet*>(iter->GetCurrentDataObject());
      if (ds && ds->GetNumberOfCells())
      {
        double bounds[6];
        ds->GetBounds(bounds);
        vtkPixelExtent screenExt;
        if ( vtkBoundingBox::IsValid(bounds)
          && this->ProjectBounds(tmpMatrix->Element[0], viewsize, bounds, screenExt) )
        {
          // this block is visible
          // save it's screen extent
          // and accumulate its bounds
          blockExts.push_back(screenExt);
          bbox.AddBounds(bounds);
        }
        //else { cerr << "leaf " << ds << " not visible " << endl << endl;}
      }
    }
    iter->Delete();
    // process accumulated dataset bounds
    double bounds[6];
    bbox.GetBounds(bounds);
    if ( vtkBoundingBox::IsValid(bounds)
      && this->ProjectBounds(tmpMatrix->Element[0], viewsize, bounds, dataExt) )
    {
      return 1;
    }
    return 0;
  }
  //cerr << "ds " << ds << " no cells " << endl;
  return 0;
}

// Description:
// Shrink an extent to tightly bound non-zero values
void vtkSurfaceLICHelper::GetPixelBounds(float *rgba, int ni, vtkPixelExtent &ext)
{
  vtkPixelExtent text;
  for (int j=ext[2]; j<=ext[3]; ++j)
  {
    for (int i=ext[0]; i<=ext[1]; ++i)
    {
      if (rgba[4*(j*ni+i)+3] > 0.0f)
      {
        text[0] = text[0] > i ? i : text[0];
        text[1] = text[1] < i ? i : text[1];
        text[2] = text[2] > j ? j : text[2];
        text[3] = text[3] < j ? j : text[3];
      }
    }
  }
  ext = text;
}

// Description:
// Shrink a set of extents to tightly bound non-zero values
// cull extent if it's empty
void vtkSurfaceLICHelper::GetPixelBounds(float *rgba, int ni,
  std::deque<vtkPixelExtent> &blockExts)
{
  std::vector<vtkPixelExtent> tmpExts(blockExts.begin(),blockExts.end());
  blockExts.clear();
  size_t nBlocks = tmpExts.size();
  for (size_t b=0; b<nBlocks; ++b)
  {
    vtkPixelExtent &tmpExt = tmpExts[b];
    GetPixelBounds(rgba, ni, tmpExt);
    if (!tmpExt.Empty())
    {
      blockExts.push_back(tmpExt);
    }
  }
}
