/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestLine.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkNew.h"
#include "vtkOpenGLUniforms.h"

#include <algorithm>
#include <vector>

int UnitTestOpenGLUniforms(int, char*[])
{
  vtkNew<vtkOpenGLUniforms> uni;

  // uniform i
  int ini = 1;
  int outi = 0;
  uni->SetUniformi("i", ini);
  uni->GetUniformi("i", outi);
  if (outi != ini)
  {
    return EXIT_FAILURE;
  }

  // Test generic get functions
  std::vector<int> vi;
  if (!uni->GetUniform("i", vi))
  {
    std::cerr << "<1>" << std::endl;
    return EXIT_FAILURE;
  }
  if (vi[0] != ini)
  {
    std::cerr << "<2>" << std::endl;
    return EXIT_FAILURE;
  }
  std::vector<float> vf;
  if (uni->GetUniform("i", vf))
  {
    std::cerr << "<3>" << std::endl;
    return EXIT_FAILURE;
  }

  // uniform f
  float inf = 1.0f;
  float outf = 0.0f;
  uni->SetUniformf("f", inf);
  uni->GetUniformf("f", outf);
  if (outf != inf)
  {
    return EXIT_FAILURE;
  }

  // Test generic get functions
  if (!uni->GetUniform("f", vf))
  {
    std::cerr << "<4>" << std::endl;
    return EXIT_FAILURE;
  }
  if (vf.size() != 1 && vf[0] != inf)
  {
    std::cerr << "<5>" << std::endl;
    return EXIT_FAILURE;
  }
  if (uni->GetUniform("f", vi))
  {
    std::cerr << "<6>" << std::endl;
    return EXIT_FAILURE;
  }

  // uniform 2i
  int in2i[2] = { 1, 2 };
  int out2i[2] = { 0, 0 };
  uni->SetUniform2i("2i", in2i);
  uni->GetUniform2i("2i", out2i);
  if (!std::equal(in2i, in2i + 2, out2i))
  {
    return EXIT_FAILURE;
  }

  // uniform 2f
  float in2f[2] = { 1.0f, 2.0f };
  float out2f[2] = { 0.0f, 0.0f };
  uni->SetUniform2f("2f", in2f);
  uni->GetUniform2f("2f", out2f);
  if (!std::equal(in2f, in2f + 2, out2f))
  {
    return EXIT_FAILURE;
  }

  // uniform 3f
  float in3f[3] = { 1.0f, 2.0f, 3.0f };
  float out3f[3] = { 0.0f, 0.0f, 0.0f };
  uni->SetUniform3f("3f", in3f);
  uni->GetUniform3f("3f", out3f);
  if (!std::equal(in3f, in3f + 3, out3f))
  {
    return EXIT_FAILURE;
  }

  // uniform 4f
  float in4f[4] = { 1.0f, 2.0f, 3.0f };
  float out4f[4] = { 0.0f, 0.0f, 0.0f };
  uni->SetUniform4f("4f", in4f);
  uni->GetUniform4f("4f", out4f);
  if (!std::equal(in4f, in4f + 4, out4f))
  {
    return EXIT_FAILURE;
  }

  // uniform Mat3x3
  float inMat3x3[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
  float outMat3x3[9];
  std::fill(outMat3x3, outMat3x3 + 9, 0.0f);
  uni->SetUniformMatrix3x3("Mat3x3f", inMat3x3);
  uni->GetUniformMatrix3x3("Mat3x3f", outMat3x3);
  if (!std::equal(inMat3x3, inMat3x3 + 9, outMat3x3))
  {
    return EXIT_FAILURE;
  }

  // uniform Mat4x4
  float inMat4x4[16] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f };
  float outMat4x4[16];
  std::fill(outMat4x4, outMat4x4 + 16, 0.0f);
  uni->SetUniformMatrix4x4("Mat4x4f", inMat4x4);
  uni->GetUniformMatrix4x4("Mat4x4f", outMat4x4);
  if (!std::equal(inMat4x4, inMat4x4 + 16, outMat4x4))
  {
    return EXIT_FAILURE;
  }

  // uniform 1iv
  int in1iv[2] = { 1, 2 };
  std::vector<int> out1iv;
  uni->SetUniform1iv("1iv", 2, in1iv);
  uni->GetUniform1iv("1iv", out1iv);
  if (!std::equal(in1iv, in1iv + 2, out1iv.begin()))
  {
    return EXIT_FAILURE;
  }

  // uniform 1fv
  float in1fv[2] = { 1.0f, 2.0f };
  std::vector<float> out1fv;
  uni->SetUniform1fv("1fv", 2, in1fv);
  uni->GetUniform1fv("1fv", out1fv);
  if (!std::equal(in1fv, in1fv + 2, out1fv.begin()))
  {
    return EXIT_FAILURE;
  }

  // uniform 2fv
  float in2fv[2][2] = { { 1.0f, 2.0f }, { 3.0f, 4.0f } };
  std::vector<float> out2fv;
  uni->SetUniform2fv("2fv", 2, in2fv);
  uni->GetUniform2fv("2fv", out2fv);
  float* in2fvArray = reinterpret_cast<float*>(in2fv);
  if (!std::equal(in2fvArray, in2fvArray + 4, out2fv.begin()))
  {
    return EXIT_FAILURE;
  }

  // uniform 3fv
  float in3fv[2][3] = { { 1.0f, 2.0f, 3.0f }, { 4.0f, 5.0f, 6.0f } };
  std::vector<float> out3fv;
  uni->SetUniform3fv("3fv", 2, in3fv);
  uni->GetUniform3fv("3fv", out3fv);
  float* in3fvArray = reinterpret_cast<float*>(in3fv);
  if (!std::equal(in3fvArray, in3fvArray + 6, out3fv.begin()))
  {
    return EXIT_FAILURE;
  }

  // uniform 4fv
  float in4fv[2][4] = { { 1.0f, 2.0f, 3.0f, 4.0f }, { 5.0f, 6.0f, 7.0f, 8.0f } };
  std::vector<float> out4fv;
  uni->SetUniform4fv("4fv", 2, in4fv);
  uni->GetUniform4fv("4fv", out4fv);
  float* in4fvArray = reinterpret_cast<float*>(in4fv);
  if (!std::equal(in4fvArray, in4fvArray + 8, out4fv.begin()))
  {
    return EXIT_FAILURE;
  }

  // uniform Matrix4x4v
  float inMat4x4v[32] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 2.0f };
  std::vector<float> outMat4x4v;
  uni->SetUniformMatrix4x4v("Mat4x4v", 2, inMat4x4v);
  uni->GetUniformMatrix4x4v("Mat4x4v", outMat4x4v);
  if (!std::equal(inMat4x4v, inMat4x4v + 32, outMat4x4v.begin()))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
