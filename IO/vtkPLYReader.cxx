/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLYReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPLYReader.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPLY.h"
#include "vtkPolyData.h"

#include <ctype.h>
#include <stddef.h>

vtkStandardNewMacro(vtkPLYReader);


// Construct object with merging set to true.
vtkPLYReader::vtkPLYReader()
{
  this->FileName = NULL;

  this->SetNumberOfInputPorts(0);
}

vtkPLYReader::~vtkPLYReader()
{
  delete [] this->FileName;
}

typedef struct _plyVertex {
  float x[3];             // the usual 3-space position of a vertex
  unsigned char red;
  unsigned char green;
  unsigned char blue;
} plyVertex;

typedef struct _plyFace {
  unsigned char intensity; // optional face attributes
  unsigned char red;
  unsigned char green;
  unsigned char blue;
  unsigned char nverts;   // number of vertex indices in list
  int *verts;             // vertex index list
} plyFace;

int vtkPLYReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  PlyProperty vertProps[] = {
    {"x", PLY_FLOAT, PLY_FLOAT, static_cast<int>(offsetof(plyVertex,x)),
     0, 0, 0, 0},
    {"y", PLY_FLOAT, PLY_FLOAT, static_cast<int>(offsetof(plyVertex,x)+sizeof(float)),
     0, 0, 0, 0},
    {"z", PLY_FLOAT, PLY_FLOAT, static_cast<int>(offsetof(plyVertex,x)+sizeof(float)+sizeof(float)),
     0, 0, 0, 0},
    {"red", PLY_UCHAR, PLY_UCHAR, static_cast<int>(offsetof(plyVertex,red)), 0, 0, 0, 0},
    {"green", PLY_UCHAR, PLY_UCHAR, static_cast<int>(offsetof(plyVertex,green)), 0, 0, 0, 0},
    {"blue", PLY_UCHAR, PLY_UCHAR, static_cast<int>(offsetof(plyVertex,blue)), 0, 0, 0, 0},
  };
  PlyProperty faceProps[] = {
    {"vertex_indices", PLY_INT, PLY_INT,
     static_cast<int>(offsetof(plyFace,verts)),
     1, PLY_UCHAR, PLY_UCHAR, static_cast<int>(offsetof(plyFace,nverts))},
    {"intensity", PLY_UCHAR, PLY_UCHAR, 
     static_cast<int>(offsetof(plyFace,intensity)), 0, 0, 0, 0},
    {"red", PLY_UCHAR, PLY_UCHAR, static_cast<int>(offsetof(plyFace,red)), 0, 0, 0, 0},
    {"green", PLY_UCHAR, PLY_UCHAR, static_cast<int>(offsetof(plyFace,green)), 0, 0, 0, 0},
    {"blue", PLY_UCHAR, PLY_UCHAR, static_cast<int>(offsetof(plyFace,blue)), 0, 0, 0, 0},
  };

  if (!this->FileName)
    {
    vtkErrorMacro(<<"A File Name must be specified.");
    return 0;
    }

  // open a PLY file for reading
  PlyFile *ply;
  int nelems, fileType, numElems, nprops;
  char **elist, *elemName;
  float version;
  
  if ( !(ply = vtkPLY::ply_open_for_reading(this->FileName, &nelems, &elist,
                                            &fileType, &version)) )
    {
    vtkWarningMacro(<<"Could not open PLY file");
    return 0;
    }

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

  // Check for optional attribute data. We can handle intensity; and the
  // triplet red, green, blue.
  bool intensityAvailable=false;
  vtkUnsignedCharArray *intensity=NULL;
  if ( (elem = vtkPLY::find_element (ply, "face")) != NULL &&
       vtkPLY::find_property (elem, "intensity", &index) != NULL )
    {
    intensity = vtkUnsignedCharArray::New();
    intensity->SetName("intensity");
    intensityAvailable = true;
    output->GetCellData()->AddArray(intensity);
    output->GetCellData()->SetActiveScalars("intensity");
    intensity->Delete();
    }

  bool RGBCellsAvailable=false;
  vtkUnsignedCharArray *RGBCells=NULL;
  if ( (elem = vtkPLY::find_element (ply, "face")) != NULL &&
       vtkPLY::find_property (elem, "red", &index) != NULL &&
       vtkPLY::find_property (elem, "green", &index) != NULL &&
       vtkPLY::find_property (elem, "blue", &index) != NULL )
    {
    RGBCells = vtkUnsignedCharArray::New();
    RGBCells->SetName("RGB");
    RGBCellsAvailable = true;
    output->GetCellData()->AddArray(RGBCells);
    output->GetCellData()->SetActiveScalars("RGB");
    RGBCells->Delete();
    }

  bool RGBPointsAvailable=false;
  vtkUnsignedCharArray *RGBPoints=NULL;
  if ( (elem = vtkPLY::find_element (ply, "vertex")) != NULL &&
       vtkPLY::find_property (elem, "red", &index) != NULL &&
       vtkPLY::find_property (elem, "green", &index) != NULL &&
       vtkPLY::find_property (elem, "blue", &index) != NULL )
    {
    RGBPoints = vtkUnsignedCharArray::New();
    RGBPointsAvailable = true;
    RGBPoints->SetName("RGB");
    output->GetPointData()->SetScalars(RGBPoints);
    RGBPoints->Delete();
    }

  // Okay, now we can grab the data
  int numPts=0, numPolys=0;
  for (int i = 0; i < nelems; i++)
    {
    //get the description of the first element */
    elemName = elist[i];
    vtkPLY::ply_get_element_description (ply, elemName, &numElems, &nprops);

    // if we're on vertex elements, read them in
    if ( elemName && !strcmp ("vertex", elemName) )
      {
      // Create a list of points
      numPts = numElems;
      vtkPoints *pts = vtkPoints::New();
      pts->SetDataTypeToFloat();
      pts->SetNumberOfPoints(numPts);
      
      // Setup to read the PLY elements
      vtkPLY::ply_get_property (ply, elemName, &vertProps[0]);
      vtkPLY::ply_get_property (ply, elemName, &vertProps[1]);
      vtkPLY::ply_get_property (ply, elemName, &vertProps[2]);

      if ( RGBPointsAvailable )
        {
        vtkPLY::ply_get_property (ply, elemName, &vertProps[3]);
        vtkPLY::ply_get_property (ply, elemName, &vertProps[4]);
        vtkPLY::ply_get_property (ply, elemName, &vertProps[5]);
        RGBPoints->SetNumberOfComponents(3);
        RGBPoints->SetNumberOfTuples(numPts);
        }
      plyVertex vertex;
      for (int j=0; j < numPts; j++)
        {
        vtkPLY::ply_get_element (ply, (void *) &vertex);
        pts->SetPoint (j, vertex.x);
        if ( RGBPointsAvailable )
          {
          RGBPoints->SetTuple3(j,vertex.red,vertex.green,vertex.blue);
          }
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
      if ( intensityAvailable )
        {
        vtkPLY::ply_get_property (ply, elemName, &faceProps[1]);
        RGBCells->SetNumberOfComponents(1);
        RGBCells->SetNumberOfTuples(numPolys);
        }
      if ( RGBCellsAvailable )
        {
        vtkPLY::ply_get_property (ply, elemName, &faceProps[2]);
        vtkPLY::ply_get_property (ply, elemName, &faceProps[3]);
        vtkPLY::ply_get_property (ply, elemName, &faceProps[4]);
        RGBCells->SetNumberOfComponents(3);
        RGBCells->SetNumberOfTuples(numPolys);
        }
      
      // grab all the face elements
      for (int j=0; j < numPolys; j++)
        {
        //grab and element from the file
        face.verts = verts;
        vtkPLY::ply_get_element (ply, (void *) &face);
        for (int k=0; k<face.nverts; k++)
          {
          vtkVerts[k] = face.verts[k];
          }
        polys->InsertNextCell(face.nverts,vtkVerts);
        if ( intensityAvailable )
          {
          intensity->SetValue(j,face.intensity);
          }
        if ( RGBCellsAvailable )
          {
          RGBCells->SetValue(3*j,face.red);
          RGBCells->SetValue(3*j+1,face.green);
          RGBCells->SetValue(3*j+2,face.blue);
          }
        }
      output->SetPolys(polys);
      polys->Delete();
      }//if face

    free(elist[i]); //allocated by ply_open_for_reading
    elist[i] = NULL;
    
    }//for all elements of the PLY file
  free(elist); //allocated by ply_open_for_reading
  
  vtkDebugMacro( <<"Read: " << numPts << " points, "
                 << numPolys << " polygons");

  // close the PLY file 
  vtkPLY::ply_close (ply);

  return 1;
}

int vtkPLYReader::CanReadFile(const char *filename)
{
  FILE *fd = fopen(filename, "rb");
  if (!fd) return 0;

  char line[4] = {};
  const char *result = fgets(line, sizeof(line), fd);
  fclose(fd);
  return (result && strncmp(result, "ply", 3) == 0);
}

void vtkPLYReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "File Name: "
     << (this->FileName ? this->FileName : "(none)") << "\n";

}
