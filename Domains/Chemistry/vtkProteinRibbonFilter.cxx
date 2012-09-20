/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProteinRibbonFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkProteinRibbonFilter.h"

#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkTubeFilter.h"
#include "vtkDoubleArray.h"
#include "vtkSplineFilter.h"
#include "vtkInformation.h"
#include "vtkStringArray.h"
#include "vtkCellArray.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkProteinRibbonFilter)

vtkProteinRibbonFilter::vtkProteinRibbonFilter()
{
}

vtkProteinRibbonFilter::~vtkProteinRibbonFilter()
{
}

int vtkProteinRibbonFilter::FillInputPortInformation(int port,
                                                     vtkInformation *info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }

  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}

int vtkProteinRibbonFilter::RequestData(vtkInformation *,
                                        vtkInformationVector **inputVector,
                                        vtkInformationVector *outputVector)
{
    vtkPolyData *input = vtkPolyData::GetData(inputVector[0]);
    vtkPolyData *output = vtkPolyData::GetData(outputVector);

    // extract alpha-carbon backbone from input poly data
    vtkStringArray *atomTypes =
      vtkStringArray::SafeDownCast(
        input->GetPointData()->GetAbstractArray("atom_types"));
    if(!atomTypes)
      {
      vtkErrorMacro(<< "Atom Type String Array Required");
      return 0;
      }

    vtkNew<vtkPoints> points;
    for(int i = 0; i < input->GetNumberOfPoints(); i++)
      {
      vtkStdString type = atomTypes->GetValue(i);
      if(type == "CA")
        {
        points->InsertNextPoint(input->GetPoint(i));
        }
      }

    vtkNew<vtkCellArray> lines;
    lines->InsertNextCell(points->GetNumberOfPoints());
    for(int i = 0; i < points->GetNumberOfPoints(); i++)
      {
      lines->InsertCellPoint(i);
      }

    vtkNew<vtkPolyData> backbone;
    backbone->SetPoints(points.GetPointer());
    backbone->SetLines(lines.GetPointer());

    vtkNew<vtkSplineFilter> splineFilter;
    splineFilter->SetNumberOfSubdivisions(750);
    splineFilter->SetInputData(backbone.GetPointer());

    vtkNew<vtkTubeFilter> tubeFilter;
    tubeFilter->SetInputConnection(splineFilter->GetOutputPort());
    tubeFilter->SetRadius(0.4f);
    tubeFilter->SetNumberOfSides(20);
    tubeFilter->SetCapping(true);
    tubeFilter->Update();

    output->ShallowCopy(tubeFilter->GetOutput());

    return 1;
}

void vtkProteinRibbonFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
