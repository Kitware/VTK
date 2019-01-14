/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationUnsignedLongKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkInformationUnsignedLongKey
 * @brief   Key for unsigned long values in vtkInformation.
 *
 * vtkInformationUnsignedLongKey is used to represent keys for unsigned long values
 * in vtkInformation.
*/

#ifndef vtkInformationUnsignedLongKey_h
#define vtkInformationUnsignedLongKey_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

class VTKCOMMONCORE_EXPORT vtkInformationUnsignedLongKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationUnsignedLongKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkInformationUnsignedLongKey(const char* name, const char* location);
  ~vtkInformationUnsignedLongKey() override;

  /**
   * This method simply returns a new vtkInformationUnsignedLongKey, given a
   * name and a location. This method is provided for wrappers. Use the
   * constructor directly from C++ instead.
   */
  static vtkInformationUnsignedLongKey* MakeKey(const char* name, const char* location)
  {
    return new vtkInformationUnsignedLongKey(name, location);
  }

  //@{
  /**
   * Get/Set the value associated with this key in the given
   * information object.
   */
  void Set(vtkInformation* info, unsigned long);
  unsigned long Get(vtkInformation* info);
  //@}

  /**
   * Copy the entry associated with this key from one information
   * object to another.  If there is no entry in the first information
   * object for this key, the value is removed from the second.
   */
  void ShallowCopy(vtkInformation* from, vtkInformation* to) override;

  /**
   * Print the key's value in an information object to a stream.
   */
  void Print(ostream& os, vtkInformation* info) override;

protected:
  /**
   * Get the address at which the actual value is stored.  This is
   * meant for use from a debugger to add watches and is therefore not
   * a public method.
   */
  unsigned long* GetWatchAddress(vtkInformation* info);

private:
  vtkInformationUnsignedLongKey(const vtkInformationUnsignedLongKey&) = delete;
  void operator=(const vtkInformationUnsignedLongKey&) = delete;
};

#endif
