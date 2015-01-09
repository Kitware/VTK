#ifdef _WIN32
#include <windows.h>
#endif

#include <thrust/copy.h>
#include <thrust/device_vector.h>

#include <cuda_gl_interop.h>

#include "vtkScalarsToColors.h"
#include "vtkPistonDataObject.h"
#include "vtkPistonDataWrangling.h"
#include "vtkPistonScalarsColors.h"
#include "vtkPistonMinMax.h"
#include "vtkPistonReference.h"

#include "vtkgl.h"

#include <iostream>

using namespace std;

namespace vtkpiston {

bool AlmostEqualRelativeAndAbs(float A, float B,
            float maxDiff, float maxRelDiff)
  {
    // Check if the numbers are really close -- needed
    // when comparing numbers near zero.
    float diff = fabs(A - B);
    if (diff <= maxDiff)
        return true;

    A = fabs(A);
    B = fabs(B);
    float largest = (B > A) ? B : A;

    if (diff <= largest * maxRelDiff)
        return true;
    return false;
  }

template <typename ValueType>
struct color_map : thrust::unary_function<ValueType, float3>
{
    const ValueType min;
    const ValueType max;
    const int size;
    float *table;
    const int numberOfChanels;

    color_map(float *rtable, int arrSize, int noOfChanels,
      ValueType rMin, ValueType rMax) :
      min(rMin),
      max(rMax),
      size((arrSize / noOfChanels) - 1),
      table(rtable),
      numberOfChanels(noOfChanels)
      {
      }

    __host__ __device__
    float3 operator()(ValueType val)
    {
      int index = 0;
      if((max - min) > 0.0)
        {
        index = ( (val - min) / (max - min) ) * size;
        }

      if (index < 0) index = 0;
      if (index > size) index = size;
      index *= numberOfChanels;

      float3 color;
      if(numberOfChanels == 1)
        {
        color = make_float3(table[index], table[index], table[index]);
        }
      else if(numberOfChanels == 2)
        {
        color = make_float3(table[index], table[index + 1], 0.0f);
        }
      else if(numberOfChanels == 3)
        {
        color = make_float3(table[index], table[index + 1], table[index + 2]);
        }
      else
        {
        // Not supported
        }

      return color;
    }
};

//------------------------------------------------------------------------------
void CudaGLInit()
{
  cudaDeviceProp prop;
  int dev;

  // Fill it with zeros
  memset(&prop,0,sizeof(cudaDeviceProp));

  // Pick a GPU capable of 1.0 or better
  prop.major=1; prop.minor=0;
  cudaChooseDevice(&dev,&prop);

  // Set OpenGL device
  cudaError_t res = cudaGLSetGLDevice(dev);

  if (res != cudaSuccess)
    {
    cerr << "Set device failed  ... " << cudaGetErrorString(res) << endl;
    return;
    }
}

//------------------------------------------------------------------------------
void CudaRegisterBuffer(struct cudaGraphicsResource **vboResource,
                        GLuint vboBuffer)
{
  cudaError_t res =
    cudaGraphicsGLRegisterBuffer(vboResource, vboBuffer,
                                cudaGraphicsMapFlagsWriteDiscard);
  if (res != cudaSuccess)
  {
    cerr << "Register buffer failed ... " << cudaGetErrorString(res) << endl;
    return;
  }
}

//------------------------------------------------------------------------------
void CudaUnregisterResource(struct cudaGraphicsResource *vboResource)
{
  cudaError_t res = cudaGraphicsUnregisterResource(vboResource);
  if (res != cudaSuccess)
  {
    cerr << "Unregister buffer failed ... " << cudaGetErrorString(res) << endl;
    return;
  }
}

//------------------------------------------------------------------------------
void CudaTransferToGL(vtkPistonDataObject *id, unsigned long dataObjectMTimeCache,
                      vtkPistonScalarsColors *psc,
                      cudaGraphicsResource **vboResources,
                      bool &hasNormals, bool &hasColors)
{
  vtkPistonReference *tr = id->GetReference();
  if (tr->type != VTK_POLY_DATA || tr->data == NULL)
    {
    // Type mismatch, don't bother trying
    return;
    }

  vtk_polydata *pD = (vtk_polydata *)tr->data;

  // Claim access to buffer for cuda
  cudaError_t res;
  res = cudaGraphicsMapResources(3, vboResources, 0);
  if (res != cudaSuccess)
  {
    cerr << "Claim for CUDA failed ... " << cudaGetErrorString(res) << endl;
    return;
  }

  size_t num_bytes;
  float *vertexBufferData, *normalsBufferData;
  float3 *colorsBufferData;
  res = cudaGraphicsResourceGetMappedPointer
      ((void **)&vertexBufferData, &num_bytes, vboResources[0]);
  if(res != cudaSuccess)
  {
    cerr << "Get mappedpointer for vertices failed ... "
         << cudaGetErrorString(res) << endl;
    return;
  }
  res = cudaGraphicsResourceGetMappedPointer
      ((void **)&normalsBufferData, &num_bytes, vboResources[1]);
  if(res != cudaSuccess)
  {
    cerr << "Get mappedpointer for normals failed ... "
         << cudaGetErrorString(res) << endl;
    return;
  }
  res = cudaGraphicsResourceGetMappedPointer
      ((void **)&colorsBufferData, &num_bytes, vboResources[2]);
  if(res != cudaSuccess)
  {
    cerr << "Get mappedpointer for colors failed ... "
         << cudaGetErrorString(res) << endl;
    return;
  }

  // Copy on card verts to the shared on card gl buffer
  thrust::copy(pD->points->begin(), pD->points->end(),
               thrust::device_ptr<float>(vertexBufferData));

  hasNormals = false;
  if (pD->normals)
    {
    hasNormals = true;

    // Copy on card verts to the shared on card gl buffer
    thrust::copy(pD->normals->begin(), pD->normals->end(),
                 thrust::device_ptr<float>(normalsBufferData));
    }
  hasColors = false;

  if (pD->scalars)
    {
    double scalarRange[2];
    id->GetScalarsRange(scalarRange);

    hasColors = true;

    if(id->GetMTime() > dataObjectMTimeCache)
      {
      vtkPiston::minmax_pair<float> result = vtkPiston::find_min_max(
                                              pD->scalars);

      scalarRange[0] = static_cast<double>(result.min_val);
      scalarRange[1] = static_cast<double>(result.max_val);

      // Set parameters to compute scalars colors
      const int numvalues = 256;
      id->SetScalarsRange(scalarRange);
      psc->SetTableRange(scalarRange[0], scalarRange[1]);
      psc->SetNumberOfValues(numvalues);
      }

    std::vector<float> *colors = psc->ComputeScalarsColorsf(VTK_RGB);

    // Copy to GPU
    thrust::device_vector<float> onGPU(colors->begin(), colors->end());
    float *raw_ptr = thrust::raw_pointer_cast(&onGPU[0]);

    // Now run each scalar data through the map to choose a color for it

    // \NOTE: Since GPU most likely going to calculate range using single
    // floating point precision, we may lose precision and hence, we need
    // to check if the range min and max are almost equal
    //TODO: Remove this when piston gives us exactly same values for
    //isocontour.
    float tempRange[2] =
      {
      static_cast<float>(scalarRange[0]),
      static_cast<float>(scalarRange[1])
      };
    if( AlmostEqualRelativeAndAbs(scalarRange[0], scalarRange[1],
                                  numeric_limits<float>::epsilon(),
                                  numeric_limits<float>::epsilon() * 10) )
      {
      tempRange[1] = tempRange[0]+1.0;
      }

    color_map<float> colorMap(raw_ptr, onGPU.size(), VTK_RGB, tempRange[0], tempRange[1]);
    thrust::copy(thrust::make_transform_iterator(pD->scalars->begin(),
                                                 colorMap),
                 thrust::make_transform_iterator(pD->scalars->end(), colorMap),
                 thrust::device_ptr<float3>(colorsBufferData));
    }

  // Allow GL to access again
  res = cudaGraphicsUnmapResources(3, vboResources, 0);
  if (res != cudaSuccess)
  {
    cerr << "Release from CUDA failed ... " << cudaGetErrorString(res) << endl;
    return;
  }

  return;
}

} //namespace
