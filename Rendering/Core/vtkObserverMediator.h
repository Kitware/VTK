/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObserverMediator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkObserverMediator
 * @brief   manage contention for cursors and other resources
 *
 * The vtkObserverMediator is a helper class that manages requests for
 * cursor changes from multiple interactor observers (e.g. widgets). It keeps
 * a list of widgets (and their priorities) and their current requests for
 * cursor shape. It then satisfies requests based on widget priority and the
 * relative importance of the request (e.g., a lower priority widget
 * requesting a particular cursor shape will overrule a higher priority
 * widget requesting a default shape).
 *
 * @sa
 * vtkAbstractWidget vtkWidgetRepresentation
*/

#ifndef vtkObserverMediator_h
#define vtkObserverMediator_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkRenderWindowInteractor;
class vtkInteractorObserver;
class vtkObserverMap;


class VTKRENDERINGCORE_EXPORT vtkObserverMediator : public vtkObject
{
public:
  /**
   * Instantiate the class.
   */
  static vtkObserverMediator *New();

  //@{
  /**
   * Standard macros.
   */
  vtkTypeMacro(vtkObserverMediator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Specify the instance of vtkRenderWindow whose cursor shape is
   * to be managed.
   */
  void SetInteractor(vtkRenderWindowInteractor* iren);
  vtkGetObjectMacro(Interactor, vtkRenderWindowInteractor);
  //@}

  /**
   * Method used to request a cursor shape. Note that the shape is specified
   * using one of the integral values determined in vtkRenderWindow.h. The
   * method returns a non-zero value if the shape was successfully changed.
   */
  int RequestCursorShape(vtkInteractorObserver*, int cursorShape);

  /**
   * Remove all requests for cursor shape from a given interactor.
   */
  void RemoveAllCursorShapeRequests(vtkInteractorObserver*);

protected:
  vtkObserverMediator();
  ~vtkObserverMediator() VTK_OVERRIDE;

  // The render window whose cursor we are controlling
  vtkRenderWindowInteractor *Interactor;

  // A map whose key is the observer*, and whose data value is a cursor
  // request. Note that a special compare function is used to sort the
  // widgets based on the observer's priority.
  vtkObserverMap *ObserverMap; //given a widget*, return its data

  // The current observer and cursor shape
  vtkInteractorObserver *CurrentObserver;
  int                    CurrentCursorShape;

private:
  vtkObserverMediator(const vtkObserverMediator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkObserverMediator&) VTK_DELETE_FUNCTION;
};

#endif
