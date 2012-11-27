#include <thrust/device_vector.h>
#include <thrust/copy.h>
#include "vtkgl.h"
#include <piston/threshold_geometry.h>
#include <piston/vtk_image3d.h>
#include <iostream>
#include "vtkPistonDataObject.h"
#include "vtkPistonDataWrangling.h"
#include "vtkPistonReference.h"

using namespace std;
using namespace piston;

namespace vtkpiston {

// execution method found in vtkPistonThreshold.cu
void ExecutePistonThreshold(vtkPistonDataObject *inData,
                            float minvalue, float maxvalue,
                            vtkPistonDataObject *outData)
{
  vtkPistonReference *ti = inData->GetReference();
  if (ti->type != VTK_IMAGE_DATA || ti->data == NULL)
    {
    //cerr << "NVM" << endl;
    //type mismatch, don't bother trying
    return;
    }
  vtk_image3d<SPACE>*gpuData = (vtk_image3d<SPACE>*)ti->data;
  threshold_geometry<vtk_image3d<SPACE> > pistonFunctor(*gpuData, minvalue, maxvalue);

  // execute the PISTON filter
  pistonFunctor();

  vtkPistonReference *to = outData->GetReference();
  DeleteData(to);

  to->type = VTK_POLY_DATA;
  vtk_polydata *newD = new vtk_polydata;
  to->data = (void*)newD;
  //geometry
  newD->nPoints = pistonFunctor.vertices_indices.size();
  newD->vertsPer = 4; //this piston filter produces quads
  newD->points = new thrust::device_vector<float>(newD->nPoints*3);
  thrust::device_vector<float3> *tmp = (thrust::device_vector<float3> *)newD->points;
  thrust::copy(thrust::make_transform_iterator(pistonFunctor.vertices_begin(), float4tofloat3()),
               thrust::make_transform_iterator(pistonFunctor.vertices_end(), float4tofloat3()),
               tmp->begin());
  //attributes
  newD->scalars = new thrust::device_vector<float>(newD->nPoints);
  thrust::copy(pistonFunctor.scalars_begin(), pistonFunctor.scalars_end(), newD->scalars->begin());
  outData->SetScalarsArrayName(inData->GetScalarsArrayName());
  newD->normals = new thrust::device_vector<float>(newD->nPoints*3);
  //TODO: FIX UP THIS CAST
  thrust::device_vector<float3>* casted = reinterpret_cast<thrust::device_vector<float3>*>(newD->normals);
  thrust::copy(pistonFunctor.normals_begin(), pistonFunctor.normals_end(), casted->begin());
}

} //namespace
