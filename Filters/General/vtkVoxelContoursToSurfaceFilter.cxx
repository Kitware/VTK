/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoxelContoursToSurfaceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVoxelContoursToSurfaceFilter.h"

#include "vtkAppendPolyData.h"
#include "vtkCellArray.h"
#include "vtkContourFilter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStructuredPoints.h"

vtkStandardNewMacro(vtkVoxelContoursToSurfaceFilter);

vtkVoxelContoursToSurfaceFilter::vtkVoxelContoursToSurfaceFilter()
{
  this->MemoryLimitInBytes = 10000000;
  this->Spacing[0]         = 1.0;
  this->Spacing[1]         = 1.0;
  this->Spacing[2]         = 1.0;
  this->LineList           = new double[4*1000];
  this->LineListLength     = 0;
  this->LineListSize       = 1000;
  this->SortedXList        = NULL;
  this->SortedYList        = NULL;
  this->WorkingList        = NULL;
  this->IntersectionList   = NULL;
  this->SortedListSize     = 0;
}

vtkVoxelContoursToSurfaceFilter::~vtkVoxelContoursToSurfaceFilter()
{
  delete [] this->LineList;
  delete [] this->SortedXList;
  delete [] this->SortedYList;
  delete [] this->WorkingList;
  delete [] this->IntersectionList;
}

void vtkVoxelContoursToSurfaceFilter::AddLineToLineList( double x1, double y1,
                                                   double x2, double y2 )
{
  // Do we need to increase the size of our list?
  if ( this->LineListLength >= this->LineListSize )
    {
    // Double the space we had before
    double *newList = new double[this->LineListSize*4*2];
    memcpy( newList, this->LineList, 4*this->LineListSize*sizeof(double) );
    delete [] this->LineList;
    this->LineList = newList;
    this->LineListSize *= 2;
    }

  // Now we are sure we have space - add the line
  this->LineList[4*this->LineListLength + 0] = x1;
  this->LineList[4*this->LineListLength + 1] = y1;
  this->LineList[4*this->LineListLength + 2] = x2;
  this->LineList[4*this->LineListLength + 3] = y2;
  this->LineListLength++;
}

void vtkVoxelContoursToSurfaceFilter::SortLineList()
{
  int     i, j;
  double   tmp[4];
  double   tmpval;


  // Make sure we have enough space in our sorted list
  if ( this->SortedListSize < this->LineListLength )
    {
    delete [] this->SortedXList;
    delete [] this->SortedYList;
    delete [] this->WorkingList;
    delete [] this->IntersectionList;

    this->SortedXList = new double[4*this->LineListLength];
    this->SortedYList = new double[4*this->LineListLength];
    this->SortedListSize = this->LineListLength;

    // Create the space we'll need for our working list of indices
    // The is the list of lines that we are currently considering
    // for intersections. Lines move in then out of the list as
    // we pass the first then the second endpoint. This will be
    // used during the CastXLines and CastYLines methods, and is
    // the same size as the number of lines.
    this->WorkingList = new int[this->LineListLength];

    // Create the space we'll need for the intersection list
    // There can't be more intersections than there are possible
    // lines. Although it is highly doubtful we'll actually use
    // all this space, it isn't much and it makes the code simpler
    // not to have to worry about exceeding the bounds. This will be
    // used during the CastXLines and CastYLines methods, and is
    // the same size as the number of lines.
    this->IntersectionList = new double[this->LineListLength];
    }

  // Copy the lines into the lists
  memcpy( this->SortedXList, this->LineList,
          4*this->LineListLength*sizeof(double) );
  memcpy( this->SortedYList, this->LineList,
          4*this->LineListLength*sizeof(double) );

  // Now sort on x and y
  // Use a simple bubble sort - will improve if necessary
  for ( i = 0; i < this->LineListLength; i++ )
    {
    // swap x entry if necessary to keep min x the first endpoint
    if ( this->SortedXList[4*i + 0] > this->SortedXList[4*i + 2] )
      {
      tmpval = this->SortedXList[4*i];
      this->SortedXList[4*i] = this->SortedXList[4*i + 2];
      this->SortedXList[4*i + 2] = tmpval;
      tmpval = this->SortedXList[4*i + 1];
      this->SortedXList[4*i + 1] = this->SortedXList[4*i + 3];
      this->SortedXList[4*i + 3] = tmpval;
      }

    // swap y entry if necessary to keep min y the first endpoint
    if ( this->SortedYList[4*i + 1] > this->SortedYList[4*i + 3] )
      {
      tmpval = this->SortedYList[4*i];
      this->SortedYList[4*i] = this->SortedYList[4*i + 2];
      this->SortedYList[4*i + 2] = tmpval;
      tmpval = this->SortedYList[4*i + 1];
      this->SortedYList[4*i + 1] = this->SortedYList[4*i + 3];
      this->SortedYList[4*i + 3] = tmpval;
      }

    // Sort x list
    for ( j = i; j > 0; j-- )
      {
      if ( this->SortedXList[j*4] < this->SortedXList[(j-1)*4] )
        {
        memcpy( tmp, this->SortedXList + j*4, 4*sizeof(double) );
        memcpy( this->SortedXList + j*4,
                this->SortedXList + (j-1)*4, 4*sizeof(double) );
        memcpy( this->SortedXList + (j-1)*4, tmp, 4*sizeof(double) );
        }
      else
        {
        break;
        }
      }

    // Sort y list
    for ( j = i; j > 0; j-- )
      {
      if ( this->SortedYList[j*4+1] < this->SortedYList[(j-1)*4+1] )
        {
        memcpy( tmp, this->SortedYList + j*4, 4*sizeof(double) );
        memcpy( this->SortedYList + j*4, this->SortedYList + (j-1)*4,
                4*sizeof(double) );
        memcpy( this->SortedYList + (j-1)*4, tmp, 4*sizeof(double) );
        }
      else
        {
        break;
        }
      }
    }
}


void vtkVoxelContoursToSurfaceFilter::CastLines( float *slicePtr,
                                                 double gridOrigin[3],
                                                 int gridSize[3],
                                                 int type )
{
  double   axis1, axis2;
  double   d1, d2;
  int     index;
  int     i, j;
  double   tmp;
  double   *line;
  float   *currSlicePtr;
  int     currSlice;
  int     currentIntersection;
  double   sign;
  double   *sortedList;
  double   low1, low2, high1, high2;
  int     increment1, increment2;
  int     offset1, offset2, offset3, offset4;

  // this is the x direction
  if ( type == 0 )
    {
    low1 = gridOrigin[0];
    high1 = gridOrigin[0] + (double)gridSize[0];
    low2 = gridOrigin[1];
    high2 = gridOrigin[1] + (double)gridSize[1];
    increment1 = gridSize[0];
    increment2 = 1;
    sortedList = this->SortedXList;
    offset1 = 0;
    offset2 = 2;
    offset3 = 1;
    offset4 = 3;
    }
  // This is the y direction
  else
    {
    low1 = gridOrigin[1];
    high1 = gridOrigin[1] + (double)gridSize[1];
    low2 = gridOrigin[0];
    high2 = gridOrigin[0] + (double)gridSize[0];
    increment1 = 1;
    increment2 = gridSize[0];
    sortedList = this->SortedYList;
    offset1 = 1;
    offset2 = 3;
    offset3 = 0;
    offset4 = 2;
    }

  // Initialize the working list to nothing. We will start
  // looking at index = 0 for the next line to add to the
  // working list
  this->WorkingListLength = 0;
  index = 0;

  // Loop through the x or y lines
  for ( axis1 = low1, currSlice = 0; axis1 < high1; axis1 += 1.0, currSlice++ )
    {
    // Initialize the intersection list to nothing
    this->IntersectionListLength = 0;

    // Add lines to the working list if necessary
    for ( ; index < this->LineListLength; index++ )
      {
      if ( sortedList[4*index + offset1] < axis1 )
        {
        this->WorkingList[this->WorkingListLength] = index;
        this->WorkingListLength++;
        }
      else
        {
        break;
        }
      }

    // Do the intersections, removing lines from the
    // working list if necessary
    for ( i = 0; i < this->WorkingListLength; i++ )
      {
      line = sortedList + 4*this->WorkingList[i];

      // Yes, it intersects, add it to the intersection list
      if ( line[offset1] < axis1 && line[offset2] > axis1 )
        {
        // Compute the intersection distance
        // For x lines this is y = y1 + (y2 - y1)*((x - x1)/(x2 - x1))
        // For y lines this is x = x1 + (x2 - x1)*((y - y1)/(y2 - y1))
        this->IntersectionList[this->IntersectionListLength] =
          line[offset3] + (line[offset4] - line[offset3]) *
          ((axis1 - line[offset1]) / (line[offset2] - line[offset1] ));

        // Make sure this distance is sorted
        for ( j = this->IntersectionListLength; j > 0; j-- )
          {
          if ( this->IntersectionList[j] < this->IntersectionList[j-1] )
            {
            tmp = this->IntersectionList[j];
            this->IntersectionList[j] = this->IntersectionList[j-1];
            this->IntersectionList[j-1] = tmp;
            }
          else
            {
            break;
            }
          }
        this->IntersectionListLength++;
        }
      // No, it doesn't intersect, remove it from the working list
      else
        {
        for ( j = i; j < (this->WorkingListLength-1); j++ )
          {
          this->WorkingList[j] = this->WorkingList[j+1];
          }
        this->WorkingListLength--;
        i--;
        }
      }

    // Now we have all the intersections for the x or y line, in sorted
    // order. Use them to fill in distances (as long as there are
    // any)
    if ( this->IntersectionListLength )
      {
      currSlicePtr = slicePtr + currSlice*increment2;
      currentIntersection = 0;
      // We are starting outside which has a negative distance
      sign = -1.0;
      for ( axis2 = low2; axis2 < high2; axis2 += 1.0 )
        {
        while( currentIntersection < this->IntersectionListLength &&
               this->IntersectionList[currentIntersection] < axis2 )

          {
          currentIntersection++;

          // Each time we cross a line we are moving across an
          // inside/outside boundary
          sign *= -1.0;
          }
        // We are now positioned at an x or y value between currentIntersection
        // and currentIntersection - 1 (except at boundaries where we are
        // before intersection 0 or after the last intersection)

        if ( currentIntersection == 0 )
          {
          d1 = axis2 - this->IntersectionList[currentIntersection];
          *currSlicePtr = (*currSlicePtr > d1 )?(*currSlicePtr):(d1);
          }
        else if ( currentIntersection == this->IntersectionListLength )
          {
          d1 = this->IntersectionList[currentIntersection-1] - axis2;
          *currSlicePtr = (*currSlicePtr > d1 )?(*currSlicePtr):(d1);
          }
        else
          {
          d1 = axis2 - this->IntersectionList[currentIntersection-1];
          d2 = this->IntersectionList[currentIntersection] - axis2;
          d1 = ( d1 < d2 )?(d1):(d2);
          if ( type == 0 )
            {
            *currSlicePtr = sign*d1;
            }
          else
            {
            *currSlicePtr =
              (sign*(*currSlicePtr) < d1 )?(*currSlicePtr):(sign*d1);
            }
          }

        currSlicePtr += increment1;
        }
      }
    }
}

void vtkVoxelContoursToSurfaceFilter::PushDistances( float *volumePtr,
                                                     int gridSize[3],
                                                     int chunkSize )
{
  int    i, j, k;
  float  *vptr;

  // Push distances along x (both ways) and y (both ways) on each slice
  for ( k = 0; k < chunkSize; k++ )
    {
    // Do the x rows
    for ( j = 0; j < gridSize[1]; j++ )
      {
      vptr = volumePtr + k*gridSize[0]*gridSize[1] + j*gridSize[0];
      vptr++;

      // first one way
      for ( i = 1; i < gridSize[0]; i++ )
        {
        if ( *vptr > 0 &&  *(vptr-1) + 1 < *(vptr) )
          {
          *vptr = *(vptr-1) + 1;
          }
        else if ( *vptr < 0 && *(vptr-1) - 1 > *(vptr) )
          {
          *vptr = *(vptr-1) - 1;
          }
        vptr++;
        }

      vptr -= 2;
      i    -= 2;

      // then the other
      for ( ; i >= 0; i-- )
        {
        if ( *vptr > 0 &&  *(vptr+1) + 1 < *(vptr) )
          {
          *vptr = *(vptr+1) + 1;
          }
        else if ( *vptr < 0 && *(vptr+1) - 1 > *(vptr) )
          {
          *vptr = *(vptr+1) - 1;
          }
        }

      }


    // Do the y columns
    for ( i = 0; i < gridSize[0]; i++ )
      {
      vptr = volumePtr + k*gridSize[0]*gridSize[1] + i;

      vptr+=gridSize[0];

      // first one way
      for ( j = 1; j < gridSize[1]; j++ )
        {
        if ( *vptr > 0 &&  *(vptr-gridSize[0]) + 1 < *(vptr) )
          {
          *vptr = *(vptr-gridSize[0]) + 1;
          }
        else if ( *vptr < 0 && *(vptr-gridSize[0]) - 1 > *(vptr) )
          {
          *vptr = *(vptr-gridSize[0]) - 1;
          }
        vptr += gridSize[0];
        }

      vptr -= 2*gridSize[0];
      j    -= 2;

      // then the other
      for ( ; j >= 0; j-- )
        {
        if ( *vptr > 0 &&  *(vptr+gridSize[0]) + 1 < *(vptr) )
          {
          *vptr = *(vptr+gridSize[0]) + 1;
          }
        else if ( *vptr < 0 && *(vptr+gridSize[0]) - 1 > *(vptr) )
          {
          *vptr = *(vptr+gridSize[0]) - 1;
          }
        }

      }
    }
}

// Append data sets into single unstructured grid
int vtkVoxelContoursToSurfaceFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkCellArray         *inputPolys = input->GetPolys();
  int                  gridSize[3];
  double                gridOrigin[3];
  double                contourBounds[6];
  int                  chunkSize;
  int                  currentSlice, lastSlice, currentIndex;
  int                  i, j;
  int                  numberOfInputCells;
  int                  currentInputCellIndex;
  vtkIdType            npts = 0;
  vtkIdType            *pts = 0;
  double                point1[3], point2[3];
  double                currentZ;
  vtkStructuredPoints  *volume;
  float                *volumePtr, *slicePtr;
  vtkContourFilter     *contourFilter;
  vtkPolyData          *contourOutput;
  vtkAppendPolyData    *appendFilter;

  vtkDebugMacro(<<"Creating surfaces from contours");

  // Get the bounds of the input contours
  input->GetBounds( contourBounds );

  if (contourBounds[0] > contourBounds[1])
    { // empty input
    return 1;
    }

  // From the bounds, compute the grid size, and origin

  // The origin of the grid should be (-0.5, -0.5, 0.0) away from the
  // lower bounds of the contours. This is because we want the grid
  // to lie halfway between integer endpoint locations of the line
  // segments on each plane. Also, we want an extra plane on each end
  // for capping
  gridOrigin[0] = contourBounds[0] - 0.5;
  gridOrigin[1] = contourBounds[2] - 0.5;
  gridOrigin[2] = contourBounds[4] - 1.0;

  // The difference between the bounds, plus one to account a
  // sample on the first and last location, plus one to account
  // for the larger grid size ( the 0.5 unit border ) On Z, we
  // want to sample exactly on the contours so we don't need to
  // add the extra 1, but we have added two extra planes so we
  // need another 2.
  gridSize[0] = (int) (contourBounds[1] - contourBounds[0] + 2);
  gridSize[1] = (int) (contourBounds[3] - contourBounds[2] + 2);
  gridSize[2] = (int) (contourBounds[5] - contourBounds[4] + 3);

  // How many slices in a chunk? This will later be decremented
  // by one to account for the fact that the last slice in the
  // previous chuck is copied to the first slice in the next chunk.
  // Stay within memory limit. There are 4 bytes per double.
  chunkSize = this->MemoryLimitInBytes / ( gridSize[0] * gridSize[1] * 4 );
  if ( chunkSize > gridSize[2] )
    {
    chunkSize = gridSize[2];
    }

  currentSlice          = 0;
  currentZ              = contourBounds[4] - 1.0;
  currentIndex          = 0;
  lastSlice             = gridSize[2] - 1;
  numberOfInputCells    = inputPolys->GetNumberOfCells();
  currentInputCellIndex = 0;

  volume = vtkStructuredPoints::New();
  volume->SetDimensions( gridSize[0], gridSize[1], chunkSize );
  volume->SetSpacing( this->Spacing );
  volume->AllocateScalars( VTK_FLOAT, 1 );
  volumePtr =
    (float *)(volume->GetPointData()->GetScalars()->GetVoidPointer(0));


  contourFilter = vtkContourFilter::New();
  contourFilter->SetInputData( volume );
  contourFilter->SetNumberOfContours(1);
  contourFilter->SetValue( 0, 0.0 );

  appendFilter = vtkAppendPolyData::New();

  inputPolys->InitTraversal();
  inputPolys->GetNextCell( npts, pts );

  while ( currentSlice <= lastSlice )
    {
    // Make sure the origin of the volume is in the right
    // place so that the appended polydata all matches up
    // nicely.
    volume->SetOrigin( gridOrigin[0], gridOrigin[1],
                       gridOrigin[2] +
                       this->Spacing[2] * (currentSlice - (currentSlice!=0)) );

    for ( i = currentIndex; i < chunkSize; i++ )
      {
      slicePtr = volumePtr + i * gridSize[0] * gridSize[1];

      // Clear out the slice memory - set it all to a large negative
      // value indicating no surfaces are nearby, and we assume we
      // are outside of any surface
      for ( j = 0; j < gridSize[0] * gridSize[1]; j++ )
        {
        *(slicePtr+j) = -9.99e10;
        }

      // If we are past the end, don't do anything
      if ( currentSlice > lastSlice )
        {
        continue;
        }

      this->LineListLength = 0;

      // Read in the lines for the contours on this slice
      while ( currentInputCellIndex < numberOfInputCells )
        {
        // Check if we are still on the right z slice
        input->GetPoint( pts[0], point1 );
        if ( point1[2] != currentZ )
          {
          break;
          }

        // This contour is on the right z slice - add the lines
        // to our list
        for ( j = 0; j < npts; j++ )
          {
          input->GetPoint( pts[j], point1 );
          input->GetPoint( pts[(j+1)%npts], point2 );
          this->AddLineToLineList( point1[0], point1[1],
                                   point2[0], point2[1] );
          }

        inputPolys->GetNextCell( npts, pts );
        currentInputCellIndex++;
        }

      // Sort the contours in x and y
      this->SortLineList();

      // Cast lines in x and y filling in distance
      this->CastLines( slicePtr, gridOrigin, gridSize, 0 );
      this->CastLines( slicePtr, gridOrigin, gridSize, 1 );

      // Move on to the next slice
      currentSlice++;
      currentIndex++;
      currentZ += 1.0;
      }

    this->PushDistances( volumePtr, gridSize, chunkSize );

    // Update the contour filter and grab the output
    // Make a new output for it, then grab the output and
    // add it to the append filter, then delete the output
    // which is ok since it was registered by the appendFilter
    contourOutput = vtkPolyData::New();
    contourFilter->Update();
    contourOutput->ShallowCopy(contourFilter->GetOutput());
    appendFilter->AddInputData( contourOutput );
    contourOutput->Delete();


    if ( currentSlice <= lastSlice )
      {
      // Copy last slice to first slice
      memcpy( volumePtr, volumePtr + (chunkSize-1)*gridSize[0]*gridSize[1],
              sizeof(float) * gridSize[0] * gridSize[1] );

      // reset currentIndex to 1
      currentIndex = 1;
      }
    }

  appendFilter->Update();

  // Grab the appended data as the output to this filter
  output->SetPoints( appendFilter->GetOutput()->GetPoints() );
  output->SetVerts(  appendFilter->GetOutput()->GetVerts() );
  output->SetLines(  appendFilter->GetOutput()->GetLines() );
  output->SetPolys(  appendFilter->GetOutput()->GetPolys() );
  output->SetStrips( appendFilter->GetOutput()->GetStrips() );
  output->GetPointData()->PassData(appendFilter->GetOutput()->GetPointData());

  contourFilter->Delete();
  appendFilter->Delete();
  volume->Delete();

  return 1;
}


void vtkVoxelContoursToSurfaceFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Memory Limit (in bytes): " <<
    this->MemoryLimitInBytes << endl;

  os << indent << "Spacing: " << this->Spacing[0] << " " <<
    this->Spacing[1] << " " << this->Spacing[2] << endl;
}
