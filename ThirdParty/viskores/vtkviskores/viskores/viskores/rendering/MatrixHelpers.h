//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_rendering_MatrixHelpers_h
#define viskores_rendering_MatrixHelpers_h

#include <viskores/Matrix.h>
#include <viskores/VectorAnalysis.h>

namespace viskores
{
namespace rendering
{

struct MatrixHelpers
{
  static VISKORES_CONT void CreateOGLMatrix(const viskores::Matrix<viskores::Float32, 4, 4>& mtx,
                                            viskores::Float32* oglM)
  {
    oglM[0] = mtx[0][0];
    oglM[1] = mtx[1][0];
    oglM[2] = mtx[2][0];
    oglM[3] = mtx[3][0];
    oglM[4] = mtx[0][1];
    oglM[5] = mtx[1][1];
    oglM[6] = mtx[2][1];
    oglM[7] = mtx[3][1];
    oglM[8] = mtx[0][2];
    oglM[9] = mtx[1][2];
    oglM[10] = mtx[2][2];
    oglM[11] = mtx[3][2];
    oglM[12] = mtx[0][3];
    oglM[13] = mtx[1][3];
    oglM[14] = mtx[2][3];
    oglM[15] = mtx[3][3];
  }

  static VISKORES_CONT viskores::Matrix<viskores::Float32, 4, 4> ViewMatrix(
    const viskores::Vec3f_32& position,
    const viskores::Vec3f_32& lookAt,
    const viskores::Vec3f_32& up)
  {
    viskores::Vec3f_32 viewDir = position - lookAt;
    viskores::Vec3f_32 right = viskores::Cross(up, viewDir);
    viskores::Vec3f_32 ru = viskores::Cross(viewDir, right);

    viskores::Normalize(viewDir);
    viskores::Normalize(right);
    viskores::Normalize(ru);

    viskores::Matrix<viskores::Float32, 4, 4> matrix;
    viskores::MatrixIdentity(matrix);

    matrix(0, 0) = right[0];
    matrix(0, 1) = right[1];
    matrix(0, 2) = right[2];
    matrix(1, 0) = ru[0];
    matrix(1, 1) = ru[1];
    matrix(1, 2) = ru[2];
    matrix(2, 0) = viewDir[0];
    matrix(2, 1) = viewDir[1];
    matrix(2, 2) = viewDir[2];

    matrix(0, 3) = -viskores::Dot(right, position);
    matrix(1, 3) = -viskores::Dot(ru, position);
    matrix(2, 3) = -viskores::Dot(viewDir, position);

    return matrix;
  }

  static VISKORES_CONT viskores::Matrix<viskores::Float32, 4, 4> WorldMatrix(
    const viskores::Vec3f_32& neworigin,
    const viskores::Vec3f_32& newx,
    const viskores::Vec3f_32& newy,
    const viskores::Vec3f_32& newz)
  {
    viskores::Matrix<viskores::Float32, 4, 4> matrix;
    viskores::MatrixIdentity(matrix);

    matrix(0, 0) = newx[0];
    matrix(0, 1) = newy[0];
    matrix(0, 2) = newz[0];
    matrix(1, 0) = newx[1];
    matrix(1, 1) = newy[1];
    matrix(1, 2) = newz[1];
    matrix(2, 0) = newx[2];
    matrix(2, 1) = newy[2];
    matrix(2, 2) = newz[2];

    matrix(0, 3) = neworigin[0];
    matrix(1, 3) = neworigin[1];
    matrix(2, 3) = neworigin[2];

    return matrix;
  }

  static VISKORES_CONT viskores::Matrix<viskores::Float32, 4, 4>
  CreateScale(const viskores::Float32 x, const viskores::Float32 y, const viskores::Float32 z)
  {
    viskores::Matrix<viskores::Float32, 4, 4> matrix;
    viskores::MatrixIdentity(matrix);
    matrix[0][0] = x;
    matrix[1][1] = y;
    matrix[2][2] = z;

    return matrix;
  }

  static VISKORES_CONT viskores::Matrix<viskores::Float32, 4, 4> TrackballMatrix(
    viskores::Float32 p1x,
    viskores::Float32 p1y,
    viskores::Float32 p2x,
    viskores::Float32 p2y)
  {
    const viskores::Float32 RADIUS = 0.80f;     //z value lookAt x = y = 0.0
    const viskores::Float32 COMPRESSION = 3.5f; // multipliers for x and y.
    const viskores::Float32 AR3 = RADIUS * RADIUS * RADIUS;

    viskores::Matrix<viskores::Float32, 4, 4> matrix;

    viskores::MatrixIdentity(matrix);
    if (p1x == p2x && p1y == p2y)
    {
      return matrix;
    }

    viskores::Vec3f_32 p1(p1x, p1y, AR3 / ((p1x * p1x + p1y * p1y) * COMPRESSION + AR3));
    viskores::Vec3f_32 p2(p2x, p2y, AR3 / ((p2x * p2x + p2y * p2y) * COMPRESSION + AR3));
    viskores::Vec3f_32 axis = viskores::Normal(viskores::Cross(p2, p1));

    viskores::Vec3f_32 p2_p1(p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2]);
    viskores::Float32 t = viskores::Magnitude(p2_p1);
    t = viskores::Min(viskores::Max(t, -1.0f), 1.0f);
    viskores::Float32 phi = static_cast<viskores::Float32>(-2.0f * asin(t / (2.0f * RADIUS)));
    viskores::Float32 val = static_cast<viskores::Float32>(sin(phi / 2.0f));
    axis[0] *= val;
    axis[1] *= val;
    axis[2] *= val;

    //quaternion
    viskores::Float32 q[4] = {
      axis[0], axis[1], axis[2], static_cast<viskores::Float32>(cos(phi / 2.0f))
    };

    // normalize quaternion to unit magnitude
    t = 1.0f /
      static_cast<viskores::Float32>(sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]));
    q[0] *= t;
    q[1] *= t;
    q[2] *= t;
    q[3] *= t;

    matrix(0, 0) = 1 - 2 * (q[1] * q[1] + q[2] * q[2]);
    matrix(0, 1) = 2 * (q[0] * q[1] + q[2] * q[3]);
    matrix(0, 2) = (2 * (q[2] * q[0] - q[1] * q[3]));

    matrix(1, 0) = 2 * (q[0] * q[1] - q[2] * q[3]);
    matrix(1, 1) = 1 - 2 * (q[2] * q[2] + q[0] * q[0]);
    matrix(1, 2) = (2 * (q[1] * q[2] + q[0] * q[3]));

    matrix(2, 0) = (2 * (q[2] * q[0] + q[1] * q[3]));
    matrix(2, 1) = (2 * (q[1] * q[2] - q[0] * q[3]));
    matrix(2, 2) = (1 - 2 * (q[1] * q[1] + q[0] * q[0]));

    return matrix;
  }
};
}
} //namespace viskores::rendering

#endif // viskores_rendering_MatrixHelpers_h
