/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationExecutivePortVectorKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkInformationExecutivePortVectorKey
 * @brief   Key for vtkExecutive/Port value pair vectors.
 *
 * vtkInformationExecutivePortVectorKey is used to represent keys in
 * vtkInformation for values that are vectors of vtkExecutive
 * instances paired with port numbers.
*/

#ifndef vtkInformationExecutivePortVectorKey_h
#define vtkInformationExecutivePortVectorKey_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkFilteringInformationKeyManager.h" // Manage instances of this type.

class vtkExecutive;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkInformationExecutivePortVectorKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationExecutivePortVectorKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  vtkInformationExecutivePortVectorKey(const char* name, const char* location);
  ~vtkInformationExecutivePortVectorKey() VTK_OVERRIDE;

  /**
   * This method simply returns a new vtkInformationExecutivePortVectorKey,
   * given a name and a location. This method is provided for wrappers. Use
   * the constructor directly from C++ instead.
   */
  static vtkInformationExecutivePortVectorKey* MakeKey(const char* name, const char* location)
  {
    return new vtkInformationExecutivePortVectorKey(name, location);
  }

  //@{
  /**
   * Get/Set the value associated with this key in the given
   * information object.
   */
  void Append(vtkInformation* info, vtkExecutive* executive, int port);
  void Remove(vtkInformation* info, vtkExecutive* executive, int port);
  void Set(vtkInformation* info, vtkExecutive** executives, int* ports, int length);
  vtkExecutive** GetExecutives(vtkInformation* info);
  int* GetPorts(vtkInformation* info);
  void Get(vtkInformation* info, vtkExecutive** executives, int* ports);
  int Length(vtkInformation* info);
  //@}

  /**
   * Copy the entry associated with this key from one information
   * object to another.  If there is no entry in the first information
   * object for this key, the value is removed from the second.
   */
  void ShallowCopy(vtkInformation* from, vtkInformation* to) VTK_OVERRIDE;

  /**
   * Remove this key from the given information object.
   */
  void Remove(vtkInformation* info) VTK_OVERRIDE;

  /**
   * Report a reference this key has in the given information object.
   */
  void Report(vtkInformation* info, vtkGarbageCollector* collector) VTK_OVERRIDE;

  /**
   * Print the key's value in an information object to a stream.
   */
  void Print(ostream& os, vtkInformation* info) VTK_OVERRIDE;

protected:

  //@{
  /**
   * Get the address at which the actual value is stored.  This is
   * meant for use from a debugger to add watches and is therefore not
   * a public method.
   */
  vtkExecutive** GetExecutivesWatchAddress(vtkInformation* info);
  int* GetPortsWatchAddress(vtkInformation* info);
  //@}

private:
  vtkInformationExecutivePortVectorKey(const vtkInformationExecutivePortVectorKey&) VTK_DELETE_FUNCTION;
  void operator=(const vtkInformationExecutivePortVectorKey&) VTK_DELETE_FUNCTION;
};

#endif
