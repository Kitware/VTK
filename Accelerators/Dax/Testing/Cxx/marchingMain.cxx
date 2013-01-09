//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkMarchingCubes.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLImageDataReader.h>


namespace
{

template<typename T>
void RunVTKPipeline(t *t)
{
  vtkNew<vtkDaxMarchingCubes> cubes;
  cubes->SetInputConnection(t->GetOutputPort());
  cubes->SetNumberOfContours(1);
  cubes->SetValue(0,10);
  cubes->Update();
}

} // Anonymous namespace



int main(int argc, char* argv[])
  {
  //create the sample grid
  vtkImageMandelbrotSource *src = vtkImageMandelbrotSource::New();
  src->SetWholeExtent(0,40,0,40,0,40);

  //run the pipeline
  RunVTKPipeline(src);
  }
