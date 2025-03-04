// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellCentersPointPlacer.h"

#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkCell.h"
#include "vtkCellPicker.h"
#include "vtkDataSet.h"
#include "vtkInteractorObserver.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkProp.h"
#include "vtkPropCollection.h"
#include "vtkRenderer.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCellCentersPointPlacer);

//------------------------------------------------------------------------------
vtkCellCentersPointPlacer::vtkCellCentersPointPlacer()
{
  this->PickProps = vtkPropCollection::New();
  this->CellPicker = vtkCellPicker::New();
  this->CellPicker->PickFromListOn();

  this->Mode = vtkCellCentersPointPlacer::CellPointsMean;
}

//------------------------------------------------------------------------------
vtkCellCentersPointPlacer::~vtkCellCentersPointPlacer()
{
  this->PickProps->Delete();
  this->CellPicker->Delete();
}

//------------------------------------------------------------------------------
void vtkCellCentersPointPlacer::AddProp(vtkProp* prop)
{
  this->PickProps->AddItem(prop);
  this->CellPicker->AddPickList(prop);
}

//------------------------------------------------------------------------------
void vtkCellCentersPointPlacer::RemoveViewProp(vtkProp* prop)
{
  this->PickProps->RemoveItem(prop);
  this->CellPicker->DeletePickList(prop);
}

//------------------------------------------------------------------------------
void vtkCellCentersPointPlacer::RemoveAllProps()
{
  this->PickProps->RemoveAllItems();
  this->CellPicker->InitializePickList(); // clear the pick list.. remove
                                          // old props from it...
}

//------------------------------------------------------------------------------
vtkTypeBool vtkCellCentersPointPlacer::HasProp(vtkProp* prop)
{
  int index = this->PickProps->IndexOfFirstOccurence(prop);

#if defined(VTK_LEGACY_REMOVE)
  return (index >= 0);
#else
  // VTK_DEPRECATED_IN_9_5_0()
  // Keep "#if" block and remove this "#else" when removing 9.5.0 deprecations

  // The implementation used to call IsItemPresent(), which, despite its name,
  // returned an index, not a boolean.  Preserve the old behaviour.  0 means
  // the item is not found, otherwise return the index + 1.
  return index + 1;
#endif
}

//------------------------------------------------------------------------------
int vtkCellCentersPointPlacer::GetNumberOfProps()
{
  return this->PickProps->GetNumberOfItems();
}

//------------------------------------------------------------------------------
int vtkCellCentersPointPlacer::ComputeWorldPosition(vtkRenderer* ren, double displayPos[2],
  double* vtkNotUsed(refWorldPos), double worldPos[3], double worldOrient[9])
{
  return this->ComputeWorldPosition(ren, displayPos, worldPos, worldOrient);
}

//------------------------------------------------------------------------------
int vtkCellCentersPointPlacer::ComputeWorldPosition(
  vtkRenderer* ren, double displayPos[2], double worldPos[3], double vtkNotUsed(worldOrient)[9])
{
  vtkDebugMacro(<< "Request for computing world position at "
                << "display position of " << displayPos[0] << "," << displayPos[1]);

  if (this->CellPicker->Pick(displayPos[0], displayPos[1], 0.0, ren))
  {
    if (vtkAssemblyPath* path = this->CellPicker->GetPath())
    {

      // We are checking if the prop present in the path is present
      // in the list supplied to us.. If it is, that prop will be picked.
      // If not, no prop will be picked.

      bool found = false;
      vtkAssemblyNode* node = nullptr;
      vtkCollectionSimpleIterator sit;
      this->PickProps->InitTraversal(sit);

      while (vtkProp* p = this->PickProps->GetNextProp(sit))
      {
        vtkCollectionSimpleIterator psit;
        path->InitTraversal(psit);

        for (int i = 0; i < path->GetNumberOfItems() && !found; ++i)
        {
          node = path->GetNextNode(psit);
          found = (node->GetViewProp() == p);
        }

        if (found)
        {
          vtkIdType pickedCellId = this->CellPicker->GetCellId();
          vtkCell* pickedCell = this->CellPicker->GetDataSet()->GetCell(pickedCellId);

          if (this->Mode == vtkCellCentersPointPlacer::ParametricCenter)
          {
            double pcoords[3];
            pickedCell->GetParametricCenter(pcoords);
            double* weights = new double[pickedCell->GetNumberOfPoints()];

            int subId;
            pickedCell->EvaluateLocation(subId, pcoords, worldPos, weights);
            delete[] weights;
          }

          if (this->Mode == vtkCellCentersPointPlacer::CellPointsMean)
          {
            const vtkIdType nPoints = pickedCell->GetNumberOfPoints();
            vtkPoints* points = pickedCell->GetPoints();
            worldPos[0] = worldPos[1] = worldPos[2] = 0.0;
            double pp[3];
            for (vtkIdType i = 0; i < nPoints; i++)
            {
              points->GetPoint(i, pp);
              worldPos[0] += pp[0];
              worldPos[1] += pp[1];
              worldPos[2] += pp[2];
            }

            worldPos[0] /= (static_cast<double>(nPoints));
            worldPos[1] /= (static_cast<double>(nPoints));
            worldPos[2] /= (static_cast<double>(nPoints));
          }

          if (this->Mode == vtkCellCentersPointPlacer::None)
          {
            this->CellPicker->GetPickPosition(worldPos);
          }

          return 1;
        }
      }
    }
  }

  return 0;
}

//------------------------------------------------------------------------------
int vtkCellCentersPointPlacer::ValidateWorldPosition(
  double worldPos[3], double* vtkNotUsed(worldOrient))
{
  return this->ValidateWorldPosition(worldPos);
}

//------------------------------------------------------------------------------
int vtkCellCentersPointPlacer::ValidateWorldPosition(double vtkNotUsed(worldPos)[3])
{
  return 1;
}

//------------------------------------------------------------------------------
int vtkCellCentersPointPlacer::ValidateDisplayPosition(
  vtkRenderer*, double vtkNotUsed(displayPos)[2])
{
  return 1;
}

//------------------------------------------------------------------------------
void vtkCellCentersPointPlacer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CellPicker: " << this->CellPicker << endl;
  if (this->CellPicker)
  {
    this->CellPicker->PrintSelf(os, indent.GetNextIndent());
  }

  os << indent << "PickProps: " << this->PickProps << endl;
  if (this->PickProps)
  {
    this->PickProps->PrintSelf(os, indent.GetNextIndent());
  }

  os << indent << "Mode: " << this->Mode << endl;
}
VTK_ABI_NAMESPACE_END
