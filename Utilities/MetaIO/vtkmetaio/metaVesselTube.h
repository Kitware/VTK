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

#ifndef ITKMetaIO_METAVESSELTUBE_H
#define ITKMetaIO_METAVESSELTUBE_H

#include "metaUtils.h"
#include "metaObject.h"
#include "metaTube.h"

#ifdef _MSC_VER
#pragma warning ( disable: 4251 )
#endif

#include <list>


/*!    MetaVesselTube (.h and .cpp)
 *
 * Description:
 *    Reads and Writes MetaVesselTubeFiles.
 *
 * \author Julien Jomier
 *
 * \date May 22, 2002
 */

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

class METAIO_EXPORT VesselTubePnt
  : public TubePnt
{
public:
  VesselTubePnt(int dim);
  ~VesselTubePnt();
};




class METAIO_EXPORT MetaVesselTube : public MetaTube
{
  /////
  //
  // PUBLIC
  //
  ////
  public:

    MetaVesselTube(void);

    MetaVesselTube(const char *_headerName);

    MetaVesselTube(const MetaVesselTube *_VesselTube);

    MetaVesselTube(unsigned int dim);

    ~MetaVesselTube(void) override;

    void Clear(void) override;

    void PrintInfo(void) const override;
};

#if (METAIO_USE_NAMESPACE)
};
#endif

#endif
