/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkglVector_h
#define __vtkglVector_h

#include <Eigen/Dense>

namespace vtkgl {

/** Typedefs for vector types. */
typedef double Real;
typedef Eigen::Matrix<Real, 2, 1> Vector2;
typedef Eigen::Matrix<Real, 3, 1> Vector3;
typedef Eigen::Matrix<Real, 4, 1> Vector4;

typedef Eigen::Matrix<float, 2, 1> Vector2f;
typedef Eigen::Matrix<float, 3, 1> Vector3f;
typedef Eigen::Matrix<float, 4, 1> Vector4f;
typedef Eigen::Matrix<double, 2, 1> Vector2d;
typedef Eigen::Matrix<double, 3, 1> Vector3d;
typedef Eigen::Matrix<double, 4, 1> Vector4d;
typedef Eigen::Matrix<int, 2, 1> Vector2i;
typedef Eigen::Matrix<int, 3, 1> Vector3i;
typedef Eigen::Matrix<int, 4, 1> Vector4i;
typedef Eigen::Matrix<unsigned char, 2, 1> Vector2ub;
typedef Eigen::Matrix<unsigned char, 3, 1> Vector3ub;
typedef Eigen::Matrix<unsigned char, 4, 1> Vector4ub;

}

#endif
