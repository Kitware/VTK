/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOBJReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOBJReader.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include <cctype>

vtkStandardNewMacro(vtkOBJReader);

// Description:
// Instantiate object with NULL filename.
vtkOBJReader::vtkOBJReader()
{
  this->FileName = NULL;

  this->SetNumberOfInputPorts(0);
}

vtkOBJReader::~vtkOBJReader()
{
  delete [] this->FileName;
  this->FileName = NULL;
}

/*---------------------------------------------------------------------------*\

This is only partial support for the OBJ format, which is quite complicated.
To find a full specification, search the net for "OBJ format", eg.:

    http://en.wikipedia.org/wiki/Obj
    http://netghost.narod.ru/gff/graphics/summary/waveobj.htm

We support the following types:

v <x> <y> <z>

    vertex

vn <x> <y> <z>

    vertex normal

vt <x> <y>

    texture coordinate

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

l <v_a> <v_b> ...

    lines linking vertices v_a, v_b, etc. which are 1-based
    indices into the vertex list

p <v_a> <v_b> ...

    points located at the vertices v_a, v_b, etc. which are 1-based
    indices into the vertex list

\*---------------------------------------------------------------------------*/


int vtkOBJReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!this->FileName)
  {
    vtkErrorMacro(<< "A FileName must be specified.");
    return 0;
  }

  FILE *in = fopen(this->FileName,"r");

  if (in == NULL)
  {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    return 0;
  }

  vtkDebugMacro(<<"Reading file");

  // initialize some structures to store the file contents in
  vtkPoints *points = vtkPoints::New();
  std::vector<vtkFloatArray*> tcoords_vector;
  vtkFloatArray *normals = vtkFloatArray::New();
  normals->SetNumberOfComponents(3);
  normals->SetName("Normals");
  vtkCellArray *polys = vtkCellArray::New();
  vtkCellArray *tcoord_polys = vtkCellArray::New();

  vtkCellArray *pointElems = vtkCellArray::New();
  vtkCellArray *lineElems = vtkCellArray::New();
  vtkCellArray *normal_polys = vtkCellArray::New();

  bool hasTCoords = false;
  bool hasNormals = false;
  bool tcoords_same_as_verts = true;
  bool normals_same_as_verts = true;
  bool everything_ok = true; // (use of this flag avoids early return and associated memory leak)

  // -- work through the file line by line, assigning into the above 7 structures as appropriate --

  { // (make a local scope section to emphasise that the variables below are only used here)

  const int MAX_LINE = 1024;
  char rawLine[MAX_LINE];
  char tcoordsName[100];
  float xyz[3];
  int numPoints = 0;
  int numTCoords = 0;
  int numNormals = 0;

  // First loop to initialize the data arrays for the different set of texture coordinates
  int lineNr = 0;
  while (everything_ok && fgets(rawLine, MAX_LINE, in) != NULL)
  {
    lineNr++;
    char *pLine = rawLine;
    char *pEnd = rawLine + strlen(rawLine);

    // find the first non-whitespace character
    while (isspace(*pLine) && pLine < pEnd) { pLine++; }

    // this first non-whitespace is the command
    const char *cmd = pLine;

    // skip over non-whitespace
    while (!isspace(*pLine) && pLine < pEnd) { pLine++; }

    // terminate command
    if (pLine < pEnd)
    {
      *pLine = '\0';
      pLine++;
    }

    // if line starts by "usemtl", we're listing a new set of texture coordinates
    if (strcmp(cmd, "usemtl") == 0)
    {
      // Read name of texture coordinate
      if (sscanf(pLine, "%s", tcoordsName) == 1)
      {
        // Go to next line to see if any texture coordinates exist
        if (fgets(rawLine, MAX_LINE, in) != NULL)
        {
          lineNr++;
          pLine = rawLine;
          pEnd = rawLine + strlen(rawLine);
          while (isspace(*pLine) && pLine < pEnd) { pLine++; }
          cmd = pLine;
          while (!isspace(*pLine) && pLine < pEnd) { pLine++; }
          if (pLine < pEnd)
          {
            *pLine = '\0';
            pLine++;
          }
          // if the line starts by "vt", there are texture coordinates associated
          if (strcmp(cmd, "vt") == 0)
          {
            vtkFloatArray* tcoords = vtkFloatArray::New();
            tcoords->SetNumberOfComponents(2);
            tcoords->SetName(tcoordsName);
            tcoords_vector.push_back(tcoords);
          }
        }
        else
        {
          vtkErrorMacro(<<"Error reading continuation line at line " << lineNr);
          everything_ok = false;
        }
      }
      else
      {
        vtkErrorMacro(<<"Error reading 'usemtl' at line " << lineNr);
        everything_ok = false;
      }
    }
  } // (end of first while loop)

  // If no material texture coordinates are found, add default TCoords
  if(tcoords_vector.size() == 0)
  {
    vtkFloatArray *tcoords = vtkFloatArray::New();
    tcoords->SetNumberOfComponents(2);
    tcoords->SetName("TCoords");
    tcoords_vector.push_back(tcoords);
    strcpy(tcoordsName,"TCoords");
  }

  // Second loop to parse points, faces, texture coordinates, normals...
  lineNr = 0;
  fseek(in, 0, SEEK_SET);
  while (everything_ok && fgets(rawLine, MAX_LINE, in) != NULL)
  {
    lineNr++;
    char *pLine = rawLine;
    char *pEnd = rawLine + strlen(rawLine);

    // find the first non-whitespace character
    while (isspace(*pLine) && pLine < pEnd) { pLine++; }

    // this first non-whitespace is the command
    const char *cmd = pLine;

    // skip over non-whitespace
    while (!isspace(*pLine) && pLine < pEnd) { pLine++; }

    // terminate command
    if (pLine < pEnd)
    {
      *pLine = '\0';
      pLine++;
    }

    // in the OBJ format the first characters determine how to interpret the line:
    if (strcmp(cmd, "v") == 0)
    {
      // this is a vertex definition, expect three floats, separated by whitespace:
      if (sscanf(pLine, "%f %f %f", xyz, xyz+1, xyz+2) == 3)
      {
        points->InsertNextPoint(xyz);
        numPoints++;
      }
      else
      {
        vtkErrorMacro(<<"Error reading 'v' at line " << lineNr);
        everything_ok = false;
      }
    }
    else if (strcmp(cmd, "usemtl") == 0)
    {
      // this is a new material name (for texture coordinates), expect one string:
      if (sscanf(pLine, "%s", tcoordsName) != 1)
      {
        vtkErrorMacro(<<"Error reading 'usemtl' at line " << lineNr);
        everything_ok = false;
      }
    }
    else if (strcmp(cmd, "vt") == 0)
    {
      // this is a tcoord, expect two floats, separated by whitespace:
      if (sscanf(pLine, "%f %f", xyz, xyz+1) == 2)
      {
        for(unsigned int i = 0; i < tcoords_vector.size(); ++i)
        {
          vtkFloatArray* tcoords = tcoords_vector.at(i);
          if(strcmp(tcoords->GetName(), tcoordsName) == 0)
          {
            // Add data to current array
            tcoords->InsertNextTuple(xyz);
          }
          else
          {
            // Add (-1,-1) to other arrays
            tcoords->InsertNextTuple2(-1.0, -1.0);
          }
        }
        numTCoords++;
      }
      else
      {
        vtkErrorMacro(<<"Error reading 'vt' at line " << lineNr);
        everything_ok = false;
      }
    }
    else if (strcmp(cmd, "vn") == 0)
    {
      // this is a normal, expect three floats, separated by whitespace:
      if (sscanf(pLine, "%f %f %f", xyz, xyz+1, xyz+2) == 3)
      {
        normals->InsertNextTuple(xyz);
        hasNormals = true;
        numNormals++;
      }
      else
      {
        vtkErrorMacro(<<"Error reading 'vn' at line " << lineNr);
        everything_ok = false;
      }
    }
    else if (strcmp(cmd, "p") == 0)
    {
      // this is a point definition, consisting of 1-based indices separated by whitespace and /
      pointElems->InsertNextCell(0); // we don't yet know how many points are to come

      int nVerts=0; // keep a count of how many there are

      while (everything_ok && pLine < pEnd)
      {
        // find next non-whitespace character
        while (isspace(*pLine) && pLine < pEnd) { pLine++; }

        if (pLine < pEnd)         // there is still data left on this line
        {
          int iVert;
          if (sscanf(pLine, "%d", &iVert) == 1)
          {
            if (iVert < 0)
            {
              pointElems->InsertCellPoint(numPoints+iVert);
            }
            else
            {
              pointElems->InsertCellPoint(iVert-1);
            }
            nVerts++;
          }
          else if (strcmp(pLine, "\\\n") == 0)
          {
            // handle backslash-newline continuation
            if (fgets(rawLine, MAX_LINE, in) != NULL)
            {
              lineNr++;
              pLine = rawLine;
              pEnd = rawLine + strlen(rawLine);
              continue;
            }
            else
            {
              vtkErrorMacro(<<"Error reading continuation line at line " << lineNr);
              everything_ok = false;
            }
          }
          else
          {
            vtkErrorMacro(<<"Error reading 'p' at line " << lineNr);
            everything_ok = false;
          }
          // skip over what we just sscanf'd
          // (find the first whitespace character)
          while (!isspace(*pLine) && pLine < pEnd) { pLine++; }
        }
      }

      if (nVerts < 1)
      {
        vtkErrorMacro
        (
            <<"Error reading file near line " << lineNr
            << " while processing the 'p' command"
        );
        everything_ok = false;
      }

      // now we know how many points there were in this cell
      pointElems->UpdateCellCount(nVerts);
    }
    else if (strcmp(cmd, "l") == 0)
    {
      // this is a line definition, consisting of 1-based indices separated by whitespace and /
      lineElems->InsertNextCell(0); // we don't yet know how many points are to come

      int nVerts=0; // keep a count of how many there are

      while (everything_ok && pLine < pEnd)
      {
        // find next non-whitespace character
        while (isspace(*pLine) && pLine < pEnd) { pLine++; }

        if (pLine < pEnd)         // there is still data left on this line
        {
          int iVert, dummyInt;
          if (sscanf(pLine, "%d/%d", &iVert, &dummyInt) == 2)
          {
            // we simply ignore texture information
            if (iVert < 0)
            {
              lineElems->InsertCellPoint(numPoints+iVert);
            }
            else
            {
              lineElems->InsertCellPoint(iVert-1);
            }
            nVerts++;
          }
          else if (sscanf(pLine, "%d", &iVert) == 1)
          {
            if (iVert < 0)
            {
              lineElems->InsertCellPoint(numPoints+iVert);
            }
            else
            {
              lineElems->InsertCellPoint(iVert-1);
            }
            nVerts++;
          }
          else if (strcmp(pLine, "\\\n") == 0)
          {
            // handle backslash-newline continuation
            if (fgets(rawLine, MAX_LINE, in) != NULL)
            {
              lineNr++;
              pLine = rawLine;
              pEnd = rawLine + strlen(rawLine);
              continue;
            }
            else
            {
              vtkErrorMacro(<<"Error reading continuation line at line " << lineNr);
              everything_ok = false;
            }
          }
          else
          {
            vtkErrorMacro(<<"Error reading 'l' at line " << lineNr);
            everything_ok = false;
          }
          // skip over what we just sscanf'd
          // (find the first whitespace character)
          while (!isspace(*pLine) && pLine < pEnd) { pLine++; }
        }
      }

      if (nVerts < 2)
      {
        vtkErrorMacro
        (
            <<"Error reading file near line " << lineNr
            << " while processing the 'l' command"
        );
        everything_ok = false;
      }

      // now we know how many points there were in this cell
      lineElems->UpdateCellCount(nVerts);
    }
    else if (strcmp(cmd, "f") == 0)
    {
      // this is a face definition, consisting of 1-based indices separated by whitespace and /

      polys->InsertNextCell(0); // we don't yet know how many points are to come
      tcoord_polys->InsertNextCell(0);
      normal_polys->InsertNextCell(0);

      int nVerts=0, nTCoords=0, nNormals=0; // keep a count of how many of each there are

      while (everything_ok && pLine < pEnd)
      {
        // find the first non-whitespace character
        while (isspace(*pLine) && pLine < pEnd) { pLine++; }

        if (pLine < pEnd)         // there is still data left on this line
        {
          int iVert,iTCoord,iNormal;
          if (sscanf(pLine, "%d/%d/%d", &iVert, &iTCoord, &iNormal) == 3)
          {
            if (iVert < 0)
            {
              polys->InsertCellPoint(numPoints+iVert);
            }
            else
            {
              polys->InsertCellPoint(iVert-1);
            }
            nVerts++;

            // Current index is relative to last texture index
            if (iTCoord < 0)
            {
              tcoord_polys->InsertCellPoint(numTCoords + iTCoord);
            }
            else
            {
              tcoord_polys->InsertCellPoint(iTCoord - 1);
            }
            nTCoords++;

            // Current index is relative to last normal index
            if (iNormal < 0)
            {
              normal_polys->InsertCellPoint(numNormals + iNormal);
            }
            else
            {
              normal_polys->InsertCellPoint(iNormal - 1);
            }
            nNormals++;
            if (iTCoord != iVert)
              tcoords_same_as_verts = false;
            if (iNormal != iVert)
              normals_same_as_verts = false;
          }
          else if (sscanf(pLine, "%d//%d", &iVert, &iNormal) == 2)
          {
            if (iVert < 0)
            {
              polys->InsertCellPoint(numPoints+iVert);
            }
            else
            {
              polys->InsertCellPoint(iVert-1);
            }
            nVerts++;

            // Current index is relative to last normal index
            if (iNormal < 0)
            {
              normal_polys->InsertCellPoint(numNormals + iNormal);
            }
            else
            {
              normal_polys->InsertCellPoint(iNormal - 1);
            }
            nNormals++;
            if (iNormal != iVert)
              normals_same_as_verts = false;
          }
          else if (sscanf(pLine, "%d/%d", &iVert, &iTCoord) == 2)
          {
            if (iVert < 0)
            {
              polys->InsertCellPoint(numPoints+iVert);
            }
            else
            {
              polys->InsertCellPoint(iVert-1);
            }
            nVerts++;

            // Current index is relative to last texture index
            if (iTCoord < 0)
            {
              tcoord_polys->InsertCellPoint(numTCoords + iTCoord);
            }
            else
            {
              tcoord_polys->InsertCellPoint(iTCoord - 1);
            }
            nTCoords++;
            if (iTCoord != iVert)
              tcoords_same_as_verts = false;
          }
          else if (sscanf(pLine, "%d", &iVert) == 1)
          {
            if (iVert < 0)
            {
              polys->InsertCellPoint(numPoints+iVert);
            }
            else
            {
              polys->InsertCellPoint(iVert-1);
            }
            nVerts++;
          }
          else if (strcmp(pLine, "\\\n") == 0)
          {
            // handle backslash-newline continuation
            if (fgets(rawLine, MAX_LINE, in) != NULL)
            {
              lineNr++;
              pLine = rawLine;
              pEnd = rawLine + strlen(rawLine);
              continue;
            }
            else
            {
              vtkErrorMacro(<<"Error reading continuation line at line " << lineNr);
              everything_ok = false;
            }
          }
          else
          {
            vtkErrorMacro(<<"Error reading 'f' at line " << lineNr);
            everything_ok = false;
          }
          // skip over what we just read
          // (find the first whitespace character)
          while (!isspace(*pLine) && pLine < pEnd) { pLine++; }
        }
      }

      // count of tcoords and normals must be equal to number of vertices or zero
      if ( nVerts < 3 ||
           (nTCoords > 0 && nTCoords != nVerts) ||
           (nNormals > 0 && nNormals != nVerts)
         )
      {
        vtkErrorMacro
        (
            <<"Error reading file near line " << lineNr
            << " while processing the 'f' command"
        );
        everything_ok = false;
      }

      // now we know how many points there were in this cell
      polys->UpdateCellCount(nVerts);
      tcoord_polys->UpdateCellCount(nTCoords);
      normal_polys->UpdateCellCount(nNormals);

      // also make a note of whether any cells have tcoords, and whether any have normals
      if (nTCoords > 0) { hasTCoords = true; }
      if (nNormals > 0) { hasNormals = true; }
    }
    else
    {
      //vtkDebugMacro(<<"Ignoring line: "<<rawLine);
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
    if (
        (!hasTCoords || tcoords_same_as_verts) &&
        (!hasNormals || normals_same_as_verts)
       )
    {
      vtkDebugMacro(<<"Copying file data into the output directly");

      output->SetPoints(points);
      if (pointElems->GetNumberOfCells())
      {
        output->SetVerts(pointElems);
      }
      if (lineElems->GetNumberOfCells())
      {
        output->SetLines(lineElems);
      }
      if (polys->GetNumberOfCells())
      {
        output->SetPolys(polys);
      }

      // if there is an exact correspondence between tcoords and vertices then can simply
      // assign the tcoords points as point data
      if (hasTCoords && tcoords_same_as_verts)
      {
        for(unsigned int i = 0; i < tcoords_vector.size(); ++i)
        {
          vtkFloatArray* tcoords = tcoords_vector.at(i);
          output->GetPointData()->AddArray(tcoords);
          if(i == 0)
          {
            output->GetPointData()->SetActiveTCoords(tcoords->GetName());
          }
        }
      }

      // if there is an exact correspondence between normals and vertices then can simply
      // assign the normals as point data
      if (hasNormals && normals_same_as_verts)
      {
        output->GetPointData()->SetNormals(normals);
      }
      output->Squeeze();
    }
    // otherwise we can duplicate the vertices as necessary (a bit slower)
    else
    {
      vtkDebugMacro(<<"Duplicating vertices so that tcoords and normals are correct");

      vtkPoints *new_points = vtkPoints::New();
      std::vector<vtkFloatArray*> new_tcoords_vector;
      for(unsigned int i = 0; i < tcoords_vector.size(); ++i)
      {
        vtkFloatArray* tcoords = tcoords_vector.at(i);
        vtkFloatArray *new_tcoords = vtkFloatArray::New();
        new_tcoords->SetName(tcoords->GetName());
        new_tcoords->SetNumberOfComponents(2);
        new_tcoords_vector.push_back(new_tcoords);
      }
      vtkFloatArray *new_normals = vtkFloatArray::New();
      new_normals->SetNumberOfComponents(3);
      new_normals->SetName("Normals");
      vtkCellArray *new_polys = vtkCellArray::New();

      // for each poly, copy its vertices into new_points (and point at them)
      // also copy its tcoords into new_tcoords
      // also copy its normals into new_normals
      polys->InitTraversal();
      tcoord_polys->InitTraversal();
      normal_polys->InitTraversal();

      vtkIdType dummy_warning_prevention_mechanism[1];
      vtkIdType n_pts=-1,*pts=dummy_warning_prevention_mechanism;
      vtkIdType n_tcoord_pts=-1,*tcoord_pts=dummy_warning_prevention_mechanism;
      vtkIdType n_normal_pts=-1,*normal_pts=dummy_warning_prevention_mechanism;
      for (int i=0; i<polys->GetNumberOfCells(); ++i)
      {
        polys->GetNextCell(n_pts,pts);
        tcoord_polys->GetNextCell(n_tcoord_pts,tcoord_pts);
        normal_polys->GetNextCell(n_normal_pts,normal_pts);

        // If some vertices have tcoords and not others (likewise normals)
        // then we must do something else VTK will complain. (crash on render attempt)
        // Easiest solution is to delete polys that don't have complete tcoords (if there
        // are any tcoords in the dataset) or normals (if there are any normals in the dataset).

        if (
            (n_pts != n_tcoord_pts && hasTCoords) ||
            (n_pts != n_normal_pts && hasNormals)
           )
        {
          // skip this poly
          vtkDebugMacro(<<"Skipping poly "<<i+1<<" (1-based index)");
        }
        else
        {
          // copy the corresponding points, tcoords and normals across
          for (int j=0; j<n_pts; ++j)
          {
            // copy the tcoord for this point across (if there is one)
            if (n_tcoord_pts>0)
            {
              for(unsigned int k = 0; k < tcoords_vector.size(); ++k)
              {
                vtkFloatArray* new_tcoords = new_tcoords_vector.at(k);
                vtkFloatArray* tcoords = tcoords_vector.at(k);
                new_tcoords->InsertNextTuple(tcoords->GetTuple(tcoord_pts[j]));
              }
            }
            // copy the normal for this point across (if there is one)
            if (n_normal_pts>0)
            {
              new_normals->InsertNextTuple(normals->GetTuple(normal_pts[j]));
            }
            // copy the vertex into the new structure and update
            // the vertex index in the polys structure (pts is a pointer into it)
            pts[j] = new_points->InsertNextPoint(points->GetPoint(pts[j]));
          }
          // copy this poly (pointing at the new points) into the new polys list
          new_polys->InsertNextCell(n_pts,pts);
        }
      }

      // use the new structures for the output
      output->SetPoints(new_points);
      output->SetPolys(new_polys);
      if (hasTCoords)
      {
        for(unsigned int i = 0; i < new_tcoords_vector.size(); ++i)
        {
          vtkFloatArray* new_tcoords = new_tcoords_vector.at(i);
          output->GetPointData()->AddArray(new_tcoords);
          if(i == 0)
          {
            output->GetPointData()->SetActiveTCoords(new_tcoords->GetName());
          }
        }
      }
      if (hasNormals)
      {
        output->GetPointData()->SetNormals(new_normals);
      }

      // TODO: fixup for pointElems and lineElems too

      output->Squeeze();

      new_points->Delete();
      new_polys->Delete();
      for(unsigned int i = 0; i < new_tcoords_vector.size(); ++i)
      {
        vtkFloatArray* new_tcoords = new_tcoords_vector.at(i);
        new_tcoords->Delete();
      }
      new_normals->Delete();
    }
  }

  points->Delete();
  for(unsigned int i = 0; i < tcoords_vector.size(); ++i)
  {
    vtkFloatArray* tcoords = tcoords_vector.at(i);
    tcoords->Delete();
  }
  normals->Delete();
  polys->Delete();
  tcoord_polys->Delete();
  normal_polys->Delete();

  lineElems->Delete();
  pointElems->Delete();

  return 1;
}


void vtkOBJReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "File Name: "
     << (this->FileName ? this->FileName : "(none)") << "\n";

}


// ************************************************************************* //
