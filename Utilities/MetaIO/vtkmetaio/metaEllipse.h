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

#ifndef ITKMetaIO_METAELLIPSE_H
#define ITKMetaIO_METAELLIPSE_H

#include "metaUtils.h"
#include "metaObject.h"

#include <list>


/*!    MetaEllipse (.h and .cpp)
 *
 * Description:
 *    Reads and Writes MetaEllipseFiles.
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


class METAIO_EXPORT MetaEllipse : public MetaObject
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
    MetaEllipse(void);

    MetaEllipse(const char *_headerName);

    MetaEllipse(const MetaEllipse *_ellipse);

    MetaEllipse(unsigned int dim);

    ~MetaEllipse(void) MET_OVERRIDE;

    void PrintInfo(void) const MET_OVERRIDE;

    void CopyInfo(const MetaObject * _object) MET_OVERRIDE;

    void  Clear(void) MET_OVERRIDE;

    void  Radius(const float* radius);
    void  Radius(float radius);
    void  Radius(float r1,float r2);
    void  Radius(float r1,float r2, float r3);
    const float* Radius(void) const;


  ////
  //
  // PROTECTED
  //
  ////
  protected:

    void  M_Destroy(void) MET_OVERRIDE;

    void  M_SetupReadFields(void) MET_OVERRIDE;

    void  M_SetupWriteFields(void) MET_OVERRIDE;

    bool  M_Read(void) MET_OVERRIDE;

    float m_Radius[100];  // "Radius = "     0

  };

#if (METAIO_USE_NAMESPACE)
};
#endif


#endif
