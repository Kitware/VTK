// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXdmf3ArrayKeeper
 * @brief   LRU cache of XDMF Arrays
 *
 * vtkXdmf3ArrayKeeper maintains the in memory cache of recently used XdmfArrays.
 * Each array that is loaded from XDMF is put in the cache and/or marked with the
 * current timestep. A release method frees arrays that have not been recently
 * used.
 *
 * This file is a helper for the vtkXdmf3Reader and not intended to be
 * part of VTK public API
 */

#ifndef vtkXdmf3ArrayKeeper_h
#define vtkXdmf3ArrayKeeper_h

#include "vtkIOXdmf3Module.h" // For export macro
#include <map>

class XdmfArray;

VTK_ABI_NAMESPACE_BEGIN

#ifdef _MSC_VER
#pragma warning(push)           // save
#pragma warning(disable : 4251) // needs to have dll-interface to be used by clients of class
#endif
class VTKIOXDMF3_EXPORT vtkXdmf3ArrayKeeper : public std::map<XdmfArray*, unsigned int>
{
public:
  /**
   * Constructor
   */
  vtkXdmf3ArrayKeeper();

  /**
   * Destructor
   */
  ~vtkXdmf3ArrayKeeper();

  /**
   * Call to mark arrays that will be accessed with a new timestamp
   */
  void BumpGeneration();

  /**
   * Call whenever you a new XDMF array is accessed.
   */
  void Insert(XdmfArray* val);

  /**
   * Call to free all open arrays that are currently open but not in use.
   * Force argument frees all arrays.
   */
  void Release(bool force);

  vtkXdmf3ArrayKeeper(const vtkXdmf3ArrayKeeper&) = delete;

private:
  unsigned int generation;
};
#ifdef _MSC_VER
#pragma warning(pop) // restore
#endif

VTK_ABI_NAMESPACE_END
#endif // vtkXdmf3ArrayKeeper_h
// VTK-HeaderTest-Exclude: vtkXdmf3ArrayKeeper.h
