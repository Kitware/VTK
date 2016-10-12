/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGLWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkWebGLWidget.h"

#include "vtkActor2D.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkObjectFactory.h"
#include "vtkScalarBarActor.h"
#include "vtkWebGLExporter.h"
#include "vtkWebGLObject.h"

#include <sstream>

vtkStandardNewMacro(vtkWebGLWidget);

vtkWebGLWidget::vtkWebGLWidget()
{
  this->binaryData = NULL;
  this->iswidget = false;
  this->binarySize = 0;
  this->orientation = 1;
  this->interactAtServer = false;
  this->title = NULL;
}

vtkWebGLWidget::~vtkWebGLWidget()
{
  if (this->binaryData) delete[] this->binaryData;
  while(this->colors.size() !=0)
  {
    double* xrgb = this->colors.back();
    this->colors.pop_back();
    delete[] xrgb;
  }
  if (this->title) delete[] this->title;
}

unsigned char* vtkWebGLWidget::GetBinaryData(int vtkNotUsed(part))
{
  this->hasChanged = false;
  return this->binaryData;
}

int vtkWebGLWidget::GetBinarySize(int vtkNotUsed(part))
{
  return this->binarySize;
}

void vtkWebGLWidget::GenerateBinaryData()
{
  delete[] this->binaryData;
  std::string oldMD5 = this->MD5;

  size_t pos = 0;
  //Calculate the size used
  //NumOfColors, Type, Position, Size, Colors, Orientation, numberOfLabels
  int total = static_cast<int>(sizeof(int) + 1 + 4*sizeof(float) + this->colors.size()*(sizeof(float)+3*sizeof(char)) + 1 + 1 + strlen(this->title));
  this->binaryData = new unsigned char[total];
  int colorSize = static_cast<int>(this->colors.size());

  memset(this->binaryData, 0, total);                                                         //Fill array with 0
  memcpy(&this->binaryData[pos], &colorSize, sizeof(int)); pos+=sizeof(int);                  //Binary Data Size
  this->binaryData[pos++] = 'C';                                                              //Object Type
  memcpy(&this->binaryData[pos], &this->position, sizeof(float)*2); pos+=sizeof(float)*2;     //Position (double[2])
  memcpy(&this->binaryData[pos], &this->size, sizeof(float)*2); pos+=sizeof(float)*2;         //Size (double[2])
  unsigned char rgb[3];
  for(size_t i=0; i<colors.size(); i++)                                                       //Array of Colors (double, char[3])
  {
    float v = (float)this->colors[i][0];
    memcpy(&this->binaryData[pos], &v, sizeof(float)); pos+=sizeof(float);
    rgb[0] = (unsigned char)((int)(this->colors[i][1]*255));
    rgb[1] = (unsigned char)((int)(this->colors[i][2]*255));
    rgb[2] = (unsigned char)((int)(this->colors[i][3]*255));
    memcpy(&this->binaryData[pos], rgb, 3*sizeof(unsigned char)); pos+=sizeof(unsigned char)*3;
  }
  unsigned char aux; aux = (unsigned char)this->orientation;
  memcpy(&this->binaryData[pos], &aux, 1); pos++;
  aux = (unsigned char)this->numberOfLabels;
  memcpy(&this->binaryData[pos], &aux, 1); pos++;
  memcpy(&this->binaryData[pos], this->title, strlen(this->title)); pos+=strlen(this->title);

  this->binarySize = total;
  vtkWebGLExporter::ComputeMD5(this->binaryData, total, this->MD5);
  this->hasChanged = this->MD5.compare(oldMD5) != 0;
}

int vtkWebGLWidget::GetNumberOfParts()
{
  return 1;
}

void vtkWebGLWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkWebGLWidget::GetDataFromColorMap(vtkActor2D *actor)
{
  vtkScalarBarActor* scalarbar = vtkScalarBarActor::SafeDownCast(actor);
  this->numberOfLabels = scalarbar->GetNumberOfLabels();

  std::stringstream theTitle;
  char* componentTitle = scalarbar->GetComponentTitle();

  theTitle << scalarbar->GetTitle();
  if (componentTitle && strlen(componentTitle) > 0)
  {
    theTitle << " ";
    theTitle << componentTitle;
  }

  delete[] this->title;
  std::string tmp = theTitle.str();
  this->title = new char[tmp.length()+1];
  strcpy(this->title, tmp.c_str());
  this->hasTransparency = (scalarbar->GetUseOpacity() != 0);
  this->orientation = scalarbar->GetOrientation();

  //Colors
  vtkDiscretizableColorTransferFunction* lookup = vtkDiscretizableColorTransferFunction::SafeDownCast(scalarbar->GetLookupTable());
  int num = 5*lookup->GetSize();
  double* range = lookup->GetRange();
  double v, s; v = range[0]; s = (range[1]-range[0])/(num-1);
  for(int i=0; i<num; i++)
  {
    double* xrgb = new double[4];
    scalarbar->GetLookupTable()->GetColor(v, &xrgb[1]);
    xrgb[0] = v;
    this->colors.push_back(xrgb);
    v += s;
  }

  this->textFormat = scalarbar->GetLabelFormat();        //Float Format ex.: %-#6.3g
  this->textPosition = scalarbar->GetTextPosition();     //Orientacao dos textos; 1;
  double *pos = scalarbar->GetPosition();
  double *siz = scalarbar->GetPosition2();
  this->position[0] = pos[0]; this->position[1] = pos[1];//Widget Position
  this->size[0] = siz[0]; this->size[1] = siz[1];        //Widget Size
}
