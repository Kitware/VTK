/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationIntegerVectorKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkInformationIntegerVectorKey
 * @brief   Key for integer vector values.
 *
 * vtkInformationIntegerVectorKey is used to represent keys for integer
 * vector values in vtkInformation.h
*/

#ifndef vtkInformationIntegerVectorKey_h
#define vtkInformationIntegerVectorKey_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

class VTKCOMMONCORE_EXPORT vtkInformationIntegerVectorKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationIntegerVectorKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  vtkInformationIntegerVectorKey(const char* name, const char* location,
                                 int length=-1);
  ~vtkInformationIntegerVectorKey() VTK_OVERRIDE;

  /**
   * This method simply returns a new vtkInformationIntegerVectorKey, given a
   * name, a location and a required length. This method is provided for
   * wrappers. Use the constructor directly from C++ instead.
   */
  static vtkInformationIntegerVectorKey* MakeKey(const char* name, const char* location,
    int length=-1)
  {
    return new vtkInformationIntegerVectorKey(name, location, length);
  }

  //@{
  /**
   * Get/Set the value associated with this key in the given
   * information object.
   */
  void Append(vtkInformation* info, int value);
  void Set(vtkInformation* info, const int* value, int length);
  void Set(vtkInformation* info);
  int* Get(vtkInformation* info);
  int  Get(vtkInformation* info, int idx);
  void Get(vtkInformation* info, int* value);
  int Length(vtkInformation* info);
  //@}

  /**
   * Copy the entry associated with this key from one information
   * object to another.  If there is no entry in the first information
   * object for this key, the value is removed from the second.
   */
  void ShallowCopy(vtkInformation* from, vtkInformation* to) VTK_OVERRIDE;

  /**
   * Print the key's value in an information object to a stream.
   */
  void Print(ostream& os, vtkInformation* info) VTK_OVERRIDE;

protected:
  // The required length of the vector value (-1 is no restriction).
  int RequiredLength;

  /**
   * Get the address at which the actual value is stored.  This is
   * meant for use from a debugger to add watches and is therefore not
   * a public method.
   */
  int* GetWatchAddress(vtkInformation* info);

private:
  vtkInformationIntegerVectorKey(const vtkInformationIntegerVectorKey&) VTK_DELETE_FUNCTION;
  void operator=(const vtkInformationIntegerVectorKey&) VTK_DELETE_FUNCTION;
};

#endif
