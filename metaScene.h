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
#  define ITKMetaIO_METASCENE_H

#  include "metaUtils.h"
#  include "metaObject.h"

#  ifdef _MSC_VER
#    pragma warning(disable : 4251)
#  endif

#  include <list>


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

#  if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE
{
#  endif

class METAIO_EXPORT MetaScene : public MetaObject
{

  // PUBLIC
public:
  typedef std::list<MetaObject *> ObjectListType;

  // Constructors & Destructor
  MetaScene();

  explicit MetaScene(const MetaScene * _scene);

  explicit MetaScene(unsigned int dim);

  ~MetaScene() override;

  void
  PrintInfo() const override;

  void
  CopyInfo(const MetaObject * _object) override;

  void
  AddObject(MetaObject * object);

  // This function only reads registered tubes
  bool
  Read(const char * _headerName = nullptr) override;

  bool
  Write(const char * _headName = nullptr) override;

  bool
  Append(const char * = nullptr) override
  {
    std::cout << "Not Implemented !" << std::endl;
    return true;
  }

  void
  Clear() override;


  //    NObjects(...)
  //       Required Field
  //       Number of points which compose the tube
  void
  NObjects(int nobjects);
  int
  NObjects() const;


  ObjectListType *
  GetObjectList()
  {
    return &m_ObjectList;
  }

  // PROTECTED
protected:
  bool m_ElementByteOrderMSB{};

  void
  M_SetupReadFields() override;

  void
  M_SetupWriteFields() override;

  bool
  M_Read() override;

  bool
  M_Write() override;

  int m_NObjects{}; // "NObjects = "         0

  ObjectListType m_ObjectList;
};

#  if (METAIO_USE_NAMESPACE)
};
#  endif

#endif
