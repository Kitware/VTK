#include <thrust/copy.h>
#include <thrust/device_vector.h>
#include <thrust/sort.h>
#include <piston/image3d.h>
#include <piston/vtk_image3d.h>
#include "vtkPistonDataObject.h"
#include "vtkPistonDataWrangling.h"
#include "vtkPistonReference.h"
#include <iostream>

using namespace std;
using namespace piston;

namespace vtkpiston {

void ExecutePistonSort(vtkPistonDataObject *inData, vtkPistonDataObject *outData)
{
  vtkPistonReference *ti = inData->GetReference();
  if (ti->type != VTK_IMAGE_DATA || ti->data == NULL)
    {
    //type mismatch, don't bother trying
    return;
    }
  vtk_image3d</*int, float,*/ SPACE>*gpuData = (vtk_image3d</*int, float,*/ SPACE>*)ti->data;

  thrust::device_vector<float>*scalars = new thrust::device_vector<float>(gpuData->NPoints);
  thrust::copy(gpuData->point_data_begin(), gpuData->point_data_end(), scalars->begin());

  //Sort the copy of the data array
  thrust::sort(scalars->begin(), scalars->end());

  // Copy array name
  outData->SetScalarsArrayName(inData->GetScalarsArrayName());

  //get a hold of my output, clean it, and put new results there
  vtkPistonReference *tr = outData->GetReference();
  DeleteData(tr);
  tr->type = VTK_IMAGE_DATA;
  int dims[3];
  dims[0] = gpuData->dim0;
  dims[1] = gpuData->dim1;
  dims[2] = gpuData->dim2;
  vtk_image3d<SPACE> *newD =
    new vtk_image3d<SPACE>(dims, gpuData->origin,
                                       gpuData->spacing,
                                       gpuData->extents, *scalars);

  tr->data = (void*)newD;
}

}//namespace
