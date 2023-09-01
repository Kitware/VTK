// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTriangleFilter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkTriangleStrip.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTriangleFilter);

//-------------------------------------------------------------------------
int vtkTriangleFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numCells = input->GetNumberOfCells();
  vtkIdType cellNum = 0;
  vtkIdType numPts, newId;
  vtkIdType npts = 0;
  const vtkIdType* pts = nullptr;
  int i, j;
  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();
  vtkIdType updateInterval;
  vtkCellArray* cells;
  vtkPoints* inPts = input->GetPoints();

  bool abort = false;
  updateInterval = numCells / 100 + 1;
  outCD->CopyAllocate(inCD, numCells);

  // Do each of the verts, lines, polys, and strips separately
  // verts
  if (input->GetVerts()->GetNumberOfCells() > 0)
  {
    cells = input->GetVerts();
    if (this->PassVerts)
    {
      newId = output->GetNumberOfCells();
      vtkNew<vtkCellArray> newCells;
      newCells->AllocateCopy(cells);
      for (cells->InitTraversal(); cells->GetNextCell(npts, pts) && !abort; cellNum++)
      {
        if (!(cellNum % updateInterval)) // manage progress reports / early abort
        {
          this->UpdateProgress((float)cellNum / numCells);
          abort = this->CheckAbort();
        }
        if (npts > 1)
        {
          for (i = 0; i < npts; i++)
          {
            newCells->InsertNextCell(1, pts + i);
            outCD->CopyData(inCD, cellNum, newId++);
          }
        }
        else
        {
          newCells->InsertNextCell(1, pts);
          outCD->CopyData(inCD, cellNum, newId++);
        }
      }
      output->SetVerts(newCells);
    }
    else
    {
      cellNum += cells->GetNumberOfCells(); // skip over verts
    }
  }

  // lines
  if (!abort && input->GetLines()->GetNumberOfCells() > 0)
  {
    cells = input->GetLines();
    if (this->PassLines)
    {
      newId = output->GetNumberOfCells();
      vtkNew<vtkCellArray> newCells;
      newCells->AllocateCopy(cells);
      for (cells->InitTraversal(); cells->GetNextCell(npts, pts) && !abort; cellNum++)
      {
        if (!(cellNum % updateInterval)) // manage progress reports / early abort
        {
          this->UpdateProgress((float)cellNum / numCells);
          abort = this->CheckAbort();
        }
        if (npts > 2)
        {
          for (i = 0; i < (npts - 1); i++)
          {
            newCells->InsertNextCell(2, pts + i);
            outCD->CopyData(inCD, cellNum, newId++);
          }
        }
        else
        {
          newCells->InsertNextCell(2, pts);
          outCD->CopyData(inCD, cellNum, newId++);
        }
      } // for all lines
      output->SetLines(newCells);
    }
    else
    {
      cellNum += cells->GetNumberOfCells(); // skip over lines
    }
  }

  // Output from polygons and triangle strips cell arrays are placed
  // in newPolys.
  vtkSmartPointer<vtkCellArray> newPolys;
  if (!abort && input->GetPolys()->GetNumberOfCells() > 0)
  {
    cells = input->GetPolys();
    newId = output->GetNumberOfCells();
    newPolys = vtkSmartPointer<vtkCellArray>::New();
    newPolys->AllocateCopy(cells);
    output->SetPolys(newPolys);
    vtkNew<vtkIdList> ptIds;
    ptIds->Allocate(VTK_CELL_SIZE);
    int numSimplices;
    vtkIdType triPts[3];
    // It may be necessary to specify a custom tessellation
    // tolerance.
    vtkNew<vtkPolygon> poly;
    if (this->Tolerance > 0.0)
    {
      poly->SetTolerance(this->Tolerance); // Tighten tessellation tolerance
    }

    for (cells->InitTraversal(); cells->GetNextCell(npts, pts) && !abort; cellNum++)
    {
      if (!(cellNum % updateInterval)) // manage progress reports / early abort
      {
        this->UpdateProgress((float)cellNum / numCells);
        abort = this->CheckAbort();
      }
      if (npts == 0)
      {
        continue;
      }
      if (npts == 3)
      {
        newPolys->InsertNextCell(3, pts);
        outCD->CopyData(inCD, cellNum, newId++);
      }
      else // triangulate polygon
      {
        // initialize polygon
        poly->PointIds->SetNumberOfIds(npts);
        poly->Points->SetNumberOfPoints(npts);
        for (i = 0; i < npts; i++)
        {
          poly->PointIds->SetId(i, pts[i]);
          poly->Points->SetPoint(i, inPts->GetPoint(pts[i]));
        }
        poly->TriangulateLocalIds(0, ptIds);
        numPts = ptIds->GetNumberOfIds();
        numSimplices = numPts / 3;
        for (i = 0; i < numSimplices; i++)
        {
          for (j = 0; j < 3; j++)
          {
            triPts[j] = poly->PointIds->GetId(ptIds->GetId(3 * i + j));
          }
          newPolys->InsertNextCell(3, triPts);
          outCD->CopyData(inCD, cellNum, newId++);
        } // for each simplex
      }   // triangulate polygon
    }
  }

  // strips
  if (!abort && input->GetStrips()->GetNumberOfCells() > 0)
  {
    cells = input->GetStrips();
    newId = output->GetNumberOfCells();
    if (newPolys == nullptr)
    {
      newPolys = vtkSmartPointer<vtkCellArray>::New();
      newPolys->AllocateCopy(cells);
      output->SetPolys(newPolys);
    }
    for (cells->InitTraversal(); cells->GetNextCell(npts, pts) && !abort; cellNum++)
    {
      if (!(cellNum % updateInterval)) // manage progress reports / early abort
      {
        this->UpdateProgress((float)cellNum / numCells);
        abort = this->CheckAbort();
      }
      vtkTriangleStrip::DecomposeStrip(npts, pts, newPolys);
      for (i = 0; i < (npts - 2); i++)
      {
        outCD->CopyData(inCD, cellNum, newId++);
      }
    } // for all strips
  }

  // Update output
  output->SetPoints(input->GetPoints());
  output->GetPointData()->PassData(input->GetPointData());
  output->Squeeze();

  vtkDebugMacro(<< "Converted " << input->GetNumberOfCells() << "input cells to "
                << output->GetNumberOfCells() << " output cells");

  return 1;
}

//-------------------------------------------------------------------------
void vtkTriangleFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Pass Verts: " << (this->PassVerts ? "On\n" : "Off\n");
  os << indent << "Pass Lines: " << (this->PassLines ? "On\n" : "Off\n");
}
VTK_ABI_NAMESPACE_END
