/*=========================================================================

  Program:   MetaIO
  Module:    metaEvent.h
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

#ifndef ITKMetaIO_METAEVENT_H
#define ITKMetaIO_METAEVENT_H


/*!    MetaEvent (.h)
 *
 * Description:
 *    Event abstract class
 *
 * \author Julien Jomier
 * February 20, 2003
 *
 */

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif


class METAIO_EXPORT MetaEvent
{
 
public:

  MetaEvent(){m_Level = -1;};
  virtual ~MetaEvent(){};

  virtual void SetCurrentIteration(unsigned int n) {m_CurrentIteration = n;}
  virtual void StartReading(unsigned int n) 
    {
    m_NumberOfIterations = n;
    m_Level++;
    };
  virtual void StopReading() 
    {
    m_Level--;
    };

protected:

  unsigned int m_CurrentIteration;
  unsigned int m_NumberOfIterations;
  int m_Level;

};

#if (METAIO_USE_NAMESPACE)
};
#endif


#endif
