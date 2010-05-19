/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeOutlineSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVolumeOutlineSource.h"

#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVolumeMapper.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkVolumeOutlineSource);

vtkCxxSetObjectMacro(vtkVolumeOutlineSource,VolumeMapper,vtkVolumeMapper);

//----------------------------------------------------------------------------
vtkVolumeOutlineSource::vtkVolumeOutlineSource ()
{
  this->VolumeMapper = 0;
  this->GenerateScalars = 0;
  this->GenerateFaces = 0;
  this->ActivePlaneId = -1;

  this->Color[0] = 1.0;
  this->Color[1] = 0.0;
  this->Color[2] = 0.0;

  this->ActivePlaneColor[0] = 1.0;
  this->ActivePlaneColor[1] = 1.0;
  this->ActivePlaneColor[2] = 0.0;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkVolumeOutlineSource::~vtkVolumeOutlineSource ()
{
  if (this->VolumeMapper)
    {
    this->VolumeMapper->Delete();
    this->VolumeMapper = 0;
    }
}

//----------------------------------------------------------------------------
void vtkVolumeOutlineSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "VolumeMapper: ";
  if (this->VolumeMapper)
    {
    os << this->VolumeMapper << "\n";
    }
  else
    {
    os << "(none)\n";
    } 

  os << indent << "GenerateFaces: "
     << (this->GenerateFaces ? "On\n" : "Off\n" );

  os << indent << "GenerateScalars: "
     << (this->GenerateScalars ? "On\n" : "Off\n" );

  os << indent << "Color: " << this->Color[0] << ", "
     << this->Color[1] << ", " << this->Color[2] << "\n";

  os << indent << "ActivePlaneId: " << this->ActivePlaneId << "\n";

  os << indent << "ActivePlaneColor: " << this->ActivePlaneColor[0] << ", "
     << this->ActivePlaneColor[1] << ", " << this->ActivePlaneColor[2] << "\n";
}

//----------------------------------------------------------------------------
int vtkVolumeOutlineSource::ComputeCubePlanes(
  double planes[3][4], double croppingPlanes[6], double bounds[6])
{
  // Combine the CroppingRegionPlanes and the Bounds to create
  // a single array.  For each dimension, store the planes in
  // the following order: lo_bound, lo_crop_plane, hi_crop_plane, hi_bound.
  // Also do range checking to ensure that the cropping planes
  // are clamped to the bound limits.

  for (int i = 0; i < 3; i++)
    {
    int j0 = 2*i;
    int j1 = 2*i + 1;

    double a = bounds[j0];
    double b = croppingPlanes[j0];
    double c = croppingPlanes[j1];
    double d = bounds[j1];

    // Sanity check
    if (a > d || b > c)
      {
      return 0;
      }

    // Clamp cropping planes to bounds
    if (b < a) { b = a; };
    if (b > d) { b = d; };
    if (c < a) { c = a; };
    if (c > d) { c = d; };

    planes[i][0] = a;
    planes[i][1] = b;
    planes[i][2] = c;
    planes[i][3] = d;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkVolumeOutlineSource::ComputePipelineMTime(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* vtkNotUsed(outputVector),
  int vtkNotUsed(requestFromOutputPort),
  unsigned long* mtime)
{
  unsigned long mTime = this->GetMTime();
  if (this->VolumeMapper)
    {
    unsigned long mapperMTime = this->VolumeMapper->GetMTime();
    if (mapperMTime > mTime)
      {
      mTime = mapperMTime;
      }
    vtkImageData *input = this->VolumeMapper->GetInput();
    if (input)
      {
      // Need to do this because we are not formally connected
      // to the Mapper's pipeline
      input->UpdateInformation();
      unsigned long pipelineMTime = input->GetPipelineMTime();
      if (pipelineMTime > mTime)
        {
        mTime = pipelineMTime;
        }
      }
    }

  *mtime = mTime;

  return 1;
}

//----------------------------------------------------------------------------
int vtkVolumeOutlineSource::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* vtkNotUsed(outputVector))
{
  // Get the mapper's input, since this is the most convenient
  // place to do so.

  if (!this->VolumeMapper)
    {
    vtkWarningMacro("No VolumeMapper has been set.");
    return 1;
    }

  this->Cropping = this->VolumeMapper->GetCropping();
  this->CroppingRegionFlags = this->VolumeMapper->GetCroppingRegionFlags();
  this->VolumeMapper->GetCroppingRegionPlanes(this->CroppingRegionPlanes);

  vtkImageData *data = this->VolumeMapper->GetInput();

  if (!data)
    {
    vtkWarningMacro("The VolumeMapper does not have an input set.");
    return 1;
    }

  // Don't have to update mapper's input, since it was done in
  // ComputePipelineMTime.
  // data->UpdateInformation();

  // Don't call GetBounds because we need WholeExtent, while
  // GetBounds only returns the bounds for Extent.

  double spacing[3];
  double origin[3];
  int extent[6];

  data->GetSpacing(spacing);
  data->GetOrigin(origin);
  data->GetWholeExtent(extent);

  for (int i = 0; i < 3; i++)
    {
    int j0 = 2*i;
    int j1 = j0+1;

    if (extent[j0] > extent[j1])
      {
      vtkMath::UninitializeBounds(this->Bounds);
      break;
      }

    if (spacing[i] > 0)
      {
      this->Bounds[j0] = origin[i] + spacing[i]*extent[j0];
      this->Bounds[j1] = origin[i] + spacing[i]*extent[j1];
      }
    else
      {
      this->Bounds[j0] = origin[i] + spacing[i]*extent[j1];
      this->Bounds[j1] = origin[i] + spacing[i]*extent[j0];
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkVolumeOutlineSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the output
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<< "Creating cropping region outline");

  // For each of the 3 dimensions, there are 4 planes: two bounding planes
  // on the outside, and two cropping region planes inside.
  double planes[3][4];

  if (!this->VolumeMapper || !this->VolumeMapper->GetInput() ||
      !this->ComputeCubePlanes(planes,this->CroppingRegionPlanes,this->Bounds))
    {
    // If the bounds or the cropping planes are invalid, clear the data
    output->SetPoints(0);
    output->SetLines(0);
    output->GetCellData()->SetScalars(0);

    return 1;
    }

  // Compute the tolerance for considering points or planes to be coincident
  double tol = 0;
  for (int planeDim = 0; planeDim < 3; planeDim++)
    {
    double d = planes[planeDim][3] - planes[planeDim][0];
    tol += d*d;
    }
  tol = sqrt(tol)*1e-5;

  // Create an array to nudge crop planes over to the bounds if they are
  // within tolerance of the bounds
  int tolPtId[3][4];
  this->NudgeCropPlanesToBounds(tolPtId, planes, tol);

  // The all-important cropping flags
  int flags = this->CroppingRegionFlags;

  // The active plane, which gets a special color for its scalars
  int activePlane = this->ActivePlaneId;
  if (activePlane > 5) { activePlane = -1; };

  // Convert the colors to unsigned char for scalars
  unsigned char colors[2][3];
  this->CreateColorValues(colors, this->Color, this->ActivePlaneColor);

  // Create the scalars used to color the lines
  vtkUnsignedCharArray *scalars = 0;

  if (this->GenerateScalars)
    {
    scalars = vtkUnsignedCharArray::New();
    scalars->SetNumberOfComponents(3);
    }

  // Generate all the lines for the outline.
  vtkCellArray *lines = vtkCellArray::New();
  this->GenerateLines(lines, scalars, colors, activePlane, flags, tolPtId);

  // Generate the polys for the outline
  vtkCellArray *polys =  0;

  if (this->GenerateFaces)
    {
    polys = vtkCellArray::New();
    this->GeneratePolys(polys, scalars, colors, activePlane, flags, tolPtId);
    }

  // Generate the points that are used by the lines.
  vtkPoints *points = vtkPoints::New();
  this->GeneratePoints(points, lines, polys, planes, tol);

  output->SetPoints(points);
  points->Delete();

  output->SetPolys(polys);
  if (polys)
    {
    polys->Delete();
    }

  output->SetLines(lines);
  lines->Delete();

  output->GetCellData()->SetScalars(scalars);
  if (scalars)
    {
    scalars->Delete();
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkVolumeOutlineSource::GeneratePolys(
  vtkCellArray *polys,
  vtkUnsignedCharArray *scalars,
  unsigned char colors[2][3],
  int activePlane,
  int flags,
  int tolPtId[3][4])
{
  // Loop over the three dimensions and create the face rectangles
  for (int dim0 = 0; dim0 < 3; dim0++)
    {
    // Compute the other two dimension indices
    int dim1 = (dim0+1)%3;
    int dim2 = (dim0+2)%3;

    // Indices into the cubes
    int idx[3];

    // Loop over the "dim+2" dimension
    for (int i = 0; i < 4; i++)
      {
      idx[dim2] = i;

      // Loop over the "dim+1" dimension
      for (int j = 0; j < 3; j++)
        {
        idx[dim1] = j;

        // Make sure that the rect dim is not less than tolerance
        if ((j == 0 && tolPtId[dim1][1] == 0) ||
            (j == 2 && tolPtId[dim1][2] == 3))
          {
          continue;
          } 

        // Loop over rectangle along the "dim" dimension
        for (int k = 0; k < 3; k++)
          {
          idx[dim0] = k;

          // Make sure that the rect dim is not less than tolerance
          if ((k == 0 && tolPtId[dim0][1] == 0) ||
              (k == 2 && tolPtId[dim0][2] == 3))
            {
            continue;
            } 

          // The points in the rectangle, which are nudged over to the
          // volume bounds if the cropping planes are within tolerance
          // of the volume bounds.
          int pointId[4];
          pointId[0] = (tolPtId[2][idx[2]]*16 +
                        tolPtId[1][idx[1]]*4 +
                        tolPtId[0][idx[0]]);
          idx[dim0] = k + 1;
          pointId[1] = (tolPtId[2][idx[2]]*16 +
                        tolPtId[1][idx[1]]*4 +
                        tolPtId[0][idx[0]]);
          idx[dim1] = j + 1;
          pointId[2] = (tolPtId[2][idx[2]]*16 +
                        tolPtId[1][idx[1]]*4 +
                        tolPtId[0][idx[0]]);
          idx[dim0] = k;
          pointId[3] = (tolPtId[2][idx[2]]*16 +
                        tolPtId[1][idx[1]]*4 +
                        tolPtId[0][idx[0]]);
          idx[dim1] = j;

          // Loop through the two cubes adjacent to the rectangle,
          // in order to determine whether the rectangle is internal:
          // only external faces will be drawn.  The "bitCheck"
          // holds a bit for each of these two cubes.
          int bitCheck = 0;
          int cidx[3];
          cidx[dim0] = idx[dim0];
          cidx[dim1] = idx[dim1];
          for (int ii = 0; ii < 2; ii++)
            {
            // First get idx[dim2]-1, then idx[dim2]
            cidx[dim2] = idx[dim2] + ii - 1;
            int flagval = 0;
            if (cidx[dim2] >= 0 && cidx[dim2] < 3)
              {
              int flagbit = cidx[2]*9 + cidx[1]*3 + cidx[0];
              flagval = ((flags >> flagbit) & 1);
              }
            bitCheck <<= 1;
            bitCheck |= flagval;
            }

          // Whether we need to create a face depends on bitCheck.
          // Values 00, 11 don't need lines, while 01 and 10 do.

          // If our rect isn't an internal rect
          if (bitCheck != 0x0 && bitCheck != 0x3)
            {
            // Check if the rect is on our active plane
            int active = 0;
            if (activePlane >= 0)
              {
              int planeDim = (activePlane >> 1); // same as "/ 2"
              int planeIdx = 1 + (activePlane & 1); // same as "% 2"
              if (planeDim == dim2 && i == planeIdx)
                {
                active = 1;
                }
              }

            // Insert the rectangle with the correct sense
            polys->InsertNextCell(4);
            if (bitCheck == 0x2)
              {
              polys->InsertCellPoint(pointId[0]);
              polys->InsertCellPoint(pointId[1]);
              polys->InsertCellPoint(pointId[2]);
              polys->InsertCellPoint(pointId[3]);
              }
            else // (bitCheck == 0x1)
              {
              polys->InsertCellPoint(pointId[3]);
              polys->InsertCellPoint(pointId[2]);
              polys->InsertCellPoint(pointId[1]);
              polys->InsertCellPoint(pointId[0]);
              }

            // Color the face
            if (scalars)
              {
              scalars->InsertNextTupleValue(colors[active]);
              }
            }

          } // loop over k
        } // loop over j
      } // loop over i
    } // loop over dim0
}

//----------------------------------------------------------------------------
void vtkVolumeOutlineSource::GenerateLines(
  vtkCellArray *lines,
  vtkUnsignedCharArray *scalars,
  unsigned char colors[2][3],
  int activePlane,
  int flags,
  int tolPtId[3][4])
{
  // Loop over the three dimensions and create the lines
  for (int dim0 = 0; dim0 < 3; dim0++)
    {
    // Compute the other two dimension indices
    int dim1 = (dim0+1)%3;
    int dim2 = (dim0+2)%3;

    // Indices into the cubes
    int idx[3];

    // Loop over the "dim+2" dimension
    for (int i = 0; i < 4; i++)
      {
      idx[dim2] = i;

      // Loop over the "dim+1" dimension
      for (int j = 0; j < 4; j++)
        {
        idx[dim1] = j;

        // Loop over line segments along the "dim" dimension
        for (int k = 0; k < 3; k++)
          {
          idx[dim0] = k;

          // Make sure that the segment length is not less than tolerance
          if ((k == 0 && tolPtId[dim0][1] == 0) ||
              (k == 2 && tolPtId[dim0][2] == 3))
            {
            continue;
            } 

          // The endpoints of the segment, which are nudged over to the
          // volume bounds if the cropping planes are within tolerance
          // of the volume bounds.
          int pointId0 = (tolPtId[2][idx[2]]*16 +
                          tolPtId[1][idx[1]]*4 +
                          tolPtId[0][idx[0]]);
          idx[dim0] = k + 1;
          int pointId1 = (tolPtId[2][idx[2]]*16 +
                          tolPtId[1][idx[1]]*4 +
                          tolPtId[0][idx[0]]);
          idx[dim0] = k;

          // Loop through the four cubes adjacent to the line segment,
          // in order to determine whether the line segment is on an 
          // edge: only the edge lines will be drawn.  The "bitCheck"
          // holds a bit for each of these four cubes.
          int bitCheck = 0;
          int cidx[3];
          cidx[dim0] = idx[dim0];
          for (int ii = 0; ii < 2; ii++)
            {
            // First get idx[dim1]-1, then idx[dim1]
            cidx[dim1] = idx[dim1] + ii - 1;
            for (int jj = 0; jj < 2; jj++)
              {
              // First get idx[dim2]-1, then idx[dim2], but reverse
              // the order when ii loop is on its second iteration
              cidx[dim2] = idx[dim2] + (ii^jj) - 1;
              int flagval = 0;
              if (cidx[dim1] >= 0 && cidx[dim1] < 3 &&
                  cidx[dim2] >= 0 && cidx[dim2] < 3)
                {
                int flagbit = cidx[2]*9 + cidx[1]*3 + cidx[0];
                flagval = ((flags >> flagbit) & 1);
                }
              bitCheck <<= 1;
              bitCheck |= flagval;
              }
            }

          // Whether we need a line depends on the the value of bitCheck.
          // Values 0000, 0011, 0110, 1100, 1001, 1111 don't need lines. 
          // Build a bitfield to check our bitfield values against, each
          // set bit in this new bitfield corresponds to a non-edge case. 
          const int noLineValues = ((1 << 0x0) | (1 << 0x3) | (1 << 0x6) |
                                    (1 << 0x9) | (1 << 0xc) | (1 << 0xf)); 

          // If our line segment is an edge, there is lots of work to do.
          if (((noLineValues >> bitCheck) & 1) == 0)
            {
            // Check if the line segment is on our active plane
            int active = 0;
            if (activePlane >= 0)
              {
              int planeDim = (activePlane >> 1); // same as "/ 2"
              int planeIdx = 1 + (activePlane & 1); // same as "% 2"
              if ((planeDim == dim2 && i == planeIdx) ||
                  (planeDim == dim1 && j == planeIdx))
                {
                active = 1;
                }
              }

            // Check to make sure line segment isn't already there
            int foundDuplicate = 0;
            lines->InitTraversal();
            vtkIdType npts, *pts;
            for (int cellId = 0; lines->GetNextCell(npts, pts); cellId++)
              {
              if (pts[0] == pointId0 && pts[1] == pointId1)
                {
                // Change color if current segment is on active plane
                if (scalars && active)
                  {
                  scalars->SetTupleValue(cellId, colors[active]);
                  }
                foundDuplicate = 1;
                break;
                }
              }

            if (!foundDuplicate)
              {
              // Insert the line segment
              lines->InsertNextCell(2);
              lines->InsertCellPoint(pointId0);
              lines->InsertCellPoint(pointId1);

              // Color the line segment
              if (scalars)
                {
                scalars->InsertNextTupleValue(colors[active]);
                }
              } 
            }

          } // loop over k
        } // loop over j
      } // loop over i
    } // loop over dim0
}

//----------------------------------------------------------------------------
void vtkVolumeOutlineSource::GeneratePoints(
  vtkPoints *points, vtkCellArray *lines, vtkCellArray *polys,
  double planes[3][4], double tol)
{
  // Use a bitfield to store which of the 64 points we need.
  // Two 32-bit ints are a convenient, portable way to do this.
  unsigned int pointBits1 = 0;
  unsigned int pointBits2 = 0;

  vtkIdType npts, *pts;
  vtkCellArray *cellArrays[2];
  cellArrays[0] = lines;
  cellArrays[1] = polys;
  
  for (int arrayId = 0; arrayId < 2; arrayId++)
    {
    if (cellArrays[arrayId])
      {
      cellArrays[arrayId]->InitTraversal();
      while (cellArrays[arrayId]->GetNextCell(npts, pts))
        {
        for (int ii = 0; ii < npts; ii++)
          {
          int pointId = pts[ii];
          if (pointId < 32) { pointBits1 |= (1 << pointId); }
          else { pointBits2 |= (1 << (pointId - 32)); }
          }
        }
      }
    }

  // Create the array of up to 64 points, and use the pointBits bitfield
  // to find out which points were used.  It is also necessary to go through
  // and update the cells with the modified point ids.
  unsigned int pointBits = pointBits1;
  int ptId = 0;
  int newPtId = 0;

  for (int i = 0; i < 4; i++)
    {
    // If we're halfway done, switch over to the next 32 bits
    if (i == 2) { pointBits = pointBits2; }
 
    for (int j = 0; j < 4; j++)
      {
      for (int k = 0; k < 4; k++)
        {
        // Check to see if this point was actually used
        if ( (pointBits & 1) )
          {
          // Add or subtract tolerance as an offset to help depth check
          double x = planes[0][k] + tol*(1 - 2*(k < 2));
          double y = planes[1][j] + tol*(1 - 2*(j < 2));
          double z = planes[2][i] + tol*(1 - 2*(i < 2));

          points->InsertNextPoint(x, y, z);

          for (int arrayId = 0; arrayId < 2; arrayId++)
            {
            // Go through the cells, substitute old Id for new Id
            if (cellArrays[arrayId])
              {
              cellArrays[arrayId]->InitTraversal();
              while (cellArrays[arrayId]->GetNextCell(npts, pts))
                {
                for (int ii = 0; ii < npts; ii++)
                  {
                  if (pts[ii] == ptId) { pts[ii] = newPtId; } 
                  }
                }
              }
            }
          newPtId++;
          }
        pointBits >>= 1;
        ptId++;
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkVolumeOutlineSource::NudgeCropPlanesToBounds(
  int tolPtId[3][4], double planes[3][4], double tol)
{
  for (int dim = 0; dim < 3; dim++)
    {
    tolPtId[dim][0] = 0; tolPtId[dim][1] = 1;
    tolPtId[dim][2] = 2; tolPtId[dim][3] = 3;
    if (planes[dim][1] - planes[dim][0] < tol) { tolPtId[dim][1] = 0; }
    if (planes[dim][3] - planes[dim][2] < tol) { tolPtId[dim][2] = 3; }
    }
}

//----------------------------------------------------------------------------
void vtkVolumeOutlineSource::CreateColorValues(
  unsigned char colors[2][3], double color1[3], double color2[3])
{
  // Convert the two colors to unsigned char
  double *dcolors[2];
  dcolors[0] = color1;
  dcolors[1] = color2;

  for (int i = 0; i < 2; i++)
    {
    for (int j = 0; j < 3; j++)
      {
      double val = dcolors[i][j];
      if (val < 0) { val = 0; }
      if (val > 1) { val = 1; }
      colors[i][j] = static_cast<unsigned char>(val*255);
      }
    }
}


