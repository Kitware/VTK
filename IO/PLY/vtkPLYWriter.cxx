/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLYWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPLYWriter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPLY.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkScalarsToColors.h"

#include <stddef.h>

vtkStandardNewMacro(vtkPLYWriter);

vtkCxxSetObjectMacro(vtkPLYWriter,LookupTable,vtkScalarsToColors);

vtkPLYWriter::vtkPLYWriter()
{
  this->FileName = NULL;
  this->FileType = VTK_BINARY;
  this->DataByteOrder = VTK_LITTLE_ENDIAN;
  this->ArrayName = NULL;
  this->Component = 0;
  this->ColorMode = VTK_COLOR_MODE_DEFAULT;
  this->LookupTable = NULL;
  this->Color[0] = this->Color[1] = this->Color[2] = 255;
}

vtkPLYWriter::~vtkPLYWriter()
{
  if ( this->LookupTable )
    {
    this->LookupTable->Delete();
    }
  if ( this->ArrayName )
    {
    delete [] this->ArrayName;
    }
  delete[] this->FileName;
}

typedef struct _plyVertex {
  float x[3];             // the usual 3-space position of a vertex
  unsigned char red;
  unsigned char green;
  unsigned char blue;
} plyVertex;

typedef struct _plyFace {
  unsigned char nverts;    // number of vertex indices in list
  int *verts;              // vertex index list
  unsigned char red;
  unsigned char green;
  unsigned char blue;
} plyFace;

void vtkPLYWriter::WriteData()
{
  vtkIdType i, j, idx;
  vtkPoints *inPts;
  vtkCellArray *polys;
  vtkPolyData *input = this->GetInput();

  unsigned char *cellColors, *pointColors;
  PlyFile *ply;
  float version;
  static const char *elemNames[] = { "vertex", "face" };
  static PlyProperty vertProps[] = { // property information for a vertex
    {"x", PLY_FLOAT, PLY_FLOAT, static_cast<int>(offsetof(plyVertex,x)),
     0, 0, 0, 0},
    {"y", PLY_FLOAT, PLY_FLOAT, static_cast<int>(offsetof(plyVertex,x)+sizeof(float)),
     0, 0, 0, 0},
    {"z", PLY_FLOAT, PLY_FLOAT, static_cast<int>(offsetof(plyVertex,x)+sizeof(float)+sizeof(float)),
     0, 0, 0, 0},
    {"red", PLY_UCHAR, PLY_UCHAR, static_cast<int>(offsetof(plyVertex,red)),
     0, 0, 0, 0},
    {"green", PLY_UCHAR, PLY_UCHAR,
     static_cast<int>(offsetof(plyVertex,green)), 0, 0, 0, 0},
    {"blue", PLY_UCHAR, PLY_UCHAR, static_cast<int>(offsetof(plyVertex,blue)),
     0, 0, 0, 0},
  };
  static PlyProperty faceProps[] = { // property information for a face
    {"vertex_indices", PLY_INT, PLY_INT,
     static_cast<int>(offsetof(plyFace,verts)),
     1, PLY_UCHAR, PLY_UCHAR, static_cast<int>(offsetof(plyFace,nverts))},
    {"red", PLY_UCHAR, PLY_UCHAR, static_cast<int>(offsetof(plyFace,red)),
     0, 0, 0, 0},
    {"green", PLY_UCHAR, PLY_UCHAR, static_cast<int>(offsetof(plyFace,green)),
     0, 0, 0, 0},
    {"blue", PLY_UCHAR, PLY_UCHAR, static_cast<int>(offsetof(plyFace,blue)),
     0, 0, 0, 0},
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
      ply = vtkPLY::ply_open_for_writing(this->FileName, 2, elemNames,
                                 PLY_BINARY_LE, &version);
      }
    else
      {
      ply = vtkPLY::ply_open_for_writing(this->FileName, 2, elemNames,
                                 PLY_BINARY_BE, &version);
      }
    }
  else
    {
    ply = vtkPLY::ply_open_for_writing(this->FileName, 2, elemNames,
                               PLY_ASCII, &version);
    }

  if ( ply == NULL)
    {
    vtkErrorMacro(<< "Error opening PLY file");
    return;
    }

  // compute colors, if any
  vtkIdType numPts = inPts->GetNumberOfPoints();
  vtkIdType numPolys = polys->GetNumberOfCells();
  pointColors = this->GetColors(numPts,input->GetPointData());
  cellColors = this->GetColors(numPolys,input->GetCellData());

  // describe what properties go into the vertex and face elements
  vtkPLY::ply_element_count (ply, "vertex", numPts);
  vtkPLY::ply_describe_property (ply, "vertex", &vertProps[0]);
  vtkPLY::ply_describe_property (ply, "vertex", &vertProps[1]);
  vtkPLY::ply_describe_property (ply, "vertex", &vertProps[2]);
  if ( pointColors )
    {
    vtkPLY::ply_describe_property (ply, "vertex", &vertProps[3]);
    vtkPLY::ply_describe_property (ply, "vertex", &vertProps[4]);
    vtkPLY::ply_describe_property (ply, "vertex", &vertProps[5]);
    }

  vtkPLY::ply_element_count (ply, "face", numPolys);
  vtkPLY::ply_describe_property (ply, "face", &faceProps[0]);
  if ( cellColors )
    {
    vtkPLY::ply_describe_property (ply, "face", &faceProps[1]);
    vtkPLY::ply_describe_property (ply, "face", &faceProps[2]);
    vtkPLY::ply_describe_property (ply, "face", &faceProps[3]);
    }

  // write a comment and an object information field
  vtkPLY::ply_put_comment (ply, "VTK generated PLY File");
  vtkPLY::ply_put_obj_info (ply, "vtkPolyData points and polygons: vtk4.0");

  // complete the header
  vtkPLY::ply_header_complete (ply);

  // set up and write the vertex elements
  plyVertex vert;
  vtkPLY::ply_put_element_setup (ply, "vertex");
  double dpoint[3];
  for (i = 0; i < numPts; i++)
    {
    inPts->GetPoint(i,dpoint);
    vert.x[0] = static_cast<float>(dpoint[0]);
    vert.x[1] = static_cast<float>(dpoint[1]);
    vert.x[2] = static_cast<float>(dpoint[2]);
    if ( pointColors )
      {
      idx = 3*i;
      vert.red = *(pointColors + idx);
      vert.green = *(pointColors + idx + 1);
      vert.blue = *(pointColors + idx + 2);
      }
    vtkPLY::ply_put_element (ply, (void *) &vert);
    }

  // set up and write the face elements
  plyFace face;
  int verts[256];
  face.verts = verts;
  vtkPLY::ply_put_element_setup (ply, "face");
  vtkIdType npts = 0;
  vtkIdType *pts = 0;
  for (polys->InitTraversal(), i = 0; i < numPolys; i++)
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
      if ( cellColors )
        {
        idx = 3*i;
        face.red = *(cellColors + idx);
        face.green = *(cellColors + idx + 1);
        face.blue = *(cellColors + idx + 2);
        }
      vtkPLY::ply_put_element (ply, (void *) &face);
      }
    }//for all polygons

  delete [] pointColors;
  delete [] cellColors;

  // close the PLY file
  vtkPLY::ply_close (ply);
}

unsigned char *vtkPLYWriter::GetColors(vtkIdType num,
                                       vtkDataSetAttributes *dsa)
{
  unsigned char *colors, *c;
  vtkIdType i;
  int numComp;

  if ( this->ColorMode == VTK_COLOR_MODE_OFF ||
       (this->ColorMode == VTK_COLOR_MODE_UNIFORM_CELL_COLOR &&
        vtkPointData::SafeDownCast(dsa) != NULL) ||
       (this->ColorMode == VTK_COLOR_MODE_UNIFORM_POINT_COLOR &&
        vtkCellData::SafeDownCast(dsa) != NULL) )
    {
    return NULL;
    }
  else if ( this->ColorMode == VTK_COLOR_MODE_UNIFORM_COLOR ||
    this->ColorMode == VTK_COLOR_MODE_UNIFORM_POINT_COLOR ||
    this->ColorMode == VTK_COLOR_MODE_UNIFORM_CELL_COLOR )
    {
    colors = c = new unsigned char[3*num];
    for (i=0; i<num; i++)
      {
      *c++ = this->Color[0];
      *c++ = this->Color[1];
      *c++ = this->Color[2];
      }
    return colors;
    }
  else //we will color based on data
    {
    double *tuple;
    vtkDataArray *da;
    unsigned char *rgb;
    vtkUnsignedCharArray *rgbArray;

    if ( !this->ArrayName || (da=dsa->GetArray(this->ArrayName)) == NULL ||
         this->Component >= (numComp=da->GetNumberOfComponents()) )
      {
      return NULL;
      }
    else if ( (rgbArray=vtkUnsignedCharArray::SafeDownCast(da)) != NULL &&
              numComp == 3 )
      {//have unsigned char array of three components, copy it
      colors = c = new unsigned char[3*num];
      rgb = rgbArray->GetPointer(0);
      for (i=0; i<num; i++)
        {
        *c++ = *rgb++;
        *c++ = *rgb++;
        *c++ = *rgb++;
        }
      return colors;
      }
    else if ( this->LookupTable != NULL )
      {//use the data array mapped through lookup table
      colors = c = new unsigned char[3*num];
      for (i=0; i<num; i++)
        {
        tuple = da->GetTuple(i);
        rgb = this->LookupTable->MapValue(tuple[this->Component]);
        *c++ = rgb[0];
        *c++ = rgb[1];
        *c++ = rgb[2];
        }
      return colors;
      }
    else //no lookup table
      {
      return NULL;
      }
    }
}

void vtkPLYWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Data Byte Order: ";
  if ( this->DataByteOrder == VTK_LITTLE_ENDIAN )
    {
    os << "Little Endian\n";
    }
  else
    {
    os << "Big Endian\n";
    }

  os << indent << "Color Mode: ";
  if ( this->ColorMode == VTK_COLOR_MODE_DEFAULT )
    {
    os << "Default\n";
    }
  else if ( this->ColorMode == VTK_COLOR_MODE_UNIFORM_CELL_COLOR )
    {
    os << "Uniform Cell Color\n";
    }
  else if ( this->ColorMode == VTK_COLOR_MODE_UNIFORM_POINT_COLOR )
    {
    os << "Uniform Point Color\n";
    }
  else if ( this->ColorMode == VTK_COLOR_MODE_UNIFORM_COLOR )
    {
    os << "Uniform Color\n";
    }
  else //VTK_COLOR_MODE_OFF
    {
    os << "Off\n";
    }

  os << indent << "Array Name: "
     << (this->ArrayName ? this->ArrayName : "(none)") << "\n";

  os << indent << "Component: " << this->Component << "\n";

  os << indent << "Lookup Table: " << this->LookupTable << "\n";

  os << indent << "Color: (" << this->Color[0] << ","
     << this->Color[1] << "," << this->Color[2] << ")\n";

}


vtkPolyData* vtkPLYWriter::GetInput()
{
  return vtkPolyData::SafeDownCast(this->Superclass::GetInput());
}

vtkPolyData* vtkPLYWriter::GetInput(int port)
{
  return vtkPolyData::SafeDownCast(this->Superclass::GetInput(port));
}

int vtkPLYWriter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}
