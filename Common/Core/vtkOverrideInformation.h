// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOverrideInformation
 * @brief   Factory object override information
 *
 * vtkOverrideInformation is used to represent the information about
 * a class which is overridden in a vtkObjectFactory.
 *
 */

#ifndef vtkOverrideInformation_h
#define vtkOverrideInformation_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkObjectFactory;

class VTKCOMMONCORE_EXPORT vtkOverrideInformation : public vtkObject
{
public:
  static vtkOverrideInformation* New();
  vtkTypeMacro(vtkOverrideInformation, vtkObject);
  /**
   * Print ObjectFactor to stream.
   */
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns the name of the class being overridden.  For example,
   * if you had a factory that provided an override for
   * vtkVertex, then this function would return "vtkVertex"
   */
  const char* GetClassOverrideName() { return this->ClassOverrideName; }

  /**
   * Returns the name of the class that will override the class.
   * For example, if you had a factory that provided an override for
   * vtkVertex called vtkMyVertex, then this would return "vtkMyVertex"
   */
  const char* GetClassOverrideWithName() { return this->ClassOverrideWithName; }

  /**
   * Return a human readable or GUI displayable description of this
   * override.
   */
  const char* GetDescription() { return this->Description; }

  /**
   * Return the specific object factory that this override occurs in.
   */
  vtkObjectFactory* GetObjectFactory() { return this->ObjectFactory; }

  ///@{
  /**
   * Set the class override name
   */
  vtkSetStringMacro(ClassOverrideName);

  /**
   * Set the class override with name
   */
  vtkSetStringMacro(ClassOverrideWithName);

  /**
   * Set the description
   */
  vtkSetStringMacro(Description);
  ///@}

protected:
  virtual void SetObjectFactory(vtkObjectFactory*);

private:
  vtkOverrideInformation();
  ~vtkOverrideInformation() override;
  // allow the object factory to set the values in this
  // class, but only the object factory

  friend class vtkObjectFactory;

  char* ClassOverrideName;
  char* ClassOverrideWithName;
  char* Description;
  vtkObjectFactory* ObjectFactory;

  vtkOverrideInformation(const vtkOverrideInformation&) = delete;
  void operator=(const vtkOverrideInformation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
