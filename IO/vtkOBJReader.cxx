/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOBJReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOBJReader.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkOBJReader, "1.24");
vtkStandardNewMacro(vtkOBJReader);

// Description:
// Instantiate object with NULL filename.
vtkOBJReader::vtkOBJReader()
{
  this->FileName = NULL;
}

vtkOBJReader::~vtkOBJReader()
{
  if (this->FileName)
    {
    delete [] this->FileName;
    this->FileName = NULL;
    }
}

/*--------------------------------------------------------

This is only partial support for the OBJ format, which is 
quite complicated. To find a full specification,
search the net for "OBJ format", eg.:

http://netghost.narod.ru/gff/graphics/summary/waveobj.htm

We support the following types:

v <x> <y> <z>              vertex

vn <x> <y> <z>             vertex normal

vt <x> <y>                 texture coordinate

f <v_a> <v_b> <v_c> ...    

polygonal face linking vertices v_a, v_b, v_c, etc. which 
are 1-based indices into the vertex list

f <v_a>/<t_a> <v_b>/<t_b> ...

polygonal face as above, but with texture coordinates for 
each vertex. t_a etc. are 1-based indices into the texture
coordinates list (from the vt lines)

f <v_a>/<t_a>/<n_a> <v_b>/<t_b>/<n_b> ...

polygonal face as above, with a normal at each vertex, as a 
1-based index into the normals list (from the vn lines)

f <v_a>//<n_a> <v_b>//<n_b> ...

polygonal face as above but without texture coordinates.

Per-face tcoords and normals are supported by duplicating
the vertices on each face as necessary. 

---------------------------------------------------------*/

// a replacement for isspace()
int is_whitespace(char c)
{
  if ( c==' ' || c=='\t' || c=='\n' || c=='\r' || c=='\v' || c=='\f')
    return 1;
  else
    return 0;
}

void vtkOBJReader::Execute()
{
  if (!this->FileName) 
    {
    vtkErrorMacro(<< "A FileName must be specified.");
    return;
    }
    
  FILE *in = fopen(this->FileName,"r");
    
  if (in == NULL) 
    {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    return;
    }
    
  vtkDebugMacro(<<"Reading file");
    
  // intialise some structures to store the file contents in
  vtkPoints *points = vtkPoints::New(); 
  vtkFloatArray *tcoords = vtkFloatArray::New();
  tcoords->SetNumberOfComponents(2);
  vtkFloatArray *normals = vtkFloatArray::New();
  normals->SetNumberOfComponents(3);
  vtkCellArray *polys = vtkCellArray::New();
  vtkCellArray *tcoord_polys = vtkCellArray::New();
  int hasTCoords=0; // (false)
  int tcoords_same_as_verts=1; // (true)
  vtkCellArray *normal_polys = vtkCellArray::New();
  int hasNormals=0; // (false)
  int normals_same_as_verts=1; // (true)
    
  int everything_ok = 1; // (true)   (use of this flag avoids early return and associated memory leak)

  // -- work through the file line by line, assigning into the above six structures as appropriate --

  { // (make a local scope section to emphasise that the variables below are only used here)

  const int MAX_LINE=1024;
  char line[MAX_LINE],*pChar;
  float xyz[3];
  int iVert,iTCoord,iNormal;
  while (everything_ok && fgets(line,MAX_LINE,in)!=NULL) 
    {

    // in the OBJ format the first characters determine how to interpret the line:
    if (strncmp(line,"v ",2)==0) 
      {
      // this is a vertex definition, expect three floats, separated by whitespace:
      if (sscanf(line, "v %f %f %f", xyz, xyz + 1, xyz + 2)==3) 
        {
        points->InsertNextPoint(xyz);
        }
      else 
        {
        vtkErrorMacro(<<"Error in reading file");
        everything_ok=0; // (false)
        }
      }
    else if (strncmp(line,"vt ",3)==0) 
      {
      // this is a tcoord, expect two floats, separated by whitespace:
      if (sscanf(line, "vt %f %f", xyz, xyz + 1)==2) 
        {
        tcoords->InsertNextTuple(xyz);
        }
      else 
        {
        vtkErrorMacro(<<"Error in reading file");
        everything_ok=0; // (false)
        }
      }
    else if (strncmp(line,"vn ",3)==0) 
      {
      // this is a normal, expect three floats, separated by whitespace:
      if (sscanf(line, "vn %f %f %f", xyz, xyz + 1, xyz + 2)==3) 
        {
        normals->InsertNextTuple(xyz);
        }
      else 
        {
        vtkErrorMacro(<<"Error in reading file");
        everything_ok=0; // (false)
        }
      }
    else if (strncmp(line,"f ",2)==0 || strncmp(line,"fo ",3)==0)   // not sure why "fo" here
      {
      // this is a face definition, consisting of 1-based indices separated by whitespace and /

      polys->InsertNextCell(0); // we don't yet know how many points are to come
      tcoord_polys->InsertNextCell(0);
      normal_polys->InsertNextCell(0);

      int nVerts=0,nTCoords=0,nNormals=0; // keep a count of how many of each there are

      pChar = line + 2;
      const char *pEnd = line + strlen(line);

      while (everything_ok && pChar<pEnd) 
        {
        // find the first non-whitespace character
        while (is_whitespace(*pChar) && pChar<pEnd) { pChar++; }

        if (pChar<pEnd)         // there is still data left on this line
          {
          if (sscanf(pChar,"%d/%d/%d",&iVert,&iTCoord,&iNormal)==3) 
            {
            polys->InsertCellPoint(iVert-1); // convert to 0-based index
            nVerts++;
            tcoord_polys->InsertCellPoint(iTCoord-1);
            nTCoords++;
            if (iTCoord!=iVert && tcoords_same_as_verts)
              tcoords_same_as_verts = 0; // (false)
            normal_polys->InsertCellPoint(iNormal-1);
            nNormals++;
            if (iNormal!=iVert && normals_same_as_verts)
              normals_same_as_verts = 0; // (false)
            }
          else if (sscanf(pChar,"%d//%d",&iVert,&iNormal)==2) 
            {
            polys->InsertCellPoint(iVert-1);
            nVerts++;
            normal_polys->InsertCellPoint(iNormal-1);
            nNormals++;
            if (iNormal!=iVert && normals_same_as_verts)
              normals_same_as_verts = 0; // (false)
            }
          else if (sscanf(pChar,"%d/%d",&iVert,&iTCoord)==2) 
            {
            polys->InsertCellPoint(iVert-1);
            nVerts++;
            tcoord_polys->InsertCellPoint(iTCoord-1);
            nTCoords++;
            if (iTCoord!=iVert && tcoords_same_as_verts)
              tcoords_same_as_verts = 0; // (false)
            }
          else if (sscanf(pChar,"%d",&iVert)==1) 
            {
            polys->InsertCellPoint(iVert-1);
            nVerts++;
            }
          else 
            {
            vtkErrorMacro(<<"Error in reading file");
            everything_ok=0; // (false)
            }
          // skip over what we just read
          // (find the first whitespace character)
          while (!is_whitespace(*pChar) && pChar<pEnd) { pChar++; }

          }

        }

      // count of tcoords and normals must be equal to number of vertices or zero
      if (nVerts==0 || (nTCoords>0 && nTCoords!=nVerts) || (nNormals>0 && nNormals!=nVerts)) 
        {
        vtkErrorMacro(<<"Error in reading file");
        everything_ok=0; // (false)
        }

      // now we know how many points there were in this cell
      polys->UpdateCellCount(nVerts);
      tcoord_polys->UpdateCellCount(nTCoords);
      normal_polys->UpdateCellCount(nNormals);

      // also make a note of whether any cells have tcoords, and whether any have normals
      if (nTCoords>0 && !hasTCoords) { hasTCoords=1; }
      if (nNormals>0 && !hasNormals) { hasNormals=1; }
      }
    else 
      {
      //vtkDebugMacro(<<"Ignoring line: "<<line);
      }

    } // (end of while loop)

  } // (end of local scope section)
    
  // we have finished with the file
  fclose(in); 

  if (everything_ok)   // (otherwise just release allocated memory and return)
    {
    // -- now turn this lot into a useable vtkPolyData --

    // if there are no tcoords or normals or they match exactly 
    // then we can just copy the data into the output (easy!)
    if ((!hasTCoords||tcoords_same_as_verts) && (!hasNormals||normals_same_as_verts)) 
      {
      vtkDebugMacro(<<"Copying file data into the output directly");

      this->GetOutput()->SetPoints(points);
      this->GetOutput()->SetPolys(polys);

      // if there is an exact correspondence between tcoords and vertices then can simply
      // assign the tcoords points as point data
      if (hasTCoords && tcoords_same_as_verts)
        this->GetOutput()->GetPointData()->SetTCoords(tcoords);

      // if there is an exact correspondence between normals and vertices then can simply
      // assign the normals as point data
      if (hasNormals && normals_same_as_verts)
        this->GetOutput()->GetPointData()->SetNormals(normals);

      this->GetOutput()->Squeeze();
      }
    // otherwise we can duplicate the vertices as necessary (a bit slower)
    else 
      {
      vtkDebugMacro(<<"Duplicating vertices so that tcoords and normals are correct");

      vtkPoints *new_points = vtkPoints::New();
      vtkFloatArray *new_tcoords = vtkFloatArray::New();
      new_tcoords->SetNumberOfComponents(2);
      vtkFloatArray *new_normals = vtkFloatArray::New();
      new_normals->SetNumberOfComponents(3);
      vtkCellArray *new_polys = vtkCellArray::New();

      // for each poly, copy its vertices into new_points (and point at them)
      // also copy its tcoords into new_tcoords
      // also copy its normals into new_normals
      polys->InitTraversal();
      tcoord_polys->InitTraversal();
      normal_polys->InitTraversal();
      int i,j;
      int dummy_warning_prevention_mechanism[1];
      int n_pts=-1,*pts=dummy_warning_prevention_mechanism;
      int n_tcoord_pts=-1,*tcoord_pts=dummy_warning_prevention_mechanism;
      int n_normal_pts=-1,*normal_pts=dummy_warning_prevention_mechanism;
      for (i=0;i<polys->GetNumberOfCells();i++) 
        {
        polys->GetNextCell(n_pts,pts); 
        tcoord_polys->GetNextCell(n_tcoord_pts,tcoord_pts);
        normal_polys->GetNextCell(n_normal_pts,normal_pts);

        // If some vertices have tcoords and not others (likewise normals)
        // then we must do something else VTK will complain. (crash on render attempt)
        // Easiest solution is to delete polys that don't have complete tcoords (if there 
        // are any tcoords in the dataset) or normals (if there are any normals in the dataset).

        if ( (n_pts!=n_tcoord_pts && hasTCoords) || (n_pts!=n_normal_pts && hasNormals) ) 
          {
          // skip this poly
          vtkDebugMacro(<<"Skipping poly "<<i+1<<" (1-based index)");
          }
        else 
          {
          // copy the corresponding points, tcoords and normals across
          for (j=0;j<n_pts;j++) 
            {
            // copy the tcoord for this point across (if there is one)
            if (n_tcoord_pts>0)
              new_tcoords->InsertNextTuple(tcoords->GetTuple(tcoord_pts[j]));
            // copy the normal for this point across (if there is one)
            if (n_normal_pts>0)
              new_normals->InsertNextTuple(normals->GetTuple(normal_pts[j]));
            // copy the vertex into the new structure and update
            // the vertex index in the polys structure (pts is a pointer into it)
            pts[j] = new_points->InsertNextPoint(points->GetPoint(pts[j]));
            }
          // copy this poly (pointing at the new points) into the new polys list 
          new_polys->InsertNextCell(n_pts,pts);
          }
        }

      // use the new structures for the output
      this->GetOutput()->SetPoints(new_points);
      this->GetOutput()->SetPolys(new_polys);
      if (hasTCoords)
        this->GetOutput()->GetPointData()->SetTCoords(new_tcoords);
      if (hasNormals)
        this->GetOutput()->GetPointData()->SetNormals(new_normals);
      this->GetOutput()->Squeeze();

      new_points->Delete();
      new_polys->Delete();
      new_tcoords->Delete();
      new_normals->Delete();
      }
    }

  points->Delete();
  tcoords->Delete();
  normals->Delete();
  polys->Delete();
  tcoord_polys->Delete();
  normal_polys->Delete();
}

void vtkOBJReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "File Name: " 
     << (this->FileName ? this->FileName : "(none)") << "\n";

}

