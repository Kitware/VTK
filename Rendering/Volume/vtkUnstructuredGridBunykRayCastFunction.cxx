/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridBunykRayCastFunction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredGridBunykRayCastFunction.h"

#include "vtkArrayDispatch.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridVolumeRayCastMapper.h"
#include "vtkVolume.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkTransform.h"
#include "vtkCell.h"
#include "vtkCellType.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkVolumeProperty.h"
#include "vtkUnstructuredGridVolumeRayCastIterator.h"
#include "vtkSmartPointer.h"
#include "vtkCellIterator.h"

#include <cassert>
#include <cstdlib>

vtkStandardNewMacro(vtkUnstructuredGridBunykRayCastFunction);

#define VTK_BUNYKRCF_NUMLISTS 100000

namespace {

struct TemplateCastRayWorker
{
  vtkUnstructuredGridBunykRayCastFunction *Self;
  int NumComponents;
  int X;
  int Y;
  double FarClipZ;
  vtkUnstructuredGridBunykRayCastFunction::Intersection *&IntersectionPtr;
  vtkUnstructuredGridBunykRayCastFunction::Triangle *&CurrentTriangle;
  vtkIdType &CurrentTetra;
  vtkIdType *IntersectedCells;
  double *IntersectionLengths;
  int MaxNumIntersections;

  // Result:
  vtkIdType NumIntersections;

  TemplateCastRayWorker(
      vtkUnstructuredGridBunykRayCastFunction *self,
      int numComponents, int x, int y, double farClipZ,
      vtkUnstructuredGridBunykRayCastFunction::Intersection *&intersectionPtr,
      vtkUnstructuredGridBunykRayCastFunction::Triangle *&currentTriangle,
      vtkIdType &currentTetra, vtkIdType *intersectedCells,
      double *intersectedLengths, int maxNumIntersections
      )
    : Self(self), NumComponents(numComponents), X(x), Y(y), FarClipZ(farClipZ),
      IntersectionPtr(intersectionPtr), CurrentTriangle(currentTriangle),
      CurrentTetra(currentTetra), IntersectedCells(intersectedCells),
      IntersectionLengths(intersectedLengths),
      MaxNumIntersections(maxNumIntersections),
      NumIntersections(0)
  {}

  TemplateCastRayWorker& operator=(const TemplateCastRayWorker &) VTK_DELETE_FUNCTION;

  // Execute the algorithm with all arrays set to NULL.
  void operator()()
  {
    (*this)(static_cast<vtkAOSDataArrayTemplate<float>*>(NULL),
            static_cast<vtkAOSDataArrayTemplate<float>*>(NULL),
            static_cast<vtkAOSDataArrayTemplate<float>*>(NULL));
  }

  template <typename ScalarArrayT, typename NearArrayT, typename FarArrayT>
  void operator()(ScalarArrayT *scalarArray,
                  NearArrayT *nearIntersectionArray,
                  FarArrayT *farIntersectionArray)
  {
    typedef typename NearArrayT::ValueType ValueType;

    int imageViewportSize[2];
    this->Self->GetImageViewportSize( imageViewportSize );

    int origin[2];
    this->Self->GetImageOrigin( origin );
    float fx = this->X - origin[0];
    float fy = this->Y - origin[1];

    double *points = this->Self->GetPoints();
    vtkUnstructuredGridBunykRayCastFunction::Triangle **triangles =
        this->Self->GetTetraTriangles();

    vtkMatrix4x4 *viewToWorld = this->Self->GetViewToWorldMatrix();

    vtkUnstructuredGridBunykRayCastFunction::Triangle *nextTriangle;
    vtkIdType nextTetra;

    this->NumIntersections = 0;

    double nearZ = VTK_DOUBLE_MIN;
    double nearPoint[4];
    double viewCoords[4];
    viewCoords[0] = ((float)this->X / (float)(imageViewportSize[0]-1)) * 2.0 - 1.0;
    viewCoords[1] = ((float)this->Y / (float)(imageViewportSize[1]-1)) * 2.0 - 1.0;
    // viewCoords[2] set when an intersection is found.
    viewCoords[3] = 1.0;
    if (this->CurrentTriangle)
    {
      // Find intersection in currentTriangle (the entry point).
      nearZ = -( fx*this->CurrentTriangle->A + fy*this->CurrentTriangle->B +
                 this->CurrentTriangle->D) / this->CurrentTriangle->C;

      viewCoords[2] = nearZ;

      viewToWorld->MultiplyPoint( viewCoords, nearPoint );
      nearPoint[0] /= nearPoint[3];
      nearPoint[1] /= nearPoint[3];
      nearPoint[2] /= nearPoint[3];
    }

    while (this->NumIntersections < this->MaxNumIntersections)
    {
      // If we have exited the mesh (or are entering it for the first time,
      // find the next intersection with an external face (which has already
      // been found with rasterization).
      if (!this->CurrentTriangle)
      {
        if (!this->IntersectionPtr)
        {
          break;  // No more intersections.
        }
        this->CurrentTriangle = this->IntersectionPtr->TriPtr;
        this->CurrentTetra    = this->IntersectionPtr->TriPtr->ReferredByTetra[0];
        this->IntersectionPtr = this->IntersectionPtr->Next;

        // Find intersection in currentTriangle (the entry point).
        nearZ = -( fx*this->CurrentTriangle->A + fy*this->CurrentTriangle->B +
                   this->CurrentTriangle->D) / this->CurrentTriangle->C;

        viewCoords[2] = nearZ;

        viewToWorld->MultiplyPoint( viewCoords, nearPoint );
        nearPoint[0] /= nearPoint[3];
        nearPoint[1] /= nearPoint[3];
        nearPoint[2] /= nearPoint[3];
      }

      // Find all triangles that the ray may exit.
      vtkUnstructuredGridBunykRayCastFunction::Triangle *candidate[3];

      int index = 0;
      int i;
      for ( i = 0; i < 4; i++ )
      {
        if ( triangles[this->CurrentTetra*4+i] != this->CurrentTriangle )
        {
          if ( index == 3 )
          {
            vtkGenericWarningMacro( "Ugh - found too many triangles!" );
          }
          else
          {
            candidate[index++] = triangles[this->CurrentTetra*4+i];
          }
        }
      }

      double farZ = VTK_DOUBLE_MAX;
      int minIdx = -1;

      // Determine which face the ray exits the cell from.
      for ( i = 0; i < 3; i++ )
      {
        // Far intersection is the nearest intersectation that is farther
        // than nearZ.
        double tmpZ = 1.0;
        if (candidate[i]->C != 0.0)
        {
          tmpZ =
            -( fx*candidate[i]->A +
               fy*candidate[i]->B +
               candidate[i]->D) / candidate[i]->C;
        }
        if (tmpZ > nearZ && tmpZ < farZ)
        {
          farZ = tmpZ;
          minIdx = i;
        }
      }

      // Now, the code above should ensure that farZ > nearZ, but I have
      // seen the case where we reach here with farZ == nearZ.  This is very
      // bad as we need ensure we always move forward so that we do not get
      // into loops.  I think there is something with GCC 3.2.3 that makes
      // the optimizer be too ambitous and turn the > into >=.
      if ((minIdx == -1) || (farZ <= nearZ))
      {
        // The ray never exited the cell?  Perhaps numerical inaccuracies
        // got us here.  Just bail out as if we exited the mesh.
        nextTriangle = NULL;
        nextTetra = -1;
      }
      else
      {
        if (farZ > this->FarClipZ)
        {
          // Exit happened after point of interest.  Bail out now (in case
          // we wish to restart).
          return;
        }

        if (this->IntersectedCells)
        {
          this->IntersectedCells[this->NumIntersections] = this->CurrentTetra;
        }

        nextTriangle = candidate[minIdx];

        // Compute intersection with exiting face.
        double farPoint[4];
        viewCoords[2] = farZ;
        viewToWorld->MultiplyPoint( viewCoords, farPoint );
        farPoint[0] /= farPoint[3];
        farPoint[1] /= farPoint[3];
        farPoint[2] /= farPoint[3];
        double dist
          = sqrt(  (nearPoint[0]-farPoint[0])*(nearPoint[0]-farPoint[0])
                 + (nearPoint[1]-farPoint[1])*(nearPoint[1]-farPoint[1])
                 + (nearPoint[2]-farPoint[2])*(nearPoint[2]-farPoint[2]) );

        if (this->IntersectionLengths)
        {
          this->IntersectionLengths[this->NumIntersections] = dist;
        }

        // compute the barycentric weights
        float ax, ay;
        double a1, b1, c1;
        ax = points[3*this->CurrentTriangle->PointIndex[0]];
        ay = points[3*this->CurrentTriangle->PointIndex[0]+1];
        b1 = ((fx-ax)*this->CurrentTriangle->P2Y - (fy-ay)*this->CurrentTriangle->P2X) / this->CurrentTriangle->Denominator;
        c1 = ((fy-ay)*this->CurrentTriangle->P1X - (fx-ax)*this->CurrentTriangle->P1Y) / this->CurrentTriangle->Denominator;
        a1 = 1.0 - b1 - c1;

        double a2, b2, c2;
        ax = points[3*nextTriangle->PointIndex[0]];
        ay = points[3*nextTriangle->PointIndex[0]+1];
        b2 = ((fx-ax)*nextTriangle->P2Y - (fy-ay)*nextTriangle->P2X) / nextTriangle->Denominator;
        c2 = ((fy-ay)*nextTriangle->P1X - (fx-ax)*nextTriangle->P1Y) / nextTriangle->Denominator;
        a2 = 1.0 - b2 - c2;

        if (nearIntersectionArray)
        {
          for (int c = 0; c < this->NumComponents; c++)
          {
            ValueType A, B, C;
            A = scalarArray->GetTypedComponent(this->CurrentTriangle->PointIndex[0], c);
            B = scalarArray->GetTypedComponent(this->CurrentTriangle->PointIndex[1], c);
            C = scalarArray->GetTypedComponent(this->CurrentTriangle->PointIndex[2], c);
            nearIntersectionArray->SetTypedComponent(
                  this->NumIntersections, c,
                  static_cast<ValueType>(a1 * A + b1 * B + c1 * C));
          }
        }

        if (farIntersectionArray)
        {
          for (int c = 0; c < this->NumComponents; c++)
          {
            ValueType A, B, C;
            A = scalarArray->GetTypedComponent(nextTriangle->PointIndex[0], c);
            B = scalarArray->GetTypedComponent(nextTriangle->PointIndex[1], c);
            C = scalarArray->GetTypedComponent(nextTriangle->PointIndex[2], c);
            farIntersectionArray->SetTypedComponent(
                  this->NumIntersections, c,
                  static_cast<ValueType>(a2 * A + b2 * B + c2 * C));
          }
        }

        this->NumIntersections++;

        // The far triangle has one or two tetras in its referred list.
        // If one, return -1 for next tetra and NULL for next triangle
        // since we are exiting. If two, return the one that isn't the
        // current one.
        if ( (nextTriangle)->ReferredByTetra[1] == -1 )
        {
          nextTetra = -1;
          nextTriangle = NULL;
        }
        else
        {
          if ( nextTriangle->ReferredByTetra[0] == this->CurrentTetra )
          {
            nextTetra = nextTriangle->ReferredByTetra[1];
          }
          else
          {
            nextTetra = nextTriangle->ReferredByTetra[0];
          }
        }

        nearZ = farZ;
        nearPoint[0] = farPoint[0];
        nearPoint[1] = farPoint[1];
        nearPoint[2] = farPoint[2];
        nearPoint[3] = farPoint[3];
      }

      this->CurrentTriangle = nextTriangle;
      this->CurrentTetra    = nextTetra;
    }
  }
};

} // end anon namespace

//-----------------------------------------------------------------------------

// This is an internal hidden class.

class vtkUnstructuredGridBunykRayCastIterator
  : public vtkUnstructuredGridVolumeRayCastIterator
{
public:
  vtkTypeMacro(vtkUnstructuredGridBunykRayCastIterator,
                       vtkUnstructuredGridVolumeRayCastIterator);
  static vtkUnstructuredGridBunykRayCastIterator *New();

  void Initialize(int x, int y) VTK_OVERRIDE;

  vtkIdType GetNextIntersections(vtkIdList *intersectedCells,
                                 vtkDoubleArray *intersectionLengths,
                                 vtkDataArray *scalars,
                                 vtkDataArray *nearIntersections,
                                 vtkDataArray *farIntersections) VTK_OVERRIDE;

  vtkSetObjectMacro(RayCastFunction, vtkUnstructuredGridBunykRayCastFunction);
  vtkGetObjectMacro(RayCastFunction, vtkUnstructuredGridBunykRayCastFunction);

protected:
  vtkUnstructuredGridBunykRayCastIterator();
  ~vtkUnstructuredGridBunykRayCastIterator() VTK_OVERRIDE;

  int RayPosition[2];

  vtkUnstructuredGridBunykRayCastFunction *RayCastFunction;

  vtkUnstructuredGridBunykRayCastFunction::Intersection *IntersectionPtr;
  vtkUnstructuredGridBunykRayCastFunction::Triangle     *CurrentTriangle;
  vtkIdType                                              CurrentTetra;

private:
  vtkUnstructuredGridBunykRayCastIterator(const vtkUnstructuredGridBunykRayCastIterator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkUnstructuredGridBunykRayCastIterator&) VTK_DELETE_FUNCTION;
};

vtkStandardNewMacro(vtkUnstructuredGridBunykRayCastIterator);

vtkUnstructuredGridBunykRayCastIterator::vtkUnstructuredGridBunykRayCastIterator()
{
  this->RayCastFunction = NULL;
}

vtkUnstructuredGridBunykRayCastIterator::~vtkUnstructuredGridBunykRayCastIterator()
{
  this->SetRayCastFunction(NULL);
}

void vtkUnstructuredGridBunykRayCastIterator::Initialize(int x, int y)
{
  this->RayPosition[0] = x;  this->RayPosition[1] = y;

  this->IntersectionPtr
    = this->RayCastFunction->GetIntersectionList(this->RayPosition[0],
                                                 this->RayPosition[1]);
  this->CurrentTriangle = NULL;
  this->CurrentTetra = -1;

  // Intersect cells until we get to Bounds[0] (the near clip plane).
  TemplateCastRayWorker worker(
      this->RayCastFunction, 0, this->RayPosition[0], this->RayPosition[1],
      this->Bounds[0], this->IntersectionPtr, this->CurrentTriangle,
      this->CurrentTetra, NULL, NULL, this->MaxNumberOfIntersections);
  do
  {
    worker();
  }
  while (worker.NumIntersections > 0);
}

vtkIdType vtkUnstructuredGridBunykRayCastIterator::GetNextIntersections(
                                         vtkIdList *intersectedCells,
                                         vtkDoubleArray *intersectionLengths,
                                         vtkDataArray *scalars,
                                         vtkDataArray *nearIntersections,
                                         vtkDataArray *farIntersections)
{
  if (intersectedCells)
  {
    intersectedCells->SetNumberOfIds(this->MaxNumberOfIntersections);
  }
  if (intersectionLengths)
  {
    intersectionLengths->SetNumberOfComponents(1);
    intersectionLengths->SetNumberOfTuples(this->MaxNumberOfIntersections);
  }

  vtkIdType numIntersections = 0;

  if (!scalars)
  {
    TemplateCastRayWorker worker(
        this->RayCastFunction, 0, this->RayPosition[0], this->RayPosition[1],
        this->Bounds[1], this->IntersectionPtr, this->CurrentTriangle,
        this->CurrentTetra,
        intersectedCells ? intersectedCells->GetPointer(0) : NULL,
        intersectionLengths ? intersectionLengths->GetPointer(0) : NULL,
        this->MaxNumberOfIntersections);
    worker();
    numIntersections = worker.NumIntersections;
  }
  else
  {
    if (   (scalars->GetDataType() != nearIntersections->GetDataType())
        || (scalars->GetDataType() != farIntersections->GetDataType()) )
    {
      vtkErrorMacro(<< "Data types for scalars do not match up.");
    }

    nearIntersections->SetNumberOfComponents(scalars->GetNumberOfComponents());
    nearIntersections->SetNumberOfTuples(this->MaxNumberOfIntersections);
    farIntersections->SetNumberOfComponents(scalars->GetNumberOfComponents());
    farIntersections->SetNumberOfTuples(this->MaxNumberOfIntersections);

    TemplateCastRayWorker worker(
        this->RayCastFunction, scalars->GetNumberOfComponents(),
        this->RayPosition[0], this->RayPosition[1], this->Bounds[1],
        this->IntersectionPtr, this->CurrentTriangle, this->CurrentTetra,
        intersectedCells ? intersectedCells->GetPointer(0) : NULL,
        intersectionLengths ? intersectionLengths->GetPointer(0) : NULL,
        this->MaxNumberOfIntersections
        );

    if (!vtkArrayDispatch::Dispatch3SameValueType::Execute(scalars,
                                                           nearIntersections,
                                                           farIntersections,
                                                           worker))
    {
      vtkWarningMacro("Dispatch failed for scalars and intersections.");
    }
    else
    {
      numIntersections = worker.NumIntersections;
    }

    nearIntersections->SetNumberOfTuples(numIntersections);
    farIntersections->SetNumberOfTuples(numIntersections);
  }

  if (intersectedCells)
  {
    intersectedCells->SetNumberOfIds(numIntersections);
  }
  if (intersectionLengths)
  {
    intersectionLengths->SetNumberOfTuples(numIntersections);
  }

  return numIntersections;
}

//-----------------------------------------------------------------------------

// Constructor - initially everything to null, and create a matrix for use later
vtkUnstructuredGridBunykRayCastFunction::vtkUnstructuredGridBunykRayCastFunction()
{
  this->Renderer          = NULL;
  this->Volume            = NULL;
  this->Mapper            = NULL;
  this->Valid             = 0;
  this->Points            = NULL;
  this->Image             = NULL;
  this->TriangleList      = NULL;
  this->TetraTriangles    = NULL;
  this->TetraTrianglesSize= 0;
  this->NumberOfPoints    = 0;
  this->ImageSize[0]      = 0;
  this->ImageSize[1]      = 0;
  this->ViewToWorldMatrix = vtkMatrix4x4::New();

  for (int i = 0; i < VTK_BUNYKRCF_MAX_ARRAYS; i++ )
  {
    this->IntersectionBuffer[i] = NULL;
    this->IntersectionBufferCount[i] = 0;
  }

  this->SavedTriangleListInput       = NULL;
}

// Destructor - release all memory
vtkUnstructuredGridBunykRayCastFunction::~vtkUnstructuredGridBunykRayCastFunction()
{
  delete [] this->Points;

  this->ClearImage();
  delete [] this->Image;
  this->Image = NULL;

  delete [] this->TetraTriangles;

  int i;
  for (i = 0; i < 20; i++ )
  {
    delete [] this->IntersectionBuffer[i];
  }

  while ( this->TriangleList )
  {
    Triangle *tmp;
    tmp = this->TriangleList->Next;
    delete this->TriangleList;
    this->TriangleList = tmp;
  }

  this->ViewToWorldMatrix->Delete();
}

// Clear the intersection image. This does NOT release memory -
// it just sets the link pointers to NULL. The memory is
// contained in the IntersectionBuffer arrays.
void vtkUnstructuredGridBunykRayCastFunction::ClearImage()
{
  int i;
  if ( this->Image )
  {
    for ( i = 0; i < this->ImageSize[0]*this->ImageSize[1]; i++ )
    {
      this->Image[i] = NULL;
    }
  }

  for ( i = 0; i < VTK_BUNYKRCF_MAX_ARRAYS; i++ )
  {
    this->IntersectionBufferCount[i] = 0;
  }
}

// Since we are managing the memory ourself for these intersections,
// we need a new method. In this method we return an unused
// intersection element from our storage arrays. If we don't
// have one, we create a new storage array (unless we have run
// out of memory). The memory can never shrink, and will only be
// deleted when the class is destructed.
void *vtkUnstructuredGridBunykRayCastFunction::NewIntersection()
{
  // Look for the first buffer that has enough space, or the
  // first one that has not yet been allocated
  int i;
  for ( i = 0; i < VTK_BUNYKRCF_MAX_ARRAYS; i++ )
  {
    if ( !this->IntersectionBuffer[i] ||
         this->IntersectionBufferCount[i] < VTK_BUNYKRCF_ARRAY_SIZE )
    {
      break;
    }
  }

  // We have run out of space - return NULL
  if ( i == VTK_BUNYKRCF_MAX_ARRAYS )
  {
    vtkErrorMacro("Out of space for intersections!");
    return NULL;
  }

  // We need another array - allocate it and set its count to 0 indicating
  // that we have not used any elements yet
  if ( !this->IntersectionBuffer[i] )
  {
    this->IntersectionBuffer[i] = new Intersection[VTK_BUNYKRCF_ARRAY_SIZE];
    this->IntersectionBufferCount[i] = 0;
  }

  // Return the first unused element
  return (this->IntersectionBuffer[i] + (this->IntersectionBufferCount[i]++));

}

// The Initialize method is called from the ray caster at the start of
// rendering. In this method we check if the render is valid (there is
// a renderer, a volume, a mapper, input, etc). We build the basic
// structured if necessary. Then we compute the view dependent information
// such as plane equations and barycentric coordinates per triangle,
// tranfromed points in view space, and the intersection list per pixel.
void vtkUnstructuredGridBunykRayCastFunction::Initialize( vtkRenderer *ren,
                                                          vtkVolume   *vol )
{
  // Check if this is a valid render - we have all the required info
  // such as the volume, renderer, mapper, input, etc.
  this->Valid = this->CheckValidity( ren, vol );
  if ( !this->Valid )
  {
    return;
  }

  // Cache some objects for later use during rendering
  this->Mapper     = vtkUnstructuredGridVolumeRayCastMapper::SafeDownCast( vol->GetMapper() );
  this->Renderer   = ren;
  this->Volume     = vol;


  vtkUnstructuredGridBase *input = this->Mapper->GetInput();
  int numPoints = input->GetNumberOfPoints();

  // If the number of points have changed, recreate the structure
  if ( numPoints != this->NumberOfPoints )
  {
    delete [] this->Points;
    this->Points = new double[3*numPoints];
    this->NumberOfPoints = numPoints;
  }

  // Get the image size from the ray cast mapper. The ImageViewportSize is
  // the size of the whole viewport (this does not necessary equal pixel
  // size since we may be over / undersampling on the image plane). The size
  // (which will be stored in ImageSize) and the ImageOrigin represent the
  // subregion of the whole image that we will be considering.
  int size[2];
  this->Mapper->GetImageInUseSize( size );
  this->Mapper->GetImageOrigin( this->ImageOrigin );
  this->Mapper->GetImageViewportSize( this->ImageViewportSize );

  // If our intersection image is not the right size, recreate it.
  // Clear out any old intersections
  this->ClearImage();
  if ( this->ImageSize[0]*this->ImageSize[1] != size[0]*size[1] )
  {
    delete [] this->Image;
    this->Image = new Intersection *[size[0]*size[1]];
    this->ImageSize[0] = size[0];
    this->ImageSize[1] = size[1];
    this->ClearImage();
  }

  // Transform the points. As a by product, compute the ViewToWorldMatrix
  // that will be used later
  this->TransformPoints();

  // If it has not yet been built, or the data has changed in
  // some way, we will need to recreate the triangle list. This is
  // view independent - although we will leave space in the structure
  // for the view dependent info
  this->UpdateTriangleList();

  // For each triangle store the plane equation and barycentric
  // coefficients to be used to speed up rendering
  this->ComputeViewDependentInfo();

  // Project each boundary triangle onto the image and store intersections
  // sorted by depth
  this->ComputePixelIntersections();
}

int vtkUnstructuredGridBunykRayCastFunction::CheckValidity( vtkRenderer *ren,
                                                            vtkVolume *vol )
{
  // We must have a renderer
  if ( !ren )
  {
    vtkErrorMacro("No Renderer");
    return 0;
  }

  // We must have a volume
  if ( !vol )
  {
    vtkErrorMacro("No Volume");
    return 0;
  }

  // We must have a mapper of the correct type
  vtkUnstructuredGridVolumeRayCastMapper *mapper =
    vtkUnstructuredGridVolumeRayCastMapper::SafeDownCast( vol->GetMapper() );
  if ( !mapper )
  {
    vtkErrorMacro("No mapper or wrong type");
    return 0;
  }

  // The mapper must have input
  vtkUnstructuredGridBase *input = mapper->GetInput();
  if ( !input )
  {
    vtkErrorMacro("No input to mapper");
    return 0;
  }

  // The input must have some points. This is a silent
  // error - just render nothing if it occurs.
  int numPoints = input->GetNumberOfPoints();
  if ( numPoints == 0 )
  {
    this->Valid = 0;
    return 0;
  }

  return 1;
}


// This is done once per render - transform the points into view coordinate.
// We also compute the ViewToWorldMatrix here (by inverting the matrix
// we use to project to view coordinates) so that later on in the
// rendering process we can convert points back to world coordinates.
void vtkUnstructuredGridBunykRayCastFunction::TransformPoints()
{
  vtkRenderer *ren = this->Renderer;
  vtkVolume   *vol = this->Volume;

  ren->ComputeAspect();
  double *aspect = ren->GetAspect();

  vtkTransform *perspectiveTransform = vtkTransform::New();
  vtkMatrix4x4 *perspectiveMatrix = vtkMatrix4x4::New();

  // Get the view matrix in two steps - there is a one step method in camera
  // but it turns off stereo so we do not want to use that one
  vtkCamera *cam = ren->GetActiveCamera();
  perspectiveTransform->Identity();
  perspectiveTransform->
    Concatenate(cam->GetProjectionTransformMatrix(aspect[0]/
                                                  aspect[1], 0.0, 1.0 ));
  perspectiveTransform->Concatenate(cam->GetViewTransformMatrix());
  perspectiveTransform->Concatenate(vol->GetMatrix());
  perspectiveMatrix->DeepCopy(perspectiveTransform->GetMatrix());

  // Invert this project matrix and store for later use
  this->ViewToWorldMatrix->DeepCopy(perspectiveTransform->GetMatrix());
  this->ViewToWorldMatrix->Invert();

  double *origPtr;
  double *transformedPtr = this->Points;
  double in[4], out[4];
  in[3] = 1.0;
  vtkUnstructuredGridBase *input = this->Mapper->GetInput();
  int numPoints = input->GetNumberOfPoints();

  // Loop through all the points and transform them
  for ( int i = 0; i < numPoints; i++ )
  {
    origPtr = input->GetPoint( i );
    in[0] = origPtr[0];
    in[1] = origPtr[1];
    in[2] = origPtr[2];
    perspectiveMatrix->MultiplyPoint( in, out );
    transformedPtr[0] = (out[0]/out[3] + 1.0)/2.0 *
      (double)this->ImageViewportSize[0] - this->ImageOrigin[0];
    transformedPtr[1] = (out[1]/out[3] + 1.0)/2.0 *
      (double)this->ImageViewportSize[1] - this->ImageOrigin[1];
    transformedPtr[2] =  out[2]/out[3];

    transformedPtr += 3;
  }

  perspectiveTransform->Delete();
  perspectiveMatrix->Delete();

}

// This is done once per change in the data - build a list of
// enumerated triangles (up to four per tetra). Don't store
// duplicates, so we'll have to search for them.
void  vtkUnstructuredGridBunykRayCastFunction::UpdateTriangleList()
{
  int needsUpdate = 0;

  // If we have never created the list, we need updating
  if ( !this->TriangleList )
  {
    needsUpdate = 1;
  }

  // If the data has changed in some way then we need to update
  vtkUnstructuredGridBase *input = this->Mapper->GetInput();
  if ( this->SavedTriangleListInput != input ||
       input->GetMTime() > this->SavedTriangleListMTime.GetMTime() )
  {
    needsUpdate = 1;
  }


  // If we don't need updating, return
  if ( !needsUpdate )
  {
    return;
  }


  // Clear out the old triangle list
  while ( this->TriangleList )
  {
    Triangle *tmp;
    tmp = this->TriangleList->Next;
    delete this->TriangleList;
    this->TriangleList = tmp;
  }
  this->TriangleList = NULL;

  // A temporary structure to reduce search time - VTK_BUNYKRCF_NUMLISTS small
  // lists instead of one big one
  Triangle *tmpList[VTK_BUNYKRCF_NUMLISTS];

  vtkIdType i;
  for ( i = 0; i < VTK_BUNYKRCF_NUMLISTS; i++ )
  {
    tmpList[i] = NULL;
  }

  vtkIdType numCells = input->GetNumberOfCells();

  // Provide a warnings for anomalous conditions.
  int nonTetraWarningNeeded = 0;
  int faceUsed3TimesWarning = 0;

  // Create a set of links from each tetra to the four triangles
  // This is redundant information, but saves time during rendering

  if(this->TetraTriangles!=0 && numCells!=this->TetraTrianglesSize)
  {
    delete [] this->TetraTriangles;
    this->TetraTriangles=0;
  }
  if(this->TetraTriangles==0)
  {
    this->TetraTriangles = new Triangle *[4 * numCells];
    this->TetraTrianglesSize=numCells;
  }

  // Loop through all the cells
  vtkSmartPointer<vtkCellIterator> cellIter =
      vtkSmartPointer<vtkCellIterator>::Take(input->NewCellIterator());
  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal();
       cellIter->GoToNextCell())
  {
    // We only handle tetra
    if (cellIter->GetCellType() != VTK_TETRA)
    {
      nonTetraWarningNeeded = 1;
      continue;
    }

    // Get the four points
    i = cellIter->GetCellId();
    vtkIdList *ptIds = cellIter->GetPointIds();
    vtkIdType pts[4];
    pts[0] = ptIds->GetId(0);
    pts[1] = ptIds->GetId(1);
    pts[2] = ptIds->GetId(2);
    pts[3] = ptIds->GetId(3);

    // Build each of the four triangles
    int ii, jj;
    for ( jj = 0; jj < 4; jj++ )
    {
      vtkIdType tri[3];
      int idx = 0;
      for ( ii = 0; ii < 4; ii++ )
      {
        if ( ii != jj )
        {
          tri[idx++] = pts[ii];
        }
      }

      if ( tri[0] > tri[1] )
      {
        vtkIdType tmptri = tri[0];
        tri[0] = tri[1];
        tri[1] = tmptri;
      }
      if ( tri[1] > tri[2] )
      {
        vtkIdType tmptri = tri[1];
        tri[1] = tri[2];
        tri[2] = tmptri;
      }
      if ( tri[0] > tri[1] )
      {
        vtkIdType tmptri = tri[0];
        tri[0] = tri[1];
        tri[1] = tmptri;
      }

      // Do we have this triangle already?
      Triangle *triPtr = tmpList[tri[0]%VTK_BUNYKRCF_NUMLISTS];
      while ( triPtr )
      {
        if ( triPtr->PointIndex[0] == tri[0] &&
             triPtr->PointIndex[1] == tri[1] &&
             triPtr->PointIndex[2] == tri[2] )
        {
          break;
        }
        triPtr = triPtr->Next;
      }

      if ( triPtr )
      {
        if ( triPtr->ReferredByTetra[1] != -1 )
        {
          faceUsed3TimesWarning = 1;
        }
        triPtr->ReferredByTetra[1] = i;
        this->TetraTriangles[i*4+jj] = triPtr;
      }
      else
      {
        Triangle *next = new Triangle;
        next->PointIndex[0] = tri[0];
        next->PointIndex[1] = tri[1];
        next->PointIndex[2] = tri[2];
        next->ReferredByTetra[0] =  i;
        next->ReferredByTetra[1] = -1;

        next->Next = tmpList[tri[0]%VTK_BUNYKRCF_NUMLISTS];
        tmpList[tri[0]%VTK_BUNYKRCF_NUMLISTS] = next;
        this->TetraTriangles[i*4+jj] = next;
      }
    }
  }

  if ( nonTetraWarningNeeded )
  {
    vtkWarningMacro("Input contains more than tetrahedra - only tetrahedra are supported");
  }
  if ( faceUsed3TimesWarning )
  {
    vtkWarningMacro("Degenerate topology - cell face used more than twice");
  }

  // Put the list together
  for ( i = 0; i < VTK_BUNYKRCF_NUMLISTS; i++ )
  {
    if ( tmpList[i] )
    {
      Triangle *last = tmpList[i];
      while ( last->Next )
      {
        last = last->Next;
      }
      last->Next = this->TriangleList;
      this->TriangleList = tmpList[i];
    }
  }

  this->SavedTriangleListInput = input;
  this->SavedTriangleListMTime.Modified();
}

void  vtkUnstructuredGridBunykRayCastFunction::ComputeViewDependentInfo()
{
  Triangle *triPtr = this->TriangleList;
  while ( triPtr )
  {
    double P1[3], P2[3];
    double A[3], B[3], C[3];

    A[0] = this->Points[3*triPtr->PointIndex[0]];
    A[1] = this->Points[3*triPtr->PointIndex[0]+1];
    A[2] = this->Points[3*triPtr->PointIndex[0]+2];
    B[0] = this->Points[3*triPtr->PointIndex[1]];
    B[1] = this->Points[3*triPtr->PointIndex[1]+1];
    B[2] = this->Points[3*triPtr->PointIndex[1]+2];
    C[0] = this->Points[3*triPtr->PointIndex[2]];
    C[1] = this->Points[3*triPtr->PointIndex[2]+1];
    C[2] = this->Points[3*triPtr->PointIndex[2]+2];

    P1[0] = B[0] - A[0];
    P1[1] = B[1] - A[1];
    P1[2] = B[2] - A[2];

    P2[0] = C[0] - A[0];
    P2[1] = C[1] - A[1];
    P2[2] = C[2] - A[2];

    triPtr->Denominator = P1[0]*P2[1] - P2[0]*P1[1];

    if ( triPtr->Denominator < 0 )
    {
      double T[3];
      triPtr->Denominator = -triPtr->Denominator;
      T[0]  = P1[0];
      T[1]  = P1[1];
      T[2]  = P1[2];
      P1[0] = P2[0];
      P1[1] = P2[1];
      P1[2] = P2[2];
      P2[0] = T[0];
      P2[1] = T[1];
      P2[2] = T[2];
      vtkIdType tmpIndex = triPtr->PointIndex[1];
      triPtr->PointIndex[1] = triPtr->PointIndex[2];
      triPtr->PointIndex[2] = tmpIndex;
    }

    triPtr->P1X = P1[0];
    triPtr->P1Y = P1[1];
    triPtr->P2X = P2[0];
    triPtr->P2Y = P2[1];

    double result[3];
    vtkMath::Cross( P1, P2, result );
    triPtr->A = result[0];
    triPtr->B = result[1];
    triPtr->C = result[2];
    triPtr->D = -(A[0]*result[0] + A[1]*result[1] + A[2]*result[2]);

    triPtr = triPtr->Next;
  }
}

void vtkUnstructuredGridBunykRayCastFunction::ComputePixelIntersections()
{
  Triangle *triPtr = this->TriangleList;
  while ( triPtr )
  {
    if ( triPtr->ReferredByTetra[1] == -1 )
    {
      if ( this->IsTriangleFrontFacing( triPtr, triPtr->ReferredByTetra[0] ) )
      {
        int   minX = static_cast<int>(this->Points[3*triPtr->PointIndex[0]]);
        int   maxX = minX+1;
        int   minY = static_cast<int>(this->Points[3*triPtr->PointIndex[0]+1]);
        int   maxY = minY+1;

        int tmp;

        tmp = static_cast<int>(this->Points[3*triPtr->PointIndex[1]]);
        minX = (tmp<minX)?(tmp):(minX);
        maxX = ((tmp+1)>maxX)?(tmp+1):(maxX);

        tmp = static_cast<int>(this->Points[3*triPtr->PointIndex[1]+1]);
        minY = (tmp<minY)?(tmp):(minY);
        maxY = ((tmp+1)>maxY)?(tmp+1):(maxY);

        tmp = static_cast<int>(this->Points[3*triPtr->PointIndex[2]]);
        minX = (tmp<minX)?(tmp):(minX);
        maxX = ((tmp+1)>maxX)?(tmp+1):(maxX);

        tmp = static_cast<int>(this->Points[3*triPtr->PointIndex[2]+1]);
        minY = (tmp<minY)?(tmp):(minY);
        maxY = ((tmp+1)>maxY)?(tmp+1):(maxY);

        double minZ = this->Points[3*triPtr->PointIndex[0]+2];
        double ftmp;

        ftmp = this->Points[3*triPtr->PointIndex[1]+2];
        minZ = (ftmp<minZ)?(ftmp):(minZ);

        ftmp = this->Points[3*triPtr->PointIndex[2]+2];
        minZ = (ftmp<minZ)?(ftmp):(minZ);

        if ( minX < this->ImageSize[0] - 1 &&
             minY < this->ImageSize[1] - 1 &&
             maxX >= 0 && maxY >= 0 && minZ > 0.0 )
        {
          minX = (minX<0)?(0):(minX);
          maxX = (maxX>(this->ImageSize[0]-1))?(this->ImageSize[0]-1):(maxX);

          minY = (minY<0)?(0):(minY);
          maxY = (maxY>(this->ImageSize[1]-1))?(this->ImageSize[1]-1):(maxY);

          int x, y;
          double ax, ay, az;
          ax = this->Points[3*triPtr->PointIndex[0]];
          ay = this->Points[3*triPtr->PointIndex[0]+1];
          az = this->Points[3*triPtr->PointIndex[0]+2];

          for ( y = minY; y <= maxY; y++ )
          {
            double qy = (double)y - ay;
            for ( x = minX; x <= maxX; x++ )
            {
              double qx = (double)x - ax;
              if ( this->InTriangle( qx, qy, triPtr ) )
              {
                Intersection *intersect = (Intersection *)this->NewIntersection();
                if ( intersect )
                {
                  intersect->TriPtr = triPtr;
                  intersect->Z      = az;
                  intersect->Next   = NULL;

                  if ( !this->Image[y*this->ImageSize[0] + x] ||
                       intersect->Z < this->Image[y*this->ImageSize[0] + x]->Z )
                  {
                    intersect->Next = this->Image[y*this->ImageSize[0] + x];
                    this->Image[y*this->ImageSize[0] + x] = intersect;
                  }
                  else
                  {
                    Intersection *test = this->Image[y*this->ImageSize[0] + x];
                    while ( test->Next && intersect->Z > test->Next->Z )
                    {
                      test = test->Next;
                    }
                    Intersection *tmpNext = test->Next;
                    test->Next = intersect;
                    intersect->Next = tmpNext;
                  }
                }
              }
            }
          }
        }
      }
    }
    triPtr = triPtr->Next;
  }
}

// Taken from equation on bottom of left column of page 3 - but note that the
// equation in the paper has a mistake: (q1+q2) must be less than 1 (not denom as
// stated in the paper).
int  vtkUnstructuredGridBunykRayCastFunction::InTriangle( double x, double y, Triangle *triPtr )
{
  double q1, q2;

  q1 = (x*triPtr->P2Y - y*triPtr->P2X) / triPtr->Denominator;
  q2 = (y*triPtr->P1X - x*triPtr->P1Y) / triPtr->Denominator;

  if ( q1 >= 0 && q2 >= 0 && (q1+q2) <= 1.0 )
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

int  vtkUnstructuredGridBunykRayCastFunction::IsTriangleFrontFacing( Triangle *triPtr, vtkIdType tetraIndex )
{
  vtkCell *cell = this->Mapper->GetInput()->GetCell(tetraIndex);

  vtkIdType pts[4];
  pts[0] = cell->GetPointId(0);
  pts[1] = cell->GetPointId(1);
  pts[2] = cell->GetPointId(2);
  pts[3] = cell->GetPointId(3);

  for( int i = 0; i < 4; i++ )
  {
    if ( pts[i] != triPtr->PointIndex[0] &&
         pts[i] != triPtr->PointIndex[1] &&
         pts[i] != triPtr->PointIndex[2] )
    {
      double d =
        triPtr->A*this->Points[3*pts[i]] +
        triPtr->B*this->Points[3*pts[i]+1] +
        triPtr->C*this->Points[3*pts[i]+2] +
        triPtr->D;

      return (d>0);
    }
  }

  assert(0);
  return false;
}

vtkUnstructuredGridVolumeRayCastIterator
    *vtkUnstructuredGridBunykRayCastFunction::NewIterator()
{
  if (!this->Valid) return NULL;

  vtkUnstructuredGridBunykRayCastIterator *iterator
    = vtkUnstructuredGridBunykRayCastIterator::New();
  iterator->SetRayCastFunction(this);

  return iterator;
}

void vtkUnstructuredGridBunykRayCastFunction::Finalize( )
{
  this->Renderer = NULL;
  this->Volume   = NULL;
  this->Mapper   = NULL;

  this->Valid    = 0;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridBunykRayCastFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  // Do not want to print this->ViewToWorldMatrix , this->ImageViewportSize
  // this->ScalarOpacityUnitDistance , or this->ImageOrigin - these are
  // internal ivar and not part of the public API for this class
}


