/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGradientAndVorticity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkGradientFilter.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridReader.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

#include <vtkstd/vector>

#define VTK_CREATE(type, var)                                   \
  vtkSmartPointer<type> var = vtkSmartPointer<type>::New()

namespace
{
  double Tolerance = 0.00001;
  
  bool ArePointsWithinTolerance(double v1, double v2)
  {
    if(v1 == v2)
      {
      return true;
      }
    if(v1 == 0.0)
      {
      if(fabs(v2) < Tolerance)
        {
        return true;
        }
      cout << fabs(v2) << " (fabs(v2)) should be less than " 
           << Tolerance << endl;
      return false;
      }
    if(fabs(v1/v2) < Tolerance)
      {
        return true;
        }
    cout << fabs(v1/v2) << " (fabs(v1/v2)) should be less than " 
         << Tolerance << endl;
    return false;
  }

//-----------------------------------------------------------------------------
  void CreateCellData(vtkDataSet* Grid, int NumberOfComponents, int Offset,
                      const char* ArrayName)
  {
    vtkIdType NumberOfCells = Grid->GetNumberOfCells();
    VTK_CREATE(vtkDoubleArray, Array);
    Array->SetNumberOfComponents(NumberOfComponents);
    Array->SetNumberOfTuples(NumberOfCells);
    vtkstd::vector<double> TupleValues(NumberOfComponents);
    double Point[3], ParametricCenter[3], weights[100];
    for(vtkIdType i=0;i<NumberOfCells;i++)
      {
      vtkCell* Cell = Grid->GetCell(i);
      Cell->GetParametricCenter(ParametricCenter);
      int subId = 0;
      Cell->EvaluateLocation(subId, ParametricCenter, Point, weights);
      for(int j=0;j<NumberOfComponents;j++)
        {// +offset makes the curl/vorticity nonzero
        TupleValues[j] = Point[(j+Offset)%3]; 
        }
      Array->SetTupleValue(i, &TupleValues[0]);
      }
    Array->SetName(ArrayName);
    Grid->GetCellData()->AddArray(Array);
  }

//-----------------------------------------------------------------------------
  void CreatePointData(vtkDataSet* Grid, int NumberOfComponents, int Offset,
                       const char* ArrayName)
  {
    vtkIdType NumberOfPoints = Grid->GetNumberOfPoints();
    VTK_CREATE(vtkDoubleArray, Array);
    Array->SetNumberOfComponents(NumberOfComponents);
    Array->SetNumberOfTuples(NumberOfPoints);
    vtkstd::vector<double> TupleValues(NumberOfComponents);
    double Point[3];
    for(vtkIdType i=0;i<NumberOfPoints;i++)
      {
      Grid->GetPoint(i, Point);
      for(int j=0;j<NumberOfComponents;j++)
        {// +offset makes the curl/vorticity nonzero
        TupleValues[j] = Point[(j+Offset)%3]; 
        }
      Array->SetTupleValue(i, &TupleValues[0]);
      }
    Array->SetName(ArrayName);
    Grid->GetPointData()->AddArray(Array);
  }
  
//-----------------------------------------------------------------------------
  int IsGradientCorrect(vtkDoubleArray* Gradients, int Offset)
  {
    int NumberOfComponents = Gradients->GetNumberOfComponents();
    for(vtkIdType i=0;i<Gradients->GetNumberOfTuples();i++)
      {
      double* Values = Gradients->GetTuple(i);
      for(int OrigComp=0;OrigComp<NumberOfComponents/3;OrigComp++)
        {
        for(int GradDir=0;GradDir<3;GradDir++)
          {
          if((OrigComp-GradDir+Offset)%3 == 0)
            {
            if(fabs(Values[OrigComp*3+GradDir]-1.) > Tolerance)
              {
              vtkGenericWarningMacro("Gradient value should be one but is "
                                     << Values[OrigComp*3+GradDir]);
              return 0;
              }
            }
          else if(fabs(Values[OrigComp*3+GradDir]) > Tolerance)
            {
            vtkGenericWarningMacro("Gradient value should be zero but is "
                                   << Values[OrigComp*3+GradDir]);
            return 0;
            }
          }
        }
      }
    return 1;
  }
  
//-----------------------------------------------------------------------------
// we assume that the gradients are correct and so we can compute the "real"
// vorticity from it
  int IsVorticityCorrect(vtkDoubleArray* Gradients, vtkDoubleArray* Vorticity)
  {
    if(Gradients->GetNumberOfComponents() != 9 || 
       Vorticity->GetNumberOfComponents() != 3)
      {
      vtkGenericWarningMacro("Bad number of components.");
      return 0;
      }
    for(vtkIdType i=0;i<Gradients->GetNumberOfTuples();i++)
      {
      double* g = Gradients->GetTuple(i);
      double* v = Vorticity->GetTuple(i);
      if(!ArePointsWithinTolerance(v[0], g[7]-g[5]))
        {
        vtkGenericWarningMacro("Bad vorticity[0] value " << v[0] << " " << 
                               g[7]-g[5] << " difference is " << (v[0]-g[7]+g[5]));
        return 0;
        }
      else if(!ArePointsWithinTolerance(v[1], g[2]-g[6]))
        {
        vtkGenericWarningMacro("Bad vorticity[1] value " << v[1] << " " << 
                               g[2]-g[6] << " difference is " << (v[1]-g[2]+g[6]));
        return 0;
        }
      else if(!ArePointsWithinTolerance(v[2], g[3]-g[1]))
        {
        vtkGenericWarningMacro("Bad vorticity[2] value " << v[2] << " " << 
                               g[3]-g[1] << " difference is " << (v[2]-g[3]+g[1]));
        return 0;
        }
      }
    
    return 1;
  }

//-----------------------------------------------------------------------------
  int PerformTest(vtkDataSet* Grid)
  {
    // Cleaning out the existing field data so that I can replace it with 
    // an analytic function that I know the gradient of
    Grid->GetPointData()->Initialize();
    Grid->GetCellData()->Initialize();
    const char FieldName[] = "LinearField";
    int Offset = 1;
    int NumberOfComponents = 3;
    CreateCellData(Grid, NumberOfComponents, Offset, FieldName);
    CreatePointData(Grid, NumberOfComponents, Offset, FieldName);
    
    VTK_CREATE(vtkGradientFilter, CellGradients);
    CellGradients->SetInput(Grid);
    CellGradients->SetInputScalars(
      vtkDataObject::FIELD_ASSOCIATION_CELLS, FieldName);
    const char ResultName[] = "Result";
    CellGradients->SetResultArrayName(ResultName);
    
    VTK_CREATE(vtkGradientFilter, PointGradients);
    PointGradients->SetInput(Grid);
    PointGradients->SetInputScalars(
      vtkDataObject::FIELD_ASSOCIATION_POINTS, FieldName);
    PointGradients->SetResultArrayName(ResultName);
    
    CellGradients->Update();
    PointGradients->Update();
    
    vtkDoubleArray* GradCellArray = vtkDoubleArray::SafeDownCast(
      vtkDataSet::SafeDownCast(
        CellGradients->GetOutput())->GetCellData()->GetArray(ResultName));
    
    if(!Grid->IsA("vtkUnstructuredGrid"))
      {
      // ignore cell gradients if this is an unstructured grid
      // because the accuracy is so lousy
      if(!IsGradientCorrect(GradCellArray, Offset))
        {
        return 1;
        }
      }
    
    vtkDoubleArray* GradPointArray = vtkDoubleArray::SafeDownCast(
      vtkDataSet::SafeDownCast(
        PointGradients->GetOutput())->GetPointData()->GetArray(ResultName));
    
    if(!IsGradientCorrect(GradPointArray, Offset))
      {
      return 1;
      }
    
    if(NumberOfComponents == 3) 
      {
      // now check on the vorticity calculations
      VTK_CREATE(vtkGradientFilter, CellVorticity);
      CellVorticity->SetInput(Grid);
      CellVorticity->SetInputScalars(
        vtkDataObject::FIELD_ASSOCIATION_CELLS, FieldName);
      CellVorticity->SetResultArrayName(ResultName);
      CellVorticity->SetComputeVorticity(1);
      CellVorticity->Update();
      
      VTK_CREATE(vtkGradientFilter, PointVorticity);
      PointVorticity->SetInput(Grid);
      PointVorticity->SetInputScalars(
        vtkDataObject::FIELD_ASSOCIATION_POINTS, FieldName);
      PointVorticity->SetResultArrayName(ResultName);
      PointVorticity->SetComputeVorticity(1);
      PointVorticity->Update();
      
      // cell stuff
      vtkDoubleArray* VorticityCellArray = vtkDoubleArray::SafeDownCast(
        vtkDataSet::SafeDownCast(
          CellVorticity->GetOutput())->GetCellData()->GetArray(ResultName));
      
      if(!IsVorticityCorrect(GradCellArray, VorticityCellArray))
        {
        return 1;
        }
      
      // point stuff
      vtkDoubleArray* VorticityPointArray = vtkDoubleArray::SafeDownCast(
        vtkDataSet::SafeDownCast(
          PointVorticity->GetOutput())->GetPointData()->GetArray(ResultName));
      
      if(!IsVorticityCorrect(GradPointArray, VorticityPointArray))
        {
        return 1;
        }
      }
    
    return 0;
  }
} // end local namespace

//-----------------------------------------------------------------------------
int TestGradientAndVorticity(int argc, char *argv[])
{
  int i;
  // Need to get the data root.
  const char *data_root = NULL;
  for (i = 0; i < argc-1; i++)
    {
    if (strcmp("-D", argv[i]) == 0)
      {
      data_root = argv[i+1];
      break;
      }
    }
  if (!data_root)
    {
    vtkGenericWarningMacro(
      "Need to specify the directory to VTK_DATA_ROOT with -D <dir>.");
    return 1;
    }

  vtkStdString filename;
  filename = data_root;
  filename += "/Data/SampleStructGrid.vtk";
  VTK_CREATE(vtkStructuredGridReader, StructuredGridReader);
  StructuredGridReader->SetFileName(filename.c_str());
  StructuredGridReader->Update();
  vtkDataSet* Grid = vtkDataSet::SafeDownCast(
    StructuredGridReader->GetOutput());

  if(PerformTest(Grid))
    {
    return 1;
    }

  // convert the structured grid to an unstructured grid
  VTK_CREATE(vtkUnstructuredGrid, UG);
  UG->SetPoints(vtkStructuredGrid::SafeDownCast(Grid)->GetPoints());
  UG->Allocate(Grid->GetNumberOfCells());
  for(vtkIdType id=0;id<Grid->GetNumberOfCells();id++)
    {
    vtkCell* Cell = Grid->GetCell(id);
    UG->InsertNextCell(Cell->GetCellType(), Cell->GetPointIds());
    }
  
  return PerformTest(UG);
}
