/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUGFacetReader.cc
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
#include "vtkUGFacetReader.hh"

vtkUGFacetReader::vtkUGFacetReader()
{
  this->Filename = NULL;
}

vtkUGFacetReader::~vtkUGFacetReader()
{
  if ( this->Filename ) delete [] this->Filename;
}

void vtkUGFacetReader::Execute()
{
  FILE *fp;
  char header[36];
  struct {float  v1[3], v2[3], v3[3], n1[3], n2[3], n3[3];} facet;
  int ptId[3];
  short ugiiColor, direction;
  int numberTris, numFacetSets, setNumber, facetNumber;
  vtkFloatPoints *newPoints;
  vtkFloatNormals *newNormals;
  vtkCellArray *newPolys;
  vtkPolyData *output=(vtkPolyData *)this->Output;

  vtkDebugMacro(<<"Reading UG facet file...");
  if ( this->Filename == NULL )
    {
    vtkErrorMacro(<<"No filename specified...please specify one.");
    return;
    }

  // open the file
  if ( (fp = fopen(this->Filename, "r")) == NULL) 
    {
    vtkErrorMacro(<<"Cannot open file specified.");
    return;
    }

  // read the header stuff
  if ( fread (header, 1, 2, fp) <= 0 ||
  fread (&numFacetSets, 4, 1, fp) <= 0 ||
  fread (header, 1, 36, fp) <= 0 )
    {
    vtkErrorMacro(<<"File ended prematurely");
    return;
    }

  // allocate memory
  newPoints = new vtkFloatPoints(25000,25000);
  newNormals = new vtkFloatNormals(25000,25000);
  newPolys = new vtkCellArray;
  newPolys->Allocate(newPolys->EstimateSize(25000,3),25000);

  // loop over all facet sets, extracting triangles
  for (setNumber=0; setNumber < numFacetSets; setNumber++) 
    {

    if ( fread (&ugiiColor, 2, 1, fp) <= 0 ||
    fread (&direction, 2, 1, fp) <= 0 ||
    fread (&numberTris, 4, 1, fp) <= 0 )
      {
      vtkErrorMacro(<<"File ended prematurely");
      break;
      }

    for (facetNumber=0; facetNumber < numberTris; facetNumber++)
      {
      if ( fread(&facet,72,1,fp) <= 0 )
        {
        vtkErrorMacro(<<"File ended prematurely");
        break;
        }

      ptId[0] = newPoints->InsertNextPoint(facet.v1);
      ptId[1] = newPoints->InsertNextPoint(facet.v2);
      ptId[2] = newPoints->InsertNextPoint(facet.v3);

      newNormals->InsertNormal(ptId[0],facet.n1);
      newNormals->InsertNormal(ptId[1],facet.n2);
      newNormals->InsertNormal(ptId[2],facet.n3);

      newPolys->InsertNextCell(3,ptId);
      }
    }

  // update output
  vtkDebugMacro(<<"Read " 
                << newPolys->GetNumberOfCells() << " triangles, "
                << newPoints->GetNumberOfPoints() << " points.");

  output->SetPoints(newPoints);
  newPoints->Delete();

  output->GetPointData()->SetNormals(newNormals);
  newNormals->Delete();
 
  output->SetPolys(newPolys);
  newPolys->Delete();

  output->Squeeze();
}

void vtkUGFacetReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolySource::PrintSelf(os,indent);

  os << indent << "Filename: " 
     << (this->Filename ? this->Filename : "(none)") << "\n";
}

