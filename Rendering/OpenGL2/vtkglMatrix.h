/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkglMatrix_h
#define __vtkglMatrix_h

#include <Eigen/Dense>

namespace vtkgl {

/** Typedefs for vector types. */
typedef Eigen::Matrix<double, 2, 2> Matrix2;
typedef Eigen::Matrix<double, 3, 3> Matrix3;
typedef Eigen::Matrix<double, 4, 4> Matrix4;
typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> MatrixX;

typedef Eigen::Matrix<float, 2, 2> Matrix2f;
typedef Eigen::Matrix<float, 3, 3> Matrix3f;
typedef Eigen::Matrix<float, 4, 4> Matrix4f;
typedef Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> MatrixXf;

}

#endif
