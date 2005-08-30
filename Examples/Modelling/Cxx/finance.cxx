/*=========================================================================

  Program:   Visualization Toolkit
  Module:    finance.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"
#include "vtkAxes.h"
#include "vtkContourFilter.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkGaussianSplatter.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTubeFilter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkTestUtilities.h"

static vtkDataSet *ReadFinancialData(const char *fname, const char *x, const char *y, const char *z, const char *s);
static int ParseFile(FILE *file, const char *tag, float *data);

int main( int argc, char *argv[] )
{
  double bounds[6];
 
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/financial.txt");
  // read data
  vtkDataSet *dataSet = ReadFinancialData(fname, "MONTHLY_PAYMENT","INTEREST_RATE",
                              "LOAN_AMOUNT","TIME_LATE");
  delete[] fname;
  if ( ! dataSet ) exit(0);

  // construct pipeline for original population
  vtkGaussianSplatter *popSplatter = vtkGaussianSplatter::New();
    popSplatter->SetInput(dataSet);
    popSplatter->SetSampleDimensions(50,50,50);
    popSplatter->SetRadius(0.05);
    popSplatter->ScalarWarpingOff();
  vtkContourFilter *popSurface = vtkContourFilter::New();
    popSurface->SetInputConnection(popSplatter->GetOutputPort());
    popSurface->SetValue(0,0.01);
  vtkPolyDataMapper *popMapper = vtkPolyDataMapper::New();
    popMapper->SetInputConnection(popSurface->GetOutputPort());
    popMapper->ScalarVisibilityOff();
  vtkActor *popActor = vtkActor::New();
    popActor->SetMapper(popMapper);
    popActor->GetProperty()->SetOpacity(0.3);
    popActor->GetProperty()->SetColor(.9,.9,.9);

  // construct pipeline for delinquent population
  vtkGaussianSplatter *lateSplatter = vtkGaussianSplatter::New();
    lateSplatter->SetInput(dataSet);
    lateSplatter->SetSampleDimensions(50,50,50);
    lateSplatter->SetRadius(0.05);
    lateSplatter->SetScaleFactor(0.005);
  vtkContourFilter *lateSurface = vtkContourFilter::New();
    lateSurface->SetInputConnection(lateSplatter->GetOutputPort());
    lateSurface->SetValue(0,0.01);
  vtkPolyDataMapper *lateMapper = vtkPolyDataMapper::New();
    lateMapper->SetInputConnection(lateSurface->GetOutputPort());
    lateMapper->ScalarVisibilityOff();
  vtkActor *lateActor = vtkActor::New();
    lateActor->SetMapper(lateMapper);
    lateActor->GetProperty()->SetColor(1.0,0.0,0.0);

  // create axes
  popSplatter->Update();
  popSplatter->GetOutput()->GetBounds(bounds);
  vtkAxes *axes = vtkAxes::New();
    axes->SetOrigin(bounds[0], bounds[2], bounds[4]);
    axes->SetScaleFactor(popSplatter->GetOutput()->GetLength()/5);
  vtkTubeFilter *axesTubes = vtkTubeFilter::New();
    axesTubes->SetInputConnection(axes->GetOutputPort());
    axesTubes->SetRadius(axes->GetScaleFactor()/25.0);
    axesTubes->SetNumberOfSides(6);
  vtkPolyDataMapper *axesMapper = vtkPolyDataMapper::New();
    axesMapper->SetInputConnection(axesTubes->GetOutputPort());
  vtkActor *axesActor = vtkActor::New();
    axesActor->SetMapper(axesMapper);

  // graphics stuff
  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  // read data  //set up renderer
  renderer->AddActor(lateActor);
  renderer->AddActor(axesActor);
  renderer->AddActor(popActor);
  renderer->SetBackground(1,1,1);
  renWin->SetSize(300,300);

  // interact with data
  iren->Initialize();

  renWin->Render();
  iren->Start();

  // Clean up
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  popSplatter->Delete();
  popSurface->Delete();
  popMapper->Delete();
  popActor->Delete();
  lateSplatter->Delete();
  lateSurface->Delete();
  lateMapper->Delete();
  lateActor->Delete();
  axes->Delete();
  axesTubes->Delete();
  axesMapper->Delete();
  axesActor->Delete();
  dataSet->Delete();

  return 0;
}

static vtkDataSet *ReadFinancialData(const char* filename, const char *x, const char *y, const char *z, const char *s)
{
  float xyz[3];
  FILE *file;
  int i, npts;
  char tag[80];
 
  if ( (file = fopen(filename,"r")) == 0 )
    {
    cerr << "ERROR: Can't read file: " << filename << "\n";
    return NULL;
    }
  
  fscanf (file, "%s %d", tag, &npts); // read number of points
  
  vtkUnstructuredGrid *dataSet = vtkUnstructuredGrid::New();
  float *xV = new float[npts];
  float *yV = new float[npts];
  float *zV = new float[npts];
  float *sV = new float[npts];

  if ( ! ParseFile(file, x, xV) || ! ParseFile(file, y, yV) ||
  ! ParseFile(file, z, zV) || ! ParseFile(file, s, sV) )
    {
    cerr << "Couldn't read data!\n";
    delete [] xV;
    delete [] yV;
    delete [] zV;
    delete [] sV;
    dataSet->Delete();
    return NULL;
    }

  vtkPoints *newPts = vtkPoints::New();
  vtkFloatArray *newScalars = vtkFloatArray::New();

  for (i=0; i<npts; i++)
    {
    xyz[0] = xV[i]; xyz[1] = yV[i]; xyz[2] = zV[i]; 
    newPts->InsertPoint(i, xyz);
    newScalars->InsertValue(i, sV[i]);
    }

  dataSet->SetPoints(newPts);
  dataSet->GetPointData()->SetScalars(newScalars);

  newPts->Delete(); //reference counted - it's okay
  newScalars->Delete();

  return dataSet;
}

static int ParseFile(FILE *file, const char *label, float *data)
{
  char tag[80];
  int i, npts, readData=0;
  float min=VTK_LARGE_FLOAT;
  float max=(-VTK_LARGE_FLOAT);

  if ( file == NULL || label == NULL ) return 0;

  rewind(file);
  
  fscanf(file, "%s %d", tag, &npts);
  
  while ( !readData && fscanf(file, "%s", tag) == 1 )
    {
    if ( ! strcmp(tag,label) )
      {
      readData = 1;
      for (i=0; i<npts; i++) 
        {
        fscanf(file, "%f", data+i);
        if ( data[i] < min ) min = data[i];
        if ( data[i] > min ) max = data[i];
        }
      // normalize data
      for (i=0; i<npts; i++) data[i] = min + data[i]/(max-min);
      }
    else
      {
      for (i=0; i<npts; i++) fscanf(file, "%*f");
      }
    }

  if ( ! readData ) return 0;
  else return 1;
}
