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

#include <vector>

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
      std::cout << fabs(v2) << " (fabs(v2)) should be less than "
           << Tolerance << std::endl;
      return false;
      }
    if(fabs(v1/v2) < Tolerance)
      {
        return true;
        }
    std::cout << fabs(v1/v2) << " (fabs(v1/v2)) should be less than "
         << Tolerance << std::endl;
    return false;
  }

//-----------------------------------------------------------------------------
  void CreateCellData(vtkDataSet* grid, int numberOfComponents, int offset,
                      const char* arrayName)
  {
    vtkIdType numberOfCells = grid->GetNumberOfCells();
    VTK_CREATE(vtkDoubleArray, array);
    array->SetNumberOfComponents(numberOfComponents);
    array->SetNumberOfTuples(numberOfCells);
    std::vector<double> tupleValues(numberOfComponents);
    double point[3], parametricCenter[3], weights[100];
    for(vtkIdType i=0;i<numberOfCells;i++)
      {
      vtkCell* cell = grid->GetCell(i);
      cell->GetParametricCenter(parametricCenter);
      int subId = 0;
      cell->EvaluateLocation(subId, parametricCenter, point, weights);
      for(int j=0;j<numberOfComponents;j++)
        {// +offset makes the curl/vorticity nonzero
        tupleValues[j] = point[(j+offset)%3];
        }
      array->SetTupleValue(i, &tupleValues[0]);
      }
    array->SetName(arrayName);
    grid->GetCellData()->AddArray(array);
  }

//-----------------------------------------------------------------------------
  void CreatePointData(vtkDataSet* grid, int numberOfComponents, int offset,
                       const char* arrayName)
  {
    vtkIdType numberOfPoints = grid->GetNumberOfPoints();
    VTK_CREATE(vtkDoubleArray, array);
    array->SetNumberOfComponents(numberOfComponents);
    array->SetNumberOfTuples(numberOfPoints);
    std::vector<double> tupleValues(numberOfComponents);
    double point[3];
    for(vtkIdType i=0;i<numberOfPoints;i++)
      {
      grid->GetPoint(i, point);
      for(int j=0;j<numberOfComponents;j++)
        {// +offset makes the curl/vorticity nonzero
        tupleValues[j] = point[(j+offset)%3];
        }
      array->SetTupleValue(i, &tupleValues[0]);
      }
    array->SetName(arrayName);
    grid->GetPointData()->AddArray(array);
  }

//-----------------------------------------------------------------------------
  int IsGradientCorrect(vtkDoubleArray* gradients, int offset)
  {
    int numberOfComponents = gradients->GetNumberOfComponents();
    for(vtkIdType i=0;i<gradients->GetNumberOfTuples();i++)
      {
      double* values = gradients->GetTuple(i);
      for(int origComp=0;origComp<numberOfComponents/3;origComp++)
        {
        for(int gradDir=0;gradDir<3;gradDir++)
          {
          if((origComp-gradDir+offset)%3 == 0)
            {
            if(fabs(values[origComp*3+gradDir]-1.) > Tolerance)
              {
              vtkGenericWarningMacro("Gradient value should be one but is "
                                     << values[origComp*3+gradDir]);
              return 0;
              }
            }
          else if(fabs(values[origComp*3+gradDir]) > Tolerance)
            {
            vtkGenericWarningMacro("Gradient value should be zero but is "
                                   << values[origComp*3+gradDir]);
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
  int IsVorticityCorrect(vtkDoubleArray* gradients, vtkDoubleArray* vorticity)
  {
    if(gradients->GetNumberOfComponents() != 9 ||
       vorticity->GetNumberOfComponents() != 3)
      {
      vtkGenericWarningMacro("Bad number of components.");
      return 0;
      }
    for(vtkIdType i=0;i<gradients->GetNumberOfTuples();i++)
      {
      double* g = gradients->GetTuple(i);
      double* v = vorticity->GetTuple(i);
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
// we assume that the gradients are correct and so we can compute the "real"
// vorticity from it
  int IsQCriterionCorrect(vtkDoubleArray* gradients, vtkDoubleArray* qCriterion)
  {
    if(gradients->GetNumberOfComponents() != 9 ||
       qCriterion->GetNumberOfComponents() != 1)
      {
      vtkGenericWarningMacro("Bad number of components.");
      return 0;
      }
    for(vtkIdType i=0;i<gradients->GetNumberOfTuples();i++)
      {
      double* g = gradients->GetTuple(i);
      double qc = qCriterion->GetValue(i);

      double t1 = .25*(
        (g[7]-g[5])*(g[7]-g[5]) +
        (g[3]-g[1])*(g[3]-g[1]) +
        (g[2]-g[6])*(g[2]-g[6]) );
      double t2 = .5 * ( g[0]*g[0]+g[4]*g[4]+
                         g[8]*g[8]+ .5 *(
                           (g[3]+g[1])*(g[3]+g[1]) +
                           (g[6]+g[2])*(g[6]+g[2]) +
                           (g[7]+g[5])*(g[7]+g[5]) ) );

      if(!ArePointsWithinTolerance(qc, t1 - t2))
        {
        vtkGenericWarningMacro("Bad Q-criterion value " << qc << " " <<
                               t1-t2 << " difference is " << (qc-t1+t2));
        return 0;
        }
      }

    return 1;
  }

//-----------------------------------------------------------------------------
  int PerformTest(vtkDataSet* grid)
  {
    // Cleaning out the existing field data so that I can replace it with
    // an analytic function that I know the gradient of
    grid->GetPointData()->Initialize();
    grid->GetCellData()->Initialize();
    const char fieldName[] = "LinearField";
    int offset = 1;
    int numberOfComponents = 3;
    CreateCellData(grid, numberOfComponents, offset, fieldName);
    CreatePointData(grid, numberOfComponents, offset, fieldName);

    VTK_CREATE(vtkGradientFilter, cellGradients);
    cellGradients->SetInputData(grid);
    cellGradients->SetInputScalars(
      vtkDataObject::FIELD_ASSOCIATION_CELLS, fieldName);
    const char resultName[] = "Result";
    cellGradients->SetResultArrayName(resultName);

    VTK_CREATE(vtkGradientFilter, pointGradients);
    pointGradients->SetInputData(grid);
    pointGradients->SetInputScalars(
      vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldName);
    pointGradients->SetResultArrayName(resultName);

    cellGradients->Update();
    pointGradients->Update();

    vtkDoubleArray* gradCellArray = vtkDoubleArray::SafeDownCast(
      vtkDataSet::SafeDownCast(
        cellGradients->GetOutput())->GetCellData()->GetArray(resultName));

    if(!grid->IsA("vtkUnstructuredGrid"))
      {
      // ignore cell gradients if this is an unstructured grid
      // because the accuracy is so lousy
      if(!IsGradientCorrect(gradCellArray, offset))
        {
        return EXIT_FAILURE;
        }
      }

    vtkDoubleArray* gradPointArray = vtkDoubleArray::SafeDownCast(
      vtkDataSet::SafeDownCast(
        pointGradients->GetOutput())->GetPointData()->GetArray(resultName));

    if(!IsGradientCorrect(gradPointArray, offset))
      {
      return EXIT_FAILURE;
      }

    if(numberOfComponents == 3)
      {
      // now check on the vorticity calculations
      VTK_CREATE(vtkGradientFilter, cellVorticity);
      cellVorticity->SetInputData(grid);
      cellVorticity->SetInputScalars(
        vtkDataObject::FIELD_ASSOCIATION_CELLS, fieldName);
      cellVorticity->SetResultArrayName(resultName);
      cellVorticity->SetComputeVorticity(1);
      cellVorticity->Update();

      VTK_CREATE(vtkGradientFilter, pointVorticity);
      pointVorticity->SetInputData(grid);
      pointVorticity->SetInputScalars(
        vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldName);
      pointVorticity->SetResultArrayName(resultName);
      pointVorticity->SetComputeVorticity(1);
      pointVorticity->SetComputeQCriterion(1);
      pointVorticity->Update();

      // cell stuff
      vtkDoubleArray* vorticityCellArray = vtkDoubleArray::SafeDownCast(
        vtkDataSet::SafeDownCast(
          cellVorticity->GetOutput())->GetCellData()->GetArray("Vorticity"));

      if(!IsVorticityCorrect(gradCellArray, vorticityCellArray))
        {
        return EXIT_FAILURE;
        }

      // point stuff
      vtkDoubleArray* vorticityPointArray = vtkDoubleArray::SafeDownCast(
        vtkDataSet::SafeDownCast(
          pointVorticity->GetOutput())->GetPointData()->GetArray("Vorticity"));

      if(!IsVorticityCorrect(gradPointArray, vorticityPointArray))
        {
        return EXIT_FAILURE;
        }
      vtkDoubleArray* qCriterionPointArray = vtkDoubleArray::SafeDownCast(
        vtkDataSet::SafeDownCast(
          pointVorticity->GetOutput())->GetPointData()->GetArray("Q-criterion"));
      if(!IsQCriterionCorrect(gradPointArray, qCriterionPointArray))
        {
        return EXIT_FAILURE;
        }
      }

    return EXIT_SUCCESS;
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
    return EXIT_FAILURE;
    }

  vtkStdString filename;
  filename = data_root;
  filename += "/Data/SampleStructGrid.vtk";
  VTK_CREATE(vtkStructuredGridReader, structuredGridReader);
  structuredGridReader->SetFileName(filename.c_str());
  structuredGridReader->Update();
  vtkDataSet* grid = vtkDataSet::SafeDownCast(
    structuredGridReader->GetOutput());

  if(PerformTest(grid))
    {
    return EXIT_FAILURE;
    }

  // convert the structured grid to an unstructured grid
  VTK_CREATE(vtkUnstructuredGrid, ug);
  ug->SetPoints(vtkStructuredGrid::SafeDownCast(grid)->GetPoints());
  ug->Allocate(grid->GetNumberOfCells());
  for(vtkIdType id=0;id<grid->GetNumberOfCells();id++)
    {
    vtkCell* cell = grid->GetCell(id);
    ug->InsertNextCell(cell->GetCellType(), cell->GetPointIds());
    }

  return PerformTest(ug);
}
