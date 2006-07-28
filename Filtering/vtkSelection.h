/*=========================================================================

  Program:   ParaView
  Module:    vtkSelection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSelection - Datastructure to store results of selection
// .SECTION Description
// vtkSelection is the root node of a vtkSelectionNode tree. Currently,
// it does not implement any additional methods but is there so that the
// root node can be differentiated by classname alone. In the future,
// functions specific to the root node alone will be implemented here.
// .SECTION See Also
// vtkSelectionNode

#ifndef __vtkSelection_h
#define __vtkSelection_h

#include "vtkSelectionNode.h"

class VTK_FILTERING_EXPORT vtkSelection : public vtkSelectionNode
{
public:
  vtkTypeRevisionMacro(vtkSelection,vtkSelectionNode);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSelection* New();

protected:
  vtkSelection();
  ~vtkSelection();

private:
  vtkSelection(const vtkSelection&);  // Not implemented.
  void operator=(const vtkSelection&);  // Not implemented.
};

#endif
