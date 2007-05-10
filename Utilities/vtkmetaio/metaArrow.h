/*=========================================================================

  Program:   MetaIO
  Module:    metaArrow.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "metaTypes.h"

#ifndef ITKMetaIO_METAARROW_H
#define ITKMetaIO_METAARROW_H

#include "metaUtils.h"
#include "metaObject.h"

#include <list>


/*!    MetaArrow (.h and .cpp)
 *
 * Description:
 *    Reads and Writes MetaArrowFiles.
 *
 * \author Julien Jomier
 * 
 * \date Jan 05, 2005
 * 
 * Depends on:
 *    MetaUtils.h
 *    MetaObject.h
 */

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

class METAIO_EXPORT MetaArrow : public MetaObject
  {

  /////
  //
  // PUBLIC
  //
  ////
  public:

    ////
    //
    // Constructors & Destructor
    //
    ////
    MetaArrow(void);

    MetaArrow(const char *_headerName);   

    MetaArrow(const MetaArrow *_Arrow);    

    MetaArrow(unsigned int dim);

    ~MetaArrow(void);

    void PrintInfo(void) const;

    void CopyInfo(const MetaObject * _object);

    void  Clear(void);
    
    void  Length(float length);
    float Length(void) const;

    void  Lenght(float length) {this->Length(length);}
    float Lenght(void) const {return Length();}


  ////
  //
  // PROTECTED
  //
  ////
  protected:

    void  M_Destroy(void);

    void  M_SetupReadFields(void);

    void  M_SetupWriteFields(void);

    bool  M_Read(void);

    float M_Length; // default 1.0

  };

#if (METAIO_USE_NAMESPACE)
};
#endif

#endif
