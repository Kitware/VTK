/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSTLReader.cxx
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
#include <ctype.h>
#include <string.h>
#include "vtkSTLReader.h"
#include "vtkByteSwap.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"

//---------------------------------------------------------------------------
vtkSTLReader* vtkSTLReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSTLReader");
  if(ret)
    {
    return (vtkSTLReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkSTLReader;
}

#define VTK_ASCII 0
#define VTK_BINARY 1

// Construct object with merging set to true.
vtkSTLReader::vtkSTLReader()
{
  this->FileName = NULL;
  this->Merging = 1;
  this->ScalarTags = 0;
  this->Locator = NULL;
}

vtkSTLReader::~vtkSTLReader()
{
  if (this->FileName)
    {
    delete [] this->FileName;
    }
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
}

// Overload standard modified time function. If locator is modified,
// then this object is modified as well.
unsigned long vtkSTLReader::GetMTime()
{
  unsigned long mTime1=this->vtkPolyDataSource::GetMTime();
  unsigned long mTime2;
  
  if (this->Locator)
    {
    mTime2 = this->Locator->GetMTime();
    mTime1 = ( mTime1 > mTime2 ? mTime1 : mTime2 );
    }

  return mTime1;
}


void vtkSTLReader::Execute()
{
  FILE *fp;
  vtkPoints *newPts, *mergedPts;
  vtkCellArray *newPolys, *mergedPolys;
  vtkScalars *newScalars=0, *mergedScalars=0;
  vtkPolyData *output = this->GetOutput();
  
  // All of the data in the first piece.
  if (output->GetUpdatePiece() > 0)
    {
    return;
    }
  
  if (!this->FileName)
    {
    vtkErrorMacro(<<"A FileName must be specified.");
    return;
    }

  // Initialize
  //
  if ((fp = fopen(this->FileName, "r")) == NULL)
    {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    return;
    }

  newPts = vtkPoints::New();
  newPts->Allocate(5000,10000);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(10000,20000);

  // Depending upon file type, read differently
  //
  if ( this->GetSTLFileType(fp) == VTK_ASCII )
    {
	if (ScalarTags) 
	{
      newScalars = vtkScalars::New();
	  newScalars->Allocate(5000,10000);
	}
	if ( this->ReadASCIISTL(fp,newPts,newPolys,newScalars) )
      {
      return;
      }
    }
  else
    {
    fclose(fp);
    fp = fopen(this->FileName, "rb");
    if ( this->ReadBinarySTL(fp,newPts,newPolys) )
      {
      return;
      }
    }

  vtkDebugMacro(<< "Read: " 
               << newPts->GetNumberOfPoints() << " points, "
               << newPolys->GetNumberOfCells() << " triangles");

  fclose(fp);
  //
  // If merging is on, create hash table and merge points/triangles.
  //
//  if (0)
  if ( this->Merging )  
  {
    int npts, i;
    vtkIdType *pts, nodes[3];
    float *x;
    int nextCell=0;

    mergedPts = vtkPoints::New();
    mergedPts->Allocate(newPts->GetNumberOfPoints()/2);
    mergedPolys = vtkCellArray::New();
    mergedPolys->Allocate(newPolys->GetSize());
	if (newScalars) {
		mergedScalars = vtkScalars::New();
		mergedScalars->Allocate(newPolys->GetSize());
	}

    if ( this->Locator == NULL )
      {
      this->CreateDefaultLocator();
      }
    this->Locator->InitPointInsertion (mergedPts, newPts->GetBounds());

    for (newPolys->InitTraversal(); newPolys->GetNextCell(npts,pts); )
      {
      for (i=0; i < 3; i++) 
        {
        x = newPts->GetPoint(pts[i]);
		this->Locator->InsertUniquePoint(x, nodes[i]);
        }

      if ( nodes[0] != nodes[1] &&
           nodes[0] != nodes[2] && 
           nodes[1] != nodes[2] )
        {
        mergedPolys->InsertNextCell(3,nodes);
		if (newScalars) 
          {
	      mergedScalars->InsertNextScalar(newScalars->GetScalar(nextCell));	
		  }
        }
        nextCell++;
      }

      newPts->Delete();
      newPolys->Delete();
	  if (newScalars) {
		  newScalars->Delete();
	  }

      vtkDebugMacro(<< "Merged to: " 
                   << mergedPts->GetNumberOfPoints() << " points, " 
                   << mergedPolys->GetNumberOfCells() << " triangles");
	}
  else
    {
    mergedPts = newPts;
    mergedPolys = newPolys;
    mergedScalars = newScalars;
	}
//
// Update ourselves
//
  output->SetPoints(mergedPts);
  mergedPts->Delete();

  output->SetPolys(mergedPolys);
  mergedPolys->Delete();

  if (mergedScalars) {
	  output->GetCellData()->SetScalars(mergedScalars);
	  mergedScalars->Delete();
  }

  if (this->Locator)
    {
    this->Locator->Initialize(); //free storage
    }

  output->Squeeze();
}

int vtkSTLReader::ReadBinarySTL(FILE *fp, vtkPoints *newPts, vtkCellArray *newPolys)
{
  int i, numTris;
  vtkIdType pts[3];
  unsigned long   ulint;
  unsigned short  ibuff2;
  char    header[81];
  typedef struct  {float  n[3], v1[3], v2[3], v3[3];} facet_t;
  facet_t facet;

  vtkDebugMacro(<< " Reading BINARY STL file");

  //  File is read to obtain raw information as well as bounding box
  //
  fread (header, 1, 80, fp);
  fread (&ulint, 1, 4, fp);
  vtkByteSwap::Swap4LE(&ulint);

  // Many .stl files contain bogus count.  Hence we will ignore and read 
  //   until end of file.
  //
  if ( (numTris = (int) ulint) <= 0 )
    {
    vtkDebugMacro(<< "Bad binary count: attempting to correct (" 
                  << numTris << ")");
    }

  for ( i=0; fread(&facet,48,1,fp) > 0; i++ )
    {
    fread(&ibuff2,2,1,fp); //read extra junk

    vtkByteSwap::Swap4LE (facet.n);
    vtkByteSwap::Swap4LE (facet.n+1);
    vtkByteSwap::Swap4LE (facet.n+2);

    vtkByteSwap::Swap4LE (facet.v1);
    vtkByteSwap::Swap4LE (facet.v1+1);
    vtkByteSwap::Swap4LE (facet.v1+2);
    pts[0] = newPts->InsertNextPoint(facet.v1);

    vtkByteSwap::Swap4LE (facet.v2);
    vtkByteSwap::Swap4LE (facet.v2+1);
    vtkByteSwap::Swap4LE (facet.v2+2);
    pts[1] = newPts->InsertNextPoint(facet.v2);

    vtkByteSwap::Swap4LE (facet.v3);
    vtkByteSwap::Swap4LE (facet.v3+1);
    vtkByteSwap::Swap4LE (facet.v3+2);
    pts[2] = newPts->InsertNextPoint(facet.v3);

    newPolys->InsertNextCell(3,pts);

    if ( (i % 5000) == 0 && i != 0 )
      {
      vtkDebugMacro(<< "triangle# " << i);
      this->UpdateProgress((i%50000)/50000.0);
      }
    }

  return 0;
}

int vtkSTLReader::ReadASCIISTL(FILE *fp, vtkPoints *newPts, vtkCellArray *newPolys, vtkScalars *scalars)
{
  char line[256];
  float x[3];
  vtkIdType pts[3];
  int done;
  int currentSolid = 0;

  vtkDebugMacro(<< " Reading ASCII STL file");

  //  Ingest header and junk to get to first vertex
  //
  fgets (line, 255, fp);

  done = (fscanf(fp,"%s %*s %f %f %f\n", line, x, x+1, x+2)==EOF);

  //  Go into loop, reading  facet normal and vertices
  //
//  while (fscanf(fp,"%*s %*s %f %f %f\n", x, x+1, x+2)!=EOF) 
  while (!done)
	{
//if (ctr>=253840) { 
//    fprintf(stdout, "Reading record %d\n", ctr);
//}
//ctr += 7;
    fgets (line, 255, fp);
    fscanf (fp, "%*s %f %f %f\n", x,x+1,x+2);
    pts[0] = newPts->InsertNextPoint(x);
	fscanf (fp, "%*s %f %f %f\n", x,x+1,x+2);
    pts[1] = newPts->InsertNextPoint(x);
	fscanf (fp, "%*s %f %f %f\n", x,x+1,x+2);
    pts[2] = newPts->InsertNextPoint(x);
	fgets (line, 255, fp); // end loop
    fgets (line, 255, fp); // end facet

    newPolys->InsertNextCell(3,pts);
	if (scalars) 
	{
		scalars->InsertNextScalar(currentSolid);
	}

	if ( (newPolys->GetNumberOfCells() % 5000) == 0 )
      {
      vtkDebugMacro(<< "triangle# " << newPolys->GetNumberOfCells());
      this->UpdateProgress((newPolys->GetNumberOfCells()%50000)/50000.0);
      }
	done = (fscanf(fp,"%s", line)==EOF);
	if (strcmp(line, "ENDSOLID") == 0) 
	{
		currentSolid++;
		fgets(line, 255, fp);
		done = feof(fp);
		while (strncmp(line, "SOLID", 5) && !done) 
		{
			fgets(line, 255, fp);
			done = feof(fp);
		}

		done = (fscanf(fp,"%s", line)==EOF);
	}
	if (!done) {
		done = (fscanf(fp,"%*s %f %f %f\n", x, x+1, x+2)==EOF);
  	}
  }
  //fprintf(stdout, "Maximum ctr val %d\n", ctr);
  return 0;
}

int vtkSTLReader::GetSTLFileType(FILE *fp)
{
  unsigned char header[256];
  int type, i;
  int numChars;

  //  Read a little from the file to figure what type it is.
  //
  // skip 255 characters so we are past any first line comment */
  numChars = fread ((unsigned char *)header, 1, 255, fp);
  for (i = 0, type=VTK_ASCII; i< numChars && type == VTK_ASCII; i++) // don't test \0
    {
    if (header[i] > 127)
      {
      type = VTK_BINARY;
      }
    }

  // Reset file for reading
  //
  rewind (fp);
  return type;
}

// Specify a spatial locator for merging points. By
// default an instance of vtkMergePoints is used.
void vtkSTLReader::SetLocator(vtkPointLocator *locator)
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

void vtkSTLReader::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
    {
    this->Locator = vtkMergePoints::New();
    }
}

void vtkSTLReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataSource::PrintSelf(os,indent);

  os << indent << "File Name: " 
     << (this->FileName ? this->FileName : "(none)") << "\n";

  os << indent << "Merging: " << (this->Merging ? "On\n" : "Off\n");
  os << indent << "ScalarTags: " << (this->ScalarTags ? "On\n" : "Off\n");
  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }
}
