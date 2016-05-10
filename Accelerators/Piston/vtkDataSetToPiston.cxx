/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToPiston.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataSetToPiston.h"

#include "vtkCellArray.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPistonDataObject.h"

//----------------------------------------------------------------------------
namespace vtkpiston {
  //forward declarations of methods defined in the cuda implementation
  void CopyToGPU(vtkImageData *id, vtkPistonDataObject *od);
  void CopyToGPU(vtkPolyData *id, vtkPistonDataObject *od);
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkDataSetToPiston);

//----------------------------------------------------------------------------
vtkDataSetToPiston::vtkDataSetToPiston()
{
  VTK_LEGACY_BODY(vtkDataSetToPiston::vtkDataSetToPiston, "VTK 6.3");
}

//----------------------------------------------------------------------------
vtkDataSetToPiston::~vtkDataSetToPiston()
{
}

//----------------------------------------------------------------------------
void vtkDataSetToPiston::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int
vtkDataSetToPiston
::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkDataSetToPiston::RequestData(vtkInformation *vtkNotUsed(request),
                                     vtkInformationVector** inputVector,
                                     vtkInformationVector* outputVector)
{
  vtkPistonDataObject *od = vtkPistonDataObject::GetData(outputVector);

  vtkDataObject *ido = this->GetInputDataObject(0,0);
  vtkDataSet *ds = vtkDataSet::SafeDownCast(ido);
  vtkImageData *id = vtkImageData::GetData(inputVector[0]);
  if (id)
    {
    double ibds[6];
    double *origin = id->GetOrigin();
    double *spacing = id->GetSpacing();
    int *ext = id->GetExtent();

    ibds[0] = origin[0] + ext[0]*spacing[0];
    ibds[1] = origin[0] + ext[1]*spacing[0];
    ibds[2] = origin[1] + ext[2]*spacing[1];
    ibds[3] = origin[1] + ext[3]*spacing[1];
    ibds[4] = origin[2] + ext[4]*spacing[2];
    ibds[5] = origin[2] + ext[5]*spacing[2];
    od->SetBounds(ibds);
    od->SetOrigin(id->GetOrigin());
    od->SetSpacing(id->GetSpacing());
    }
  else
    {
    od->SetBounds(ds->GetBounds());
    }

  switch (ido->GetDataObjectType())
    {
    case VTK_UNIFORM_GRID:
    case VTK_STRUCTURED_POINTS:
    case VTK_IMAGE_DATA:
      {
      if (id->GetNumberOfScalarComponents() > 1)
        {
        vtkErrorMacro("This filter can handle only 1 component arrays");
        return 1;
        }
      int numPts = id->GetNumberOfPoints();
      if ( numPts < 1 )
        {
        vtkErrorMacro(<<"Points must be defined!");
        return 1;
        }
      int association;
      vtkFloatArray *inArray = vtkArrayDownCast<vtkFloatArray>(
        this->GetInputArrayToProcess(0,id, association));
      //this filter expects that input has point associated float scalars
      if (!inArray
          || association != vtkDataObject::FIELD_ASSOCIATION_POINTS ||
          inArray->GetDataType() != VTK_FLOAT)
        {
        vtkErrorMacro(<< "Can't handle the type of array given.\n");
        return 1;
        }
      vtkpiston::CopyToGPU(id, od);
      }
      break;
    case VTK_POLY_DATA:
      {
      vtkPolyData *idp = vtkPolyData::GetData(inputVector[0]);
      //convert to simplices that piston can handle
      //TODO: support points, lines and tets in addition to triangles
      //TODO: support cell attributes
      vtkPolyData *triangulated = vtkPolyData::New();
      vtkPoints *tPts = vtkPoints::New();
      triangulated->SetPoints(tPts);
      tPts->Delete();
      triangulated->GetPointData()->CopyStructure(idp->GetPointData());
      triangulated->GetPointData()->CopyAllocate(idp->GetPointData());
      triangulated->GetPointData()->SetCopyNormals(1);

      vtkCellArray *cells = idp->GetPolys();
      vtkIdType npts =0;
      vtkIdType *index = 0;
      vtkIdType opnt = 0;
      for (cells->InitTraversal(); cells->GetNextCell(npts, index); )
      {
        vtkIdType triangle[3];
        triangle[0] = index[0];
        triangle[1] = index[1];
        triangle[2] = index[2];
        //the first triangle
        triangulated->GetPoints()->InsertNextPoint(idp->GetPoint(triangle[0]));
        triangulated->GetPointData()->CopyData(idp->GetPointData(), triangle[0], opnt++);
        triangulated->GetPoints()->InsertNextPoint(idp->GetPoint(triangle[1]));
        triangulated->GetPointData()->CopyData(idp->GetPointData(), triangle[1], opnt++);
        triangulated->GetPoints()->InsertNextPoint(idp->GetPoint(triangle[2]));
        triangulated->GetPointData()->CopyData(idp->GetPointData(), triangle[2], opnt++);
        // the remaining triangles, of which
        // each introduces a triangle after extraction
        for ( vtkIdType i = 3; i < npts; i ++ )
        {
          triangle[1] = triangle[2];
          triangle[2] = index[i];
          triangulated->GetPoints()->InsertNextPoint(idp->GetPoint(triangle[0]));
          triangulated->GetPointData()->CopyData(idp->GetPointData(), triangle[0], opnt++);
          triangulated->GetPoints()->InsertNextPoint(idp->GetPoint(triangle[1]));
          triangulated->GetPointData()->CopyData(idp->GetPointData(), triangle[1], opnt++);
          triangulated->GetPoints()->InsertNextPoint(idp->GetPoint(triangle[2]));
          triangulated->GetPointData()->CopyData(idp->GetPointData(), triangle[2], opnt++);
        }
      }
      vtkpiston::CopyToGPU(triangulated, od);
      triangulated->Delete();
      }
      break;
    default:
      vtkWarningMacro(<< "I don't have a converter from " << ido->GetClassName() << " yet.");
    }
  return 1;
}
