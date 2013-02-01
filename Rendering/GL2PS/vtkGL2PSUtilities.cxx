/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGL2PSUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGL2PSUtilities.h"

#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPath.h"
#include "vtkRenderWindow.h"
#include "vtkTextProperty.h"
#include "vtkTextRenderer.h"

#include "vtk_gl2ps.h"

#include <cassert>
#include <sstream>
#include <string>

vtkStandardNewMacro(vtkGL2PSUtilities)
bool vtkGL2PSUtilities::TextAsPath = false;
vtkRenderWindow *vtkGL2PSUtilities::RenderWindow = NULL;

void vtkGL2PSUtilities::DrawString(const char *str,
                                   vtkTextProperty *tprop, double pos[])
{
  if (!vtkGL2PSUtilities::TextAsPath)
    {
    const char *fontname = vtkGL2PSUtilities::TextPropertyToPSFontName(tprop);

    GLint align = static_cast<GLint>(
          vtkGL2PSUtilities::TextPropertyToGL2PSAlignment(tprop));

    GLfloat angle = static_cast<GLfloat>(tprop->GetOrientation());

    GLint fontSize = static_cast<GLint>(tprop->GetFontSize());

    GL2PSrgba rgba;
    double rgbad[3];
    tprop->GetColor(rgbad[0], rgbad[1], rgbad[2]);
    rgba[0] = static_cast<GLfloat>(rgbad[0]);
    rgba[1] = static_cast<GLfloat>(rgbad[1]);
    rgba[2] = static_cast<GLfloat>(rgbad[2]);
    rgba[3] = tprop->GetOpacity();

    glRasterPos3dv(pos);
    gl2psTextOptColor(str, fontname, fontSize, align, angle, rgba);
    }
  else
    {
    // Render the string to a path and then draw it to GL2PS:
    vtkNew<vtkPath> path;
    if (vtkTextRenderer *tren = vtkTextRenderer::GetInstance())
      {
      tren->StringToPath(tprop, str, path.GetPointer());
      }
    else
      {
      vtkNew<vtkGL2PSUtilities> dummy;
      vtkErrorWithObjectMacro(dummy.GetPointer(),
                              <<"vtkTextRenderer unavailable.");
      return;
      }
    // Get color
    double rgbd[3];
    tprop->GetColor(rgbd[0], rgbd[1], rgbd[2]);
    unsigned char rgba[4] = {
      static_cast<unsigned char>(rgbd[0]*255),
      static_cast<unsigned char>(rgbd[1]*255),
      static_cast<unsigned char>(rgbd[2]*255),
      static_cast<unsigned char>(tprop->GetOpacity()*255)};

    // Get device coordinate
    double devicePos[3] = {pos[0], pos[1], pos[2]};
    vtkGL2PSUtilities::ProjectPoint(devicePos);

    vtkGL2PSUtilities::DrawPath(path.GetPointer(), pos, devicePos, rgba);
    }
}

const char *vtkGL2PSUtilities::TextPropertyToPSFontName(vtkTextProperty *tprop)
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
      else if (italic)
        {
        return "Helvetica-Italic";
        }
      }
      break;
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
      else if (italic)
        {
        return "Times-Oblique";
        }
      }
      break;
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
      else if (italic)
        {
        return "Courier-Oblique";
        }
      }
      break;
    case VTK_UNKNOWN_FONT:
    default:
      break;
    }

  return "Helvetica";
}

int vtkGL2PSUtilities::TextPropertyToGL2PSAlignment(vtkTextProperty *tprop)
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

void vtkGL2PSUtilities::Draw3DPath(vtkPath *path, vtkMatrix4x4 *actorMatrix,
                                   double rasterPos[3],
                                   unsigned char actorColor[4])
{
  double translation[2] = {0.0, 0.0};
  vtkNew<vtkPath> projPath;
  projPath->DeepCopy(path);
  vtkGL2PSUtilities::ProjectPoints(projPath->GetPoints(), actorMatrix);
  vtkGL2PSUtilities::DrawPath(projPath.GetPointer(), rasterPos, translation,
                              actorColor);
}

void vtkGL2PSUtilities::DrawPath(vtkPath *path, double rasterPos[3],
                                 double windowPos[2], unsigned char rgba[4],
                                 double scale[2], double rotateAngle,
                                 float strokeWidth)
{
  switch (gl2psGetFileFormat())
    {
    case GL2PS_PS:
    case GL2PS_EPS:
      vtkGL2PSUtilities::DrawPathPS(path, rasterPos, windowPos, rgba, scale,
                                    rotateAngle, strokeWidth);
      break;
    case GL2PS_SVG:
      vtkGL2PSUtilities::DrawPathSVG(path, rasterPos, windowPos, rgba, scale,
                                     rotateAngle, strokeWidth);
      break;
    case GL2PS_PDF:
      vtkGL2PSUtilities::DrawPathPDF(path, rasterPos, windowPos, rgba, scale,
                                     rotateAngle, strokeWidth);
      break;
    default:
      break;
    }
}

void vtkGL2PSUtilities::DrawPathPS(vtkPath *path, double rasterPos[3],
                                   double windowPos[2], unsigned char rgba[4],
                                   double scale[2], double rotateAngle,
                                   float strokeWidth)
{
  vtkFloatArray *points =
      vtkFloatArray::SafeDownCast(path->GetPoints()->GetData());
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

  glRasterPos3dv(rasterPos);
  gl2psSpecial(gl2psGetFileFormat(), out.str().c_str());
}

void vtkGL2PSUtilities::DrawPathPDF(vtkPath *path, double rasterPos[3],
                                    double windowPos[2], unsigned char rgba[4],
                                    double scale[2], double rotateAngle,
                                    float strokeWidth)
{
  vtkFloatArray *points =
      vtkFloatArray::SafeDownCast(path->GetPoints()->GetData());
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

  glRasterPos3dv(rasterPos);
  gl2psSpecial(gl2psGetFileFormat(), out.str().c_str());
}

void vtkGL2PSUtilities::DrawPathSVG(vtkPath *path, double rasterPos[3],
                                    double windowPos[2], unsigned char rgba[4],
                                    double scale[2], double rotateAngle,
                                    float strokeWidth)
{
  vtkFloatArray *points =
      vtkFloatArray::SafeDownCast(path->GetPoints()->GetData());
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

  // Get the size of the render window -- needed to calculate the SVG position.
  if (!vtkGL2PSUtilities::RenderWindow)
    {
    vtkNew<vtkGL2PSUtilities> dummy;
    vtkErrorWithObjectMacro(dummy.GetPointer(), << "No render window set!");
    return;
    }
  double windowHeight =
      static_cast<double>(vtkGL2PSUtilities::RenderWindow->GetSize()[1]);

  // These are only used in an assertion, ifdef silences warning on non-debug
  // builds
#ifndef NDEBUG
  float *ptBegin = pt;
  int *codeBegin = code;
#endif
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
        const float &conicX = *pt;
        const float &conicY = *(++pt);
        ++pt; // Flush z
        const float &nextX = *(++pt);
        const float &nextY = *(++pt);
        ++pt; // Flush z

        // Perform degree elevation:
        curveto[0][0] = conicX;
        curveto[0][1] = conicY;
        curveto[1][0] = curX = nextX;
        curveto[1][1] = curY = nextY;
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
        curveto[2][0] = curX = *(++pt);
        curveto[2][1] = curY = *(++pt);
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

  glRasterPos3dv(rasterPos);
  gl2psSpecial(gl2psGetFileFormat(), out.str().c_str());
}


inline void vtkGL2PSUtilities::ProjectPoint(double point[3],
                                            vtkMatrix4x4 *actorMatrix)
{
  // Build transformation matrix
  double glMatrix[16];
  glGetDoublev(GL_PROJECTION_MATRIX, glMatrix);
  vtkNew<vtkMatrix4x4> projectionMatrix;
  projectionMatrix->DeepCopy(glMatrix);
  projectionMatrix->Transpose();

  glGetDoublev(GL_MODELVIEW_MATRIX, glMatrix);
  vtkNew<vtkMatrix4x4> modelviewMatrix;
  modelviewMatrix->DeepCopy(glMatrix);
  modelviewMatrix->Transpose();

  vtkNew<vtkMatrix4x4> transformMatrix;
  vtkMatrix4x4::Multiply4x4(projectionMatrix.GetPointer(),
                            modelviewMatrix.GetPointer(),
                            transformMatrix.GetPointer());
  if (actorMatrix)
    {
    vtkMatrix4x4::Multiply4x4(transformMatrix.GetPointer(),
                              actorMatrix,
                              transformMatrix.GetPointer());
    }

  double viewport[4];
  glGetDoublev(GL_VIEWPORT, viewport);
  double depthRange[2];
  glGetDoublev(GL_DEPTH_RANGE, depthRange);

  const double halfWidth = viewport[2] * 0.5;
  const double halfHeight = viewport[3] * 0.5;
  const double zFactor1 = (depthRange[1] - depthRange[0]) * 0.5;
  const double zFactor2 = (depthRange[1] + depthRange[0]) * 0.5;

  double tmp[4] = {point[0], point[1], point[2], 1.0};
  vtkGL2PSUtilities::ProjectPoint(tmp, transformMatrix.GetPointer(), viewport,
                                  halfWidth, halfHeight, zFactor1, zFactor2);
  point[0] = tmp[0];
  point[1] = tmp[1];
  point[2] = tmp[2];
}

inline void vtkGL2PSUtilities::ProjectPoint(double point[4],
                                            vtkMatrix4x4 *transformMatrix,
                                            double viewportOrigin[],
                                            double halfWidth, double halfHeight,
                                            double zfact1, double zfact2)
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

inline void vtkGL2PSUtilities::ProjectPoints(vtkPoints *points,
                                             vtkMatrix4x4 *actorMatrix)
{
  // Build transformation matrix
  double glMatrix[16];
  glGetDoublev(GL_PROJECTION_MATRIX, glMatrix);
  vtkNew<vtkMatrix4x4> projectionMatrix;
  projectionMatrix->DeepCopy(glMatrix);
  projectionMatrix->Transpose();

  glGetDoublev(GL_MODELVIEW_MATRIX, glMatrix);
  vtkNew<vtkMatrix4x4> modelviewMatrix;
  modelviewMatrix->DeepCopy(glMatrix);
  modelviewMatrix->Transpose();

  vtkNew<vtkMatrix4x4> transformMatrix;
  vtkMatrix4x4::Multiply4x4(projectionMatrix.GetPointer(),
                            modelviewMatrix.GetPointer(),
                            transformMatrix.GetPointer());
  if (actorMatrix)
    {
    vtkMatrix4x4::Multiply4x4(transformMatrix.GetPointer(),
                              actorMatrix,
                              transformMatrix.GetPointer());
    }

  double viewport[4];
  glGetDoublev(GL_VIEWPORT, viewport);
  double depthRange[2];
  glGetDoublev(GL_DEPTH_RANGE, depthRange);

  const double halfWidth = viewport[2] * 0.5;
  const double halfHeight = viewport[3] * 0.5;
  const double zFactor1 = (depthRange[1] - depthRange[0]) * 0.5;
  const double zFactor2 = (depthRange[1] + depthRange[0]) * 0.5;

  double point[4];
  for (vtkIdType i = 0; i < points->GetNumberOfPoints(); ++i)
    {
    points->GetPoint(i, point);
    point[3] = 1.0;
    vtkGL2PSUtilities::ProjectPoint(point, transformMatrix.GetPointer(),
                                    viewport, halfWidth, halfHeight, zFactor1,
                                    zFactor2);
    points->SetPoint(i, point);
    }
}
