// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

VTK_ABI_NAMESPACE_BEGIN
class vtkExecutive;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkInformationExecutivePortVectorKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationExecutivePortVectorKey, vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkInformationExecutivePortVectorKey(const char* name, const char* location);
  ~vtkInformationExecutivePortVectorKey() override;

  /**
   * This method simply returns a new vtkInformationExecutivePortVectorKey,
   * given a name and a location. This method is provided for wrappers. Use
   * the constructor directly from C++ instead.
   */
  static VTK_NEWINSTANCE vtkInformationExecutivePortVectorKey* MakeKey(
    const char* name, const char* location)
  {
    return new vtkInformationExecutivePortVectorKey(name, location);
  }

  ///@{
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
  ///@}

  /**
   * Copy the entry associated with this key from one information
   * object to another.  If there is no entry in the first information
   * object for this key, the value is removed from the second.
   */
  void ShallowCopy(vtkInformation* from, vtkInformation* to) override;

  /**
   * Remove this key from the given information object.
   */
  void Remove(vtkInformation* info) override;

  /**
   * Report a reference this key has in the given information object.
   */
  void Report(vtkInformation* info, vtkGarbageCollector* collector) override;

  /**
   * Print the key's value in an information object to a stream.
   */
  void Print(ostream& os, vtkInformation* info) override;

protected:
  ///@{
  /**
   * Get the address at which the actual value is stored.  This is
   * meant for use from a debugger to add watches and is therefore not
   * a public method.
   */
  vtkExecutive** GetExecutivesWatchAddress(vtkInformation* info);
  int* GetPortsWatchAddress(vtkInformation* info);
  ///@}

private:
  vtkInformationExecutivePortVectorKey(const vtkInformationExecutivePortVectorKey&) = delete;
  void operator=(const vtkInformationExecutivePortVectorKey&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
