/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPistonDataWrangling.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPistonDataWrangling
 * @brief   Miscellaneous conversion code.
 *
 * Miscellaneous code that is used in conversion between vtk and piston.
 * The vtk_polydata struct is important as that is how piston's polygonal
 * results get brought back to the CPU.
*/

#ifndef vtkPistonDataWrangling_h
#define vtkPistonDataWrangling_h

#include <thrust/version.h>
#if THRUST_VERSION >= 100600
# define SPACE thrust::device_space_tag
#else
# define SPACE thrust::detail::default_device_space_tag
#endif

namespace vtkpiston {

void DeleteData(vtkPistonReference *);

typedef struct
{
  //GPU side representation of a vtkPolyData
  //this is the sibling of vtk_image3D in piston
  int nPoints;
  int vertsPer;
  thrust::device_vector<float> *points;
  thrust::device_vector<float> *scalars;
  thrust::device_vector<float> *normals;
} vtk_polydata;

struct tuple2float3 :
  thrust::unary_function<thrust::tuple<float, float, float>, float3>
{
    __host__ __device__
      float3 operator()(thrust::tuple<float, float, float> xyz) {
        return make_float3((float) thrust::get<0>(xyz),
                           (float) thrust::get<1>(xyz),
                           (float) thrust::get<2>(xyz));
    }
};

struct float4tofloat3 : thrust::unary_function<float4, float3>
{
  __host__ __device__
  float3 operator()(float4 xyzw) {
    return make_float3
          ((float) xyzw.x,
           (float) xyzw.y,
           (float) xyzw.z);
  }
};

} //namespace

#endif //vtkPistonDataWrangling_h
// VTK-HeaderTest-Exclude: vtkPistonDataWrangling.h
