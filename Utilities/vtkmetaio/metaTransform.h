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

#ifndef ITKMetaIO_METATRANSFORM_H
#define ITKMetaIO_METATRANSFORM_H

#include "metaUtils.h"
#include "metaObject.h"

#include <list>


/*!    MetaTransform (.h and .cpp)
 *
 * Description:
 *    Reads and Writes MetaTransformFiles.
 *
 * \author Julien Jomier
 * 
 * \date Feb 14, 2005
 * 
 * Depends on:
 *    MetaUtils.h
 *    MetaObject.h
 */

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif


class METAIO_EXPORT MetaTransform : public MetaObject
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
    MetaTransform(void);

    MetaTransform(const char *_headerName);   

    MetaTransform(const MetaTransform *_group);    

    MetaTransform(unsigned int dim);

    ~MetaTransform(void);

    void PrintInfo(void) const;

    void CopyInfo(const MetaObject * _object);

    void  Clear(void);

    // Set/Get the parameters of the transforms
    const double * Parameters(void) const;
    void  Parameters(unsigned int dimension, const double * _parameters);

    unsigned int NParameters() {return parametersDimension;}

    unsigned int TransformOrder() {return transformOrder;}
    void TransformOrder(unsigned int order) {transformOrder = order;}

    // Set/Get the grid spacing
    const double * GridSpacing(void) const;
    void  GridSpacing(const double * _gridSize);

   // Set/Get the grid origin
    const double * GridOrigin(void) const;
    void  GridOrigin(const double * _gridOrigin);

    // Set/Get the grid region size
    const double * GridRegionSize(void) const;
    void  GridRegionSize(const double * _gridRegionSize);

    // Set/Get the grid region index
    const double * GridRegionIndex(void) const;
    void  GridRegionIndex(const double * _gridRegionIndex);

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
    bool  M_Write(void);

    double* parameters;
    unsigned int parametersDimension;
    unsigned int transformOrder;

  // This ivars are used for the BSplineTransform
  double gridSpacing[100];
  double gridOrigin[100];
  double gridRegionSize[100];
  double gridRegionIndex[100];

  };

#if (METAIO_USE_NAMESPACE)
};
#endif


#endif
