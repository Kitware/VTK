/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMCubesReader.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <sys/stat.h>
#include "vtkMCubesReader.hh"
#include "vtkMergePoints.hh"

// Description:
// Construct object with FlipNormals and Normals set to true.
vtkMCubesReader::vtkMCubesReader()
{
  this->Filename = NULL;
  this->LimitsFilename = NULL;

  this->Locator = NULL;
  this->SelfCreatedLocator = 0;

  this->FlipNormals = 0;
  this->Normals = 1;
}

vtkMCubesReader::~vtkMCubesReader()
{
  if (this->Filename) delete [] this->Filename;
  if (this->LimitsFilename) delete [] this->LimitsFilename;
  if (this->SelfCreatedLocator) this->Locator->Delete();
}

void vtkMCubesReader::Execute()
{
  FILE *fp, *limitp;
  vtkFloatPoints *newPts;
  vtkCellArray *newPolys;
  vtkFloatNormals *newNormals;
  float bounds[6];
  int i, j, k, numPts, numTris;
  typedef struct {float x[3], n[3];} pointType;
  pointType point;
  struct  stat buf;
  int nodes[3], numDegenerate=0;
  float direction, n[3], dummy[2];
  vtkPolyData *output = this->GetOutput();
  
  vtkDebugMacro(<<"Reading marching cubes file");
  //
  // Initialize
  //

  if ( this->Filename == NULL )
    {
    vtkErrorMacro(<< "Please specify input filename");
    return;
    }
  if ( (fp = fopen(this->Filename, "r")) == NULL)
    {
    vtkErrorMacro(<< "File " << this->Filename << " not found");
    return;
    }

  // Try to read limits file to get bounds. Otherwise, read data.
  if ( this->LimitsFilename != NULL && 
  (limitp = fopen (this->LimitsFilename, "r")) != NULL &&
  stat (this->Filename, &buf) == 0 )
    {
    // skip first three pairs
    fread (dummy, sizeof(float), 2, limitp);
    fread (dummy, sizeof(float), 2, limitp);
    fread (dummy, sizeof(float), 2, limitp);

    // next three pairs are x, y, z limits
    for (i = 0; i < 6; i++) fread (&bounds[i], sizeof (float), 1, limitp);

    fclose (limitp);

    // calculate the number of triangles and vertices from file size

    numTris = buf.st_size / (18*sizeof(float)); //3 points + normals
    numPts = numTris * 3;	    
    }
  else // read data to get bounds
    {
    bounds[0] = bounds[2] = bounds[4] = VTK_LARGE_FLOAT;
    bounds[1] = bounds[3] = bounds[5] = -VTK_LARGE_FLOAT;
    for (i=0; fread(&point, sizeof(pointType), 1, fp); i++) 
      {
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
  newPts = new vtkFloatPoints(numPts);
  newPolys = new vtkCellArray;
  newPolys->Allocate(newPolys->EstimateSize(numTris,3));

  if ( this->Normals ) newNormals = new vtkFloatNormals(numPts);
  
  if ( this->Locator == NULL ) this->CreateDefaultLocator();
  this->Locator->InitPointInsertion (newPts, bounds);

  direction = this->FlipNormals ? 1.0 : -1.0;

  for ( i=0; i<numTris; i++) 
    {
    for (j=0; j<3; j++) 
      {
      fread (&point, sizeof(pointType), 1, fp);
      nodes[j] = this->Locator->InsertPoint(point.x);
      if ( this->Normals )
        {
        for (k=0; k<3; k++) n[k] = point.n[k] * direction;
        newNormals->InsertNormal(nodes[j],n);
        }
      }
    if ( nodes[0] != nodes[1] && nodes[0] != nodes[2] && 
    nodes[1] != nodes[2] )
      newPolys->InsertNextCell(3,nodes);
    else
      numDegenerate++;
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

  if (this->Locator) this->Locator->Initialize(); //free storage
}

// Description:
// Specify a spatial locator for merging points. By default, 
// an instance of vtkMergePoints is used.
void vtkMCubesReader::SetLocator(vtkPointLocator *locator)
{
  if ( this->Locator != locator ) 
    {
    if ( this->SelfCreatedLocator ) this->Locator->Delete();
    this->SelfCreatedLocator = 0;
    this->Locator = locator;
    this->Modified();
    }
}

void vtkMCubesReader::CreateDefaultLocator()
{
  if ( this->SelfCreatedLocator ) this->Locator->Delete();
  this->Locator = new vtkMergePoints;
  this->SelfCreatedLocator = 1;
}

void vtkMCubesReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolySource::PrintSelf(os,indent);

  os << indent << "Filename: " 
     << (this->Filename ? this->Filename : "(none)") << "\n";
  os << indent << "Limits Filename: " 
     << (this->LimitsFilename ? this->LimitsFilename : "(none)") << "\n";
  os << indent << "Normals: " << (this->Normals ? "On\n" : "Off\n");
  os << indent << "FlipNormals: " << (this->FlipNormals ? "On\n" : "Off\n");
}
