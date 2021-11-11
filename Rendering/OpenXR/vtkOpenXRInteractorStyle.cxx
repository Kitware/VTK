/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenXRInteractorStyle.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenXRInteractorStyle.h"

#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTreeIterator.h"

#include "vtkBillboardTextActor3D.h"
#include "vtkCoordinate.h"
#include "vtkTextActor.h"
#include "vtkTextActor3D.h"

#include "vtkAssemblyPath.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCellPicker.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkHardwareSelector.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkMapper.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPropPicker.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSphereSource.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include "vtkTimerLog.h"
#include "vtkVRHardwarePicker.h"

#include "vtkOpenXRCamera.h"
#include "vtkOpenXRRenderWindow.h"
#include "vtkOpenXRRenderWindowInteractor.h"
#include "vtkVRMenuRepresentation.h"
#include "vtkVRMenuWidget.h"
#include "vtkVRModel.h"

#include "vtkWidgetRepresentation.h"

#include <sstream>

// Map controller inputs to interaction states
vtkStandardNewMacro(vtkOpenXRInteractorStyle);

//------------------------------------------------------------------------------
void vtkOpenXRInteractorStyle::SetupActions(vtkRenderWindowInteractor* iren)
{
  vtkOpenXRRenderWindowInteractor* oiren = vtkOpenXRRenderWindowInteractor::SafeDownCast(iren);

  if (oiren)
  {
    oiren->AddAction("startmovement", vtkCommand::ViewerMovement3DEvent);
    oiren->AddAction("movement", vtkCommand::ViewerMovement3DEvent);
    oiren->AddAction("nextcamerapose", vtkCommand::NextPose3DEvent);
    oiren->AddAction("triggeraction", vtkCommand::Select3DEvent);
    oiren->AddAction("positionprop", vtkCommand::PositionProp3DEvent);
    oiren->AddAction("showmenu", vtkCommand::Menu3DEvent);
  }
}
