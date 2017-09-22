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

#ifndef XDMFTEMPLATE_HPP_
#define XDMFTEMPLATE_HPP_

// C Compatible Includes
#include "Xdmf.hpp"
#include "XdmfItem.hpp"
#include "XdmfItemFactory.hpp"
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
class XDMF_EXPORT XdmfTemplate : public virtual XdmfItem {

public:

  /**
   * Creates a new instance of the XdmfTemplate object
   *
   * @return    A constructed XdmfTemplate object.
   */
  static shared_ptr<XdmfTemplate> New();

  virtual ~XdmfTemplate();

  LOKI_DEFINE_VISITABLE(XdmfTemplate, XdmfItem);
  static const std::string ItemTag;

  /**
   * Writes all tracked arrays to heavy data via the provided
   * heavy data writer then stores the heavy data descriptions.
   *
   * @return    The ID of the step that was added
   */
  virtual unsigned int addStep();

  /**
   * Clears the current data from the tracked arrays.
   */
  virtual void clearStep();

  /**
   * Gets the XdmfItem that serves as the structure for the template.
   *
   * @return    The XdmfItem that serves as the structure for the template.
   */
  virtual shared_ptr<XdmfItem> getBase();

  /**
   * Gets the heavy data writer that is used to write step data to heavy data.
   *
   * @return    The heavy data writer
   */
  shared_ptr<XdmfHeavyDataWriter> getHeavyDataWriter();

  std::map<std::string, std::string> getItemProperties() const;

  std::string getItemTag() const;

  /**
   * Gets the number of steps currently contained within the template.
   *
   * @return    The number of steps contained within the template.
   */
  unsigned int getNumberSteps() const;

  /**
   * Gets the number of arrays tracked across timesteps.
   *
   * @return    The numer of tracked arrays.
   */
  unsigned int getNumberTrackedArrays() const;

  /**
   * Gets the tracked array at the specified index. The index of the array
   * depends on when the internal visitor encountered the array in question.
   *
   * @return    The requested array.
   */
  XdmfArray * getTrackedArray(unsigned int index);

  using XdmfItem::insert;

  /*
   *
   */
  virtual void preallocateSteps(unsigned int numSteps);

  /**
   * 
   */
  virtual void removeStep(unsigned int stepId);

  /**
   * Sets the item to define the structure for each step of the template.
   *
   * When the base is set all uninitialized arrays are added to
   * the list of tracked arrays.
   *
   * @param     newBase The item to serve as the structure.
   */
  virtual void setBase(shared_ptr<XdmfItem> newBase);

  /**
   * Sets the heavy data writer with which the template will write
   * to heavy data when adding a step.
   *
   * @param     writer  The writer to be used to write to heavy data.
   */
  void setHeavyDataWriter(shared_ptr<XdmfHeavyDataWriter> writer);

  /**
   * Reads in the heavy data associated with the provided step id.
   *
   * @param     stepId  The id of the step whose heavy data
   *                    is to be read in from file
   */
  virtual void setStep(unsigned int stepId);

  /**
   * Adds an array to the list of tracked arrays if that array
   * is not already there.
   *
   * The setBase method automatically sets uninitialized arrays
   * to be tracked, this can be used to add any missed by setBase.
   *
   * @param     newArray        The array to be tracked.
   */
  virtual void trackArray(shared_ptr<XdmfArray> newArray);

  virtual void traverse(const shared_ptr<XdmfBaseVisitor> visitor);

protected:

  XdmfTemplate();

  virtual void
  populateItem(const std::map<std::string, std::string> & itemProperties,
               const std::vector<shared_ptr<XdmfItem> > & childItems,
               const XdmfCoreReader * const reader);

  shared_ptr<XdmfHeavyDataWriter> mHeavyWriter;

  shared_ptr<XdmfItem> mBase;
  std::vector<XdmfArray *> mTrackedArrays;
  std::vector<std::string> mDataTypes;
  std::vector<std::string> mDataDescriptions;
  std::vector<std::vector<shared_ptr<XdmfHeavyDataController> > > mDataControllers;
  std::vector<shared_ptr<const XdmfArrayType> > mTrackedArrayTypes;
  std::vector<std::vector<unsigned int> > mTrackedArrayDims;
  unsigned int mCurrentStep;
  unsigned int mNumSteps;
  shared_ptr<XdmfItemFactory> mItemFactory;

private:

  XdmfTemplate(const XdmfTemplate &);  // Not implemented.
  void operator=(const XdmfTemplate &);  // Not implemented.

};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

struct XDMFTEMPLATE; // Simply as a typedef to ensure correct typing
typedef struct XDMFTEMPLATE XDMFTEMPLATE;

XDMF_ITEM_C_CHILD_DECLARE(XdmfTemplate, XDMFTEMPLATE, XDMF)

#ifdef __cplusplus
}
#endif


#endif /* XDMFTEMPLATE_HPP_ */
