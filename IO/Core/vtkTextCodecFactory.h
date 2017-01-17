/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkTextCodecFactory.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   vtkTextCodecFactory
 * @brief   maintain a list of text codecs and return instances
 *
 *
 * A single class to hold registered codecs and return instances of them based
 * on either a decriptive name (UTF16 or latin-1) or by asking who can handle a
 * given std::vector<unsigned char>
 *
 * @par Thanks:
 * Thanks to Tim Shed from Sandia National Laboratories for his work
 * on the concepts and to Marcus Hanwell and Jeff Baumes of Kitware for
 * keeping me out of the weeds
 *
 * @sa
 * vtkTextCodec
 *
*/

#ifndef vtkTextCodecFactory_h
#define vtkTextCodecFactory_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkTextCodec;

class VTKIOCORE_EXPORT vtkTextCodecFactory : public vtkObject
{
public:
  vtkTypeMacro(vtkTextCodecFactory, vtkObject);
  static vtkTextCodecFactory* New() ;
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Type for Creation callback.
   */
  typedef vtkTextCodec* (*CreateFunction)();

  //@{
  /**
   * Provides mechanism to register/unregister additional callbacks to create
   * concrete subclasses of vtkTextCodecFactory to handle different protocols.
   * The registered callbacks are tried in the order they are registered.
   */
  static void RegisterCreateCallback(CreateFunction callback);
  static void UnRegisterCreateCallback(CreateFunction callback);
  static void UnRegisterAllCreateCallbacks();
  //@}

  /**
   * Given a codec/storage name try to find one of our registered codecs that
   * can handle it.  This is non-deterministic, very messy and should not be
   * your first thing to try.
   * The registered callbacks are tried in the order they are registered.
   */
  static vtkTextCodec* CodecForName(const char* CodecName);

  /**
   * Given a snippet of the stored data name try to find one of our registered
   * codecs that can handle transforming it into unicode.
   * The registered callbacks are tried in the order they are registered.
   */
  static vtkTextCodec* CodecToHandle(istream& InputStream);

  /**
   * Initialize core text codecs - needed for the static compilation case.
   */
  static void Initialize();

protected:
  vtkTextCodecFactory();
  ~vtkTextCodecFactory() VTK_OVERRIDE;

private:
  vtkTextCodecFactory(const vtkTextCodecFactory &) VTK_DELETE_FUNCTION;
  void operator=(const vtkTextCodecFactory &) VTK_DELETE_FUNCTION;

  //@{
  /**
   * Data structure used to store registered callbacks.
   */
  class CallbackVector;
  static CallbackVector* Callbacks;
  //@}

};

#endif // vtkTextCodecFactory_h
