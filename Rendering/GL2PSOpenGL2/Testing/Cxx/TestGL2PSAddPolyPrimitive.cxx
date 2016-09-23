/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGL2PSAddPolyPrimitive.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This test checks that we can use GL2PS without an OpenGL context using
// a buffer size of 0, gl2psAddPolyPrimitive, and gl2psForceRasterPos.

#include "vtk_gl2ps.h"

#include "vtkTestingInteractor.h"

#include <iostream>
#include <string>
#include <vector>

namespace {

static inline void setVertex(GL2PSvertex &vert,
                             float x, float y, float z,
                             float r, float g, float b, float a)
{
  vert.xyz[0] = x;
  vert.xyz[1] = y;
  vert.xyz[2] = z;
  vert.rgba[0] = r;
  vert.rgba[1] = g;
  vert.rgba[2] = b;
  vert.rgba[3] = a;
}

static inline void generatePixelData(std::vector<float> &data,
                                     size_t width, size_t height)
{
  data.resize(width * height * 4);
  for (size_t h = 0; h < height; ++h)
  {
    size_t rowOffset = h * width * 4;
    for (size_t w = 0; w < width; ++w)
    {
      size_t pixel = rowOffset + (w * 4);
      data[pixel    ] = h / static_cast<float>(height);
      data[pixel + 1] = 0.f;
      data[pixel + 2] = w / static_cast<float>(width);
      data[pixel + 3] = 1.f;
    }
  }
}

} // end anon namespace

int TestGL2PSAddPolyPrimitive(int , char * [])
{
  std::string filename = vtkTestingInteractor::TempDirectory +
      std::string("/TestGL2PSAddPolyPrimitive.ps");
  FILE *stream = fopen(filename.c_str(), "wb");
  if (stream == NULL)
  {
    std::cerr << "Error opening output file." << std::endl;
    return EXIT_FAILURE;
  }

  GLint viewport[4] = { 0, 0, 400, 400 };
  GLint result = gl2psBeginPage("AddPolyPrimitive Test", "VTK", viewport,
                                GL2PS_PS, GL2PS_SIMPLE_SORT,
                                GL2PS_NO_OPENGL_CONTEXT | GL2PS_NO_BLENDING,
                                GL_RGBA, 0, NULL, 0, 0, 0, 0, stream, 0);
  if (result != GL2PS_SUCCESS)
  {
    std::cerr << "gl2psBeginPage failed." << std::endl;
    return EXIT_FAILURE;
  }

  // AddPolyPrimitive arguments:
  GL2PSvertex vertices[3]; // Vertices.
  GLint offset = 0; // line offset
  GLushort pattern = 0xffff; // glLineStipple pattern
  GLint factor = 1; // glLineStipple repeat factor
  GLfloat ofactor = 0.f; // glPolygonOffset factor
  GLfloat ounits = 0.f; // glPolygonOffset units
  GLfloat width = 1; // linewidth or pointsize
  // Something to do with gl2psEnable(GL2PS_POLYGON_BOUNDARY), which is not
  // implemented according to the docs.
  char boundary = 0;

  // Point:
  setVertex(vertices[0], 200, 307.5, 0, 0.f, 0.f, 1.f, 1.f);
  gl2psAddPolyPrimitive(GL2PS_POINT, 1, vertices, offset, ofactor, ounits,
                        pattern, factor, /*width=*/15, boundary);

  // Line:
  // Note that the first vertex's color is used for the entire line.
  setVertex(vertices[0], 100, 50, 0, 1.f, 0.f, 0.f, 1.f);
  setVertex(vertices[1], 300, 50, 0, 0.f, 0.f, 1.f, 1.f);
  gl2psAddPolyPrimitive(GL2PS_LINE, 2, vertices, offset, ofactor, ounits,
                        pattern, factor, width, boundary);

  // Triangle:
  setVertex(vertices[0], 100, 100, 0, 1.f, 0.f, 0.f, 1.f);
  setVertex(vertices[1], 300, 100, 0, 0.f, 1.f, 0.f, 1.f);
  setVertex(vertices[2], 200, 300, 0, 0.f, 0.f, 1.f, 1.f);
  gl2psAddPolyPrimitive(GL2PS_TRIANGLE, 3, vertices, offset, ofactor, ounits,
                        pattern, factor, width, boundary);

  // Text:
  setVertex(vertices[0], 200, 325, 0, 0.f, 0.f, 0.f, 1.f);
  gl2psForceRasterPos(vertices);
  gl2psTextOpt("GL2PS with no OpenGL", "Helvetica", 12, GL2PS_TEXT_B, 0.f);

  // DrawPixels:
  std::vector<float> pixelData;
  generatePixelData(pixelData, 100, 100);
  setVertex(vertices[0], 275, 275, 0, 0.f, 0.f, 0.f, 0.f);
  gl2psForceRasterPos(vertices);
  gl2psDrawPixels(100, 100, 0, 0, GL_RGBA, GL_FLOAT, &pixelData[0]);

  result = gl2psEndPage();
  if (result != GL2PS_SUCCESS)
  {
    std::cerr << "gl2psEndPage failed." << std::endl;
    return EXIT_FAILURE;
  }

  fclose(stream);

  return EXIT_SUCCESS;
}
