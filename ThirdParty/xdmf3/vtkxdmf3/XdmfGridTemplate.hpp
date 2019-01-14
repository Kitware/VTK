/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfTemplate.hpp                                                    */
/*                                                                           */
/*  Author:                                                                  */
/*     Andrew Burns                                                          */
/*     andrew.j.burns2@us.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2013 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/

#ifndef XDMFGRIDTEMPLATE_HPP_
#define XDMFGRIDTEMPLATE_HPP_

// C Compatible Includes
#include "Xdmf.hpp"
#include "XdmfItem.hpp"
#include "XdmfGridCollection.hpp"
#include "XdmfItemFactory.hpp"
#include "XdmfTemplate.hpp"
#include "XdmfArray.hpp"
#include "XdmfHeavyDataWriter.hpp"

#ifdef __cplusplus

// Includes

/**
 * @brief Defines a template that can be filled with multiple sets of data.
 *
 * An XdmfTemplate defines a structure. The arrays within that structure
 * are stored if they are not initialized when the structure is first set.
 * Steps can then be added and references to heavy data are produced and
 * stored for later retrieval.
 *
 * This effectively lets an object have several variations with different
 * contained data.
 */
class XDMF_EXPORT XdmfGridTemplate : public XdmfTemplate,
                                     public XdmfGridCollection {

public:

  /**
   * Creates a new instance of the XdmfTemplate object
   *
   * @return    A constructed XdmfTemplate object.
   */
  static shared_ptr<XdmfGridTemplate> New();

  virtual ~XdmfGridTemplate();

  LOKI_DEFINE_VISITABLE(XdmfGridTemplate, XdmfGrid);
  static const std::string ItemTag;

  /**
   * Writes all tracked arrays to heavy data via the provided
   * heavy data writer then stores the heavy data descriptions.
   *
   * @return    The ID of the step that was added
   */
  virtual unsigned int addStep();

  std::map<std::string, std::string> getItemProperties() const;

  std::string getItemTag() const;

  using XdmfGrid::insert;

  // Overriding the parent versions so that all of these reference the Base item
  // instead of the template

  virtual shared_ptr<XdmfArray> getTimes();

  virtual shared_ptr<XdmfGridCollection> getGridCollection(const unsigned int index);

  virtual shared_ptr<const XdmfGridCollection> getGridCollection(const unsigned int index) const;

  virtual shared_ptr<XdmfGridCollection> getGridCollection(const std::string & Name);

  virtual shared_ptr<const XdmfGridCollection> getGridCollection(const std::string & Name) const;

  virtual unsigned int getNumberGridCollections() const;

  virtual void insert(const shared_ptr<XdmfGridCollection> GridCollection);

  virtual void removeGridCollection(const unsigned int index);

  virtual void removeGridCollection(const std::string & Name);

  virtual shared_ptr<XdmfCurvilinearGrid> getCurvilinearGrid(const unsigned int index);

  virtual shared_ptr<const XdmfCurvilinearGrid> getCurvilinearGrid(const unsigned int index) const;

  virtual shared_ptr<XdmfCurvilinearGrid> getCurvilinearGrid(const std::string & Name);

  virtual shared_ptr<const XdmfCurvilinearGrid> getCurvilinearGrid(const std::string & Name) const;

  virtual unsigned int getNumberCurvilinearGrids() const;

  virtual void insert(const shared_ptr<XdmfCurvilinearGrid> CurvilinearGrid);

  virtual void removeCurvilinearGrid(const unsigned int index);

  virtual void removeCurvilinearGrid(const std::string & Name);

  virtual shared_ptr<XdmfRectilinearGrid> getRectilinearGrid(const unsigned int index);

  virtual shared_ptr<const XdmfRectilinearGrid> getRectilinearGrid(const unsigned int index) const;

  virtual shared_ptr<XdmfRectilinearGrid> getRectilinearGrid(const std::string & Name);

  virtual shared_ptr<const XdmfRectilinearGrid> getRectilinearGrid(const std::string & Name) const;

  virtual unsigned int getNumberRectilinearGrids() const;

  virtual void insert(const shared_ptr<XdmfRectilinearGrid> RectilinearGrid);

  virtual void removeRectilinearGrid(const unsigned int index);

  virtual void removeRectilinearGrid(const std::string & Name);

  virtual shared_ptr<XdmfRegularGrid> getRegularGrid(const unsigned int index);

  virtual shared_ptr<const XdmfRegularGrid> getRegularGrid(const unsigned int index) const;

  virtual shared_ptr<XdmfRegularGrid> getRegularGrid(const std::string & Name);

  virtual shared_ptr<const XdmfRegularGrid> getRegularGrid(const std::string & Name) const;

  virtual unsigned int getNumberRegularGrids() const;

  virtual void insert(const shared_ptr<XdmfRegularGrid> RegularGrid);

  virtual void removeRegularGrid(const unsigned int index);

  virtual void removeRegularGrid(const std::string & Name);

  virtual shared_ptr<XdmfUnstructuredGrid> getUnstructuredGrid(const unsigned int index);

  virtual shared_ptr<const XdmfUnstructuredGrid> getUnstructuredGrid(const unsigned int index) const;

  virtual shared_ptr<XdmfUnstructuredGrid> getUnstructuredGrid(const std::string & Name);

  virtual shared_ptr<const XdmfUnstructuredGrid> getUnstructuredGrid(const std::string & Name) const;

  virtual unsigned int getNumberUnstructuredGrids() const;

  virtual void insert(const shared_ptr<XdmfUnstructuredGrid> UnstructuredGrid);

  virtual void removeUnstructuredGrid(const unsigned int index);

  virtual void removeUnstructuredGrid(const std::string & Name);

  /**
   *
   */
  virtual void removeStep(unsigned int stepId);

  virtual void setBase(shared_ptr<XdmfItem> newBase);

  /**
   * Reads in the heavy data associated with the provided step id.
   *
   * @param     stepId  The id of the step whose heavy data
   *                    is to be read in from file
   */
  void setStep(unsigned int stepId);

  void setStep(shared_ptr<XdmfTime> time);

  void traverse(const shared_ptr<XdmfBaseVisitor> visitor);

  XdmfGridTemplate(XdmfGridTemplate &);

protected:

  XdmfGridTemplate();

  virtual void
  populateItem(const std::map<std::string, std::string> & itemProperties,
               const std::vector<shared_ptr<XdmfItem> > & childItems,
               const XdmfCoreReader * const reader);

  shared_ptr<XdmfArray> mTimeCollection;

private:

  XdmfGridTemplate(const XdmfGridTemplate &);  // Not implemented.
  void operator=(const XdmfGridTemplate &);  // Not implemented.

};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

struct XDMFGRIDTEMPLATE; // Simply as a typedef to ensure correct typing
typedef struct XDMFGRIDTEMPLATE XDMFGRIDTEMPLATE;

XDMF_ITEM_C_CHILD_DECLARE(XdmfGridTemplate, XDMFGRIDTEMPLATE, XDMF)

#ifdef __cplusplus
}
#endif


#endif /* XDMFGRIDTEMPLATE_HPP_ */
