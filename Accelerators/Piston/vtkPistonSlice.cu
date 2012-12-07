#include <thrust/copy.h>
#include <thrust/device_vector.h>
#include "vtkgl.h"

#include <piston/marching_cube.h>
#include <piston/vtk_image3d.h>
#include <piston/vtk_plane_field.h>

#include "vtkPistonDataObject.h"
#include "vtkPistonDataWrangling.h"
#include "vtkPistonReference.h"
#include <iostream>

using namespace std;
using namespace piston;

namespace vtkpiston {

void ExecutePistonSlice(vtkPistonDataObject *inData,
                        float* origin, float*normal, float offset,
                        vtkPistonDataObject *outData)
{
  vtkPistonReference *ti = inData->GetReference();
  if (ti->type != VTK_IMAGE_DATA || ti->data == NULL)
    {
    // type mismatch, don't bother trying
    return;
    }
  vtk_image3d<SPACE>*gpuData =
      (vtk_image3d<SPACE>*)ti->data;

  float dataOrigin[3];
  dataOrigin[0] = static_cast<float>(gpuData->origin[0]);
  dataOrigin[1] = static_cast<float>(gpuData->origin[1]);
  dataOrigin[2] = static_cast<float>(gpuData->origin[2]);

  float dataSpacing[3];
  dataSpacing[0] = static_cast<float>(gpuData->spacing[0]);
  dataSpacing[1] = static_cast<float>(gpuData->spacing[1]);
  dataSpacing[2] = static_cast<float>(gpuData->spacing[2]);

  int dims[3];
  dims[0] = gpuData->dim0;
  dims[1] = gpuData->dim1;
  dims[2] = gpuData->dim2;

  vtk_plane_field<int, float, SPACE > *plane =
    new vtk_plane_field<int, float, SPACE >(
      dataOrigin, normal, dims, dataSpacing, gpuData->extents);

  marching_cube<vtk_plane_field<int, float, SPACE>,
    vtk_image3d<SPACE> > pistonFunctor(*plane, *gpuData, offset);

  // Execute the piston filter
  pistonFunctor();

  vtkPistonReference *to = outData->GetReference();
  DeleteData(to);

  to->type = VTK_POLY_DATA;
  vtk_polydata *newD = new vtk_polydata;
  to->data = (void*)newD;
  //geometry
  newD->nPoints = pistonFunctor.vertices.size();
  newD->vertsPer = 3; //this piston filter produces triangles
  newD->points = new thrust::device_vector<float>(newD->nPoints*3);
  thrust::device_vector<float3> *tmp =
    (thrust::device_vector<float3> *)newD->points;
  thrust::copy(thrust::make_transform_iterator(pistonFunctor.vertices_begin(),
                                               float4tofloat3()),
               thrust::make_transform_iterator(pistonFunctor.vertices_end(),
                                               float4tofloat3()),
               tmp->begin());
  //attributes
  newD->scalars = new thrust::device_vector<float>(newD->nPoints);
  thrust::copy(pistonFunctor.scalars_begin(), pistonFunctor.scalars_end(),
               newD->scalars->begin());
  outData->SetScalarsArrayName(inData->GetScalarsArrayName());
  newD->normals = new thrust::device_vector<float>(newD->nPoints*3);
  //TODO: FIX UP THIS CAST
  thrust::device_vector<float3>* casted =
    reinterpret_cast<thrust::device_vector<float3>*>(newD->normals);
  thrust::copy(pistonFunctor.normals_begin(), pistonFunctor.normals_end(),
               casted->begin());
}

} //namespace
