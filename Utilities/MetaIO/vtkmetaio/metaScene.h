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

#ifndef ITKMetaIO_METASCENE_H
#define ITKMetaIO_METASCENE_H

#include "metaUtils.h"
#include "metaObject.h"

#ifdef _MSC_VER
#pragma warning ( disable: 4251 )
#endif

#include <list>


/*!    MetaScene (.h and .cpp)
 *
 * Description:
 *    Reads and Writes MetaTubeFiles.
 *
 * \author Julien Jomier
 *
 * \date July, 2002
 *
 */

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

class METAIO_EXPORT MetaScene : public MetaObject
  {

  /////
  //
  // PUBLIC
  //
  ////
  public:

   typedef METAIO_STL::list<MetaObject*>    ObjectListType;

   ////
    //
    // Constructors & Destructor
    //
    ////
    MetaScene(void);

    MetaScene(const MetaScene *_scene);

    MetaScene(unsigned int dim);

    ~MetaScene(void) MET_OVERRIDE;

    void PrintInfo(void) const MET_OVERRIDE;

    void CopyInfo(const MetaObject * _object) MET_OVERRIDE;

    void AddObject(MetaObject* object);

    //
    //
    //
    // This function only reads registered tubes
    bool Read(const char *_headerName=NULL);

    bool Write(const char *_headName=NULL);

    bool Append(const char* =NULL) MET_OVERRIDE {METAIO_STREAM::cout << "Not Implemented !" << METAIO_STREAM::endl;return true;}

    void  Clear(void) MET_OVERRIDE;


    //    NObjects(...)
    //       Required Field
    //       Number of points wich compose the tube
    void  NObjects(int nobjects);
    int   NObjects(void) const;


    ObjectListType * GetObjectList(void) {return & m_ObjectList;}

  ////
  //
  // PROTECTED
  //
  ////
  protected:

    bool  m_ElementByteOrderMSB;

    void  M_Destroy(void) MET_OVERRIDE;

    void  M_SetupReadFields(void) MET_OVERRIDE;

    void  M_SetupWriteFields(void) MET_OVERRIDE;

    bool  M_Read(void) MET_OVERRIDE;

    bool  M_Write(void) MET_OVERRIDE;

    int m_NObjects;      // "NObjects = "         0

    ObjectListType    m_ObjectList;

  };

#if (METAIO_USE_NAMESPACE)
};
#endif

#endif
