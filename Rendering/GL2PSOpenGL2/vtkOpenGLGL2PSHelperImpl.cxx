/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLGL2PSHelperImpl.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLGL2PSHelperImpl.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPath.h"
#include "vtkPointData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkTransformFeedback.h"
#include "vtkTextProperty.h"
#include "vtkTextRenderer.h"

#include "vtk_gl2ps.h"

#include <algorithm>
#include <cassert>
#include <sstream>

vtkStandardNewMacro(vtkOpenGLGL2PSHelperImpl)

//------------------------------------------------------------------------------
void vtkOpenGLGL2PSHelperImpl::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOpenGLGL2PSHelperImpl::ProcessTransformFeedback(
    vtkTransformFeedback *tfc, vtkRenderer *ren, vtkActor *act)
{
  float col[4] = { static_cast<float>(act->GetProperty()->GetColor()[0]),
                   static_cast<float>(act->GetProperty()->GetColor()[1]),
                   static_cast<float>(act->GetProperty()->GetColor()[2]),
                   static_cast<float>(act->GetProperty()->GetOpacity()) };
  this->ProcessTransformFeedback(tfc, ren, col);
}

//------------------------------------------------------------------------------
void vtkOpenGLGL2PSHelperImpl::ProcessTransformFeedback(
    vtkTransformFeedback *tfc, vtkRenderer *ren, unsigned char col[4])
{
  float colf[4] = { col[0] / 255.f,
                    col[1] / 255.f,
                    col[2] / 255.f,
                    col[3] / 255.f };
  this->ProcessTransformFeedback(tfc, ren, colf);
}

//------------------------------------------------------------------------------
void vtkOpenGLGL2PSHelperImpl::ProcessTransformFeedback(
    vtkTransformFeedback *tfc, vtkRenderer *ren, float col[4])
{
  if (tfc->GetNumberOfVertices() == 0)
  {
    // Nothing to do.
    return;
  }

  // Captured data:
  typedef std::vector<vtkTransformFeedback::VaryingMetaData> VarVector;
  const VarVector &vars = tfc->GetVaryings();
  unsigned char *data = static_cast<unsigned char *>(tfc->GetBufferData());
  unsigned char *dataEnd = data + tfc->GetBufferSize();
  size_t vertexSize = tfc->GetBytesPerVertex();
  (void)vertexSize; // Only used in asserts
  assert("Sanity" &&
         vertexSize == (dataEnd - data) / tfc->GetNumberOfVertices());
  int primitiveMode = tfc->GetPrimitiveMode();

  float pointSize = this->PointSize * this->PointSizeFactor;
  float lineWidth = this->LineWidth * this->LineWidthFactor;

  if (!data)
  {
    vtkErrorMacro("TransformFeedback buffer is NULL.");
    return;
  }

  // Info to transform clip --> display coords
  double renVp[4];
  ren->GetViewport(renVp);
  int *winSize = ren->GetRenderWindow()->GetSize();
  int vp[4] = { static_cast<int>(renVp[0] * winSize[0]),
                static_cast<int>(renVp[1] * winSize[1]),
                static_cast<int>(renVp[2] * winSize[0]),
                static_cast<int>(renVp[3] * winSize[1]) };
  float halfW = (vp[2] - vp[0]) * 0.5f;
  float halfH = (vp[3] - vp[1]) * 0.5f;

  // We handle lines, verts, and triangles, so allocate three verts:
  GL2PSvertex verts[3];
  size_t curVert = 0;

  // Process all vertices:
  while (data < dataEnd)
  {
    assert("Sanity" && data + vertexSize <= dataEnd);
    bool posSet = false;
    bool colorSet = false;
    GL2PSvertex &vert = verts[curVert];

    // Process all roles for this vertex:
    for (VarVector::const_iterator it = vars.begin(), itEnd = vars.end();
         it != itEnd; ++it)
    {
      size_t varSize = tfc->GetBytesPerVertex(it->Role);

      switch (it->Role)
      {
        case vtkTransformFeedback::Vertex_ClipCoordinate_F:
        {
          posSet = true;
          float *fdata = reinterpret_cast<float*>(data);

          // Clip --> NDC
          float invW = 1.0f / fdata[3];
          vert.xyz[0] = fdata[0] / invW;
          vert.xyz[1] = fdata[1] / invW;
          vert.xyz[2] = fdata[2] / invW;

          // NDC --> DC
          vert.xyz[0] = halfW * vert.xyz[0] + (vp[0] + halfW);
          vert.xyz[1] = halfH * vert.xyz[1] + (vp[1] + halfH);

          break;
        }

        case vtkTransformFeedback::Color_RGBA_F:
        {
          float *fdata = reinterpret_cast<float*>(data);
          std::copy(fdata, fdata + 4, vert.rgba);
          colorSet = true;
          break;
        }

        default:
          vtkWarningMacro("Unhandled data role: " << it->Role);
      }

      // Move to next var / vertex:
      assert("In bounds" && data + varSize <= dataEnd);
      data += varSize;
    }

    // Sanity check:
    if (!posSet)
    {
      std::fill(vert.xyz, vert.xyz + 3, 0.f);
      vtkErrorMacro("Position info missing from capture.");
    }

    // Set color from actor if needed:
    if (!colorSet)
    {
      std::copy(col, col + 4, vert.rgba);
    }

    // Emit primitive / move to next vertex
    switch (primitiveMode)
    {
      case GL_POINTS:
        gl2psAddPolyPrimitive(GL2PS_POINT, 1, verts, 0, 0.f, 0.f, 0xffff, 1,
                              pointSize, 0);
        break;

      case GL_LINES:
        curVert = (curVert + 1) % 2;
        if (curVert == 0)
        {
          gl2psAddPolyPrimitive(GL2PS_LINE, 2, verts, 0, 0.f, 0.f,
                                this->LineStipple, 1, lineWidth, 0);
        }
        break;

      case GL_TRIANGLES:
        curVert = (curVert + 1) % 3;
        if (curVert == 0)
        {
          gl2psAddPolyPrimitive(GL2PS_TRIANGLE, 3, verts, 0, 0.f, 0.f, 0xffff,
                                1, 1, 0);
        }
        break;

      default:
        vtkWarningMacro("Unhandled primitive mode: " << primitiveMode);
        break;
    }
  }

  assert("In bounds." && data == dataEnd);
}

//------------------------------------------------------------------------------
void vtkOpenGLGL2PSHelperImpl::DrawString(const std::string &str,
                                          vtkTextProperty *tprop,
                                          double pos[3],
                                          double backgroundDepth,
                                          vtkRenderer *ren)
{
  if (str.empty())
  {
    return;
  }

  vtkTextRenderer *tren(vtkTextRenderer::GetInstance());
  if (tren == NULL)
  {
    vtkErrorMacro("vtkTextRenderer unavailable.");
    return;
  }

  int dpi = this->RenderWindow->GetDPI();

  // Draw the background if needed:
  if (tprop->GetBackgroundOpacity() > 0.)
  {
    vtkTextRenderer::Metrics metrics;
    if (tren->GetMetrics(tprop, str, metrics, dpi))
    {
      double bgPos[3] = { pos[0], pos[1], backgroundDepth };

      GL2PSvertex bgVerts[5];

      double bgColor[4];
      tprop->GetBackgroundColor(bgColor);
      bgColor[3] = tprop->GetBackgroundOpacity();
      std::copy(bgColor, bgColor + 4, bgVerts[0].rgba);
      std::copy(bgColor, bgColor + 4, bgVerts[1].rgba);
      std::copy(bgColor, bgColor + 4, bgVerts[2].rgba);
      std::copy(bgColor, bgColor + 4, bgVerts[3].rgba);
      std::copy(bgColor, bgColor + 4, bgVerts[4].rgba);

      bgVerts[0].xyz[0] = static_cast<float>(bgPos[0] + metrics.TopLeft[0]);
      bgVerts[0].xyz[1] = static_cast<float>(bgPos[1] + metrics.TopLeft[1]);
      bgVerts[0].xyz[2] = static_cast<float>(bgPos[2]);
      bgVerts[1].xyz[0] = static_cast<float>(bgPos[0] + metrics.BottomLeft[0]);
      bgVerts[1].xyz[1] = static_cast<float>(bgPos[1] + metrics.BottomLeft[1]);
      bgVerts[1].xyz[2] = static_cast<float>(bgPos[2]);
      bgVerts[2].xyz[0] = static_cast<float>(bgPos[0] + metrics.BottomRight[0]);
      bgVerts[2].xyz[1] = static_cast<float>(bgPos[1] + metrics.BottomRight[1]);
      bgVerts[2].xyz[2] = static_cast<float>(bgPos[2]);
      bgVerts[3].xyz[0] = static_cast<float>(bgPos[0] + metrics.TopRight[0]);
      bgVerts[3].xyz[1] = static_cast<float>(bgPos[1] + metrics.TopRight[1]);
      bgVerts[3].xyz[2] = static_cast<float>(bgPos[2]);
      bgVerts[4].xyz[0] = bgVerts[0].xyz[0];
      bgVerts[4].xyz[1] = bgVerts[0].xyz[1];
      bgVerts[4].xyz[2] = bgVerts[0].xyz[2];

      gl2psAddPolyPrimitive(GL2PS_TRIANGLE, 3, bgVerts,     0, 0, 0, 0xffff, 0,
                            0, 0);
      gl2psAddPolyPrimitive(GL2PS_TRIANGLE, 3, bgVerts + 2, 0, 0, 0, 0xffff, 0,
                            0, 0);
    }
  }

  // Is this mathtext?
  bool isMath = tren->DetectBackend(str) == vtkTextRenderer::MathText;

  // Export text as either a path or a text object.
  if (!isMath && !this->TextAsPath)
  {
    const char *fontname = this->TextPropertyToPSFontName(tprop);

    GLint align = static_cast<GLint>(this->TextPropertyToGL2PSAlignment(tprop));

    GLfloat angle = static_cast<GLfloat>(tprop->GetOrientation());

    // GL2PS assumes 72 DPI, so we'll have to adjust the font size:
    GLint fontSize = static_cast<GLint>(tprop->GetFontSize() * (dpi / 72.));

    GL2PSrgba rgba;
    double rgbad[3];
    tprop->GetColor(rgbad[0], rgbad[1], rgbad[2]);
    rgba[0] = static_cast<GLfloat>(rgbad[0]);
    rgba[1] = static_cast<GLfloat>(rgbad[1]);
    rgba[2] = static_cast<GLfloat>(rgbad[2]);
    rgba[3] = tprop->GetOpacity();

    GL2PSvertex gl2psRasterPos;
    gl2psRasterPos.xyz[0] = static_cast<float>(pos[0]);
    gl2psRasterPos.xyz[1] = static_cast<float>(pos[1]);
    gl2psRasterPos.xyz[2] = static_cast<float>(pos[2]);
    gl2psRasterPos.rgba[0] = 0.f;
    gl2psRasterPos.rgba[1] = 0.f;
    gl2psRasterPos.rgba[2] = 0.f;
    gl2psRasterPos.rgba[3] = 0.f;
    gl2psForceRasterPos(&gl2psRasterPos);
    gl2psTextOptColor(str.c_str(), fontname, fontSize, align, angle, rgba);
  }
  else
  {
    // Render the string to a path and then draw it to GL2PS:
    vtkNew<vtkPath> path;
    tren->StringToPath(tprop, str, path.GetPointer(), dpi);
    // Get color
    double rgbd[3];
    tprop->GetColor(rgbd[0], rgbd[1], rgbd[2]);
    unsigned char rgba[4] = {
      static_cast<unsigned char>(rgbd[0]*255),
      static_cast<unsigned char>(rgbd[1]*255),
      static_cast<unsigned char>(rgbd[2]*255),
      static_cast<unsigned char>(tprop->GetOpacity()*255)};

    double devicePos[3] = {pos[0], pos[1], pos[2]};
    this->ProjectPoint(devicePos, ren);

    this->DrawPath(path.GetPointer(), pos, devicePos, rgba, NULL, 0.0, -1.f,
                   (std::string("Pathified string: ") + str).c_str());
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLGL2PSHelperImpl::DrawPath(vtkPath *path, double rasterPos[3],
                                        double windowPos[2],
                                        unsigned char rgba[4], double scale[2],
                                        double rotateAngle, float strokeWidth,
                                        const char *label)
{
  // Replace newlines in label -- these will throw off the comments.
  std::string l(label ? label : "");
  size_t idx = 0;
  while ((idx = l.find('\n', idx)) != std::string::npos)
  {
    l.replace(idx, 1, "\\n");
  }

  switch (gl2psGetFileFormat())
  {
    case GL2PS_PS:
    case GL2PS_EPS:
      this->DrawPathPS(path, rasterPos, windowPos, rgba, scale,
                       rotateAngle, strokeWidth, l);
      break;
    case GL2PS_SVG:
      this->DrawPathSVG(path, rasterPos, windowPos, rgba, scale,
                        rotateAngle, strokeWidth, l);
      break;
    case GL2PS_PDF:
      this->DrawPathPDF(path, rasterPos, windowPos, rgba, scale,
                        rotateAngle, strokeWidth, l);
      break;
    default:
      break;
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLGL2PSHelperImpl::Draw3DPath(
    vtkPath *path, vtkMatrix4x4 *actorMatrix, double rasterPos[3],
    unsigned char actorColor[4], vtkRenderer *ren, const char *label)
{
  double translation[2] = {0.0, 0.0};
  vtkNew<vtkPath> projPath;
  projPath->DeepCopy(path);
  this->ProjectPoints(projPath->GetPoints(), ren, actorMatrix);
  this->DrawPath(projPath.GetPointer(), rasterPos, translation,
                 actorColor, NULL, 0.0, -1.f, label);
}

//------------------------------------------------------------------------------
void vtkOpenGLGL2PSHelperImpl::DrawImage(vtkImageData *input, double pos[3])
{
  // Must be RGB/RGBA:
  GLenum format = 0;
  switch (input->GetNumberOfScalarComponents())
  {
    case 3:
      format = GL_RGB;
      break;
    case 4:
      format = GL_RGBA;
      break;
    default:
      vtkErrorMacro("Invalid image format: Must be RGB or RGBA.");
      return;
  }

  if (input->GetDimensions()[2] != 1)
  {
    vtkErrorMacro("Invalid image format: 3D ImageData are not supported.");
    return;
  }

  vtkDataArray *inScalars = input->GetPointData()->GetScalars();
  if (!inScalars || inScalars->GetNumberOfTuples() == 0)
  {
    return;
  }

  if (!vtkDataTypesCompare(inScalars->GetDataType(), VTK_FLOAT))
  {
    vtkErrorMacro("Invalid image format: Expected float scalars.");
    return;
  }

  GL2PSvertex gl2psRasterPos;
  gl2psRasterPos.xyz[0] = static_cast<float>(pos[0]);
  gl2psRasterPos.xyz[1] = static_cast<float>(pos[1]);
  gl2psRasterPos.xyz[2] = static_cast<float>(pos[2]);
  gl2psRasterPos.rgba[0] = 0.f;
  gl2psRasterPos.rgba[1] = 0.f;
  gl2psRasterPos.rgba[2] = 0.f;
  gl2psRasterPos.rgba[3] = 0.f;

  int dims[3];
  input->GetDimensions(dims);

  gl2psForceRasterPos(&gl2psRasterPos);
  gl2psDrawPixels(dims[0], dims[1], 0, 0, format, GL_FLOAT,
                  inScalars->GetVoidPointer(0));
}

//------------------------------------------------------------------------------
vtkOpenGLGL2PSHelperImpl::vtkOpenGLGL2PSHelperImpl()
{
}

//------------------------------------------------------------------------------
vtkOpenGLGL2PSHelperImpl::~vtkOpenGLGL2PSHelperImpl()
{
}

//------------------------------------------------------------------------------
const char *
vtkOpenGLGL2PSHelperImpl::TextPropertyToPSFontName(vtkTextProperty *tprop)
{
  bool bold = tprop->GetBold() != 0;
  bool italic = tprop->GetItalic() != 0;

  switch (tprop->GetFontFamily())
  {
    case VTK_ARIAL:
    {
      if (!bold && !italic)
      {
        return "Helvetica";
      }
      else if (bold && italic)
      {
        return "Helvetica-BoldItalic";
      }
      else if (bold)
      {
        return "Helvetica-Bold";
      }
      else // (italic)
      {
        return "Helvetica-Italic";
      }
    }
    case VTK_TIMES:
    {
      if (!bold && !italic)
      {
        return "Times-Roman";
      }
      else if (bold && italic)
      {
        return "Times-BoldOblique";
      }
      else if (bold)
      {
        return "Times-Bold";
      }
      else // (italic)
      {
        return "Times-Oblique";
      }
    }
    case VTK_COURIER:
    {
      if (!bold && !italic)
      {
        return "Courier";
      }
      else if (bold && italic)
      {
        return "Courier-BoldOblique";
      }
      else if (bold)
      {
        return "Courier-Bold";
      }
      else // (italic)
      {
        return "Courier-Oblique";
      }
    }
    case VTK_UNKNOWN_FONT:
    default:
      break;
  }

  return "Helvetica";
}

//------------------------------------------------------------------------------
int
vtkOpenGLGL2PSHelperImpl::TextPropertyToGL2PSAlignment(vtkTextProperty *tprop)
{
  switch (tprop->GetJustification())
  {
    case VTK_TEXT_LEFT:
      switch (tprop->GetVerticalJustification())
      {
        case VTK_TEXT_TOP:
        {
          return GL2PS_TEXT_TL;
        }
        case VTK_TEXT_CENTERED:
        {
          return GL2PS_TEXT_CL;
        }
        case VTK_TEXT_BOTTOM:
        {
          return GL2PS_TEXT_BL;
        }
      }
      break;
    case VTK_TEXT_CENTERED:
      switch (tprop->GetVerticalJustification())
      {
        case VTK_TEXT_TOP:
        {
          return GL2PS_TEXT_T;
        }
        case VTK_TEXT_CENTERED:
        {
          return GL2PS_TEXT_C;
        }
        case VTK_TEXT_BOTTOM:
        {
          return GL2PS_TEXT_B;
        }
      }
      break;
    case VTK_TEXT_RIGHT:
      switch (tprop->GetVerticalJustification())
      {
        case VTK_TEXT_TOP:
        {
          return GL2PS_TEXT_TR;
        }
        case VTK_TEXT_CENTERED:
        {
          return GL2PS_TEXT_CR;
        }
        case VTK_TEXT_BOTTOM:
        {
          return GL2PS_TEXT_BR;
        }
      }
    default:
      break;
  }

  return GL2PS_TEXT_BL;
}

//------------------------------------------------------------------------------
void vtkOpenGLGL2PSHelperImpl::GetTransformParameters(
    vtkRenderer *ren, vtkMatrix4x4 *actorMatrix, vtkMatrix4x4 *xform,
    double vpOrigin[2], double halfSize[2], double zFact[2])
{
  // figure out the same aspect ratio used by the render engine
  // (see vtkOpenGLCamera::Render())
  int  lowerLeft[2];
  int usize, vsize;
  double aspect1[2];
  double aspect2[2];
  ren->GetTiledSizeAndOrigin(&usize, &vsize, lowerLeft, lowerLeft+1);
  ren->ComputeAspect();
  ren->GetAspect(aspect1);
  ren->vtkViewport::ComputeAspect();
  ren->vtkViewport::GetAspect(aspect2);
  double aspectModification = (aspect1[0] * aspect2[1]) /
                              (aspect1[1] * aspect2[0]);
  double aspect = aspectModification * usize / vsize;

  vtkCamera *cam = ren->GetActiveCamera();
  xform->DeepCopy(cam->GetCompositeProjectionTransformMatrix(aspect, -1, 1));

  if (actorMatrix)
  {
    vtkMatrix4x4::Multiply4x4(xform, actorMatrix, xform);
  }

  vpOrigin[0] = static_cast<double>(lowerLeft[0]);
  vpOrigin[1] = static_cast<double>(lowerLeft[1]);
  halfSize[0] = static_cast<double>(usize) * 0.5;
  halfSize[1] = static_cast<double>(vsize) * 0.5;

  double depthRange[2];
  glGetDoublev(GL_DEPTH_RANGE, depthRange);
  zFact[0] = (depthRange[1] - depthRange[0]) * 0.5;
  zFact[1] = (depthRange[1] + depthRange[0]) * 0.5;
}

//------------------------------------------------------------------------------
inline void vtkOpenGLGL2PSHelperImpl::ProjectPoint(double point[3],
                                                   vtkRenderer *ren,
                                                   vtkMatrix4x4 *actorMatrix)
{
  vtkNew<vtkMatrix4x4> xform;
  double vpOrigin[2];
  double halfSize[2];
  double zFact[2];
  vtkOpenGLGL2PSHelperImpl::GetTransformParameters(
        ren, actorMatrix, xform.GetPointer(), vpOrigin, halfSize, zFact);

  double tmp[4] = { point[0], point[1], point[2], 1. };
  vtkOpenGLGL2PSHelperImpl::ProjectPoint(
        tmp, xform.GetPointer(), vpOrigin, halfSize[0], halfSize[1],
        zFact[0], zFact[1]);
}

//------------------------------------------------------------------------------
inline void vtkOpenGLGL2PSHelperImpl::ProjectPoint(
    double point[4], vtkMatrix4x4 *transformMatrix, double viewportOrigin[2],
    double halfWidth, double halfHeight, double zfact1, double zfact2)
{
  // Convert world to clip coordinates:
  // <out point> = [projection] [modelview] [actor matrix] <in point>
  transformMatrix->MultiplyPoint(point, point);
  // Clip to NDC
  const double invW = 1.0 / point[3];
  point[0] *= invW;
  point[1] *= invW;
  point[2] *= invW;
  // NDC to device:
  point[0] = point[0] * halfWidth + viewportOrigin[0] + halfWidth;
  point[1] = point[1] * halfHeight + viewportOrigin[1] + halfHeight;
  point[2] = point[2] * zfact1 + zfact2;
}

//------------------------------------------------------------------------------
inline void vtkOpenGLGL2PSHelperImpl::ProjectPoints(vtkPoints *points,
                                                    vtkRenderer *ren,
                                                    vtkMatrix4x4 *actorMatrix)
{
  vtkNew<vtkMatrix4x4> xform;
  double vpOrigin[2];
  double halfSize[2];
  double zFact[2];
  vtkOpenGLGL2PSHelperImpl::GetTransformParameters(
        ren, actorMatrix, xform.GetPointer(), vpOrigin, halfSize, zFact);

  double point[4];
  for (vtkIdType i = 0; i < points->GetNumberOfPoints(); ++i)
  {
    points->GetPoint(i, point);
    point[3] = 1.0;
    vtkOpenGLGL2PSHelperImpl::ProjectPoint(
          point, xform.GetPointer(), vpOrigin, halfSize[0], halfSize[1],
          zFact[0], zFact[1]);
    points->SetPoint(i, point);
  }
}

//------------------------------------------------------------------------------
inline void vtkOpenGLGL2PSHelperImpl::UnprojectPoint(
    double point[4], vtkMatrix4x4 *invTransformMatrix, double viewportOrigin[2],
    double halfWidth, double halfHeight, double zfact1, double zfact2)
{
  point[0] = (point[0] - viewportOrigin[0] - halfWidth) / halfWidth;
  point[1] = (point[1] - viewportOrigin[1] - halfHeight) / halfHeight;
  point[2] = (point[2] - zfact2) / zfact1;

  point[0] *= point[3];
  point[1] *= point[3];
  point[2] *= point[3];

  invTransformMatrix->MultiplyPoint(point, point);
}

//------------------------------------------------------------------------------
inline void vtkOpenGLGL2PSHelperImpl::UnprojectPoints(
    double *points3D, vtkIdType numPoints, vtkRenderer *ren,
    vtkMatrix4x4 *actorMatrix)
{
  vtkNew<vtkMatrix4x4> xform;
  double vpOrigin[2];
  double halfSize[2];
  double zFact[2];
  vtkOpenGLGL2PSHelperImpl::GetTransformParameters(
        ren, actorMatrix, xform.GetPointer(), vpOrigin, halfSize, zFact);

  xform->Invert();


  double point[4];
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    std::copy(points3D + (i * 3), points3D + ((i + 1) * 3), point);
    point[3] = 1.0;
    vtkOpenGLGL2PSHelperImpl::UnprojectPoint(
          point, xform.GetPointer(), vpOrigin, halfSize[0], halfSize[1],
          zFact[0], zFact[1]);
    std::copy(point, point + 3, points3D + (i * 3));
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLGL2PSHelperImpl::DrawPathPS(
    vtkPath *path, double rasterPos[3], double windowPos[2],
    unsigned char rgba[4], double scale[2], double rotateAngle,
    float strokeWidth, const std::string &label)
{
  vtkFloatArray *points =
      vtkArrayDownCast<vtkFloatArray>(path->GetPoints()->GetData());
  vtkIntArray *codes = path->GetCodes();

  if (points->GetNumberOfTuples() != codes->GetNumberOfTuples())
  {
    return;
  }

  std::stringstream out;
  out.setf(std::ios_base::left | std::ios_base::fixed);
  const int floatPrec = 2;
  const int floatWidth = 6;
  out.precision(floatPrec);
  out.width(floatWidth);

  float curveto[3][2];
  float curX = 0;
  float curY = 0;

  float *pt = points->GetPointer(0);
  int *code = codes->GetPointer(0);

  // These are only used in an assertion, ifdef silences warning on non-debug
  // builds
#ifndef NDEBUG
  float *ptBegin = pt;
  int *codeBegin = code;
#endif
  int *codeEnd = code + codes->GetNumberOfTuples();
  if (!label.empty())
  {
    out << "% " << label << endl;
  }
  out << "gsave" << endl;
  out << "initmatrix" << endl;
  out << windowPos[0] << " " << windowPos[1] << " translate" << endl;
  if (scale)
  {
    out << scale[0] << " " << scale[1] << " scale" << endl;
  }
  out << rotateAngle << " rotate" << endl;
  out << "newpath" << endl;
  while (code < codeEnd)
  {
    assert(pt - ptBegin == (code - codeBegin) * 3);

    switch (static_cast<vtkPath::ControlPointType>(*code))
    {
      case vtkPath::MOVE_TO:
        curX = *pt;
        curY = *(++pt);
        ++pt; // Flush z
        out << curX << " " << curY << " moveto\n";
        break;
      case vtkPath::LINE_TO:
        curX = *pt;
        curY = *(++pt);
        ++pt; // Flush z
        out << curX << " " << curY << " lineto\n";
        break;
      case vtkPath::CONIC_CURVE:
        // Postscript doesn't support conic curves -- elevate order to cubic:
      {
        // Next point should be a CONIC_CURVE as well
        code += 1;
        const float &conicX = *pt;
        const float &conicY = *(++pt);
        ++pt; // Flush z
        const float &nextX = *(++pt);
        const float &nextY = *(++pt);
        ++pt; // Flush z

        // Perform degree elevation:
        curveto[0][0] = (1.0/3.0)*curX   + (2.0/3.0)*conicX;
        curveto[0][1] = (1.0/3.0)*curY   + (2.0/3.0)*conicY;
        curveto[1][0] = (2.0/3.0)*conicX + (1.0/3.0)*nextX;
        curveto[1][1] = (2.0/3.0)*conicY + (1.0/3.0)*nextY;
        curveto[2][0] = curX = nextX;
        curveto[2][1] = curY = nextY;
        out << curveto[0][0] << " " << curveto[0][1] << endl
            << curveto[1][0] << " " << curveto[1][1] << endl
            << curveto[2][0] << " " << curveto[2][1] << " curveto" << endl;
      }
        break;
      case vtkPath::CUBIC_CURVE:
      {
        // Next two points should be CUBIC_CURVEs as well
        code += 2;

        curveto[0][0] = *pt;
        curveto[0][1] = *(++pt);
        ++pt;
        curveto[1][0] = *(++pt);
        curveto[1][1] = *(++pt);
        ++pt;
        curveto[2][0] = curX = *(++pt);
        curveto[2][1] = curY = *(++pt);
        ++pt;
        out << curveto[0][0] << " " << curveto[0][1] << endl
            << curveto[1][0] << " " << curveto[1][1] << endl
            << curveto[2][0] << " " << curveto[2][1] << " curveto" << endl;
      }
        break;
      default:
        out << "% Unrecognized control code: " << *code << endl;
        pt +=2;
        break;
    }

    ++code;
    ++pt;
  }

  out << static_cast<float>(rgba[0])/255.f << " " <<
         static_cast<float>(rgba[1])/255.f << " " <<
         static_cast<float>(rgba[2])/255.f << " setrgbcolor" << endl;

  if (strokeWidth > 1e-5)
  {
    out << strokeWidth << " setlinewidth\nstroke" << endl;
  }
  else
  {
    out << "fill" << endl;
  }
  out << "grestore" << endl;

  GL2PSvertex gl2psRasterPos;
  gl2psRasterPos.xyz[0] = static_cast<float>(rasterPos[0]);
  gl2psRasterPos.xyz[1] = static_cast<float>(rasterPos[1]);
  gl2psRasterPos.xyz[2] = static_cast<float>(rasterPos[2]);
  gl2psRasterPos.rgba[0] = 0.f;
  gl2psRasterPos.rgba[1] = 0.f;
  gl2psRasterPos.rgba[2] = 0.f;
  gl2psRasterPos.rgba[3] = 0.f;
  gl2psForceRasterPos(&gl2psRasterPos);
  gl2psSpecial(gl2psGetFileFormat(), out.str().c_str());
}

//------------------------------------------------------------------------------
void vtkOpenGLGL2PSHelperImpl::DrawPathPDF(
    vtkPath *path, double rasterPos[3], double windowPos[2],
    unsigned char rgba[4], double scale[2], double rotateAngle,
    float strokeWidth, const std::string &/*label*/)
{
  vtkFloatArray *points =
      vtkArrayDownCast<vtkFloatArray>(path->GetPoints()->GetData());
  vtkIntArray *codes = path->GetCodes();

  if (points->GetNumberOfTuples() != codes->GetNumberOfTuples())
  {
    return;
  }

  std::stringstream out;
  out.setf(std::ios_base::left | std::ios_base::fixed);
  const int floatPrec = 2;
  const int floatWidth = 6;
  out.precision(floatPrec);
  out.width(floatWidth);

  float curveto[3][2];
  float curX = 0;
  float curY = 0;

  float *pt = points->GetPointer(0);
  int *code = codes->GetPointer(0);

  // These are only used in an assertion, ifdef silences warning on non-debug
  // builds
#ifndef NDEBUG
  float *ptBegin = pt;
  int *codeBegin = code;
#endif
  int *codeEnd = code + codes->GetNumberOfTuples();

  // Push state. PDF doesn't let you reset the CTM, so the hope is that it is
  // identity when this block starts...
  out << "q" << endl;
  // color
  out << static_cast<float>(rgba[0])/255.f << " " <<
         static_cast<float>(rgba[1])/255.f << " " <<
         static_cast<float>(rgba[2])/255.f <<
         (strokeWidth > 1e-5 ? " RG" : " rg") << endl;
  // opacity
  out << static_cast<float>(rgba[3])/255.f
      << (strokeWidth > 1e-5 ? " CA" : " ca") << endl;
  // translate
  out << 1.f << " " << 0.f << " " << 0.f << " " << 1.f << " "
      << windowPos[0] << " " << windowPos[1] << " cm" << endl;
  // rotate
  float sT = sin(vtkMath::RadiansFromDegrees(rotateAngle));
  float cT = cos(vtkMath::RadiansFromDegrees(rotateAngle));
  out << cT << " " << sT << " " << -sT << " " << cT << " " << 0.f << " " << 0.f
      << " cm" << endl;
  // scale
  if (scale)
  {
    out << scale[0] << " " << 0.f << " " << 0.f << " " << scale[1] << " " << 0.f
        << " " << 0.f << " cm" << endl;
  }

  while (code < codeEnd)
  {
    assert(pt - ptBegin == (code - codeBegin) * 3);

    switch (static_cast<vtkPath::ControlPointType>(*code))
    {
      case vtkPath::MOVE_TO:
        curX = *pt;
        curY = *(++pt);
        ++pt; // Flush z
        out << curX << " " << curY << " m\n";
        break;
      case vtkPath::LINE_TO:
        curX = *pt;
        curY = *(++pt);
        ++pt; // Flush z
        out << curX << " " << curY << " l\n";
        break;
      case vtkPath::CONIC_CURVE:
        // Postscript doesn't support conic curves -- elevate order to cubic:
      {
        // Next point should be a CONIC_CURVE as well
        code += 1;
        const float &conicX = *pt;
        const float &conicY = *(++pt);
        ++pt; // Flush z
        const float &nextX = *(++pt);
        const float &nextY = *(++pt);
        ++pt; // Flush z

        // Perform degree elevation:
        curveto[0][0] = (1.0/3.0)*curX   + (2.0/3.0)*conicX;
        curveto[0][1] = (1.0/3.0)*curY   + (2.0/3.0)*conicY;
        curveto[1][0] = (2.0/3.0)*conicX + (1.0/3.0)*nextX;
        curveto[1][1] = (2.0/3.0)*conicY + (1.0/3.0)*nextY;
        curveto[2][0] = curX = nextX;
        curveto[2][1] = curY = nextY;
        out << curveto[0][0] << " " << curveto[0][1] << endl
            << curveto[1][0] << " " << curveto[1][1] << endl
            << curveto[2][0] << " " << curveto[2][1] << " c" << endl;
      }
        break;
      case vtkPath::CUBIC_CURVE:
      {
        // Next two points should be CUBIC_CURVEs as well
        code += 2;

        curveto[0][0] = *pt;
        curveto[0][1] = *(++pt);
        ++pt;
        curveto[1][0] = *(++pt);
        curveto[1][1] = *(++pt);
        ++pt;
        curveto[2][0] = curX = *(++pt);
        curveto[2][1] = curY = *(++pt);
        ++pt;
        out << curveto[0][0] << " " << curveto[0][1] << endl
            << curveto[1][0] << " " << curveto[1][1] << endl
            << curveto[2][0] << " " << curveto[2][1] << " c" << endl;
      }
        break;
      default:
        out << "% Unrecognized control code: " << *code << endl;
        pt +=2;
        break;
    }

    ++code;
    ++pt;
  }

  out << "h ";
  if (strokeWidth > 1e-5)
  {
    out << strokeWidth << " w S" << endl;
  }
  else
  {
    out<< "f" << endl;
  }
  out << "Q" << endl; // Pop state

  GL2PSvertex gl2psRasterPos;
  gl2psRasterPos.xyz[0] = static_cast<float>(rasterPos[0]);
  gl2psRasterPos.xyz[1] = static_cast<float>(rasterPos[1]);
  gl2psRasterPos.xyz[2] = static_cast<float>(rasterPos[2]);
  gl2psRasterPos.rgba[0] = 0.f;
  gl2psRasterPos.rgba[1] = 0.f;
  gl2psRasterPos.rgba[2] = 0.f;
  gl2psRasterPos.rgba[3] = 0.f;
  gl2psForceRasterPos(&gl2psRasterPos);
  gl2psSpecial(gl2psGetFileFormat(), out.str().c_str());
}

//------------------------------------------------------------------------------
void vtkOpenGLGL2PSHelperImpl::DrawPathSVG(
    vtkPath *path, double rasterPos[3], double windowPos[2],
    unsigned char rgba[4], double scale[2], double rotateAngle,
    float strokeWidth, const std::string &label)
{
  vtkFloatArray *points =
      vtkArrayDownCast<vtkFloatArray>(path->GetPoints()->GetData());
  vtkIntArray *codes = path->GetCodes();

  if (points->GetNumberOfTuples() != codes->GetNumberOfTuples())
  {
    return;
  }

  std::stringstream out;
  out.setf(std::ios_base::left | std::ios_base::fixed);

  const int floatPrec = 2;
  const int floatWidth = 6;
  out.precision(floatPrec);
  out.width(floatWidth);

  float curveto[3][2];
  float curX = 0;
  float curY = 0;

  float *pt = points->GetPointer(0);
  int *code = codes->GetPointer(0);

  double windowHeight = static_cast<double>(this->RenderWindow->GetSize()[1]);

  // These are only used in an assertion, ifdef silences warning on non-debug
  // builds
#ifndef NDEBUG
  float *ptBegin = pt;
  int *codeBegin = code;
#endif

  if (!label.empty())
  {
    out << "<!-- " << label << " -->" << endl;
  }

  int *codeEnd = code + codes->GetNumberOfTuples();
  out << "<g transform=\"" << endl
      << "     translate(" << windowPos[0] << " "
      << windowHeight - windowPos[1] << ")" << endl;
  if (scale)
  {
    out << "     scale(" << scale[0] << " " << -scale[1] << ")" << endl;
  }
  else
  {
    out << "     scale(1.0 -1.0)" << endl;
  }
  out << "     rotate(" << rotateAngle << ")\"" << endl;
  if (strokeWidth > 1e-5)
  {
    out << "   fill=\"none\"" << endl
        << "   stroke-width=\"" << strokeWidth << "\"" << endl
        << "   stroke=\"rgb(" << std::setprecision(0)
        << static_cast<int>(rgba[0]) << ","
        << static_cast<int>(rgba[1]) << ","
        << static_cast<int>(rgba[2])
        << std::setprecision(floatPrec) << ")\"" << endl;
  }
  else
  {
    out << "   stroke=\"none\"" << endl
        << "   fill=\"rgb(" << std::setprecision(0)
             << static_cast<int>(rgba[0]) << ","
             << static_cast<int>(rgba[1]) << ","
             << static_cast<int>(rgba[2])
             << std::setprecision(floatPrec) << ")\"" << endl;
  }
    out << "   opacity=\"" << static_cast<float>(rgba[3])/255.f << "\"\n"
      << ">" << endl
      << "  <path d=\"" << std::right << endl;

  while (code < codeEnd)
  {
    assert(pt - ptBegin == (code - codeBegin) * 3);

    switch (static_cast<vtkPath::ControlPointType>(*code))
    {
      case vtkPath::MOVE_TO:
        curX = *pt;
        curY = *(++pt);
        ++pt; // Flush z
        out << "    M " << curX << " " << curY << endl;
        break;
      case vtkPath::LINE_TO:
        curX = *pt;
        curY = *(++pt);
        ++pt; // Flush z
        out << "    L " << curX << " " << curY << endl;
        break;
      case vtkPath::CONIC_CURVE:
      {
        // Next point should be a CONIC_CURVE as well
        code += 1;

        // Perform degree elevation:
        curveto[0][0] = *pt;
        curveto[0][1] = *(++pt);
        ++pt; // Flush z
        curveto[1][0] = *(++pt);
        curveto[1][1] = *(++pt);
        ++pt; // Flush z

        out << "    Q " << curveto[0][0] << " " << curveto[0][1] << endl
            << "      " << curveto[1][0] << " " << curveto[1][1] << endl;
      }
        break;
      case vtkPath::CUBIC_CURVE:
      {
        // Next two points should be CUBIC_CURVEs as well
        code += 2;

        curveto[0][0] = *pt;
        curveto[0][1] = *(++pt);
        ++pt;
        curveto[1][0] = *(++pt);
        curveto[1][1] = *(++pt);
        ++pt;
        curveto[2][0] = *(++pt);
        curveto[2][1] = *(++pt);
        ++pt;

        out << "    C " << curveto[0][0] << " " << curveto[0][1] << endl
            << "      " << curveto[1][0] << " " << curveto[1][1] << endl
            << "      " << curveto[2][0] << " " << curveto[2][1] << endl;
      }
        break;
      default:
        out << "<!-- Unrecognized control code: " << *code << " -->" << endl;
        pt +=2;
        break;
    }

    ++code;
    ++pt;
  }

  out << "    \" />" << endl
      << "</g>" << endl;

  GL2PSvertex gl2psRasterPos;
  gl2psRasterPos.xyz[0] = static_cast<float>(rasterPos[0]);
  gl2psRasterPos.xyz[1] = static_cast<float>(rasterPos[1]);
  gl2psRasterPos.xyz[2] = static_cast<float>(rasterPos[2]);
  gl2psRasterPos.rgba[0] = 0.f;
  gl2psRasterPos.rgba[1] = 0.f;
  gl2psRasterPos.rgba[2] = 0.f;
  gl2psRasterPos.rgba[3] = 0.f;
  gl2psForceRasterPos(&gl2psRasterPos);
  gl2psSpecial(gl2psGetFileFormat(), out.str().c_str());
}
