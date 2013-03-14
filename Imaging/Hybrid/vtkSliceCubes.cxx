/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSliceCubes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSliceCubes.h"

#include "vtkByteSwap.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkMarchingCubesTriangleCases.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkShortArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkVolumeReader.h"

vtkStandardNewMacro(vtkSliceCubes);

vtkCxxSetObjectMacro(vtkSliceCubes,Reader,vtkVolumeReader);

// Description:
// Construct with NULL reader, output FileName specification, and limits
// FileName.
vtkSliceCubes::vtkSliceCubes()
{
  this->Reader = NULL;
  this->FileName = NULL;
  this->LimitsFileName = NULL;
  this->Value = 0.0;
}

vtkSliceCubes::~vtkSliceCubes()
{
  if (this->FileName)
    {
    delete [] this->FileName;
    }
  if (this->LimitsFileName)
    {
    delete [] this->LimitsFileName;
    }
  this->SetReader(NULL);
}

// Description:
// Method causes object to read slices and generate isosurface.
void vtkSliceCubes::Update()
{
  this->Execute();
}

// Calculate the gradient using central difference.
// NOTE: We calculate the negative of the gradient for efficiency
template <class T>
void ComputePointGradient(int i, int j, int k, int dims[3],
                          double Spacing[3], double n[3], T *s0, T *s1, T *s2)
{
  double sp, sm;

  // x-direction
  if ( i == 0 )
    {
    sp = s1[i+1 + j*dims[0]];
    sm = s1[i + j*dims[0]];
    n[0] = (sm - sp) / Spacing[0];
    }
  else if ( i == (dims[0]-1) )
    {
    sp = s1[i + j*dims[0]];
    sm = s1[i-1 + j*dims[0]];
    n[0] = (sm - sp) / Spacing[0];
    }
  else
    {
    sp = s1[i+1 + j*dims[0]];
    sm = s1[i-1 + j*dims[0]];
    n[0] = 0.5 * (sm - sp) / Spacing[0];
    }

  // y-direction
  if ( j == 0 )
    {
    sp = s1[i + (j+1)*dims[0]];
    sm = s1[i + j*dims[0]];
    n[1] = (sm - sp) / Spacing[1];
    }
  else if ( j == (dims[1]-1) )
    {
    sp = s1[i + j*dims[0]];
    sm = s1[i + (j-1)*dims[0]];
    n[1] = (sm - sp) / Spacing[1];
    }
  else

    {
    sp = s1[i + (j+1)*dims[0]];
    sm = s1[i + (j-1)*dims[0]];
    n[1] = 0.5 * (sm - sp) / Spacing[1];
    }

  // z-direction
  // z-direction
  if ( k == 0 )
    {
    sp = s2[i + j*dims[0]];
    sm = s1[i + j*dims[0]];
    n[2] = (sm - sp) / Spacing[2];
    }
  else if ( k == (dims[2]-1) )
    {
    sp = s1[i + j*dims[0]];
    sm = s0[i + j*dims[0]];
    n[2] = (sm - sp) / Spacing[2];
    }
  else
    {
    sp = s2[i + j*dims[0]];
    sm = s0[i + j*dims[0]];
    n[2] = 0.5 * (sm - sp) / Spacing[2];
    }
}

template <class T, class S>
int vtkSliceCubesContour(T *slice, S *scalars, int imageRange[2], int dims[3],
                         double origin[3], double Spacing[3], double value,
                         double xmin[3], double xmax[3], FILE *outFP,
                         vtkVolumeReader *reader, unsigned char debug)
{
  S *slice0scalars=NULL, *slice1scalars;
  S *slice2scalars, *slice3scalars;
  T *slice0, *slice1, *slice2, *slice3;
  vtkImageData *sp;
  vtkDoubleArray *doubleScalars=NULL;
  int numTriangles=0, numComp = 0;
  double s[8];
  int i, j, k, idx, jOffset, ii, index, *vert, jj, sliceSize=0;
  static int CASE_MASK[8] = {1,2,4,8,16,32,64,128};
  vtkMarchingCubesTriangleCases *triCase, *triCases;
  EDGE_LIST  *edge;
  double pts[8][3], grad[8][3];
  double t, *x1, *x2, *n1, *n2;
  double xp, yp, zp;
  float point[6];
  static int edges[12][2] = { {0,1}, {1,2}, {3,2}, {0,3},
                              {4,5}, {5,6}, {7,6}, {4,7},
                              {0,4}, {1,5}, {3,7}, {2,6}};

  triCases =  vtkMarchingCubesTriangleCases::GetCases();

  if ( slice == NULL ) //have to do conversion to double slice-by-slice
    {
    sliceSize = dims[0] * dims[1];
    doubleScalars = vtkDoubleArray::New();
    doubleScalars->Allocate(sliceSize);
    }

  slice1scalars = NULL;
  slice2scalars = scalars;
  slice2scalars->Register(NULL);

  if (debug)  vtkGenericWarningMacro(<< "  Slice# " << imageRange[0]);

  if ( slice2scalars == NULL )
    {
    return 0;
    }
  if ( slice != NULL )
    {
    slice1 = slice2 = slice2scalars->GetPointer(0);
    }
  else
    {//get as double
    numComp = scalars->GetNumberOfComponents();
    slice2scalars->GetData(0,sliceSize-1,0,numComp-1,doubleScalars);
    slice1 = slice2 = (T *) doubleScalars->GetPointer(0);
    }

  sp = reader->GetImage(imageRange[0]+1);
  slice3scalars = (S *) sp->GetPointData()->GetScalars();
  slice3scalars->Register(NULL);
  sp->Delete();

  if (debug)  vtkGenericWarningMacro(<< "  Slice# " << imageRange[0]+1 );

  if ( slice != NULL )
    {
    slice3 = slice3scalars->GetPointer(0);
    }
  else
    {//get as double: cast is ok because this code is only executed for double type
    slice3scalars->GetData(0,sliceSize-1,0,numComp-1,doubleScalars);
    slice3 = (T *) doubleScalars->GetPointer(0);
    }

  if ( !slice2 || !slice3 )
    {
    vtkGenericWarningMacro(<< "Cannot allocate data!");
    return 0;
    }

  // Generate triangles and normals from slices
  for (k=0; k < (dims[2]-1); k++)
    {

    // swap things around
    if ( slice0scalars != NULL )
      {
      slice0scalars->Delete();
      }
    slice0scalars = slice1scalars;
    slice0 = slice1;
    slice1scalars = slice2scalars;
    slice1 = slice2;
    slice2scalars = slice3scalars;
    slice2 = slice3;
    if ( k < (dims[2]-2) )
      {
      if (debug)  vtkGenericWarningMacro(<< "  Slice# " << imageRange[0]+k+2);
      sp = reader->GetImage(imageRange[0]+k+2);
      slice3scalars = (S *) sp->GetPointData()->GetScalars();
      if ( slice3scalars == NULL )
        {
        vtkGenericWarningMacro(<< "Can't read all the requested slices");
        goto PREMATURE_TERMINATION;
        }
      slice3scalars->Register(NULL);
      sp->Delete();
      if ( slice != NULL )
        {
        slice3 = slice3scalars->GetPointer(0);
        }
      else
        {//get as double
        slice3scalars->GetData(0,sliceSize-1,0,numComp-1,doubleScalars);
        slice3 = (T *) doubleScalars->GetPointer(0);
        }
      }

    pts[0][2] = origin[2] + k*Spacing[2];
    zp = origin[2] + (k+1)*Spacing[2];
    for ( j=0; j < (dims[1]-1); j++)
      {
      jOffset = j*dims[0];
      pts[0][1] = origin[1] + j*Spacing[1];
      yp = origin[1] + (j+1)*Spacing[1];
      for ( i=0; i < (dims[0]-1); i++)
        {
        //get scalar values
        idx = i + jOffset;
        s[0] = slice1[idx];
        s[1] = slice1[idx+1];
        s[2] = slice1[idx+1 + dims[0]];
        s[3] = slice1[idx + dims[0]];
        s[4] = slice2[idx];
        s[5] = slice2[idx+1];
        s[6] = slice2[idx+1 + dims[0]];
        s[7] = slice2[idx + dims[0]];

        // Build the case table
        for ( ii=0, index = 0; ii < 8; ii++)
          {
          if ( s[ii] >= value )
            {
            index |= CASE_MASK[ii];
            }
          }

        if ( index == 0 || index == 255 ) // no surface
          {
          continue;
          }
        //create voxel points
        pts[0][0] = origin[0] + i*Spacing[0];
        xp = origin[0] + (i+1)*Spacing[0];

        pts[1][0] = xp;
        pts[1][1] = pts[0][1];
        pts[1][2] = pts[0][2];

        pts[2][0] = xp;
        pts[2][1] = yp;
        pts[2][2] = pts[0][2];

        pts[3][0] = pts[0][0];
        pts[3][1] = yp;
        pts[3][2] = pts[0][2];

        pts[4][0] = pts[0][0];
        pts[4][1] = pts[0][1];
        pts[4][2] = zp;

        pts[5][0] = xp;
        pts[5][1] = pts[0][1];
        pts[5][2] = zp;

        pts[6][0] = xp;
        pts[6][1] = yp;
        pts[6][2] = zp;

        pts[7][0] = pts[0][0];
        pts[7][1] = yp;
        pts[7][2] = zp;

        //create gradients
        ComputePointGradient(i,j, k, dims, Spacing, grad[0],
                             slice0, slice1, slice2);
        ComputePointGradient(i+1,j, k, dims, Spacing, grad[1],
                             slice0, slice1, slice2);
        ComputePointGradient(i+1,j+1, k, dims, Spacing, grad[2],
                             slice0, slice1, slice2);
        ComputePointGradient(i,j+1, k, dims, Spacing, grad[3],
                             slice0, slice1, slice2);
        ComputePointGradient(i,j, k+1, dims, Spacing, grad[4],
                             slice1, slice2, slice3);
        ComputePointGradient(i+1,j, k+1, dims, Spacing, grad[5],
                             slice1, slice2, slice3);
        ComputePointGradient(i+1,j+1, k+1, dims, Spacing, grad[6],
                             slice1, slice2, slice3);
        ComputePointGradient(i,j+1, k+1, dims, Spacing, grad[7],
                             slice1, slice2, slice3);

        triCase = triCases + index;
        edge = triCase->edges;

        for ( ; edge[0] > -1; edge += 3 )
          {
          for (ii=0; ii<3; ii++) //insert triangle
            {
            vert = edges[edge[ii]];
            t = (value - s[vert[0]]) / (s[vert[1]] - s[vert[0]]);
            x1 = pts[vert[0]];
            x2 = pts[vert[1]];
            n1 = grad[vert[0]];
            n2 = grad[vert[1]];
            for (jj=0; jj<3; jj++)
              {
              point[jj] = x1[jj] + t * (x2[jj] - x1[jj]);
              point[jj+3] = n1[jj] + t * (n2[jj] - n1[jj]);
              if (point[jj] < xmin[jj] )
                {
                xmin[jj] = point[jj];
                }
              if (point[jj] > xmax[jj] )
                {
                xmax[jj] = point[jj];
                }
              }
            vtkMath::Normalize(point+3);
            // swap bytes if necessary
            bool status=vtkByteSwap::SwapWrite4BERange(point,6,outFP);
            if(!status)
              {
              vtkGenericWarningMacro(<< "SwapWrite failed!");
              }
            }
          numTriangles++;
          }//for each triangle
        }//for i
      }//for j
    }//for k

  // Close things down
  PREMATURE_TERMINATION:

  fclose(outFP);
  if ( slice == NULL )
    {
    doubleScalars->Delete();
    }
  if (slice0scalars && slice0scalars != slice1scalars)
    {
    slice0scalars->Delete();
    }
  if (slice3scalars && slice3scalars != slice2scalars)
    {
    slice3scalars->Delete();
    }
  slice1scalars->Delete();
  slice2scalars->Delete();

  return numTriangles;
}

void vtkSliceCubes::Execute()
{
  FILE *outFP;
  vtkImageData *tempStructPts;
  vtkDataArray *inScalars;
  int dims[3], imageRange[2];
  double xmin[3], xmax[3];
  double origin[3], Spacing[3];

  // check input/initalize
  vtkDebugMacro(<< "Executing slice cubes");
  if ( this->Reader == NULL )
   {
   vtkErrorMacro(<<"No reader specified...can't generate isosurface");
   return;
   }

  if ( this->FileName == NULL )
   {
   vtkErrorMacro(<<"No FileName specified...can't output isosurface");
   return;
   }

  if ( (outFP = fopen(this->FileName, "wb")) == NULL )
   {
   vtkErrorMacro(<<"Cannot open specified output file...");
   return;
   }

  // get image dimensions from the readers first slice
  this->Reader->GetImageRange(imageRange);
  tempStructPts = this->Reader->GetImage(imageRange[0]);
  tempStructPts->GetDimensions(dims);
  tempStructPts->GetOrigin(origin);
  tempStructPts->GetSpacing(Spacing);

  dims[2] = (imageRange[1] - imageRange[0] + 1);

  if ( (dims[0]*dims[1]*dims[2]) <= 1 || dims[2] < 2 )
    {
    vtkErrorMacro(<<"Bad dimensions...slice must be 3D volume");
    fclose(outFP);
    return;
    }

  xmin[0]=xmin[1]=xmin[2] = VTK_DOUBLE_MAX;
  xmax[0]=xmax[1]=xmax[2] = -VTK_DOUBLE_MAX;

  inScalars = tempStructPts->GetPointData()->GetScalars();
  if ( inScalars == NULL )
    {
    vtkErrorMacro(<<"Must have scalars to generate isosurface");
    tempStructPts->Delete();
    fclose(outFP);
    return;
    }
  inScalars->Register(this);
  tempStructPts->Delete();

  if (inScalars->GetNumberOfComponents() == 1 )
    {
    switch (inScalars->GetDataType())
      {
      case VTK_CHAR:
        {
        vtkCharArray *scalars = (vtkCharArray *)inScalars;
        char *s = scalars->GetPointer(0);
        vtkSliceCubesContour(s,scalars,imageRange,dims,origin,
                             Spacing,this->Value,
                             xmin,xmax,outFP,this->Reader,this->Debug);
        }
      break;
      case VTK_UNSIGNED_CHAR:
        {
        vtkUnsignedCharArray *scalars = (vtkUnsignedCharArray *)inScalars;
        unsigned char *s = scalars->GetPointer(0);
        vtkSliceCubesContour(s,scalars,imageRange,dims,origin,
                             Spacing,this->Value,
                             xmin,xmax,outFP,this->Reader,this->Debug);
        }
      break;
      case VTK_SHORT:
        {
        vtkShortArray *scalars = (vtkShortArray *)inScalars;
        short *s = scalars->GetPointer(0);
        vtkSliceCubesContour(s,scalars,imageRange,dims,origin,
                             Spacing,this->Value,
                             xmin,xmax,outFP,this->Reader,this->Debug);
        }
      break;
      case VTK_UNSIGNED_SHORT:
        {
        vtkUnsignedShortArray *scalars = (vtkUnsignedShortArray *)inScalars;
        unsigned short *s = scalars->GetPointer(0);
        vtkSliceCubesContour(s,scalars,imageRange,dims,origin,
                             Spacing,this->Value,
                             xmin,xmax,outFP,this->Reader,this->Debug);
        }
      break;
      case VTK_INT:
        {
        vtkIntArray *scalars = (vtkIntArray *)inScalars;
        int *s = scalars->GetPointer(0);
        vtkSliceCubesContour(s,scalars,imageRange,dims,origin,
                             Spacing,this->Value,
                             xmin,xmax,outFP,this->Reader,this->Debug);
        }
      break;
      case VTK_UNSIGNED_INT:
        {
        vtkUnsignedIntArray *scalars = (vtkUnsignedIntArray *)inScalars;
        unsigned int *s = scalars->GetPointer(0);
        vtkSliceCubesContour(s,scalars,imageRange,dims,origin,
                             Spacing,this->Value,
                             xmin,xmax,outFP,this->Reader,this->Debug);
        }
      break;
      case VTK_LONG:
        {
        vtkLongArray *scalars = (vtkLongArray *)inScalars;
        long *s = scalars->GetPointer(0);
        vtkSliceCubesContour(s,scalars,imageRange,dims,origin,
                             Spacing,this->Value,
                             xmin,xmax,outFP,this->Reader,this->Debug);
        }
      break;
      case VTK_UNSIGNED_LONG:
        {
        vtkUnsignedLongArray *scalars = (vtkUnsignedLongArray *)inScalars;
        unsigned long *s = scalars->GetPointer(0);
        vtkSliceCubesContour(s,scalars,imageRange,dims,origin,
                             Spacing,this->Value,
                             xmin,xmax,outFP,this->Reader,this->Debug);
        }
      break;
      case VTK_FLOAT:
        {
        vtkFloatArray *scalars = (vtkFloatArray *)inScalars;
        float *s = scalars->GetPointer(0);
        vtkSliceCubesContour(s,scalars,imageRange,dims,origin,
                             Spacing,this->Value,
                             xmin,xmax,outFP,this->Reader,this->Debug);
        }
      break;
      case VTK_DOUBLE:
        {
        vtkDoubleArray *scalars = (vtkDoubleArray *)inScalars;
        double *s = scalars->GetPointer(0);
        vtkSliceCubesContour(s,scalars,imageRange,dims,origin,
                             Spacing,this->Value,
                             xmin,xmax,outFP,this->Reader,this->Debug);
        }
      break;
      }//switch
    }

  else //multiple components have to convert
    {
    vtkDoubleArray *scalars = (vtkDoubleArray *)inScalars;
    double *s = NULL; //clue to convert data to double
    vtkSliceCubesContour(s,scalars,imageRange,dims,origin,
                         Spacing,this->Value,
                         xmin,xmax,outFP,this->Reader,this->Debug);
    }

  inScalars->UnRegister(this);

  if ( this->LimitsFileName )
    {
    int i;
    float t;

    if ( (outFP = fopen(this->LimitsFileName, "wb")) == NULL )
      {
      vtkWarningMacro(<<"Sorry, couldn't write limits file...");
      }
    else
      {
      bool status=true;
      float forigin[3];
      for (i=0; i<3 && status; i++)
        {
        t = origin[i] + (dims[i] - 1)*Spacing[i];
        forigin[i] = (float)origin[i];
        status=vtkByteSwap::SwapWrite4BERange(forigin+i,1,outFP);
        if(!status)
          {
          vtkWarningMacro(<<"SwapWrite failed.");
          }
        // swap if necessary
        if(status)
          {
          status=vtkByteSwap::SwapWrite4BERange(&t,1,outFP);
          if(!status)
            {
            vtkWarningMacro(<<"SwapWrite failed.");
            }
          }
        }
      float ftmp;
      for (i=0; i<3 && status; i++)
        {
        ftmp = (float)xmin[i];
        status=vtkByteSwap::SwapWrite4BERange(&ftmp,1,outFP);
        if(!status)
          {
          vtkWarningMacro(<<"SwapWrite failed.");
          }
        ftmp = (float)xmax[i];
        if(status)
          {
          status=vtkByteSwap::SwapWrite4BERange(&ftmp,1,outFP);
          if(!status)
            {
            vtkWarningMacro(<<"SwapWrite failed.");
            }
          }
        }
      }
     fclose(outFP);
    }
}

void vtkSliceCubes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Iso Value: " << this->Value << "\n";

  if ( this->Reader )
    {
    os << indent << "Reader:\n";
    this->Reader->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Reader: (none)\n";
    }

  os << indent << "File Name: "
     << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "Limits File Name: "
     << (this->LimitsFileName ? this->LimitsFileName : "(none)") << "\n";
}
