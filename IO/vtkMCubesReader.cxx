/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMCubesReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <sys/types.h>
#include <sys/stat.h>
#include "vtkMCubesReader.h"
#include "vtkMergePoints.h"
#include "vtkByteSwap.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkMCubesReader* vtkMCubesReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMCubesReader");
  if(ret)
    {
    return (vtkMCubesReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMCubesReader;
}




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

void vtkMCubesReader::Execute()
{
  FILE *fp;
  FILE *limitp = NULL;
  vtkPoints *newPts;
  vtkCellArray *newPolys;
  vtkNormals *newNormals = NULL;
  float bounds[6];
  int i, j, k, numPts, numTris;
  typedef struct {float x[3], n[3];} pointType;
  pointType point;
  struct  stat buf;
  int numDegenerate=0;
  vtkIdType nodes[3];
  float direction, n[3], dummy[2];
  vtkPolyData *output = this->GetOutput();
  int byteOrder = this->GetDataByteOrder();
  
  vtkDebugMacro(<<"Reading marching cubes file");
  
  //
  // Initialize
  //

  if ( this->FileName == NULL )
    {
    vtkErrorMacro(<< "Please specify input FileName");
    return;
    }
  if ( (fp = fopen(this->FileName, "rb")) == NULL)
    {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    return;
    }

  // Try to read limits file to get bounds. Otherwise, read data.
  if ( this->LimitsFileName != NULL && 
  (limitp = fopen (this->LimitsFileName, "rb")) != NULL &&
  stat (this->FileName, &buf) == 0 )
    {
    // skip first three pairs
    fread (dummy, sizeof(float), 2, limitp);
    fread (dummy, sizeof(float), 2, limitp);
    fread (dummy, sizeof(float), 2, limitp);

    // next three pairs are x, y, z limits
    for (i = 0; i < 6; i++) 
      {
      fread (&bounds[i], sizeof (float), 1, limitp);
      }
    // do swapping if necc
    if (byteOrder == VTK_FILE_BYTE_ORDER_BIG_ENDIAN)
      {
      vtkByteSwap::Swap4BERange(bounds,6);
      }
    else
      {
      vtkByteSwap::Swap4LERange(bounds,6);
      }
    fclose (limitp);

    // calculate the number of triangles and vertices from file size
    numTris = buf.st_size / (18*sizeof(float)); //3 points + normals
    numPts = numTris * 3;	    
    }
  else // read data to get bounds
    {
    fseek (fp, this->HeaderSize, 0);
    bounds[0] = bounds[2] = bounds[4] = VTK_LARGE_FLOAT;
    bounds[1] = bounds[3] = bounds[5] = -VTK_LARGE_FLOAT;
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
    newNormals = vtkNormals::New();
    newNormals->Allocate(numPts/3,numPts/3);
    }
  
  if ( this->Locator == NULL )
    {
    this->CreateDefaultLocator();
    }
  this->Locator->InitPointInsertion (newPts, bounds);

  direction = this->FlipNormals ? -1.0 : 1.0;

  for ( i=0; i<numTris; i++) 
    {
    for (j=0; j<3; j++) 
      {
      int val;
      val = fread (&point, sizeof(pointType), 1, fp);
      if (val != 1)
         {
         vtkErrorMacro(<<"Error reading triange " << i << " (" << numTris << "), point/normal " << j);
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
      if ( this->Locator->InsertUniquePoint(point.x, nodes[j]) )
        {
        if ( this->Normals )
          {
          for (k=0; k<3; k++)
	    {
	    n[k] = point.n[k] * direction;
	    }
          newNormals->InsertNormal(nodes[j],n);
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
}

// Specify a spatial locator for merging points. By default, 
// an instance of vtkMergePoints is used.
void vtkMCubesReader::SetLocator(vtkPointLocator *locator)
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
  vtkPolyDataSource::PrintSelf(os,indent);

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
  unsigned long mTime=this-> vtkPolyDataSource::GetMTime();
  unsigned long time;

  if ( this->Locator != NULL )
    {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }
  return mTime;
}



