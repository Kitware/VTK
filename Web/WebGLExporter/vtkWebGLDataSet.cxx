/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGLDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkWebGLDataSet.h"

#include "vtkObjectFactory.h"
#include "vtkWebGLExporter.h"

vtkStandardNewMacro(vtkWebGLDataSet);

std::string vtkWebGLDataSet::GetMD5()
  {
  return this->MD5;
  }

vtkWebGLDataSet::vtkWebGLDataSet()
  {
  this->NumberOfVertices = 0;
  this->NumberOfPoints = 0;
  this->NumberOfIndexes = 0;
  this->vertices = NULL;
  this->normals = NULL;
  this->indexes = NULL;
  this->points = NULL;
  this->tcoords = NULL;
  this->colors = NULL;
  this->binary = NULL;
  this->binarySize = 0;
  this->hasChanged = false;
  }

vtkWebGLDataSet::~vtkWebGLDataSet()
  {
  if (this->vertices)
    {
    delete[] this->vertices;
    }
  if (this->normals)
    {
    delete[] this->normals;
    }
  if (this->indexes)
    {
    delete[] this->indexes;
    }
  if (this->points)
    {
    delete[] this->points;
    }
  if (this->tcoords)
    {
    delete[] this->tcoords;
    }
  if (this->colors)
    {
    delete[] this->colors;
    }
  if (this->binary)
    {
    delete[] this->binary;
    }
  }

void vtkWebGLDataSet::SetVertices(float* v, int size)
  {
  if (this->vertices) delete[] this->vertices;
  this->vertices = v;
  this->NumberOfVertices = size;
  this->webGLType = wTRIANGLES;
  this->hasChanged = true;
  }

void vtkWebGLDataSet::SetIndexes(short* i, int size)
  {
  if (this->indexes) delete[] this->indexes;
  this->indexes = i;
  this->NumberOfIndexes = size;
  this->hasChanged = true;
  }

void vtkWebGLDataSet::SetNormals(float* n)
  {
  if (this->normals) delete[] this->normals;
  this->normals = n;
  this->hasChanged = true;
  }

void vtkWebGLDataSet::SetColors(unsigned char* c)
  {
  if (this->colors) delete[] this->colors;
  this->colors = c;
  this->hasChanged = true;
  }

void vtkWebGLDataSet::SetPoints(float* p, int size)
  {
  if (this->points) delete[] this->points;
  this->points = p;
  this->NumberOfPoints = size;
  this->webGLType = wLINES;
  this->hasChanged = true;
  }

void vtkWebGLDataSet::SetTCoords(float *t)
  {
  if (this->tcoords) delete[] this->tcoords;
  this->tcoords = t;
  this->hasChanged = true;
  }

unsigned char* vtkWebGLDataSet::GetBinaryData()
  {
  this->hasChanged = false;
  return this->binary;
  }

int vtkWebGLDataSet::GetBinarySize()
  {
  return this->binarySize;
  }

void vtkWebGLDataSet::SetMatrix(float* m)
  {
  this->Matrix = m;
  this->hasChanged = true;
  }

void vtkWebGLDataSet::GenerateBinaryData()
  {
  if (this->NumberOfIndexes == 0 && this->webGLType != wPOINTS)
    {
    return;
    }
  int size=0, pos=0, total=0;
  if (this->binary)
    {
    delete[] this->binary;
    }
  this->binarySize = 0;

  if(this->webGLType == wLINES)
    {
    pos = sizeof(pos);
    size = this->NumberOfPoints*sizeof(this->points[0]);

    //Calculate the size used by each data
    total = sizeof(pos) + 1 + sizeof(this->NumberOfPoints) + size*3                     //Size, Type, NumberOfPoints, Points
        + sizeof(this->colors[0])*this->NumberOfPoints*4 + sizeof(this->NumberOfIndexes) //Color, NumberOfIndex
        + this->NumberOfIndexes*sizeof(this->indexes[0]) + sizeof(this->Matrix[0])*16;   //Index, Matrix
    this->binary = new unsigned char[total];
    memset(this->binary,0,total);

    this->binary[pos++] = 'L';
    memcpy(&this->binary[pos], &this->NumberOfPoints, sizeof(this->NumberOfPoints)); pos+=sizeof(this->NumberOfPoints); //Points
    memcpy(&this->binary[pos], this->points, size*3); pos+=size*3;
    memcpy(&this->binary[pos], this->colors, sizeof(this->colors[0])*this->NumberOfPoints*4); pos+=sizeof(this->colors[0])*this->NumberOfPoints*4;
    memcpy(&this->binary[pos], &this->NumberOfIndexes, sizeof(this->NumberOfIndexes)); pos+=sizeof(this->NumberOfIndexes);
    memcpy(&this->binary[pos], this->indexes, this->NumberOfIndexes*sizeof(this->indexes[0])); pos+=this->NumberOfIndexes*sizeof(this->indexes[0]);
    memcpy(&this->binary[pos], this->Matrix, sizeof(this->Matrix[0])*16); pos+=sizeof(this->Matrix[0])*16;              //Matrix

    memcpy(&this->binary[0], &pos, sizeof(pos));
    this->binarySize = total;
    }
  else if (this->webGLType == wTRIANGLES)
    {
    pos = sizeof(pos);
    size = sizeof(this->vertices[0])*this->NumberOfVertices;

    //Calculate the size used by each data
    total = sizeof(pos) + 1 + sizeof(this->NumberOfVertices) + size*(3+3)                   //Size, Type, VertCount, Vert, Normal
        + sizeof(this->colors[0])*this->NumberOfVertices*4 + sizeof(this->NumberOfIndexes)  //Color, IndicCount
        + this->NumberOfIndexes*sizeof(this->indexes[0]) + sizeof(this->Matrix[0])*16;      //Index, Matrix
    if (this->tcoords) total += size*2;                                                     //TCoord
    this->binary = new unsigned char[total];
    memset(this->binary,0,total);

    this->binary[pos++] = 'M';
    memcpy(&this->binary[pos], &this->NumberOfVertices, sizeof(this->NumberOfVertices)); pos+=sizeof(this->NumberOfVertices); //VertCount
    memcpy(&this->binary[pos], this->vertices, size*3); pos+=size*3;                                                          //Vertices
    memcpy(&this->binary[pos], this->normals, size*3); pos+=size*3;                                                           //Normals
    memcpy(&this->binary[pos], this->colors, sizeof(this->colors[0])*this->NumberOfVertices*4); pos+=sizeof(this->colors[0])*this->NumberOfVertices*4;//Colors
    memcpy(&this->binary[pos], &this->NumberOfIndexes, sizeof(this->NumberOfIndexes)); pos+=sizeof(this->NumberOfIndexes);    //IndCount
    memcpy(&this->binary[pos], this->indexes, this->NumberOfIndexes*sizeof(this->indexes[0])); pos+=this->NumberOfIndexes*sizeof(this->indexes[0]);
    memcpy(&this->binary[pos], this->Matrix, sizeof(this->Matrix[0])*16); pos+=sizeof(this->Matrix[0])*16;                    //Matrix
    if (this->tcoords) memcpy(&this->binary[pos], this->tcoords, size*2); pos+=size*2;                                        //TCoord

    memcpy(&this->binary[0], &pos, sizeof(pos));
    this->binarySize = total;
    }
  else if (this->webGLType == wPOINTS)
    {
    pos = sizeof(pos);
    size = this->NumberOfPoints*sizeof(this->points[0]);

    //Calculate the size used by each data
    total = sizeof(pos) + 1 + sizeof(this->NumberOfPoints) + size*3                    //Size, Type, NumberOfPoints, Points
        + sizeof(this->colors[0])*this->NumberOfPoints*4 + sizeof(this->Matrix[0])*16; //Color, Matrix
    this->binary = new unsigned char[total];
    memset(this->binary,0,total);

    this->binary[pos++] = 'P';
    memcpy(&this->binary[pos], &this->NumberOfPoints, sizeof(this->NumberOfPoints)); pos+=sizeof(this->NumberOfPoints); //Points
    memcpy(&this->binary[pos], this->points, size*3); pos+=size*3;
    memcpy(&this->binary[pos], this->colors, sizeof(this->colors[0])*this->NumberOfPoints*4); pos+=sizeof(this->colors[0])*this->NumberOfPoints*4;
    memcpy(&this->binary[pos], this->Matrix, sizeof(this->Matrix[0])*16); pos+=sizeof(this->Matrix[0])*16;              //Matrix

    memcpy(&this->binary[0], &pos, sizeof(pos));
    this->binarySize = total;
    }
  vtkWebGLExporter::ComputeMD5((unsigned char*)this->binary, this->binarySize, this->MD5);
  this->hasChanged = true;
  }

void vtkWebGLDataSet::SetType(WebGLObjectTypes t)
  {
  this->webGLType = t;
  }

bool vtkWebGLDataSet::HasChanged()
  {
  return this->hasChanged;
  }

void vtkWebGLDataSet::PrintSelf(ostream& os, vtkIndent indent)
  {
  this->Superclass::PrintSelf(os, indent);
  }
