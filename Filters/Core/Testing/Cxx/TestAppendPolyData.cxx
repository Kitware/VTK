/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAppendPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkAppendPolyData.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkCellArray.h>

int TestAppendPolyData(int, char *[])
{
  vtkSmartPointer<vtkPoints> points1 = vtkSmartPointer<vtkPoints>::New();
  points1->InsertNextPoint(0,0,0);
  points1->InsertNextPoint(1,1,1);
  
  vtkSmartPointer<vtkPoints> points2 = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> vertices = vtkSmartPointer<vtkCellArray>::New();
  vtkIdType pid[1];
  pid[0] = points2->InsertNextPoint(5,5,5);
  vertices->InsertNextCell ( 1,pid );
  pid[0] = points2->InsertNextPoint(6,6,6);
  vertices->InsertNextCell ( 1,pid );
  
  vtkSmartPointer<vtkPolyData> polydata1 = vtkSmartPointer<vtkPolyData>::New();
  polydata1->SetPoints(points1);
  polydata1->SetVerts(vertices);
  /*
  cout << "polydata1" << endl;
  cout << "There are " << polydata1->GetNumberOfPoints() << " points." << endl;
  cout << "There are " << polydata1->GetNumberOfCells() << " cells." << endl;
  */
  vtkSmartPointer<vtkXMLPolyDataWriter> writer1 = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  writer1->SetFileName("polydata1.vtp");
  writer1->SetInput(polydata1);
  writer1->Write();
      
  vtkSmartPointer<vtkPolyData> polydata2 = vtkSmartPointer<vtkPolyData>::New();
  polydata2->SetPoints(points2);
  /*
  cout << "polydata2" << endl;
  cout << "There are " << polydata2->GetNumberOfPoints() << " points." << endl;
  cout << "There are " << polydata2->GetNumberOfCells() << " cells." << endl;
  */
  vtkSmartPointer<vtkXMLPolyDataWriter> writer2 = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  writer2->SetFileName("polydata2.vtp");
  writer2->SetInput(polydata2);
  writer2->Write();
  
  vtkSmartPointer<vtkAppendPolyData> appendFilter = vtkSmartPointer<vtkAppendPolyData>::New();
  appendFilter->AddInput(polydata1);
  appendFilter->AddInput(polydata2);
  appendFilter->Update();
  
  vtkPolyData* polydataCombined = appendFilter->GetOutput();
  vtkSmartPointer<vtkXMLPolyDataWriter> writerCombined = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  writerCombined->SetFileName("polydataCombined.vtp");
  writerCombined->SetInput(polydataCombined);
  writerCombined->Write();
  /*
  cout << "Combined" << endl;
  cout << "There are " << polydataCombined->GetNumberOfPoints() << " points." << endl;
  cout << "There are " << polydataCombined->GetNumberOfCells() << " cells." << endl;
  */
  if(polydataCombined->GetNumberOfPoints() != polydata1->GetNumberOfPoints() + polydata2->GetNumberOfPoints())
    {
    cerr << "The combined number of points is incorrect." << endl;
    return EXIT_FAILURE;
    }
    
  if(polydataCombined->GetNumberOfCells() != polydata1->GetNumberOfCells() + polydata2->GetNumberOfCells())
    {
    cerr << "The combined number of cells is incorrect." << endl;
    return EXIT_FAILURE;
    }
    
  return EXIT_SUCCESS;
}

