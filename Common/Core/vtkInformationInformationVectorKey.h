/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationInformationVectorKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkInformationInformationVectorKey
 * @brief   Key for vtkInformation vectors.
 *
 * vtkInformationInformationVectorKey is used to represent keys in
 * vtkInformation for vectors of other vtkInformation objects.
*/

#ifndef vtkInformationInformationVectorKey_h
#define vtkInformationInformationVectorKey_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

class vtkInformationVector;

class VTKCOMMONCORE_EXPORT vtkInformationInformationVectorKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationInformationVectorKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkInformationInformationVectorKey(const char* name, const char* location);
  ~vtkInformationInformationVectorKey() override;

  //@{
  /**
   * Get/Set the value associated with this key in the given
   * information object.
   */
  void Set(vtkInformation* info, vtkInformationVector*);
  vtkInformationVector* Get(vtkInformation* info);
  //@}

  /**
   * Copy the entry associated with this key from one information
   * object to another.  If there is no entry in the first information
   * object for this key, the value is removed from the second.
   */
  void ShallowCopy(vtkInformation* from, vtkInformation* to) override;

  /**
   * Duplicate (new instance created) the entry associated with this key from
   * one information object to another (new instances of any contained
   * vtkInformation and vtkInformationVector objects are created).
   */
  void DeepCopy(vtkInformation* from, vtkInformation* to) override;

  /**
   * Report a reference this key has in the given information object.
   */
  void Report(vtkInformation* info, vtkGarbageCollector* collector) override;

private:
  vtkInformationInformationVectorKey(const vtkInformationInformationVectorKey&) = delete;
  void operator=(const vtkInformationInformationVectorKey&) = delete;
};

#endif
