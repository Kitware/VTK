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

#ifndef ITKMetaIO_METABLOB_H
#define ITKMetaIO_METABLOB_H

#include "metaUtils.h"
#include "metaObject.h"

#ifdef _MSC_VER
#pragma warning ( disable: 4251 )
#endif

#include <list>


/*!    MetaBlob (.h and .cxx)
 *
 * Description:
 *    Reads and Writes MetaBlobFiles.
 *
 * \author Julien Jomier
 * 
 * \date July 02, 2002
 * 
 * Depends on:
 *    MetaUtils.h
 *    MetaFileLib.h
 */

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

class METAIO_EXPORT BlobPnt
{
public:

  BlobPnt(int dim);
  ~BlobPnt();
  
  unsigned int m_Dim;
  float* m_X;
  float  m_Color[4];
};




class METAIO_EXPORT MetaBlob : public MetaObject
  {

  /////
  //
  // PUBLIC
  //
  ////
  public:

   typedef METAIO_STL::list<BlobPnt*> PointListType;
    ////
    //
    // Constructors & Destructor
    //
    ////
    MetaBlob(void);

    MetaBlob(const char *_headerName);   

    MetaBlob(const MetaBlob *_blob); 
    
    MetaBlob(unsigned int dim);

    ~MetaBlob(void);

    void PrintInfo(void) const;

    void CopyInfo(const MetaObject * _object);

    //    NPoints(...)
    //       Required Field
    //       Number of points wich compose the blob
    void  NPoints(int npnt);
    int   NPoints(void) const;

    //    PointDim(...)
    //       Required Field
    //       Definition of points
    void        PointDim(const char* pointDim);
    const char* PointDim(void) const;


    void  Clear(void);

    PointListType & GetPoints(void) {return m_PointList;}
    const PointListType & GetPoints(void) const  {return m_PointList;}
 
    MET_ValueEnumType ElementType(void) const;
    void  ElementType(MET_ValueEnumType _elementType);

  ////
  //
  // PROTECTED
  //
  ////
  protected:

    bool  m_ElementByteOrderMSB;

    void  M_Destroy(void);

    void  M_SetupReadFields(void);

    void  M_SetupWriteFields(void);

    bool  M_Read(void);

    bool  M_Write(void);

    int   m_NPoints;      // "NPoints = "         0

    char m_PointDim[255]; // "PointDim = "       "x y z r"

    PointListType m_PointList;

    MET_ValueEnumType m_ElementType;
  };

#if (METAIO_USE_NAMESPACE)
};
#endif

#endif
