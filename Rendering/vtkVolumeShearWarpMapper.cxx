/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeShearWarpMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVolumeShearWarpMapper.h"

#include "vtkVolume.h"
#include "vtkRenderer.h"
#include "vtkFiniteDifferenceGradientEstimator.h"
#include "vtkRenderWindow.h"
#include "vtkGraphicsFactory.h"
#include "vtkMatrix4x4.h"
#include "vtkTransform.h"
#include "vtkTimerLog.h"
#include "vtkPlaneCollection.h"

#include "vtkEncodedGradientShader.h"
#include "vtkEncodedGradientEstimator.h"
#include "vtkStructuredPoints.h"
#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkVolumeProperty.h"
#include "vtkPiecewiseFunction.h"

#include "vtkOpenGLVolumeShearWarpMapper.h"

#define vtkVSWMultiplyPointMacro( A, B, M ) \
  B[0] = A[0]*M[0]  + A[1]*M[1]  + A[2]*M[2]  + M[3]; \
  B[1] = A[0]*M[4]  + A[1]*M[5]  + A[2]*M[6]  + M[7]; \
  B[2] = A[0]*M[8]  + A[1]*M[9]  + A[2]*M[10] + M[11]; \
  B[3] = A[0]*M[12] + A[1]*M[13] + A[2]*M[14] + M[15]; \
  if ( B[3] != 1.0 ) { B[0] /= B[3]; B[1] /= B[3]; B[2] /= B[3]; }

#define vtkVSWMultiplyNormalMacro( A, B, M ) \
  B[0] = A[0]*M[0]  + A[1]*M[4]  + A[2]*M[8]; \
  B[1] = A[0]*M[1]  + A[1]*M[5]  + A[2]*M[9]; \
  B[2] = A[0]*M[2]  + A[1]*M[6]  + A[2]*M[10]
  
vtkCxxRevisionMacro(vtkVolumeShearWarpMapper, "1.2");

//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkVolumeShearWarpMapper);
//----------------------------------------------------------------------------

// Factor the view matrix into shear and warp
void vtkVolumeShearWarpMapper::FactorViewMatrix()
{

  ComputeViewportMatrix();
  ComputeViewMatrix();

  if (this->ParallelProjection)
    ComputePrincipalAxisParallel();
  else if (this->MyPerspectiveProjection)
  ComputePrincipalAxisParallel();
  else
//      ComputePrincipalAxisParallel();
    ComputePrincipalAxisPerspective();
  
    
  ComputePermutationMatrix();
  
  if (this->ParallelProjection) 
    ComputeShearMatrixParallel();
  else if (this->MyPerspectiveProjection)
  ComputeShearMatrixParallel();
  else
//    ComputeShearMatrixParallel();
    ComputeShearMatrixPerspective();
    
  ComputeWarpMatrix();
}


// Compute the view matrix for parallel projection
void vtkVolumeShearWarpMapper::ComputeViewMatrix()
{
  vtkTransform *view = vtkTransform::New();
  view->SetMatrix(VoxelsToViewMatrix);
  view->Inverse();  

  WorldViewingDirection[0] = 0.0;
  WorldViewingDirection[1] = 0.0;
  WorldViewingDirection[2] = 1.0;
  WorldViewingDirection[3] = 0.0;

  WorldEyePosition[0] = 0.0;
  WorldEyePosition[1] = 0.0;
  WorldEyePosition[2] = -1.0;
  WorldEyePosition[3] = 0.0;

  // Compute viewing direction in object space (for parallel projection)
  view->MultiplyPoint(WorldViewingDirection,ObjectViewingDirection);

  // Compute eye position in object space (for perspective projection)
  view->MultiplyPoint(WorldEyePosition,ObjectEyePosition);
  
}

// Compute the viewport matrix
void vtkVolumeShearWarpMapper::ComputeViewportMatrix()
{
  ViewportMatrix->Identity();
  ViewportMatrix->Element[0][0] = 0.5 * (double) ImageViewportSize[0];
  ViewportMatrix->Element[0][3] = 0.5 * (double) ImageViewportSize[0];
  ViewportMatrix->Element[1][1] = 0.5 * (double) ImageViewportSize[1];
  ViewportMatrix->Element[1][3] = 0.5 * (double) ImageViewportSize[1];
}


// Compute the principal viewing axis for parallel projection
void vtkVolumeShearWarpMapper::ComputePrincipalAxisParallel()
{
  double x = fabs(ObjectViewingDirection[0]);
  double y = fabs(ObjectViewingDirection[1]);
  double z = fabs(ObjectViewingDirection[2]);
  
  if (x >= y)
  {
    if (x >= z)
      this->MajorAxis = VTK_X_AXIS;
    else
      this->MajorAxis = VTK_Z_AXIS;
  }
  else
  {
    if (y >= z)
      this->MajorAxis = VTK_Y_AXIS;
    else
      this->MajorAxis = VTK_Z_AXIS;

  }

  if (ObjectViewingDirection[this->MajorAxis] > 0)
    this->ReverseOrder = 0;
  else
    this->ReverseOrder = 1;
}

// Compute the principal viewing axis for perspective projection
void vtkVolumeShearWarpMapper::ComputePrincipalAxisPerspective()
{
  double vertex[3];
  double distance[3];
  double eye[3];
  double ax, ay, az;
  double maximumDistance;
  int maximumCount;
  int count[3];
  int order[3];
  int axis[8];
  int i;

  eye[0] = ObjectEyePosition[0] / ObjectEyePosition[3];
  eye[1] = ObjectEyePosition[1] / ObjectEyePosition[3];
  eye[2] = ObjectEyePosition[2] / ObjectEyePosition[3];
  
  // Find principal axes:
  for (i=0; i<8; i++)
  {
    // Generate volume corners:
    vertex[0] = -0.5 + (double)(i % 2);
    vertex[1] = -0.5 + (double)((i/2) % 2);
    vertex[2] = -0.5 + (double)((i/4) % 2);

    vertex[0] *= this->GetInput()->GetDimensions()[0];
    vertex[1] *= this->GetInput()->GetDimensions()[1];
    vertex[2] *= this->GetInput()->GetDimensions()[2];

    distance[0] = vertex[0] - eye[0];
    distance[1] = vertex[1] - eye[1];
    distance[2] = vertex[2] - eye[2];

    // Determine the principal viewing axis and the stacking order:
    ax = fabs(distance[0]);

    ay = fabs(distance[1]);
    az = fabs(distance[2]);

    maximumDistance = ax;

    if (ay > maximumDistance)
      maximumDistance = ay;

    if (az > maximumDistance)
      maximumDistance = az;

    if (maximumDistance == ax)
    {
      axis[i] = VTK_X_AXIS;
      order[0] = (distance[0] < 0.0) ? 1 : 0;
    }
    else if (maximumDistance == ay)
    {
      axis[i] = VTK_Y_AXIS;
      order[1] = (distance[1] < 0.0) ? 1 : 0;
    }
    else
    {
      axis[i] = VTK_Z_AXIS;
      order[2] = (distance[2] < 0.0) ? 1 : 0;
    }
  }

  // Find the dominating principal axis:
  for (i=0; i<3; i++)
    count[i] = 0;
    
  for (i=0; i<8; i++)
  {
    switch (axis[i])
    {
      case VTK_X_AXIS:
        count[0]++;
        break;

      case VTK_Y_AXIS:
        count[1]++;
        break;
        
      case VTK_Z_AXIS:
        count[2]++;
        break;

      default:
        break;
    }
  }

  // Assign the dominant axis for the principal axis (favor the Z axis for ties):
  maximumCount = count[0];

  if (count[1] > maximumCount)
    maximumCount = count[1];

  if (count[2] > maximumCount)
    maximumCount = count[2];

  if (maximumCount == count[2])
  {
    this->MajorAxis = VTK_Z_AXIS;
    this->ReverseOrder = order[2];
  }
  else if (maximumCount==count[1])
  {
    this->MajorAxis = VTK_Y_AXIS;
    this->ReverseOrder = order[1];
  }
  else
  {
    this->MajorAxis = VTK_X_AXIS;
    this->ReverseOrder = order[0];
  }
}


// Compute the permutation matrix (transformation from object space to standard object space)
void vtkVolumeShearWarpMapper::ComputePermutationMatrix()
{
  PermutationMatrix->Zero();

  int size[3];
  GetInput()->GetDimensions(size);

  switch (this->MajorAxis)

  {
    case VTK_X_AXIS:
      PermutationMatrix->Element[0][1] = 1.0f;
      PermutationMatrix->Element[1][2] = 1.0f;
      PermutationMatrix->Element[2][0] = 1.0f;
      PermutationMatrix->Element[3][3] = 1.0f;

      this->CountI = int(float(size[1]) / this->ImageSampleDistance);
      this->CountJ = int(float(size[2]) / this->ImageSampleDistance);
      this->CountK = int(float(size[0]) / this->ImageSampleDistance);
      break;

    case VTK_Y_AXIS:
      PermutationMatrix->Element[0][2] = 1.0f;

      PermutationMatrix->Element[1][0] = 1.0f;
      PermutationMatrix->Element[2][1] = 1.0f;
      PermutationMatrix->Element[3][3] = 1.0f;

      this->CountI = int(float(size[2]) / this->ImageSampleDistance);
      this->CountJ = int(float(size[0]) / this->ImageSampleDistance);
      this->CountK = int(float(size[1]) / this->ImageSampleDistance);
      break;

    case VTK_Z_AXIS:
    default:
      PermutationMatrix->Element[0][0] = 1.0f;
      PermutationMatrix->Element[1][1] = 1.0f;
      PermutationMatrix->Element[2][2] = 1.0f;
      PermutationMatrix->Element[3][3] = 1.0f;

      this->CountI = int(float(size[0]) / this->ImageSampleDistance);
      this->CountJ = int(float(size[1]) / this->ImageSampleDistance);
      this->CountK = int(float(size[2]) / this->ImageSampleDistance);
      break;
  }

  this->MaximumIntermediateDimension = size[0];

  if (size[1] > this->MaximumIntermediateDimension)
    this->MaximumIntermediateDimension = size[1];

  if (size[2] > this->MaximumIntermediateDimension)
    this->MaximumIntermediateDimension = size[2];

  this->MaximumIntermediateDimension *= 2;

  // Compute the viewing direction in standard object space (for parallel projection)
  this->PermutationMatrix->MultiplyPoint(ObjectViewingDirection,StandardViewingDirection);

  // Compute the eye position in standard object space (for perspective projection)
  this->PermutationMatrix->MultiplyPoint(ObjectEyePosition,StandardEyePosition);

  // Compute the permuted view to voxel matrix
  vtkMatrix4x4::Multiply4x4(this->PermutationMatrix,this->ViewToVoxelsMatrix,this->PermutedViewToVoxelsMatrix);

  // Compute the permuted voxel to view matrix
  vtkMatrix4x4::Multiply4x4(this->PermutationMatrix,this->VoxelsToViewMatrix,this->PermutedVoxelsToViewMatrix);

  
  // Get depth cueing factors
  /*
  this->DepthI = this->PermutedVoxelsToViewMatrix->Element[2][0];
  this->DepthJ = this->PermutedVoxelsToViewMatrix->Element[2][1];
  this->DepthK = this->PermutedVoxelsToViewMatrix->Element[2][2];  
  this->Depth0 = this->PermutedVoxelsToViewMatrix->Element[2][3];
  */

}

// Compute the shear matrix (transformation from object to intermediate image space)
void vtkVolumeShearWarpMapper::ComputeShearMatrixParallel()
{
  vtkMatrix4x4 *conv = vtkMatrix4x4::New();  // conversion to intermediate image coordinate system
  vtkMatrix4x4 *shear = vtkMatrix4x4::New();  // shear standard object space to intermediate image space

  // Compute shear factors:
  this->ShearI = - StandardViewingDirection[0] / StandardViewingDirection[2];
  this->ShearJ = - StandardViewingDirection[1] / StandardViewingDirection[2];
  this->Scale  = 1.0f;
  
  /* compute the intermediate image size */
  this->IntermediateWidth = this->CountI + 1 + (int) ceil((this->CountK-1)*fabs(this->ShearI));
  this->IntermediateHeight = this->CountJ + 1 + (int) ceil((this->CountK-1)*fabs(this->ShearJ));

  /* compute the translation coefficients */
  if (this->ShearI >= 0.0)
    this->TranslationI = 1.0;
  else

    this->TranslationI = 1.0 - this->ShearI * (double) (this->CountK - 1);

  if (this->ShearJ >= 0.0)
    this->TranslationJ = 1.0;
  else
    this->TranslationJ = 1.0 - this->ShearJ * (double) (this->CountK - 1);

  // Assemble standard object space shear matrix from shear factors
  shear->Identity();
  shear->Element[0][2] = this->ShearI;
  shear->Element[1][2] = this->ShearJ;

  // Add scale factor depending on object size:
//  shear->Scale((double) size[0] / (double) this->CountI, (double) size[1] / (double)  this->CountJ, (double) size[2] / (double) this->CountK);

  // Create conversion matrix for intermediate image coordinates
  conv->Identity();
  conv->Element[0][3] = 0.5 * (double) this->IntermediateWidth;
  conv->Element[1][3] = 0.5 * (double) this->IntermediateHeight;

  vtkTransform *shearTransform = vtkTransform::New();
  shearTransform->SetMatrix(this->PermutationMatrix);
  shearTransform->PostMultiply();
  shearTransform->Concatenate(shear);
  shearTransform->Concatenate(conv);
  this->ShearMatrix->DeepCopy(shearTransform->GetMatrix());
  shearTransform->Delete();

  shear->Delete();
  conv->Delete();

}

// Compute the shear matrix (transformation from object to intermediate image space)
void vtkVolumeShearWarpMapper::ComputeShearMatrixPerspective()
{

  vtkMatrix4x4 *conv = vtkMatrix4x4::New();  // conversion to intermediate image coordinate system
  vtkMatrix4x4 *shear = vtkMatrix4x4::New();  // shear standard object space to intermediate image space
  vtkMatrix4x4 *scale = vtkMatrix4x4::New();

  // Compute shear factors: fabs
  this->ShearI = - StandardEyePosition[0] / StandardEyePosition[2];
  this->ShearJ = - StandardEyePosition[1] / StandardEyePosition[2];
  this->Scale  = - StandardEyePosition[3] / StandardEyePosition[2];

  float scaleFactor = 1.0;

//  if(this->Scale < 0.0)
//    this->Scale = - this->Scale;


  if (this->ReverseOrder==1)
    scaleFactor = 1.0f / (1.0f - (this->CountK-1) * this->Scale);

  /* compute the intermediate image size */
  this->IntermediateWidth = this->CountI + 1 + (int) ceil((this->CountK-1)*fabs(this->ShearI));// - (int)(this->CountI - 1 - scaleFactor*(this->CountI-1));
  this->IntermediateHeight = this->CountJ + 1 + (int) ceil((this->CountK-1)*fabs(this->ShearJ));// - (int)(this->CountJ - 1 - scaleFactor*(this->CountJ-1));

  /* compute the translation coefficients */

  if (this->ShearI >= 0.0)
    this->TranslationI = 1.0;
  else
    this->TranslationI = 1.0 - this->ShearI * (double) (this->CountK - 1);

  if (this->ShearJ >= 0.0)
    this->TranslationJ = 1.0;
  else
    this->TranslationJ = 1.0 - this->ShearJ * (double) (this->CountK - 1);

  // Assemble standard object space shear matrix from shear factors
  shear->Identity();
  shear->Element[0][2] = this->ShearI;
  shear->Element[1][2] = this->ShearJ;
  shear->Element[3][2] = this->Scale;

  // Add scale factor depending on object size:
//  shear->Scale((double) size[0] / (double) this->CountI, (double) size[1] / (double)  this->CountJ, (double) size[2] / (double) this->CountK);

  double sf;

  if (this->ReverseOrder)
    sf = 1.0 + this->Scale * (double)(this->CountK - 1);// - shear->Element[3][2];
  else
    sf = 1.0;//shear->Element[3][2] + 1;

/*
  double sf;

  if (this->ReverseOrder==1)
    sf = 1.0 - shear->Element[3][2];
  else
    sf = shear->Element[3][2] + 1.0;

  sf = 1.0 / sf;   // invert scale factor
*/
  scale->Identity();
  scale->Element[0][0] = sf;
  scale->Element[1][1] = sf;

    // Create conversion matrix for intermediate image coordinates
  conv->Identity();
  conv->Element[0][3] = 0.5 * (double) this->IntermediateWidth;
  conv->Element[1][3] = 0.5 * (double) this->IntermediateHeight;

  vtkTransform *shearTransform = vtkTransform::New();
  shearTransform->SetMatrix(this->PermutationMatrix);
  shearTransform->PostMultiply();
//  shearTransform->Concatenate(scale);
  shearTransform->Concatenate(shear);
  shearTransform->Concatenate(conv);
  this->ShearMatrix->DeepCopy(shearTransform->GetMatrix());
  shearTransform->Delete();

  shear->Delete();
  scale->Delete();
  conv->Delete();




/*
  double lb[4] = { 0, 0, this->CountK, 1.0 };
  double rb[4] = { this->CountI, this->CountJ, this->CountK, 1.0 };
  double tlt[4],tlb[4],trb[4];
  shearTransform->MultiplyPoint(lb,tlb);
  shearTransform->MultiplyPoint(rb,trb);
  tlb[0] /= tlb[3];
  tlb[1] /= tlb[3];
  trb[0] /= trb[3];
  trb[1] /= trb[3];
  this->IntermediateWidth = trb[0] - tlb[0];
  this->IntermediateHeight = trb[1] - tlb[1];
  conv->Identity();
  conv->Element[0][3] = 0.5 * (double) this->IntermediateWidth;
  conv->Element[1][3] = 0.5 * (double) this->IntermediateHeight;
*/


}

// Compute a the two-dimensional warp matrix
void vtkVolumeShearWarpMapper::ComputeWarpMatrix()
{

  vtkTransform *warp = vtkTransform::New();
/*
  vtkTransform *inverse = vtkTransform::New();
  inverse->SetMatrix(this->ShearMatrix);
  inverse->Inverse();

  // Invert shear matrix
  warp->SetMatrix(VoxelsToViewMatrix);

  // Compute warp matrix
  warp->PreMultiply();
  warp->Concatenate(inverse);


  warp->PreMultiply();
  warp->Concatenate(ViewportMatrix);
  */

  // Compute inverse of shear matrix:
  warp->SetMatrix(ShearMatrix);
  warp->Inverse();

  // Compute warp matrices:
  warp->PostMultiply();
  warp->Concatenate(VoxelsToViewMatrix);
  warp->Concatenate(ViewportMatrix);

  WarpMatrix->DeepCopy(warp->GetMatrix());
  warp->Delete();
}

/*
// Compute a lookup table used for depth cueing
void vtkVolumeShearWarpMapper::ComputeDepthTable(int first, int last)
{
  for (int i = first; i <= last; i++)
    this->DepthTable[i] = this->FrontDepth * exp(-this->DepthDensity*(1.0 - i*this->DeltaDepth));

  this->DepthTableSize = last - first;    
}


// Perfrom second step for fast depth cueing (multiply intermediate image by second depth factor)
void vtkVolumeShearWarpMapper::DepthCueImage (vtkShearWarpPixelData *im, int slice)
{

  double depthQuant = 1.0 / this->DeltaDepth;

  float pixelDepth;
  int pixelDepthInteger;

  float uSlice = this->ShearI * slice + this->TranslationI;
  float vSlice = this->ShearJ * slice + this->TranslationJ;
  float leftDepth = this->Depth0 + this->DepthK*slice - uSlice*this->DepthI - vSlice*this->DepthJ;

  if (this->DepthI > 0)
  {
    if (this->DepthJ > 0)
      max_depth = left_depth + this->DepthI * width + this->DepthJ * height;
    else
      max_depth = left_depth + this->DepthI * width;
  }
  else
  {
    if (depth_dj > 0)
      max_depth = left_depth + this->DepthJ * height;
    else
      max_depth = left_depth;
  }

  max_depth_int = max_depth * depth_quant;

  if (max_depth_int >= vpc->dc_table_len)
  {
    // Resize table
  }
    
  float di = this->DepthI * depthQuant;
  float dj = this->DepthJ * depthQuant;

  leftDepth *= depthQuant;

  for (int j = this->IntermediateHeight; j > 0; j--)
  {
    pixelDepth = leftDepth;
    leftDepth += dj;

    for (int i = this->IntermediateWidth; i > 0; i--)
    {
      pixelDepthInteger = pixelDepth;
      pixelDepth += di;

      if (pixelDepthInteger < 0)
        pixelDepthInteger = 0;
          
      if (pixelDepthInteger >= this->DepthTableSize)
      {
        // This shouldn't happen
        pixelDepthInteger = this->DepthTableSize - 1;
      }

      im->Red *= this->DepthTable[pixelDepthInteger];
      im->Green *= this->DepthTable[pixelDepthInteger];
      im->Blue *= this->DepthTable[pixelDepthInteger];
      im++;
    }
  }
}
*/

// Simple parallel projection shear-warp without runlength encoded volume using bilinear interpolation
template <class T>

void CompositeIntermediateNearestSimple(vtkShearWarpRLEImage *image, vtkVolumeShearWarpMapper *myThis)
{
  vtkShearWarpPixelData *pixels;

  float sampledRed,sampledGreen,sampledBlue,sampledOpacity;
  float sampledValue;
  float sampledGradientMagnitude;
  float oldRed,oldGreen,oldBlue,oldOpacity;
  float newRed,newGreen,newBlue,newOpacity;
  float redDiffuse,greenDiffuse,blueDiffuse;
  float redSpecular,greenSpecular,blueSpecular;
  float gradientOpacity;
  float isoRed;
  float isoGreen;
  float isoBlue;

  int skipped;
  int i,j,k;
  int vi,vj,vk;

  float *SOTF;
  float *CTF;
  float *GTF;
  float *GOTF;
  float gradientOpacityConstant;
  int gradientOpacityIsConstant;
  unsigned short encodedNormal;
  unsigned char gradientMagnitude;

  float uSlice;
  float vSlice;

  int uSliceInteger;
  int vSliceInteger;

  T value;

  int kStart;
  int kEnd;
  int kIncrement;
  int vkStart;
  int viIncrement;
  int vjIncrement;
  int vkIncrement;

  T *dptr = (T*) myThis->GetInput()->GetScalarPointer();

  unsigned short *nptr = myThis->GradientEstimator->GetEncodedNormals();
  unsigned char *gptr = myThis->GradientEstimator->GetGradientMagnitudes();

  int *dimensions = myThis->GetInput()->GetDimensions();
  int location;
  int plane = dimensions[0]*dimensions[1];
  int halfDistance = myThis->ImageSampleDistance / 2;

  //float depthCueFactor;
  //float depthCueRatio;

  if (myThis->ReverseOrder)
  {
    kStart = myThis->CountK - 1;
    kEnd = -1 + halfDistance;
    kIncrement = -1;
  }
  else
  {
    kStart = 0;
    kEnd = myThis->CountK - halfDistance;
    kIncrement = 1;
  }

  SOTF =  myThis->Volume->GetCorrectedScalarOpacityArray();
  CTF  =  myThis->Volume->GetRGBArray();
  GTF  =  myThis->Volume->GetGrayArray();
  GOTF =  myThis->Volume->GetGradientOpacityArray();
  gradientOpacityConstant = myThis->Volume->GetGradientOpacityConstant();

  if (gradientOpacityConstant > 0.0f)
    gradientOpacityIsConstant = 1;
  else
    gradientOpacityIsConstant = 0;


  if (myThis->FunctionType == VTK_SHEAR_WARP_ISOSURFACE_FUNCTION)
  {
    isoRed = CTF[int(myThis->IsoValue)*3 + 0];
    isoGreen = CTF[int(myThis->IsoValue)*3 + 1];
    isoBlue = CTF[int(myThis->IsoValue)*3 + 2];
  }

  switch (myThis->MajorAxis)
  {
    case VTK_X_AXIS:
      viIncrement = dimensions[0] * myThis->ImageSampleDistance;
      vjIncrement = plane * myThis->ImageSampleDistance;
      vkIncrement = kIncrement * myThis->ImageSampleDistance;
      vkStart = kStart * myThis->ImageSampleDistance;
      break;

    case VTK_Y_AXIS:
      viIncrement = plane * myThis->ImageSampleDistance;
      vjIncrement = myThis->ImageSampleDistance;
      vkIncrement = kIncrement * dimensions[0] * myThis->ImageSampleDistance;
      vkStart = kStart * dimensions[0] * myThis->ImageSampleDistance;

      break;



    case VTK_Z_AXIS:                       
      default:
      viIncrement = myThis->ImageSampleDistance;
      vjIncrement = dimensions[0] * myThis->ImageSampleDistance;
      vkIncrement = kIncrement * plane * myThis->ImageSampleDistance;
      vkStart = kStart * plane * myThis->ImageSampleDistance;

      break;
  }

  /*
  float delta = myThis->DepthK - myThis->DepthI * myThis->ShearI - myThis->DepthJ * myThis->ShearJ;

  cout << "\n\nDELTA: " << delta << "\n\n";                               
  
  if (myThis->ReverseOrder == 1)
    delta = -delta;

  depthCueRatio = exp (myThis->DepthDensity * delta);
  depthCueFactor = 1.0f;
  */      
  for (k = kStart, vk = vkStart; k != kEnd; k += kIncrement, vk += vkIncrement)
  {
    uSlice = k*myThis->ShearI + myThis->TranslationI;
    vSlice = k*myThis->ShearJ + myThis->TranslationJ;
   
    uSliceInteger = (int) ceil(uSlice) - 1;
    vSliceInteger = (int) ceil(vSlice) - 1;

    // Composite one slice into the intermediate image
    for (j=0, vj = halfDistance; j < myThis->CountJ-halfDistance; j++, vj += vjIncrement)
    {
      image->Position(pixels,uSliceInteger + (vSliceInteger+j)*myThis->IntermediateWidth);

      for (i=0, vi = halfDistance; i < myThis->CountI-halfDistance; )
      {
        // Skip opaque pixels in intermediate image
        skipped = image->Skip(pixels);

        // Update both runs if to be aligned with intermediate pixels
        if (skipped > 0)
        {
          i += skipped;
          vi += viIncrement * skipped;
        }
        else
        {
          if (myThis->IntermixIntersectingGeometry)
          {
            float depth = myThis->IntermediateZBuffer[myThis->ImageSampleDistance * (uSliceInteger + i) + myThis->ImageSampleDistance * (vSliceInteger + j) * myThis->IntermediateWidth * myThis->ImageSampleDistance];
             
            if (myThis->ReverseOrder)
            {
              if (k*myThis->ImageSampleDistance <= depth)
                pixels->Offset = 1;
            }
            else
            {
              if (k*myThis->ImageSampleDistance >= depth)
                pixels->Offset = 1;
            }

          }
          
          // Only process non-opaque pixels
          if (pixels->Offset == 0)
          {
            if (myThis->IsVoxelClipped(i*myThis->ImageSampleDistance,j*myThis->ImageSampleDistance,k*myThis->ImageSampleDistance) == 1)
            {
              image->Advance(pixels,1);
              i++;
              vi += viIncrement;
              continue;
            }


            oldOpacity = pixels->Opacity;
            oldRed = pixels->Red;
            oldGreen = pixels->Green;
            oldBlue = pixels->Blue;

            location = vi + vj + vk;

            if (myThis->FunctionType == VTK_SHEAR_WARP_COMPOSITE_FUNCTION)
            {
              sampledOpacity = 0.0f;
              sampledRed = 0.0f;
              sampledGreen = 0.0f;
              sampledBlue = 0.0f;

              value = dptr[location];
              sampledOpacity += SOTF[value];
              sampledRed += CTF[value*3 + 0];
              sampledGreen += CTF[value*3 + 1];
              sampledBlue += CTF[value*3 + 2];

              if (myThis->Shade)
              {
                redDiffuse = 0.0f;
                redSpecular = 0.0f;
                blueDiffuse = 0.0f;
                blueSpecular = 0.0f;
                greenDiffuse = 0.0f;
                greenSpecular = 0.0f;
                sampledGradientMagnitude = 0.0f;
                gradientOpacity = gradientOpacityConstant;

                encodedNormal = nptr[location];

                redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal];
                redSpecular += myThis->RedSpecularShadingTable[encodedNormal];
                greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal];
                greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal];
                blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal];
                blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal];

                if (!gradientOpacityIsConstant)
                {
                  gradientMagnitude = gptr[location];
                  sampledGradientMagnitude += float(gradientMagnitude);

                  if (sampledGradientMagnitude > 255.0f)
                    gradientOpacity = GOTF[255];
                  else if (sampledGradientMagnitude < 0.0f)
                    gradientOpacity = GOTF[0];
                  else
                    gradientOpacity = GOTF[(unsigned char) sampledGradientMagnitude];
                }

                sampledRed *= redDiffuse + redSpecular;
                sampledGreen *= greenDiffuse + greenSpecular;
                sampledBlue *= blueDiffuse + blueSpecular;
                sampledOpacity *= gradientOpacity;
              }

              /*
              if (1)
              {
                sampledRed *= depthCueFactor;
                sampledGreen *= depthCueFactor;
                sampledBlue *= depthCueFactor;
              }
              */

              // Alpha Compositing
              newRed = oldRed + sampledOpacity * sampledRed * (1.0f - oldOpacity);
              newGreen = oldGreen + sampledOpacity * sampledGreen * (1.0f - oldOpacity);
              newBlue = oldBlue + sampledOpacity * sampledBlue * (1.0f - oldOpacity);
              newOpacity = oldOpacity + sampledOpacity * (1.0f - oldOpacity);
            }
            else if (myThis->FunctionType == VTK_SHEAR_WARP_MIP_FUNCTION)
            {
              sampledValue = 0.0f;

              value = dptr[location];
              sampledValue += float(value);

              // Maximum Intensity Projection
              if (sampledValue > pixels->Value)
              {

                newRed = CTF[int(sampledValue)*3+0];
                newGreen = CTF[int(sampledValue)*3+1];
                newBlue = CTF[int(sampledValue)*3+2];
                newOpacity = SOTF[int(sampledValue)];
                pixels->Value = sampledValue;
              }
              else

              {
                newRed = oldRed;
                newGreen = oldGreen;
                newBlue = oldBlue;
                newOpacity = oldOpacity;
              }
            }
            else
            {
              sampledValue = 0.0f;

              value = dptr[location];
              sampledValue += float(value);

              if (sampledValue >= myThis->IsoValue)
              {               
                sampledRed = isoRed;
                sampledGreen = isoGreen;

                sampledBlue = isoBlue;
 
                if (myThis->Shade)
                {
                  redDiffuse = 0.0f;
                  redSpecular = 0.0f;
                  blueDiffuse = 0.0f;
                  blueSpecular = 0.0f;
                  greenDiffuse = 0.0f;
                  greenSpecular = 0.0f;

                  encodedNormal = nptr[location];

                  redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal];
                  redSpecular += myThis->RedSpecularShadingTable[encodedNormal];
                  greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal];
                  greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal];
                  blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal];
                  blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal];

                  sampledRed *= redDiffuse + redSpecular;
                  sampledGreen *= greenDiffuse + greenSpecular;
                  sampledBlue *= blueDiffuse + blueSpecular;
                }

                newRed = sampledRed;
                newGreen = sampledGreen;
                newBlue = sampledBlue;
                newOpacity = 1.0f;
              }
              else
              {
                newRed = oldRed;
                newGreen = oldGreen;
                newBlue = oldBlue;
                newOpacity = oldOpacity;
              }
            }

            pixels->Red = newRed;
            pixels->Green = newGreen;
            pixels->Blue = newBlue;
            pixels->Opacity = newOpacity;            

            if (newOpacity >= 1.0f)
            {
              // The current intermediate pixel is opaque, so exit
              // loop and skip opaque pixels
              pixels->Offset = 1;
            }
            else
            {
              image->Advance(pixels,1);
              i++;
              vi += viIncrement;
            }

          }
        }
      }
    }
    /*
    depthCueFactor *= (pow(depthCueRatio,myThis->ImageSampleDistance));
    cout << depthCueFactor << ", ";
    */
  }

  //myThis->DepthCueImage(image->GetPixelData(),(kEnd-kIncrement) * myThis->ImageSampleDistance);
}


// Simple parallel projection shear-warp without runlength encoded volume using bilinear interpolation
template <class T>
void CompositeIntermediateLinearSimple(vtkShearWarpRLEImage *image, vtkVolumeShearWarpMapper *myThis)
{
  vtkShearWarpPixelData *pixels = 0;

  float sampledRed,sampledGreen,sampledBlue,sampledOpacity;
  float sampledValue;
  float sampledGradientMagnitude;
  float oldRed,oldGreen,oldBlue,oldOpacity;
  float newRed,newGreen,newBlue,newOpacity;
  float redDiffuse,greenDiffuse,blueDiffuse;
  float redSpecular,greenSpecular,blueSpecular;
  float gradientOpacity;


  float isoRed;
  float isoGreen;
  float isoBlue;

  int skipped;

  int i,j,k;
  int vi,vj,vk;

  float *SOTF;
  float *CTF;
  float *GTF;
  float *GOTF;
  float gradientOpacityConstant;
  int gradientOpacityIsConstant;
  unsigned short encodedNormal;
  unsigned char gradientMagnitude;

  float uSlice;
  float vSlice;

  int uSliceInteger;
  int vSliceInteger;

  float uSliceFractional;
  float vSliceFractional;


  float weightTopLeft;
  float weightBottomLeft;
  float weightTopRight;
  float weightBottomRight;

  float adjustedTL;
  float adjustedTR;
  float adjustedBL;
  float adjustedBR;

  T value;

  int kStart;
  int vkStart;
  int kEnd;
  int kIncrement;
  int viIncrement;  
  int vjIncrement;
  int vkIncrement;

  T *dptr = (T*) myThis->GetInput()->GetScalarPointer();

  unsigned short *nptr = myThis->GradientEstimator->GetEncodedNormals();
  unsigned char *gptr = myThis->GradientEstimator->GetGradientMagnitudes();

  int *dimensions = myThis->GetInput()->GetDimensions();
  int locationTL,locationTR,locationBL,locationBR;
  int plane = dimensions[0]*dimensions[1];

  if (myThis->ReverseOrder)
  {
    kStart = myThis->CountK - 1;
    kEnd = -1;
    kIncrement = -1;
  }
  else
  {
    kStart = 0;
    kEnd = myThis->CountK;
    kIncrement = 1;
  }

  SOTF =  myThis->Volume->GetCorrectedScalarOpacityArray();
  CTF  =  myThis->Volume->GetRGBArray();
  GTF  =  myThis->Volume->GetGrayArray();
  GOTF =  myThis->Volume->GetGradientOpacityArray();
  gradientOpacityConstant = myThis->Volume->GetGradientOpacityConstant();

  if (gradientOpacityConstant > 0.0f)

    gradientOpacityIsConstant = 1;
  else
    gradientOpacityIsConstant = 0;


  if (myThis->FunctionType == VTK_SHEAR_WARP_ISOSURFACE_FUNCTION)
  {
    isoRed = CTF[int(myThis->IsoValue)*3 + 0];
    isoGreen = CTF[int(myThis->IsoValue)*3 + 1];
    isoBlue = CTF[int(myThis->IsoValue)*3 + 2];
  }

  switch (myThis->MajorAxis)
  {
    case VTK_X_AXIS:
      viIncrement = dimensions[0] * myThis->ImageSampleDistance;
      vjIncrement = plane * myThis->ImageSampleDistance;
      vkIncrement = kIncrement * myThis->ImageSampleDistance;
      vkStart = kStart * myThis->ImageSampleDistance;
      break;

    case VTK_Y_AXIS:
      viIncrement = plane * myThis->ImageSampleDistance;
      vjIncrement = myThis->ImageSampleDistance;
      vkIncrement = kIncrement * dimensions[0] * myThis->ImageSampleDistance;
      vkStart = kStart * dimensions[0] * myThis->ImageSampleDistance;
      break;


    case VTK_Z_AXIS:
      default:
      viIncrement = myThis->ImageSampleDistance;
      vjIncrement = dimensions[0] * myThis->ImageSampleDistance;
      vkIncrement = kIncrement * plane * myThis->ImageSampleDistance;
      vkStart = kStart * plane * myThis->ImageSampleDistance;
      break;
  }
  


  for (k = kStart, vk = vkStart; k != kEnd; k += kIncrement, vk += vkIncrement)
  {
    uSlice = k*myThis->ShearI + myThis->TranslationI;
    vSlice = k*myThis->ShearJ + myThis->TranslationJ;

    uSliceInteger = (int)ceil(uSlice) - 1;
    vSliceInteger = (int)ceil(vSlice) - 1;

    uSliceFractional = uSlice - uSliceInteger;
    vSliceFractional = vSlice - vSliceInteger;

    weightTopLeft = uSliceFractional * vSliceFractional;
    weightBottomLeft = uSliceFractional * (1.0f - vSliceFractional);

    weightTopRight = (1.0f - uSliceFractional) * vSliceFractional;
    weightBottomRight = (1.0f - uSliceFractional) * (1.0f - vSliceFractional);

    // Composite one slice into the intermediate image
    for (j=0, vj = 0; j < myThis->CountJ; j++, vj += vjIncrement)
    {
      image->Position(pixels,uSliceInteger + (vSliceInteger+j)*myThis->IntermediateWidth);

      for (i=0, vi = 0; i < myThis->CountI; )
      {
        // Skip opaque pixels in intermediate image
        skipped = image->Skip(pixels);

        // Update both runs if to be aligned with intermediate pixels
        if (skipped > 0)
        {
          i += skipped;
          vi += viIncrement/*myThis->ImageSampleDistance*/ * skipped;
        }
        else
        {
          if (myThis->IntermixIntersectingGeometry)
          {
            float depth = myThis->IntermediateZBuffer[myThis->ImageSampleDistance * (uSliceInteger + i) + myThis->ImageSampleDistance * (vSliceInteger + j) * myThis->IntermediateWidth * myThis->ImageSampleDistance];


            if (myThis->ReverseOrder)
            {
              if (k*myThis->ImageSampleDistance <= depth)
                pixels->Offset = 1;
            }
            else
            {

              if (k*myThis->ImageSampleDistance >= depth)
                pixels->Offset = 1;
            }
          }

          // Only process non-opaque pixels
          if (pixels->Offset == 0)
          {
            if (myThis->IsVoxelClipped(i*myThis->ImageSampleDistance,j*myThis->ImageSampleDistance,k*myThis->ImageSampleDistance) == 1)
            {
              image->Advance(pixels,1);
              i++;

              vi += viIncrement;
              continue;
            }



            oldOpacity = pixels->Opacity;
            oldRed = pixels->Red;
            oldGreen = pixels->Green;
            oldBlue = pixels->Blue;

            locationTL = vi + vj + vk;
            locationTR = locationTL + viIncrement;
            locationBL = locationTL + vjIncrement;
            locationBR = locationBL + viIncrement;

/*            switch (myThis->MajorAxis)
            {
              case VTK_X_AXIS:
                locationTL = vj * plane + vi *dimensions[0] + vk;


                locationTR = vj * plane + (vi+myThis->ImageSampleDistance) *dimensions[0] + vk;
                locationBL = (vj+myThis->ImageSampleDistance) * plane + vi *dimensions[0] + vk;
                locationBR = (vj+myThis->ImageSampleDistance) * plane + vi *dimensions[0] + vk;

                break;

              case VTK_Y_AXIS:
                locationTL = vi * plane + vk *dimensions[0] + vj;
                locationTR = (vi+myThis->ImageSampleDistance) * plane + vk *dimensions[0] + vj;
                locationBL = vi * plane + vk *dimensions[0] + (vj+myThis->ImageSampleDistance);
                locationBR = (vi+myThis->ImageSampleDistance) * plane + vk *dimensions[0] + (vj+myThis->ImageSampleDistance);
                break;

              case VTK_Z_AXIS:
                default:
                locationTL = vk * plane + vj *dimensions[0] + vi;
                locationTR = vk * plane + vj *dimensions[0] + (vi+myThis->ImageSampleDistance);
                locationBL = vk * plane + (vj+myThis->ImageSampleDistance) *dimensions[0] + vi;
                locationBR = vk * plane + (vj+myThis->ImageSampleDistance) *dimensions[0] + (vi+myThis->ImageSampleDistance);
                break;
            }*/

            if (myThis->FunctionType == VTK_SHEAR_WARP_COMPOSITE_FUNCTION)
            {

              sampledOpacity = 0.0f;
              sampledRed = 0.0f;
              sampledGreen = 0.0f;
              sampledBlue = 0.0f;

              value = dptr[locationTL];
              sampledOpacity += SOTF[value] * weightTopLeft;
              sampledRed += CTF[value*3 + 0] * weightTopLeft;
              sampledGreen += CTF[value*3 + 1] * weightTopLeft;
              sampledBlue += CTF[value*3 + 2] * weightTopLeft;


              if (i + 1 < myThis->CountI)
              {
                value = dptr[locationTR];
                sampledOpacity += SOTF[value] * weightTopRight;
                sampledRed += CTF[value*3 + 0] * weightTopRight;
                sampledGreen += CTF[value*3 + 1] * weightTopRight;
                sampledBlue += CTF[value*3 + 2] * weightTopRight;
              }

              if (j + 1 < myThis->CountJ)
              {
                value = dptr[locationBL];
                sampledOpacity += SOTF[value] * weightBottomLeft;
                sampledRed += CTF[value*3 + 0] * weightBottomLeft;
                sampledGreen += CTF[value*3 + 1] * weightBottomLeft;
                sampledBlue += CTF[value*3 + 2] * weightBottomLeft;

                if (i + 1 < myThis->CountI)
                {
                  value = dptr[locationBR];
                  sampledOpacity += SOTF[value] * weightBottomRight;
                  sampledRed += CTF[value*3 + 0] * weightBottomRight;
                  sampledGreen += CTF[value*3 + 1] * weightBottomRight;
                  sampledBlue += CTF[value*3 + 2] * weightBottomRight;
                }
              }

              if (myThis->Shade)
            {
                redDiffuse = 0.0f;
                redSpecular = 0.0f;
                blueDiffuse = 0.0f;
                blueSpecular = 0.0f;
                greenDiffuse = 0.0f;
                greenSpecular = 0.0f;
                sampledGradientMagnitude = 0.0f;

                gradientOpacity = gradientOpacityConstant;

                encodedNormal = nptr[locationTL];

                redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * weightTopLeft;
                redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * weightTopLeft;
                greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * weightTopLeft;
                greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * weightTopLeft;
                blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * weightTopLeft;
                blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * weightTopLeft;


                if (!gradientOpacityIsConstant)
                {
                  gradientMagnitude = gptr[locationTL];
                  sampledGradientMagnitude += float(gradientMagnitude) * weightTopLeft;
                }

                if (i + 1 < myThis->CountI)
                {
                  encodedNormal = nptr[locationTR];

                  redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * weightTopRight;
                  redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * weightTopRight;
                  greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * weightTopRight;

                  greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * weightTopRight;
                  blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * weightTopRight;
                  blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * weightTopRight;

                  if (!gradientOpacityIsConstant)
                  {
                    gradientMagnitude = gptr[locationTR];
                    sampledGradientMagnitude += float(gradientMagnitude) * weightTopRight;
                  }
                }

                if (j + 1 < myThis->CountJ)
                {
                  encodedNormal = nptr[locationBL];

                  redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * weightBottomLeft;
                  redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * weightBottomLeft;
                  greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * weightBottomLeft;
                  greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * weightBottomLeft;
                  blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * weightBottomLeft;
                  blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * weightBottomLeft;

                  if (!gradientOpacityIsConstant)
                  {
                    gradientMagnitude = gptr[locationBL];
                    sampledGradientMagnitude += float(gradientMagnitude) * weightBottomLeft;
                  }

                  if (i + 1 < myThis->CountI)
                  {
                    encodedNormal = nptr[locationBR];

                    redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * weightBottomRight;
                    redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * weightBottomRight;
                    greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * weightBottomRight;
                    greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * weightBottomRight;
                    blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * weightBottomRight;

                    blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * weightBottomRight;

                    if (!gradientOpacityIsConstant)
                    {
                      gradientMagnitude = gptr[locationBR];
                      sampledGradientMagnitude += float(gradientMagnitude) * weightBottomRight;
                    }
                  }
                }

                if (!gradientOpacityIsConstant)
                {
                  if (sampledGradientMagnitude > 255.0f)
                    gradientOpacity = GOTF[255];
                  else if (sampledGradientMagnitude < 0.0f)
                    gradientOpacity = GOTF[0];
                  else
                    gradientOpacity = GOTF[(unsigned char) sampledGradientMagnitude];
                }

                sampledRed *= redDiffuse + redSpecular;
                sampledGreen *= greenDiffuse + greenSpecular;
                sampledBlue *= blueDiffuse + blueSpecular;
                sampledOpacity *= gradientOpacity;
              }

              // Alpha Compositing
              newRed = oldRed + sampledOpacity * sampledRed * (1.0f - oldOpacity);
              newGreen = oldGreen + sampledOpacity * sampledGreen * (1.0f - oldOpacity);
              newBlue = oldBlue + sampledOpacity * sampledBlue * (1.0f - oldOpacity);
              newOpacity = oldOpacity + sampledOpacity * (1.0f - oldOpacity);
            }
            else if (myThis->FunctionType == VTK_SHEAR_WARP_MIP_FUNCTION)
            {
              sampledValue = 0.0f;

              value = dptr[locationTL];
              sampledValue += float(value) * weightTopLeft;

              if (i + 1 < myThis->CountI)
              {
                value = dptr[locationTR];
                sampledValue += float(value) * weightTopRight;
              }

              if (j + 1 < myThis->CountJ)
              {
                value = dptr[locationBL];
                sampledValue += float(value) * weightBottomLeft;

                if (i + 1 < myThis->CountI)
                {
                  value = dptr[locationBR];
                  sampledValue += float(value) * weightBottomRight;
                }
              }

              // Maximum Intensity Projection
              if (sampledValue > pixels->Value)
              {
                newRed = CTF[int(sampledValue)*3+0];
                newGreen = CTF[int(sampledValue)*3+1];
                newBlue = CTF[int(sampledValue)*3+2];
                newOpacity = SOTF[int(sampledValue)];

                pixels->Value = sampledValue;
              }
              else
              {
                newRed = oldRed;
                newGreen = oldGreen;

                newBlue = oldBlue;
                newOpacity = oldOpacity;
              }
            }
            else
            {
              sampledRed = isoRed;
              sampledGreen = isoGreen;
              sampledBlue = isoBlue;

              sampledValue = 0.0f;

              value = dptr[locationTL];
              sampledValue += float(value) * weightTopLeft;

              if (i + 1 < myThis->CountI)
              {
                value = dptr[locationTR];
                sampledValue += float(value) * weightTopRight;
              }

              if (j + 1 < myThis->CountJ)
              {
                value = dptr[locationBL];
                sampledValue += float(value) * weightBottomLeft;

                if (i + 1 < myThis->CountI)

                {
                  value = dptr[locationBR];
                  sampledValue += float(value) * weightBottomRight;
                }
              }

              // Maximum Intensity Projection
              if (sampledValue >= myThis->IsoValue)
              {              
                if (myThis->Shade)
                {
                  redDiffuse = 0.0f;
                  redSpecular = 0.0f;
                  blueDiffuse = 0.0f;
                  blueSpecular = 0.0f;
                  greenDiffuse = 0.0f;
                  greenSpecular = 0.0f;

                  adjustedTL = weightTopLeft;
                  adjustedBL = weightBottomLeft;
                  adjustedTR = weightTopRight;
                  adjustedBR = weightBottomRight;

                  if (i + 1 >= myThis->CountI)
                  {
                    adjustedTL += adjustedTR;
                    adjustedBL += adjustedBR;
                  }

                  if (j + 1 >= myThis->CountJ)
                  {
                    adjustedTL += adjustedBL;
                    adjustedTR += adjustedBR;
                  }

                  encodedNormal = nptr[locationTL];

                  redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * adjustedTL;

                  redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * adjustedTL;
                  greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * adjustedTL;
                  greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * adjustedTL;
                  blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * adjustedTL;
                  blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * adjustedTL;

                  if (i + 1 < myThis->CountI)
                  {
                    encodedNormal = nptr[locationTR];

                    redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * adjustedTR;
                    redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * adjustedTR;
                    greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * adjustedTR;
                    greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * adjustedTR;
                    blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * adjustedTR;
                    blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * adjustedTR;
                  }

                  if (j + 1 < myThis->CountJ)
                  {
                    encodedNormal = nptr[locationBL];

                    redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * adjustedBL;
                    redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * adjustedBL;
                    greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * adjustedBL;
                    greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * adjustedBL;
                    blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * adjustedBL;
                    blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * adjustedBL;

                    if (i + 1 < myThis->CountI)
                    {
                      encodedNormal = nptr[locationBR];

                      redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * adjustedBR;
                      redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * adjustedBR;
                      greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * adjustedBR;
                      greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * adjustedBR;
                      blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * adjustedBR;
                      blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * adjustedBR;

                    }
                  }

                  sampledRed *= redDiffuse + redSpecular;
                  sampledGreen *= greenDiffuse + greenSpecular;
                  sampledBlue *= blueDiffuse + blueSpecular;
                  
                }


                newRed = sampledRed;
                newGreen = sampledGreen;
                newBlue = sampledBlue;
                newOpacity = 1.0f;
              }
              else
              {
                newRed = oldRed;
                newGreen = oldGreen;
                newBlue = oldBlue;
                newOpacity = oldOpacity;
              }
            }


            pixels->Red = newRed;
            pixels->Green = newGreen;
            pixels->Blue = newBlue;
            pixels->Opacity = newOpacity;

            if (newOpacity >= 1.0f)
            {
              // The current intermediate pixel is opaque, so exit

              // loop and skip opaque pixels
              pixels->Offset = 1;

            }
            else
            {
              image->Advance(pixels,1);
              i++;
              vi += viIncrement;
            }
          }
        }
      }

    }
  }
}

// Lacroute's parallel projection shear-warp algorithm with runlength encoded volume using nearest neighbour interpolation
template <class T>
void CompositeIntermediateNearestRLE(vtkShearWarpRLEImage *image, vtkVolumeShearWarpMapper *myThis)
{
  vtkShearWarpRLESlice<T> *slice;
  vtkShearWarpRLERun<T> *top;
  vtkShearWarpPixelData *pixels;

  float sampledRed,sampledGreen,sampledBlue,sampledOpacity;
  float sampledValue;
  float sampledGradientMagnitude;
  float oldRed,oldGreen,oldBlue,oldOpacity;

  float newRed,newGreen,newBlue,newOpacity;
  float redDiffuse,greenDiffuse,blueDiffuse;
  float redSpecular,greenSpecular,blueSpecular;
  float gradientOpacity;

  float isoRed;
  float isoGreen;
  float isoBlue;

  int topIndex;

  int skipped;
  int runLength;

  int i,j,k;
  int vj,vk;

  float *SOTF;
  float *CTF;
  float *GTF;
  float *GOTF;
  float gradientOpacityConstant;
  int gradientOpacityIsConstant;
  unsigned short encodedNormal;
  unsigned char gradientMagnitude;

  float uSlice;
  float vSlice;

  int uSliceInteger;
  int vSliceInteger;

  T value;

  int kStart;
  int kEnd;
  int kIncrement;
  int vkStart;
  int vkIncrement;

  int halfDistance = myThis->ImageSampleDistance / 2;

  vtkShearWarpRLEVolume<T> *encodedVolume = dynamic_cast< vtkShearWarpRLEVolume<T> * > (myThis->EncodedVolume);

  if (myThis->ReverseOrder)
  {
    kStart = myThis->CountK - 1;
    kEnd = -1 + halfDistance;

    kIncrement = -1;

    vkStart = (myThis->CountK - 1) * myThis->ImageSampleDistance - halfDistance;
    vkIncrement = -myThis->ImageSampleDistance;
  }
  else
  {
    kStart = 0;
    kEnd = myThis->CountK - halfDistance;
    kIncrement = 1;

    vkStart = halfDistance;
    vkIncrement = myThis->ImageSampleDistance;
  }

  SOTF =  myThis->Volume->GetCorrectedScalarOpacityArray();
  CTF  =  myThis->Volume->GetRGBArray();
  GTF  =  myThis->Volume->GetGrayArray();
  GOTF =  myThis->Volume->GetGradientOpacityArray();
  gradientOpacityConstant = myThis->Volume->GetGradientOpacityConstant();

  if (gradientOpacityConstant > 0.0f)
    gradientOpacityIsConstant = 1;
  else
    gradientOpacityIsConstant = 0;


  if (myThis->FunctionType == VTK_SHEAR_WARP_ISOSURFACE_FUNCTION)
  {
    isoRed = CTF[int(myThis->IsoValue)*3 + 0];
    isoGreen = CTF[int(myThis->IsoValue)*3 + 1];
    isoBlue = CTF[int(myThis->IsoValue)*3 + 2];
  }

  for (k = kStart, vk = vkStart; k != kEnd; k += kIncrement, vk += vkIncrement)
  {
    uSlice = k*myThis->ShearI + myThis->TranslationI;
    vSlice = k*myThis->ShearJ + myThis->TranslationJ;

    uSliceInteger = (int)ceil(uSlice) - 1;
    vSliceInteger = (int)ceil(vSlice) - 1;

    slice = encodedVolume->GetSlice(myThis->MajorAxis,vk);

    // Composite one slice into the intermediate image
    for (j=0, vj = halfDistance; j < myThis->CountJ-halfDistance; j++, vj += myThis->ImageSampleDistance)
    {
      top = slice->GetLineRuns(vj);
      topIndex = halfDistance;

      while (topIndex >= top->Length)

      {
        topIndex -= top->Length;
        top++;
      }
        
      image->Position(pixels,uSliceInteger + (vSliceInteger+j)*myThis->IntermediateWidth);


      for (i=0; i < myThis->CountI; )
      {
        // Update run
        while (topIndex >= top->Length)
        {

          topIndex -= top->Length;
          top++;
        }

        // Skip opaque pixels in intermediate image
        skipped = image->Skip(pixels);

        // Update both runs if to be aligned with intermediate pixels
        if (skipped > 0)
        {
          i += skipped;
          topIndex += skipped * myThis->ImageSampleDistance;
        }
        else
        {
          runLength = top->Length - topIndex;

          // Skip transparent voxels


          if (top->VoxelData == NULL)
          {
            while (topIndex < top->Length)
            {
              image->Advance(pixels,1);
              i++;
              topIndex += myThis->ImageSampleDistance;
            }

          }
          else
          {

            // This loops samples voxels, performs shading and performs
            // compositing into the intermediate image
            while (topIndex < top->Length)//h=0; h < runLength; h+=myThis->ImageSampleDistance)
            {
              if (myThis->IntermixIntersectingGeometry)
              {
                float depth = myThis->IntermediateZBuffer[myThis->ImageSampleDistance * (uSliceInteger + i) + myThis->ImageSampleDistance * (vSliceInteger + j) * myThis->IntermediateWidth * myThis->ImageSampleDistance];

                if (myThis->ReverseOrder)
                {
                  if (vk <= depth)
                    pixels->Offset = 1;
                }
                else
                {

                  if (vk >= depth)
                    pixels->Offset = 1;
                }
              }
              
              // Only process non-opaque pixels
              if (pixels->Offset == 0)
              {
                if (myThis->IsVoxelClipped(i*myThis->ImageSampleDistance,vj,vk) == 1)
                {
                  image->Advance(pixels,1);
                  i++;
                  topIndex+=myThis->ImageSampleDistance;
                  continue;
                }
                
                oldOpacity = pixels->Opacity;
                oldRed = pixels->Red;
                oldGreen = pixels->Green;
                oldBlue = pixels->Blue;

                if (myThis->FunctionType == VTK_SHEAR_WARP_COMPOSITE_FUNCTION)
                {
                  value = top->VoxelData[topIndex].Value;
                  sampledOpacity = SOTF[value];
                  sampledRed = CTF[value*3 + 0];
                  sampledGreen = CTF[value*3 + 1];
                  sampledBlue = CTF[value*3 + 2];

                  if (myThis->Shade)
                  {
                    encodedNormal = top->VoxelData[topIndex].EncodedNormal;

                    redDiffuse = myThis->RedDiffuseShadingTable[encodedNormal];
                    redSpecular = myThis->RedSpecularShadingTable[encodedNormal];
                    greenDiffuse = myThis->GreenDiffuseShadingTable[encodedNormal];
                    greenSpecular = myThis->GreenSpecularShadingTable[encodedNormal];
                    blueDiffuse = myThis->BlueDiffuseShadingTable[encodedNormal];
                    blueSpecular = myThis->BlueSpecularShadingTable[encodedNormal];

                    if (!gradientOpacityIsConstant)
                    {
                      gradientMagnitude = top->VoxelData[topIndex].GradientMagnitude;
                      sampledGradientMagnitude = float(gradientMagnitude);
                    }


                    if (!gradientOpacityIsConstant)
                    {
                      if (sampledGradientMagnitude > 255.0f)
                        gradientOpacity = GOTF[255];
                      else if (sampledGradientMagnitude < 0.0f)
                        gradientOpacity = GOTF[0];
                      else
                        gradientOpacity = GOTF[(unsigned char) sampledGradientMagnitude];
                    }
                    else
                      gradientOpacity = gradientOpacityConstant;


                    sampledRed *= redDiffuse + redSpecular;
                    sampledGreen *= greenDiffuse + greenSpecular;
                    sampledBlue *= blueDiffuse + blueSpecular;
                    sampledOpacity *= gradientOpacity;
                  }

                  // Alpha Compositing
                  newRed = oldRed + sampledOpacity * sampledRed * (1.0f - oldOpacity);
                  newGreen = oldGreen + sampledOpacity * sampledGreen * (1.0f - oldOpacity);
                  newBlue = oldBlue + sampledOpacity * sampledBlue * (1.0f - oldOpacity);
                  newOpacity = oldOpacity + sampledOpacity * (1.0f - oldOpacity);
                }
                else if (myThis->FunctionType == VTK_SHEAR_WARP_MIP_FUNCTION)
                {
                  value = top->VoxelData[topIndex].Value;
                  sampledValue = value;

                  // Maximum Intensity Projection
                  if (sampledValue > pixels->Value)
                  {
                    newRed = CTF[int(sampledValue)*3+0];
                    newGreen = CTF[int(sampledValue)*3+1];
                    newBlue = CTF[int(sampledValue)*3+2];
                    newOpacity = SOTF[int(sampledValue)];
                    pixels->Value = sampledValue;
                  }
                  else
                  {
                    newRed = oldRed;
                    newGreen = oldGreen;
                    newBlue = oldBlue;
                    newOpacity = oldOpacity;
                  }
                }
                else
                {
                  sampledRed = isoRed;
                  sampledGreen = isoGreen;
                  sampledBlue = isoBlue;
                  sampledOpacity = 1.0f;

                  if (myThis->Shade)

                  {
                    encodedNormal = top->VoxelData[topIndex].EncodedNormal;

                    redDiffuse = myThis->RedDiffuseShadingTable[encodedNormal];
                    redSpecular = myThis->RedSpecularShadingTable[encodedNormal];
                    greenDiffuse = myThis->GreenDiffuseShadingTable[encodedNormal];
                    greenSpecular = myThis->GreenSpecularShadingTable[encodedNormal];
                    blueDiffuse = myThis->BlueDiffuseShadingTable[encodedNormal];
                    blueSpecular = myThis->BlueSpecularShadingTable[encodedNormal];


                    sampledRed *= redDiffuse + redSpecular;
                    sampledGreen *= greenDiffuse + greenSpecular;
                    sampledBlue *= blueDiffuse + blueSpecular;
                  }

                  newRed = sampledRed;
                  newGreen = sampledGreen;
                  newBlue = sampledBlue;
                  newOpacity = 1.0f;


                }

                pixels->Red = newRed;
                pixels->Green = newGreen;
                pixels->Blue = newBlue;
                pixels->Opacity = newOpacity;

                if (newOpacity >= 1.0f)
                {
                  // The current intermediate pixel is opaque, so exit
                  // loop and skip opaque pixels
                  pixels->Offset = 1;
                  break;
                }

                image->Advance(pixels,1);
                i++;
                topIndex+=myThis->ImageSampleDistance;
              }
              else
              {
                // The current pixel has an offset greater than zero, so
                // we exit the loop and skip all opaque pixels
                break;
              }
            }
          }
        }
      }
    }
  }
}


// Lacroute's parallel projection shear-warp algorithm with runlength encoded volume using bilinear interpolation
template <class T>
void CompositeIntermediateLinearRLE(vtkShearWarpRLEImage *image, vtkVolumeShearWarpMapper *myThis)
{
  vtkShearWarpRLESlice<T> *slice;
  vtkShearWarpRLERun<T> *top;
  vtkShearWarpRLERun<T> *bottom;
  vtkShearWarpPixelData *pixels;

  float sampledRed,sampledGreen,sampledBlue,sampledOpacity;
  float sampledValue;
  float sampledGradientMagnitude;
  float oldRed,oldGreen,oldBlue,oldOpacity;
  float newRed,newGreen,newBlue,newOpacity;
  float redDiffuse,greenDiffuse,blueDiffuse;
  float redSpecular,greenSpecular,blueSpecular;
  float gradientOpacity;

  float isoRed;
  float isoGreen;
  float isoBlue;

  int topIndex;
  int bottomIndex;
  int topStart;
  int bottomStart;

  int skipped;
  int runLength;

  int i,j,k,h;
  int vj,vk;

  float *SOTF;
  float *CTF;
  float *GTF;
  float *GOTF;

  float gradientOpacityConstant;
  int gradientOpacityIsConstant;
  unsigned short encodedNormal;
  unsigned char gradientMagnitude;

  float uSlice;
  float vSlice;

  int uSliceInteger;
  int vSliceInteger;


  float uSliceFractional;
  float vSliceFractional;

  float weightTopLeft;
  float weightBottomLeft;
  float weightTopRight;
  float weightBottomRight;


  float adjustedTL;
  float adjustedTR;
  float adjustedBL;
  float adjustedBR;

  T value;

  int kStart;
  int kEnd;
  int kIncrement;
  int vkIncrement;

  vtkShearWarpRLEVolume<T> *encodedVolume = dynamic_cast< vtkShearWarpRLEVolume<T> * > (myThis->EncodedVolume);

  if (myThis->ReverseOrder)
  {
    kStart = myThis->CountK - 1;
    kEnd = -1;
    kIncrement = -1;
    vkIncrement = -myThis->ImageSampleDistance;
  }
  else
  {
    kStart = 0;
    kEnd = myThis->CountK;
    kIncrement = 1;
    vkIncrement = myThis->ImageSampleDistance;
  }

  SOTF =  myThis->Volume->GetCorrectedScalarOpacityArray();
  CTF  =  myThis->Volume->GetRGBArray();
  GTF  =  myThis->Volume->GetGrayArray();
  GOTF =  myThis->Volume->GetGradientOpacityArray();
  gradientOpacityConstant = myThis->Volume->GetGradientOpacityConstant();

  if (gradientOpacityConstant > 0.0f)
    gradientOpacityIsConstant = 1;
  else
    gradientOpacityIsConstant = 0;


  if (myThis->FunctionType == VTK_SHEAR_WARP_ISOSURFACE_FUNCTION)
  {
    isoRed = CTF[int(myThis->IsoValue)*3 + 0];
    isoGreen = CTF[int(myThis->IsoValue)*3 + 1];
    isoBlue = CTF[int(myThis->IsoValue)*3 + 2];
  }

  for (k = kStart, vk = kStart*myThis->ImageSampleDistance; k != kEnd; k += kIncrement, vk += vkIncrement)
  {
    topIndex = 0;
    bottomIndex = 0;
    topStart = 0;
    bottomStart = 0;

    uSlice = k*myThis->ShearI + myThis->TranslationI;
    vSlice = k*myThis->ShearJ + myThis->TranslationJ;

    uSliceInteger = (int)ceil(uSlice) - 1;
    vSliceInteger = (int)ceil(vSlice) - 1;

    uSliceFractional = uSlice - uSliceInteger;
    vSliceFractional = vSlice - vSliceInteger;

    weightTopLeft = uSliceFractional * vSliceFractional;
    weightBottomLeft = uSliceFractional * (1.0f - vSliceFractional);
    weightTopRight = (1.0f - uSliceFractional) * vSliceFractional;
    weightBottomRight = (1.0f - uSliceFractional) * (1.0f - vSliceFractional);

    slice = encodedVolume->GetSlice(myThis->MajorAxis,vk);

    // Composite one slice into the intermediate image
    for (j=0, vj = 0; j < myThis->CountJ; j++, vj += myThis->ImageSampleDistance)
    {
      top = slice->GetLineRuns(vj);

      if ((j + 1) < myThis->CountJ)
        bottom = slice->GetLineRuns(vj+myThis->ImageSampleDistance);
      else
        bottom = NULL;

      topStart = 0;
      bottomStart = 0;

      topIndex = 0;
      bottomIndex = 0;

      image->Position(pixels,uSliceInteger + (vSliceInteger+j)*myThis->IntermediateWidth);

      for (i=0; i < myThis->CountI; )
      {
        // Update runs
        while (topIndex >= top->Length)
        {
          topIndex -= top->Length;
          topStart += top->Length;
          top++;
        }

        if (bottom != NULL)
        {
          while (bottomIndex >= bottom->Length)
          {
            bottomIndex -= bottom->Length;
            bottomStart += bottom->Length;
            bottom++;
          }
        }
        
        // Skip opaque pixels in intermediate image
        skipped = image->Skip(pixels);

        // Update both runs if to be aligned with intermediate pixels
        if (skipped > 0)
        {
          i += skipped;

          topIndex += skipped * myThis->ImageSampleDistance;
          bottomIndex += skipped * myThis->ImageSampleDistance;
        }
        else
        {
          if (bottom != NULL)
          {
            if ( ((topStart + top->Length) - (topStart + topIndex)) < ((bottomStart + bottom->Length) - (bottomStart + bottomIndex)) )
              runLength = ((topStart + top->Length) - (topStart + topIndex));
            else
              runLength = ((bottomStart + bottom->Length) - (bottomStart + bottomIndex));
          }
          else
            runLength = top->Length - topIndex;

          // Skip transparent voxels in both runs

          if (top->VoxelData == NULL && (((bottom != NULL) ? bottom->VoxelData : NULL) == NULL))

          {
            for (h = 0; h < runLength; h+=myThis->ImageSampleDistance)

            {
              image->Advance(pixels,1);
              i++;
              topIndex += myThis->ImageSampleDistance;
              bottomIndex += myThis->ImageSampleDistance;
            }
          }
          else
          {

            // This loops samples voxels from both runs,
            // interpolates, performs shading and performs
            // compositing into the intermediate image
            for (h=0; h < runLength; h+= myThis->ImageSampleDistance)
            {
              if (myThis->IntermixIntersectingGeometry)
              {
                float depth = myThis->IntermediateZBuffer[myThis->ImageSampleDistance * (uSliceInteger + i) + myThis->ImageSampleDistance * (vSliceInteger + j) * myThis->IntermediateWidth * myThis->ImageSampleDistance];

                if (myThis->ReverseOrder)
                {
                  if (vk <= depth)
                    pixels->Offset = 1;
                }
                else
                {
                  if (vk >= depth)
                    pixels->Offset = 1;
                }
              }

              // Only process non-opaque pixels
              if (pixels->Offset == 0)
              {

                if (myThis->IsVoxelClipped(i*myThis->ImageSampleDistance,vj,vk) == 1)
                {
                  image->Advance(pixels,1);
                  i++;
                  topIndex += myThis->ImageSampleDistance;
                  bottomIndex += myThis->ImageSampleDistance;
                  continue;
                }


                oldOpacity = pixels->Opacity;
                oldRed = pixels->Red;
                oldGreen = pixels->Green;
                oldBlue = pixels->Blue;

                if (myThis->FunctionType == VTK_SHEAR_WARP_COMPOSITE_FUNCTION)
                {
                  sampledOpacity = 0.0f;
                  sampledRed = 0.0f;
                  sampledGreen = 0.0f;
                  sampledBlue = 0.0f;

                  if (top->VoxelData != NULL)
                  {
                    value = top->VoxelData[topIndex].Value;
                    sampledOpacity += SOTF[value] * weightTopLeft;
                    sampledRed += CTF[value*3 + 0] * weightTopLeft;
                    sampledGreen += CTF[value*3 + 1] * weightTopLeft;
                    sampledBlue += CTF[value*3 + 2] * weightTopLeft;

                    if (h + myThis->ImageSampleDistance < runLength)
                    {
                      value = top->VoxelData[topIndex+myThis->ImageSampleDistance].Value;
                      sampledOpacity += SOTF[value] * weightTopRight;
                      sampledRed += CTF[value*3 + 0] * weightTopRight;
                      sampledGreen += CTF[value*3 + 1] * weightTopRight;
                      sampledBlue += CTF[value*3 + 2] * weightTopRight;
                    }
                  }

                  if (bottom != NULL)
                  {
                    if (bottom->VoxelData != NULL)
                    {
                      value = bottom->VoxelData[bottomIndex].Value;
                      sampledOpacity += SOTF[value] * weightBottomLeft;
                      sampledRed += CTF[value*3 + 0] * weightBottomLeft;
                      sampledGreen += CTF[value*3 + 1] * weightBottomLeft;
                      sampledBlue += CTF[value*3 + 2] * weightBottomLeft;


                      if (h + myThis->ImageSampleDistance < runLength)
                      {
                        value = bottom->VoxelData[bottomIndex+myThis->ImageSampleDistance].Value;
                        sampledOpacity += SOTF[value] * weightBottomRight;
                        sampledRed += CTF[value*3 + 0] * weightBottomRight;
                        sampledGreen += CTF[value*3 + 1] * weightBottomRight;
                        sampledBlue += CTF[value*3 + 2] * weightBottomRight;
                      }
                    }

                  }

                  if (myThis->Shade)
                  {
                      redDiffuse = 0.0f;
                      redSpecular = 0.0f;
                      blueDiffuse = 0.0f;
                      blueSpecular = 0.0f;
                      greenDiffuse = 0.0f;
                      greenSpecular = 0.0f;
                      sampledGradientMagnitude = 0.0f;
                      gradientOpacity = gradientOpacityConstant;

                      if (top->VoxelData != NULL)
                      {
                        encodedNormal = top->VoxelData[topIndex].EncodedNormal;


                        redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * weightTopLeft;
                        redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * weightTopLeft;
                        greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * weightTopLeft;
                        greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * weightTopLeft;
                        blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * weightTopLeft;
                        blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * weightTopLeft;

                        if (!gradientOpacityIsConstant)
                        {
                          gradientMagnitude = top->VoxelData[topIndex].GradientMagnitude;
                          sampledGradientMagnitude += float(gradientMagnitude) * weightTopLeft;
                        }

                        if (h + myThis->ImageSampleDistance < runLength)
                        {
                          encodedNormal = top->VoxelData[topIndex+myThis->ImageSampleDistance].EncodedNormal;

                          redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * weightTopRight;
                          redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * weightTopRight;
                          greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * weightTopRight;
                          greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * weightTopRight;
                          blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * weightTopRight;
                          blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * weightTopRight;

                          if (!gradientOpacityIsConstant)
                          {
                            gradientMagnitude = top->VoxelData[topIndex+myThis->ImageSampleDistance].GradientMagnitude;
                            sampledGradientMagnitude += float(gradientMagnitude) * weightTopRight;
                          }
                        }
                      }

                      if (bottom != NULL)
                      {
                        if (bottom->VoxelData != NULL)
                        {
                          encodedNormal = bottom->VoxelData[bottomIndex].EncodedNormal;

                          redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * weightBottomLeft;
                          redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * weightBottomLeft;
                          greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * weightBottomLeft;
                          greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * weightBottomLeft;
                          blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * weightBottomLeft;
                          blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * weightBottomLeft;


                          if (!gradientOpacityIsConstant)
                          {
                            gradientMagnitude = bottom->VoxelData[bottomIndex].GradientMagnitude;
                            sampledGradientMagnitude += float(gradientMagnitude) * weightBottomLeft;
                          }

                          if (h + myThis->ImageSampleDistance < runLength)
                          {
                            encodedNormal = bottom->VoxelData[bottomIndex+myThis->ImageSampleDistance].EncodedNormal;

                            redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * weightBottomRight;
                            redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * weightBottomRight;
                            greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * weightBottomRight;
                            greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * weightBottomRight;
                            blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * weightBottomRight;
                            blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * weightBottomRight;

                            if (!gradientOpacityIsConstant)
                            {

                              gradientMagnitude = bottom->VoxelData[bottomIndex+myThis->ImageSampleDistance].GradientMagnitude;
                              sampledGradientMagnitude += float(gradientMagnitude) * weightBottomRight;
                            }
                          }
                        }
                      }

                      if (!gradientOpacityIsConstant)
                      {
                        if (sampledGradientMagnitude > 255.0f)
                          gradientOpacity = GOTF[255];
                        else if (sampledGradientMagnitude < 0.0f)
                          gradientOpacity = GOTF[0];
                        else
                          gradientOpacity = GOTF[(unsigned char) sampledGradientMagnitude];
                      }

                      sampledRed *= redDiffuse + redSpecular;
                      sampledGreen *= greenDiffuse + greenSpecular;
                      sampledBlue *= blueDiffuse + blueSpecular;
                      sampledOpacity *= gradientOpacity;
                  }

                    // Alpha Compositing
                  newRed = oldRed + sampledOpacity * sampledRed * (1.0f - oldOpacity);
                  newGreen = oldGreen + sampledOpacity * sampledGreen * (1.0f - oldOpacity);
                  newBlue = oldBlue + sampledOpacity * sampledBlue * (1.0f - oldOpacity);
                  newOpacity = oldOpacity + sampledOpacity * (1.0f - oldOpacity);
                }
                else if (myThis->FunctionType == VTK_SHEAR_WARP_MIP_FUNCTION)
                {
                  sampledValue = 0.0f;

                  if (top->VoxelData != NULL)
                  {
                    value = top->VoxelData[topIndex].Value;
                    sampledValue += float(value) * weightTopLeft;

                    if (h + myThis->ImageSampleDistance < runLength)
                    {
                      value = top->VoxelData[topIndex+myThis->ImageSampleDistance].Value;
                      sampledValue += float(value) * weightTopRight;
                    }
                  }

                  if (bottom != NULL)
                  {
                    if (bottom->VoxelData != NULL)
                    {
                      value = bottom->VoxelData[bottomIndex].Value;
                      sampledValue += float(value) * weightBottomLeft;

                      if (h + myThis->ImageSampleDistance < runLength)
                      {

                        value = bottom->VoxelData[bottomIndex+myThis->ImageSampleDistance].Value;
                        sampledValue += float(value) * weightBottomRight;
                      }
                    }
                  }

                  // Maximum Intensity Projection
                  if (sampledValue > pixels->Value)
                  {
                    newRed = CTF[int(sampledValue)*3+0];
                    newGreen = CTF[int(sampledValue)*3+1];
                    newBlue = CTF[int(sampledValue)*3+2];
                    newOpacity = SOTF[int(sampledValue)];
                    pixels->Value = sampledValue;
                  }
                  else
                  {

                    newRed = oldRed;
                    newGreen = oldGreen;
                    newBlue = oldBlue;
                    newOpacity = oldOpacity;
                  }
                }
                else
                {
                  sampledRed = isoRed;
                  sampledGreen = isoGreen;

                  sampledBlue = isoBlue;

                  if (myThis->Shade)
                  {
                    redDiffuse = 0.0f;

                    redSpecular = 0.0f;
                    blueDiffuse = 0.0f;
                    blueSpecular = 0.0f;
                    greenDiffuse = 0.0f;
                    greenSpecular = 0.0f;

                    adjustedTL = weightTopLeft;
                    adjustedBL = weightBottomLeft;
                    adjustedTR = weightTopRight;
                    adjustedBR = weightBottomRight;


                    if (h + myThis->ImageSampleDistance >= runLength)
                    {
                      adjustedTL += adjustedTR;

                      adjustedBL += adjustedBR;
                    }

                    if (top->VoxelData == NULL)
                    {
                      adjustedBL += adjustedTL;
                      adjustedBR += adjustedTR;
                    }
                    else if ((((bottom != NULL) ? bottom->VoxelData : NULL) == NULL))
                    {
                      adjustedTL += adjustedBL;
                      adjustedTR += adjustedBR;
                    }

                    if (top->VoxelData != NULL)
                    {
                      encodedNormal = top->VoxelData[topIndex].EncodedNormal;

                      redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * adjustedTL;
                      redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * adjustedTL;
                      greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * adjustedTL;
                      greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * adjustedTL;
                      blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * adjustedTL;
                      blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * adjustedTL;

                      if (h + myThis->ImageSampleDistance < runLength)
                      {
                        encodedNormal = top->VoxelData[topIndex+myThis->ImageSampleDistance].EncodedNormal;

                        redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * adjustedTR;
                        redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * adjustedTR;
                        greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * adjustedTR;
                        greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * adjustedTR;
                        blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * adjustedTR;
                        blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * adjustedTR;
                      }
                    }

                    if (bottom != NULL)
                    {
                      if (bottom->VoxelData != NULL)
                      {
                        encodedNormal = bottom->VoxelData[bottomIndex].EncodedNormal;

                        redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * adjustedBL;
                        redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * adjustedBL;
                        greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * adjustedBL;
                        greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * adjustedBL;
                        blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * adjustedBL;
                        blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * adjustedBL;

                        if (h + myThis->ImageSampleDistance < runLength)
                        {

                          encodedNormal = bottom->VoxelData[bottomIndex+myThis->ImageSampleDistance].EncodedNormal;

                          redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * adjustedBR;
                          redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * adjustedBR;
                          greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * adjustedBR;
                          greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * adjustedBR;
                          blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * adjustedBR;
                          blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * adjustedBR;

                        }
                      }

                    }

                    sampledRed *= redDiffuse + redSpecular;
                    sampledGreen *= greenDiffuse + greenSpecular;
                    sampledBlue *= blueDiffuse + blueSpecular;


                  }

                  newRed = sampledRed;
                  newGreen = sampledGreen;
                  newBlue = sampledBlue;
                  newOpacity = 1.0f;
                }

                pixels->Red = newRed;
                pixels->Green = newGreen;
                pixels->Blue = newBlue;
                pixels->Opacity = newOpacity;


                if (newOpacity >= 1.0f)
                {
                  // The current intermediate pixel is opaque, so exit
                  // loop and skip opaque pixels
                  pixels->Offset = 1;
                  break;
                }

                image->Advance(pixels,1);

                i++;
                topIndex += myThis->ImageSampleDistance;
                bottomIndex += myThis->ImageSampleDistance;
              }
              else
              {
                // The current pixel has an offset greater than zero, so
                // we exit the loop and skip all opaque pixels
                break;
              }
            }
          }
        }
      }
    }
  }
}

// Lacroute's perspective projection shear-warp algorithm with runlength encoded volume using bilinear interpolation
template <class T>
void CompositeIntermediateLinearRLEPerspective(vtkShearWarpRLEImage *image, vtkVolumeShearWarpMapper *myThis)
{  
  vtkShearWarpRLERun<T> **line;
  float *lineIndex;
  vtkShearWarpRLERun<T> *currentLine;


  int currentLineIndex;
  vtkShearWarpRLESlice<T> *slice;
  vtkShearWarpPixelData *pixels;
  int voxels;
  int footprint;
  int left;

  float sampledRed,sampledGreen,sampledBlue,sampledOpacity;
  float sampledValue;
  float sampledGradientMagnitude;
  float oldRed,oldGreen,oldBlue,oldOpacity;
  float newRed,newGreen,newBlue,newOpacity;

  float redDiffuse,greenDiffuse,blueDiffuse;
  float redSpecular,greenSpecular,blueSpecular;
  float gradientOpacity;

  float isoRed;
  float isoGreen;

  float isoBlue;

  int skipped;
  int runLength;

  int i,j,k,h,g;
  int vk;
  float vj;

  float *SOTF;
  float *CTF;
  float *GTF;
  float *GOTF;
  float gradientOpacityConstant;
  int gradientOpacityIsConstant;
  unsigned short encodedNormal;
  unsigned char gradientMagnitude;

  float uSlice;
  float vSlice;

  int uSliceInteger;
  int vSliceInteger;

  float scaleFactor;
  float weight;

  T value;


  int kStart;
  int kEnd;
  int kIncrement;
  int vkIncrement;

  vtkShearWarpRLEVolume<T> *encodedVolume = dynamic_cast< vtkShearWarpRLEVolume<T> * > (myThis->EncodedVolume);

  if (myThis->ReverseOrder)
  {
    kStart = myThis->CountK - 1;
    kEnd = -1;
    kIncrement = -1;
    vkIncrement = -myThis->ImageSampleDistance;

  }
  else
  {
    kStart = 0;
    kEnd = myThis->CountK;
    kIncrement = 1;
    vkIncrement = myThis->ImageSampleDistance;
  }

  SOTF =  myThis->Volume->GetCorrectedScalarOpacityArray();
  CTF  =  myThis->Volume->GetRGBArray();

  GTF  =  myThis->Volume->GetGrayArray();
  GOTF =  myThis->Volume->GetGradientOpacityArray();
  gradientOpacityConstant = myThis->Volume->GetGradientOpacityConstant();

  if (gradientOpacityConstant > 0.0f)
    gradientOpacityIsConstant = 1;
  else
    gradientOpacityIsConstant = 0;


  if (myThis->FunctionType == VTK_SHEAR_WARP_ISOSURFACE_FUNCTION)

  {
    isoRed = CTF[int(myThis->IsoValue)*3 + 0];
    isoGreen = CTF[int(myThis->IsoValue)*3 + 1];
    isoBlue = CTF[int(myThis->IsoValue)*3 + 2];
  }

  float vjIncrement;
  float viIncrement;


  for (k = kStart, vk = kStart*myThis->ImageSampleDistance; k != kEnd; k += kIncrement, vk += vkIncrement)
  {
    if (myThis->ReverseOrder)
      scaleFactor = 1.0f / (1.0 - float(kStart*myThis->ImageSampleDistance-vk) * myThis->Scale);
    else
      scaleFactor = 1.0f / (1.0 + float(vk) * myThis->Scale);

    footprint = (int) (1.0f + ceil ( 1.0f / scaleFactor));
    
    vjIncrement = myThis->ImageSampleDistance / scaleFactor;
    viIncrement = myThis->ImageSampleDistance / scaleFactor;


    line = new vtkShearWarpRLERun<T>*[footprint];
    lineIndex = new float[footprint];
                
    uSlice = k*myThis->ShearI + myThis->TranslationI;
    vSlice = k*myThis->ShearJ + myThis->TranslationJ;

    uSliceInteger = (int)ceil(uSlice) - 1;
    vSliceInteger = (int)ceil(vSlice) - 1;



    slice = encodedVolume->GetSlice(myThis->MajorAxis,vk);

    // Composite one slice into the intermediate image
    for (j=0, vj = 0; j < myThis->CountJ*scaleFactor; j++, vj += vjIncrement)
    {
      for (g = 0; g < footprint; g++)
      {
        if (j+g < (myThis->CountJ*scaleFactor))
          line[g] = slice->GetLineRuns(int(vj+g*myThis->ImageSampleDistance));
        else
          line[g] = NULL;

        lineIndex[g] = 0;
      }

      image->Position(pixels,uSliceInteger + (vSliceInteger+j)*myThis->IntermediateWidth);

      for (i=0; i < myThis->CountI*scaleFactor; )
      {
        // Skip opaque pixels in intermediate image
        skipped = image->Skip(pixels);

        // Update both runs if to be aligned with intermediate pixels
        if (skipped > 0)
        {
          i += skipped;
          
          for (g = 0; g < footprint; g++)
          {
            if (line[g] == NULL)
              break;

              
            lineIndex[g] += skipped * viIncrement;

            while (lineIndex[g] >= line[g]->Length)

            {
              lineIndex[g] -= line[g]->Length;
              line[g]++;
            }
          }
            
        }
        else
        {
          sampledOpacity = 0.0f;
          sampledRed = 0.0f;
          sampledGreen = 0.0f;
          sampledBlue = 0.0f;
          sampledValue = 0.0f;
          redDiffuse = 0.0f;
          redSpecular = 0.0f;
          blueDiffuse = 0.0f;
          blueSpecular = 0.0f;
          greenDiffuse = 0.0f;
          greenSpecular = 0.0f;
          sampledGradientMagnitude = 0.0f;
          gradientOpacity = gradientOpacityConstant;
          
          oldOpacity = pixels->Opacity;
          oldRed = pixels->Red;
          oldGreen = pixels->Green;
          oldBlue = pixels->Blue;

          voxels = 0;

            // This loops samples voxels from both runs,
            // interpolates, performs shading and performs
            // compositing into the intermediate image
          for (g = 0; g < footprint; g++)
          {

            weight = 1.0;
            
            if (line[g] == NULL)
              break;
            
            left = footprint * myThis->ImageSampleDistance;

            if (i + left >= myThis->CountI*scaleFactor)
              left = (int)(myThis->CountI*scaleFactor - i) * myThis->ImageSampleDistance;

            currentLine = line[g];
            currentLineIndex = (int) lineIndex[g];

            while (left > 0)
            {
              if (currentLineIndex >= currentLine->Length)
              {
                currentLineIndex -= currentLine->Length;
                currentLine++;
              }
              
              runLength = (left < currentLine->Length) ? left : currentLine->Length;
              left -= runLength;

              if (currentLine->VoxelData == NULL)
              {
                currentLineIndex += runLength;                
              }
              else
              {                
                for (h = 0; h < runLength; h+= myThis->ImageSampleDistance)
                {                    
                  voxels++;

                  if (myThis->FunctionType == VTK_SHEAR_WARP_COMPOSITE_FUNCTION)
                  {
                    value = currentLine->VoxelData[(int)currentLineIndex].Value;
          if(value > 16000)
            continue;
                    sampledOpacity += SOTF[value] * weight;
                    sampledRed += CTF[value*3 + 0] * weight;
                    sampledGreen += CTF[value*3 + 1] * weight;
                    sampledBlue += CTF[value*3 + 2] * weight;

                    if (myThis->Shade)
                    {
                      encodedNormal = currentLine->VoxelData[(int)currentLineIndex].EncodedNormal;
                      redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * weight;

                      redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * weight;
                      greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * weight;
                      greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * weight;
                      blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * weight;
                      blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * weight;

                      if (!gradientOpacityIsConstant)
                      {
                        gradientMagnitude = currentLine->VoxelData[(int)currentLineIndex].GradientMagnitude;
                        sampledGradientMagnitude += float(gradientMagnitude) * weight;
                      }
                    }
                  }
                  else if (myThis->FunctionType == VTK_SHEAR_WARP_MIP_FUNCTION)
                  {
                    value = currentLine->VoxelData[(int)currentLineIndex].Value;
                    sampledValue += float(value) * weight;

                  }
                  else
                  {
                    value = currentLine->VoxelData[(int)currentLineIndex].Value;
                    sampledValue += float(value) * weight;

                    if (myThis->Shade)
                    {

                      encodedNormal = currentLine->VoxelData[(int)currentLineIndex].EncodedNormal;
                      redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * weight;
                      redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * weight;
                      greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * weight;
                      greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * weight;
                      blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * weight;
                      blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * weight;
                    }
                  }

                  currentLineIndex += myThis->ImageSampleDistance;
                }
              }
            }
            
            lineIndex[g] += viIncrement;
            
            if (lineIndex[g] >= line[g]->Length)
            {
              lineIndex[g] -= line[g]->Length;

              line[g]++;
            }              

          }

          if (voxels > 0)
          {
            if (myThis->FunctionType == VTK_SHEAR_WARP_COMPOSITE_FUNCTION)
            {
              sampledRed /= (float)voxels;
              sampledGreen /= (float)voxels;
              sampledBlue /= (float)voxels;
              sampledOpacity /= (float)voxels;
            
              // Alpha Compositing
              if (myThis->Shade)

              {                          
                  sampledGradientMagnitude /= voxels;

                if (sampledGradientMagnitude > 255.0f)
                  gradientOpacity = GOTF[255];

                else if (sampledGradientMagnitude < 0.0f)
                  gradientOpacity = GOTF[0];
                else
                  gradientOpacity = GOTF[(unsigned char) sampledGradientMagnitude];

                redDiffuse /= (float)voxels;
                redSpecular /= (float)voxels;
                greenDiffuse /= (float)voxels;
                greenSpecular /= (float)voxels;
                blueDiffuse /= (float)voxels;
                blueSpecular /= (float)voxels;
              
                sampledRed *= redDiffuse + redSpecular;
                sampledGreen *= greenDiffuse + greenSpecular;
                sampledBlue *= blueDiffuse + blueSpecular;
                sampledOpacity *= gradientOpacity;

              }

              newRed = oldRed + sampledOpacity * sampledRed * (1.0f - oldOpacity);
              newGreen = oldGreen + sampledOpacity * sampledGreen * (1.0f - oldOpacity);
              newBlue = oldBlue + sampledOpacity * sampledBlue * (1.0f - oldOpacity);
              newOpacity = oldOpacity + sampledOpacity * (1.0f - oldOpacity);
            
            }
            else if (myThis->FunctionType == VTK_SHEAR_WARP_MIP_FUNCTION)
            {
              sampledValue /= (float)voxels;
              
              // Maximum Intensity Projection
              if (sampledValue > pixels->Value)
              {
                newRed = CTF[int(sampledValue)*3+0];
                newGreen = CTF[int(sampledValue)*3+1];
                newBlue = CTF[int(sampledValue)*3+2];
                newOpacity = SOTF[int(sampledValue)];
                pixels->Value = sampledValue;
              }
              else
              {
                newRed = oldRed;
                newGreen = oldGreen;
                newBlue = oldBlue;
                newOpacity = oldOpacity;
              }
            }
            else
            {
              sampledValue /= (float)voxels;

              // Isosurface              
              if (sampledValue > myThis->IsoValue)
              {
                sampledRed = isoRed;
                sampledGreen = isoGreen;
                sampledBlue = isoBlue;
                
                if (myThis->Shade)
                {
                  redDiffuse /= (float)voxels;
                  redSpecular /= (float)voxels;
                  greenDiffuse /= (float)voxels;
                  greenSpecular /= (float)voxels;
                  blueDiffuse /= (float)voxels;
                  blueSpecular /= (float)voxels;


                  sampledRed *= redDiffuse + redSpecular;
                  sampledGreen *= greenDiffuse + greenSpecular;
                  sampledBlue *= blueDiffuse + blueSpecular;                    
                }

                newRed = sampledRed;
                newGreen = sampledGreen;
                newBlue = sampledBlue;
                newOpacity = 1.0f;
              }
              else
              {
                newRed = oldRed;
                newGreen = oldGreen;
                newBlue = oldBlue;
                newOpacity = oldOpacity;
              }
            }

            pixels->Red = newRed;
            pixels->Green = newGreen;
            pixels->Blue = newBlue;
            pixels->Opacity = newOpacity;

            if (newOpacity >= 1.0f)
            {
              // The current intermediate pixel is opaque, so exit
              // loop and skip opaque pixels
              pixels->Offset = 1;
            }
          }

          

          image->Advance(pixels,1);
          i++;
        }
      }
    }

    delete[] lineIndex;
    delete[] line;
  }
}



// Parallel projection shear-warp fast classification algorithm using nearest neighbour interpolation
template <class T>
void CompositeIntermediateNearestUnclassified(vtkShearWarpRLEImage *image, vtkVolumeShearWarpMapper *myThis)
{
  vtkShearWarpPixelData *pixels = 0;
  vtkShearWarpOctreeRun *runs = new vtkShearWarpOctreeRun[myThis->CountJ];
  vtkShearWarpOctreeRun *top;

  int topIndex;
  float sampledRed,sampledGreen,sampledBlue,sampledOpacity;
  float sampledValue;
  float sampledGradientMagnitude;
  float oldRed,oldGreen,oldBlue,oldOpacity;
  float newRed,newGreen,newBlue,newOpacity;
  float redDiffuse,greenDiffuse,blueDiffuse;
  float redSpecular,greenSpecular,blueSpecular;
  float gradientOpacity;

  float isoRed;
  float isoGreen;
  float isoBlue;

  int skipped;

  int i,j,k;
  int vi,vj,vk;

  float *SOTF;
  float *CTF;
  float *GTF;
  float *GOTF;
  float gradientOpacityConstant;
  int gradientOpacityIsConstant;
  unsigned short encodedNormal;
  unsigned char gradientMagnitude;

  float uSlice;
  float vSlice;

  int uSliceInteger;
  int vSliceInteger;

  T value;

  int kStart;
  int vkStart;
  int kEnd;
  int kIncrement;
  int viIncrement;
  int vjIncrement;
  int vkIncrement;
  int size;

  T *dptr = (T*) myThis->GetInput()->GetScalarPointer();

  unsigned short *nptr = myThis->GradientEstimator->GetEncodedNormals();
  unsigned char *gptr = myThis->GradientEstimator->GetGradientMagnitudes();

  int *dimensions = myThis->GetInput()->GetDimensions();
  int location;
  int plane = dimensions[0]*dimensions[1];

  vtkShearWarpOctree<T> *octree = dynamic_cast< vtkShearWarpOctree<T> * > (myThis->Octree);
  
  if (myThis->ReverseOrder)
  {
    kStart = myThis->CountK - 1;
    kEnd = -1;
    kIncrement = -1;
  }
  else
  {
    kStart = 0;
    kEnd = myThis->CountK;
    kIncrement = 1;
  }

  SOTF =  myThis->Volume->GetCorrectedScalarOpacityArray();
  CTF  =  myThis->Volume->GetRGBArray();
  GTF  =  myThis->Volume->GetGrayArray();
  GOTF =  myThis->Volume->GetGradientOpacityArray();
  gradientOpacityConstant = myThis->Volume->GetGradientOpacityConstant();

  if (gradientOpacityConstant > 0.0f)
    gradientOpacityIsConstant = 1;
  else
    gradientOpacityIsConstant = 0;


  if (myThis->FunctionType == VTK_SHEAR_WARP_ISOSURFACE_FUNCTION)
  {
    isoRed = CTF[int(myThis->IsoValue)*3 + 0];
    isoGreen = CTF[int(myThis->IsoValue)*3 + 1];
    isoBlue = CTF[int(myThis->IsoValue)*3 + 2];
  }

  switch (myThis->MajorAxis)
  {
    case VTK_X_AXIS:
      viIncrement = dimensions[0] * myThis->ImageSampleDistance;
      vjIncrement = plane * myThis->ImageSampleDistance;
      vkIncrement = kIncrement * myThis->ImageSampleDistance;
      vkStart = kStart * myThis->ImageSampleDistance;
      break;

    case VTK_Y_AXIS:
      viIncrement = plane * myThis->ImageSampleDistance;
      vjIncrement = myThis->ImageSampleDistance;
      vkIncrement = kIncrement * dimensions[0] * myThis->ImageSampleDistance;
      vkStart = kStart * dimensions[0] * myThis->ImageSampleDistance;
      break;

    case VTK_Z_AXIS:
      default:
      viIncrement = myThis->ImageSampleDistance;
      vjIncrement = dimensions[0] * myThis->ImageSampleDistance;
      vkIncrement = kIncrement * plane * myThis->ImageSampleDistance;
      vkStart = kStart * plane * myThis->ImageSampleDistance;
      break;
  }


  for (k = kStart, vk = vkStart; k != kEnd; k += kIncrement, vk += vkIncrement)
  {
    uSlice = k*myThis->ShearI + myThis->TranslationI;
    vSlice = k*myThis->ShearJ + myThis->TranslationJ;


    uSliceInteger = (int)ceil(uSlice) - 1;
    vSliceInteger = (int)ceil(vSlice) - 1;


    size = 0;

    // Composite one slice into the intermediate image
    for (j=0, vj = 0; j < myThis->CountJ; j++, vj += vjIncrement)
    {
      size -= 2 * myThis->ImageSampleDistance;

      if (size <= 0)
        size = octree->GetLineRuns(runs,myThis->MajorAxis, k*myThis->ImageSampleDistance, j*myThis->ImageSampleDistance);

      top = runs;
      topIndex = 0;

      image->Position(pixels,uSliceInteger + (vSliceInteger+j)*myThis->IntermediateWidth);

      for (i=0, vi = 0; i < myThis->CountI; )
      {
        // Update runs
        while (topIndex >= top->Length)
        {
          topIndex -= top->Length;
          top++;
        }

        // Skip opaque pixels in intermediate image
        skipped = image->Skip(pixels);

        // Update both runs if to be aligned with intermediate pixels
        if (skipped > 0)
        {
          i += skipped;
          vi += skipped * viIncrement;
          topIndex += skipped * myThis->ImageSampleDistance;
        }

        else
        {
          // Skip transparent voxels


          if (top->Type == VTK_SHEAR_WARP_OCTREE_TRANSPARENT)
          {
            while (topIndex < top->Length)
            {
              image->Advance(pixels,1);
              i++;
              vi += viIncrement;
              topIndex += myThis->ImageSampleDistance;
            }
          }
          else
          {

            // This loops samples voxels, performs shading and performs
            // compositing into the intermediate image
            while (topIndex < top->Length)
            {
              if (myThis->IntermixIntersectingGeometry)
              {
                float depth = myThis->IntermediateZBuffer[myThis->ImageSampleDistance * (uSliceInteger + i) + myThis->ImageSampleDistance * (vSliceInteger + j) * myThis->IntermediateWidth * myThis->ImageSampleDistance];

                if (myThis->ReverseOrder)
                {
                  if (k*myThis->ImageSampleDistance <= depth)
                    pixels->Offset = 1;
                }
                else
                {
                  if (k*myThis->ImageSampleDistance >= depth)
                    pixels->Offset = 1;
                }
              }

              // Only process non-opaque pixels
              if (pixels->Offset == 0)
              {
                if (myThis->IsVoxelClipped(i*myThis->ImageSampleDistance,j*myThis->ImageSampleDistance,k*myThis->ImageSampleDistance) == 1)
                {
                  image->Advance(pixels,1);
                  i++;
                  vi += viIncrement;
                  topIndex += myThis->ImageSampleDistance;
                  continue;
                }
                
                oldOpacity = pixels->Opacity;
                oldRed = pixels->Red;
                oldGreen = pixels->Green;
                oldBlue = pixels->Blue;

                location = vi + vj + vk;

                if (myThis->FunctionType == VTK_SHEAR_WARP_COMPOSITE_FUNCTION)
                {
                  sampledOpacity = 0.0f;
                  sampledRed = 0.0f;
                  sampledGreen = 0.0f;
                  sampledBlue = 0.0f;

                  value = dptr[location];
                  sampledOpacity += SOTF[value];
                  sampledRed += CTF[value*3 + 0];


                  sampledGreen += CTF[value*3 + 1];
                  sampledBlue += CTF[value*3 + 2];

                  if (myThis->Shade)
                  {
                    redDiffuse = 0.0f;
                    redSpecular = 0.0f;
                    blueDiffuse = 0.0f;
                    blueSpecular = 0.0f;
                    greenDiffuse = 0.0f;
                    greenSpecular = 0.0f;
                    sampledGradientMagnitude = 0.0f;
                    gradientOpacity = gradientOpacityConstant;

                    encodedNormal = nptr[location];

                    redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal];
                    redSpecular += myThis->RedSpecularShadingTable[encodedNormal];
                    greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal];
                    greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal];
                    blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal];
                    blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal];

                    if (!gradientOpacityIsConstant)
                    {
                      gradientMagnitude = gptr[location];
                      sampledGradientMagnitude += float(gradientMagnitude);

                      if (sampledGradientMagnitude > 255.0f)
                        gradientOpacity = GOTF[255];
                      else if (sampledGradientMagnitude < 0.0f)
                        gradientOpacity = GOTF[0];
                      else
                        gradientOpacity = GOTF[(unsigned char) sampledGradientMagnitude];
                    }

                    sampledRed *= redDiffuse + redSpecular;
                    sampledGreen *= greenDiffuse + greenSpecular;
                    sampledBlue *= blueDiffuse + blueSpecular;
                    sampledOpacity *= gradientOpacity;

                  }

                  // Alpha Compositing
                  newRed = oldRed + sampledOpacity * sampledRed * (1.0f - oldOpacity);
                  newGreen = oldGreen + sampledOpacity * sampledGreen * (1.0f - oldOpacity);
                  newBlue = oldBlue + sampledOpacity * sampledBlue * (1.0f - oldOpacity);
                  newOpacity = oldOpacity + sampledOpacity * (1.0f - oldOpacity);
                }
                else if (myThis->FunctionType == VTK_SHEAR_WARP_MIP_FUNCTION)
                {
                  sampledValue = 0.0f;

                  value = dptr[location];
                  sampledValue += float(value);

                  // Maximum Intensity Projection
                  if (sampledValue > pixels->Value)
                    {

                    newRed = CTF[int(sampledValue)*3+0];
                    newGreen = CTF[int(sampledValue)*3+1];
                    newBlue = CTF[int(sampledValue)*3+2];
                    newOpacity = SOTF[int(sampledValue)];
                    pixels->Value = sampledValue;
                  }
                  else

                  {
                    newRed = oldRed;
                    newGreen = oldGreen;
                    newBlue = oldBlue;
                    newOpacity = oldOpacity;
                  }
                }
                else
                {
                  sampledValue = 0.0f;

                  value = dptr[location];
                  sampledValue += float(value);


                  if (sampledValue >= myThis->IsoValue)
                  {
                    sampledRed = isoRed;

                    sampledGreen = isoGreen;
                    sampledBlue = isoBlue;

                    if (myThis->Shade)
                    {
                      redDiffuse = 0.0f;
                      redSpecular = 0.0f;
                      blueDiffuse = 0.0f;
                      blueSpecular = 0.0f;
                      greenDiffuse = 0.0f;
                      greenSpecular = 0.0f;

                      encodedNormal = nptr[location];

                      redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal];
                      redSpecular += myThis->RedSpecularShadingTable[encodedNormal];
                      greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal];
                      greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal];

                      blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal];
                      blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal];

                      sampledRed *= redDiffuse + redSpecular;
                      sampledGreen *= greenDiffuse + greenSpecular;
                      sampledBlue *= blueDiffuse + blueSpecular;
                    }

                    newRed = sampledRed;
                    newGreen = sampledGreen;
                    newBlue = sampledBlue;
                    newOpacity = 1.0f;
                  }
                  else
                  {
                    newRed = oldRed;
                    newGreen = oldGreen;
                    newBlue = oldBlue;
                    newOpacity = oldOpacity;
                  }
                }

                pixels->Red = newRed;

                pixels->Green = newGreen;
                pixels->Blue = newBlue;
                pixels->Opacity = newOpacity;

                if (newOpacity >= 1.0f)
                {
                  // The current intermediate pixel is opaque, so exit
                  // loop and skip opaque pixels
                  pixels->Offset = 1;
                  break;
                }

                image->Advance(pixels,1);
                i++;
                vi += viIncrement;
                topIndex += myThis->ImageSampleDistance;
              }
              else
              {
                // The current pixel has an offset greater than zero, so
                // we exit the loop and skip all opaque pixels
                break;
              }
            }
          }
        }
      }

    }
  }

  delete[] runs;
}


// Parallel projection shear-warp fast classification algorithm using bilinear interpolation
template <class T>
void CompositeIntermediateLinearUnclassified(vtkShearWarpRLEImage *image, vtkVolumeShearWarpMapper *myThis)
{
  vtkShearWarpPixelData *pixels = 0;
  vtkShearWarpOctreeRun *runs = new vtkShearWarpOctreeRun[myThis->CountJ];
  vtkShearWarpOctreeRun *top,*bottom;

  int topIndex, bottomIndex, topStart, bottomStart;

  float sampledRed,sampledGreen,sampledBlue,sampledOpacity;
  float sampledValue;
  float sampledGradientMagnitude;
  float oldRed,oldGreen,oldBlue,oldOpacity;
  float newRed,newGreen,newBlue,newOpacity;
  float redDiffuse,greenDiffuse,blueDiffuse;
  float redSpecular,greenSpecular,blueSpecular;
  float gradientOpacity;

  float isoRed;
  float isoGreen;

  float isoBlue;

  int skipped;
  int runLength;

  int i,j,k,h;
  int vi,vj,vk;

  float *SOTF;
  float *CTF;
  float *GTF;
  float *GOTF;
  float gradientOpacityConstant;
  int gradientOpacityIsConstant;
  unsigned short encodedNormal;
  unsigned char gradientMagnitude;

  float uSlice;
  float vSlice;

  int uSliceInteger;
  int vSliceInteger;

  float uSliceFractional;
  float vSliceFractional;

  float weightTopLeft;
  float weightBottomLeft;
  float weightTopRight;
  float weightBottomRight;

  float adjustedTL;
  float adjustedTR;
  float adjustedBL;
  float adjustedBR;

  T value;

  int kStart;
  int vkStart;
  int kEnd;
  int kIncrement;
  int viIncrement;
  int vjIncrement;
  int vkIncrement;
  int size;

  T *dptr = (T*) myThis->GetInput()->GetScalarPointer();

  unsigned short *nptr = myThis->GradientEstimator->GetEncodedNormals();
  unsigned char *gptr = myThis->GradientEstimator->GetGradientMagnitudes();

  int *dimensions = myThis->GetInput()->GetDimensions();
  int locationTL,locationTR,locationBL,locationBR;
  int plane = dimensions[0]*dimensions[1];

  vtkShearWarpOctree<T> *octree = dynamic_cast< vtkShearWarpOctree<T> * > (myThis->Octree);
  
  if (myThis->ReverseOrder)
  {
    kStart = myThis->CountK - 1;
    kEnd = -1;
    kIncrement = -1;
  }
  else
  {
    kStart = 0;
    kEnd = myThis->CountK;
    kIncrement = 1;
  }

  SOTF =  myThis->Volume->GetCorrectedScalarOpacityArray();
  CTF  =  myThis->Volume->GetRGBArray();
  GTF  =  myThis->Volume->GetGrayArray();
  GOTF =  myThis->Volume->GetGradientOpacityArray();
  gradientOpacityConstant = myThis->Volume->GetGradientOpacityConstant();

  if (gradientOpacityConstant > 0.0f)
    gradientOpacityIsConstant = 1;
  else
    gradientOpacityIsConstant = 0;


  if (myThis->FunctionType == VTK_SHEAR_WARP_ISOSURFACE_FUNCTION)
  {
    isoRed = CTF[int(myThis->IsoValue)*3 + 0];
    isoGreen = CTF[int(myThis->IsoValue)*3 + 1];
    isoBlue = CTF[int(myThis->IsoValue)*3 + 2];
  }

  switch (myThis->MajorAxis)
  {
    case VTK_X_AXIS:
      viIncrement = dimensions[0] * myThis->ImageSampleDistance;
      vjIncrement = plane * myThis->ImageSampleDistance;
      vkIncrement = kIncrement * myThis->ImageSampleDistance;
      vkStart = kStart * myThis->ImageSampleDistance;
      break;

    case VTK_Y_AXIS:
      viIncrement = plane * myThis->ImageSampleDistance;
      vjIncrement = myThis->ImageSampleDistance;
      vkIncrement = kIncrement * dimensions[0] * myThis->ImageSampleDistance;
      vkStart = kStart * dimensions[0] * myThis->ImageSampleDistance;
      break;

    case VTK_Z_AXIS:
      default:
      viIncrement = myThis->ImageSampleDistance;
      vjIncrement = dimensions[0] * myThis->ImageSampleDistance;
      vkIncrement = kIncrement * plane * myThis->ImageSampleDistance;
      vkStart = kStart * plane * myThis->ImageSampleDistance;
      break;
  }


  for (k = kStart, vk = vkStart; k != kEnd; k += kIncrement, vk += vkIncrement)
  {
    uSlice = k*myThis->ShearI + myThis->TranslationI;
    vSlice = k*myThis->ShearJ + myThis->TranslationJ;

    uSliceInteger = (int)ceil(uSlice) - 1;
    vSliceInteger = (int)ceil(vSlice) - 1;

    uSliceFractional = uSlice - uSliceInteger;
    vSliceFractional = vSlice - vSliceInteger;

    weightTopLeft = uSliceFractional * vSliceFractional;
    weightBottomLeft = uSliceFractional * (1.0f - vSliceFractional);
    weightTopRight = (1.0f - uSliceFractional) * vSliceFractional;
    weightBottomRight = (1.0f - uSliceFractional) * (1.0f - vSliceFractional);

    size = 0;

    // Composite one slice into the intermediate image
    for (j=0, vj = 0; j < myThis->CountJ; j++, vj += vjIncrement)
    {
      size -= myThis->ImageSampleDistance;

      if (size <= 0)
      {
        size = octree->GetLineRuns(runs,myThis->MajorAxis, k*myThis->ImageSampleDistance, j*myThis->ImageSampleDistance);
        size -= myThis->ImageSampleDistance;
      }

      top = runs;

      if ((j + 1) < myThis->CountJ)
      {
        size -= myThis->ImageSampleDistance;

        if (size <= 0)
        {
          size = octree->GetLineRuns(runs, myThis->MajorAxis, k*myThis->ImageSampleDistance, (j+1)*myThis->ImageSampleDistance);
          size -= myThis->ImageSampleDistance;
        }

        bottom = runs;
      }
      else
        bottom = NULL;

      topStart = 0;
      bottomStart = 0;

      topIndex = 0;
      bottomIndex = 0;

      image->Position(pixels,uSliceInteger + (vSliceInteger+j)*myThis->IntermediateWidth);

      for (i=0, vi = 0; i < myThis->CountI; )
      {
        // Update runs
        while (topIndex >= top->Length)
        {
          topIndex -= top->Length;
          topStart += top->Length;
          top++;
        }

        if (bottom != NULL)
        {
          while (bottomIndex >= bottom->Length)
          {
            bottomIndex -= bottom->Length;
            bottomStart += bottom->Length;
            bottom++;
          }
        }

        // Skip opaque pixels in intermediate image

        skipped = image->Skip(pixels);

        // Update both runs if to be aligned with intermediate pixels
        if (skipped > 0)
        {
          i += skipped;
          vi += skipped * viIncrement;
          topIndex += skipped * myThis->ImageSampleDistance;
          bottomIndex += skipped * myThis->ImageSampleDistance;
        }
        else
        {
          if (bottom != NULL)
          {
            if ( ((topStart + top->Length) - (topStart + topIndex)) < ((bottomStart + bottom->Length) - (bottomStart + bottomIndex)) )
              runLength = ((topStart + top->Length) - (topStart + topIndex));
            else
              runLength = ((bottomStart + bottom->Length) - (bottomStart + bottomIndex));
          }
          else
            runLength = top->Length - topIndex;

          // Skip transp1arent voxels in both runs

          if (top->Type == VTK_SHEAR_WARP_OCTREE_TRANSPARENT && (((bottom != NULL) ? bottom->Type : VTK_SHEAR_WARP_OCTREE_TRANSPARENT) == VTK_SHEAR_WARP_OCTREE_TRANSPARENT))
          {
            for (h = 0; h < runLength; h+=myThis->ImageSampleDistance)
            {
              image->Advance(pixels,1);
              i++;
              vi += viIncrement;
              topIndex += myThis->ImageSampleDistance;
              bottomIndex += myThis->ImageSampleDistance;
            }
          }
          else
          {
            // This loops samples voxels from both runs,
            // interpolates, performs shading and performs
            // compositing into the intermediate image
            for (h=0; h < runLength; h+= myThis->ImageSampleDistance)
            {
              if (myThis->IntermixIntersectingGeometry)
              {
                float depth = myThis->IntermediateZBuffer[myThis->ImageSampleDistance * (uSliceInteger + i) + myThis->ImageSampleDistance * (vSliceInteger + j) * myThis->IntermediateWidth * myThis->ImageSampleDistance];

                if (myThis->ReverseOrder)
                {
                  if (k*myThis->ImageSampleDistance <= depth)
                    pixels->Offset = 1;
                }
                else
                {
                  if (k*myThis->ImageSampleDistance >= depth)
                    pixels->Offset = 1;
                }

              }
              
              // Only process non-opaque pixels
              if (pixels->Offset == 0)
              {
                if (myThis->IsVoxelClipped(i*myThis->ImageSampleDistance,j*myThis->ImageSampleDistance,k*myThis->ImageSampleDistance) == 1)
                {
                  image->Advance(pixels,1);
                  i++;
                  vi += viIncrement;

                  topIndex += myThis->ImageSampleDistance;
                  bottomIndex += myThis->ImageSampleDistance;
                  continue;
                }
                
                oldOpacity = pixels->Opacity;
                oldRed = pixels->Red;
                oldGreen = pixels->Green;
                oldBlue = pixels->Blue;

                locationTL = vi + vj + vk;
                locationTR = locationTL + viIncrement;
                locationBL = locationTL + vjIncrement;
                locationBR = locationBL + viIncrement;

                if (myThis->FunctionType == VTK_SHEAR_WARP_COMPOSITE_FUNCTION)
                {
                  sampledOpacity = 0.0f;
                  sampledRed = 0.0f;
                  sampledGreen = 0.0f;
                  sampledBlue = 0.0f;

                  value = dptr[locationTL];
                  sampledOpacity += SOTF[value] * weightTopLeft;
                  sampledRed += CTF[value*3 + 0] * weightTopLeft;
                  sampledGreen += CTF[value*3 + 1] * weightTopLeft;
                  sampledBlue += CTF[value*3 + 2] * weightTopLeft;


                  if (i + 1 < myThis->CountI)
                  {
                    value = dptr[locationTR];
                    sampledOpacity += SOTF[value] * weightTopRight;
                    sampledRed += CTF[value*3 + 0] * weightTopRight;
                    sampledGreen += CTF[value*3 + 1] * weightTopRight;
                    sampledBlue += CTF[value*3 + 2] * weightTopRight;
                  }

                  if (j + 1 < myThis->CountJ)
                  {

                    value = dptr[locationBL];
                    sampledOpacity += SOTF[value] * weightBottomLeft;
                    sampledRed += CTF[value*3 + 0] * weightBottomLeft;
                    sampledGreen += CTF[value*3 + 1] * weightBottomLeft;
                    sampledBlue += CTF[value*3 + 2] * weightBottomLeft;

                    if (i + 1 < myThis->CountI)
                    {
                      value = dptr[locationBR];
                      sampledOpacity += SOTF[value] * weightBottomRight;

                      sampledRed += CTF[value*3 + 0] * weightBottomRight;
                      sampledGreen += CTF[value*3 + 1] * weightBottomRight;
                      sampledBlue += CTF[value*3 + 2] * weightBottomRight;
                    }
                  }

                  if (myThis->Shade)
                  {
                    redDiffuse = 0.0f;
                    redSpecular = 0.0f;
                    blueDiffuse = 0.0f;
                    blueSpecular = 0.0f;
                    greenDiffuse = 0.0f;
                    greenSpecular = 0.0f;
                    sampledGradientMagnitude = 0.0f;
                    gradientOpacity = gradientOpacityConstant;

                    encodedNormal = nptr[locationTL];

                    redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * weightTopLeft;
                    redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * weightTopLeft;
                    greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * weightTopLeft;

                    greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * weightTopLeft;
                    blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * weightTopLeft;
                    blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * weightTopLeft;

                    if (!gradientOpacityIsConstant)
                    {
                      gradientMagnitude = gptr[locationTL];
                      sampledGradientMagnitude += float(gradientMagnitude) * weightTopLeft;
                    }

                    if (i + 1 < myThis->CountI)
                    {
                      encodedNormal = nptr[locationTR];

                      redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * weightTopRight;
                      redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * weightTopRight;
                      greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * weightTopRight;
                      greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * weightTopRight;
                      blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * weightTopRight;
                      blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * weightTopRight;

                      if (!gradientOpacityIsConstant)
                      {
                        gradientMagnitude = gptr[locationTR];
                        sampledGradientMagnitude += float(gradientMagnitude) * weightTopRight;
                      }
                    }

                    if (j + 1 < myThis->CountJ)
                    {
                      encodedNormal = nptr[locationBL];

                      redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * weightBottomLeft;
                      redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * weightBottomLeft;
                      greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * weightBottomLeft;
                      greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * weightBottomLeft;
                      blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * weightBottomLeft;
                      blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * weightBottomLeft;

                      if (!gradientOpacityIsConstant)
                      {
                        gradientMagnitude = gptr[locationBL];
                        sampledGradientMagnitude += float(gradientMagnitude) * weightBottomLeft;
                      }

                      if (i + 1 < myThis->CountI)
                      {
                        encodedNormal = nptr[locationBR];

                        redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * weightBottomRight;
                        redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * weightBottomRight;
                        greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * weightBottomRight;
                        greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * weightBottomRight;
                        blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * weightBottomRight;
                        blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * weightBottomRight;

                        if (!gradientOpacityIsConstant)
                        {

                          gradientMagnitude = gptr[locationBR];
                          sampledGradientMagnitude += float(gradientMagnitude) * weightBottomRight;
                        }
                      }
                    }

                    if (!gradientOpacityIsConstant)
                    {
                      if (sampledGradientMagnitude > 255.0f)
                        gradientOpacity = GOTF[255];
                      else if (sampledGradientMagnitude < 0.0f)
                        gradientOpacity = GOTF[0];
                      else
                        gradientOpacity = GOTF[(unsigned char) sampledGradientMagnitude];
                    }

                    sampledRed *= redDiffuse + redSpecular;
                    sampledGreen *= greenDiffuse + greenSpecular;
                    sampledBlue *= blueDiffuse + blueSpecular;
                    sampledOpacity *= gradientOpacity;
                  }

                  // Alpha Compositing
                  newRed = oldRed + sampledOpacity * sampledRed * (1.0f - oldOpacity);
                  newGreen = oldGreen + sampledOpacity * sampledGreen * (1.0f - oldOpacity);
                  newBlue = oldBlue + sampledOpacity * sampledBlue * (1.0f - oldOpacity);
                  newOpacity = oldOpacity + sampledOpacity * (1.0f - oldOpacity);
                }
                else if (myThis->FunctionType == VTK_SHEAR_WARP_MIP_FUNCTION)
                {
                  sampledValue = 0.0f;

                  value = dptr[locationTL];
                  sampledValue += float(value) * weightTopLeft;

                  if (i + 1 < myThis->CountI)
                  {
                    value = dptr[locationTR];
                    sampledValue += float(value) * weightTopRight;
                  }

                  if (j + 1 < myThis->CountJ)
                  {
                    value = dptr[locationBL];
                    sampledValue += float(value) * weightBottomLeft;

                    if (i + 1 < myThis->CountI)
                    {
                      value = dptr[locationBR];
                      sampledValue += float(value) * weightBottomRight;

                    }
                  }

                  // Maximum Intensity Projection
                  if (sampledValue > pixels->Value)
                  {
                    newRed = CTF[int(sampledValue)*3+0];

                    newGreen = CTF[int(sampledValue)*3+1];
                    newBlue = CTF[int(sampledValue)*3+2];
                    newOpacity = SOTF[int(sampledValue)];
                    pixels->Value = sampledValue;

                  }
                  else
                  {
                    newRed = oldRed;
                    newGreen = oldGreen;
                    newBlue = oldBlue;
                    newOpacity = oldOpacity;
                  }
                }
                else
                {

                  sampledRed = isoRed;
                  sampledGreen = isoGreen;
                  sampledBlue = isoBlue;

                  sampledValue = 0.0f;

                  value = dptr[locationTL];
                  sampledValue += float(value) * weightTopLeft;

                  if (i + 1 < myThis->CountI)
                  {
                    value = dptr[locationTR];
                    sampledValue += float(value) * weightTopRight;
                  }

                  if (j + 1 < myThis->CountJ)
                  {
                    value = dptr[locationBL];
                    sampledValue += float(value) * weightBottomLeft;

                    if (i + 1 < myThis->CountI)
                    {
                      value = dptr[locationBR];
                      sampledValue += float(value) * weightBottomRight;
                    }
                  }


                  // Isosurface
                  if (sampledValue >= myThis->IsoValue)
                  {
                    if (myThis->Shade)
                    {
                      redDiffuse = 0.0f;
                      redSpecular = 0.0f;
                      blueDiffuse = 0.0f;
                      blueSpecular = 0.0f;
                      greenDiffuse = 0.0f;
                      greenSpecular = 0.0f;

                      adjustedTL = weightTopLeft;
                      adjustedBL = weightBottomLeft;
                      adjustedTR = weightTopRight;
                      adjustedBR = weightBottomRight;

                      if (i + 1 >= myThis->CountI)
                      {
                        adjustedTL += adjustedTR;
                        adjustedBL += adjustedBR;
                      }

                      if (j + 1 >= myThis->CountJ)
                      {
                        adjustedTL += adjustedBL;
                        adjustedTR += adjustedBR;

                      }


                      encodedNormal = nptr[locationTL];

                      redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * adjustedTL;
                      redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * adjustedTL;
                      greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * adjustedTL;
                      greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * adjustedTL;
                      blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * adjustedTL;
                      blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * adjustedTL;

                      if (i + 1 < myThis->CountI)
                      {
                        encodedNormal = nptr[locationTR];

                        redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * adjustedTR;
                        redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * adjustedTR;
                        greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * adjustedTR;
                        greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * adjustedTR;
                        blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * adjustedTR;
                        blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * adjustedTR;
                      }

                      if (j + 1 < myThis->CountJ)
                      {
                        encodedNormal = nptr[locationBL];

                        redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * adjustedBL;
                        redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * adjustedBL;
                        greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * adjustedBL;
                        greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * adjustedBL;
                        blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * adjustedBL;
                        blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * adjustedBL;


                        if (i + 1 < myThis->CountI)
                        {
                          encodedNormal = nptr[locationBR];

                          redDiffuse += myThis->RedDiffuseShadingTable[encodedNormal] * adjustedBR;
                          redSpecular += myThis->RedSpecularShadingTable[encodedNormal] * adjustedBR;
                          greenDiffuse += myThis->GreenDiffuseShadingTable[encodedNormal] * adjustedBR;
                          greenSpecular += myThis->GreenSpecularShadingTable[encodedNormal] * adjustedBR;
                          blueDiffuse += myThis->BlueDiffuseShadingTable[encodedNormal] * adjustedBR;
                          blueSpecular += myThis->BlueSpecularShadingTable[encodedNormal] * adjustedBR;

                        }
                      }

                      sampledRed *= redDiffuse + redSpecular;
                      sampledGreen *= greenDiffuse + greenSpecular;
                      sampledBlue *= blueDiffuse + blueSpecular;


                    }


                    newRed = sampledRed;
                    newGreen = sampledGreen;
                    newBlue = sampledBlue;
                    newOpacity = 1.0f;
                  }
                  else
                  {
                    newRed = oldRed;
                    newGreen = oldGreen;
                    newBlue = oldBlue;
                    newOpacity = oldOpacity;
                  }

                }

                pixels->Red = newRed;
                pixels->Green = newGreen;
                pixels->Blue = newBlue;
                pixels->Opacity = newOpacity;

                if (newOpacity >= 1.0f)
                {
                  // The current intermediate pixel is opaque, so exit
                  // loop and skip opaque pixels
                  pixels->Offset = 1;
                  break;
                }


                image->Advance(pixels,1);
                i++;
                vi += viIncrement;
                topIndex += myThis->ImageSampleDistance;
                bottomIndex += myThis->ImageSampleDistance;
              }
              else
              {
                // The current pixel has an offset greater than zero, so
                // we exit the loop and skip all opaque pixels
                break;
              }
            }
          }
        }
      }
    }
  }

  delete[] runs;  
}


vtkVolumeShearWarpMapper* vtkVolumeShearWarpMapper::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret=vtkGraphicsFactory::CreateInstance("vtkVolumeShearWarpMapper");
  return (vtkVolumeShearWarpMapper*)ret;
}


vtkVolumeShearWarpMapper::vtkVolumeShearWarpMapper()
{
  this->ImageSampleDistance        =  1;
  this->MinimumImageSampleDistance =  1;
  this->MaximumImageSampleDistance =  4;
  this->AutoAdjustSampleDistances  =  1;

  this->PerspectiveMatrix      = vtkMatrix4x4::New();
  this->ViewToWorldMatrix      = vtkMatrix4x4::New();
  this->ViewToVoxelsMatrix     = vtkMatrix4x4::New();
  this->VoxelsToViewMatrix     = vtkMatrix4x4::New();
  this->WorldToVoxelsMatrix    = vtkMatrix4x4::New();

  this->VoxelsToWorldMatrix    = vtkMatrix4x4::New();
  this->VoxelTransformMatrix   = vtkMatrix4x4::New();
  this->ViewportMatrix         = vtkMatrix4x4::New();
  this->ShearMatrix            = vtkMatrix4x4::New();
  this->WarpMatrix             = vtkMatrix4x4::New();
  this->PermutationMatrix      = vtkMatrix4x4::New();
  this->PermutedViewToVoxelsMatrix = vtkMatrix4x4::New();
  this->PermutedVoxelsToViewMatrix = vtkMatrix4x4::New();
  this->PerspectiveTransform   = vtkTransform::New();

  this->EncodedVolume          = NULL;
  this->Octree                 = NULL;

  this->ImageData              = NULL;
  this->ImageWidth             = 0;
  this->ImageHeight            = 0;
  this->AllocatedSize          = 0;

  this->IntemediateImage       = NULL;
  this->IntermediateWidth      = 0;
  this->IntermediateHeight     = 0;
  this->ScalarOpacityMTime     = 0;

  this->RenderTimeTable        = NULL;
  this->RenderVolumeTable      = NULL;
  this->RenderRendererTable    = NULL;
  this->RenderTableSize        = 0;  
  this->RenderTableEntries     = 0;

  this->GradientEstimator      = vtkFiniteDifferenceGradientEstimator::New();
  this->GradientShader         = vtkEncodedGradientShader::New();

  this->IsoValue               = 0.0f;


  this->RunlengthEncoding      = 0;
  this->FastClassification     = 0;
  this->Shade                  = 0;
  this->ParallelProjection     = 0;
  this->MyPerspectiveProjection     = 0;

  this->ZBuffer                = NULL;
  this->IntermediateZBuffer    = NULL;
  this->IntermixIntersectingGeometry = 0;

  /*
  this->FrontDepth = 1.0;
  this->DepthDensity = 2.0;
  this->DeltaDepth = 1.0 / 255.0;
  this->DepthTableSize = 0;
  
  ComputeDepthTable(0,511);
  */
  this->Debug = 0;
}

void vtkVolumeShearWarpMapper::SetGradientEstimator(
                                       vtkEncodedGradientEstimator *gradest )
{

  // If we are setting it to its current value, don't do anything
  if ( this->GradientEstimator == gradest )
    {
    return;
    }

  // If we already have a gradient estimator, unregister it.
  if ( this->GradientEstimator )
    {
    this->GradientEstimator->UnRegister(this);
    this->GradientEstimator = NULL;
    }

  // If we are passing in a non-NULL estimator, register it
  if ( gradest )
    {
    gradest->Register( this );
    }

  // Actually set the estimator, and consider the object Modified
  this->GradientEstimator = gradest;
  this->Modified();
}

vtkVolumeShearWarpMapper::~vtkVolumeShearWarpMapper()
{

  this->PerspectiveMatrix->Delete();
  this->ViewToWorldMatrix->Delete();
  this->ViewToVoxelsMatrix->Delete();
  this->VoxelsToViewMatrix->Delete();
  this->WorldToVoxelsMatrix->Delete();
  this->VoxelsToWorldMatrix->Delete();
  this->VoxelTransformMatrix->Delete();
  this->ViewportMatrix->Delete();
  this->ShearMatrix->Delete();
  this->WarpMatrix->Delete();
  this->PermutationMatrix->Delete();
  this->PermutedViewToVoxelsMatrix->Delete();
  this->PerspectiveTransform->Delete();

  this->SetGradientEstimator( NULL );
  this->GradientShader->Delete();

  if (this->EncodedVolume != NULL)

    delete this->EncodedVolume;

  if (this->IntemediateImage != NULL)
    delete this->IntemediateImage;

  if (this->ImageData != NULL)
    delete[] this->ImageData;

  if (this->Octree != NULL)
    delete this->Octree;
}

void vtkVolumeShearWarpMapper::Update()
{
  if ( this->GetInput() )
    {
    this->GetInput()->UpdateInformation();
    this->GetInput()->SetUpdateExtentToWholeExtent();
    this->GetInput()->Update();
    }

}

void vtkVolumeShearWarpMapper::Render( vtkRenderer *ren,
                                       vtkVolume *vol )
{

  this->Volume = vol;

  // make sure that we have scalar input and update the scalar input
  if ( this->GetInput() == NULL )
    {
    vtkErrorMacro(<< "No Input!");

    return;
    }
  else
    {
    this->GetInput()->UpdateInformation();
    this->GetInput()->SetUpdateExtentToWholeExtent();
    this->GetInput()->Update();
    }

  // Start timing now. We didn't want to capture the update of the
  // input data in the times
  this->Timer->StartTimer();

  vol->UpdateTransferFunctions( ren );

  ren->ComputeAspect();
  double *aspect = ren->GetAspect();
  vtkCamera *cam = ren->GetActiveCamera();

  // Keep track of the projection matrix - we'll need it in a couple of places
  // Get the projection matrix. The method is called perspective, but
  // the matrix is valid for perspective and parallel viewing transforms.
  // Don't replace this with the GetCompositePerspectiveTransformMatrix because that

  // turns off stereo rendering!!!
  this->PerspectiveTransform->SetMatrix(cam->GetPerspectiveTransformMatrix(aspect[0]/aspect[1],
                                                        0.0, 1.0 ));
  this->PerspectiveTransform->Concatenate(cam->GetViewTransformMatrix());
  this->PerspectiveMatrix->DeepCopy(this->PerspectiveTransform->GetMatrix());

 
  // Compute some matrices from voxels to view and vice versa based
  // on the whole input

  this->VoxelTransformMatrix->DeepCopy( vol->GetMatrix() );



  this->ComputeMatrices( this->GetInput(), vol );
  this->ParallelProjection = cam->GetParallelProjection();

    // How big is the viewport in pixels?
  double *viewport   =  ren->GetViewport();

  int *renWinSize   =  ren->GetRenderWindow()->GetSize();

  // Save this so that we can restore it if the image is cancelled
  int oldImageSampleDistance = this->ImageSampleDistance;

  // If we are automatically adjusting the size to achieve a desired frame
  // rate, then do that adjustment here. Base the new image sample distance
  // on the previous one and the previous render time. Don't let
  // the adjusted image sample distance be less than the minimum image sample
  // distance or more than the maximum image sample distance.
  if ( this->AutoAdjustSampleDistances )
    {
    float oldTime = this->RetrieveRenderTime( ren, vol );
    float newTime = vol->GetAllocatedRenderTime();
    this->ImageSampleDistance = (this->ImageSampleDistance * int(0.5+(sqrt(oldTime / newTime))));//int(rint(sqrt(oldTime / newTime))));
    this->ImageSampleDistance =
      (this->ImageSampleDistance>this->MaximumImageSampleDistance)?
      (this->MaximumImageSampleDistance):(this->ImageSampleDistance);
    this->ImageSampleDistance =
      (this->ImageSampleDistance<this->MinimumImageSampleDistance)?

      (this->MinimumImageSampleDistance):(this->ImageSampleDistance);
    }

  vol->UpdateScalarOpacityforSampleSize( ren, this->ImageSampleDistance );

  // The full image fills the viewport. First, compute the actual viewport
  // size, then divide by the ImageSampleDistance to find the full image
  // size in pixels
  this->ImageViewportSize[0] = static_cast<int>
    (static_cast<float>(renWinSize[0]) * (viewport[2]-viewport[0]));
  this->ImageViewportSize[1] = static_cast<int>
    (static_cast<float>(renWinSize[1]) * (viewport[3]-viewport[1]));

  this->ImageViewportSize[0] =
    static_cast<int>( static_cast<float>(this->ImageViewportSize[0]) );
  this->ImageViewportSize[1] =
    static_cast<int>( static_cast<float>(this->ImageViewportSize[1]) );

/*    
  this->ImageViewportSize[0] =
    static_cast<int>( static_cast<float>(this->ImageViewportSize[0]) /
                      static_cast<float>(this->ImageSampleDistance) );
  this->ImageViewportSize[1] =
    static_cast<int>( static_cast<float>(this->ImageViewportSize[1])  /
                      static_cast<float>(this->ImageSampleDistance) );
*/
  vol->UpdateTransferFunctions( ren );

  vol->UpdateScalarOpacityforSampleSize( ren, this->ImageSampleDistance );

  this->Shade =  vol->GetProperty()->GetShade();

  this->GradientEstimator->SetInput( this->GetInput() );

  if ( this->Shade )
    {
    this->GradientShader->UpdateShadingTable( ren, vol,
                                              this->GradientEstimator );
    this->EncodedNormals =
      this->GradientEstimator->GetEncodedNormals();

    this->RedDiffuseShadingTable =
      this->GradientShader->GetRedDiffuseShadingTable(vol);
    this->GreenDiffuseShadingTable =
      this->GradientShader->GetGreenDiffuseShadingTable(vol);
    this->BlueDiffuseShadingTable =
      this->GradientShader->GetBlueDiffuseShadingTable(vol);

    this->RedSpecularShadingTable =
      this->GradientShader->GetRedSpecularShadingTable(vol);
    this->GreenSpecularShadingTable =
      this->GradientShader->GetGreenSpecularShadingTable(vol);
    this->BlueSpecularShadingTable =
      this->GradientShader->GetBlueSpecularShadingTable(vol);
    }
  else
    {

    this->EncodedNormals = NULL;
    this->RedDiffuseShadingTable = NULL;
    this->GreenDiffuseShadingTable = NULL;
    this->BlueDiffuseShadingTable = NULL;
    this->RedSpecularShadingTable = NULL;
    this->GreenSpecularShadingTable = NULL;
    this->BlueSpecularShadingTable = NULL;
    }

  // If we have non-constant opacity on the gradient magnitudes,
  // we need to use the gradient magnitudes to look up the opacity
  if ( vol->GetGradientOpacityConstant() == -1.0 )
    {
    this->GradientMagnitudes =
      this->GradientEstimator->GetGradientMagnitudes();
    }
  else
    {
    this->GradientMagnitudes = NULL;
    }

  float bounds[6];
  int dim[3];


  this->GetInput()->GetDimensions(dim);
  bounds[0] = bounds[2] = bounds[4] = 0.0;
  bounds[1] = static_cast<float>(dim[0]-1);
  bounds[3] = static_cast<float>(dim[1]-1);
  bounds[5] = static_cast<float>(dim[2]-1);

  float voxelPoint[3];
  float viewPoint[8][4];
  int i, j, k;
//  unsigned char *ucptr;
  float minX, minY, maxX, maxY, minZ, maxZ;

  minX =  1.0;
  minY =  1.0;
  maxX = -1.0;
  maxY = -1.0;
  minZ =  1.0;
  maxZ =  0.0;
    
  double camPos[3];
  double worldBounds[6];
  vol->GetBounds( worldBounds );
  int insideFlag = 0;
  ren->GetActiveCamera()->GetPosition( camPos );
  if ( camPos[0] >= worldBounds[0] &&
       camPos[0] <= worldBounds[1] &&
       camPos[1] >= worldBounds[2] &&
       camPos[1] <= worldBounds[3] &&
       camPos[2] >= worldBounds[4] &&
       camPos[2] <= worldBounds[5] )
    {
    insideFlag = 1;
    }



  // Copy the voxelsToView matrix to 16 floats
  float voxelsToViewMatrix[16];
  for ( j = 0; j < 4; j++ )
    {
    for ( i = 0; i < 4; i++ )
      {
      voxelsToViewMatrix[j*4+i] =
        static_cast<float>(this->VoxelsToViewMatrix->GetElement(j,i));
      }
    }

  // Convert the voxel bounds to view coordinates to find out the
  // size and location of the image we need to generate.
  int idx = 0;
  if ( insideFlag )
    {
    minX = -1.0;
    maxX =  1.0;
    minY = -1.0;
    maxY =  1.0;
    minZ =  0.001;
    maxZ =  0.001;
    }
  else

    {
    for ( k = 0; k < 2; k++ )
      {
      voxelPoint[2] = bounds[4+k];
      for ( j = 0; j < 2; j++ )
        {
        voxelPoint[1] = bounds[2+j];
        for ( i = 0; i < 2; i++ )
          {
          voxelPoint[0] = bounds[i];
          vtkVSWMultiplyPointMacro( voxelPoint, viewPoint[idx],
                                   voxelsToViewMatrix );

          minX = (viewPoint[idx][0]<minX)?(viewPoint[idx][0]):(minX);
          minY = (viewPoint[idx][1]<minY)?(viewPoint[idx][1]):(minY);
          maxX = (viewPoint[idx][0]>maxX)?(viewPoint[idx][0]):(maxX);
          maxY = (viewPoint[idx][1]>maxY)?(viewPoint[idx][1]):(maxY);
          minZ = (viewPoint[idx][2]<minZ)?(viewPoint[idx][2]):(minZ);
          maxZ = (viewPoint[idx][2]>maxZ)?(viewPoint[idx][2]):(maxZ);
          idx++;
          }
        }
      }
    }

  if ( minZ < 0.001 || maxZ > 0.9999 )
    {
    minX = -1.0;
    maxX =  1.0;
    minY = -1.0;
    maxY =  1.0;
    insideFlag = 1;
    }
    
  this->MinimumViewDistance =
    (minZ<0.001)?(0.001):((minZ>0.999)?(0.999):(minZ));    

  if ( !ren->GetRenderWindow()->GetAbortRender() )
    this->FactorViewMatrix();

  if ( this->ClippingPlanes )
    this->InitializeClippingPlanes( this->ClippingPlanes );
    
  if (this->IntermixIntersectingGeometry == 1)
  {
    if ( !ren->GetRenderWindow()->GetAbortRender() )
      this->ExtractZBuffer(ren,vol);
  }


  if ( !ren->GetRenderWindow()->GetAbortRender() )
    this->CompositeIntermediate();

  if ( !ren->GetRenderWindow()->GetAbortRender() )
    this->RenderTexture(ren,vol);

  if ( !ren->GetRenderWindow()->GetAbortRender() )
      {
      this->Timer->StopTimer();
      this->TimeToDraw = this->Timer->GetElapsedTime();
      this->StoreRenderTime( ren, vol, this->TimeToDraw );
      }
    // Restore the image sample distance so that automatic adjustment
    // will work correctly
    else
      {
      this->ImageSampleDistance = oldImageSampleDistance;
      }

}


#if 0
vtkVolumeShearWarpMapper *vtkVolumeShearWarpMapper::New()
{

  // First try to create the object from the vtkObjectFactory
  vtkObject* ret =
    vtkGraphicsFactory::CreateInstance("vtkVolumeShearWarpMapper");

  if (ret != NULL)
    return (vtkVolumeShearWarpMapper*)ret;
  
  return vtkOpenGLVolumeShearWarpMapper::New();
}
#endif

void vtkVolumeShearWarpMapper::ComputeMatrices( vtkImageData *data,
                                                 vtkVolume *vol )
{
  // Get the data spacing. This scaling is not accounted for in
  // the volume's matrix, so we must add it in.
  double volumeSpacing[3];
  data->GetSpacing( volumeSpacing );

  // Get the origin of the data.  This translation is not accounted for in
  // the volume's matrix, so we must add it in.
  double volumeOrigin[3];
  data->GetOrigin( volumeOrigin );

  // Get the dimensions of the data.
  int volumeDimensions[3];
  data->GetDimensions( volumeDimensions );

  // Create some transform objects that we will need later
  vtkTransform *voxelsTransform = vtkTransform::New();
  vtkTransform *voxelsToViewTransform = vtkTransform::New();

  // Get the volume matrix. This is a volume to world matrix right now.
  // We'll need to invert it, translate by the origin and scale by the
  // spacing to change it to a world to voxels matrix.
  vtkMatrix4x4 *volMatrix = vtkMatrix4x4::New();
  volMatrix->DeepCopy( vol->GetMatrix() );

  voxelsToViewTransform->SetMatrix( volMatrix );

  // Create a transform that will account for the scaling and translation of
  // the scalar data. The is the volume to voxels matrix.
  voxelsTransform->Identity();
  voxelsTransform->Translate(volumeOrigin[0],
                             volumeOrigin[1],
                             volumeOrigin[2] );

  voxelsTransform->Scale( volumeSpacing[0],

                          volumeSpacing[1],
                          volumeSpacing[2] );



  // Now concatenate the volume's matrix with this scalar data matrix
  voxelsToViewTransform->PreMultiply();
  voxelsToViewTransform->Concatenate( voxelsTransform->GetMatrix() );

  // Now we actually have the world to voxels matrix - copy it out
  this->WorldToVoxelsMatrix->DeepCopy( voxelsToViewTransform->GetMatrix() );
  this->WorldToVoxelsMatrix->Invert();

  // We also want to invert this to get voxels to world
  this->VoxelsToWorldMatrix->DeepCopy( voxelsToViewTransform->GetMatrix() );


  // Compute the voxels to view transform by concatenating the
  // voxels to world matrix with the projection matrix (world to view)

  voxelsToViewTransform->PostMultiply();
  voxelsToViewTransform->Concatenate( this->PerspectiveMatrix );



  this->VoxelsToViewMatrix->DeepCopy( voxelsToViewTransform->GetMatrix() );

  this->ViewToVoxelsMatrix->DeepCopy( this->VoxelsToViewMatrix );
  this->ViewToVoxelsMatrix->Invert();

  voxelsToViewTransform->Delete();
  voxelsTransform->Delete();
  volMatrix->Delete();
}

float vtkVolumeShearWarpMapper::RetrieveRenderTime( vtkRenderer *ren,
                                                  vtkVolume   *vol )
{
  int i;

  for ( i = 0; i < this->RenderTableEntries; i++ )
    {
    if ( this->RenderVolumeTable[i] == vol &&
         this->RenderRendererTable[i] == ren )
      {
      return this->RenderTimeTable[i];
      }
    }

  return 0.0;
}

void vtkVolumeShearWarpMapper::StoreRenderTime( vtkRenderer *ren,
                                              vtkVolume   *vol,
                                              float       time )
{
  int i;
  for ( i = 0; i < this->RenderTableEntries; i++ )

    {
    if ( this->RenderVolumeTable[i] == vol &&
         this->RenderRendererTable[i] == ren )
      {
      this->RenderTimeTable[i] = time;
      return;
      }
    }


  // Need to increase size
  if ( this->RenderTableEntries >= this->RenderTableSize )
    {
    if ( this->RenderTableSize == 0 )
      {
      this->RenderTableSize = 10;
      }
    else
      {
      this->RenderTableSize *= 2;
      }

    float       *oldTimePtr     = this->RenderTimeTable;
    vtkVolume   **oldVolumePtr   = this->RenderVolumeTable;

    vtkRenderer **oldRendererPtr = this->RenderRendererTable;

    this->RenderTimeTable     = new float [this->RenderTableSize];
    this->RenderVolumeTable   = new vtkVolume *[this->RenderTableSize];
    this->RenderRendererTable = new vtkRenderer *[this->RenderTableSize];

    for (i = 0; i < this->RenderTableEntries; i++ )
      {
      this->RenderTimeTable[i] = oldTimePtr[i];
      this->RenderVolumeTable[i] = oldVolumePtr[i];
      this->RenderRendererTable[i] = oldRendererPtr[i];
      }

    delete [] oldTimePtr;
    delete [] oldVolumePtr;
    delete [] oldRendererPtr;
    }

  this->RenderTimeTable[this->RenderTableEntries] = time;
  this->RenderVolumeTable[this->RenderTableEntries] = vol;
  this->RenderRendererTable[this->RenderTableEntries] = ren;

  this->RenderTableEntries++;
}

void vtkVolumeShearWarpMapper::CompositeIntermediate()
{
  int scalarType = this->GetInput()->GetScalarType();
  int interpolationType = this->Volume->GetProperty()->GetInterpolationType();


  this->ImageWidth = 32;
  this->ImageHeight = 32;

  while (this->ImageWidth < this->MaximumIntermediateDimension)
    this->ImageWidth = this->ImageWidth << 1;

  while (this->ImageHeight < this->MaximumIntermediateDimension)
    this->ImageHeight = this->ImageHeight << 1;

  int imageSize = this->ImageWidth * this->ImageHeight;

  if (imageSize > this->AllocatedSize)
  {
    this->AllocatedSize = imageSize;

    if (this->ImageData != NULL)
      delete[] this->ImageData;

    this->ImageData = new unsigned char[imageSize*4];

    if (this->IntemediateImage != NULL)
      delete this->IntemediateImage;

    this->IntemediateImage = new vtkShearWarpRLEImage(imageSize);
  }
  else
    this->IntemediateImage->Clear();

  if (this->RunlengthEncoding == 1)
  {
    if (this->FunctionType == VTK_SHEAR_WARP_ISOSURFACE_FUNCTION)
    {
      if (this->EncodedVolume != NULL)
      {
        if (this->EncodedVolume->IsScalarEncoded())
        {
          if (this->EncodedVolume->GetIsoValue() != this->IsoValue)
          {
            delete this->EncodedVolume;
            this->EncodedVolume = NULL;
          }
        }
        else
        {
          delete this->EncodedVolume;
          this->EncodedVolume = NULL;
        }
      }


      if (this->EncodedVolume == NULL)
      {
        switch ( scalarType )
        {
          case VTK_UNSIGNED_CHAR:
          {
            vtkShearWarpRLEVolume<unsigned char> *encodedVolume = new vtkShearWarpRLEVolume<unsigned char>();
            encodedVolume->encodeScalar(this->GetInput(),this->Volume,this->GradientEstimator,this->IsoValue);
            this->EncodedVolume = encodedVolume;
            break;
          }
          case VTK_UNSIGNED_SHORT:
          {
            vtkShearWarpRLEVolume<unsigned short> *encodedVolume = new vtkShearWarpRLEVolume<unsigned short>();
            encodedVolume->encodeScalar(this->GetInput(),this->Volume,this->GradientEstimator,this->IsoValue);
            this->EncodedVolume = encodedVolume;            
            break;
          }
        }        
      }

      switch ( scalarType )
      {
        case VTK_UNSIGNED_CHAR:
          if ( interpolationType == VTK_NEAREST_INTERPOLATION )
            CompositeIntermediateNearestRLE<unsigned char>(this->IntemediateImage,this);
          else
            CompositeIntermediateLinearRLE<unsigned char>(this->IntemediateImage,this);

          break;

        case VTK_UNSIGNED_SHORT:
          if ( interpolationType == VTK_NEAREST_INTERPOLATION )

            CompositeIntermediateNearestRLE<unsigned short>(this->IntemediateImage,this);
          else
//        if(ParallelProjection)
          CompositeIntermediateLinearRLE<unsigned short>(this->IntemediateImage,this);
//        else
//          CompositeIntermediateLinearRLEPerspective<unsigned short>(this->IntemediateImage,this);

          break;
      }      
    }
    else
    {
      // If the scalar opacity transfer function has been modified
      // the runlength encoding has to be redone
      unsigned long scalarOpacityMTime = this->Volume->GetProperty()->GetScalarOpacity()->GetMTime();

      if (this->EncodedVolume != NULL)
      {
        if ( scalarOpacityMTime > this->ScalarOpacityMTime)
        {
          delete this->EncodedVolume;
          this->EncodedVolume = NULL;
        }
      }

      if (this->EncodedVolume == NULL)
      {
        this->ScalarOpacityMTime =  scalarOpacityMTime;

        switch ( scalarType )
        {
          case VTK_UNSIGNED_CHAR:
          {
            vtkShearWarpRLEVolume<unsigned char> *encodedVolume = new vtkShearWarpRLEVolume<unsigned char>();
            encodedVolume->encodeOpacity(this->GetInput(),this->Volume,this->GradientEstimator,0.0f);
            this->EncodedVolume = encodedVolume;
            break;
          }

            
          case VTK_UNSIGNED_SHORT:
          {
            vtkShearWarpRLEVolume<unsigned short> *encodedVolume = new vtkShearWarpRLEVolume<unsigned short>();

            encodedVolume->encodeOpacity(this->GetInput(),this->Volume,this->GradientEstimator,0.0f);
            this->EncodedVolume = encodedVolume;
            break;
          }
        }
      }

      switch ( scalarType )
      {
        case VTK_UNSIGNED_CHAR:
          if ( interpolationType == VTK_NEAREST_INTERPOLATION )
            CompositeIntermediateNearestRLE<unsigned char>(this->IntemediateImage,this);
          else
            CompositeIntermediateLinearRLE<unsigned char>(this->IntemediateImage,this);

          break;

        case VTK_UNSIGNED_SHORT:
          if ( interpolationType == VTK_NEAREST_INTERPOLATION )
            CompositeIntermediateNearestRLE<unsigned short>(this->IntemediateImage,this);
          else
//        if(ParallelProjection)
          CompositeIntermediateLinearRLE<unsigned short>(this->IntemediateImage,this);
//        else
//          CompositeIntermediateLinearRLEPerspective<unsigned short>(this->IntemediateImage,this);

          break;
      }      
    }
  }
  else if (this->FastClassification == 1)
  {
    if (this->EncodedVolume != NULL)
    {
      delete this->EncodedVolume;
      this->EncodedVolume = NULL;
    }    

    if (this->Octree == NULL)
    {
      switch ( scalarType )
      {
        case VTK_UNSIGNED_CHAR:
          this->Octree = new vtkShearWarpOctree<unsigned char>();
          dynamic_cast< vtkShearWarpOctree<unsigned char> *>(this->Octree)->build(this->GetInput());
          break;
        case VTK_UNSIGNED_SHORT:
          this->Octree = new vtkShearWarpOctree<unsigned short>();
          dynamic_cast< vtkShearWarpOctree<unsigned short> *>(this->Octree)->build(this->GetInput());
          break;
      }
    }

    if (this->FunctionType == VTK_SHEAR_WARP_ISOSURFACE_FUNCTION)
    {
      if (!this->Octree->IsScalarEncoded() || this->Octree->GetIsoValue() != this->IsoValue)
      {        
        switch ( scalarType )
        {
          case VTK_UNSIGNED_CHAR:
            dynamic_cast< vtkShearWarpOctree<unsigned char> *>(this->Octree)->classifyScalar((unsigned char)this->IsoValue);
            break;
          case VTK_UNSIGNED_SHORT:
            dynamic_cast< vtkShearWarpOctree<unsigned short> *>(this->Octree)->classifyScalar((unsigned short)this->IsoValue);
            break;
        }
      }        
    }
    else
    {      
      unsigned long scalarOpacityMTime = this->Volume->GetProperty()->GetScalarOpacity()->GetMTime();

      if ( scalarOpacityMTime > this->ScalarOpacityMTime)
      {
        this->ScalarOpacityMTime =  scalarOpacityMTime;

        switch ( scalarType )
        {
          case VTK_UNSIGNED_CHAR:
            dynamic_cast< vtkShearWarpOctree<unsigned char> *>(this->Octree)->classifyOpacity(this->Volume);
            break;
          case VTK_UNSIGNED_SHORT:
            dynamic_cast< vtkShearWarpOctree<unsigned short> *>(this->Octree)->classifyOpacity(this->Volume);
            break;
        }
      }
    }    
    
    if ( interpolationType == VTK_NEAREST_INTERPOLATION )
    {
      // Nearest neighbor
      switch ( scalarType )
      {
        case VTK_UNSIGNED_CHAR:
          CompositeIntermediateNearestUnclassified<unsigned char>(this->IntemediateImage,this);
          break;
        case VTK_UNSIGNED_SHORT:
          CompositeIntermediateNearestUnclassified<unsigned short>(this->IntemediateImage,this);
          break;
      }
    }
    else
    {
      // Linear interpolation
      switch ( scalarType )
      {

        case VTK_UNSIGNED_CHAR:
          CompositeIntermediateLinearUnclassified<unsigned char>(this->IntemediateImage,this);
          break;
        case VTK_UNSIGNED_SHORT:
          CompositeIntermediateLinearUnclassified<unsigned short>(this->IntemediateImage,this);
          break;
      }
    }
  }
  else
  {
    if ( interpolationType == VTK_NEAREST_INTERPOLATION )
    {
      // Nearest neighbor
      switch ( scalarType )
      {
        case VTK_UNSIGNED_CHAR:
          CompositeIntermediateNearestSimple<unsigned char>(this->IntemediateImage,this);
          break;
        case VTK_UNSIGNED_SHORT:
          CompositeIntermediateNearestSimple<unsigned short>(this->IntemediateImage,this);
          break;
      }
    }
    else
    {
      // Linear interpolation
      switch ( scalarType )
      {
        case VTK_UNSIGNED_CHAR:
          CompositeIntermediateLinearSimple<unsigned char>(this->IntemediateImage,this);
          break;
        case VTK_UNSIGNED_SHORT:

          CompositeIntermediateLinearSimple<unsigned short>(this->IntemediateImage,this);
          break;
      }
    }
  }
  
  BuildImage(this->ImageData,this->IntemediateImage->GetPixelData());
  
  bool Debug_IntermediateImage = true;
  if(Debug_IntermediateImage)
  {
    FILE *fp;
    fp = fopen("c:\\temp\\output.raw", "wb+");
    fwrite(this->ImageData, 4, this->ImageWidth * this->ImageHeight, fp);
    fclose(fp);
  }

}


void vtkVolumeShearWarpMapper::BuildImage(unsigned char *id, vtkShearWarpPixelData *im)
{
  int i,j;
  float red,green,blue,opacity;

  int startX = 0;//int(float(this->ImageWidth) * 0.5f - float(this->IntermediateWidth) * 0.5f);
  int startY = 0;//int(float(this->ImageHeight) * 0.5f - float(this->IntermediateHeight) * 0.5f);

  for (j = 0; j < this->ImageHeight; j++)

  {
    for (i = 0; i < this->ImageWidth; i++)
    {
      red = 0.0f;
      
      green = 0.0f;
      blue = 0.0f;
      opacity = 0.0f;

      if (/*i >= startX && j >= startY && */i < startX+this->IntermediateWidth && j < startY+this->IntermediateHeight)
      {
        red = im->Red;
        green = im->Green;
        blue = im->Blue;
        opacity = im->Opacity;

        if (red > 1.0f)
          red = 1.0f;
        else if (red < 0.0f)
          red = 0.0f;

        if (green > 1.0f)

          green = 1.0f;
        else if (green < 0.0f)
          green = 0.0f;

        if (blue > 1.0f)
          blue = 1.0f;
        else if (blue < 0.0f)
          blue = 0.0f;

        if (opacity > 1.0f)
          opacity = 1.0f;
        else if (opacity < 0.0f)
          opacity = 0.0f;

        im++;
      }
      /*
      else
      {
        red = 1.0f;
        green = 0.0f;
        blue = 0.0f;
        opacity = 1.0f;
      }
      */
      id[0] = (unsigned char) (255.0f * red);
      id[1] = (unsigned char) (255.0f * green);
      id[2] = (unsigned char) (255.0f * blue);
      id[3] = (unsigned char) (255.0f * opacity);

      id += 4;
    }
  }
}

void vtkVolumeShearWarpMapper::ExtractZBuffer(vtkRenderer *ren, vtkVolume *vol)
{
  float iposition[4][2];
  float itranslation[2];
  float isx, isy;
  float ix,iy;
  float w00,w01,w10,w11;  
  
  int *renWinSize   =  ren->GetRenderWindow()->GetSize();

//  float *viewport   =  ren->GetViewport();

  // The coefficients of the 2D warp matrix
  w00 = WarpMatrix->Element[0][0];
  w01 = WarpMatrix->Element[0][1];
  w10 = WarpMatrix->Element[1][0];
  w11 = WarpMatrix->Element[1][1];

  ix =  (float) this->vtkVolumeShearWarpMapper::IntermediateWidth * this->ImageSampleDistance;
  iy =  (float) this->vtkVolumeShearWarpMapper::IntermediateHeight * this->ImageSampleDistance;


  // Intermediate
  iposition[0][0] = 0.0f * w00 + 0.0f * w01;
  iposition[0][1] = 0.0f * w10 + 0.0f * w11;

  iposition[1][0] = ix * w00 + 0.0f * w01;
  iposition[1][1] = ix * w10 + 0.0f * w11;

  iposition[2][0] = ix * w00 + iy * w01;
  iposition[2][1] = ix * w10 + iy * w11;

  iposition[3][0] = 0.0f * w00 + iy * w01;
  iposition[3][1] = 0.0f * w10 + iy * w11;

  // Intermediate
  itranslation[0] = ix * 0.5f * w00 + iy * 0.5f * w01;
  itranslation[1] = ix * 0.5f * w10 + iy * 0.5f * w11;

  // Intermediate
  isx =  1;//(float) this->vtkVolumeShearWarpMapper::ImageSampleDistance;
  isy =  1;//(float) this->vtkVolumeShearWarpMapper::ImageSampleDistance;

  double *t = vol->GetCenter();
  float a[4] = {t[0],t[1],t[2],1.0f};
  float b[4];


  this->PerspectiveMatrix->MultiplyPoint(a,b);
  b[2] = 0.0f;

  float x1 = isx*(iposition[0][0]-itranslation[0]) + b[0] * (float) renWinSize[0] * 0.5f;
  float x2 = isx*(iposition[1][0]-itranslation[0]) + b[0] * (float) renWinSize[0] * 0.5f;
  float x3 = isx*(iposition[2][0]-itranslation[0]) + b[0] * (float) renWinSize[0] * 0.5f;
  float x4 = isx*(iposition[3][0]-itranslation[0]) + b[0] * (float) renWinSize[0] * 0.5f;

  float y1 = isy*(iposition[0][1]-itranslation[1]) + b[1] * (float) renWinSize[1] * 0.5f;
  float y2 = isy*(iposition[1][1]-itranslation[1]) + b[1] * (float) renWinSize[1] * 0.5f;
  float y3 = isy*(iposition[2][1]-itranslation[1]) + b[1] * (float) renWinSize[1] * 0.5f;
  float y4 = isy*(iposition[3][1]-itranslation[1]) + b[1] * (float) renWinSize[1] * 0.5f;



  x1 = x1 + renWinSize[0] * 0.5f;
  y1 = y1 + renWinSize[1] * 0.5f;


  x2 = x2 + renWinSize[0] * 0.5f;
  y2 = y2 + renWinSize[1] * 0.5f;

  x3 = x3 + renWinSize[0] * 0.5f;
  y3 = y3 + renWinSize[1] * 0.5f;


  x4 = x4 + renWinSize[0] * 0.5f;
  y4 = y4 + renWinSize[1] * 0.5f;

  float minx = x1;
  float miny = y1;
  float maxx = x1;
  float maxy = y1;

  if (x2 < minx)
    minx = x2;

  if (y2 < miny)
    miny = y2;

  if (x3 < minx)
    minx = x3;

  if (y3 < miny)
    miny = y3;

  if (x4 < minx)
    minx = x4;

  if (y4 < miny)
    miny = y4;


  if (x2 > maxx)

    maxx = x2;

  if (y2 > maxy)
    maxy = y2;

  if (x3 > maxx)
    maxx = x3;

  if (y3 > maxy)
    maxy = y3;

  if (x4 > maxx)
    maxx = x4;

  if (y4 > maxy)
    maxy = y4;

  int left = 0;
  int top = 0;
    
  if (minx < 0)
  {
    left = (int) -minx;
    minx = 0;
  }

  if (miny < 0)
  {
    top = (int) -miny;
    miny = 0;
  }

  if (maxx > renWinSize[0] - 1)
    maxx = renWinSize[0] - 1;

  if (maxy > renWinSize[1] - 1)
    maxy = renWinSize[1] - 1;

  int zx1 = int(minx + 0.5f);
  int zy1 = int(miny + 0.5f);
  int zx2 = int(maxx - 0.5f);
  int zy2 = int(maxy - 0.5f);


  this->ZBuffer = ren->GetRenderWindow()->GetZbufferData(zx1,zy1,zx2,zy2);
  this->ZBufferSize[0] = zx2 - zx1 + 1;
  this->ZBufferSize[1] = zy2 - zy1 + 1;

  this->IntermediateZBuffer = new float[(this->ImageSampleDistance*this->IntermediateWidth) * (this->ImageSampleDistance*this->IntermediateHeight)];
  
//  this->Unwarp(this->IntermediateZBuffer,this->IntermediateWidth,this->IntermediateHeight,this->ZBuffer,this->ZBufferSize[0],this->ZBufferSize[1],this->WarpMatrix);
  this->Unwarp(this->IntermediateZBuffer, this->ImageSampleDistance*this->IntermediateWidth, this->ImageSampleDistance*this->IntermediateHeight, this->ZBuffer, left, top, this->ZBufferSize[0],this->ZBufferSize[1],this->WarpMatrix);
        
}

void vtkVolumeShearWarpMapper::Unwarp(float *destination, int dWidth, int dHeight, float *source, int left, int top, int sWidth, int sHeight, vtkMatrix4x4* w)

{
  float xs, ys;                 // source image coordinates
  float xsMin,ysMin;
  int i, j;                   // counters
  float xd, yd;               // precomputed destination values
  float pc;                   // perspective correction
  float inv00, inv01, inv03;  // elements of 1st row of inverted warp matrix
  float inv10, inv11, inv13;  // elements of 2nd row of inverted warp matrix
  float inv30, inv31, inv33;  // elements of 3rd row of inverted warp matrix
   
  inv00 = w->Element[0][0];
  inv01 = w->Element[0][1];
  inv03 = w->Element[0][3];
  inv10 = w->Element[1][0];
  inv11 = w->Element[1][1];
  inv13 = w->Element[1][3];
  inv30 = w->Element[3][0];
  inv31 = w->Element[3][1];
  inv33 = w->Element[3][3];

  xsMin = 4096;
  ysMin = 4096;

  for (j=0; j < dHeight; j++)
  {
    yd = (float)j;
    for (i=0; i < dWidth; i++)
    {
      xd = (float)i;
      pc = xd * inv30 + yd * inv31 + inv33;
      xs = ((xd * inv00 + yd * inv01 + inv03)) / pc;    
      ys = ((xd * inv10 + yd * inv11 + inv13)) / pc;

      if (xs < xsMin)
        xsMin = xs;


      if (ys < ysMin)
        ysMin = ys;
    }
  }
  
  for (j=0; j < dHeight; j++)
  {
    yd = (float)j;
    for (i=0; i < dWidth; i++)
    {
      xd = (float)i;
      pc = xd * inv30 + yd * inv31 + inv33;

      xs = ((xd * inv00 + yd * inv01 + inv03)) / pc;
      ys = ((xd * inv10 + yd * inv11 + inv13)) / pc;
           
      xs -= xsMin;
      ys -= ysMin;

      
      xs -= left;
      ys -= top;
      
      float depth;

      // Check if pixel is inside image
      if (xs > sWidth - 1 || ys > sHeight - 1 || xs < 0 || ys < 0)
        destination[i + j * dWidth] = 0.0f;
      else
      {
        depth = source[int(xs) + int(ys) * sWidth];
//        memcpy(destination + (i + j * dWidth), source + (int(xs) + int(ys) * sWidth), sizeof(float));
//        memcpy(destination + (i + j * dWidth), source + (int(xs) + int(ys) * sWidth), sizeof(float));
        depth = depth * this->PermutedViewToVoxelsMatrix->Element[2][2] + this->PermutedViewToVoxelsMatrix->Element[2][3];
        destination[i + j * dWidth] = depth;                
      }
    }        
  }
}

void vtkVolumeShearWarpMapper::InitializeClippingPlanes( vtkPlaneCollection *planes )
{
  vtkPlane *onePlane;
  double planePoint[4];
  double normalPoint[4];
  float *clippingPlane;
  float d;
  int i;  

  this->ClippingPlaneCount = planes->GetNumberOfItems();

  if (this->ClippingPlaneCount == 0)
    return;
 
  // loop through all the clipping planes
  for ( i = 0; i < this->ClippingPlaneCount; i++ )
  {    
    onePlane = (vtkPlane *)planes->GetItemAsObject(i);

    onePlane->GetOrigin(planePoint);
    onePlane->GetNormal(normalPoint);
    normalPoint[0] += planePoint[0];
    normalPoint[1] += planePoint[1];
    normalPoint[2] += planePoint[2];
    planePoint[3] = 1.0;
    normalPoint[3] = 1.0;

    this->WorldToVoxelsMatrix->MultiplyPoint(planePoint,planePoint);
    this->WorldToVoxelsMatrix->MultiplyPoint(normalPoint,normalPoint);

    this->PermutationMatrix->MultiplyPoint(planePoint,planePoint);
    this->PermutationMatrix->MultiplyPoint(normalPoint,normalPoint);

    clippingPlane = this->ClippingPlane + 4*i;
    clippingPlane[0] = normalPoint[0] - planePoint[0]; 
    clippingPlane[1] = normalPoint[1] - planePoint[1];
    clippingPlane[2] = normalPoint[2] - planePoint[2];

    d = sqrt(clippingPlane[0] * clippingPlane[0] + clippingPlane[1] * clippingPlane[1] + clippingPlane[2] * clippingPlane[2]);
    clippingPlane[0] /= d;
    clippingPlane[1] /= d;
    clippingPlane[2] /= d;

    clippingPlane[3] = clippingPlane[0] * planePoint[0] + clippingPlane[1] * planePoint[1] + clippingPlane[2] * planePoint[2];    
  }
}

int vtkVolumeShearWarpMapper::IsVoxelClipped(int x, int y, int z)
{
  float *clippingPlane;

  for (int i=0; i<this->ClippingPlaneCount; i++)
  {
    clippingPlane = this->ClippingPlane + 4*i;
  
    if (clippingPlane[0] * (float)x +
        clippingPlane[1] * (float)y +
        clippingPlane[2] * (float)z < clippingPlane[3])
      return 1;
  }

  return 0;  
}

// Print the vtkVolumeShearWarpMapper
void vtkVolumeShearWarpMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkVolumeMapper::PrintSelf(os,indent);
}




