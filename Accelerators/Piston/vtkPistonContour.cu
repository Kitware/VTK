#include <thrust/copy.h>
#include <thrust/device_vector.h>
#include "vtkgl.h"
#include <piston/marching_cube.h>
#include <piston/vtk_image3d.h>
#include "vtkPistonDataObject.h"
#include "vtkPistonReference.h"
#include "vtkPistonDataWrangling.h"

#include <iostream>

namespace vtkpiston {

using namespace std;
using namespace piston;

// execution method found in vtkPistonContour.cu
void ExecutePistonContour(vtkPistonDataObject *inData,
                          float isovalue,
                          vtkPistonDataObject *outData)
{
  vtkPistonReference *ti = inData->GetReference();
  if (ti->type != VTK_IMAGE_DATA || ti->data == NULL)
    {
    //type mismatch, don't bother trying
    return;
    }
  vtk_image3d<SPACE>*gpuData = (vtk_image3d<SPACE>*)ti->data;
  marching_cube<vtk_image3d<SPACE>, vtk_image3d<SPACE> >
    pistonFunctor(*gpuData, *gpuData, isovalue);

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
  thrust::device_vector<float3> *tmp = (thrust::device_vector<float3> *)newD->points;
  thrust::copy(thrust::make_transform_iterator(pistonFunctor.vertices_begin(), float4tofloat3()),
               thrust::make_transform_iterator(pistonFunctor.vertices_end(), float4tofloat3()),
               tmp->begin());
  //attributes
  newD->scalars = new thrust::device_vector<float>(newD->nPoints);
  outData->SetScalarsArrayName(inData->GetScalarsArrayName());
  thrust::copy(pistonFunctor.scalars_begin(), pistonFunctor.scalars_end(), newD->scalars->begin());
  newD->normals = new thrust::device_vector<float>(newD->nPoints*3);
  //TODO: FIX UP THIS CAST
  thrust::device_vector<float3>* casted = reinterpret_cast<thrust::device_vector<float3>*>(newD->normals);
  thrust::copy(pistonFunctor.normals_begin(), pistonFunctor.normals_end(), casted->begin());
}

} //namespace
