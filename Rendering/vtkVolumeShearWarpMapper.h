/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeShearWarpMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVolumeShearWarpMapper - Abstract class for a Shear Warp Volume Mapper

// .SECTION Description
// vtkVolumeShearWarpMapper is a base class for volume mappers using
// the shear-warp factorization algorithm.

// .SECTION see also
// vtkVolumeMapper, vtkOpenGLVolumeShearWarpMapper

// .SECTION Thanks
// Thanks to Stefan Bruckner for developing and contributing this code
// and to Namkug Kim for some fixing and tidying of the code

// .SECTION References
// P. Lacroute. "Fast Volume Rendering Using a Shear-
// Warp Factorization of the Viewing Transformation"
// PhD thesis, Stanford University, 1995.
//
// P. Lacroute and M. Levoy. "Fast volume rendering using
// a shear-warp factorization of the viewing transformation"
// Proceedings of the 21st annual conference
// on Computer graphics and interactive techniques,
// pages 451–458, 1994.
//
// "The InverseWarp: Non-Invasive Integration of Shear-Warp
//  Volume Rendering into Polygon Rendering Pipelines"
// Stefan Bruckner, Dieter Schmalstiegy, Helwig Hauserz,
// M. Eduard Groller

#ifndef __vtkVolumeShearWarpMapper_h
#define __vtkVolumeShearWarpMapper_h

#include "vtkVolumeMapper.h"

class vtkEncodedGradientShader;
class vtkEncodedGradientEstimator;

class vtkStructuredPoints;
class vtkCamera;
class vtkVolume;
class vtkImageData;
class vtkVolumeProperty;
class vtkPiecewiseFunction;
class vtkTransform;
class vtkMatrix4x4;

class vtkRenderer;
class vtkRenderWindow;

//#include "vtkVolumeShearWarpDataStructure.h"

class vtkShearWarpPixelData;
class vtkShearWarpRLEImage;
struct vtkShearWarpVoxelData;
class vtkShearWarpRLERun;
class vtkShearWarpRLESlice;
class vtkShearWarpBase;
class vtkShearWarpRLEVolume;
class vtkShearWarpSummedAreaTable;
struct vtkShearWarpOctreeRun;
class vtkShearWarpOctreeNode;
class vtkShearWarpOctree;

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
                                               

class VTK_RENDERING_EXPORT vtkVolumeShearWarpMapper : public vtkVolumeMapper
{
//BTX
  template <class T>
  friend void CompositeIntermediateNearestSimple(vtkShearWarpRLEImage *image, vtkVolumeShearWarpMapper *myThis);

  template <class T>
  friend void CompositeIntermediateLinearSimple(vtkShearWarpRLEImage *image, vtkVolumeShearWarpMapper *myThis);
  
  template <class T>
  friend void CompositeIntermediateNearestRLE(vtkShearWarpRLEImage *image, vtkVolumeShearWarpMapper *myThis);

  template <class T>
  friend void CompositeIntermediateLinearRLE(vtkShearWarpRLEImage *image, vtkVolumeShearWarpMapper *myThis);

  template <class T>
  friend void CompositeIntermediateNearestUnclassified(vtkShearWarpRLEImage *image, vtkVolumeShearWarpMapper *myThis);

  template <class T>
  friend void CompositeIntermediateLinearUnclassified(vtkShearWarpRLEImage *image, vtkVolumeShearWarpMapper *myThis);

  template <class T>
  friend void CompositeIntermediateLinearRLEPerspective(vtkShearWarpRLEImage *image, vtkVolumeShearWarpMapper *myThis);
//ETX

public:
  static vtkVolumeShearWarpMapper *New();  
  vtkTypeRevisionMacro(vtkVolumeShearWarpMapper,vtkVolumeMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Update the volume rendering pipeline by updating the scalar input
  virtual void Update();

  // Description:
  // Set / Get the gradient estimator used to estimate normals
  void SetGradientEstimator( vtkEncodedGradientEstimator *gradest );
  vtkGetObjectMacro( GradientEstimator, vtkEncodedGradientEstimator );

  // Description:
  // Get the gradient shader.
  vtkGetObjectMacro( GradientShader, vtkEncodedGradientShader );
  
  // Description:
  // Set/Get the value of IsoValue.
  vtkSetMacro( IsoValue, float );
  vtkGetMacro( IsoValue, float );

  // Description:
  // Enable/Disable runlength encoding
  vtkSetMacro(RunlengthEncoding,int);
  vtkGetMacro(RunlengthEncoding,int);
  vtkBooleanMacro(RunlengthEncoding,int);

  // Description:
  // Enable/Disable classification optimization (fast classification)
  vtkSetMacro(FastClassification,int);
  vtkGetMacro(FastClassification,int);
  vtkBooleanMacro(FastClassification,int);

   // Description:
  // Enable/Disable ParallelProjection or PerspectiveProjection
  vtkSetMacro(ParallelProjection,int);
  vtkGetMacro(ParallelProjection,int);
  vtkBooleanMacro(ParallelProjection, int);
  
  vtkSetMacro(MyPerspectiveProjection,int);
  vtkGetMacro(MyPerspectiveProjection,int);
  vtkBooleanMacro(MyPerspectiveProjection, int);

  // Description:
  // Set the compositing function type.
  vtkSetClampMacro( FunctionType, int,
        VTK_SHEAR_WARP_COMPOSITE_FUNCTION, VTK_SHEAR_WARP_ISOSURFACE_FUNCTION);
  vtkGetMacro(FunctionType,int);
  void SetFunctionTypeToComposite()
        {this->SetFunctionType(VTK_SHEAR_WARP_COMPOSITE_FUNCTION);};
  void SetFunctionTypeToMIP()
        {this->SetFunctionType(VTK_SHEAR_WARP_MIP_FUNCTION);};
  void SetFunctionTypeToIsosurface()
        {this->SetFunctionType(VTK_SHEAR_WARP_ISOSURFACE_FUNCTION);};

  int Debug;

  // Description:
  // Sampling distance. Default value of 1 meaning that every voxel is being
  // processed, 2 causes every second voxel to be processed, etc.
  vtkSetClampMacro( ImageSampleDistance, int, 1, 32 );
  vtkGetMacro( ImageSampleDistance, int );

  // Description:
  // This is the minimum image sample distance allow when the image
  // sample distance is being automatically adjusted
  vtkSetClampMacro( MinimumImageSampleDistance, int, 1, 32 );
  vtkGetMacro( MinimumImageSampleDistance, int );

  // Description:
  // This is the maximum image sample distance allow when the image
  // sample distance is being automatically adjusted
  vtkSetClampMacro( MaximumImageSampleDistance, int, 1, 32 );
  vtkGetMacro( MaximumImageSampleDistance, int );

  // Description:
  // If AutoAdjustSampleDistances is on, the the ImageSampleDistance
  // will be varied to achieve the allocated render time of this
  // prop (controlled by the desired update rate and any culling in
  // use).
  vtkSetClampMacro( AutoAdjustSampleDistances, int, 0, 1 );
  vtkGetMacro( AutoAdjustSampleDistances, int );
  vtkBooleanMacro( AutoAdjustSampleDistances, int );

  // Description:
  // If IntermixIntersectingGeometry is turned on, the zbuffer will be
  // captured and used to limit the traversal of the rays.
  vtkSetClampMacro( IntermixIntersectingGeometry, int, 0, 1 );
  vtkGetMacro( IntermixIntersectingGeometry, int );
  vtkBooleanMacro( IntermixIntersectingGeometry, int );
                               
//BTX
  
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Render the volume
  virtual void Render(vtkRenderer *ren, vtkVolume *vol);

//ETX

protected:
  vtkVolumeShearWarpMapper();
  ~vtkVolumeShearWarpMapper();

  // Objects / variables  needed for shading / gradient magnitude opacity
  vtkEncodedGradientEstimator  *GradientEstimator;
  vtkEncodedGradientShader     *GradientShader;
  int                          Shade;

  float          *RedDiffuseShadingTable;
  float          *GreenDiffuseShadingTable;
  float          *BlueDiffuseShadingTable;
  float          *RedSpecularShadingTable;
  float          *GreenSpecularShadingTable;
  float          *BlueSpecularShadingTable;

  unsigned short *EncodedNormals;
  unsigned char  *GradientMagnitudes;

  vtkTransform *PerspectiveTransform;
  vtkMatrix4x4 *PerspectiveMatrix;
  vtkMatrix4x4 *ViewToWorldMatrix;
  vtkMatrix4x4 *ViewToVoxelsMatrix;
  vtkMatrix4x4 *VoxelsToViewMatrix;
  vtkMatrix4x4 *WorldToVoxelsMatrix;
  vtkMatrix4x4 *VoxelsToWorldMatrix;
  vtkMatrix4x4 *VoxelTransformMatrix;
  vtkMatrix4x4 *ViewportMatrix;
  vtkMatrix4x4 *ShearMatrix;
  vtkMatrix4x4 *WarpMatrix;
  vtkMatrix4x4 *PermutationMatrix;
  vtkMatrix4x4 *PermutedViewToVoxelsMatrix;
  vtkMatrix4x4 *PermutedVoxelsToViewMatrix;

  int           IntermixIntersectingGeometry;
  float        *ZBuffer;
  float        *IntermediateZBuffer;  
  int           ZBufferSize[2];
  int           ZBufferOrigin[2];
  float         MinimumViewDistance;
  

  vtkVolume *Volume;
  vtkShearWarpBase *EncodedVolume;
  vtkShearWarpBase *Octree;
  vtkShearWarpRLEImage *IntemediateImage;
  unsigned char *ImageData;

  int ImageWidth;
  int ImageHeight;
  int AllocatedSize;
  
  unsigned long ScalarOpacityMTime;
  int FunctionType;
  float IsoValue;
  int RunlengthEncoding;
  int FastClassification;

  int CountI;
  int CountJ;
  int CountK;
    
  int ReverseOrder;
  int MajorAxis;
  int ParallelProjection;
  int MyPerspectiveProjection;

  int IntermediateWidth;
  int IntermediateHeight;
  int MaximumIntermediateDimension;

  float ShearI;
  float ShearJ;
  float TranslationI;
  float TranslationJ;
  float Scale;

  // Depth cueing stuff
  /*
  float DepthI;
  float DepthJ;
  float DepthK;
  float Depth0;
  float DeltaDepth;
  float FrontDepth;
  float DepthDensity;
  float DepthTable[512];
  int DepthTableSize;
  */
  float ClippingPlane[4*6];
  int ClippingPlaneCount;

  // This is how big the image would be if it covered the entire viewport
  int ImageViewportSize[2];
    
  double WorldViewingDirection[4];
  double ObjectViewingDirection[4];
  double StandardViewingDirection[4];

  double WorldEyePosition[4];
  double ObjectEyePosition[4];
  double StandardEyePosition[4];

  // The distance between sample points along the ray
  int                          ImageSampleDistance;
  int                          MinimumImageSampleDistance;
  int                          MaximumImageSampleDistance;
  int                          AutoAdjustSampleDistances;

  float        *RenderTimeTable;
  vtkVolume   **RenderVolumeTable;
  vtkRenderer **RenderRendererTable;
  int           RenderTableSize;
  int           RenderTableEntries;

  void StoreRenderTime( vtkRenderer *ren, vtkVolume *vol, float t );
  float RetrieveRenderTime( vtkRenderer *ren, vtkVolume *vol );

  void ComputeMatrices( vtkImageData *data, vtkVolume *vol );
  void FactorViewMatrix();
  void ComputeViewMatrix();
  void ComputeViewportMatrix();
  void ComputePrincipalAxisParallel();
  void ComputePrincipalAxisPerspective();
  void ComputePermutationMatrix();
  void ComputeShearMatrixParallel();
  void ComputeShearMatrixPerspective();
  void ComputeWarpMatrix();
  void CompositeIntermediate();
  void BuildImage(unsigned char *id, vtkShearWarpPixelData *im);

  void Unwarp(float *destination, int dWidth, int dHeight, float *source, int left, int top, int sWidth, int sHeight, vtkMatrix4x4* w);
  void ExtractZBuffer(vtkRenderer *ren, vtkVolume *vol);

  void InitializeClippingPlanes( vtkPlaneCollection *planes );
  int IsVoxelClipped(int x, int y, int z);

  // void ComputeDepthTable(int first, int last);
  //void DepthCueImage (vtkShearWarpPixelData *im, int slice);

  virtual void RenderTexture(vtkRenderer *ren, vtkVolume *vol) = 0;
   
private:
  vtkVolumeShearWarpMapper(const vtkVolumeShearWarpMapper&);  // Not implemented.
  void operator=(const vtkVolumeShearWarpMapper&);  // Not implemented.
};



#endif


