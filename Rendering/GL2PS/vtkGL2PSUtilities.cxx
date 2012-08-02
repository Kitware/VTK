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
#include "vtkObjectFactory.h"
#include "vtkPath.h"
#include "vtkTextProperty.h"

#include "vtk_gl2ps.h"

#include <cassert>
#include <sstream>
#include <string>

vtkStandardNewMacro(vtkGL2PSUtilities)

void vtkGL2PSUtilities::DrawString(const char *str,
                                   vtkTextProperty *tprop, double pos[])
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
  gl2psTextOpt(str, fontname, fontSize, align, angle, rgba);
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

void vtkGL2PSUtilities::DrawPath(vtkPath *path, double rasterPos[3],
                                 double windowSize[2], double translation[2],
                                 double scale[2], double rotateAngle,
                                 unsigned char color[3])
{
  switch (gl2psGetFileFormat())
    {
    case GL2PS_PS:
    case GL2PS_EPS:
      vtkGL2PSUtilities::DrawPathPS(path, rasterPos, windowSize, translation,
                                    scale, rotateAngle, color);
      break;
    case GL2PS_SVG:
      vtkGL2PSUtilities::DrawPathSVG(path, rasterPos, windowSize, translation,
                                     scale, rotateAngle, color);
      break;
    case GL2PS_PDF:
      vtkGL2PSUtilities::DrawPathPDF(path, rasterPos, windowSize, translation,
                                     scale, rotateAngle, color);
      break;
    default:
      break;
    }
}

void vtkGL2PSUtilities::DrawPathPS(vtkPath *path, double rasterPos[3],
                                   double /*windowSize*/[2],
                                   double translation[2], double scale[],
                                   double rotateAngle, unsigned char color[3])
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
  out << translation[0] << " " << translation[1] << " translate" << endl;
  out << scale[0] << " " << scale[1] << " scale" << endl;
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

  out << static_cast<float>(color[0])/255.f << " " <<
         static_cast<float>(color[1])/255.f << " " <<
         static_cast<float>(color[2])/255.f << " setrgbcolor" << endl;
  out << "fill" << endl;
  out << "grestore" << endl;

  glRasterPos3dv(rasterPos);
  gl2psSpecial(gl2psGetFileFormat(), out.str().c_str());
}

void vtkGL2PSUtilities::DrawPathPDF(vtkPath *path, double rasterPos[3],
                                    double /*windowSize*/[2],
                                    double translation[2], double scale[2],
                                    double rotateAngle, unsigned char color[3])
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
  out << static_cast<float>(color[0])/255.f << " " <<
         static_cast<float>(color[1])/255.f << " " <<
         static_cast<float>(color[2])/255.f << " rg" << endl;
  // translate
  out << 1.f << " " << 0.f << " " << 0.f << " " << 1.f << " "
      << translation[0] << " " << translation[1] << " cm" << endl;
  // rotate
  float sT = sin(vtkMath::RadiansFromDegrees(rotateAngle));
  float cT = cos(vtkMath::RadiansFromDegrees(rotateAngle));
  out << cT << " " << sT << " " << -sT << " " << cT << " " << 0.f << " " << 0.f
      << " cm" << endl;
  // scale
  out << scale[0] << " " << 0.f << " " << 0.f << " " << scale[1] << " " << 0.f
      << " " << 0.f << " cm" << endl;

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

  out << "h f" << endl;
  out << "Q" << endl; // Pop state

  glRasterPos3dv(rasterPos);
  gl2psSpecial(gl2psGetFileFormat(), out.str().c_str());
}

void vtkGL2PSUtilities::DrawPathSVG(vtkPath *path, double rasterPos[3],
                                    double windowSize[2],
                                    double translation[2], double scale[],
                                    double rotateAngle, unsigned char color[3])
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
  out << "<g transform=\"" << endl
      << "     translate(" << translation[0] << " "
      << windowSize[1]-translation[1] << ")" << endl
      << "     scale(" << scale[0] << " " << -scale[1] << ")" << endl
      << "     rotate(" << rotateAngle << ")\"" << endl
      << "   fill=\"rgb(" << std::setprecision(0)
      << static_cast<int>(color[0]) << ","
      << static_cast<int>(color[1]) << ","
      << static_cast<int>(color[2])
      << std::setprecision(floatPrec) << ")\">" << endl;
  out << "  <path d=\"" << std::right << endl;
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
