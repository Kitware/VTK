/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageResliceMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the side copyright notice for more information.

=========================================================================*/
#include "vtkImageResliceMapper.h"

#include "vtkImageSliceMapper.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkImageSlice.h"
#include "vtkImageData.h"
#include "vtkImageProperty.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkPoints.h"
#include "vtkMatrix4x4.h"
#include "vtkLinearTransform.h"
#include "vtkPlane.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkImageResliceToColors.h"
#include "vtkAbstractImageInterpolator.h"
#include "vtkObjectFactory.h"

// A tolerance to compensate for roundoff errors
#define VTK_RESLICE_MAPPER_VOXEL_TOL 7.62939453125e-06

vtkStandardNewMacro(vtkImageResliceMapper);

//----------------------------------------------------------------------------
vtkImageResliceMapper::vtkImageResliceMapper()
{
  this->SliceMapper = vtkImageSliceMapper::New();
  this->ImageReslice = vtkImageResliceToColors::New();
  this->ResliceMatrix = vtkMatrix4x4::New();
  this->WorldToDataMatrix = vtkMatrix4x4::New();
  this->SliceToWorldMatrix = vtkMatrix4x4::New();

  this->JumpToNearestSlice = 0;
  this->AutoAdjustImageQuality = 1;
  this->SeparateWindowLevelOperation = 1;
  this->SlabType = VTK_IMAGE_SLAB_MEAN;
  this->SlabThickness = 0.0;
  this->SlabSampleFactor = 2;
  this->ImageSampleFactor = 1;
  this->ResampleToScreenPixels = 1;
  this->InternalResampleToScreenPixels = 0;
  this->ResliceNeedUpdate = 0;

  // streaming requires an output port
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkImageResliceMapper::~vtkImageResliceMapper()
{
  if (this->SliceMapper)
  {
    this->SliceMapper->Delete();
  }
  if (this->ImageReslice)
  {
    this->ImageReslice->Delete();
  }
  if (this->ResliceMatrix)
  {
    this->ResliceMatrix->Delete();
  }
  if (this->WorldToDataMatrix)
  {
    this->WorldToDataMatrix->Delete();
  }
  if (this->SliceToWorldMatrix)
  {
    this->SliceToWorldMatrix->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkImageResliceMapper::SetSlicePlane(vtkPlane *plane)
{
  if (this->SlicePlane == plane)
  {
    return;
  }
  if (this->SlicePlane)
  {
    this->SlicePlane->Delete();
  }
  if (!plane)
  {
    this->SlicePlane = vtkPlane::New();
  }
  else
  {
    this->SlicePlane = plane;
    plane->Register(this);
  }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkImageResliceMapper::SetInterpolator(
  vtkAbstractImageInterpolator *interpolator)
{
  vtkMTimeType mtime = this->ImageReslice->GetMTime();

  this->ImageReslice->SetInterpolator(interpolator);

  if (this->ImageReslice->GetMTime() > mtime)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkAbstractImageInterpolator *vtkImageResliceMapper::GetInterpolator()
{
  return this->ImageReslice->GetInterpolator();
}

//----------------------------------------------------------------------------
void vtkImageResliceMapper::ReleaseGraphicsResources(vtkWindow *win)
{
  this->SliceMapper->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------------
void vtkImageResliceMapper::Render(vtkRenderer *ren, vtkImageSlice *prop)
{
  if (this->ResliceNeedUpdate)
  {
    this->ImageReslice->SetInputConnection(
      this->GetInputConnection(0, 0));
    this->ImageReslice->UpdateWholeExtent();
    this->ResliceNeedUpdate = 0;
  }

  // apply checkerboard pattern (should have timestamps)
  vtkImageProperty *property = prop->GetProperty();
  if (property && property->GetCheckerboard() &&
      this->InternalResampleToScreenPixels &&
      !this->SeparateWindowLevelOperation &&
      this->SliceFacesCamera)
  {
    this->CheckerboardImage(this->ImageReslice->GetOutput(),
      ren->GetActiveCamera(), property);
  }

  // delegate to vtkImageSliceMapper
  this->SliceMapper->SetInputConnection(
    this->ImageReslice->GetOutputPort());
  this->SliceMapper->GetDataToWorldMatrix()->DeepCopy(
    this->SliceToWorldMatrix);
  // the mapper uses SliceFacesCamera to decide whether to use a polygon
  // for the texture versus using a quad the size of the window
  this->SliceMapper->SetSliceFacesCamera(
    (this->SliceFacesCamera && !this->SeparateWindowLevelOperation));
  this->SliceMapper->SetExactPixelMatch(this->InternalResampleToScreenPixels);
  this->SliceMapper->SetBorder( (this->Border ||
                                 this->InternalResampleToScreenPixels) );
  this->SliceMapper->SetBackground( (this->Background &&
    !(this->SliceFacesCamera && this->InternalResampleToScreenPixels &&
      !this->SeparateWindowLevelOperation) ) );
  this->SliceMapper->SetPassColorData(!this->SeparateWindowLevelOperation);
  this->SliceMapper->SetDisplayExtent(this->ImageReslice->GetOutputExtent());

  // render pass info for members of vtkImageStack
  this->SliceMapper->MatteEnable = this->MatteEnable;
  this->SliceMapper->ColorEnable = this->ColorEnable;
  this->SliceMapper->DepthEnable = this->DepthEnable;

  // let vtkImageSliceMapper do the rest of the work
  this->SliceMapper->SetNumberOfThreads(this->NumberOfThreads);
  this->SliceMapper->SetClippingPlanes(this->ClippingPlanes);
  this->SliceMapper->Render(ren, prop);
}

//----------------------------------------------------------------------------
void vtkImageResliceMapper::Update(int port)
{
  // I don't like to override Update, or call Modified() in Update,
  // but this allows updates to be forced where MTimes can't be used
  bool resampleToScreenPixels = (this->ResampleToScreenPixels != 0);
  vtkRenderer *ren = 0;

  if (this->AutoAdjustImageQuality && resampleToScreenPixels)
  {
    // only use image-size texture if image is smaller than render window,
    // since otherwise there is far less advantage in doing so
    vtkImageSlice *prop = this->GetCurrentProp();
    ren = this->GetCurrentRenderer();
    if (ren && prop)
    {
      int *rsize = ren->GetSize();
      int maxrsize = (rsize[0] > rsize[1] ? rsize[0] : rsize[1]);
      int *isize = this->GetInput()->GetDimensions();
      int maxisize = (isize[0] > isize[1] ? isize[0] : isize[1]);
      maxisize = (isize[2] > maxisize ? isize[2] : maxisize);
      if (maxisize <= maxrsize && maxisize <= 1024)
      {
        resampleToScreenPixels = (prop->GetAllocatedRenderTime() >= 1.0);
      }
    }
  }

  if (resampleToScreenPixels)
  {
    // force update if quality has increased to "ResampleToScreenPixels"
    if (!this->InternalResampleToScreenPixels)
    {
      this->Modified();
    }
    else
    {
      // force update if renderer size has changes, since the texture
      // size is equal to the renderer size for "ResampleToScreenPixels"
      if (!ren)
      {
        ren = this->GetCurrentRenderer();
      }
      if (ren)
      {
        int *extent = this->ImageReslice->GetOutputExtent();
        int *size = ren->GetSize();
        if (size[0] != (extent[1] - extent[0] + 1) ||
            size[1] != (extent[3] - extent[2] + 1))
        {
          this->Modified();
        }
      }
    }
  }
  else if (this->InternalResampleToScreenPixels)
  {
    // if execution reaches this point in the code, then the
    // rendering has just switched to interactive quality, and it is
    // necessary to force update if modified since the last update
    if (this->GetMTime() > this->UpdateTime.GetMTime())
    {
      this->Modified();
    }
    else
    {
      // don't switch yet: wait until the camera changes position,
      // which will cause the MTime to change
      resampleToScreenPixels = true;
    }
  }

  this->InternalResampleToScreenPixels = resampleToScreenPixels;

  // Always update if something else caused the input to update
  vtkImageData *input = this->GetInput();
  if (input && input->GetUpdateTime() > this->UpdateTime.GetMTime())
  {
    this->Modified();
  }

  this->Superclass::Update(port);
  this->UpdateTime.Modified();
}

//----------------------------------------------------------------------------
void vtkImageResliceMapper::Update()
{
  this->Superclass::Update();
}

//----------------------------------------------------------------------------
int vtkImageResliceMapper::Update(
  int port, vtkInformationVector*)
{
  // One can't really make requests of a mapper so default to regular
  // update.
  this->Update(port);
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageResliceMapper::Update(vtkInformation*)
{
  // One can't really make requests of a mapper so default to regular
  // update.
  this->Update();
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageResliceMapper::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    // use superclass method to update some info
    this->Superclass::ProcessRequest(request, inputVector, outputVector);

    // need the prop and renderer
    vtkImageSlice *prop = this->GetCurrentProp();
    vtkRenderer *ren = this->GetCurrentRenderer();

    if (ren && prop)
    {
      vtkImageProperty *property = prop->GetProperty();

      // Get point/normal from camera
      if (this->SliceFacesCamera || this->SliceAtFocalPoint)
      {
        vtkCamera *camera = ren->GetActiveCamera();

        if (this->SliceFacesCamera)
        {
          double normal[3];
          camera->GetDirectionOfProjection(normal);
          normal[0] = -normal[0];
          normal[1] = -normal[1];
          normal[2] = -normal[2];
          this->SlicePlane->SetNormal(normal);
        }

        if (this->SliceAtFocalPoint)
        {
          double point[4];
          camera->GetFocalPoint(point);

          if (this->JumpToNearestSlice)
          {
            double normal[4];
            this->SlicePlane->GetNormal(normal);
            normal[3] = -vtkMath::Dot(point, normal);
            point[3] = 1.0;

            // convert normal to data coordinates
            double worldToData[16];
            vtkMatrix4x4 *dataToWorld = this->GetDataToWorldMatrix();
            vtkMatrix4x4::Transpose(*dataToWorld->Element, worldToData);
            vtkMatrix4x4::MultiplyPoint(worldToData, normal, normal);

            // find the slice orientation from the normal
            int k = 0;
            double maxsq = 0;
            double sumsq = 0;
            for (int i = 0; i < 3; i++)
            {
              double tmpsq = normal[i]*normal[i];
              sumsq += tmpsq;
              if (tmpsq > maxsq)
              {
                maxsq = tmpsq;
                k = i;
              }
            }

            // if the slice is not oblique
            if ((1.0 - maxsq/sumsq) < 1e-12)
            {
              // get the point in data coordinates
              vtkMatrix4x4::Invert(*dataToWorld->Element, worldToData);
              vtkMatrix4x4::MultiplyPoint(worldToData, point, point);

              // set the point to lie exactly on a slice
              double z = (point[k] - this->DataOrigin[k])/this->DataSpacing[k];
              if (z > VTK_INT_MIN && z < VTK_INT_MAX)
              {
                int j = vtkMath::Floor(z + 0.5);
                point[k] = j*this->DataSpacing[k] + this->DataOrigin[k];
              }

              // convert back to world coordinates
              dataToWorld->MultiplyPoint(point, point);
            }
          }

          this->SlicePlane->SetOrigin(point);
        }

      } // end of "Get point/normal from camera"

      // set the matrices
      this->UpdateResliceMatrix(ren, prop);

      // update the coords for the polygon to be textured
      this->UpdatePolygonCoords(ren);

      // set the reslice spacing/origin/extent/axes
      this->UpdateResliceInformation(ren);

      // set the reslice bits related to the property
      this->UpdateResliceInterpolation(property);

      // update anything related to the image coloring
      this->UpdateColorInformation(property);
    }

    // set the number of threads to use when executing
    this->ImageReslice->SetNumberOfThreads(this->NumberOfThreads);

    // delegate request to vtkImageReslice (generally not a good thing to
    // do, but I'm familiar with the vtkImageReslice code that gets called).
    return this->ImageReslice->ProcessRequest(
      request, inputVector, outputVector);
  }

  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    if (this->Streaming)
    {
      // delegate request to vtkImageReslice (generally not a good thing to
      // do, but I'm familiar with the vtkImageReslice code that gets called).
      return this->ImageReslice->ProcessRequest(
        request, inputVector, outputVector);
    }
    else
    {
      vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
      int ext[6];
      inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext);
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext, 6);
    }
    return 1;
  }

  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_DATA()))
  {
    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    vtkImageData *output = vtkImageData::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));

    // set output extent to avoid re-execution
    output->GetInformation()->Set(vtkDataObject::DATA_EXTENT(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()), 6);

    // do an update of Reslice on the next render
    this->ResliceNeedUpdate = 1;

    return 1;
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
// Update the WorldToData transformation matrix, which is just the
// inverse of the vtkProp3D matrix.
void vtkImageResliceMapper::UpdateWorldToDataMatrix(vtkImageSlice *prop)
{
  // copy the matrix, but only if it has changed (we do this to
  // preserve the modified time of the matrix)
  double tmpmat[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
  if (!prop->GetIsIdentity())
  {
    vtkMatrix4x4::Invert(*prop->GetMatrix()->Element, tmpmat);
  }
  double *mat = *this->WorldToDataMatrix->Element;
  for (int i = 0; i < 16; i++)
  {
    if (mat[i] != tmpmat[i])
    {
      this->WorldToDataMatrix->DeepCopy(tmpmat);
      break;
    }
  }
}

//----------------------------------------------------------------------------
// Update the SliceToWorld transformation matrix
void vtkImageResliceMapper::UpdateSliceToWorldMatrix(vtkCamera *camera)
{
  // Get slice plane in world coords by passing null as the prop matrix
  double plane[4];
  this->GetSlicePlaneInDataCoords(0, plane);

  // Make sure normal is facing towards camera
  vtkMatrix4x4 *viewMatrix = camera->GetViewTransformMatrix();
  double *ndop = viewMatrix->Element[2];
  if (vtkMath::Dot(ndop, plane) < 0)
  {
    plane[0] = -plane[0];
    plane[1] = -plane[1];
    plane[2] = -plane[2];
    plane[3] = -plane[3];
  }

  // The normal is the first three elements
  double *normal = plane;

  // The last element is -dot(normal, origin)
  double dp = -plane[3];

  // Compute rotation angle between camera axis and slice normal
  double vec[3];
  vtkMath::Cross(ndop, normal, vec);
  double costheta = vtkMath::Dot(ndop, normal);
  double sintheta = vtkMath::Norm(vec);
  double theta = atan2(sintheta, costheta);
  if (sintheta != 0)
  {
    vec[0] /= sintheta;
    vec[1] /= sintheta;
    vec[2] /= sintheta;
  }
  // convert to quaternion
  costheta = cos(0.5*theta);
  sintheta = sin(0.5*theta);
  double quat[4];
  quat[0] = costheta;
  quat[1] = vec[0]*sintheta;
  quat[2] = vec[1]*sintheta;
  quat[3] = vec[2]*sintheta;
  // convert to matrix
  double mat[3][3];
  vtkMath::QuaternionToMatrix3x3(quat, mat);

  // Create a slice-to-world transform matrix
  // The columns are v1, v2, normal
  vtkMatrix4x4 *sliceToWorld = this->SliceToWorldMatrix;

  double v1[3], v2[3];
  vtkMath::Multiply3x3(mat, viewMatrix->Element[0], v1);
  vtkMath::Multiply3x3(mat, viewMatrix->Element[1], v2);

  sliceToWorld->Element[0][0] = v1[0];
  sliceToWorld->Element[1][0] = v1[1];
  sliceToWorld->Element[2][0] = v1[2];
  sliceToWorld->Element[3][0] = 0.0;

  sliceToWorld->Element[0][1] = v2[0];
  sliceToWorld->Element[1][1] = v2[1];
  sliceToWorld->Element[2][1] = v2[2];
  sliceToWorld->Element[3][1] = 0.0;

  sliceToWorld->Element[0][2] = normal[0];
  sliceToWorld->Element[1][2] = normal[1];
  sliceToWorld->Element[2][2] = normal[2];
  sliceToWorld->Element[3][2] = 0.0;

  sliceToWorld->Element[0][3] = -dp*normal[0];
  sliceToWorld->Element[1][3] = -dp*normal[1];
  sliceToWorld->Element[2][3] = dp-dp*normal[2];
  sliceToWorld->Element[3][3] = 1.0;
}

//----------------------------------------------------------------------------
// Update the reslice matrix, which is the slice-to-data matrix.
void vtkImageResliceMapper::UpdateResliceMatrix(
  vtkRenderer *ren, vtkImageSlice *prop)
{
  // Save the old matrix
  double *matrixElements = *this->ResliceMatrix->Element;
  double oldMatrixElements[16];
  vtkMatrix4x4::DeepCopy(oldMatrixElements, matrixElements);

  // Get world-to-data matrix from the prop matrix
  this->UpdateWorldToDataMatrix(prop);

  // Check if prop matrix is orthonormal
  bool propMatrixIsOrthonormal = false;
  vtkMatrix4x4 *propMatrix = 0;
  if (!this->InternalResampleToScreenPixels)
  {
    static double tol = 1e-12;
    propMatrix = prop->GetMatrix();
    double *row0 = propMatrix->Element[0];
    double *row1 = propMatrix->Element[1];
    double *row2 = propMatrix->Element[2];
    propMatrixIsOrthonormal = (
      fabs(vtkMath::Dot(row0, row0) - 1.0) < tol &&
      fabs(vtkMath::Dot(row1, row1) - 1.0) < tol &&
      fabs(vtkMath::Dot(row2, row2) - 1.0) < tol &&
      fabs(vtkMath::Dot(row0, row1)) < tol &&
      fabs(vtkMath::Dot(row0, row2)) < tol &&
      fabs(vtkMath::Dot(row1, row2)) < tol);
  }

  // Compute SliceToWorld matrix from camera if prop matrix is not
  // orthonormal or if InternalResampleToScreenPixels is set
  if (this->InternalResampleToScreenPixels ||
      !propMatrixIsOrthonormal)
  {
    this->UpdateSliceToWorldMatrix(ren->GetActiveCamera());
    vtkMatrix4x4::Multiply4x4(
      this->WorldToDataMatrix, this->SliceToWorldMatrix, this->ResliceMatrix);
  }
  else
  {
    // Get the matrices used to compute the reslice matrix
    vtkMatrix4x4 *resliceMatrix = this->ResliceMatrix;
    vtkMatrix4x4 *viewMatrix =
      ren->GetActiveCamera()->GetViewTransformMatrix();

    // Get slice plane in world coords by passing null as the matrix
    double wplane[4];
    this->GetSlicePlaneInDataCoords(0, wplane);

    // Check whether normal is facing towards camera, the "ndop" is
    // the negative of the direction of projection for the camera
    double *ndop = viewMatrix->Element[2];
    double dotprod = vtkMath::Dot(ndop, wplane);

    // Get slice plane in data coords by passing the prop matrix, flip
    // normal to face the camera
    double plane[4];
    this->GetSlicePlaneInDataCoords(propMatrix, plane);
    if (dotprod < 0)
    {
      plane[0] = -plane[0];
      plane[1] = -plane[1];
      plane[2] = -plane[2];
      plane[3] = -plane[3];

      wplane[0] = -wplane[0];
      wplane[1] = -wplane[1];
      wplane[2] = -wplane[2];
      wplane[3] = -wplane[3];
    }

    // Find the largest component of the normal
    int maxi = 0;
    double maxv = 0.0;
    for (int i = 0; i < 3; i++)
    {
      double tmp = plane[i]*plane[i];
      if (tmp > maxv)
      {
        maxi = i;
        maxv = tmp;
      }
    }

    // Create the corresponding axis
    double axis[3];
    axis[0] = 0.0;
    axis[1] = 0.0;
    axis[2] = 0.0;
    axis[maxi] = ((plane[maxi] < 0.0) ? -1.0 : 1.0);

    // Create two orthogonal axes
    double saxis[3], taxis[3];
    taxis[0] = 0.0;
    taxis[1] = 1.0;
    taxis[2] = 0.0;
    if (maxi == 1)
    {
      taxis[1] = 0.0;
      taxis[2] = 1.0;
    }
    vtkMath::Cross(taxis, axis, saxis);

    // The normal is the first three elements
    double *normal = plane;

    // The last element is -dot(normal, origin)
    double dp = (-plane[3] +
                 wplane[0]*propMatrix->Element[0][3] +
                 wplane[1]*propMatrix->Element[1][3] +
                 wplane[2]*propMatrix->Element[2][3]);

    // Compute the rotation angle between the axis and the normal
    double vec[3];
    vtkMath::Cross(axis, normal, vec);
    double costheta = vtkMath::Dot(axis, normal);
    double sintheta = vtkMath::Norm(vec);
    double theta = atan2(sintheta, costheta);
    if (sintheta != 0)
    {
      vec[0] /= sintheta;
      vec[1] /= sintheta;
      vec[2] /= sintheta;
    }
    // convert to quaternion
    costheta = cos(0.5*theta);
    sintheta = sin(0.5*theta);
    double quat[4];
    quat[0] = costheta;
    quat[1] = vec[0]*sintheta;
    quat[2] = vec[1]*sintheta;
    quat[3] = vec[2]*sintheta;
    // convert to matrix
    double mat[3][3];
    vtkMath::QuaternionToMatrix3x3(quat, mat);

    // Create a slice-to-data transform matrix
    // The columns are v1, v2, normal
    double v1[3], v2[3];
    vtkMath::Multiply3x3(mat, saxis, v1);
    vtkMath::Multiply3x3(mat, taxis, v2);

    resliceMatrix->Element[0][0] = v1[0];
    resliceMatrix->Element[1][0] = v1[1];
    resliceMatrix->Element[2][0] = v1[2];
    resliceMatrix->Element[3][0] = 0.0;

    resliceMatrix->Element[0][1] = v2[0];
    resliceMatrix->Element[1][1] = v2[1];
    resliceMatrix->Element[2][1] = v2[2];
    resliceMatrix->Element[3][1] = 0.0;

    resliceMatrix->Element[0][2] = normal[0];
    resliceMatrix->Element[1][2] = normal[1];
    resliceMatrix->Element[2][2] = normal[2];
    resliceMatrix->Element[3][2] = 0.0;

    resliceMatrix->Element[0][3] = dp*(propMatrix->Element[2][0] - normal[0]) -
      (propMatrix->Element[0][3]*propMatrix->Element[0][0] +
       propMatrix->Element[1][3]*propMatrix->Element[1][0] +
       propMatrix->Element[2][3]*propMatrix->Element[2][0]);
    resliceMatrix->Element[1][3] = dp*(propMatrix->Element[2][1] - normal[1]) -
      (propMatrix->Element[0][3]*propMatrix->Element[0][1] +
       propMatrix->Element[1][3]*propMatrix->Element[1][1] +
       propMatrix->Element[2][3]*propMatrix->Element[2][1]);
    resliceMatrix->Element[2][3] = dp*(propMatrix->Element[2][2] - normal[2]) -
      (propMatrix->Element[0][3]*propMatrix->Element[0][2] +
       propMatrix->Element[1][3]*propMatrix->Element[1][2] +
       propMatrix->Element[2][3]*propMatrix->Element[2][2]);
    resliceMatrix->Element[3][3] = 1.0;

    // Compute the SliceToWorldMatrix
    vtkMatrix4x4::Multiply4x4(propMatrix, resliceMatrix,
      this->SliceToWorldMatrix);
  }

  // If matrix changed, mark as modified so that Reslice will update
  int matrixChanged = 0;
  for (int j = 0; j < 16; j++)
  {
    matrixChanged |= (matrixElements[j] != oldMatrixElements[j]);
  }
  if (matrixChanged)
  {
    this->ResliceMatrix->Modified();
  }
}

//----------------------------------------------------------------------------
// Do all the fancy math to set up the reslicing
void vtkImageResliceMapper::UpdateResliceInformation(vtkRenderer *ren)
{
  vtkMatrix4x4 *resliceMatrix = this->ResliceMatrix;
  vtkImageResliceToColors *reslice = this->ImageReslice;

  int extent[6];
  double spacing[3];
  double origin[3];

  // Get current spacing and origin
  reslice->GetOutputSpacing(spacing);
  reslice->GetOutputOrigin(origin);
  reslice->GetOutputExtent(extent);

  // Get the view matrix
  vtkCamera *camera = ren->GetActiveCamera();
  vtkMatrix4x4 *viewMatrix = camera->GetViewTransformMatrix();

  // Get slice plane in world coords by passing null as the matrix
  double plane[4];
  this->GetSlicePlaneInDataCoords(0, plane);

  // Check whether normal is facing towards camera, the "ndop" is
  // the negative of the direction of projection for the camera
  double *ndop = viewMatrix->Element[2];
  if (vtkMath::Dot(ndop, plane) < 0)
  {
    plane[0] = -plane[0];
    plane[1] = -plane[1];
    plane[2] = -plane[2];
    plane[3] = -plane[3];
  }

  // Get the z position of the slice in slice coords
  // (requires plane to be normalized by GetSlicePlaneInDataCoords)
  double z = (plane[2] - 2.0)*plane[3];

  if (this->InternalResampleToScreenPixels)
  {
    // Get the projection matrix
    double aspect = ren->GetTiledAspectRatio();
    vtkMatrix4x4 *projMatrix = camera->GetProjectionTransformMatrix(
                                 aspect, 0, 1);

    // Compute other useful matrices
    double worldToView[16];
    double viewToWorld[16];
    double planeWorldToView[16];
    vtkMatrix4x4::Multiply4x4(
      *projMatrix->Element, *viewMatrix->Element, worldToView);
    vtkMatrix4x4::Invert(worldToView, viewToWorld);
    vtkMatrix4x4::Transpose(viewToWorld, planeWorldToView);

    double worldToSlice[16];
    double viewToSlice[16];
    vtkMatrix4x4::Invert(*this->SliceToWorldMatrix->Element, worldToSlice);
    vtkMatrix4x4::Multiply4x4(worldToSlice, viewToWorld, viewToSlice);

    // Transform the plane into view coordinates, using the transpose
    // of the inverse of the world-to-view matrix
    vtkMatrix4x4::MultiplyPoint(planeWorldToView, plane, plane);

    // Compute the bounds in slice coords
    double xmin = VTK_DOUBLE_MAX;
    double xmax = -VTK_DOUBLE_MAX;
    double ymin = VTK_DOUBLE_MAX;
    double ymax = -VTK_DOUBLE_MAX;

    for (int i = 0; i < 4; i++)
    {
      // The four corners of the view
      double x = (((i & 1) == 0) ? -1.0 : 1.0);
      double y = (((i & 2) == 0) ? -1.0 : 1.0);

      double hpoint[4];
      hpoint[0] = x;
      hpoint[1] = y;
      hpoint[2] = 0.0;
      hpoint[3] = 1.0;

      if (fabs(plane[2]) < 1e-6)
      {
        // Looking at plane edge-on, just put some
        // points at front clipping plane, others at back plane
        hpoint[2] = (((i & 1) == 0) ? 0.0 : 1.0);
      }
      else
      {
        // Intersect with the slice plane
        hpoint[2] = - (x*plane[0] + y*plane[1] + plane[3])/plane[2];

        // Clip to the front and back clipping planes
        if (hpoint[2] < 0)
        {
          hpoint[2] = 0.0;
        }
        else if (hpoint[2] > 1)
        {
          hpoint[2] = 1.0;
        }
      }

      // Transform into slice coords
      vtkMatrix4x4::MultiplyPoint(viewToSlice, hpoint, hpoint);

      x = hpoint[0]/hpoint[3];
      y = hpoint[1]/hpoint[3];

      // Find min/max in slice coords
      if (x < xmin) { xmin = x; }
      if (x > xmax) { xmax = x; }
      if (y < ymin) { ymin = y; }
      if (y > ymax) { ymax = y; }
    }

    // The ResliceExtent is always set to the renderer size,
    // this is the maximum size ever required and sticking to
    // this size avoids any memory reallocation on GPU or CPU
    int *size = ren->GetSize();
    int xsize = ((size[0] <= 0) ? 1 : size[0]);
    int ysize = ((size[1] <= 0) ? 1 : size[1]);

    extent[0] = 0;
    extent[1] = xsize - 1;
    extent[2] = 0;
    extent[3] = ysize - 1;
    extent[4] = 0;
    extent[5] = 0;

    // Find the spacing
    spacing[0] = (xmax - xmin)/xsize;
    spacing[1] = (ymax - ymin)/ysize;

    // Corner of resliced plane, including half-pixel offset to
    // exactly match texels to pixels in the final rendering
    origin[0] = xmin + 0.5*spacing[0];
    origin[1] = ymin + 0.5*spacing[1];
    origin[2] = z;
  }
  else
  {
    // Compute texel spacing from image spacing
    double inputSpacing[3];
    this->GetInput()->GetSpacing(inputSpacing);
    inputSpacing[0] = fabs(inputSpacing[0]);
    inputSpacing[1] = fabs(inputSpacing[1]);
    inputSpacing[2] = fabs(inputSpacing[2]);
    for (int j = 0; j < 2; j++)
    {
      double xc = this->ResliceMatrix->Element[j][0];
      double yc = this->ResliceMatrix->Element[j][1];
      double zc = this->ResliceMatrix->Element[j][2];
      double s = (xc*xc*inputSpacing[0] +
                  yc*yc*inputSpacing[1] +
                  zc*zc*inputSpacing[2])/sqrt(xc*xc + yc*yc + zc*zc);
      s /= this->ImageSampleFactor;
      // only modify if difference is greater than roundoff tolerance
      if (fabs((s - spacing[j])/s) > 1e-12)
      {
        spacing[j] = s;
      }
    }

    // Find the bounds for the texture
    double xmin = VTK_DOUBLE_MAX;
    double xmax = -VTK_DOUBLE_MAX;
    double ymin = VTK_DOUBLE_MAX;
    double ymax = -VTK_DOUBLE_MAX;

    vtkPoints *points = this->SliceMapper->GetPoints();
    vtkIdType n = points->GetNumberOfPoints();
    if (n == 0)
    {
      double inputOrigin[3];
      this->GetInput()->GetOrigin(inputOrigin);
      xmin = inputOrigin[0];
      xmax = inputOrigin[0];
      ymin = inputOrigin[1];
      ymax = inputOrigin[1];
    }

    for (vtkIdType k = 0; k < n; k++)
    {
      double point[3];
      points->GetPoint(k, point);

      xmin = ((xmin < point[0]) ? xmin : point[0]);
      xmax = ((xmax > point[0]) ? xmax : point[0]);
      ymin = ((ymin < point[1]) ? ymin : point[1]);
      ymax = ((ymax > point[1]) ? ymax : point[1]);
    }

    double tol = VTK_RESLICE_MAPPER_VOXEL_TOL;
    int xsize = vtkMath::Floor((xmax - xmin)/spacing[0] + tol);
    int ysize = vtkMath::Floor((ymax - ymin)/spacing[1] + tol);
    if (this->Border == 0)
    {
      xsize += 1;
      ysize += 1;
    }
    if (xsize < 1) { xsize = 1; }
    if (ysize < 1) { ysize = 1; }

    // Keep old size if possible, to avoid memory reallocation
    if ((xsize - 1) > extent[1] || (ysize - 1) > extent[3] ||
        (0.9*extent[1]/xsize) > 1.0 || (0.9*extent[3]/ysize) > 1.0)
    {
      extent[1] = xsize - 1;
      extent[3] = ysize - 1;
    }
    extent[0] = 0;
    extent[2] = 0;
    extent[4] = 0;
    extent[5] = 0;

    double x0 = xmin + 0.5*spacing[0]*(this->Border != 0);
    double y0 = ymin + 0.5*spacing[1]*(this->Border != 0);

    double dx = x0 - origin[0];
    double dy = y0 - origin[1];
    double dz = z - origin[2];

    // only modify origin if it has changed by tolerance
    if (dx*dx + dy*dy + dz*dz > tol*tol*spacing[0]*spacing[1])
    {
      origin[0] = x0;
      origin[1] = y0;
      origin[2] = z;
    }
  }

  // Prepare for reslicing
  reslice->SetResliceAxes(resliceMatrix);
  reslice->SetOutputExtent(extent);
  reslice->SetOutputSpacing(spacing);
  reslice->SetOutputOrigin(origin);

  if ((this->SliceFacesCamera && this->InternalResampleToScreenPixels) ||
      this->SlabThickness > 0)
  {
    // if slice follows camera, use reslice to set the border
    reslice->SetBorder(this->Border);
  }
  else
  {
    // tell reslice to use a double-thickness border,
    // since the polygon geometry will dictate the actual size
    reslice->SetBorder(2);
  }
}

//----------------------------------------------------------------------------
// Do all the fancy math to set up the reslicing
void vtkImageResliceMapper::UpdateColorInformation(vtkImageProperty *property)
{
  vtkScalarsToColors *lookupTable = this->DefaultLookupTable;

  if (property)
  {
    double colorWindow = property->GetColorWindow();
    double colorLevel = property->GetColorLevel();
    if (property->GetLookupTable())
    {
      lookupTable = property->GetLookupTable();
      if (!property->GetUseLookupTableScalarRange())
      {
        lookupTable->SetRange(colorLevel - 0.5*colorWindow,
                              colorLevel + 0.5*colorWindow);
      }
    }
    else
    {
      lookupTable->SetRange(colorLevel - 0.5*colorWindow,
                            colorLevel + 0.5*colorWindow);
    }
  }
  else
  {
    lookupTable->SetRange(0, 255);
  }
  this->ImageReslice->SetBypass(this->SeparateWindowLevelOperation != 0);
  this->ImageReslice->SetLookupTable(lookupTable);
  double backgroundColor[4] = { 0.0, 0.0, 0.0, 0.0 };
  if (this->Background)
  {
    this->GetBackgroundColor(property, backgroundColor);
    backgroundColor[0] *= 255;
    backgroundColor[1] *= 255;
    backgroundColor[2] *= 255;
    backgroundColor[3] *= 255;
  }
  this->ImageReslice->SetBackgroundColor(backgroundColor);
}

//----------------------------------------------------------------------------
// Set the reslice interpolation and slab thickness
void vtkImageResliceMapper::UpdateResliceInterpolation(
  vtkImageProperty *property)
{
  // set the interpolation mode and border
  int interpMode = VTK_RESLICE_NEAREST;
  int slabSlices = 1;

  if (property)
  {
    switch(property->GetInterpolationType())
    {
      case VTK_NEAREST_INTERPOLATION:
        interpMode = VTK_RESLICE_NEAREST;
        break;
      case VTK_LINEAR_INTERPOLATION:
        interpMode = VTK_RESLICE_LINEAR;
        break;
      case VTK_CUBIC_INTERPOLATION:
        interpMode = VTK_RESLICE_CUBIC;
        break;
    }
  }

  // set up the slice spacing for slab views
  double spacing[3], inputSpacing[3];
  this->ImageReslice->GetOutputSpacing(spacing);
  this->GetInput()->GetSpacing(inputSpacing);
  inputSpacing[0] = fabs(inputSpacing[0]);
  inputSpacing[1] = fabs(inputSpacing[1]);
  inputSpacing[2] = fabs(inputSpacing[2]);
  double xc = this->ResliceMatrix->Element[2][0];
  double yc = this->ResliceMatrix->Element[2][1];
  double zc = this->ResliceMatrix->Element[2][2];
  spacing[2] = (xc*xc*inputSpacing[0] +
                yc*yc*inputSpacing[1] +
                zc*zc*inputSpacing[2])/sqrt(xc*xc + yc*yc + zc*zc);

  // slab slice spacing is half the input slice spacing
  int n = vtkMath::Ceil(this->SlabThickness/spacing[2]);
  slabSlices = 1 + this->SlabSampleFactor*n;
  if (slabSlices > 1)
  {
    spacing[2] = this->SlabThickness/(slabSlices - 1);
  }
  this->ImageReslice->SetOutputSpacing(spacing);
  int slabMode = this->SlabType;
  double scalarScale = 1.0;
  if (slabMode == VTK_IMAGE_SLAB_SUM)
  {
    // "sum" means integrating over the path length of each ray through
    // the volume, so we need to include the sample spacing as a factor
    scalarScale = spacing[2];
  }

  this->ImageReslice->SetInterpolationMode(interpMode);
  this->ImageReslice->SetSlabMode(slabMode);
  this->ImageReslice->SetSlabNumberOfSlices(slabSlices);
  this->ImageReslice->SetScalarScale(scalarScale);
  this->ImageReslice->SlabTrapezoidIntegrationOn();
}

//----------------------------------------------------------------------------
void vtkImageResliceMapper::CheckerboardImage(
  vtkImageData *input, vtkCamera *camera, vtkImageProperty *property)
{
  // Use focal point as center of checkerboard pattern.  This guarantees
  // exactly the same checkerboard for all images in the scene, which is
  // useful when doing multiple overlays.
  double focalPoint[4];
  camera->GetFocalPoint(focalPoint);
  focalPoint[3] = 1.0;

  double worldToSlice[16];
  vtkMatrix4x4::Invert(*this->SliceToWorldMatrix->Element, worldToSlice);

  vtkMatrix4x4::MultiplyPoint(worldToSlice, focalPoint, focalPoint);
  if (focalPoint[3] != 0.0)
  {
    focalPoint[0] /= focalPoint[3];
    focalPoint[1] /= focalPoint[3];
    focalPoint[2] /= focalPoint[3];
  }

  // Get the checkerboard spacing and apply the offset fraction
  double checkSpacing[2], checkOffset[2];
  property->GetCheckerboardSpacing(checkSpacing);
  property->GetCheckerboardOffset(checkOffset);
  checkOffset[0] = checkOffset[0]*checkSpacing[0] + focalPoint[0];
  checkOffset[1] = checkOffset[1]*checkSpacing[1] + focalPoint[1];

  // Adjust according to the origin and spacing of the slice data
  double origin[3], spacing[3];
  input->GetSpacing(spacing);
  input->GetOrigin(origin);
  checkOffset[0] = (checkOffset[0] - origin[0])/spacing[0];
  checkOffset[1] = (checkOffset[1] - origin[1])/spacing[1];
  checkSpacing[0] /= spacing[0];
  checkSpacing[1] /= spacing[1];

  // Apply the checkerboard to the data
  int extent[6];
  input->GetExtent(extent);
  unsigned char *data = static_cast<unsigned char *>(
    input->GetScalarPointerForExtent(extent));

  vtkImageMapper3D::CheckerboardRGBA(
    data, extent[1] - extent[0] + 1, extent[3] - extent[2] + 1,
    checkOffset[0], checkOffset[1], checkSpacing[0], checkSpacing[1]);
}

//----------------------------------------------------------------------------
// Compute the vertices of the polygon in the slice coordinate system
#define VTK_IRM_MAX_VERTS 32
#define VTK_IRM_MAX_COORDS 96
void vtkImageResliceMapper::UpdatePolygonCoords(vtkRenderer *ren)
{
  // Get the projection matrix
  double aspect = ren->GetTiledAspectRatio();
  vtkCamera *camera = ren->GetActiveCamera();
  vtkMatrix4x4 *viewMatrix = camera->GetViewTransformMatrix();
  vtkMatrix4x4 *projMatrix = camera->GetProjectionTransformMatrix(
                               aspect, 0, 1);

  // Compute other useful matrices
  double worldToView[16];
  double viewToWorld[16];
  vtkMatrix4x4::Multiply4x4(
    *projMatrix->Element, *viewMatrix->Element, worldToView);
  vtkMatrix4x4::Invert(worldToView, viewToWorld);

  double worldToSlice[16];
  double viewToSlice[16];
  vtkMatrix4x4::Invert(*this->SliceToWorldMatrix->Element, worldToSlice);
  vtkMatrix4x4::Multiply4x4(worldToSlice, viewToWorld, viewToSlice);

  // Get slice plane in world coords by passing null as the matrix
  double plane[4];
  this->GetSlicePlaneInDataCoords(0, plane);

  // Check whether normal is facing towards camera, the "ndop" is
  // the negative of the direction of projection for the camera
  double *ndop = viewMatrix->Element[2];
  if (vtkMath::Dot(ndop, plane) < 0)
  {
    plane[0] = -plane[0];
    plane[1] = -plane[1];
    plane[2] = -plane[2];
    plane[3] = -plane[3];
  }

  // Get the z position of the slice in slice coords
  // (requires plane to be normalized by GetSlicePlaneInDataCoords)
  double z = (plane[2] - 2.0)*plane[3];

  // Generate a tolerance based on the screen pixel size
  double fpoint[4];
  camera->GetFocalPoint(fpoint);
  fpoint[3] = 1.0;
  vtkMatrix4x4::MultiplyPoint(worldToView, fpoint, fpoint);
  fpoint[0] /= fpoint[3];
  fpoint[1] /= fpoint[3];
  fpoint[2] /= fpoint[3];
  fpoint[3] = 1.0;

  double topOfScreen[4], botOfScreen[4];
  fpoint[1] -= 1.0;
  vtkMatrix4x4::MultiplyPoint(viewToWorld, fpoint, topOfScreen);
  fpoint[1] += 2.0;
  vtkMatrix4x4::MultiplyPoint(viewToWorld, fpoint, botOfScreen);

  topOfScreen[0] /= topOfScreen[3];
  topOfScreen[1] /= topOfScreen[3];
  topOfScreen[2] /= topOfScreen[3];
  topOfScreen[3] = 1.0;

  botOfScreen[0] /= botOfScreen[3];
  botOfScreen[1] /= botOfScreen[3];
  botOfScreen[2] /= botOfScreen[3];
  botOfScreen[3] = 1.0;

  // height of view in world coords at focal point
  double viewHeight =
    sqrt(vtkMath::Distance2BetweenPoints(topOfScreen, botOfScreen));

  // height of view in pixels
  int height = ren->GetSize()[1];

  double tol = (height == 0 ? 0.5 : viewHeight*0.5/height);

  // make the data bounding box (with or without border)
  double b = (this->Border ? 0.5 : VTK_RESLICE_MAPPER_VOXEL_TOL);
  double bounds[6];
  for (int ii = 0; ii < 3; ii++)
  {
    double c = b*this->DataSpacing[ii];
    int lo = this->DataWholeExtent[2*ii];
    int hi = this->DataWholeExtent[2*ii+1];
    if (lo == hi && tol > c)
    { // apply tolerance to avoid degeneracy
      c = tol;
    }
    bounds[2*ii]   = lo*this->DataSpacing[ii] + this->DataOrigin[ii] - c;
    bounds[2*ii+1] = hi*this->DataSpacing[ii] + this->DataOrigin[ii] + c;
  }

  // transform the vertices to the slice coord system
  double xpoints[8], ypoints[8];
  double weights1[8], weights2[8];
  bool above[8], below[8];
  double mat[16];
  vtkMatrix4x4::Multiply4x4(*this->WorldToDataMatrix->Element,
                            *this->SliceToWorldMatrix->Element, mat);
  vtkMatrix4x4::Invert(mat, mat);

  // arrays for the list of polygon points
  int n = 0;
  double newxpoints[VTK_IRM_MAX_VERTS];
  double newypoints[VTK_IRM_MAX_VERTS];
  double cx = 0.0;
  double cy = 0.0;

  for (int i = 0; i < 8; i++)
  {
    double point[4];
    point[0] = bounds[0 + ((i>>0)&1)];
    point[1] = bounds[2 + ((i>>1)&1)];
    point[2] = bounds[4 + ((i>>2)&1)];
    point[3] = 1.0;
    vtkMatrix4x4::MultiplyPoint(mat, point, point);
    xpoints[i] = point[0]/point[3];
    ypoints[i] = point[1]/point[3];
    weights1[i] = point[2]/point[3] - z - 0.5*this->SlabThickness;
    weights2[i] = weights1[i] + this->SlabThickness;
    below[i] = (weights1[i] < 0);
    above[i] = (weights2[i] >= 0);

    if (this->SlabThickness > 0 && above[i] && below[i])
    {
      newxpoints[n] = xpoints[i];
      newypoints[n] = ypoints[i];
      cx += xpoints[i];
      cy += ypoints[i];
      n++;
    }
  }

  // go through the edges and find the new points
  for (int j = 0; j < 12; j++)
  {
    // verts from edges (sorry about this..)
    int i1 = (j & 3) | (((j<<1) ^ (j<<2)) & 4);
    int i2 = (i1 ^ (1 << (j>>2)));

    double *weights = weights2;
    bool *side = above;
    int m = 1 + (this->SlabThickness > 0);
    for (int k = 0; k < m; k++)
    {
      if (side[i1] ^ side[i2])
      {
        double w1 = weights[i2];
        double w2 = -weights[i1];
        double x = (w1*xpoints[i1] + w2*xpoints[i2])/(w1 + w2);
        double y = (w1*ypoints[i1] + w2*ypoints[i2])/(w1 + w2);
        newxpoints[n] = x;
        newypoints[n] = y;
        cx += x;
        cy += y;
        n++;
      }
      weights = weights1;
      side = below;
    }
  }

  double coords[VTK_IRM_MAX_COORDS];

  if (n > 0)
  {
    // centroid
    cx /= n;
    cy /= n;

    // sort the points to make a convex polygon
    double angles[VTK_IRM_MAX_VERTS];
    for (int k = 0; k < n; k++)
    {
      double x = newxpoints[k];
      double y = newypoints[k];
      double t = atan2(y - cy, x - cx);
      int kk;
      for (kk = 0; kk < k; kk++)
      {
        if (t < angles[kk]) { break; }
      }
      for (int jj = k; jj > kk; --jj)
      {
        int jj3 = jj*3;
        angles[jj] = angles[jj-1];
        coords[jj3] = coords[jj3-3];
        coords[jj3+1] = coords[jj3-2];
        coords[jj3+2] = coords[jj3-1];
      }
      int kk3 = kk*3;
      angles[kk] = t;
      coords[kk3] = x;
      coords[kk3+1] = y;
      coords[kk3+2] = z;
    }
  }

  // remove degenerate points
  if (n > 0)
  {
    bool found = true;
    int m = 0;
    do
    {
      m = 0;
      double xl = coords[3*(n-1)+0];
      double yl = coords[3*(n-1)+1];
      for (int k = 0; k < n; k++)
      {
        double x = coords[3*k+0];
        double y = coords[3*k+1];

        if (((x - xl)*(x - xl) + (y - yl)*(y - yl)) > tol*tol)
        {
          coords[3*m+0] = x;
          coords[3*m+1] = y;
          xl = x;
          yl = y;
          m++;
        }
      }
      found = (m < n);
      n = m;
    }
    while (found && n > 0);
  }

  // find convex hull
  if (this->SlabThickness > 0 && n > 0)
  {
    bool found = true;
    int m = 0;
    do
    {
      m = 0;
      double xl = coords[3*(n-1)+0];
      double yl = coords[3*(n-1)+1];
      for (int k = 0; k < n; k++)
      {
        double x = coords[3*k+0];
        double y = coords[3*k+1];
        int k1 = (k + 1) % n;
        double xn = coords[3*k1+0];
        double yn = coords[3*k1+1];

        if ((xn-xl)*(y-yl) - (yn-yl)*(x-xl) < tol*tol)
        {
          coords[3*m+0] = x;
          coords[3*m+1] = y;
          xl = x;
          yl = y;
          m++;
        }
      }
      found = (m < n);
      n = m;
    }
    while (found && n > 0);
  }

  vtkPoints *points = this->SliceMapper->GetPoints();
  if (!points)
  {
    points = vtkPoints::New();
    points->SetDataTypeToDouble();
    this->SliceMapper->SetPoints(points);
    points->Delete();
  }

  points->SetNumberOfPoints(n);
  for (int k = 0; k < n; k++)
  {
    points->SetPoint(k, &coords[3*k]);
  }
}

//----------------------------------------------------------------------------
void vtkImageResliceMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "JumpToNearestSlice: "
     << (this->JumpToNearestSlice ? "On\n" : "Off\n");
  os << indent << "AutoAdjustImageQuality: "
     << (this->AutoAdjustImageQuality ? "On\n" : "Off\n");
  os << indent << "SeparateWindowLevelOperation: "
     << (this->SeparateWindowLevelOperation ? "On\n" : "Off\n");
  os << indent << "ResampleToScreenPixels: "
     << (this->ResampleToScreenPixels ? "On\n" : "Off\n");
  os << indent << "SlabThickness: " << this->SlabThickness << "\n";
  os << indent << "SlabType: " << this->GetSlabTypeAsString() << "\n";
  os << indent << "SlabSampleFactor: " << this->SlabSampleFactor << "\n";
  os << indent << "ImageSampleFactor: " << this->ImageSampleFactor << "\n";
  os << indent << "Interpolator: " << this->GetInterpolator() << "\n";
}

//----------------------------------------------------------------------------
const char *vtkImageResliceMapper::GetSlabTypeAsString()
{
  switch (this->SlabType)
  {
    case VTK_IMAGE_SLAB_MIN:
      return "Min";
    case VTK_IMAGE_SLAB_MAX:
      return "Max";
    case VTK_IMAGE_SLAB_MEAN:
      return "Mean";
    case VTK_IMAGE_SLAB_SUM:
      return "Sum";
  }
  return "";
}

//----------------------------------------------------------------------------
vtkMTimeType vtkImageResliceMapper::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();

  // Check whether interpolator has changed
  vtkAbstractImageInterpolator *interpolator =
    this->ImageReslice->GetInterpolator();
  if (interpolator)
  {
    vtkMTimeType mTime2 = interpolator->GetMTime();
    if (mTime2 > mTime)
    {
      mTime = mTime2;
    }
  }

  // Include camera in MTime so that REQUEST_INFORMATION
  // will be called if the camera changes
  if (this->SliceFacesCamera || this->SliceAtFocalPoint ||
      this->InternalResampleToScreenPixels)
  {
    vtkRenderer *ren = this->GetCurrentRenderer();
    if (ren)
    {
      vtkCamera *camera = ren->GetActiveCamera();
      vtkMTimeType mTime2 = camera->GetMTime();
      mTime = (mTime2 > mTime ? mTime2 : mTime);
    }
  }

  if (!this->SliceFacesCamera || !this->SliceAtFocalPoint)
  {
    vtkMTimeType sTime = this->SlicePlane->GetMTime();
    mTime = (sTime > mTime ? sTime : mTime);
  }

  vtkImageSlice *prop = this->GetCurrentProp();
  if (prop != NULL)
  {
    vtkMTimeType mTime2 = prop->GetUserTransformMatrixMTime();
    mTime = (mTime2 > mTime ? mTime2 : mTime);

    vtkImageProperty *property = prop->GetProperty();
    if (property != NULL)
    {
      bool useMTime = true;
      if (this->SeparateWindowLevelOperation)
      {
        // only care about property if interpolation mode has changed,
        // since interpolation is the only property-related operation
        // done by vtkImageReslice if SeparateWindowLevelOperation is on
        int imode = this->ImageReslice->GetInterpolationMode();
        this->UpdateResliceInterpolation(property);
        useMTime = (imode != this->ImageReslice->GetInterpolationMode());
      }
      if (useMTime)
      {
        mTime2 = property->GetMTime();
        mTime = (mTime2 > mTime ? mTime2 : mTime);

        vtkScalarsToColors *lookupTable = property->GetLookupTable();
        if (lookupTable != NULL)
        {
          // check the lookup table mtime
          mTime2 = lookupTable->GetMTime();
          mTime = (mTime2 > mTime ? mTime2 : mTime);
        }
      }
    }
  }

  return mTime;
}

//----------------------------------------------------------------------------
double *vtkImageResliceMapper::GetBounds()
{
  // Modify to give just the slice bounds
  if (!this->GetInput())
  {
    vtkMath::UninitializeBounds(this->Bounds);
    return this->Bounds;
  }
  else
  {
    this->UpdateInformation();
    double *spacing = this->DataSpacing;
    double *origin = this->DataOrigin;
    int *extent = this->DataWholeExtent;

    // expand by half a pixel if border is on
    double border = 0.5*(this->Border != 0);

    // swap the extent if the spacing is negative
    int swapX = (spacing[0] < 0);
    int swapY = (spacing[1] < 0);
    int swapZ = (spacing[2] < 0);

    this->Bounds[0+swapX] = origin[0] + (extent[0] - border) * spacing[0];
    this->Bounds[2+swapY] = origin[1] + (extent[2] - border) * spacing[1];
    this->Bounds[4+swapZ] = origin[2] + (extent[4] - border) * spacing[2];

    this->Bounds[1-swapX] = origin[0] + (extent[1] + border) * spacing[0];
    this->Bounds[3-swapY] = origin[1] + (extent[3] + border) * spacing[1];
    this->Bounds[5-swapZ] = origin[2] + (extent[5] + border) * spacing[2];

    return this->Bounds;
  }
}

//----------------------------------------------------------------------------
void vtkImageResliceMapper::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // These filters share our input and are therefore involved in a
  // reference loop.
  vtkGarbageCollectorReport(collector, this->ImageReslice, "ImageReslice");
  vtkGarbageCollectorReport(collector, this->SliceMapper, "SliceMapper");
}
