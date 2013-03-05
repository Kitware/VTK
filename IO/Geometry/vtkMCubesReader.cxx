/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMCubesReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMCubesReader.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkMergePoints.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkIncrementalPointLocator.h"

#include <sys/types.h>
#include <sys/stat.h>

vtkStandardNewMacro(vtkMCubesReader);

// Construct object with FlipNormals turned off and Normals set to true.
vtkMCubesReader::vtkMCubesReader()
{
  this->FileName = NULL;
  this->LimitsFileName = NULL;

  this->Locator = NULL;

#ifndef VTK_WORDS_BIGENDIAN
  this->SwapBytes = 1;
#else
  this->SwapBytes = 0;
#endif
  this->HeaderSize = 0;
  this->FlipNormals = 0;
  this->Normals = 1;

  this->SetNumberOfInputPorts(0);
}

vtkMCubesReader::~vtkMCubesReader()
{
  if (this->FileName)
    {
    delete [] this->FileName;
    }
  if (this->LimitsFileName)
    {
    delete [] this->LimitsFileName;
    }
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
}

int vtkMCubesReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  FILE *fp;
  FILE *limitp;
  vtkPoints *newPts;
  vtkCellArray *newPolys;
  vtkFloatArray *newNormals = NULL;
  double bounds[6];
  int i, j, k, numPts, numTris;
  typedef struct {float x[3], n[3];} pointType;
  pointType point;
  struct  stat buf;
  int numDegenerate=0;
  vtkIdType nodes[3];
  float direction, n[3], dummy[2];
  int byteOrder = this->GetDataByteOrder();

  vtkDebugMacro(<<"Reading marching cubes file");

  //
  // Initialize
  //

  if ( this->FileName == NULL )
    {
    vtkErrorMacro(<< "Please specify input FileName");
    return 0;
    }
  if ( (fp = fopen(this->FileName, "rb")) == NULL)
    {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    return 0;
    }

  // Try to read limits file to get bounds. Otherwise, read data.
  if ( this->LimitsFileName != NULL &&
  (limitp = fopen (this->LimitsFileName, "rb")) != NULL &&
  stat (this->FileName, &buf) == 0 )
    {
    bool errorOccurred = false;

    // skip first three pairs
    float fbounds[6];
    if (fread (dummy, sizeof(float), 2, limitp) != 2)
      {
      errorOccurred = true;
      }
    else if (fread (dummy, sizeof(float), 2, limitp) != 2)
      {
      errorOccurred = true;
      }
    if (fread (dummy, sizeof(float), 2, limitp) != 2)
      {
      errorOccurred = true;
      }

    // next three pairs are x, y, z limits
    for (i = 0; i < 6 && !errorOccurred; i++)
      {
      if (fread (&fbounds[i], sizeof (float), 1, limitp) != 1)
        {
        errorOccurred = true;
        }
      }

    if (errorOccurred)
      {
      vtkErrorMacro ("MCubesReader error reading file: " << this->LimitsFileName
                     << " Premature EOF while reading limits.");
      fclose (limitp);
      fclose (fp);
      return 0;
      }

    // do swapping if necc
    if (byteOrder == VTK_FILE_BYTE_ORDER_BIG_ENDIAN)
      {
      vtkByteSwap::Swap4BERange(fbounds,6);
      }
    else
      {
      vtkByteSwap::Swap4LERange(fbounds,6);
      }
    fclose (limitp);
    bounds[0] = fbounds[0];
    bounds[1] = fbounds[1];
    bounds[2] = fbounds[2];
    bounds[3] = fbounds[3];
    bounds[4] = fbounds[4];
    bounds[5] = fbounds[5];

    // calculate the number of triangles and vertices from file size
    numTris = buf.st_size / (18*sizeof(float)); //3 points + normals
    numPts = numTris * 3;
    }
  else // read data to get bounds
    {
    fseek (fp, this->HeaderSize, 0);
    // cannot use vtkMath uninitialze bounds for this computation
    bounds[0] = bounds[2] = bounds[4] = VTK_DOUBLE_MAX;
    bounds[1] = bounds[3] = bounds[5] = VTK_DOUBLE_MIN;
    for (i=0; fread(&point, sizeof(pointType), 1, fp); i++)
      {
      // swap bytes if necc
      if (byteOrder == VTK_FILE_BYTE_ORDER_BIG_ENDIAN)
        {
        vtkByteSwap::Swap4BERange((float *) (&point),6);
        }
      else
        {
        vtkByteSwap::Swap4LERange((float *) (&point),6);
        }
      for (j=0; j<3; j++)
        {
        bounds[2*j] = (bounds[2*j] < point.x[j] ? bounds[2*j] : point.x[j]);
        bounds[2*j+1] = (bounds[2*j+1] > point.x[j] ? bounds[2*j+1] : point.x[j]);
        }

      if ( i && ((i % 10000) == 0) )
        {
        vtkDebugMacro(<<"Triangle vertices #" << i);
        }
      }
    numTris = i / 3;
    numPts = i;
    }
//
// Now re-read and merge
//
  rewind (fp);
  fseek (fp, this->HeaderSize, 0);

  newPts = vtkPoints::New();
  newPts->Allocate(numPts/3,numPts/3);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(numTris,3));

  if ( this->Normals )
    {
    newNormals = vtkFloatArray::New();
    newNormals->SetNumberOfComponents(3);
    newNormals->Allocate(numPts,numPts);
    }

  if ( this->Locator == NULL )
    {
    this->CreateDefaultLocator();
    }
  this->Locator->InitPointInsertion (newPts, bounds);

  direction = this->FlipNormals ? -1.0 : 1.0;

  double dp[3];
  for ( i=0; i<numTris; i++)
    {
    for (j=0; j<3; j++)
      {
      int val;
      val = static_cast<int>(
        fread (&point, static_cast<int>(sizeof(pointType)), 1, fp));
      if (val != 1)
         {
         vtkErrorMacro(<<"Error reading triange " << i
                       << " (" << numTris << "), point/normal " << j);
         }

      // swap bytes if necc
      if (byteOrder == VTK_FILE_BYTE_ORDER_BIG_ENDIAN)
        {
        vtkByteSwap::Swap4BERange((float *) (&point),6);
        }
      else
        {
        vtkByteSwap::Swap4LERange((float *) (&point),6);
        }
      dp[0] = point.x[0];
      dp[1] = point.x[1];
      dp[2] = point.x[2];
      if ( this->Locator->InsertUniquePoint(dp, nodes[j]) )
        {
        if ( this->Normals )
          {
          for (k=0; k<3; k++)
            {
            n[k] = point.n[k] * direction;
            }
          newNormals->InsertTuple(nodes[j],n);
          }
        }
      }
    if ( nodes[0] != nodes[1] &&
         nodes[0] != nodes[2] &&
         nodes[1] != nodes[2] )
      {
      newPolys->InsertNextCell(3,nodes);
      }
    else
      {
      numDegenerate++;
      }
    }
  vtkDebugMacro(<< "Read: "
                << newPts->GetNumberOfPoints() << " points, "
                << newPolys->GetNumberOfCells() << " triangles\n"
                << "(Removed " << numDegenerate << " degenerate triangles)");

  fclose(fp);
//
// Update ourselves
//
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();

  if (this->Normals)
    {
    output->GetPointData()->SetNormals(newNormals);
    newNormals->Delete();
    }
  output->Squeeze(); // might have merged stuff

  if (this->Locator)
    {
    this->Locator->Initialize(); //free storage
    }

  return 1;
}

// Specify a spatial locator for merging points. By default,
// an instance of vtkMergePoints is used.
void vtkMCubesReader::SetLocator(vtkIncrementalPointLocator *locator)
{
  if ( this->Locator == locator )
    {
    return;
    }
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
  if ( locator )
    {
    locator->Register(this);
    }
  this->Locator = locator;
  this->Modified();
}

void vtkMCubesReader::SetDataByteOrderToBigEndian()
{
#ifndef VTK_WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}

void vtkMCubesReader::SetDataByteOrderToLittleEndian()
{
#ifdef VTK_WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}

void vtkMCubesReader::SetDataByteOrder(int byteOrder)
{
  if ( byteOrder == VTK_FILE_BYTE_ORDER_BIG_ENDIAN )
    {
    this->SetDataByteOrderToBigEndian();
    }
  else
    {
    this->SetDataByteOrderToLittleEndian();
    }
}

int vtkMCubesReader::GetDataByteOrder()
{
#ifdef VTK_WORDS_BIGENDIAN
  if ( this->SwapBytes )
    {
    return VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
    }
  else
    {
    return VTK_FILE_BYTE_ORDER_BIG_ENDIAN;
    }
#else
  if ( this->SwapBytes )
    {
    return VTK_FILE_BYTE_ORDER_BIG_ENDIAN;
    }
  else
    {
    return VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
    }
#endif
}

const char *vtkMCubesReader::GetDataByteOrderAsString()
{
#ifdef VTK_WORDS_BIGENDIAN
  if ( this->SwapBytes )
    {
    return "LittleEndian";
    }
  else
    {
    return "BigEndian";
    }
#else
  if ( this->SwapBytes )
    {
    return "BigEndian";
    }
  else
    {
    return "LittleEndian";
    }
#endif
}

void vtkMCubesReader::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
    {
    this->Locator = vtkMergePoints::New();
    }
}

void vtkMCubesReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "File Name: "
     << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "Limits File Name: "
     << (this->LimitsFileName ? this->LimitsFileName : "(none)") << "\n";
  os << indent << "Normals: " << (this->Normals ? "On\n" : "Off\n");
  os << indent << "FlipNormals: " << (this->FlipNormals ? "On\n" : "Off\n");
  os << indent << "HeaderSize: " << this->HeaderSize << "\n";
  os << indent << "Swap Bytes: " << (this->SwapBytes ? "On\n" : "Off\n");

  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }
}

unsigned long int vtkMCubesReader::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long time;

  if ( this->Locator != NULL )
    {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }
  return mTime;
}



