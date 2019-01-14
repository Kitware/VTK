/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfGridController.hpp                                              */
/*                                                                           */
/*  Author:                                                                  */
/*     Andrew Burns                                                          */
/*     andrew.j.burns2@arl.army.mil                                          */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2015 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/

#ifndef XDMFGRIDCONTROLLER_HPP_
#define XDMFGRIDCONTROLLER_HPP_

// C Compatible Includes
#include "Xdmf.hpp"

#ifdef __cplusplus

// Forward Declarations
class XdmfGrid;

// Includes
#include <string>
#include "XdmfSharedPtr.hpp"

/**
 * @brief Couples an XdmfGrid with a grid on a different XML file.
 *
 * Serves as an method to reduce memory usage by leaving part of
 * the xdmf tree in file.
 */
class XDMF_EXPORT XdmfGridController : public virtual XdmfItem {

public:

  /**
   * Creates a link to an xdmf tree in another file.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGrid.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setGridController
   * @until //#setGridController
   *
   * Python
   *
   * @dontinclude XdmfExampleGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setGridController
   * @until #//setGridController
   *
   * @param     filePath        
   * @param     xmlPath         
   *
   * @return    A reference to the external xdmf tree
   */
  static shared_ptr<XdmfGridController>
  New(const std::string & filePath,
      const std::string & xmlPath);

  friend class XdmfWriter;
  friend class XdmfGrid;

  virtual ~XdmfGridController();

  static const std::string ItemTag;

  /**
   * Gets the file path of the grid that this reference reads from.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGrid.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setGridController
   * @until //#setGridController
   * @skipline //#getFilePath
   * @until //#getFilePath
   *
   * Python
   *
   * @dontinclude XdmfExampleGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setGridController
   * @until #//setGridController
   * @skipline #//getFilePath
   * @until #//getFilePath
   *
   * @return    The file path.
   */
  std::string getFilePath() const;

  std::map<std::string, std::string> getItemProperties() const;

  virtual std::string getItemTag() const;

  /**
   * Gets the XML path that refers to the base node in the reference file.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGrid.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setGridController
   * @until //#setGridController
   * @skipline //#getXMLPath
   * @until //#getXMLPath
   *
   * Python
   *
   * @dontinclude XdmfExampleGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setGridController
   * @until #//setGridController
   * @skipline #//getXMLPath
   * @until #//getXMLPath
   *
   * @return    The XML path.
   */
  std::string getXMLPath() const;

  /**
   * Reads the grid that his controller references.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfGrid.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setGridController
   * @until //#setGridController
   * @skipline //#controllerRead
   * @until //#controllerRead
   *
   * Python
   *
   * @dontinclude XdmfExampleGrid.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setGridController
   * @until #//setGridController
   * @skipline #//controllerRead
   * @until #//controllerRead
   *
   * @return    The grid read from the controller's stored location
   */
  virtual shared_ptr<XdmfGrid> read();

  XdmfGridController(const XdmfGridController&);

protected:

  XdmfGridController(const std::string & filePath,
                     const std::string & xmlPath);

  const std::string mFilePath;
  const std::string mXMLPath;

private:

//  XdmfGridController(const XdmfGridController&);  // Not implemented.
  void operator=(const XdmfGridController &);  // Not implemented.

};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

#ifndef XDMFGRIDCDEFINE
#define XDMFGRIDCDEFINE
struct XDMFGRID; // Simply as a typedef to ensure correct typing
typedef struct XDMFGRID XDMFGRID;
#endif

struct XDMFGRIDCONTROLLER; // Simply as a typedef to ensure correct typing
typedef struct XDMFGRIDCONTROLLER XDMFGRIDCONTROLLER;

XDMF_EXPORT XDMFGRIDCONTROLLER * XdmfGridControllerNew(char * filePath,
                                                       char * xmlPath);

XDMF_EXPORT char * XdmfGridControllerGetFilePath(XDMFGRIDCONTROLLER * controller);

XDMF_EXPORT char * XdmfGridControllerGetXMLPath(XDMFGRIDCONTROLLER * controller);

XDMF_EXPORT XDMFGRID * XdmfGridControllerRead(XDMFGRIDCONTROLLER * controller);

XDMF_ITEM_C_CHILD_DECLARE(XdmfGridController, XDMFGRIDCONTROLLER, XDMF)

#ifdef __cplusplus
}
#endif

#endif /* XDMFGRIDCONTROLLER_HPP_ */
