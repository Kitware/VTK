/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
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

  MetaEvent(){m_Level = -1;}
  virtual ~MetaEvent(){}

  virtual void SetCurrentIteration(unsigned int n) {m_CurrentIteration = n;}
  virtual void StartReading(unsigned int n)
    {
    m_NumberOfIterations = n;
    m_Level++;
    }
  virtual void StopReading()
    {
    m_Level--;
    }

protected:

  unsigned int m_CurrentIteration;
  unsigned int m_NumberOfIterations;
  int m_Level;

};

#if (METAIO_USE_NAMESPACE)
};
#endif


#endif
