/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridBunykRayCastFunction.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredGridBunykRayCastFunction.h"
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
#include "vtkDataArray.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkVolumeProperty.h"

vtkCxxRevisionMacro(vtkUnstructuredGridBunykRayCastFunction, "1.4");
vtkStandardNewMacro(vtkUnstructuredGridBunykRayCastFunction);

#define VTK_BUNYKRCF_NUMLISTS 100000

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
  this->ViewToWorldMatrix = vtkMatrix4x4::New();
  
  for (int i = 0; i < VTK_BUNYKRCF_MAX_ARRAYS; i++ )
    {
    this->IntersectionBuffer[i] = NULL;
    this->IntersectionBufferCount[i] = 0;
    }

  this->ColorTable                   = NULL;
  this->ColorTableSize               = NULL;
  this->ColorTableShift              = NULL;
  this->ColorTableScale              = NULL;
  
  this->SavedRGBFunction             = NULL;
  this->SavedGrayFunction            = NULL;
  this->SavedScalarOpacityFunction   = NULL;
  this->SavedColorChannels           = NULL;
  this->SavedScalarOpacityDistance   = NULL;
  this->SavedSampleDistance          = 0.0;
  this->SavedBlendMode               = -1;
  this->SavedNumberOfComponents      = 0;
  this->SavedParametersInput         = NULL;
  
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
  
  this->SetNumberOfComponents( 0 );  
}

void vtkUnstructuredGridBunykRayCastFunction::SetNumberOfComponents( int num )
{
  if ( num == this->SavedNumberOfComponents )
    {
    return;
    }
  
  int i;
  for ( i = 0; i < this->SavedNumberOfComponents; i++ )
    {
    delete [] this->ColorTable[i];
    }
  
  delete [] this->ColorTable;
  delete [] this->ColorTableSize;
  delete [] this->ColorTableShift;
  delete [] this->ColorTableScale;
  
  delete [] this->SavedRGBFunction;
  delete [] this->SavedGrayFunction;
  delete [] this->SavedScalarOpacityFunction;
  delete [] this->SavedColorChannels;
  delete [] this->SavedScalarOpacityDistance;
  
  this->ColorTable                   = NULL;
  this->ColorTableSize               = NULL;
  this->ColorTableShift              = NULL;
  this->ColorTableScale              = NULL;
  
  this->SavedRGBFunction             = NULL;
  this->SavedGrayFunction            = NULL;
  this->SavedScalarOpacityFunction   = NULL;
  this->SavedColorChannels           = NULL;
  this->SavedScalarOpacityDistance   = NULL;
  this->SavedParametersInput         = NULL;
   
  this->SavedNumberOfComponents      = num;
  
  if ( num > 0 )
    {
    this->ColorTable      = new double *[num];
    this->ColorTableSize  = new    int  [num];
    this->ColorTableShift = new double  [num];
    this->ColorTableScale = new double  [num];
    
    this->SavedRGBFunction             = new vtkColorTransferFunction *[num];
    this->SavedGrayFunction            = new     vtkPiecewiseFunction *[num];
    this->SavedScalarOpacityFunction   = new     vtkPiecewiseFunction *[num];

    this->SavedColorChannels         = new    int [num];
    this->SavedScalarOpacityDistance = new double [num];
    
    for ( i = 0; i < num; i++ )
      {
      this->ColorTable[i]      = NULL;
      this->ColorTableSize[i]  = 0;
      this->ColorTableShift[i] = 0.0;
      this->ColorTableScale[i] = 1.0;
      
      this->SavedRGBFunction[i]            = NULL;
      this->SavedGrayFunction[i]           = NULL;
      this->SavedScalarOpacityFunction[i]  = NULL;
      this->SavedColorChannels[i]          = 0;
      this->SavedScalarOpacityDistance[i]  = 0.0;
      }
    }
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

// The Intialize method is called from the ray caster at the start of 
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
  this->Scalars    = (void *)this->Mapper->GetInput()->GetPointData()->GetScalars()->GetVoidPointer(0);
  this->ScalarType =  this->Mapper->GetInput()->GetPointData()->GetScalars()->GetDataType();
  
  
  vtkUnstructuredGrid *input = this->Mapper->GetInput();
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
  
  // Tranform the points. As a by product, compute the ViewToWorldMatrix
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

  // Update the tables for mapping scalar value to color / opacity
  this->UpdateColorTable();
  
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
  vtkUnstructuredGrid *input = mapper->GetInput();
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
  ren->ComputeAspect();
  float *aspect = ren->GetAspect();

  vtkTransform *perspectiveTransform = vtkTransform::New();
  vtkMatrix4x4 *perspectiveMatrix = vtkMatrix4x4::New();
  
  // Get the view matrix in two steps -  there is a one step method in camera but it turns
  // off stereo so we do not want to use that one
  vtkCamera *cam = ren->GetActiveCamera();
  perspectiveTransform->Identity();
  perspectiveTransform->Concatenate(cam->GetPerspectiveTransformMatrix(aspect[0]/aspect[1], 0.0, 1.0 ));
  perspectiveTransform->Concatenate(cam->GetViewTransformMatrix());
  perspectiveMatrix->DeepCopy(perspectiveTransform->GetMatrix());
  
  // Invert this project matrix and store for later use
  this->ViewToWorldMatrix->DeepCopy(perspectiveTransform->GetMatrix());
  this->ViewToWorldMatrix->Invert();
  
  float *origPtr;
  double *transformedPtr = this->Points;
  double in[4], out[4];
  in[3] = 1.0;
  vtkUnstructuredGrid *input = this->Mapper->GetInput();
  int numPoints = input->GetNumberOfPoints(); 

  // Loop through all the points and transform them
  for ( int i = 0; i < numPoints; i++ )
    {
    origPtr = input->GetPoint( i );
    in[0] = origPtr[0];
    in[1] = origPtr[1];
    in[2] = origPtr[2];
    perspectiveMatrix->MultiplyPoint( in, out );
    transformedPtr[0] = (out[0]/out[3] + 1.0)/2.0 * (double)this->ImageViewportSize[0] - this->ImageOrigin[0];
    transformedPtr[1] = (out[1]/out[3] + 1.0)/2.0 * (double)this->ImageViewportSize[1] - this->ImageOrigin[1];
    transformedPtr[2] =  out[2]/out[3];
  
    transformedPtr += 3;
    }
  
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
  vtkUnstructuredGrid *input = this->Mapper->GetInput();
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
  
  int i;
  for ( i = 0; i < VTK_BUNYKRCF_NUMLISTS; i++ )
    {
    tmpList[i] = NULL;      
    }
    
  int numCells = input->GetNumberOfCells();
  
  // Provide a warning if we find anything other than tetra
  int warningNeeded = 0;
    
  int searchCount = 0;

  // Create a set of links from each tetra to the four triangles
  // This is redundant information, but saves time during rendering
  this->TetraTriangles = new Triangle *[4 * numCells];
    
  // Loop through all the cells
  for ( i = 0; i < numCells; i++ )
    {
    // We only handle tetra
    if ( input->GetCellType(i) != VTK_TETRA )
      {
      warningNeeded = 1;
      continue;
      }
      
    // Get the cell
    vtkCell *cell = input->GetCell(i);

    // Get the four points
    int pts[4];
    pts[0] = cell->GetPointId(0);
    pts[1] = cell->GetPointId(1);
    pts[2] = cell->GetPointId(2);
    pts[3] = cell->GetPointId(3);
      
    // Build each of the four triangles
    int ii, jj;
    for ( jj = 0; jj < 4; jj++ )
      {
      int tri[3];
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
        int tmptri = tri[0];
        tri[0] = tri[1];
        tri[1] = tmptri;
        }
      if ( tri[1] > tri[2] )
        {
        int tmptri = tri[1];
        tri[1] = tri[2];
        tri[2] = tmptri;
        }
      if ( tri[0] > tri[1] )
        {
        int tmptri = tri[0];
        tri[0] = tri[1];
        tri[1] = tmptri;
        }
        
      // Do we have this triangle already?
      Triangle *triPtr = tmpList[tri[0]%VTK_BUNYKRCF_NUMLISTS];
      while ( triPtr )
        {
        searchCount++;
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
          vtkErrorMacro("Degenerate topology - cell face used more than twice");
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
    
  if ( warningNeeded )
    {
    vtkWarningMacro("Input contains more than tetrahedra - only tetrahedra are supported");
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
  int numExterior = 0;
  int numInterior = 0;
  
  Triangle *triPtr = this->TriangleList;
  while ( triPtr )
    {
    if ( triPtr->ReferredByTetra[1] != -1 )
      {
      numInterior++;
      }
    
    if ( triPtr->ReferredByTetra[1] == -1 )
      {
      numExterior++;
      
      if ( this->IsTriangleFrontFacing( triPtr, triPtr->ReferredByTetra[0] ) )
        {
        int   minX = static_cast<int>(this->Points[3*triPtr->PointIndex[0]]);
        int   maxX = static_cast<int>(minX+1);
        int   minY = static_cast<int>(this->Points[3*triPtr->PointIndex[0]+1]);
        int   maxY = static_cast<int>(minY+1);
        
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
                  Intersection *tmp = test->Next;
                  test->Next = intersect;
                  intersect->Next = tmp;
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

// Update the color table to store the mapping from scalar value to color/opacity.
// Although the volume property supports the notion of non-independent components, 
// this mapper only supports independent components (where each component specified
// an independent property, not a single property such as a 3 component dataset 
// representing color)
void vtkUnstructuredGridBunykRayCastFunction::UpdateColorTable()
{
  int needToUpdate = 0;

  // Get the image data
  vtkUnstructuredGrid *input = this->Mapper->GetInput();

  // Set the number of components. If this is different than previous, it will
  // reset all the arrays to the right size (filled with null). 
  int components = input->GetPointData()->GetScalars()->GetNumberOfComponents();
  this->SetNumberOfComponents( components );
  
  // Has the data itself changed?
  if ( input != this->SavedParametersInput ||
       input->GetMTime() > this->SavedParametersMTime.GetMTime() )
    {
    needToUpdate = 1;
    }

  // What is the blending mode?
  int blendMode = this->Mapper->GetBlendMode();
  if ( blendMode != this->SavedBlendMode )
    {
    needToUpdate = 1;
    }
  
  // Has the sample distance changed?
  if ( this->SavedSampleDistance != this->SampleDistance )
    {
    needToUpdate = 1;
    }
  
  vtkColorTransferFunction **rgbFunc               = new vtkColorTransferFunction *[components];
  vtkPiecewiseFunction     **grayFunc              = new     vtkPiecewiseFunction *[components];
  vtkPiecewiseFunction     **scalarOpacityFunc     = new     vtkPiecewiseFunction *[components];
  int                       *colorChannels         = new                      int  [components];
  float                     *scalarOpacityDistance = new                    float  [components];
  
  int c;
     
  vtkVolume *vol = this->Volume;
  for ( c = 0; c < components; c++ )
    {
    rgbFunc[c]               = vol->GetProperty()->GetRGBTransferFunction(c);
    grayFunc[c]              = vol->GetProperty()->GetGrayTransferFunction(c);
    scalarOpacityFunc[c]     = vol->GetProperty()->GetScalarOpacity(c);
    colorChannels[c]         = vol->GetProperty()->GetColorChannels(c);
    scalarOpacityDistance[c] = vol->GetProperty()->GetScalarOpacityUnitDistance(c);
    
    // Has the number of color channels changed?
    if ( this->SavedColorChannels[c] != colorChannels[c] )
      {
      needToUpdate = 1;
      }

    // Has the color transfer function changed in some way,
    // and we are using it?
    if ( colorChannels[c] == 3 )
      {
      if ( this->SavedRGBFunction[c] != rgbFunc[c] ||
           this->SavedParametersMTime.GetMTime() < rgbFunc[c]->GetMTime() )
        {
        needToUpdate = 1;
        }
      }

    // Has the gray transfer function changed in some way,
    // and we are using it?
    if ( colorChannels[c] == 1 )
      {
      if ( this->SavedGrayFunction[c] != grayFunc[c] ||
           this->SavedParametersMTime.GetMTime() < grayFunc[c]->GetMTime() )
        {
        needToUpdate = 1;
        }
      }
  
    // Has the scalar opacity transfer function changed in some way?
    if ( this->SavedScalarOpacityFunction[c] != scalarOpacityFunc[c] ||
         this->SavedParametersMTime.GetMTime() < scalarOpacityFunc[c]->GetMTime() )
      {
      needToUpdate = 1;
      }

    // Has the distance over which the scalar opacity function is defined changed?
    if ( this->SavedScalarOpacityDistance[c] != scalarOpacityDistance[c] )
      {
      needToUpdate = 1;
      }
    }
  
  // If we have not found any need to update, free memory and return now
  if ( !needToUpdate )
    {
    delete [] rgbFunc;
    delete [] grayFunc;
    delete [] scalarOpacityFunc;
    delete [] colorChannels;
    delete [] scalarOpacityDistance;
    return;
    }

    for ( c = 0; c < components; c++ )
    {
    this->SavedRGBFunction[c]             = rgbFunc[c];
    this->SavedGrayFunction[c]            = grayFunc[c];
    this->SavedScalarOpacityFunction[c]   = scalarOpacityFunc[c];
    this->SavedColorChannels[c]           = colorChannels[c];
    this->SavedScalarOpacityDistance[c]   = scalarOpacityDistance[c];
    }
  
  this->SavedSampleDistance          = this->SampleDistance;
  this->SavedBlendMode               = blendMode;
  this->SavedParametersInput         = input;
  
  this->SavedParametersMTime.Modified();

  int scalarType = input->GetPointData()->GetScalars()->GetDataType();
  
  int i;
  float tmpArray[3*65536];
  
  // Find the scalar range
  float *scalarRange = new float [2*components];
  for ( c = 0; c < components; c++ )
    {
    input->GetPointData()->GetScalars()->GetRange((scalarRange+2*c), c);
    
    // Is the difference between max and min less than 65536? If so, and if
    // the data is not of float or double type, use a simple offset mapping.
    // If the difference between max and min is 65536 or greater, or the data
    // is of type float or double, we must use an offset / scaling mapping.
    // In this case, the array size will be 65536 - we need to figure out the 
    // offset and scale factor.
    float offset;
    float scale;
    
    int arraySizeNeeded;
    
    if ( scalarType == VTK_FLOAT ||
         scalarType == VTK_DOUBLE ||
         scalarRange[c*2+1] - scalarRange[c*2] > 65535 )
      {
      arraySizeNeeded = 65536;
      offset          = -scalarRange[2*c];
      scale           = 65535.0 / (scalarRange[2*c+1] - scalarRange[2*c]);
      }
    else
      {
      arraySizeNeeded = static_cast<int>(scalarRange[2*c+1] - scalarRange[2*c]) + 1;
      offset          = -scalarRange[2*c]; 
      scale           = 1.0;
      }
    
    if ( this->ColorTableSize[c] != arraySizeNeeded )
      {
      delete [] this->ColorTable[c];
      this->ColorTable[c] = new double [4*arraySizeNeeded];
      }
    
    this->ColorTableSize[c]   = arraySizeNeeded;
    this->ColorTableShift[c]  = offset;
    this->ColorTableScale[c]  = scale;  
    }
  
  for ( c = 0; c < components; c++ )
    {
    // Sample the transfer functions between the min and max.
    if ( colorChannels[c] == 1 )
      {
      float tmpArray2[65536];
      grayFunc[c]->GetTable( scalarRange[c*2], scalarRange[c*2+1], 
                             this->ColorTableSize[c], tmpArray2 );
      for ( int index = 0; index < this->ColorTableSize[c]; index++ )
        {
        tmpArray[3*index  ] = tmpArray2[index];
        tmpArray[3*index+1] = tmpArray2[index];
        tmpArray[3*index+2] = tmpArray2[index];
        }
      }
    else
      {
      rgbFunc[c]->GetTable( scalarRange[c*2], scalarRange[c*2+1], 
                            this->ColorTableSize[c], tmpArray );
      }
    // Add color to the color table in double format
    for ( i = 0; i < this->ColorTableSize[c]; i++ )
      {
      this->ColorTable[c][4*i  ] = (double)(tmpArray[3*i  ]);
      this->ColorTable[c][4*i+1] = (double)(tmpArray[3*i+1]);
      this->ColorTable[c][4*i+2] = (double)(tmpArray[3*i+2]);
      }
    
    scalarOpacityFunc[c]->GetTable( scalarRange[c*2], scalarRange[c*2+1], 
                                    this->ColorTableSize[c], tmpArray );
    
    // Correct the opacity array for the spacing between the planes if we are
    // using a composite blending operation
    if ( this->Mapper->GetBlendMode() == vtkUnstructuredGridVolumeMapper::COMPOSITE_BLEND )
      {
      float *ptr = tmpArray;    
      double factor = this->SampleDistance / vol->GetProperty()->GetScalarOpacityUnitDistance(c);
      for ( i = 0; i < this->ColorTableSize[c]; i++ )
        {
        if ( *ptr > 0.0001 )
          {
          *ptr =  1.0-pow((double)(1.0-(*ptr)),factor);
          }
        ptr++;
        }
      }

    // Add opacity to color table in double format
    for ( i = 0; i < this->ColorTableSize[c]; i++ )
      {
      this->ColorTable[c][4*i+3] = (double)(tmpArray[i]);
      }    
    }
  
  // Clean up the temporary arrays we created
  delete [] rgbFunc;
  delete [] grayFunc;
  delete [] scalarOpacityFunc;
  delete [] colorChannels;
  delete [] scalarOpacityDistance;
  delete [] scalarRange;
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

int  vtkUnstructuredGridBunykRayCastFunction::IsTriangleFrontFacing( Triangle *triPtr, int tetraIndex )
{
  vtkCell *cell = this->Mapper->GetInput()->GetCell(tetraIndex);

  int pts[4];
  pts[0] = cell->GetPointId(0);
  pts[1] = cell->GetPointId(1);
  pts[2] = cell->GetPointId(2);
  pts[3] = cell->GetPointId(3);
  
  int i;
  for( i = 0; i < 4; i++ )
    {
    if ( pts[i] != triPtr->PointIndex[0] &&
         pts[i] != triPtr->PointIndex[1] &&
         pts[i] != triPtr->PointIndex[2] )
      {
      break;
      }
    }
  
  double d = 
    triPtr->A*this->Points[3*pts[i]] +
    triPtr->B*this->Points[3*pts[i]+1] +
    triPtr->C*this->Points[3*pts[i]+2] +
    triPtr->D;
  
  return (d>0);
}

template <class T>
void vtkUnstructuredGridBunykRayCastFunctionCastRay( T scalars, 
                                                     vtkUnstructuredGridBunykRayCastFunction *self,
                                                     int x, int y, 
                                                     double bounds[2],
                                                     float color[4] )
{
  int origin[2];
  self->GetImageOrigin( origin );
  float fx = x - origin[0];
  float fy = y - origin[1];

  double *points    = self->GetPoints();
  vtkUnstructuredGridBunykRayCastFunction::Triangle
    **triangles = self->GetTetraTriangles();
  
  vtkMatrix4x4 *viewToWorld = self->GetViewToWorldMatrix();
  
  vtkUnstructuredGridBunykRayCastFunction::Intersection 
    *intersectionPtr = self->GetIntersectionList( x, y ); 

  vtkUnstructuredGridBunykRayCastFunction::Triangle *currentTriangle;
  int currentTetra;

  vtkUnstructuredGridBunykRayCastFunction::Triangle *nextTriangle;
  int nextTetra;

  double **colorTable      = self->GetColorTable();
  double  *colorTableShift = self->GetColorTableShift();
  double  *colorTableScale = self->GetColorTableScale();

  while ( intersectionPtr )
    {
    currentTriangle = intersectionPtr->TriPtr;
    currentTetra    = intersectionPtr->TriPtr->ReferredByTetra[0];
    
    do
      {
      vtkUnstructuredGridBunykRayCastFunction::Triangle *candidate[3];
  
      int index = 0;
      int i;
      for ( i = 0; i < 4; i++ )
        {
        if ( triangles[currentTetra*4+i] != currentTriangle )
          {
          if ( index == 3 )
            {
            cout << "Ugh - found too many triangles!" << endl;
            }
          else
            {
            candidate[index++] = triangles[currentTetra*4+i];
            }
          }
        }

      double minZ = VTK_FLOAT_MAX;
      int minIdx = -1;
      
      double testZ = 
        -( fx*currentTriangle->A + 
           fy*currentTriangle->B + 
           currentTriangle->D) / currentTriangle->C;
      
      double tmpP[4], p1[4], p2[4];
      tmpP[0] = fx;
      tmpP[1] = fy;
      tmpP[2] = testZ;
      tmpP[3] = 1.0;
      viewToWorld->MultiplyPoint( tmpP, p1 );
      p1[0] /= p1[3];
      p1[1] /= p1[3];
      p1[2] /= p1[3];
      
      for ( i = 0; i < 3; i++ )
        {
        double ax, ay;
        ax = points[3*candidate[i]->PointIndex[0]];
        ay = points[3*candidate[i]->PointIndex[0]+1];
        
        if ( self->InTriangle( (fx-ax), (fy-ay), candidate[i] ) )
          {
          double tmpZ = 
            -( fx*candidate[i]->A + 
               fy*candidate[i]->B + 
               candidate[i]->D) / candidate[i]->C;
    
          if (tmpZ > testZ && tmpZ < minZ)
            {
            minZ = tmpZ;
            minIdx = i;
            }
          }
        }
      
      if ( minIdx == -1 || minZ > bounds[1] )
        {
        nextTriangle = NULL;
        nextTetra = -1;
        }
      else
        {
        nextTriangle = candidate[minIdx];
      
        tmpP[2] = minZ;
        viewToWorld->MultiplyPoint( tmpP, p2 );
        p2[0] /= p1[3];
        p2[1] /= p1[3];
        p2[2] /= p1[3];
        double dist = sqrt( (p1[0]-p2[0])*(p1[0]-p2[0]) + 
                            (p1[1]-p2[1])*(p1[1]-p2[1]) + 
                            (p1[2]-p2[2])*(p1[2]-p2[2]) );
      
      
        // Compute scalar of triangle 1
        double A, B, C;
        
        A = *(scalars + currentTriangle->PointIndex[0]);
        B = *(scalars + currentTriangle->PointIndex[1]);
        C = *(scalars + currentTriangle->PointIndex[2]);
      
        double ax, ay;
        ax = points[3*currentTriangle->PointIndex[0]];
        ay = points[3*currentTriangle->PointIndex[0]+1];
      
        double a, b, c;
        b = ((fx-ax)*currentTriangle->P2Y - (fy-ay)*currentTriangle->P2X) / currentTriangle->Denominator;
        c = ((fy-ay)*currentTriangle->P1X - (fx-ax)*currentTriangle->P1Y) / currentTriangle->Denominator;
        a = 1.0 - b - c;
      
        double v1 = a * A + b * B + c * C;
      
        A = *(scalars + nextTriangle->PointIndex[0]);
        B = *(scalars + nextTriangle->PointIndex[1]);
        C = *(scalars + nextTriangle->PointIndex[2]);
      
        ax = points[3*nextTriangle->PointIndex[0]];
        ay = points[3*nextTriangle->PointIndex[0]+1];
      
        b = ((fx-ax)*nextTriangle->P2Y - (fy-ay)*nextTriangle->P2X) / nextTriangle->Denominator;
        c = ((fy-ay)*nextTriangle->P1X - (fx-ax)*nextTriangle->P1Y) / nextTriangle->Denominator;
        a = 1.0 - b - c;
      
        double v2 = a * A + b * B + c * C;

        double color1[4], color2[4];
        double *tmpptr1 = 
          colorTable[0] +
          4 * (unsigned short)((v1+colorTableShift[0])*colorTableScale[0]); 
        double *tmpptr2 = 
          colorTable[0] +
          4 * (unsigned short)((v2+colorTableShift[0])*colorTableScale[0]); 

        color1[3] = *(tmpptr1+3);
        color1[0] = *(tmpptr1)   * color1[3];
        color1[1] = *(tmpptr1+1) * color1[3];
        color1[2] = *(tmpptr1+2) * color1[3];

        color2[3] = *(tmpptr2+3);
        color2[0] = *(tmpptr2)   * color2[3];
        color2[1] = *(tmpptr2+1) * color2[3];
        color2[2] = *(tmpptr2+2) * color2[3];

        color[0] = color[0] + 
          0.5*(color1[0]+color2[0])*(1.0 - color[3])*dist -
            (3*color1[0]*color1[3] + 5*color2[0]*color1[3] + 
             color1[0]*color2[3] + 3*color2[0]*color2[3]) * dist * dist / 24.0;
        
        color[1] = color[1] + 
          0.5*(color1[1]+color2[1])*(1.0 - color[3])*dist -
            (3*color1[1]*color1[3] + 5*color2[1]*color1[3] + 
             color1[1]*color2[3] + 3*color2[1]*color2[3]) * dist * dist / 24.0;
        
        color[2] = color[2] + 
          0.5*(color1[2]+color2[2])*(1.0 - color[3])*dist -
            (3*color1[2]*color1[3] + 5*color2[2]*color1[3] + 
             color1[2]*color2[3] + 3*color2[2]*color2[3]) * dist * dist / 24.0;

        color[3] = color[3] + (1-color[3])*0.5*(color1[3]+color2[3])*dist;
        
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
          if ( nextTriangle->ReferredByTetra[0] == currentTetra )
            {
            nextTetra = nextTriangle->ReferredByTetra[1];
            }
          else
            {
            nextTetra = nextTriangle->ReferredByTetra[0];
            }
          }
        }
      
      currentTriangle = nextTriangle;
      currentTetra    = nextTetra;
      
      } while (currentTriangle);
    
    intersectionPtr = intersectionPtr->Next;
    }
}

void vtkUnstructuredGridBunykRayCastFunction::CastRay( int x, int y, double bounds[2], float color[4] )
{
  color[0] = 0.0;
  color[1] = 0.0;
  color[2] = 0.0;
  color[3] = 0.0;
  
  if ( !this->Valid ) 
    {
    return;
    }
  
  
  switch ( this->ScalarType )
    {
    vtkTemplateMacro6( vtkUnstructuredGridBunykRayCastFunctionCastRay,
                      (VTK_TT *)this->Scalars, this,
                      x, y, bounds, color );
    }
  
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
  
  // Do not want to print ViewToWorldMatrix or ImageOrigin - these are internal
  // ivar and not part of the public API for this class
}


