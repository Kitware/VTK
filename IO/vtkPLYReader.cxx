/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLYReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Mike Dresser MD/PhD
             Director of Core Facility for Imaging
             Program in Molecular and Cell Biology
             Oklahoma Medical Research Foundation


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
#include "vtkPLYReader.h"
#include "vtkPLY.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"

//---------------------------------------------------------------------------
vtkPLYReader* vtkPLYReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPLYReader");
  if(ret)
    {
    return (vtkPLYReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPLYReader;
}

// Construct object with merging set to true.
vtkPLYReader::vtkPLYReader()
{
  this->FileName = NULL;
}

vtkPLYReader::~vtkPLYReader()
{
  if (this->FileName)
    {
    delete [] this->FileName;
    }
}

typedef struct _plyVertex {
  float x[3];             // the usual 3-space position of a vertex
} plyVertex;

typedef struct _plyFace {
  unsigned char nverts;   // number of vertex indices in list
  int *verts;             // vertex index list
} plyFace;

void vtkPLYReader::Execute()
{
  PlyProperty vertProps[] = {
    {"x", PLY_FLOAT, PLY_FLOAT, offsetof(plyVertex,x[0]), 0, 0, 0, 0},
    {"y", PLY_FLOAT, PLY_FLOAT, offsetof(plyVertex,x[1]), 0, 0, 0, 0},
    {"z", PLY_FLOAT, PLY_FLOAT, offsetof(plyVertex,x[2]), 0, 0, 0, 0},
  };
  PlyProperty faceProps[] = {
    {"vertex_indices", PLY_INT, PLY_INT, offsetof(plyFace,verts),
     1, PLY_UCHAR, PLY_UCHAR, offsetof(plyFace,nverts)},
  };

  int i, j, k;
  int numPts, numPolys;
  PlyProperty **plist;
  vtkPolyData *output = (vtkPolyData *)this->GetOutput();

  if (!this->FileName)
    {
    vtkErrorMacro(<<"A File Name must be specified.");
    return;
    }

  // open a PLY file for reading
  PlyFile *ply;
  int nelems, fileType, numElems, nprops;
  char **elist, *elemName;
  float version;
  
  ply = vtkPLY::ply_open_for_reading(this->FileName, &nelems, &elist, 
                             &fileType, &version);

  // Check to make sure that we can read geometry
  PlyElement *elem;
  int index;
  if ( (elem = vtkPLY::find_element (ply, "vertex")) == NULL ||
       vtkPLY::find_property (elem, "x", &index) == NULL ||
       vtkPLY::find_property (elem, "y", &index) == NULL ||
       vtkPLY::find_property (elem, "z", &index) == NULL ||
       (elem = vtkPLY::find_element (ply, "face")) == NULL ||
       vtkPLY::find_property (elem, "vertex_indices", &index) == NULL )
    {
    vtkErrorMacro(<<"Cannot read geometry");
    vtkPLY::ply_close (ply);
    }

  for (i = 0; i < nelems; i++) 
    {
    //get the description of the first element */
    elemName = elist[i];
    plist = vtkPLY::ply_get_element_description (ply, elemName, &numElems, &nprops);

    // if we're on vertex elements, read them in
    if ( elemName && !strcmp ("vertex", elemName) ) 
      {
      // Create a list of points
      numPts = numElems;
      vtkPoints *pts = vtkPoints::New();
      pts->SetDataTypeToFloat();
      pts->SetNumberOfPoints(numPts);
      float *ptsPtr = ((vtkFloatArray *)pts->GetData())->GetPointer(0);
      
      // Setup to read the PLY elements
      vtkPLY::ply_get_property (ply, elemName, &vertProps[0]);
      vtkPLY::ply_get_property (ply, elemName, &vertProps[1]);
      vtkPLY::ply_get_property (ply, elemName, &vertProps[2]);

      for (j=0; j < numPts; j++, ptsPtr+=3) 
        {
        vtkPLY::ply_get_element (ply, (void *) ptsPtr);
        }
      output->SetPoints(pts);
      pts->Delete();
      }//if vertex

    else if ( elemName && !strcmp ("face", elemName) ) 
      {
      // Create a polygonal array
      numPolys = numElems;
      vtkCellArray *polys = vtkCellArray::New();
      polys->Allocate(polys->EstimateSize(numPolys,3),numPolys/2);
      plyFace face;
      int verts[256];
      vtkIdType vtkVerts[256];

      // Get the face properties
      vtkPLY::ply_get_property (ply, elemName, &faceProps[0]);
      
      // grab all the face elements
      for (j=0; j < numPolys; j++) 
        {
        //grab and element from the file
        face.verts = verts;
        vtkPLY::ply_get_element (ply, (void *) &face);
        for (k=0; k<face.nverts; k++)
          {
          vtkVerts[k] = face.verts[k];
          }
        polys->InsertNextCell(face.nverts,vtkVerts);
        }
      output->SetPolys(polys);
      polys->Delete();
      }//if face
    
    }//for all elements of the PLY file
  
  vtkDebugMacro( <<"Read: " << numPts << " points, " 
                 << numPolys << " polygons");

  // close the PLY file 
  vtkPLY::ply_close (ply);

}

void vtkPLYReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataSource::PrintSelf(os,indent);

  os << indent << "File Name: " 
     << (this->FileName ? this->FileName : "(none)") << "\n";

}
