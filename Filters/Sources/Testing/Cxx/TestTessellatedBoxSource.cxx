/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTessellatedBoxSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "vtkTessellatedBoxSource.h"
#include "vtkXMLPolyDataWriter.h"

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkXMLHierarchicalBoxDataReader.h"
#include "vtkStructuredPoints.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkVolumeProperty.h"
#include "vtkVolume.h"
#include "vtkContourFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkRegressionTestImage.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkTestUtilities.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkUniformGrid.h"
#include "vtkOutlineFilter.h"
#include "vtkLookupTable.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkCellDataToPointData.h"
#include "vtkClipConvexPolyData.h"
#include "vtkPlaneCollection.h"
#include "vtkPlane.h"

int TestTessellatedBoxSource(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  vtkTessellatedBoxSource *boxSource=vtkTessellatedBoxSource::New();
  boxSource->SetBounds(0,1,0,1,0,1);
  boxSource->QuadsOn();
  boxSource->SetLevel(4);
  boxSource->Update();
  vtkXMLPolyDataWriter *writer=vtkXMLPolyDataWriter::New();
  writer->SetInputConnection(boxSource->GetOutputPort());
  boxSource->Delete();
  writer->SetFileName("box.vtp");
  writer->SetDataModeToAscii();
  writer->Update();
  
  vtkClipConvexPolyData *clip=vtkClipConvexPolyData::New();
  clip->SetInputConnection(boxSource->GetOutputPort());
  
  vtkPlaneCollection *planes=vtkPlaneCollection::New();
  clip->SetPlanes(planes);
  planes->Delete();
  
  vtkPlane *p=vtkPlane::New();
  planes->AddItem(p);
  p->Delete();
  
  double origin[3]={0.5,0.5,0.5};
  double direction[3]={0,0,1};
  
  p->SetOrigin( origin );
  p->SetNormal( direction );
  planes->AddItem(p);
  
  vtkXMLPolyDataWriter *writer2=vtkXMLPolyDataWriter::New();
  writer2->SetInputConnection(clip->GetOutputPort());
  clip->Delete();
  writer2->SetFileName("clipbox.vtp");
  writer2->SetDataModeToAscii();
  writer2->Update();
  writer2->Delete();
  
  writer->Delete();  

  return 0; // 0==success.
}
