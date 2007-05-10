/*=========================================================================

  Program:   MetaIO
  Module:    metaGroup.h
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

#ifndef ITKMetaIO_METAGROUP_H
#define ITKMetaIO_METAGROUP_H

#include "metaUtils.h"
#include "metaObject.h"

#include <list>


/*!    MetaGroup (.h and .cpp)
 *
 * Description:
 *    Reads and Writes MetaGroupFiles.
 *
 * \author Julien Jomier
 * 
 * \date May 22, 2002
 * 
 * Depends on:
 *    MetaUtils.h
 *    MetaObject.h
 */

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

class METAIO_EXPORT MetaGroup : public MetaObject
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
    MetaGroup(void);

    MetaGroup(const char *_headerName);   

    MetaGroup(const MetaGroup *_group);    

    MetaGroup(unsigned int dim);

    ~MetaGroup(void);

    void PrintInfo(void) const;

    void CopyInfo(const MetaObject * _object);

    void  Clear(void);
    

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

  };

#if (METAIO_USE_NAMESPACE)
};
#endif


#endif
