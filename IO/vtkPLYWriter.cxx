/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLYWriter.cxx
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
#include "vtkPLYWriter.h"
#include "ply.h"
#include "vtkObjectFactory.h"

//--------------------------------------------------------------------------
vtkPLYWriter* vtkPLYWriter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPLYWriter");
  if(ret)
    {
    return (vtkPLYWriter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPLYWriter;
}

vtkPLYWriter::vtkPLYWriter()
{
  this->FileType = VTK_BINARY;
  this->DataByteOrder = VTK_LITTLE_ENDIAN;
 
  this->WritePointScalars = 0;
  this->PointLookupTable = NULL;
  
  this->WriteCellScalars = 0;
  this->CellLookupTable = NULL;
  
}

typedef struct _plyVertex {
  float x[3];             /* the usual 3-space position of a vertex */
} plyVertex;

typedef struct _plyFace {
  unsigned char nverts;    /* number of vertex indices in list */
  int *verts;              /* vertex index list */
} plyFace;

void vtkPLYWriter::WriteData()
{
  int i, j;
  vtkPoints *inPts;
  vtkCellArray *polys;
  vtkPolyData *input = this->GetInput();
  PlyFile *ply;
  float version;
  static char *elemNames[] = { "vertex", "face" };
  static PlyProperty vertProps[] = { // property information for a vertex
    {"x", PLY_FLOAT, PLY_FLOAT, offsetof(plyVertex,x[0]), 0, 0, 0, 0},
    {"y", PLY_FLOAT, PLY_FLOAT, offsetof(plyVertex,x[1]), 0, 0, 0, 0},
    {"z", PLY_FLOAT, PLY_FLOAT, offsetof(plyVertex,x[2]), 0, 0, 0, 0},
  };
  static PlyProperty faceProps[] = { // property information for a face
    {"vertex_indices", PLY_INT, PLY_INT, offsetof(plyFace,verts),
     1, PLY_UCHAR, PLY_UCHAR, offsetof(plyFace,nverts)},
  };

  // Get input and check data
  polys = input->GetPolys();
  inPts = input->GetPoints();
  if (inPts == NULL || polys == NULL )
    {
    vtkErrorMacro(<<"No data to write!");
    return;
    }

  if ( this->FileName == NULL)
    {
    vtkErrorMacro(<< "Please specify FileName to write");
    return;
    }

  // Open the file in appropriate way
  if ( this->FileType == VTK_BINARY )
    {
    if ( this->DataByteOrder == VTK_LITTLE_ENDIAN )
      {
      ply = ply_open_for_writing(this->FileName, 2, elemNames, 
                                 PLY_BINARY_LE, &version);
      }
    else
      {
      ply = ply_open_for_writing(this->FileName, 2, elemNames, 
                                 PLY_BINARY_BE, &version);
      }
    }
  else
    {
    ply = ply_open_for_writing(this->FileName, 2, elemNames, 
                               PLY_ASCII, &version);
    }

  if ( ply == NULL)
    {
    vtkErrorMacro(<< "Error opening PLY file");
    return;
    }
  
  // describe what properties go into the vertex and face elements
  int numPts = inPts->GetNumberOfPoints();
  ply_element_count (ply, "vertex", numPts);
  ply_describe_property (ply, "vertex", &vertProps[0]);
  ply_describe_property (ply, "vertex", &vertProps[1]);
  ply_describe_property (ply, "vertex", &vertProps[2]);

  int numPolys = polys->GetNumberOfCells();
  ply_element_count (ply, "face", numPolys);
  ply_describe_property (ply, "face", &faceProps[0]);

  // write a comment and an object information field
  ply_put_comment (ply, "VTK generated PLY File");
  ply_put_obj_info (ply, "vtkPolyData points and polygons: vtk4.0");

  // complete the header
  ply_header_complete (ply);

  /* set up and write the vertex elements */
  plyVertex vert;
  ply_put_element_setup (ply, "vertex");
  for (i = 0; i < numPts; i++)
    {
    inPts->GetPoint(i,vert.x);
    ply_put_element (ply, (void *) &vert);
    }

  /* set up and write the face elements */
  plyFace face;
  int verts[256];
  face.verts = verts;
  ply_put_element_setup (ply, "face");
  vtkIdType npts, *pts;
  for (i = 0; i < numPolys; i++)
    {
    polys->GetNextCell(npts,pts);
    if ( npts > 256 )
      {
      vtkErrorMacro(<<"Ply file only supports polygons with <256 points");
      }
    else
      {
      for (j=0; j<npts; j++)
        {
        face.nverts = npts;
        verts[j] = (int)pts[j];
        }
      ply_put_element (ply, (void *) &face);
      }
    }//for all polygons

  /* close the PLY file */
  ply_close (ply);
  
  }
  


