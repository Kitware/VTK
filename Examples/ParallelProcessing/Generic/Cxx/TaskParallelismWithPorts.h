/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TaskParallelismWithPorts.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __TASKPARA_H
#define __TASKPARA_H

#include "vtkMultiProcessController.h"
#include "vtkRTAnalyticSource.h"
#include "vtkFieldDataToAttributeDataFilter.h"
#include "vtkAttributeDataToFieldDataFilter.h"
#include "vtkImageShrink3D.h"
#include "vtkGlyph3D.h"
#include "vtkGlyphSource2D.h"
#include "vtkImageGradient.h"
#include "vtkImageGradientMagnitude.h"
#include "vtkImageGaussianSmooth.h"
#include "vtkProbeFilter.h"
#include "vtkDataSetMapper.h"
#include "vtkContourFilter.h"
#include "vtkActor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkAssignAttribute.h"


typedef void (*taskFunction)(double data);


void task3(double data);
void task4(double data);

  
static const double EXTENT = 20;

static const int WINDOW_WIDTH = 400; 
static const int WINDOW_HEIGHT = 300; 

#endif
