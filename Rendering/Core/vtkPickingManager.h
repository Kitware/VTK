/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPickingManager.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*==============================================================================

  Library: MSVTK

  Copyright (c) Kitware Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

/**
 * @class   vtkPickingManager
 * Class defines API to manage the picking process.
 *
 * The Picking Manager (PM) coordinates picking across widgets simultaneously.
 * It maintains a collection of registered pickers;
 * when the manager is picked (e.g. vtkPickingManager::Pick()),
 * a pick is run on each picker but only the best picker
 * (e.g. closest to camera point) is selected.
 * It finally returns the widget/representation or picker that was
 * selected.
 * @warning
 * Every time a vtkWidget and/or a vtkWidgetRepresentation is instantiated,
 * it automatically registers its picker(s) and start being managed by
 * delegating all its pick calls to the picking manager.
 * It is possible to customize with the management in two ways:
 * * at the widget level, the "ManagesPicking" variable can be changed
 * from the widget/representation class to tell
 * whether to use the manager or not.
 * * Directly disable the picking manager itself  with the boolean variable
 * \sa Enabled using vtkPickingManager::EnabledOn(), EnabledOff(),
 * SetEnabled(bool).
 * @par Important:
 * The picking manager is not active by default as it slightly reduces the
 * performances when interacting with the scene.
 * @par Important:
 * When registering pickers, a null object is considered valid because we can
 * managed picker without any associated object.
 * It is really important to note that a null object is different from one
 * to an other !!
 * This has been done to allow adding multiple times the same picker to the manager
 * by not passing the referenced object to not force the supression of all pickers
*/

#ifndef vtkPickingManager_h
#define vtkPickingManager_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro

class vtkAbstractPicker;
class vtkAbstractPropPicker;
class vtkAssemblyPath;
class vtkRenderer;
class vtkRenderWindowInteractor;

class VTKRENDERINGCORE_EXPORT vtkPickingManager : public vtkObject
{
public:
  static vtkPickingManager *New();
  vtkTypeMacro(vtkPickingManager,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Enable/Disable management.
   * When disabled, it redirects every pick on the picker.
   * By default the picking manager is disabled when initialized.
   */
  vtkBooleanMacro(Enabled, bool);
  vtkSetMacro(Enabled, bool);
  vtkGetMacro(Enabled, bool);
  //@}

  //@{
  /**
   * Enable/Disable optimization depending on the renderWindowInteractor events.
   * The mechanism keeps in cache the last selected picker as well as the last
   * render time to recompute the selection only if a new render event
   * occurred after the last selection; otherwise, it simply returns the last
   * picker selected.
   * By default pickingManagers does use the optimization.
   * Warning: Turning off the caching significantly decreases performance.
   */
  void SetOptimizeOnInteractorEvents(bool optimize);
  vtkGetMacro(OptimizeOnInteractorEvents, bool);
  //@}

  //@{
  /**
   * Set the window interactor associated with the manager.
   */
  void SetInteractor(vtkRenderWindowInteractor* iren);
  vtkGetMacro(Interactor, vtkRenderWindowInteractor*);
  //@}

  /**
   * Register a picker into the picking manager.
   * It can be internally associated (optional) with an \a object.
   * This allows the removal of all the pickers of the given object.
   * Note that a picker can be registered multiple times with different objects.
   * \sa RemovePicker(), RemoveObject().
   */
  void AddPicker(vtkAbstractPicker* picker, vtkObject* object = 0);

  /**
   * Unregister the \a picker from the picking manager.
   * If \a object is non null, only the pair (\a picker, \a object) is removed.
   */
  void RemovePicker(vtkAbstractPicker* picker, vtkObject* object = 0);

  /**
   * Remove all occurrence of the \a object from the registered list.
   * If a picker associated with the \a object is not also associated with
   * any other object, it is removed from the list as well.
   */
  void RemoveObject(vtkObject* object);

  /**
   * Run the picking selection process and return true if the \a object
   * is associated with the given picker if it is the best one, false otherwise.
   * If OptimizeOnInteractorEvents is true, the pick can reuse cached
   * information.
   */
  bool Pick(vtkAbstractPicker* picker, vtkObject* object);

  /**
   * Run the picking selection process and return true if the \a object
   * is associated with the best picker.
   * This is an overloaded function.
   */
  bool Pick(vtkObject* object);

  /**
   * Run the picking selection process and return if \a picker is the one
   * selected.
   * This is an overloaded function.
   */
  bool Pick(vtkAbstractPicker* picker);

  /**
   * If the picking manager is enabled, it runs the picking selection process
   * and return the assembly path associated to the picker passed as
   * argument if it is the one mediated.
   * Otherwise it simply proceeds to a pick using the given renderer and
   * returns the corresponding assembly path.
   */
  vtkAssemblyPath* GetAssemblyPath(double X, double Y, double Z,
                                   vtkAbstractPropPicker* picker,
                                   vtkRenderer* renderer,
                                   vtkObject* obj);

  /**
   * Return the number of pickers registered.
   * If the same picker is added multiple times with different objects, it is
   * counted once.
   */
  int GetNumberOfPickers();

  /**
   * Return the number of objects linked with a given \a picker.
   * Note: a null object is counted as an associated object.
   */
  int GetNumberOfObjectsLinked(vtkAbstractPicker* picker);

protected:
  vtkPickingManager();
  ~vtkPickingManager() VTK_OVERRIDE;

  // Used to associate the manager with the interactor
  vtkRenderWindowInteractor* Interactor;
  bool Enabled;
  bool OptimizeOnInteractorEvents;

private:
  vtkPickingManager(const vtkPickingManager&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPickingManager&) VTK_DELETE_FUNCTION;

  class vtkInternal;
  vtkInternal* Internal;
};

#endif
