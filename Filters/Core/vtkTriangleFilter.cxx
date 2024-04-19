// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTriangleFilter.h"

#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
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
  // get the input and output
  vtkPolyData* input = vtkPolyData::GetData(inputVector[0]);
  vtkPolyData* output = vtkPolyData::GetData(outputVector);

  const vtkIdType numInVerts = input->GetNumberOfVerts();
  const vtkIdType numInLines = input->GetNumberOfLines();
  const vtkIdType numInPolys = input->GetNumberOfPolys();
  const vtkIdType numInStrips = input->GetNumberOfStrips();
  const vtkIdType numInCells = numInVerts + numInLines + numInPolys + numInStrips;
  vtkCellArray* inVerts = input->GetVerts();
  vtkCellArray* inLines = input->GetLines();
  vtkCellArray* inPolys = input->GetPolys();
  vtkCellArray* inStrips = input->GetStrips();
  vtkPoints* inPts = input->GetPoints();
  vtkCellData* inCD = input->GetCellData();

  vtkCellData* outCD = output->GetCellData();

  const vtkIdType* pts;
  vtkIdType npts;
  vtkIdType inCellId = 0;
  vtkIdType outCellId;

  bool abort = false;
  const vtkIdType updateInterval = numInCells / 100 + 1;
  outCD->CopyAllocate(inCD, numInCells);

  // Do each of the verts, lines, polys, and strips separately
  // verts
  if (numInVerts > 0)
  {
    if (this->PassVerts)
    {
      if (this->PreservePolys)
      {
        output->SetVerts(inVerts);
      }
      else if (inVerts->GetMaxCellSize() == 1)
      {
        output->SetVerts(inVerts);
        if (numInVerts == numInCells)
        {
          outCD->PassData(inCD);
        }
        else
        {
          outCD->CopyData(inCD, output->GetNumberOfCells(), numInVerts, inCellId);
        }
        inCellId += numInVerts;
      }
      else
      {
        outCellId = output->GetNumberOfCells();
        vtkNew<vtkCellArray> newCells;
        newCells->AllocateCopy(inVerts);

        auto iter = vtk::TakeSmartPointer(inVerts->NewIterator());
        for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal() && !abort;
             iter->GoToNextCell(), ++inCellId)
        {
          if (!(inCellId % updateInterval)) // manage progress reports / early abort
          {
            this->UpdateProgress((float)inCellId / numInCells);
            abort = this->CheckAbort();
          }
          iter->GetCurrentCell(npts, pts);
          if (npts > 1)
          {
            for (vtkIdType i = 0; i < npts; ++i)
            {
              newCells->InsertNextCell(1, pts + i);
              outCD->CopyData(inCD, inCellId, outCellId++);
            }
          }
          else
          {
            newCells->InsertNextCell(1, pts);
            outCD->CopyData(inCD, inCellId, outCellId++);
          }
        }
        output->SetVerts(newCells);
      }
    }
    else
    {
      inCellId += numInVerts; // skip over verts
    }
  }

  // lines
  if (!abort && numInLines > 0)
  {
    if (this->PassLines)
    {
      if (this->PreservePolys)
      {
        output->SetLines(inLines);
      }
      else if (inLines->GetMaxCellSize() == 2)
      {
        output->SetLines(inLines);
        if (numInLines == numInCells)
        {
          outCD->PassData(inCD);
        }
        else
        {
          outCD->CopyData(inCD, output->GetNumberOfCells(), numInLines, inCellId);
        }
        inCellId += numInLines;
      }
      else
      {
        outCellId = output->GetNumberOfCells();
        vtkNew<vtkCellArray> newCells;
        newCells->AllocateCopy(inLines);

        auto iter = vtk::TakeSmartPointer(inLines->NewIterator());
        for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal() && !abort;
             iter->GoToNextCell(), ++inCellId)
        {
          if (!(inCellId % updateInterval)) // manage progress reports / early abort
          {
            this->UpdateProgress((float)inCellId / numInCells);
            abort = this->CheckAbort();
          }
          iter->GetCurrentCell(npts, pts);
          if (npts > 2)
          {
            for (vtkIdType i = 0; i < (npts - 1); i++)
            {
              newCells->InsertNextCell(2, pts + i);
              outCD->CopyData(inCD, inCellId, outCellId++);
            }
          }
          else
          {
            newCells->InsertNextCell(2, pts);
            outCD->CopyData(inCD, inCellId, outCellId++);
          }
        } // for all lines
        output->SetLines(newCells);
      }
    }
    else
    {
      inCellId += numInLines; // skip over lines
    }
  }

  // Output from polygons and triangle strips cell arrays are placed
  // in newPolys.
  vtkSmartPointer<vtkCellArray> newPolys;
  if (!abort && numInPolys > 0)
  {
    newPolys = vtkSmartPointer<vtkCellArray>::New();
    if (this->PreservePolys)
    {
      if (numInStrips == 0)
      {
        newPolys->ShallowCopy(inPolys);
      }
      else
      {
        newPolys->DeepCopy(inPolys);
      }
      output->SetPolys(newPolys);
    }
    else if (inPolys->GetMaxCellSize() == 3)
    {
      if (numInStrips == 0)
      {
        newPolys->ShallowCopy(inPolys);
      }
      else
      {
        newPolys->DeepCopy(inPolys);
      }
      output->SetPolys(newPolys);
      output->SetLines(inLines);
      if (numInPolys == numInCells)
      {
        outCD->PassData(inCD);
      }
      else
      {
        outCD->CopyData(inCD, output->GetNumberOfCells(), numInPolys, inCellId);
      }
      inCellId += numInPolys;
    }
    else
    {
      outCellId = output->GetNumberOfCells();
      newPolys->AllocateCopy(inPolys);

      vtkNew<vtkIdList> ptIds;
      ptIds->Allocate(VTK_CELL_SIZE);
      vtkIdType triPts[3];
      // It may be necessary to specify a custom tessellation
      // tolerance.
      vtkNew<vtkPolygon> poly;
      if (this->Tolerance > 0.0)
      {
        poly->SetTolerance(this->Tolerance); // Tighten tessellation tolerance
      }

      auto iter = vtk::TakeSmartPointer(inPolys->NewIterator());
      for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal() && !abort;
           iter->GoToNextCell(), ++inCellId)
      {
        if (!(inCellId % updateInterval)) // manage progress reports / early abort
        {
          this->UpdateProgress((float)inCellId / numInCells);
          abort = this->CheckAbort();
        }
        iter->GetCurrentCell(npts, pts);
        if (npts == 3)
        {
          newPolys->InsertNextCell(3, pts);
          outCD->CopyData(inCD, inCellId, outCellId++);
        }
        else // triangulate polygon
        {
          // initialize polygon
          poly->PointIds->SetNumberOfIds(npts);
          poly->Points->SetNumberOfPoints(npts);
          for (vtkIdType i = 0; i < npts; i++)
          {
            poly->PointIds->SetId(i, pts[i]);
            poly->Points->SetPoint(i, inPts->GetPoint(pts[i]));
          }
          poly->TriangulateLocalIds(0, ptIds);
          const int numSimplices = ptIds->GetNumberOfIds() / 3;
          for (vtkIdType i = 0; i < numSimplices; i++)
          {
            for (vtkIdType j = 0; j < 3; j++)
            {
              triPts[j] = poly->PointIds->GetId(ptIds->GetId(3 * i + j));
            }
            newPolys->InsertNextCell(3, triPts);
            outCD->CopyData(inCD, inCellId, outCellId++);
          } // for each simplex
        }   // triangulate polygon
      }
      output->SetPolys(newPolys);
    }
  }

  // if PreservePolys is on, then we need to copy all the cell data till now
  const vtkIdType numInCellsHere = numInVerts + numInLines + numInPolys;
  if (this->PreservePolys && numInCellsHere > 0)
  {
    if (numInStrips == 0)
    {
      outCD->PassData(inCD);
    }
    else
    {
      outCD->CopyData(inCD, 0, numInCellsHere, 0);
      inCellId += numInCellsHere;
    }
  }

  // strips
  if (!abort && numInStrips > 0)
  {
    outCellId = output->GetNumberOfCells();
    if (newPolys == nullptr)
    {
      newPolys = vtkSmartPointer<vtkCellArray>::New();
      newPolys->AllocateCopy(inStrips);
    }

    auto iter = vtk::TakeSmartPointer(inStrips->NewIterator());
    for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal() && !abort;
         iter->GoToNextCell(), ++inCellId)
    {
      if (!(inCellId % updateInterval)) // manage progress reports / early abort
      {
        this->UpdateProgress((float)inCellId / numInCells);
        abort = this->CheckAbort();
      }
      iter->GetCurrentCell(npts, pts);
      vtkTriangleStrip::DecomposeStrip(npts, pts, newPolys);
      for (vtkIdType i = 0; i < (npts - 2); i++)
      {
        outCD->CopyData(inCD, inCellId, outCellId++);
      }
    } // for all strips
    output->SetPolys(newPolys);
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
