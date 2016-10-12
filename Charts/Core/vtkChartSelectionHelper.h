/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkChartSelectionHelper
 * @brief   helper functions for making selections in
 * charts.
 *
 *
 * This contains several inline methods intended for use inside chart
 * implementations to make chart selections easier. This is intended for
 * internal use and the API should not be considered stable.
*/

#ifndef vtkChartSelectionHelper_h
#define vtkChartSelectionHelper_h

#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkAnnotationLink.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkIdTypeArray.h"
#include "vtkContextScene.h"
#include "vtkContextMouseEvent.h"
#include "vtkInformation.h"
#include "vtkPlot.h"
#include "vtkTable.h"

#include <vector>
#include <algorithm>

namespace vtkChartSelectionHelper
{

/**
 * Populate the annotation link with the supplied selectionIds array, and set
 * the appropriate node properties for a standard row based chart selection.
 */
static void MakeSelection(vtkAnnotationLink *link, vtkIdTypeArray *selectionIds,
                          vtkPlot *plot)
{
  assert(link != NULL && selectionIds != NULL);

  if (plot)
  {
    // We are building up plot-based selections, using multiple nodes.
    vtkSelection *selection = link->GetCurrentSelection();
    vtkSmartPointer<vtkSelectionNode> node;
    for (unsigned int i = 0; i < selection->GetNumberOfNodes(); ++i)
    {
      vtkSelectionNode *tmp = selection->GetNode(i);
      vtkPlot *selectionPlot =
          vtkPlot::SafeDownCast(tmp->GetProperties()->Get(vtkSelectionNode::PROP()));
      if (selectionPlot == plot)
      {
        node = tmp;
        break;
      }
    }
    if (!node)
    {
      node = vtkSmartPointer<vtkSelectionNode>::New();
      selection->AddNode(node.GetPointer());
      node->SetContentType(vtkSelectionNode::INDICES);
      node->SetFieldType(vtkSelectionNode::POINT);
      node->GetProperties()->Set(vtkSelectionNode::PROP(), plot);
      node->GetProperties()->Set(vtkSelectionNode::SOURCE(), plot->GetInput());
    }
    node->SetSelectionList(selectionIds);
  }
  else
  {
    // Use a simple single selection node layout, remove previous selections.
    vtkNew<vtkSelection> selection;
    vtkNew<vtkSelectionNode> node;
    selection->AddNode(node.GetPointer());
    node->SetContentType(vtkSelectionNode::INDICES);
    node->SetFieldType(vtkSelectionNode::POINT);
    node->SetSelectionList(selectionIds);
    link->SetCurrentSelection(selection.GetPointer());
  }
}

//@{
/**
 * Subtract the supplied selection from the oldSelection.
 */
static void MinusSelection(vtkIdTypeArray *selection, vtkIdTypeArray *oldSelection)
{
  // We rely on the selection id arrays being sorted.
  std::vector<vtkIdType> output;
  vtkIdType *ptrSelection =
      static_cast<vtkIdType *>(selection->GetVoidPointer(0));
  vtkIdType *ptrOldSelection =
      static_cast<vtkIdType *>(oldSelection->GetVoidPointer(0));
  vtkIdType oldSize = oldSelection->GetNumberOfTuples();
  vtkIdType size = selection->GetNumberOfTuples();
  vtkIdType i = 0;
  vtkIdType iOld = 0;
//@}

  while (i < size && iOld < oldSize)
  {
    if (ptrSelection[i] > ptrOldSelection[iOld]) // Skip the value.
    {
      output.push_back(ptrOldSelection[iOld++]);
    }
    else if (ptrSelection[i] == ptrOldSelection[iOld]) // Match - remove.
    {
      ++i;
      ++iOld;
    }
    else if (ptrSelection[i] < ptrOldSelection[iOld]) // Add the new value.
    {
      ++i;
    }
  }
  while (iOld < oldSize)
  {
    output.push_back(ptrOldSelection[iOld++]);
  }
  selection->SetNumberOfTuples(output.size());
  ptrSelection = static_cast<vtkIdType *>(selection->GetVoidPointer(0));
  for (std::vector<vtkIdType>::iterator it = output.begin();
       it != output.end(); ++it, ++ptrSelection)
  {
    *ptrSelection = *it;
  }
}

//@{
/**
 * Add the supplied selection from the oldSelection.
 */
static void AddSelection(vtkIdTypeArray *selection, vtkIdTypeArray *oldSelection)
{
  // Add all unique array indices to create a new combined array.
  vtkIdType *ptrSelection =
      static_cast<vtkIdType *>(selection->GetVoidPointer(0));
  vtkIdType *ptrOldSelection =
      static_cast<vtkIdType *>(oldSelection->GetVoidPointer(0));
  std::vector<vtkIdType> output(selection->GetNumberOfTuples()
                                + oldSelection->GetNumberOfTuples());
  std::vector<vtkIdType>::iterator it;
  it = std::set_union(ptrSelection,
                      ptrSelection + selection->GetNumberOfTuples(),
                      ptrOldSelection,
                      ptrOldSelection + oldSelection->GetNumberOfTuples(),
                      output.begin());
  int newSize = int(it - output.begin());
  selection->SetNumberOfTuples(newSize);
  ptrSelection = static_cast<vtkIdType *>(selection->GetVoidPointer(0));
  for (std::vector<vtkIdType>::iterator i = output.begin(); i != it;
       ++i, ++ptrSelection)
  {
    *ptrSelection = *i;
  }
}
//@}

//@{
/**
 * Toggle the supplied selection from the oldSelection.
 */
static void ToggleSelection(vtkIdTypeArray *selection, vtkIdTypeArray *oldSelection)
{
  // We rely on the selection id arrays being sorted.
  std::vector<vtkIdType> output;
  vtkIdType *ptrSelection =
      static_cast<vtkIdType *>(selection->GetVoidPointer(0));
  vtkIdType *ptrOldSelection =
      static_cast<vtkIdType *>(oldSelection->GetVoidPointer(0));
  vtkIdType oldSize = oldSelection->GetNumberOfTuples();
  vtkIdType size = selection->GetNumberOfTuples();
  vtkIdType i = 0;
  vtkIdType iOld = 0;
  while (i < size && iOld < oldSize)
  {
    if (ptrSelection[i] > ptrOldSelection[iOld]) // Retain the value.
    {
      output.push_back(ptrOldSelection[iOld++]);
    }
    else if (ptrSelection[i] == ptrOldSelection[iOld]) // Match - toggle.
    {
      ++i;
      ++iOld;
    }
    else if (ptrSelection[i] < ptrOldSelection[iOld]) // Add the new value.
    {
      output.push_back(ptrSelection[i++]);
    }
  }
  while (i < size)
  {
    output.push_back(ptrSelection[i++]);
  }
  while (iOld < oldSize)
  {
    output.push_back(ptrOldSelection[iOld++]);
  }
  selection->SetNumberOfTuples(output.size());
  ptrSelection = static_cast<vtkIdType *>(selection->GetVoidPointer(0));
  for (std::vector<vtkIdType>::iterator it = output.begin();
       it != output.end(); ++it, ++ptrSelection)
  {
    *ptrSelection = *it;
  }
}
//@}

/**
 * Build a selection based on the supplied selectionMode using the new
 * plotSelection and combining it with the oldSelection. If link is not NULL
 * then the resulting selection will be set on the link.
 */
static void BuildSelection(vtkAnnotationLink *link, int selectionMode,
                           vtkIdTypeArray *plotSelection, vtkIdTypeArray *oldSelection,
                           vtkPlot *plot)
{
  if (!plotSelection || !oldSelection)
  {
    return;
  }

  // Build a selection and set it on the annotation link if not null.
  switch(selectionMode)
  {
    case vtkContextScene::SELECTION_ADDITION:
      AddSelection(plotSelection, oldSelection);
      break;
    case vtkContextScene::SELECTION_SUBTRACTION:
      MinusSelection(plotSelection, oldSelection);
      break;
    case vtkContextScene::SELECTION_TOGGLE:
      ToggleSelection(plotSelection, oldSelection);
      break;
    case vtkContextScene::SELECTION_DEFAULT:
    default:
      // Nothing necessary - overwrite the old selection.
      break;
  }

  if (link)
  {
    MakeSelection(link, plotSelection, plot);
  }
}

//@{
/**
 * Combine the SelectionMode with any mouse modifiers to get an effective
 * selection mode for this click event.
 */
static int GetMouseSelectionMode(const vtkContextMouseEvent &mouse, int selectionMode)
{
  // Mouse modifiers override the current selection mode.
  if (mouse.GetModifiers() & vtkContextMouseEvent::SHIFT_MODIFIER &&
      mouse.GetModifiers() & vtkContextMouseEvent::CONTROL_MODIFIER)
  {
    return vtkContextScene::SELECTION_TOGGLE;
  }
  else if (mouse.GetModifiers() & vtkContextMouseEvent::CONTROL_MODIFIER)
  {
    return vtkContextScene::SELECTION_ADDITION;
  }
  else if (mouse.GetModifiers() & vtkContextMouseEvent::SHIFT_MODIFIER)
  {
    return vtkContextScene::SELECTION_SUBTRACTION;
  }
  return selectionMode;
}
//@}

} // End vtkChartSelectionHelper namespace

#endif // vtkChartSelectionHelper_h
// VTK-HeaderTest-Exclude: vtkChartSelectionHelper.h
