/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeShearWarpDataStructure.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkShearWarpBase.h"

#define VTK_X_AXIS  0
#define VTK_Y_AXIS  1
#define VTK_Z_AXIS  2
#define VTK_SHEAR_WARP_COMPOSITE_FUNCTION      0
#define VTK_SHEAR_WARP_MIP_FUNCTION            1
#define VTK_SHEAR_WARP_ISOSURFACE_FUNCTION     2

#define VTK_SHEAR_WARP_OCTREE_TRANSPARENT      0
#define VTK_SHEAR_WARP_OCTREE_NONTRANSPARENT   1
#define VTK_SHEAR_WARP_OCTREE_COMBINATION      2
#define VTK_SHEAR_WARP_OCTREE_MINIMUM_SIZE     16
                                               
// Intermediate image pixel data for early ray termination
class vtkShearWarpPixelData
{
public:
  float Red;
  float Green;
  float Blue;
  float Opacity;
  float Value;
  int Offset;
};


// Runlength encoded intermediate image
class vtkShearWarpRLEImage
{
public:
  vtkShearWarpRLEImage(int size)
  {
    this->PixelData = new vtkShearWarpPixelData[size];
    this->ImageSize = size;
    this->Clear();      
  };

  ~vtkShearWarpRLEImage()
  {
    if (this->PixelData != NULL)
      delete[] this->PixelData;
  };

  // Reset all pixels to default values
  void Clear()
  {
    for (int i = 0; i < this->ImageSize; i++)
    {
      this->PixelData[i].Red = 0.0f;
      this->PixelData[i].Green = 0.0f;
      this->PixelData[i].Blue = 0.0f;
      this->PixelData[i].Opacity = 0.0f;
      this->PixelData[i].Value = 0.0f;
      this->PixelData[i].Offset = 0;
    }
  };

  // Reset the current pixel pointer to the first pixel
  void First(vtkShearWarpPixelData * &ptr)
  {
    ptr = this->PixelData;
  };

  // Sets the current pixel pointer to the specified position
  void Position (vtkShearWarpPixelData * &ptr, int position)
  {
    ptr = this->PixelData + position;
  }; 

  // Advances the current pixel pointer by the specified increment
  void Advance(vtkShearWarpPixelData * &ptr, int count)
  {
    ptr = ptr + count;
  };

  // Skip over transparent voxels and returns the count of skipped voxels
  int Skip(vtkShearWarpPixelData * &ptr)
  {
    vtkShearWarpPixelData *data = ptr;
    int runLength = 0;
    int pathLength = 0;
    int offset = 0;
    
    while (ptr->Offset > 0)
    {
      runLength += ptr->Offset;
      ptr += ptr->Offset;
    }

    data->Offset = runLength;

    while  (data->Offset > 0)
    {
      offset = data->Offset;
      data->Offset = runLength - pathLength;
      pathLength += offset;
      data += offset;
    }
      
    return runLength;
  };
        
  // Retrieves a pointer to the first pixel
  vtkShearWarpPixelData * GetPixelData()
  {
    return this->PixelData;
  };

  // Retrieves the allocated image size
  int GetSize()
  {
    return this->ImageSize;
  };

private:
  // the pixel data
  vtkShearWarpPixelData *PixelData;

  // the allocated image size
  int ImageSize;

};


// Voxel data for runlength encoding, contains the scalar value and shading information
template <class T>
struct vtkShearWarpVoxelData
{
public:
  T Value;
  unsigned short EncodedNormal;
  unsigned char GradientMagnitude;  
};



// An RLE run. It has a length and a pointer to the first voxel.
template <class T>
class vtkShearWarpRLERun
{
public:
  unsigned char Length;
  vtkShearWarpVoxelData<T> *VoxelData;
};


// A runlength encoded voxel slice which provides scanline-wise access to the data
template <class T>
class vtkShearWarpRLESlice
{
public:
  vtkShearWarpRLESlice()
  {
    this->SliceRuns = NULL;
    this->VoxelData = NULL;
    this->LineRuns = NULL;
  };

  ~vtkShearWarpRLESlice()
  {
    if (this->LineRuns != NULL)
      delete[] this->LineRuns;

    if (this->SliceRuns != NULL)
      delete[] this->SliceRuns;

    if (this->VoxelData != NULL)
      delete[] this->VoxelData;
  };
  
  
  // Encodes the data by opacity (for alpha compositing)
  void encodeOpacity (vtkImageData* data, vtkVolume *volume, vtkEncodedGradientEstimator *gradest, int axis, int k, float opacityThreshold)
  {
    if (data == NULL || volume == NULL)
      return;

    T voxelScalar = 0;
    float voxelOpacity = 0.0f;
    unsigned char currentIndex = 0;
    int currentRun = 0;
    int location = 0;
    bool transparentRun = false;

    float *SOTF = volume->GetCorrectedScalarOpacityArray();

    T *dptr = (T*) data->GetScalarPointer();
    unsigned short *nptr = gradest->GetEncodedNormals();
    unsigned char *gptr = gradest->GetGradientMagnitudes();

    int i,j;
    
    int jCount = 0;
    int iCount = 0;
    int voxelIndex = 0;

    int *dimensions = data->GetDimensions();
    int plane = dimensions[0] * dimensions[1];
    int iIncrement,jIncrement;
    int vi,vj,vk;

    switch (axis)
    {
      case VTK_X_AXIS:
        iCount = dimensions[1];
        jCount = dimensions[2];
        vk = k;
        iIncrement = dimensions[0];
        jIncrement = plane;
        break;

      case VTK_Y_AXIS:
        iCount = dimensions[2];
        jCount = dimensions[0];
        vk = k*dimensions[0];
        iIncrement = plane;
        jIncrement = 1;
        break;

      case VTK_Z_AXIS:
      default:
        iCount = dimensions[0];
        jCount = dimensions[1];
        vk = k*plane;
        iIncrement = 1;
        jIncrement = dimensions[0];
        break;
    }
 
    // First we determine the number of runs in the slice
    for (j=0,vj=0; j<jCount;j++, vj += jIncrement)
    {
      for (i=0,vi=0; i<iCount;i++, vi += iIncrement)
      {
        location = vi + vj + vk;
        voxelScalar = dptr[location];        
        voxelOpacity = SOTF[voxelScalar];

        if (voxelOpacity > opacityThreshold)
        {
          if (!transparentRun && i > 0 && currentIndex < 254)
          {
            currentIndex++;
          }
          else
          {
            currentIndex = 0;
            currentRun++;
            transparentRun = false;
          }

          voxelIndex++;
          
        }
        else
        {
          if (transparentRun && i > 0 && currentIndex < 254)
          {
            currentIndex++;
          }
          else
          {
            currentIndex = 0;
            currentRun++;
            transparentRun = true;
          }
        }
      }
    }

    this->LineRuns = new vtkShearWarpRLERun<T>*[jCount];
    this->SliceRuns = new vtkShearWarpRLERun<T>[currentRun];
    this->VoxelData = new vtkShearWarpVoxelData<T>[voxelIndex];

    vtkShearWarpRLERun<T> *activeRun = &this->SliceRuns[0];

    voxelIndex = 0;
    currentRun = 0;

    // Now we run-length-encode the slice
    for (j=0,vj=0; j<jCount;j++, vj += jIncrement)
    {
      this->LineRuns[j] = activeRun;
      currentIndex = 0;

      for (i=0,vi=0; i<iCount;i++, vi += iIncrement)
      {
        location = vi + vj + vk;        
        voxelScalar = dptr[location];        
        voxelOpacity = SOTF[voxelScalar];

        if (voxelOpacity > opacityThreshold)
        {
         
          if (!transparentRun && i > 0 && currentIndex < 254)
          {
            currentIndex++;
          }
          else
          {
            if (i > 0)
            {
              activeRun->Length = currentIndex + 1;
              activeRun++;
              currentRun++;
              currentIndex = 0;
            }
                                                 
            activeRun->VoxelData = &this->VoxelData[voxelIndex];
            transparentRun = false;
          }

          // Set voxel data
          this->VoxelData[voxelIndex].Value = voxelScalar;
          this->VoxelData[voxelIndex].EncodedNormal = nptr[location];
          this->VoxelData[voxelIndex].GradientMagnitude = gptr[location];
          
          voxelIndex++;
        }
        else
        {
          if (transparentRun && i > 0 && currentIndex < 254)
          {
            currentIndex++;
          }
          else
          {
            if (i > 0)
            {
              activeRun->Length = currentIndex + 1;
              activeRun++;
              currentRun++;
              currentIndex = 0;
            }
              
            activeRun->VoxelData = NULL;
            transparentRun = true;
          }
        }
      }

      activeRun->Length = currentIndex + 1;
      activeRun++;
      currentRun++;

    }                                                                                                  
  };

  // Encodes the data by scalar value (for isosurface display)
  void encodeScalar (vtkImageData* data, vtkVolume *volume, vtkEncodedGradientEstimator *gradest, int axis, int k, float isoValue)
  {
    if (data == NULL || volume == NULL)
      return;
    
    T voxelScalar = 0;
    unsigned char currentIndex = 0;
    int currentRun = 0;
    int location = 0;
    bool transparentRun = false;

    T *dptr = (T*) data->GetScalarPointer();
    unsigned short *nptr = gradest->GetEncodedNormals();
    unsigned char *gptr = gradest->GetGradientMagnitudes();

    int i,j;

    int jCount = 0;
    int iCount = 0;
    int voxelIndex = 0;

    int *dimensions = data->GetDimensions();
    int plane = dimensions[0] * dimensions[1];
    int iIncrement,jIncrement;
    int vi,vj,vk;

    switch (axis)
    {
      case VTK_X_AXIS:
        iCount = dimensions[1];
        jCount = dimensions[2];
        vk = k;
        iIncrement = dimensions[0];
        jIncrement = plane;
        break;

      case VTK_Y_AXIS:
        iCount = dimensions[2];
        jCount = dimensions[0];
        vk = k*dimensions[0];
        iIncrement = plane;
        jIncrement = 1;
        break;

      case VTK_Z_AXIS:
      default:
        iCount = dimensions[0];
        jCount = dimensions[1];
        vk = k*plane;
        iIncrement = 1;
        jIncrement = dimensions[0];        
        break;
    }

    // First we determine the number of runs in the slice
    for (j=0,vj=0; j<jCount;j++, vj += jIncrement)
    {
      for (i=0,vi=0; i<iCount;i++, vi += iIncrement)
      {
        location = vi + vj + vk;
        voxelScalar = dptr[location];

        if (voxelScalar >= isoValue)
        {
          if (!transparentRun && i > 0 && currentIndex < 254)
          {
            currentIndex++;
          }
          else
          {
            currentIndex = 0;
            currentRun++;
            transparentRun = false;
          }

          voxelIndex++;

        }
        else
        {
          if (transparentRun && i > 0 && currentIndex < 254)
          {
            currentIndex++;
          }
          else
          {
            currentIndex = 0;
            currentRun++;
            transparentRun = true;
          }
        }
      }
    }

    this->LineRuns = new vtkShearWarpRLERun<T>*[jCount];
    this->SliceRuns = new vtkShearWarpRLERun<T>[currentRun];
    this->VoxelData = new vtkShearWarpVoxelData<T>[voxelIndex];

    vtkShearWarpRLERun<T> *activeRun = this->SliceRuns;

    voxelIndex = 0;
    currentRun = 0;

    // Now we run-length-encode the slice
    for (j=0,vj=0; j<jCount;j++, vj += jIncrement)
    {
      this->LineRuns[j] = activeRun;
      currentIndex = 0;

      for (i=0,vi=0; i<iCount;i++, vi += iIncrement)
      {
        location = vi + vj + vk;
        voxelScalar = dptr[location];

        if (voxelScalar >= isoValue)
        {
          if (!transparentRun && i > 0 && currentIndex < 254)
          {
            currentIndex++;
          }
          else
          {
            if (i > 0)
            {
              activeRun->Length = currentIndex + 1;
              activeRun++;
              currentRun++;
              currentIndex = 0;
            }

            activeRun->VoxelData = &this->VoxelData[voxelIndex];
            transparentRun = false;
          }

          // Set voxel data
          this->VoxelData[voxelIndex].Value = voxelScalar;
          this->VoxelData[voxelIndex].EncodedNormal = nptr[location];
          this->VoxelData[voxelIndex].GradientMagnitude = gptr[location];

          voxelIndex++;
        }
        else
        {
          if (transparentRun && i > 0 && currentIndex < 254)
          {
            currentIndex++;
          }
          else
          {
            if (i > 0)
            {
              activeRun->Length = currentIndex + 1;
              activeRun++;
              currentRun++;
              currentIndex = 0;
            }

            activeRun->VoxelData = NULL;
            transparentRun = true;
          }
        }
      }

      activeRun->Length = currentIndex + 1;
      activeRun++;
      currentRun++;
    }
  };

  // Returns a pointer to the first run of a specified scanline
  vtkShearWarpRLERun<T> * GetLineRuns(int line)
  {
    return this->LineRuns[line];
  }
  
private:
  // pointers to the first run for every scanline
  vtkShearWarpRLERun<T> **LineRuns;

  // all runs of the slice
  vtkShearWarpRLERun<T> *SliceRuns;

  // the voxel data of the slice
  vtkShearWarpVoxelData<T> *VoxelData;
          
};

// Base class for encoded volume
class vtkShearWarpBase
{
public:
  vtkShearWarpBase()
  {
    this->VolumeDimensions[0] = 0;
    this->VolumeDimensions[1] = 0;
    this->VolumeDimensions[2] = 0;
  };
  
  virtual ~vtkShearWarpBase()
  {
  };
  
  // Returns the volume dimensions
  int * GetDimensions()
  {
    return this->VolumeDimensions;
  };

  // Returns the encoded isovalue, if the volume is scalar encoded
  float GetIsoValue()
  {
    return this->IsoValue;
  };

  // Returns true if the volume is opacity encoded
  bool IsOpacityEncoded()
  {
    return (this->OpacityEncoded == 1);
  };
  
  // Returns true if the volume is scalar encoded
  bool IsScalarEncoded()
  {
    return (!this->OpacityEncoded && this->IsoValue >= 0.0);
  }          

protected:
  // the volume dimensions
  int VolumeDimensions[3];

  // the encoded isovalue
  float IsoValue;

  // encoding type flag
  int OpacityEncoded;    
};


// A runlength encoded volume. It contains voxel data encoded for each major viewing direction.
template <class T>
class VTK_RENDERING_EXPORT vtkShearWarpRLEVolume : public vtkShearWarpBase
{
public:
  vtkShearWarpRLEVolume()
  {
    for (int l=0;l<3;l++)
      this->EncodedSlices[l] = NULL;

    this->OpacityEncoded = 0;
    this->IsoValue = -1.0f;
  };

  virtual ~vtkShearWarpRLEVolume()
  {
    for (int l=0;l<3;l++)
      if (this->EncodedSlices[l] != NULL)
        delete[] this->EncodedSlices[l];
  };

  // Encodes the volume by opacity (for alpha-compositing)
  void encodeOpacity(vtkImageData *data, vtkVolume *volume, vtkEncodedGradientEstimator* gradest, float opacityThreshold)
  {
    int l;
    this->IsoValue = -1.0f;
    this->OpacityEncoded = 1;
    
    for (l=0;l<3;l++)
      {
      if (this->EncodedSlices[l] != NULL)
        {
        delete[] this->EncodedSlices[l];
        }
      }
    
    int *dimensions = data->GetDimensions();    
    this->Volume = volume;
    this->VolumeDimensions[0] = dimensions[0];
    this->VolumeDimensions[1] = dimensions[1];
    this->VolumeDimensions[2] = dimensions[2];

    for (l=0; l<3; l++)
    {
      this->EncodedSlices[l] = new vtkShearWarpRLESlice<T>[dimensions[l]];
      
      for (int k = 0; k < dimensions[l]; k++)
        {
        this->EncodedSlices[l][k].encodeOpacity(data,volume,gradest,l,k,opacityThreshold);
        }
    }
  };

  // Encodes the volume by scalar (for isosurface display)
  void encodeScalar(vtkImageData *data, vtkVolume *volume, vtkEncodedGradientEstimator* gradest, float isoValue)
  {
    int l;
    this->IsoValue = isoValue;
    this->OpacityEncoded = 0;    
    
    for (l=0;l<3;l++)
      {
      if (this->EncodedSlices[l] != NULL)
        {
        delete[] this->EncodedSlices[l];
        }
      }

    int *dimensions = data->GetDimensions();
    this->Volume = volume;
    this->VolumeDimensions[0] = dimensions[0];
    this->VolumeDimensions[1] = dimensions[1];
    this->VolumeDimensions[2] = dimensions[2];

    for (l=0; l<3; l++)
    {
      this->EncodedSlices[l] = new vtkShearWarpRLESlice<T>[dimensions[l]];

      for (int k = 0; k < dimensions[l]; k++)
        {
        this->EncodedSlices[l][k].encodeScalar(data,volume,gradest,l,k,isoValue);
        }
    }
  };

  // Returns the slice
  vtkShearWarpRLESlice<T> * GetSlice(int axis, int slice)
  {
    return &this->EncodedSlices[axis][slice];
  };

  // Returns a pointer to the source volume
  vtkVolume * GetVolume()
  {
    return this->Volume;
  };


private:
  // the encoded slices for all three principal axes
  vtkShearWarpRLESlice<T> *EncodedSlices[3];

  // the source volume
  vtkVolume *Volume;

};

template <class T>
class vtkShearWarpSummedAreaTable
{
public:
  vtkShearWarpSummedAreaTable()
  {
    this->Table = new float[2 << ((sizeof(T)*8)-1)];
    this->Opacity = NULL;
  };

  ~vtkShearWarpSummedAreaTable()
  {
    if (this->Table)
      delete[] this->Table;
  };

  void build(float *SOTF, T upper)
  {
    this->Table[0] = SOTF[0];
    this->Opacity = SOTF;

    for (int i=1;i<=upper;i++)
    {
      this->Table[i] = this->Table[i-1] + SOTF[i];
    }    
  };

  float integrate(T min, T max)
  {
    if (min != max)
      return this->Table[max] - this->Table[min];
    else
      return this->Opacity[min];
    
  };

private:
  float *Table;
  float *Opacity;
};

struct vtkShearWarpOctreeRun
{
  unsigned short Length;
  unsigned char Type;
};

template <class T>
class vtkShearWarpOctreeNode
{
public:
  vtkShearWarpOctreeNode()
  {
    this->Children = NULL;
  };
  
  ~vtkShearWarpOctreeNode()
  {
    if (this->Children != NULL)
      delete[] this->Children;
  };

  T GetMinimum()
  {
    return this->Minimum;
  };

  T GetMaximum()
  {
    return this->Maximum;
  };

  void build(vtkImageData *data, int min[3], int max[3], int level)
  {
//    cout << "minX: " << min[0] << " - minY: " << min[1] << " - minZ: " << min[2] << "\n";
//    cout << "maxX: " << max[0] << " - maxY: " << max[1] << " - maxZ: " << max[2] << "\n\n";
    
    if (this->Children != NULL)
    {
      delete[] this->Children;
      this->Children = NULL;
    }
    
    if (max[0] <= min[0] && max[1] <= min[1] && max[2] <= min[2])
    {
      this->Minimum = *((T*) data->GetScalarPointer(max[0],max[1],max[2]));
      this->Maximum = this->Minimum;
    }
    else
    {
      int center[3] = {(max[0]+min[0]) / 2, (max[1]+min[1]) / 2, (max[2]+min[2]) / 2};
      int newMin[3];
      int newMax[3];
      this->Children = new vtkShearWarpOctreeNode<T>[8];

      newMin[0] = min[0];
      newMin[1] = min[1];
      newMin[2] = min[2];
      newMax[0] = center[0];
      newMax[1] = center[1];
      newMax[2] = center[2];
      this->Children[0].build(data,newMin,newMax,level+1);
      
      newMin[0] = center[0]+1;
      newMin[1] = min[1];
      newMin[2] = min[2];
      newMax[0] = max[0];
      newMax[1] = center[1];
      newMax[2] = center[2];
      this->Children[1].build(data,newMin,newMax,level+1);

      newMin[0] = min[0];
      newMin[1] = center[1]+1;
      newMin[2] = min[2];
      newMax[0] = center[0];
      newMax[1] = max[1];
      newMax[2] = center[2];
      this->Children[2].build(data,newMin,newMax,level+1);
      
      newMin[0] = center[0]+1;
      newMin[1] = center[1]+1;
      newMin[2] = min[2];
      newMax[0] = max[0];
      newMax[1] = max[1];
      newMax[2] = center[2];
      this->Children[3].build(data,newMin,newMax,level+1);
    
      newMin[0] = min[0];
      newMin[1] = min[1];
      newMin[2] = center[2]+1;
      newMax[0] = center[0];
      newMax[1] = center[1];
      newMax[2] = max[2];
      this->Children[4].build(data,newMin,newMax,level+1);

      newMin[0] = center[0]+1;
      newMin[1] = min[1];
      newMin[2] = center[2]+1;
      newMax[0] = max[0];
      newMax[1] = center[1];
      newMax[2] = max[2];
      this->Children[5].build(data,newMin,newMax,level+1);

      newMin[0] = min[0];
      newMin[1] = center[1]+1;
      newMin[2] = center[2]+1;
      newMax[0] = center[0];
      newMax[1] = max[1];
      newMax[2] = max[2];
      this->Children[6].build(data,newMin,newMax,level+1);

      newMin[0] = center[0]+1;
      newMin[1] = center[1]+1;
      newMin[2] = center[2]+1;
      newMax[0] = max[0];
      newMax[1] = max[1];
      newMax[2] = max[2];
      this->Children[7].build(data,newMin,newMax,level+1);

      this->Minimum = this->Children[0].Minimum;
      this->Maximum = this->Children[0].Maximum;

      bool equalMinimum = true;
      bool equalMaximum = true;
      
      for (int i=1; i < 8; i++)
      {
        if (this->Minimum != this->Children[i].Minimum)
        {        
          if (this->Children[i].Minimum < this->Minimum)
            this->Minimum = this->Children[i].Minimum;

          equalMinimum = false;
        }

        if (this->Maximum != this->Children[i].Maximum)
        {
          if (this->Children[i].Maximum > this->Maximum)
            this->Maximum = this->Children[i].Maximum;

          equalMaximum = false;
        }
      }

      // If minimum and maximum of all children are equal, we can remove them
      if (equalMinimum && equalMaximum)
      {
        delete[] this->Children;
        this->Children = NULL;
      }
      else
      {
        // Remove children if node already is at the lowest level
/*        if ((max[0] - min[0] + 1) <= VTK_SHEAR_WARP_OCTREE_MINIMUM_SIZE &&
            (max[1] - min[1] + 1) <= VTK_SHEAR_WARP_OCTREE_MINIMUM_SIZE &&
            (max[2] - min[2] + 1) <= VTK_SHEAR_WARP_OCTREE_MINIMUM_SIZE)*/
        if (level >= 4)
        {
          delete[] this->Children;
          this->Children = NULL;
        }
      }
    }
  };

  void classifyOpacity(vtkShearWarpSummedAreaTable<T> *table)
  {
    float integral = table->integrate(this->Minimum,this->Maximum);

    if (integral == 0.0f)
    {
      this->Status = VTK_SHEAR_WARP_OCTREE_TRANSPARENT;
    }
    else if (this->Children == NULL)
    {
      this->Status = VTK_SHEAR_WARP_OCTREE_NONTRANSPARENT;
    }
    else
    {
      this->Status = VTK_SHEAR_WARP_OCTREE_COMBINATION;

      for (int i = 0; i < 8; i++)
        this->Children[i].classifyOpacity(table);
    }
  };  

  void classifyScalar(T value)
  {
    if (this->Minimum >= value || this->Maximum >= value)
    {
      if (this->Children == NULL)
      {
        this->Status = VTK_SHEAR_WARP_OCTREE_NONTRANSPARENT;      
      }
      else
      {
        this->Status = VTK_SHEAR_WARP_OCTREE_COMBINATION;

        for (int i = 0; i < 8; i++)
          this->Children[i].classifyScalar(value);
      }
    }
    else
    {
      this->Status = VTK_SHEAR_WARP_OCTREE_TRANSPARENT;
    }
  };

  int computeRuns(vtkShearWarpOctreeRun *& runs, int axis, int slices, int lines, int voxels, int slice, int line)
  {
    static const int increments[3][3] = {{2,4,1},{4,1,2},{1,2,4}};
            
    if (this->Status == VTK_SHEAR_WARP_OCTREE_COMBINATION)
    {
      int child = 0;
//      int half = size / 2;
      int halfSlices = slices / 2;
      int halfLines = lines / 2;
      int halfVoxels = voxels / 2;

      if (slice > halfSlices)
      {
        child += increments[axis][2];
        slice -= halfSlices;

        halfSlices = slices - halfSlices;
      }
        
      if (line > halfLines)
      {
        child += increments[axis][1];
        line -= halfLines;

        halfLines = lines - halfLines;
      }

      int a = this->Children[child].computeRuns(runs, axis, halfSlices, halfLines, halfVoxels, slice,line);
      int b = this->Children[child + increments[axis][0]].computeRuns(runs, axis, halfSlices, halfLines, voxels-halfVoxels, slice,line);

      if (a < b)
        return a;
      else
        return b;
    }
    else
    {
      if (runs->Type == this->Status)
      {
        runs->Length += voxels;//size;
      }
      else
      {
        if (runs[0].Type != 255)          
          runs++;

        runs->Type = this->Status;
        runs->Length = voxels;//size;        
      }

      return voxels;//size;
      
    }
  }

private:  
  vtkShearWarpOctreeNode<T> *Children;
  unsigned char Status;
  T Minimum;
  T Maximum;
};

template <class T>
class VTK_RENDERING_EXPORT vtkShearWarpOctree : public vtkShearWarpBase
{
public:
  vtkTypeRevisionMacro(vtkShearWarpOctree,vtkShearWarpBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkShearWarpOctree()
  {
  };

  virtual ~vtkShearWarpOctree()
  {
    
  };

  void build(vtkImageData* data)
  {
    int min[3],max[3];
    data->GetDimensions(this->Dimensions);
    data->GetExtent(min[0],max[0],min[1],max[1],min[2],max[2]);
    this->Root.build(data,min,max,0);
  };

  void classifyOpacity(vtkVolume* volume)
  {
    this->Table.build(volume->GetCorrectedScalarOpacityArray(),this->Root.GetMaximum());
    this->Root.classifyOpacity(&this->Table);
    this->OpacityEncoded = 1;
  };

  void classifyScalar(T value)
  {
    this->Root.classifyScalar(value);
    this->OpacityEncoded = 0;
  };

  int GetLineRuns(vtkShearWarpOctreeRun *runs, int axis, int slice, int line)
  {
    static const int sizes[3][3] = {{this->Dimensions[1],this->Dimensions[2],this->Dimensions[0]},
                                    {this->Dimensions[2],this->Dimensions[0],this->Dimensions[1]},
                                    {this->Dimensions[0],this->Dimensions[1],this->Dimensions[2]}};
        
    runs[0].Type = 255;
    runs[0].Length = 0;
    return this->Root.computeRuns(runs,axis,/*this->Dimensions[axis],*/sizes[axis][2],sizes[axis][1],sizes[axis][0],slice,line);
  };
  
private:
  vtkShearWarpOctreeNode<T> Root;
  vtkShearWarpSummedAreaTable<T> Table;
  int Dimensions[3];
  
  vtkShearWarpOctree(const vtkShearWarpOctree&);  // Not implemented.
  void operator=(const vtkShearWarpOctree&);  // Not implemented.
};
