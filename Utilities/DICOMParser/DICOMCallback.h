/*=========================================================================

  Program:   DICOMParser
  Module:    DICOMCallback.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2003 Matt Turek
  All rights reserved.
  See Copyright.txt for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __DICOM_CALLBACK_H_
#define __DICOM_CALLBACK_H_

#ifdef _MSC_VER
#pragma warning ( disable : 4514 )
#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4503 )
#pragma warning ( disable : 4710 )
#pragma warning ( disable : 4702 )
#pragma warning ( push, 3 )
#endif

#include "DICOMConfig.h"
#include "DICOMParser.h"

//
// Pure virtual class that specifies the interface
// for a DICOMCallback.
//
// The DICOMParser allows a vector of callbacks to
// be specified for each group element tag.  When
// a group, element with a registered callback is
// encountered, the callback is called and passed
// the group, element, type, data, and data length.
//

class DICOM_EXPORT DICOMCallback
{
 public:
  virtual ~DICOMCallback() {};
  virtual void Execute(DICOMParser *parser,
                       doublebyte group,
                       doublebyte element,
                       DICOMParser::VRTypes type,
                       unsigned char* val,
                       quadbyte len) = 0;
};

//
// Subclass of DICOMCallback which can be used
// with member functions.
//
template <class T>
class DICOMMemberCallback : public DICOMCallback
{
 public:
  typedef  void (T::*TMemberFunctionPointer)(DICOMParser *parser,
                                             doublebyte group,
                                             doublebyte element,
                                             DICOMParser::VRTypes type,
                                             unsigned char* val,
                                             quadbyte len);


  //
  // Method to set the object and member function pointers
  // that will be called in the callback.
  //
  void SetCallbackFunction(T* object,
                           TMemberFunctionPointer memberFunction)
  {
    ObjectThis = object;
    MemberFunction = memberFunction;
  }

  //
  // Execute method implementation from DICOMCallback.
  //
  void Execute(DICOMParser *parser,
               doublebyte group,
               doublebyte element,
               DICOMParser::VRTypes type,
               unsigned char* val,
               quadbyte len)
  {
    if (MemberFunction)
      {
      ((*ObjectThis).*(MemberFunction))(parser, group, element, type, val,len);
      }
  }

 protected:
  T* ObjectThis;
  TMemberFunctionPointer MemberFunction;

};

#ifdef _MSC_VER
#pragma warning ( pop )
#endif

#endif

