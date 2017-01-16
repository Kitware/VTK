/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUInfoList.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkGPUInfoList
 * @brief   Stores the list of GPUs VRAM information.
 *
 * vtkGPUInfoList stores a list of vtkGPUInfo. An host can have
 * several GPUs. It creates and sets the list by probing the host with system
 * calls. This an abstract class. Concrete classes are OS specific.
 * @sa
 * vtkGPUInfo vtkDirectXGPUInfoList vtkCoreGraphicsGPUInfoList
*/

#ifndef vtkGPUInfoList_h
#define vtkGPUInfoList_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkGPUInfoListArray; // STL Pimpl
class vtkGPUInfo;

class VTKRENDERINGCORE_EXPORT vtkGPUInfoList : public vtkObject
{
public:
  static vtkGPUInfoList *New();
  vtkTypeMacro(vtkGPUInfoList, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Build the list of vtkInfoGPU if not done yet.
   * Default implementation created an empty list. Useful if there is no
   * implementation available for a given architecture yet.
   * \post probed: IsProbed()
   */
  virtual void Probe() = 0;

  /**
   * Tells if the operating system has been probed. Initial value is false.
   */
  virtual bool IsProbed();

  /**
   * Return the number of GPUs.
   * \pre probed: IsProbed()
   */
  virtual int GetNumberOfGPUs();

  /**
   * Return information about GPU i.
   * \pre probed: IsProbed()
   * \pre valid_index: i>=0 && i<GetNumberOfGPUs()
   * \post result_exists: result!=0
   */
  virtual vtkGPUInfo *GetGPUInfo(int i);

protected:
  //@{
  /**
   * Default constructor. Set Probed to false. Set Array to NULL.
   */
  vtkGPUInfoList();
  ~vtkGPUInfoList() VTK_OVERRIDE;
  //@}

  bool Probed;
  vtkGPUInfoListArray *Array;

private:
  vtkGPUInfoList(const vtkGPUInfoList&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGPUInfoList&) VTK_DELETE_FUNCTION;
};

#endif
