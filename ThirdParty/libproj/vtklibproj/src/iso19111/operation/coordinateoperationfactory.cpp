/******************************************************************************
 *
 * Project:  PROJ
 * Purpose:  ISO19111:2019 implementation
 * Author:   Even Rouault <even dot rouault at spatialys dot com>
 *
 ******************************************************************************
 * Copyright (c) 2018, Even Rouault <even dot rouault at spatialys dot com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#ifndef FROM_PROJ_CPP
#define FROM_PROJ_CPP
#endif

#include "proj/common.hpp"
#include "proj/coordinateoperation.hpp"
#include "proj/crs.hpp"
#include "proj/io.hpp"
#include "proj/metadata.hpp"
#include "proj/util.hpp"

#include "proj/internal/internal.hpp"
#include "proj/internal/io_internal.hpp"
#include "proj/internal/tracing.hpp"

#include "coordinateoperation_internal.hpp"
#include "coordinateoperation_private.hpp"
#include "oputils.hpp"

// PROJ include order is sensitive
// clang-format off
#include "proj.h"
#include "proj_internal.h" // M_PI
// clang-format on
#include "proj_constants.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <memory>
#include <set>
#include <string>
#include <vector>

// #define TRACE_CREATE_OPERATIONS
// #define DEBUG_SORT
// #define DEBUG_CONCATENATED_OPERATION
#if defined(DEBUG_SORT) || defined(DEBUG_CONCATENATED_OPERATION)
#include <iostream>

void dumpWKT(const NS_PROJ::crs::CRS *crs);
void dumpWKT(const NS_PROJ::crs::CRS *crs) {
    auto f(NS_PROJ::io::WKTFormatter::create(
        NS_PROJ::io::WKTFormatter::Convention::WKT2_2019));
    std::cerr << crs->exportToWKT(f.get()) << std::endl;
}

void dumpWKT(const NS_PROJ::crs::CRSPtr &crs);
void dumpWKT(const NS_PROJ::crs::CRSPtr &crs) { dumpWKT(crs.get()); }

void dumpWKT(const NS_PROJ::crs::CRSNNPtr &crs);
void dumpWKT(const NS_PROJ::crs::CRSNNPtr &crs) {
    dumpWKT(crs.as_nullable().get());
}

void dumpWKT(const NS_PROJ::crs::GeographicCRSPtr &crs);
void dumpWKT(const NS_PROJ::crs::GeographicCRSPtr &crs) { dumpWKT(crs.get()); }

void dumpWKT(const NS_PROJ::crs::GeographicCRSNNPtr &crs);
void dumpWKT(const NS_PROJ::crs::GeographicCRSNNPtr &crs) {
    dumpWKT(crs.as_nullable().get());
}

#endif

using namespace NS_PROJ::internal;

// ---------------------------------------------------------------------------

NS_PROJ_START
namespace operation {

// ---------------------------------------------------------------------------

#ifdef TRACE_CREATE_OPERATIONS

//! @cond Doxygen_Suppress

static std::string objectAsStr(const common::IdentifiedObject *obj) {
    std::string ret(obj->nameStr());
    const auto &ids = obj->identifiers();
    if (!ids.empty()) {
        ret += " (";
        ret += (*ids[0]->codeSpace()) + ":" + ids[0]->code();
        ret += ")";
    }
    return ret;
}
//! @endcond

#endif

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress

static double getPseudoArea(const metadata::ExtentPtr &extent) {
    if (!extent)
        return 0.0;
    const auto &geographicElements = extent->geographicElements();
    if (geographicElements.empty())
        return 0.0;
    auto bbox = dynamic_cast<const metadata::GeographicBoundingBox *>(
        geographicElements[0].get());
    if (!bbox)
        return 0;
    double w = bbox->westBoundLongitude();
    double s = bbox->southBoundLatitude();
    double e = bbox->eastBoundLongitude();
    double n = bbox->northBoundLatitude();
    if (w > e) {
        e += 360.0;
    }
    // Integrate cos(lat) between south_lat and north_lat
    return (e - w) * (std::sin(common::Angle(n).getSIValue()) -
                      std::sin(common::Angle(s).getSIValue()));
}

//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct CoordinateOperationContext::Private {
    io::AuthorityFactoryPtr authorityFactory_{};
    metadata::ExtentPtr extent_{};
    double accuracy_ = 0.0;
    SourceTargetCRSExtentUse sourceAndTargetCRSExtentUse_ =
        CoordinateOperationContext::SourceTargetCRSExtentUse::SMALLEST;
    SpatialCriterion spatialCriterion_ =
        CoordinateOperationContext::SpatialCriterion::STRICT_CONTAINMENT;
    bool usePROJNames_ = true;
    GridAvailabilityUse gridAvailabilityUse_ =
        GridAvailabilityUse::USE_FOR_SORTING;
    IntermediateCRSUse allowUseIntermediateCRS_ = CoordinateOperationContext::
        IntermediateCRSUse::IF_NO_DIRECT_TRANSFORMATION;
    std::vector<std::pair<std::string, std::string>>
        intermediateCRSAuthCodes_{};
    bool discardSuperseded_ = true;
    bool allowBallpark_ = true;
};
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
CoordinateOperationContext::~CoordinateOperationContext() = default;
//! @endcond

// ---------------------------------------------------------------------------

CoordinateOperationContext::CoordinateOperationContext()
    : d(internal::make_unique<Private>()) {}

// ---------------------------------------------------------------------------

/** \brief Return the authority factory, or null */
const io::AuthorityFactoryPtr &
CoordinateOperationContext::getAuthorityFactory() const {
    return d->authorityFactory_;
}

// ---------------------------------------------------------------------------

/** \brief Return the desired area of interest, or null */
const metadata::ExtentPtr &
CoordinateOperationContext::getAreaOfInterest() const {
    return d->extent_;
}

// ---------------------------------------------------------------------------

/** \brief Set the desired area of interest, or null */
void CoordinateOperationContext::setAreaOfInterest(
    const metadata::ExtentPtr &extent) {
    d->extent_ = extent;
}

// ---------------------------------------------------------------------------

/** \brief Return the desired accuracy (in metre), or 0 */
double CoordinateOperationContext::getDesiredAccuracy() const {
    return d->accuracy_;
}

// ---------------------------------------------------------------------------

/** \brief Set the desired accuracy (in metre), or 0 */
void CoordinateOperationContext::setDesiredAccuracy(double accuracy) {
    d->accuracy_ = accuracy;
}

// ---------------------------------------------------------------------------

/** \brief Return whether ballpark transformations are allowed */
bool CoordinateOperationContext::getAllowBallparkTransformations() const {
    return d->allowBallpark_;
}

// ---------------------------------------------------------------------------

/** \brief Set whether ballpark transformations are allowed */
void CoordinateOperationContext::setAllowBallparkTransformations(bool allow) {
    d->allowBallpark_ = allow;
}

// ---------------------------------------------------------------------------

/** \brief Set how source and target CRS extent should be used
 * when considering if a transformation can be used (only takes effect if
 * no area of interest is explicitly defined).
 *
 * The default is
 * CoordinateOperationContext::SourceTargetCRSExtentUse::SMALLEST.
 */
void CoordinateOperationContext::setSourceAndTargetCRSExtentUse(
    SourceTargetCRSExtentUse use) {
    d->sourceAndTargetCRSExtentUse_ = use;
}

// ---------------------------------------------------------------------------

/** \brief Return how source and target CRS extent should be used
 * when considering if a transformation can be used (only takes effect if
 * no area of interest is explicitly defined).
 *
 * The default is
 * CoordinateOperationContext::SourceTargetCRSExtentUse::SMALLEST.
 */
CoordinateOperationContext::SourceTargetCRSExtentUse
CoordinateOperationContext::getSourceAndTargetCRSExtentUse() const {
    return d->sourceAndTargetCRSExtentUse_;
}

// ---------------------------------------------------------------------------

/** \brief Set the spatial criterion to use when comparing the area of
 * validity
 * of coordinate operations with the area of interest / area of validity of
 * source and target CRS.
 *
 * The default is STRICT_CONTAINMENT.
 */
void CoordinateOperationContext::setSpatialCriterion(
    SpatialCriterion criterion) {
    d->spatialCriterion_ = criterion;
}

// ---------------------------------------------------------------------------

/** \brief Return the spatial criterion to use when comparing the area of
 * validity
 * of coordinate operations with the area of interest / area of validity of
 * source and target CRS.
 *
 * The default is STRICT_CONTAINMENT.
 */
CoordinateOperationContext::SpatialCriterion
CoordinateOperationContext::getSpatialCriterion() const {
    return d->spatialCriterion_;
}

// ---------------------------------------------------------------------------

/** \brief Set whether PROJ alternative grid names should be substituted to
 * the official authority names.
 *
 * This only has effect is an authority factory with a non-null database context
 * has been attached to this context.
 *
 * If set to false, it is still possible to
 * obtain later the substitution by using io::PROJStringFormatter::create()
 * with a non-null database context.
 *
 * The default is true.
 */
void CoordinateOperationContext::setUsePROJAlternativeGridNames(
    bool usePROJNames) {
    d->usePROJNames_ = usePROJNames;
}

// ---------------------------------------------------------------------------

/** \brief Return whether PROJ alternative grid names should be substituted to
 * the official authority names.
 *
 * The default is true.
 */
bool CoordinateOperationContext::getUsePROJAlternativeGridNames() const {
    return d->usePROJNames_;
}

// ---------------------------------------------------------------------------

/** \brief Return whether transformations that are superseded (but not
 * deprecated)
 * should be discarded.
 *
 * The default is true.
 */
bool CoordinateOperationContext::getDiscardSuperseded() const {
    return d->discardSuperseded_;
}

// ---------------------------------------------------------------------------

/** \brief Set whether transformations that are superseded (but not deprecated)
 * should be discarded.
 *
 * The default is true.
 */
void CoordinateOperationContext::setDiscardSuperseded(bool discard) {
    d->discardSuperseded_ = discard;
}

// ---------------------------------------------------------------------------

/** \brief Set how grid availability is used.
 *
 * The default is USE_FOR_SORTING.
 */
void CoordinateOperationContext::setGridAvailabilityUse(
    GridAvailabilityUse use) {
    d->gridAvailabilityUse_ = use;
}

// ---------------------------------------------------------------------------

/** \brief Return how grid availability is used.
 *
 * The default is USE_FOR_SORTING.
 */
CoordinateOperationContext::GridAvailabilityUse
CoordinateOperationContext::getGridAvailabilityUse() const {
    return d->gridAvailabilityUse_;
}

// ---------------------------------------------------------------------------

/** \brief Set whether an intermediate pivot CRS can be used for researching
 * coordinate operations between a source and target CRS.
 *
 * Concretely if in the database there is an operation from A to C
 * (or C to A), and another one from C to B (or B to C), but no direct
 * operation between A and B, setting this parameter to
 * ALWAYS/IF_NO_DIRECT_TRANSFORMATION, allow chaining both operations.
 *
 * The current implementation is limited to researching one intermediate
 * step.
 *
 * By default, with the IF_NO_DIRECT_TRANSFORMATION strategy, all potential
 * C candidates will be used if there is no direct transformation.
 */
void CoordinateOperationContext::setAllowUseIntermediateCRS(
    IntermediateCRSUse use) {
    d->allowUseIntermediateCRS_ = use;
}

// ---------------------------------------------------------------------------

/** \brief Return whether an intermediate pivot CRS can be used for researching
 * coordinate operations between a source and target CRS.
 *
 * Concretely if in the database there is an operation from A to C
 * (or C to A), and another one from C to B (or B to C), but no direct
 * operation between A and B, setting this parameter to
 * ALWAYS/IF_NO_DIRECT_TRANSFORMATION, allow chaining both operations.
 *
 * The default is IF_NO_DIRECT_TRANSFORMATION.
 */
CoordinateOperationContext::IntermediateCRSUse
CoordinateOperationContext::getAllowUseIntermediateCRS() const {
    return d->allowUseIntermediateCRS_;
}

// ---------------------------------------------------------------------------

/** \brief Restrict the potential pivot CRSs that can be used when trying to
 * build a coordinate operation between two CRS that have no direct operation.
 *
 * @param intermediateCRSAuthCodes a vector of (auth_name, code) that can be
 * used as potential pivot RS
 */
void CoordinateOperationContext::setIntermediateCRS(
    const std::vector<std::pair<std::string, std::string>>
        &intermediateCRSAuthCodes) {
    d->intermediateCRSAuthCodes_ = intermediateCRSAuthCodes;
}

// ---------------------------------------------------------------------------

/** \brief Return the potential pivot CRSs that can be used when trying to
 * build a coordinate operation between two CRS that have no direct operation.
 *
 */
const std::vector<std::pair<std::string, std::string>> &
CoordinateOperationContext::getIntermediateCRS() const {
    return d->intermediateCRSAuthCodes_;
}

// ---------------------------------------------------------------------------

/** \brief Creates a context for a coordinate operation.
 *
 * If a non null authorityFactory is provided, the resulting context should
 * not be used simultaneously by more than one thread.
 *
 * If authorityFactory->getAuthority() is the empty string, then coordinate
 * operations from any authority will be searched, with the restrictions set
 * in the authority_to_authority_preference database table.
 * If authorityFactory->getAuthority() is set to "any", then coordinate
 * operations from any authority will be searched
 * If authorityFactory->getAuthority() is a non-empty string different of "any",
 * then coordinate operations will be searched only in that authority namespace.
 *
 * @param authorityFactory Authority factory, or null if no database lookup
 * is allowed.
 * Use io::authorityFactory::create(context, std::string()) to allow all
 * authorities to be used.
 * @param extent Area of interest, or null if none is known.
 * @param accuracy Maximum allowed accuracy in metre, as specified in or
 * 0 to get best accuracy.
 * @return a new context.
 */
CoordinateOperationContextNNPtr CoordinateOperationContext::create(
    const io::AuthorityFactoryPtr &authorityFactory,
    const metadata::ExtentPtr &extent, double accuracy) {
    auto ctxt = NN_NO_CHECK(
        CoordinateOperationContext::make_unique<CoordinateOperationContext>());
    ctxt->d->authorityFactory_ = authorityFactory;
    ctxt->d->extent_ = extent;
    ctxt->d->accuracy_ = accuracy;
    return ctxt;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct CoordinateOperationFactory::Private {

    struct Context {
        // This is the extent of the source CRS and target CRS of the initial
        // CoordinateOperationFactory::createOperations() public call, not
        // necessarily the ones of intermediate
        // CoordinateOperationFactory::Private::createOperations() calls.
        // This is used to compare transformations area of use against the
        // area of use of the source & target CRS.
        const metadata::ExtentPtr &extent1;
        const metadata::ExtentPtr &extent2;
        const CoordinateOperationContextNNPtr &context;
        bool inCreateOperationsWithDatumPivotAntiRecursion = false;
        bool inCreateOperationsGeogToVertWithAlternativeGeog = false;
        bool inCreateOperationsGeogToVertWithIntermediateVert = false;
        bool skipHorizontalTransformation = false;
        std::map<std::pair<io::AuthorityFactory::ObjectType, std::string>,
                 std::list<std::pair<std::string, std::string>>>
            cacheNameToCRS{};

        Context(const metadata::ExtentPtr &extent1In,
                const metadata::ExtentPtr &extent2In,
                const CoordinateOperationContextNNPtr &contextIn)
            : extent1(extent1In), extent2(extent2In), context(contextIn) {}
    };

    static std::vector<CoordinateOperationNNPtr>
    createOperations(const crs::CRSNNPtr &sourceCRS,
                     const crs::CRSNNPtr &targetCRS, Context &context);

  private:
    static constexpr bool disallowEmptyIntersection = true;

    static void
    buildCRSIds(const crs::CRSNNPtr &crs, Private::Context &context,
                std::list<std::pair<std::string, std::string>> &ids);

    static std::vector<CoordinateOperationNNPtr> findOpsInRegistryDirect(
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        Private::Context &context, bool &resNonEmptyBeforeFiltering);

    static std::vector<CoordinateOperationNNPtr>
    findOpsInRegistryDirectTo(const crs::CRSNNPtr &targetCRS,
                              Private::Context &context);

    static std::vector<CoordinateOperationNNPtr>
    findsOpsInRegistryWithIntermediate(
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        Private::Context &context,
        bool useCreateBetweenGeodeticCRSWithDatumBasedIntermediates);

    static void createOperationsFromProj4Ext(
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        const crs::BoundCRS *boundSrc, const crs::BoundCRS *boundDst,
        std::vector<CoordinateOperationNNPtr> &res);

    static bool createOperationsFromDatabase(
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        Private::Context &context, const crs::GeodeticCRS *geodSrc,
        const crs::GeodeticCRS *geodDst, const crs::GeographicCRS *geogSrc,
        const crs::GeographicCRS *geogDst, const crs::VerticalCRS *vertSrc,
        const crs::VerticalCRS *vertDst,
        std::vector<CoordinateOperationNNPtr> &res);

    static std::vector<CoordinateOperationNNPtr>
    createOperationsGeogToVertFromGeoid(const crs::CRSNNPtr &sourceCRS,
                                        const crs::CRSNNPtr &targetCRS,
                                        const crs::VerticalCRS *vertDst,
                                        Context &context);

    static std::vector<CoordinateOperationNNPtr>
    createOperationsGeogToVertWithIntermediateVert(
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        const crs::VerticalCRS *vertDst, Context &context);

    static std::vector<CoordinateOperationNNPtr>
    createOperationsGeogToVertWithAlternativeGeog(
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        Context &context);

    static void createOperationsFromDatabaseWithVertCRS(
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        Private::Context &context, const crs::GeographicCRS *geogSrc,
        const crs::GeographicCRS *geogDst, const crs::VerticalCRS *vertSrc,
        const crs::VerticalCRS *vertDst,
        std::vector<CoordinateOperationNNPtr> &res);

    static void createOperationsGeodToGeod(
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        Private::Context &context, const crs::GeodeticCRS *geodSrc,
        const crs::GeodeticCRS *geodDst,
        std::vector<CoordinateOperationNNPtr> &res);

    static void createOperationsDerivedTo(
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        Private::Context &context, const crs::DerivedCRS *derivedSrc,
        std::vector<CoordinateOperationNNPtr> &res);

    static void createOperationsBoundToGeog(
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        Private::Context &context, const crs::BoundCRS *boundSrc,
        const crs::GeographicCRS *geogDst,
        std::vector<CoordinateOperationNNPtr> &res);

    static void createOperationsBoundToVert(
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        Private::Context &context, const crs::BoundCRS *boundSrc,
        const crs::VerticalCRS *vertDst,
        std::vector<CoordinateOperationNNPtr> &res);

    static void createOperationsVertToVert(
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        Private::Context &context, const crs::VerticalCRS *vertSrc,
        const crs::VerticalCRS *vertDst,
        std::vector<CoordinateOperationNNPtr> &res);

    static void createOperationsVertToGeog(
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        Private::Context &context, const crs::VerticalCRS *vertSrc,
        const crs::GeographicCRS *geogDst,
        std::vector<CoordinateOperationNNPtr> &res);

    static void createOperationsVertToGeogBallpark(
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        Private::Context &context, const crs::VerticalCRS *vertSrc,
        const crs::GeographicCRS *geogDst,
        std::vector<CoordinateOperationNNPtr> &res);

    static void createOperationsBoundToBound(
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        Private::Context &context, const crs::BoundCRS *boundSrc,
        const crs::BoundCRS *boundDst,
        std::vector<CoordinateOperationNNPtr> &res);

    static void createOperationsCompoundToGeog(
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        Private::Context &context, const crs::CompoundCRS *compoundSrc,
        const crs::GeographicCRS *geogDst,
        std::vector<CoordinateOperationNNPtr> &res);

    static void createOperationsToGeod(
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        Private::Context &context, const crs::GeodeticCRS *geodDst,
        std::vector<CoordinateOperationNNPtr> &res);

    static void createOperationsCompoundToCompound(
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        Private::Context &context, const crs::CompoundCRS *compoundSrc,
        const crs::CompoundCRS *compoundDst,
        std::vector<CoordinateOperationNNPtr> &res);

    static void createOperationsBoundToCompound(
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        Private::Context &context, const crs::BoundCRS *boundSrc,
        const crs::CompoundCRS *compoundDst,
        std::vector<CoordinateOperationNNPtr> &res);

    static std::vector<CoordinateOperationNNPtr> createOperationsGeogToGeog(
        std::vector<CoordinateOperationNNPtr> &res,
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        Private::Context &context, const crs::GeographicCRS *geogSrc,
        const crs::GeographicCRS *geogDst);

    static void createOperationsWithDatumPivot(
        std::vector<CoordinateOperationNNPtr> &res,
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        const crs::GeodeticCRS *geodSrc, const crs::GeodeticCRS *geodDst,
        Context &context);

    static bool
    hasPerfectAccuracyResult(const std::vector<CoordinateOperationNNPtr> &res,
                             const Context &context);

    static void setCRSs(CoordinateOperation *co, const crs::CRSNNPtr &sourceCRS,
                        const crs::CRSNNPtr &targetCRS);
};
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
CoordinateOperationFactory::~CoordinateOperationFactory() = default;
//! @endcond

// ---------------------------------------------------------------------------

CoordinateOperationFactory::CoordinateOperationFactory() : d(nullptr) {}

// ---------------------------------------------------------------------------

/** \brief Find a CoordinateOperation from sourceCRS to targetCRS.
 *
 * This is a helper of createOperations(), using a coordinate operation
 * context
 * with no authority factory (so no catalog searching is done), no desired
 * accuracy and no area of interest.
 * This returns the first operation of the result set of createOperations(),
 * or null if none found.
 *
 * @param sourceCRS source CRS.
 * @param targetCRS source CRS.
 * @return a CoordinateOperation or nullptr.
 */
CoordinateOperationPtr CoordinateOperationFactory::createOperation(
    const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS) const {
    auto res = createOperations(
        sourceCRS, targetCRS,
        CoordinateOperationContext::create(nullptr, nullptr, 0.0));
    if (!res.empty()) {
        return res[0];
    }
    return nullptr;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress

// ---------------------------------------------------------------------------

struct PrecomputedOpCharacteristics {
    double area_{};
    double accuracy_{};
    bool isPROJExportable_ = false;
    bool hasGrids_ = false;
    bool gridsAvailable_ = false;
    bool gridsKnown_ = false;
    size_t stepCount_ = 0;
    bool isApprox_ = false;
    bool hasBallparkVertical_ = false;
    bool isNullTransformation_ = false;

    PrecomputedOpCharacteristics() = default;
    PrecomputedOpCharacteristics(double area, double accuracy,
                                 bool isPROJExportable, bool hasGrids,
                                 bool gridsAvailable, bool gridsKnown,
                                 size_t stepCount, bool isApprox,
                                 bool hasBallparkVertical,
                                 bool isNullTransformation)
        : area_(area), accuracy_(accuracy), isPROJExportable_(isPROJExportable),
          hasGrids_(hasGrids), gridsAvailable_(gridsAvailable),
          gridsKnown_(gridsKnown), stepCount_(stepCount), isApprox_(isApprox),
          hasBallparkVertical_(hasBallparkVertical),
          isNullTransformation_(isNullTransformation) {}
};

// ---------------------------------------------------------------------------

// We could have used a lambda instead of this old-school way, but
// filterAndSort() is already huge.
struct SortFunction {

    const std::map<CoordinateOperation *, PrecomputedOpCharacteristics> &map;

    explicit SortFunction(const std::map<CoordinateOperation *,
                                         PrecomputedOpCharacteristics> &mapIn)
        : map(mapIn) {}

    // Sorting function
    // Return true if a < b
    bool compare(const CoordinateOperationNNPtr &a,
                 const CoordinateOperationNNPtr &b) const {
        auto iterA = map.find(a.get());
        assert(iterA != map.end());
        auto iterB = map.find(b.get());
        assert(iterB != map.end());

        // CAUTION: the order of the comparisons is extremely important
        // to get the intended result.

        if (iterA->second.isPROJExportable_ &&
            !iterB->second.isPROJExportable_) {
            return true;
        }
        if (!iterA->second.isPROJExportable_ &&
            iterB->second.isPROJExportable_) {
            return false;
        }

        if (!iterA->second.isApprox_ && iterB->second.isApprox_) {
            return true;
        }
        if (iterA->second.isApprox_ && !iterB->second.isApprox_) {
            return false;
        }

        if (!iterA->second.hasBallparkVertical_ &&
            iterB->second.hasBallparkVertical_) {
            return true;
        }
        if (iterA->second.hasBallparkVertical_ &&
            !iterB->second.hasBallparkVertical_) {
            return false;
        }

        if (!iterA->second.isNullTransformation_ &&
            iterB->second.isNullTransformation_) {
            return true;
        }
        if (iterA->second.isNullTransformation_ &&
            !iterB->second.isNullTransformation_) {
            return false;
        }

        // Operations where grids are all available go before other
        if (iterA->second.gridsAvailable_ && !iterB->second.gridsAvailable_) {
            return true;
        }
        if (iterB->second.gridsAvailable_ && !iterA->second.gridsAvailable_) {
            return false;
        }

        // Operations where grids are all known in our DB go before other
        if (iterA->second.gridsKnown_ && !iterB->second.gridsKnown_) {
            return true;
        }
        if (iterB->second.gridsKnown_ && !iterA->second.gridsKnown_) {
            return false;
        }

        // Operations with known accuracy go before those with unknown accuracy
        const double accuracyA = iterA->second.accuracy_;
        const double accuracyB = iterB->second.accuracy_;
        if (accuracyA >= 0 && accuracyB < 0) {
            return true;
        }
        if (accuracyB >= 0 && accuracyA < 0) {
            return false;
        }

        if (accuracyA < 0 && accuracyB < 0) {
            // unknown accuracy ? then prefer operations with grids, which
            // are likely to have best practical accuracy
            if (iterA->second.hasGrids_ && !iterB->second.hasGrids_) {
                return true;
            }
            if (!iterA->second.hasGrids_ && iterB->second.hasGrids_) {
                return false;
            }
        }

        // Operations with larger non-zero area of use go before those with
        // lower one
        const double areaA = iterA->second.area_;
        const double areaB = iterB->second.area_;
        if (areaA > 0) {
            if (areaA > areaB) {
                return true;
            }
            if (areaA < areaB) {
                return false;
            }
        } else if (areaB > 0) {
            return false;
        }

        // Operations with better accuracy go before those with worse one
        if (accuracyA >= 0 && accuracyA < accuracyB) {
            return true;
        }
        if (accuracyB >= 0 && accuracyB < accuracyA) {
            return false;
        }

        if (accuracyA >= 0 && accuracyA == accuracyB) {
            // same accuracy ? then prefer operations without grids
            if (!iterA->second.hasGrids_ && iterB->second.hasGrids_) {
                return true;
            }
            if (iterA->second.hasGrids_ && !iterB->second.hasGrids_) {
                return false;
            }
        }

        // The less intermediate steps, the better
        if (iterA->second.stepCount_ < iterB->second.stepCount_) {
            return true;
        }
        if (iterB->second.stepCount_ < iterA->second.stepCount_) {
            return false;
        }

        const auto &a_name = a->nameStr();
        const auto &b_name = b->nameStr();
        // The shorter name, the better ?
        if (a_name.size() < b_name.size()) {
            return true;
        }
        if (b_name.size() < a_name.size()) {
            return false;
        }

        // Arbitrary final criterion. We actually return the greater element
        // first, so that "Amersfoort to WGS 84 (4)" is presented before
        // "Amersfoort to WGS 84 (3)", which is probably a better guess.

        // Except for French NTF (Paris) to NTF, where the (1) conversion
        // should be preferred because in the remarks of (2), it is mentioned
        // OGP prefers value from IGN Paris (code 1467)...
        if (a_name.find("NTF (Paris) to NTF (1)") != std::string::npos &&
            b_name.find("NTF (Paris) to NTF (2)") != std::string::npos) {
            return true;
        }
        if (a_name.find("NTF (Paris) to NTF (2)") != std::string::npos &&
            b_name.find("NTF (Paris) to NTF (1)") != std::string::npos) {
            return false;
        }
        if (a_name.find("NTF (Paris) to RGF93 (1)") != std::string::npos &&
            b_name.find("NTF (Paris) to RGF93 (2)") != std::string::npos) {
            return true;
        }
        if (a_name.find("NTF (Paris) to RGF93 (2)") != std::string::npos &&
            b_name.find("NTF (Paris) to RGF93 (1)") != std::string::npos) {
            return false;
        }

        return a_name > b_name;
    }

    bool operator()(const CoordinateOperationNNPtr &a,
                    const CoordinateOperationNNPtr &b) const {
        const bool ret = compare(a, b);
#if 0
        std::cerr << a->nameStr() << " < " << b->nameStr() << " : " << ret << std::endl;
#endif
        return ret;
    }
};

// ---------------------------------------------------------------------------

static size_t getStepCount(const CoordinateOperationNNPtr &op) {
    auto concat = dynamic_cast<const ConcatenatedOperation *>(op.get());
    size_t stepCount = 1;
    if (concat) {
        stepCount = concat->operations().size();
    }
    return stepCount;
}

// ---------------------------------------------------------------------------

// Return number of steps that are transformations (and not conversions)
static size_t getTransformationStepCount(const CoordinateOperationNNPtr &op) {
    auto concat = dynamic_cast<const ConcatenatedOperation *>(op.get());
    size_t stepCount = 1;
    if (concat) {
        stepCount = 0;
        for (const auto &subOp : concat->operations()) {
            if (dynamic_cast<const Conversion *>(subOp.get()) == nullptr) {
                stepCount++;
            }
        }
    }
    return stepCount;
}

// ---------------------------------------------------------------------------

static bool isNullTransformation(const std::string &name) {
    if (name.find(" + ") != std::string::npos)
        return false;
    return starts_with(name, BALLPARK_GEOCENTRIC_TRANSLATION) ||
           starts_with(name, BALLPARK_GEOGRAPHIC_OFFSET) ||
           starts_with(name, NULL_GEOGRAPHIC_OFFSET) ||
           starts_with(name, NULL_GEOCENTRIC_TRANSLATION);
}

// ---------------------------------------------------------------------------

struct FilterResults {

    FilterResults(const std::vector<CoordinateOperationNNPtr> &sourceListIn,
                  const CoordinateOperationContextNNPtr &contextIn,
                  const metadata::ExtentPtr &extent1In,
                  const metadata::ExtentPtr &extent2In,
                  bool forceStrictContainmentTest)
        : sourceList(sourceListIn), context(contextIn), extent1(extent1In),
          extent2(extent2In), areaOfInterest(context->getAreaOfInterest()),
          desiredAccuracy(context->getDesiredAccuracy()),
          sourceAndTargetCRSExtentUse(
              context->getSourceAndTargetCRSExtentUse()) {

        computeAreaOfInterest();
        filterOut(forceStrictContainmentTest);
    }

    FilterResults &andSort() {
        sort();

        // And now that we have a sorted list, we can remove uninteresting
        // results
        // ...
        removeSyntheticNullTransforms();
        removeUninterestingOps();
        removeDuplicateOps();
        removeSyntheticNullTransforms();
        return *this;
    }

    // ----------------------------------------------------------------------

    // cppcheck-suppress functionStatic
    const std::vector<CoordinateOperationNNPtr> &getRes() { return res; }

    // ----------------------------------------------------------------------
  private:
    const std::vector<CoordinateOperationNNPtr> &sourceList;
    const CoordinateOperationContextNNPtr &context;
    const metadata::ExtentPtr &extent1;
    const metadata::ExtentPtr &extent2;
    metadata::ExtentPtr areaOfInterest;
    const double desiredAccuracy = context->getDesiredAccuracy();
    const CoordinateOperationContext::SourceTargetCRSExtentUse
        sourceAndTargetCRSExtentUse;

    bool hasOpThatContainsAreaOfInterestAndNoGrid = false;
    std::vector<CoordinateOperationNNPtr> res{};

    // ----------------------------------------------------------------------
    void computeAreaOfInterest() {

        // Compute an area of interest from the CRS extent if the user did
        // not specify one
        if (!areaOfInterest) {
            if (sourceAndTargetCRSExtentUse ==
                CoordinateOperationContext::SourceTargetCRSExtentUse::
                    INTERSECTION) {
                if (extent1 && extent2) {
                    areaOfInterest =
                        extent1->intersection(NN_NO_CHECK(extent2));
                }
            } else if (sourceAndTargetCRSExtentUse ==
                       CoordinateOperationContext::SourceTargetCRSExtentUse::
                           SMALLEST) {
                if (extent1 && extent2) {
                    if (getPseudoArea(extent1) < getPseudoArea(extent2)) {
                        areaOfInterest = extent1;
                    } else {
                        areaOfInterest = extent2;
                    }
                } else if (extent1) {
                    areaOfInterest = extent1;
                } else {
                    areaOfInterest = extent2;
                }
            }
        }
    }

    // ---------------------------------------------------------------------------

    void filterOut(bool forceStrictContainmentTest) {

        // Filter out operations that do not match the expected accuracy
        // and area of use.
        const auto spatialCriterion =
            forceStrictContainmentTest
                ? CoordinateOperationContext::SpatialCriterion::
                      STRICT_CONTAINMENT
                : context->getSpatialCriterion();
        bool hasOnlyBallpark = true;
        bool hasNonBallparkWithoutExtent = false;
        bool hasNonBallparkOpWithExtent = false;
        const bool allowBallpark = context->getAllowBallparkTransformations();
        for (const auto &op : sourceList) {
            if (desiredAccuracy != 0) {
                const double accuracy = getAccuracy(op);
                if (accuracy < 0 || accuracy > desiredAccuracy) {
                    continue;
                }
            }
            if (!allowBallpark && op->hasBallparkTransformation()) {
                continue;
            }
            if (areaOfInterest) {
                bool emptyIntersection = false;
                auto extent = getExtent(op, true, emptyIntersection);
                if (!extent) {
                    if (!op->hasBallparkTransformation()) {
                        hasNonBallparkWithoutExtent = true;
                    }
                    continue;
                }
                if (!op->hasBallparkTransformation()) {
                    hasNonBallparkOpWithExtent = true;
                }
                bool extentContains =
                    extent->contains(NN_NO_CHECK(areaOfInterest));
                if (!hasOpThatContainsAreaOfInterestAndNoGrid &&
                    extentContains) {
                    if (!op->hasBallparkTransformation() &&
                        op->gridsNeeded(nullptr, true).empty()) {
                        hasOpThatContainsAreaOfInterestAndNoGrid = true;
                    }
                }
                if (spatialCriterion ==
                        CoordinateOperationContext::SpatialCriterion::
                            STRICT_CONTAINMENT &&
                    !extentContains) {
                    continue;
                }
                if (spatialCriterion ==
                        CoordinateOperationContext::SpatialCriterion::
                            PARTIAL_INTERSECTION &&
                    !extent->intersects(NN_NO_CHECK(areaOfInterest))) {
                    continue;
                }
            } else if (sourceAndTargetCRSExtentUse ==
                       CoordinateOperationContext::SourceTargetCRSExtentUse::
                           BOTH) {
                bool emptyIntersection = false;
                auto extent = getExtent(op, true, emptyIntersection);
                if (!extent) {
                    if (!op->hasBallparkTransformation()) {
                        hasNonBallparkWithoutExtent = true;
                    }
                    continue;
                }
                if (!op->hasBallparkTransformation()) {
                    hasNonBallparkOpWithExtent = true;
                }
                bool extentContainsExtent1 =
                    !extent1 || extent->contains(NN_NO_CHECK(extent1));
                bool extentContainsExtent2 =
                    !extent2 || extent->contains(NN_NO_CHECK(extent2));
                if (!hasOpThatContainsAreaOfInterestAndNoGrid &&
                    extentContainsExtent1 && extentContainsExtent2) {
                    if (!op->hasBallparkTransformation() &&
                        op->gridsNeeded(nullptr, true).empty()) {
                        hasOpThatContainsAreaOfInterestAndNoGrid = true;
                    }
                }
                if (spatialCriterion ==
                    CoordinateOperationContext::SpatialCriterion::
                        STRICT_CONTAINMENT) {
                    if (!extentContainsExtent1 || !extentContainsExtent2) {
                        continue;
                    }
                } else if (spatialCriterion ==
                           CoordinateOperationContext::SpatialCriterion::
                               PARTIAL_INTERSECTION) {
                    bool extentIntersectsExtent1 =
                        !extent1 || extent->intersects(NN_NO_CHECK(extent1));
                    bool extentIntersectsExtent2 =
                        extent2 && extent->intersects(NN_NO_CHECK(extent2));
                    if (!extentIntersectsExtent1 || !extentIntersectsExtent2) {
                        continue;
                    }
                }
            }
            if (!op->hasBallparkTransformation()) {
                hasOnlyBallpark = false;
            }
            res.emplace_back(op);
        }

        // In case no operation has an extent and no result is found,
        // retain all initial operations that match accuracy criterion.
        if ((res.empty() && !hasNonBallparkOpWithExtent) ||
            (hasOnlyBallpark && hasNonBallparkWithoutExtent)) {
            for (const auto &op : sourceList) {
                if (desiredAccuracy != 0) {
                    const double accuracy = getAccuracy(op);
                    if (accuracy < 0 || accuracy > desiredAccuracy) {
                        continue;
                    }
                }
                if (!allowBallpark && op->hasBallparkTransformation()) {
                    continue;
                }
                res.emplace_back(op);
            }
        }
    }

    // ----------------------------------------------------------------------

    void sort() {

        // Precompute a number of parameters for each operation that will be
        // useful for the sorting.
        std::map<CoordinateOperation *, PrecomputedOpCharacteristics> map;
        const auto gridAvailabilityUse = context->getGridAvailabilityUse();
        for (const auto &op : res) {
            bool dummy = false;
            auto extentOp = getExtent(op, true, dummy);
            double area = 0.0;
            if (extentOp) {
                if (areaOfInterest) {
                    area = getPseudoArea(
                        extentOp->intersection(NN_NO_CHECK(areaOfInterest)));
                } else if (extent1 && extent2) {
                    auto x = extentOp->intersection(NN_NO_CHECK(extent1));
                    auto y = extentOp->intersection(NN_NO_CHECK(extent2));
                    area = getPseudoArea(x) + getPseudoArea(y) -
                           ((x && y)
                                ? getPseudoArea(x->intersection(NN_NO_CHECK(y)))
                                : 0.0);
                } else if (extent1) {
                    area = getPseudoArea(
                        extentOp->intersection(NN_NO_CHECK(extent1)));
                } else if (extent2) {
                    area = getPseudoArea(
                        extentOp->intersection(NN_NO_CHECK(extent2)));
                } else {
                    area = getPseudoArea(extentOp);
                }
            }

            bool hasGrids = false;
            bool gridsAvailable = true;
            bool gridsKnown = true;
            if (context->getAuthorityFactory()) {
                const auto gridsNeeded = op->gridsNeeded(
                    context->getAuthorityFactory()->databaseContext(),
                    gridAvailabilityUse ==
                        CoordinateOperationContext::GridAvailabilityUse::
                            KNOWN_AVAILABLE);
                for (const auto &gridDesc : gridsNeeded) {
                    hasGrids = true;
                    if (gridAvailabilityUse ==
                            CoordinateOperationContext::GridAvailabilityUse::
                                USE_FOR_SORTING &&
                        !gridDesc.available) {
                        gridsAvailable = false;
                    }
                    if (gridDesc.packageName.empty() &&
                        !(!gridDesc.url.empty() && gridDesc.openLicense) &&
                        !gridDesc.available) {
                        gridsKnown = false;
                    }
                }
            }

            const auto stepCount = getStepCount(op);

            bool isPROJExportable = false;
            auto formatter = io::PROJStringFormatter::create();
            try {
                op->exportToPROJString(formatter.get());
                // Grids might be missing, but at least this is something
                // PROJ could potentially process
                isPROJExportable = true;
            } catch (const std::exception &) {
            }

#if 0
            std::cerr << op->nameStr() << " ";
            std::cerr << area << " ";
            std::cerr << getAccuracy(op) << " ";
            std::cerr << isPROJExportable << " ";
            std::cerr << hasGrids << " ";
            std::cerr << gridsAvailable << " ";
            std::cerr << gridsKnown << " ";
            std::cerr << stepCount << " ";
            std::cerr << op->hasBallparkTransformation() << " ";
            std::cerr << isNullTransformation(op->nameStr()) << " ";
            std::cerr << std::endl;
#endif
            map[op.get()] = PrecomputedOpCharacteristics(
                area, getAccuracy(op), isPROJExportable, hasGrids,
                gridsAvailable, gridsKnown, stepCount,
                op->hasBallparkTransformation(),
                op->nameStr().find("ballpark vertical transformation") !=
                    std::string::npos,
                isNullTransformation(op->nameStr()));
        }

        // Sort !
        SortFunction sortFunc(map);
        std::sort(res.begin(), res.end(), sortFunc);

// Debug code to check consistency of the sort function
#ifdef DEBUG_SORT
        constexpr bool debugSort = true;
#elif !defined(NDEBUG)
        const bool debugSort = getenv("PROJ_DEBUG_SORT_FUNCT") != nullptr;
#endif
#if defined(DEBUG_SORT) || !defined(NDEBUG)
        if (debugSort) {
            const bool assertIfIssue =
                !(getenv("PROJ_DEBUG_SORT_FUNCT_ASSERT") != nullptr);
            for (size_t i = 0; i < res.size(); ++i) {
                for (size_t j = i + 1; j < res.size(); ++j) {
                    if (sortFunc(res[j], res[i])) {
#ifdef DEBUG_SORT
                        std::cerr << "Sorting issue with entry " << i << "("
                                  << res[i]->nameStr() << ") and " << j << "("
                                  << res[j]->nameStr() << ")" << std::endl;
#endif
                        if (assertIfIssue) {
                            assert(false);
                        }
                    }
                }
            }
        }
#endif
    }

    // ----------------------------------------------------------------------

    void removeSyntheticNullTransforms() {

        // If we have more than one result, and than the last result is the
        // default "Ballpark geographic offset" or "Ballpark geocentric
        // translation" operations we have synthetized, and that at least one
        // operation has the desired area of interest and does not require the
        // use of grids, remove it as all previous results are necessarily
        // better
        if (hasOpThatContainsAreaOfInterestAndNoGrid && res.size() > 1) {
            const auto &opLast = res.back();
            if (opLast->hasBallparkTransformation() ||
                isNullTransformation(opLast->nameStr())) {
                std::vector<CoordinateOperationNNPtr> resTemp;
                for (size_t i = 0; i < res.size() - 1; i++) {
                    resTemp.emplace_back(res[i]);
                }
                res = std::move(resTemp);
            }
        }
    }

    // ----------------------------------------------------------------------

    void removeUninterestingOps() {

        // Eliminate operations that bring nothing, ie for a given area of use,
        // do not keep operations that have similar or worse accuracy, but
        // involve more (non conversion) steps
        std::vector<CoordinateOperationNNPtr> resTemp;
        metadata::ExtentPtr lastExtent;
        double lastAccuracy = -1;
        size_t lastStepCount = 0;
        CoordinateOperationPtr lastOp;

        bool first = true;
        for (const auto &op : res) {
            const auto curAccuracy = getAccuracy(op);
            bool dummy = false;
            const auto curExtent = getExtent(op, true, dummy);
            const auto curStepCount = getTransformationStepCount(op);

            if (first) {
                resTemp.emplace_back(op);
                first = false;
            } else {
                if (lastOp->_isEquivalentTo(op.get())) {
                    continue;
                }
                const bool sameExtent =
                    ((!curExtent && !lastExtent) ||
                     (curExtent && lastExtent &&
                      curExtent->contains(NN_NO_CHECK(lastExtent)) &&
                      lastExtent->contains(NN_NO_CHECK(curExtent))));
                if (((curAccuracy >= lastAccuracy && lastAccuracy >= 0) ||
                     (curAccuracy < 0 && lastAccuracy >= 0)) &&
                    sameExtent && curStepCount > lastStepCount) {
                    continue;
                }

                resTemp.emplace_back(op);
            }

            lastOp = op.as_nullable();
            lastStepCount = curStepCount;
            lastExtent = curExtent;
            lastAccuracy = curAccuracy;
        }
        res = std::move(resTemp);
    }

    // ----------------------------------------------------------------------

    // cppcheck-suppress functionStatic
    void removeDuplicateOps() {

        if (res.size() <= 1) {
            return;
        }

        // When going from EPSG:4807 (NTF Paris) to EPSG:4171 (RGC93), we get
        // EPSG:7811, NTF (Paris) to RGF93 (2), 1 m
        // and unknown id, NTF (Paris) to NTF (1) + Inverse of RGF93 to NTF (2),
        // 1 m
        // both have same PROJ string and extent
        // Do not keep the later (that has more steps) as it adds no value.

        std::set<std::string> setPROJPlusExtent;
        std::vector<CoordinateOperationNNPtr> resTemp;
        for (const auto &op : res) {
            auto formatter = io::PROJStringFormatter::create();
            try {
                std::string key(op->exportToPROJString(formatter.get()));
                bool dummy = false;
                auto extentOp = getExtent(op, true, dummy);
                if (extentOp) {
                    const auto &geogElts = extentOp->geographicElements();
                    if (geogElts.size() == 1) {
                        auto bbox = dynamic_cast<
                            const metadata::GeographicBoundingBox *>(
                            geogElts[0].get());
                        if (bbox) {
                            double w = bbox->westBoundLongitude();
                            double s = bbox->southBoundLatitude();
                            double e = bbox->eastBoundLongitude();
                            double n = bbox->northBoundLatitude();
                            key += "-";
                            key += toString(w);
                            key += "-";
                            key += toString(s);
                            key += "-";
                            key += toString(e);
                            key += "-";
                            key += toString(n);
                        }
                    }
                }

                if (setPROJPlusExtent.find(key) == setPROJPlusExtent.end()) {
                    resTemp.emplace_back(op);
                    setPROJPlusExtent.insert(key);
                }
            } catch (const std::exception &) {
                resTemp.emplace_back(op);
            }
        }
        res = std::move(resTemp);
    }
};

// ---------------------------------------------------------------------------

/** \brief Filter operations and sort them given context.
 *
 * If a desired accuracy is specified, only keep operations whose accuracy
 * is at least the desired one.
 * If an area of interest is specified, only keep operations whose area of
 * use include the area of interest.
 * Then sort remaining operations by descending area of use, and increasing
 * accuracy.
 */
static std::vector<CoordinateOperationNNPtr>
filterAndSort(const std::vector<CoordinateOperationNNPtr> &sourceList,
              const CoordinateOperationContextNNPtr &context,
              const metadata::ExtentPtr &extent1,
              const metadata::ExtentPtr &extent2) {
#ifdef TRACE_CREATE_OPERATIONS
    ENTER_FUNCTION();
    logTrace("number of results before filter and sort: " +
             toString(static_cast<int>(sourceList.size())));
#endif
    auto resFiltered =
        FilterResults(sourceList, context, extent1, extent2, false)
            .andSort()
            .getRes();
#ifdef TRACE_CREATE_OPERATIONS
    logTrace("number of results after filter and sort: " +
             toString(static_cast<int>(resFiltered.size())));
#endif
    return resFiltered;
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
// Apply the inverse() method on all elements of the input list
static std::vector<CoordinateOperationNNPtr>
applyInverse(const std::vector<CoordinateOperationNNPtr> &list) {
    auto res = list;
    for (auto &op : res) {
#ifdef DEBUG
        auto opNew = op->inverse();
        assert(opNew->targetCRS()->isEquivalentTo(op->sourceCRS().get()));
        assert(opNew->sourceCRS()->isEquivalentTo(op->targetCRS().get()));
        op = opNew;
#else
        op = op->inverse();
#endif
    }
    return res;
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress

void CoordinateOperationFactory::Private::buildCRSIds(
    const crs::CRSNNPtr &crs, Private::Context &context,
    std::list<std::pair<std::string, std::string>> &ids) {
    const auto &authFactory = context.context->getAuthorityFactory();
    assert(authFactory);
    for (const auto &id : crs->identifiers()) {
        const auto &authName = *(id->codeSpace());
        const auto &code = id->code();
        if (!authName.empty()) {
            const auto tmpAuthFactory = io::AuthorityFactory::create(
                authFactory->databaseContext(), authName);
            try {
                // Consistency check for the ID attached to the object.
                // See https://github.com/OSGeo/PROJ/issues/1982 where EPSG:4656
                // is attached to a GeographicCRS whereas it is a ProjectedCRS
                if (tmpAuthFactory->createCoordinateReferenceSystem(code)
                        ->_isEquivalentTo(
                            crs.get(),
                            util::IComparable::Criterion::
                                EQUIVALENT_EXCEPT_AXIS_ORDER_GEOGCRS)) {
                    ids.emplace_back(authName, code);
                } else {
                    // TODO? log this inconsistency
                }
            } catch (const std::exception &) {
                // TODO? log this inconsistency
            }
        }
    }
    if (ids.empty()) {
        std::vector<io::AuthorityFactory::ObjectType> allowedObjects;
        auto geogCRS = dynamic_cast<const crs::GeographicCRS *>(crs.get());
        if (geogCRS) {
            allowedObjects.push_back(
                geogCRS->coordinateSystem()->axisList().size() == 2
                    ? io::AuthorityFactory::ObjectType::GEOGRAPHIC_2D_CRS
                    : io::AuthorityFactory::ObjectType::GEOGRAPHIC_3D_CRS);
        } else if (dynamic_cast<crs::ProjectedCRS *>(crs.get())) {
            allowedObjects.push_back(
                io::AuthorityFactory::ObjectType::PROJECTED_CRS);
        } else if (dynamic_cast<crs::VerticalCRS *>(crs.get())) {
            allowedObjects.push_back(
                io::AuthorityFactory::ObjectType::VERTICAL_CRS);
        }
        if (!allowedObjects.empty()) {

            const std::pair<io::AuthorityFactory::ObjectType, std::string> key(
                allowedObjects[0], crs->nameStr());
            auto iter = context.cacheNameToCRS.find(key);
            if (iter != context.cacheNameToCRS.end()) {
                ids = iter->second;
                return;
            }

            const auto &authFactoryName = authFactory->getAuthority();
            try {
                const auto tmpAuthFactory = io::AuthorityFactory::create(
                    authFactory->databaseContext(),
                    (authFactoryName.empty() || authFactoryName == "any")
                        ? std::string()
                        : authFactoryName);

                auto matches = tmpAuthFactory->createObjectsFromName(
                    crs->nameStr(), allowedObjects, false, 2);
                if (matches.size() == 1 &&
                    crs->_isEquivalentTo(
                        matches.front().get(),
                        util::IComparable::Criterion::EQUIVALENT) &&
                    !matches.front()->identifiers().empty()) {
                    const auto &tmpIds = matches.front()->identifiers();
                    ids.emplace_back(*(tmpIds[0]->codeSpace()),
                                     tmpIds[0]->code());
                }
            } catch (const std::exception &) {
            }
            context.cacheNameToCRS[key] = ids;
        }
    }
}

// ---------------------------------------------------------------------------

static std::vector<std::string>
getCandidateAuthorities(const io::AuthorityFactoryPtr &authFactory,
                        const std::string &srcAuthName,
                        const std::string &targetAuthName) {
    const auto &authFactoryName = authFactory->getAuthority();
    std::vector<std::string> authorities;
    if (authFactoryName == "any") {
        authorities.emplace_back();
    }
    if (authFactoryName.empty()) {
        authorities = authFactory->databaseContext()->getAllowedAuthorities(
            srcAuthName, targetAuthName);
        if (authorities.empty()) {
            authorities.emplace_back();
        }
    } else {
        authorities.emplace_back(authFactoryName);
    }
    return authorities;
}

// ---------------------------------------------------------------------------

// Look in the authority registry for operations from sourceCRS to targetCRS
std::vector<CoordinateOperationNNPtr>
CoordinateOperationFactory::Private::findOpsInRegistryDirect(
    const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
    Private::Context &context, bool &resNonEmptyBeforeFiltering) {
    const auto &authFactory = context.context->getAuthorityFactory();
    assert(authFactory);

#ifdef TRACE_CREATE_OPERATIONS
    ENTER_BLOCK("findOpsInRegistryDirect(" + objectAsStr(sourceCRS.get()) +
                " --> " + objectAsStr(targetCRS.get()) + ")");
#endif

    resNonEmptyBeforeFiltering = false;
    std::list<std::pair<std::string, std::string>> sourceIds;
    std::list<std::pair<std::string, std::string>> targetIds;
    buildCRSIds(sourceCRS, context, sourceIds);
    buildCRSIds(targetCRS, context, targetIds);

    const auto gridAvailabilityUse = context.context->getGridAvailabilityUse();
    for (const auto &idSrc : sourceIds) {
        const auto &srcAuthName = idSrc.first;
        const auto &srcCode = idSrc.second;
        for (const auto &idTarget : targetIds) {
            const auto &targetAuthName = idTarget.first;
            const auto &targetCode = idTarget.second;

            const auto authorities(getCandidateAuthorities(
                authFactory, srcAuthName, targetAuthName));
            std::vector<CoordinateOperationNNPtr> res;
            for (const auto &authority : authorities) {
                const auto authName =
                    authority == "any" ? std::string() : authority;
                const auto tmpAuthFactory = io::AuthorityFactory::create(
                    authFactory->databaseContext(), authName);
                auto resTmp =
                    tmpAuthFactory->createFromCoordinateReferenceSystemCodes(
                        srcAuthName, srcCode, targetAuthName, targetCode,
                        context.context->getUsePROJAlternativeGridNames(),
                        gridAvailabilityUse ==
                                CoordinateOperationContext::
                                    GridAvailabilityUse::
                                        DISCARD_OPERATION_IF_MISSING_GRID ||
                            gridAvailabilityUse ==
                                CoordinateOperationContext::
                                    GridAvailabilityUse::KNOWN_AVAILABLE,
                        gridAvailabilityUse ==
                            CoordinateOperationContext::GridAvailabilityUse::
                                KNOWN_AVAILABLE,
                        context.context->getDiscardSuperseded(), true, false,
                        context.extent1, context.extent2);
                res.insert(res.end(), resTmp.begin(), resTmp.end());
                if (authName == "PROJ") {
                    continue;
                }
                if (!res.empty()) {
                    resNonEmptyBeforeFiltering = true;
                    auto resFiltered =
                        FilterResults(res, context.context, context.extent1,
                                      context.extent2, false)
                            .getRes();
#ifdef TRACE_CREATE_OPERATIONS
                    logTrace("filtering reduced from " +
                             toString(static_cast<int>(res.size())) + " to " +
                             toString(static_cast<int>(resFiltered.size())));
#endif
                    return resFiltered;
                }
            }
        }
    }
    return std::vector<CoordinateOperationNNPtr>();
}

// ---------------------------------------------------------------------------

// Look in the authority registry for operations to targetCRS
std::vector<CoordinateOperationNNPtr>
CoordinateOperationFactory::Private::findOpsInRegistryDirectTo(
    const crs::CRSNNPtr &targetCRS, Private::Context &context) {
#ifdef TRACE_CREATE_OPERATIONS
    ENTER_BLOCK("findOpsInRegistryDirectTo({any} -->" +
                objectAsStr(targetCRS.get()) + ")");
#endif

    const auto &authFactory = context.context->getAuthorityFactory();
    assert(authFactory);

    std::list<std::pair<std::string, std::string>> ids;
    buildCRSIds(targetCRS, context, ids);

    const auto gridAvailabilityUse = context.context->getGridAvailabilityUse();
    for (const auto &id : ids) {
        const auto &targetAuthName = id.first;
        const auto &targetCode = id.second;

        const auto authorities(getCandidateAuthorities(
            authFactory, targetAuthName, targetAuthName));
        for (const auto &authority : authorities) {
            const auto tmpAuthFactory = io::AuthorityFactory::create(
                authFactory->databaseContext(),
                authority == "any" ? std::string() : authority);
            auto res = tmpAuthFactory->createFromCoordinateReferenceSystemCodes(
                std::string(), std::string(), targetAuthName, targetCode,
                context.context->getUsePROJAlternativeGridNames(),

                gridAvailabilityUse ==
                        CoordinateOperationContext::GridAvailabilityUse::
                            DISCARD_OPERATION_IF_MISSING_GRID ||
                    gridAvailabilityUse ==
                        CoordinateOperationContext::GridAvailabilityUse::
                            KNOWN_AVAILABLE,
                gridAvailabilityUse == CoordinateOperationContext::
                                           GridAvailabilityUse::KNOWN_AVAILABLE,
                context.context->getDiscardSuperseded(), true, true,
                context.extent1, context.extent2);
            if (!res.empty()) {
                auto resFiltered =
                    FilterResults(res, context.context, context.extent1,
                                  context.extent2, false)
                        .getRes();
#ifdef TRACE_CREATE_OPERATIONS
                logTrace("filtering reduced from " +
                         toString(static_cast<int>(res.size())) + " to " +
                         toString(static_cast<int>(resFiltered.size())));
#endif
                return resFiltered;
            }
        }
    }
    return std::vector<CoordinateOperationNNPtr>();
}

//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress

// Look in the authority registry for operations from sourceCRS to targetCRS
// using an intermediate pivot
std::vector<CoordinateOperationNNPtr>
CoordinateOperationFactory::Private::findsOpsInRegistryWithIntermediate(
    const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
    Private::Context &context,
    bool useCreateBetweenGeodeticCRSWithDatumBasedIntermediates) {

#ifdef TRACE_CREATE_OPERATIONS
    ENTER_BLOCK("findsOpsInRegistryWithIntermediate(" +
                objectAsStr(sourceCRS.get()) + " --> " +
                objectAsStr(targetCRS.get()) + ")");
#endif

    const auto &authFactory = context.context->getAuthorityFactory();
    assert(authFactory);

    std::list<std::pair<std::string, std::string>> sourceIds;
    std::list<std::pair<std::string, std::string>> targetIds;
    buildCRSIds(sourceCRS, context, sourceIds);
    buildCRSIds(targetCRS, context, targetIds);

    const auto gridAvailabilityUse = context.context->getGridAvailabilityUse();
    for (const auto &idSrc : sourceIds) {
        const auto &srcAuthName = idSrc.first;
        const auto &srcCode = idSrc.second;
        for (const auto &idTarget : targetIds) {
            const auto &targetAuthName = idTarget.first;
            const auto &targetCode = idTarget.second;

            const auto authorities(getCandidateAuthorities(
                authFactory, srcAuthName, targetAuthName));
            assert(!authorities.empty());

            const auto tmpAuthFactory = io::AuthorityFactory::create(
                authFactory->databaseContext(),
                (authFactory->getAuthority() == "any" || authorities.size() > 1)
                    ? std::string()
                    : authorities.front());

            std::vector<CoordinateOperationNNPtr> res;
            if (useCreateBetweenGeodeticCRSWithDatumBasedIntermediates) {
                res =
                    tmpAuthFactory
                        ->createBetweenGeodeticCRSWithDatumBasedIntermediates(
                            sourceCRS, srcAuthName, srcCode, targetCRS,
                            targetAuthName, targetCode,
                            context.context->getUsePROJAlternativeGridNames(),
                            gridAvailabilityUse ==
                                    CoordinateOperationContext::
                                        GridAvailabilityUse::
                                            DISCARD_OPERATION_IF_MISSING_GRID ||
                                gridAvailabilityUse ==
                                    CoordinateOperationContext::
                                        GridAvailabilityUse::KNOWN_AVAILABLE,
                            gridAvailabilityUse ==
                                CoordinateOperationContext::
                                    GridAvailabilityUse::KNOWN_AVAILABLE,
                            context.context->getDiscardSuperseded(),
                            authFactory->getAuthority() != "any" &&
                                    authorities.size() > 1
                                ? authorities
                                : std::vector<std::string>(),
                            context.extent1, context.extent2);
            } else {
                io::AuthorityFactory::ObjectType intermediateObjectType =
                    io::AuthorityFactory::ObjectType::CRS;

                // If doing GeogCRS --> GeogCRS, only use GeogCRS as
                // intermediate CRS
                // Avoid weird behavior when doing NAD83 -> NAD83(2011)
                // that would go through NAVD88 otherwise.
                if (context.context->getIntermediateCRS().empty() &&
                    dynamic_cast<const crs::GeographicCRS *>(sourceCRS.get()) &&
                    dynamic_cast<const crs::GeographicCRS *>(targetCRS.get())) {
                    intermediateObjectType =
                        io::AuthorityFactory::ObjectType::GEOGRAPHIC_CRS;
                }
                res = tmpAuthFactory->createFromCRSCodesWithIntermediates(
                    srcAuthName, srcCode, targetAuthName, targetCode,
                    context.context->getUsePROJAlternativeGridNames(),
                    gridAvailabilityUse ==
                            CoordinateOperationContext::GridAvailabilityUse::
                                DISCARD_OPERATION_IF_MISSING_GRID ||
                        gridAvailabilityUse ==
                            CoordinateOperationContext::GridAvailabilityUse::
                                KNOWN_AVAILABLE,
                    gridAvailabilityUse ==
                        CoordinateOperationContext::GridAvailabilityUse::
                            KNOWN_AVAILABLE,
                    context.context->getDiscardSuperseded(),
                    context.context->getIntermediateCRS(),
                    intermediateObjectType,
                    authFactory->getAuthority() != "any" &&
                            authorities.size() > 1
                        ? authorities
                        : std::vector<std::string>(),
                    context.extent1, context.extent2);
            }
            if (!res.empty()) {

                auto resFiltered =
                    FilterResults(res, context.context, context.extent1,
                                  context.extent2, false)
                        .getRes();
#ifdef TRACE_CREATE_OPERATIONS
                logTrace("filtering reduced from " +
                         toString(static_cast<int>(res.size())) + " to " +
                         toString(static_cast<int>(resFiltered.size())));
#endif
                return resFiltered;
            }
        }
    }
    return std::vector<CoordinateOperationNNPtr>();
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
static TransformationNNPtr
createBallparkGeographicOffset(const crs::CRSNNPtr &sourceCRS,
                               const crs::CRSNNPtr &targetCRS,
                               const io::DatabaseContextPtr &dbContext) {

    const crs::GeographicCRS *geogSrc =
        dynamic_cast<const crs::GeographicCRS *>(sourceCRS.get());
    const crs::GeographicCRS *geogDst =
        dynamic_cast<const crs::GeographicCRS *>(targetCRS.get());
    const bool isSameDatum = geogSrc && geogDst &&
                             geogSrc->datumNonNull(dbContext)->_isEquivalentTo(
                                 geogDst->datumNonNull(dbContext).get(),
                                 util::IComparable::Criterion::EQUIVALENT);

    auto name = buildOpName(isSameDatum ? NULL_GEOGRAPHIC_OFFSET
                                        : BALLPARK_GEOGRAPHIC_OFFSET,
                            sourceCRS, targetCRS);

    const auto &sourceCRSExtent = getExtent(sourceCRS);
    const auto &targetCRSExtent = getExtent(targetCRS);
    const bool sameExtent =
        sourceCRSExtent && targetCRSExtent &&
        sourceCRSExtent->_isEquivalentTo(
            targetCRSExtent.get(), util::IComparable::Criterion::EQUIVALENT);

    util::PropertyMap map;
    map.set(common::IdentifiedObject::NAME_KEY, name)
        .set(common::ObjectUsage::DOMAIN_OF_VALIDITY_KEY,
             sameExtent ? NN_NO_CHECK(sourceCRSExtent)
                        : metadata::Extent::WORLD);
    const common::Angle angle0(0);

    std::vector<metadata::PositionalAccuracyNNPtr> accuracies;
    if (isSameDatum) {
        accuracies.emplace_back(metadata::PositionalAccuracy::create("0"));
    }

    const auto singleSourceCRS =
        dynamic_cast<const crs::SingleCRS *>(sourceCRS.get());
    const auto singleTargetCRS =
        dynamic_cast<const crs::SingleCRS *>(targetCRS.get());
    if ((singleSourceCRS &&
         singleSourceCRS->coordinateSystem()->axisList().size() == 3) ||
        (singleTargetCRS &&
         singleTargetCRS->coordinateSystem()->axisList().size() == 3)) {
        return Transformation::createGeographic3DOffsets(
            map, sourceCRS, targetCRS, angle0, angle0, common::Length(0),
            accuracies);
    } else {
        return Transformation::createGeographic2DOffsets(
            map, sourceCRS, targetCRS, angle0, angle0, accuracies);
    }
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress

// ---------------------------------------------------------------------------

struct MyPROJStringExportableGeodToGeod final
    : public io::IPROJStringExportable {
    crs::GeodeticCRSPtr geodSrc{};
    crs::GeodeticCRSPtr geodDst{};

    MyPROJStringExportableGeodToGeod(const crs::GeodeticCRSPtr &geodSrcIn,
                                     const crs::GeodeticCRSPtr &geodDstIn)
        : geodSrc(geodSrcIn), geodDst(geodDstIn) {}

    ~MyPROJStringExportableGeodToGeod() override;

    void
    // cppcheck-suppress functionStatic
    _exportToPROJString(io::PROJStringFormatter *formatter) const override {

        formatter->startInversion();
        geodSrc->_exportToPROJString(formatter);
        formatter->stopInversion();
        geodDst->_exportToPROJString(formatter);
    }
};

MyPROJStringExportableGeodToGeod::~MyPROJStringExportableGeodToGeod() = default;

// ---------------------------------------------------------------------------

struct MyPROJStringExportableHorizVertical final
    : public io::IPROJStringExportable {
    CoordinateOperationPtr horizTransform{};
    CoordinateOperationPtr verticalTransform{};
    crs::GeographicCRSPtr geogDst{};

    MyPROJStringExportableHorizVertical(
        const CoordinateOperationPtr &horizTransformIn,
        const CoordinateOperationPtr &verticalTransformIn,
        const crs::GeographicCRSPtr &geogDstIn)
        : horizTransform(horizTransformIn),
          verticalTransform(verticalTransformIn), geogDst(geogDstIn) {}

    ~MyPROJStringExportableHorizVertical() override;

    void
    // cppcheck-suppress functionStatic
    _exportToPROJString(io::PROJStringFormatter *formatter) const override {

        formatter->pushOmitZUnitConversion();

        horizTransform->_exportToPROJString(formatter);

        formatter->startInversion();
        geogDst->addAngularUnitConvertAndAxisSwap(formatter);
        formatter->stopInversion();

        formatter->popOmitZUnitConversion();

        formatter->pushOmitHorizontalConversionInVertTransformation();
        verticalTransform->_exportToPROJString(formatter);
        formatter->popOmitHorizontalConversionInVertTransformation();

        formatter->pushOmitZUnitConversion();
        geogDst->addAngularUnitConvertAndAxisSwap(formatter);
        formatter->popOmitZUnitConversion();
    }
};

MyPROJStringExportableHorizVertical::~MyPROJStringExportableHorizVertical() =
    default;

// ---------------------------------------------------------------------------

struct MyPROJStringExportableHorizVerticalHorizPROJBased final
    : public io::IPROJStringExportable {
    CoordinateOperationPtr opSrcCRSToGeogCRS{};
    CoordinateOperationPtr verticalTransform{};
    CoordinateOperationPtr opGeogCRStoDstCRS{};
    crs::GeographicCRSPtr interpolationGeogCRS{};

    MyPROJStringExportableHorizVerticalHorizPROJBased(
        const CoordinateOperationPtr &opSrcCRSToGeogCRSIn,
        const CoordinateOperationPtr &verticalTransformIn,
        const CoordinateOperationPtr &opGeogCRStoDstCRSIn,
        const crs::GeographicCRSPtr &interpolationGeogCRSIn)
        : opSrcCRSToGeogCRS(opSrcCRSToGeogCRSIn),
          verticalTransform(verticalTransformIn),
          opGeogCRStoDstCRS(opGeogCRStoDstCRSIn),
          interpolationGeogCRS(interpolationGeogCRSIn) {}

    ~MyPROJStringExportableHorizVerticalHorizPROJBased() override;

    void
    // cppcheck-suppress functionStatic
    _exportToPROJString(io::PROJStringFormatter *formatter) const override {

        bool saveHorizontalCoords = false;
        const auto transf =
            dynamic_cast<Transformation *>(opSrcCRSToGeogCRS.get());
        if (transf && opSrcCRSToGeogCRS->sourceCRS()->_isEquivalentTo(
                          opGeogCRStoDstCRS->targetCRS()
                              ->demoteTo2D(std::string(), nullptr)
                              .get(),
                          util::IComparable::Criterion::EQUIVALENT)) {
            const int methodEPSGCode = transf->method()->getEPSGCode();

            const bool bGeocentricTranslation =
                methodEPSGCode ==
                    EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_GEOCENTRIC ||
                methodEPSGCode ==
                    EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_GEOGRAPHIC_2D ||
                methodEPSGCode ==
                    EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_GEOGRAPHIC_3D;

            if ((bGeocentricTranslation &&
                 !(transf->parameterValueNumericAsSI(
                       EPSG_CODE_PARAMETER_X_AXIS_TRANSLATION) == 0 &&
                   transf->parameterValueNumericAsSI(
                       EPSG_CODE_PARAMETER_Y_AXIS_TRANSLATION) == 0 &&
                   transf->parameterValueNumericAsSI(
                       EPSG_CODE_PARAMETER_Z_AXIS_TRANSLATION) == 0)) ||

                methodEPSGCode ==
                    EPSG_CODE_METHOD_COORDINATE_FRAME_GEOCENTRIC ||
                methodEPSGCode ==
                    EPSG_CODE_METHOD_COORDINATE_FRAME_GEOGRAPHIC_2D ||
                methodEPSGCode ==
                    EPSG_CODE_METHOD_COORDINATE_FRAME_GEOGRAPHIC_3D ||
                methodEPSGCode ==
                    EPSG_CODE_METHOD_TIME_DEPENDENT_COORDINATE_FRAME_GEOCENTRIC ||
                methodEPSGCode ==
                    EPSG_CODE_METHOD_TIME_DEPENDENT_COORDINATE_FRAME_GEOGRAPHIC_2D ||
                methodEPSGCode ==
                    EPSG_CODE_METHOD_TIME_DEPENDENT_COORDINATE_FRAME_GEOGRAPHIC_3D ||
                methodEPSGCode == EPSG_CODE_METHOD_POSITION_VECTOR_GEOCENTRIC ||
                methodEPSGCode ==
                    EPSG_CODE_METHOD_POSITION_VECTOR_GEOGRAPHIC_2D ||
                methodEPSGCode ==
                    EPSG_CODE_METHOD_POSITION_VECTOR_GEOGRAPHIC_3D ||
                methodEPSGCode ==
                    EPSG_CODE_METHOD_TIME_DEPENDENT_POSITION_VECTOR_GEOCENTRIC ||
                methodEPSGCode ==
                    EPSG_CODE_METHOD_TIME_DEPENDENT_POSITION_VECTOR_GEOGRAPHIC_2D ||
                methodEPSGCode ==
                    EPSG_CODE_METHOD_TIME_DEPENDENT_POSITION_VECTOR_GEOGRAPHIC_3D) {
                saveHorizontalCoords = true;
            }
        }

        if (saveHorizontalCoords) {
            formatter->addStep("push");
            formatter->addParam("v_1");
            formatter->addParam("v_2");
        }

        formatter->pushOmitZUnitConversion();

        opSrcCRSToGeogCRS->_exportToPROJString(formatter);

        formatter->startInversion();
        interpolationGeogCRS->addAngularUnitConvertAndAxisSwap(formatter);
        formatter->stopInversion();

        formatter->popOmitZUnitConversion();

        formatter->pushOmitHorizontalConversionInVertTransformation();
        verticalTransform->_exportToPROJString(formatter);
        formatter->popOmitHorizontalConversionInVertTransformation();

        formatter->pushOmitZUnitConversion();

        interpolationGeogCRS->addAngularUnitConvertAndAxisSwap(formatter);

        opGeogCRStoDstCRS->_exportToPROJString(formatter);

        formatter->popOmitZUnitConversion();

        if (saveHorizontalCoords) {
            formatter->addStep("pop");
            formatter->addParam("v_1");
            formatter->addParam("v_2");
        }
    }
};

MyPROJStringExportableHorizVerticalHorizPROJBased::
    ~MyPROJStringExportableHorizVerticalHorizPROJBased() = default;

//! @endcond

} // namespace operation
NS_PROJ_END

#if 0
namespace dropbox{ namespace oxygen {
template<> nn<std::shared_ptr<NS_PROJ::operation::MyPROJStringExportableGeodToGeod>>::~nn() = default;
template<> nn<std::shared_ptr<NS_PROJ::operation::MyPROJStringExportableHorizVertical>>::~nn() = default;
template<> nn<std::shared_ptr<NS_PROJ::operation::MyPROJStringExportableHorizVerticalHorizPROJBased>>::~nn() = default;
}}
#endif

NS_PROJ_START
namespace operation {

//! @cond Doxygen_Suppress

// ---------------------------------------------------------------------------

static std::string buildTransfName(const std::string &srcName,
                                   const std::string &targetName) {
    std::string name("Transformation from ");
    name += srcName;
    name += " to ";
    name += targetName;
    return name;
}

// ---------------------------------------------------------------------------

static std::string buildConvName(const std::string &srcName,
                                 const std::string &targetName) {
    std::string name("Conversion from ");
    name += srcName;
    name += " to ";
    name += targetName;
    return name;
}

// ---------------------------------------------------------------------------

static SingleOperationNNPtr createPROJBased(
    const util::PropertyMap &properties,
    const io::IPROJStringExportableNNPtr &projExportable,
    const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
    const crs::CRSPtr &interpolationCRS,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies,
    bool hasBallparkTransformation) {
    return util::nn_static_pointer_cast<SingleOperation>(
        PROJBasedOperation::create(properties, projExportable, false, sourceCRS,
                                   targetCRS, interpolationCRS, accuracies,
                                   hasBallparkTransformation));
}

// ---------------------------------------------------------------------------

static CoordinateOperationNNPtr
createGeodToGeodPROJBased(const crs::CRSNNPtr &geodSrc,
                          const crs::CRSNNPtr &geodDst) {

    auto exportable = util::nn_make_shared<MyPROJStringExportableGeodToGeod>(
        util::nn_dynamic_pointer_cast<crs::GeodeticCRS>(geodSrc),
        util::nn_dynamic_pointer_cast<crs::GeodeticCRS>(geodDst));

    auto properties = util::PropertyMap().set(
        common::IdentifiedObject::NAME_KEY,
        buildTransfName(geodSrc->nameStr(), geodDst->nameStr()));
    return createPROJBased(properties, exportable, geodSrc, geodDst, nullptr,
                           {}, false);
}

// ---------------------------------------------------------------------------

static std::string
getRemarks(const std::vector<operation::CoordinateOperationNNPtr> &ops) {
    std::string remarks;
    for (const auto &op : ops) {
        const auto &opRemarks = op->remarks();
        if (!opRemarks.empty()) {
            if (!remarks.empty()) {
                remarks += '\n';
            }

            std::string opName(op->nameStr());
            if (starts_with(opName, INVERSE_OF)) {
                opName = opName.substr(INVERSE_OF.size());
            }

            remarks += "For ";
            remarks += opName;

            const auto &ids = op->identifiers();
            if (!ids.empty()) {
                std::string authority(*ids.front()->codeSpace());
                if (starts_with(authority, "INVERSE(") &&
                    authority.back() == ')') {
                    authority = authority.substr(strlen("INVERSE("),
                                                 authority.size() - 1 -
                                                     strlen("INVERSE("));
                }
                if (starts_with(authority, "DERIVED_FROM(") &&
                    authority.back() == ')') {
                    authority = authority.substr(strlen("DERIVED_FROM("),
                                                 authority.size() - 1 -
                                                     strlen("DERIVED_FROM("));
                }

                remarks += " (";
                remarks += authority;
                remarks += ':';
                remarks += ids.front()->code();
                remarks += ')';
            }
            remarks += ": ";
            remarks += opRemarks;
        }
    }
    return remarks;
}

// ---------------------------------------------------------------------------

static CoordinateOperationNNPtr createHorizVerticalPROJBased(
    const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
    const operation::CoordinateOperationNNPtr &horizTransform,
    const operation::CoordinateOperationNNPtr &verticalTransform,
    bool checkExtent) {

    auto geogDst = util::nn_dynamic_pointer_cast<crs::GeographicCRS>(targetCRS);
    assert(geogDst);

    auto exportable = util::nn_make_shared<MyPROJStringExportableHorizVertical>(
        horizTransform, verticalTransform, geogDst);

    const bool horizTransformIsNoOp =
        starts_with(horizTransform->nameStr(), NULL_GEOGRAPHIC_OFFSET) &&
        horizTransform->nameStr().find(" + ") == std::string::npos;
    if (horizTransformIsNoOp) {
        auto properties = util::PropertyMap();
        properties.set(common::IdentifiedObject::NAME_KEY,
                       verticalTransform->nameStr());
        bool dummy = false;
        auto extent = getExtent(verticalTransform, true, dummy);
        if (extent) {
            properties.set(common::ObjectUsage::DOMAIN_OF_VALIDITY_KEY,
                           NN_NO_CHECK(extent));
        }
        const auto &remarks = verticalTransform->remarks();
        if (!remarks.empty()) {
            properties.set(common::IdentifiedObject::REMARKS_KEY, remarks);
        }
        return createPROJBased(
            properties, exportable, sourceCRS, targetCRS, nullptr,
            verticalTransform->coordinateOperationAccuracies(),
            verticalTransform->hasBallparkTransformation());
    } else {
        bool emptyIntersection = false;
        auto ops = std::vector<CoordinateOperationNNPtr>{horizTransform,
                                                         verticalTransform};
        auto extent = getExtent(ops, true, emptyIntersection);
        if (checkExtent && emptyIntersection) {
            std::string msg(
                "empty intersection of area of validity of concatenated "
                "operations");
            throw InvalidOperationEmptyIntersection(msg);
        }
        auto properties = util::PropertyMap();
        properties.set(common::IdentifiedObject::NAME_KEY,
                       computeConcatenatedName(ops));

        if (extent) {
            properties.set(common::ObjectUsage::DOMAIN_OF_VALIDITY_KEY,
                           NN_NO_CHECK(extent));
        }

        const auto remarks = getRemarks(ops);
        if (!remarks.empty()) {
            properties.set(common::IdentifiedObject::REMARKS_KEY, remarks);
        }

        std::vector<metadata::PositionalAccuracyNNPtr> accuracies;
        const double accuracy = getAccuracy(ops);
        if (accuracy >= 0.0) {
            accuracies.emplace_back(
                metadata::PositionalAccuracy::create(toString(accuracy)));
        }

        return createPROJBased(
            properties, exportable, sourceCRS, targetCRS, nullptr, accuracies,
            horizTransform->hasBallparkTransformation() ||
                verticalTransform->hasBallparkTransformation());
    }
}

// ---------------------------------------------------------------------------

static CoordinateOperationNNPtr createHorizVerticalHorizPROJBased(
    const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
    const operation::CoordinateOperationNNPtr &opSrcCRSToGeogCRS,
    const operation::CoordinateOperationNNPtr &verticalTransform,
    const operation::CoordinateOperationNNPtr &opGeogCRStoDstCRS,
    const crs::GeographicCRSPtr &interpolationGeogCRS, bool checkExtent) {

    auto exportable =
        util::nn_make_shared<MyPROJStringExportableHorizVerticalHorizPROJBased>(
            opSrcCRSToGeogCRS, verticalTransform, opGeogCRStoDstCRS,
            interpolationGeogCRS);

    std::vector<CoordinateOperationNNPtr> ops;
    if (!(starts_with(opSrcCRSToGeogCRS->nameStr(), NULL_GEOGRAPHIC_OFFSET) &&
          opSrcCRSToGeogCRS->nameStr().find(" + ") == std::string::npos)) {
        ops.emplace_back(opSrcCRSToGeogCRS);
    }
    ops.emplace_back(verticalTransform);
    if (!(starts_with(opGeogCRStoDstCRS->nameStr(), NULL_GEOGRAPHIC_OFFSET) &&
          opGeogCRStoDstCRS->nameStr().find(" + ") == std::string::npos)) {
        ops.emplace_back(opGeogCRStoDstCRS);
    }

    bool hasBallparkTransformation = false;
    for (const auto &op : ops) {
        hasBallparkTransformation |= op->hasBallparkTransformation();
    }
    bool emptyIntersection = false;
    auto extent = getExtent(ops, false, emptyIntersection);
    if (checkExtent && emptyIntersection) {
        std::string msg(
            "empty intersection of area of validity of concatenated "
            "operations");
        throw InvalidOperationEmptyIntersection(msg);
    }
    auto properties = util::PropertyMap();
    properties.set(common::IdentifiedObject::NAME_KEY,
                   computeConcatenatedName(ops));

    if (extent) {
        properties.set(common::ObjectUsage::DOMAIN_OF_VALIDITY_KEY,
                       NN_NO_CHECK(extent));
    }

    const auto remarks = getRemarks(ops);
    if (!remarks.empty()) {
        properties.set(common::IdentifiedObject::REMARKS_KEY, remarks);
    }

    std::vector<metadata::PositionalAccuracyNNPtr> accuracies;
    const double accuracy = getAccuracy(ops);
    if (accuracy >= 0.0) {
        accuracies.emplace_back(
            metadata::PositionalAccuracy::create(toString(accuracy)));
    }

    return createPROJBased(properties, exportable, sourceCRS, targetCRS,
                           nullptr, accuracies, hasBallparkTransformation);
}

//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress

std::vector<CoordinateOperationNNPtr>
CoordinateOperationFactory::Private::createOperationsGeogToGeog(
    std::vector<CoordinateOperationNNPtr> &res, const crs::CRSNNPtr &sourceCRS,
    const crs::CRSNNPtr &targetCRS, Private::Context &context,
    const crs::GeographicCRS *geogSrc, const crs::GeographicCRS *geogDst) {

    assert(sourceCRS.get() == geogSrc);
    assert(targetCRS.get() == geogDst);

    const auto &src_pm = geogSrc->primeMeridian()->longitude();
    const auto &dst_pm = geogDst->primeMeridian()->longitude();
    auto offset_pm =
        (src_pm.unit() == dst_pm.unit())
            ? common::Angle(src_pm.value() - dst_pm.value(), src_pm.unit())
            : common::Angle(
                  src_pm.convertToUnit(common::UnitOfMeasure::DEGREE) -
                      dst_pm.convertToUnit(common::UnitOfMeasure::DEGREE),
                  common::UnitOfMeasure::DEGREE);

    double vconvSrc = 1.0;
    const auto &srcCS = geogSrc->coordinateSystem();
    const auto &srcAxisList = srcCS->axisList();
    if (srcAxisList.size() == 3) {
        vconvSrc = srcAxisList[2]->unit().conversionToSI();
    }
    double vconvDst = 1.0;
    const auto &dstCS = geogDst->coordinateSystem();
    const auto &dstAxisList = dstCS->axisList();
    if (dstAxisList.size() == 3) {
        vconvDst = dstAxisList[2]->unit().conversionToSI();
    }

    std::string name(buildTransfName(geogSrc->nameStr(), geogDst->nameStr()));

    const auto &authFactory = context.context->getAuthorityFactory();
    const auto dbContext =
        authFactory ? authFactory->databaseContext().as_nullable() : nullptr;

    const bool sameDatum = geogSrc->datumNonNull(dbContext)->_isEquivalentTo(
        geogDst->datumNonNull(dbContext).get(),
        util::IComparable::Criterion::EQUIVALENT);

    // Do the CRS differ by their axis order ?
    bool axisReversal2D = false;
    bool axisReversal3D = false;
    if (!srcCS->_isEquivalentTo(dstCS.get(),
                                util::IComparable::Criterion::EQUIVALENT)) {
        auto srcOrder = srcCS->axisOrder();
        auto dstOrder = dstCS->axisOrder();
        if (((srcOrder == cs::EllipsoidalCS::AxisOrder::LAT_NORTH_LONG_EAST ||
              srcOrder == cs::EllipsoidalCS::AxisOrder::
                              LAT_NORTH_LONG_EAST_HEIGHT_UP) &&
             (dstOrder == cs::EllipsoidalCS::AxisOrder::LONG_EAST_LAT_NORTH ||
              dstOrder == cs::EllipsoidalCS::AxisOrder::
                              LONG_EAST_LAT_NORTH_HEIGHT_UP)) ||
            ((srcOrder == cs::EllipsoidalCS::AxisOrder::LONG_EAST_LAT_NORTH ||
              srcOrder == cs::EllipsoidalCS::AxisOrder::
                              LONG_EAST_LAT_NORTH_HEIGHT_UP) &&
             (dstOrder == cs::EllipsoidalCS::AxisOrder::LAT_NORTH_LONG_EAST ||
              dstOrder == cs::EllipsoidalCS::AxisOrder::
                              LAT_NORTH_LONG_EAST_HEIGHT_UP))) {
            if (srcAxisList.size() == 3 || dstAxisList.size() == 3)
                axisReversal3D = true;
            else
                axisReversal2D = true;
        }
    }

    // Do they differ by vertical units ?
    if (vconvSrc != vconvDst && geogSrc->ellipsoid()->_isEquivalentTo(
                                    geogDst->ellipsoid().get(),
                                    util::IComparable::Criterion::EQUIVALENT)) {
        if (offset_pm.value() == 0 && !axisReversal2D && !axisReversal3D) {
            // If only by vertical units, use a Change of Vertical
            // Unit
            // transformation
            const double factor = vconvSrc / vconvDst;
            auto conv = Conversion::createChangeVerticalUnit(
                util::PropertyMap().set(common::IdentifiedObject::NAME_KEY,
                                        name),
                common::Scale(factor));
            conv->setCRSs(sourceCRS, targetCRS, nullptr);
            conv->setHasBallparkTransformation(!sameDatum);
            res.push_back(conv);
            return res;
        } else {
            auto op = createGeodToGeodPROJBased(sourceCRS, targetCRS);
            op->setHasBallparkTransformation(!sameDatum);
            res.emplace_back(op);
            return res;
        }
    }

    // Do the CRS differ only by their axis order ?
    if (sameDatum && (axisReversal2D || axisReversal3D)) {
        auto conv = Conversion::createAxisOrderReversal(axisReversal3D);
        conv->setCRSs(sourceCRS, targetCRS, nullptr);
        res.emplace_back(conv);
        return res;
    }

    std::vector<CoordinateOperationNNPtr> steps;
    // If both are geographic and only differ by their prime
    // meridian,
    // apply a longitude rotation transformation.
    if (geogSrc->ellipsoid()->_isEquivalentTo(
            geogDst->ellipsoid().get(),
            util::IComparable::Criterion::EQUIVALENT) &&
        src_pm.getSIValue() != dst_pm.getSIValue()) {

        steps.emplace_back(Transformation::createLongitudeRotation(
            util::PropertyMap()
                .set(common::IdentifiedObject::NAME_KEY, name)
                .set(common::ObjectUsage::DOMAIN_OF_VALIDITY_KEY,
                     metadata::Extent::WORLD),
            sourceCRS, targetCRS, offset_pm));
        // If only the target has a non-zero prime meridian, chain a
        // null geographic offset and then the longitude rotation
    } else if (src_pm.getSIValue() == 0 && dst_pm.getSIValue() != 0) {
        auto datum = datum::GeodeticReferenceFrame::create(
            util::PropertyMap(), geogDst->ellipsoid(),
            util::optional<std::string>(), geogSrc->primeMeridian());
        std::string interm_crs_name(geogDst->nameStr());
        interm_crs_name += " altered to use prime meridian of ";
        interm_crs_name += geogSrc->nameStr();
        auto interm_crs =
            util::nn_static_pointer_cast<crs::CRS>(crs::GeographicCRS::create(
                util::PropertyMap()
                    .set(common::IdentifiedObject::NAME_KEY, interm_crs_name)
                    .set(common::ObjectUsage::DOMAIN_OF_VALIDITY_KEY,
                         metadata::Extent::WORLD),
                datum, dstCS));

        steps.emplace_back(
            createBallparkGeographicOffset(sourceCRS, interm_crs, dbContext));

        steps.emplace_back(Transformation::createLongitudeRotation(
            util::PropertyMap()
                .set(common::IdentifiedObject::NAME_KEY,
                     buildTransfName(geogSrc->nameStr(), interm_crs->nameStr()))
                .set(common::ObjectUsage::DOMAIN_OF_VALIDITY_KEY,
                     metadata::Extent::WORLD),
            interm_crs, targetCRS, offset_pm));

    } else {
        // If the prime meridians are different, chain a longitude
        // rotation and the null geographic offset.
        if (src_pm.getSIValue() != dst_pm.getSIValue()) {
            auto datum = datum::GeodeticReferenceFrame::create(
                util::PropertyMap(), geogSrc->ellipsoid(),
                util::optional<std::string>(), geogDst->primeMeridian());
            std::string interm_crs_name(geogSrc->nameStr());
            interm_crs_name += " altered to use prime meridian of ";
            interm_crs_name += geogDst->nameStr();
            auto interm_crs = util::nn_static_pointer_cast<crs::CRS>(
                crs::GeographicCRS::create(
                    util::PropertyMap().set(common::IdentifiedObject::NAME_KEY,
                                            interm_crs_name),
                    datum, srcCS));

            steps.emplace_back(Transformation::createLongitudeRotation(
                util::PropertyMap()
                    .set(common::IdentifiedObject::NAME_KEY,
                         buildTransfName(geogSrc->nameStr(),
                                         interm_crs->nameStr()))
                    .set(common::ObjectUsage::DOMAIN_OF_VALIDITY_KEY,
                         metadata::Extent::WORLD),
                sourceCRS, interm_crs, offset_pm));
            steps.emplace_back(createBallparkGeographicOffset(
                interm_crs, targetCRS, dbContext));
        } else {
            steps.emplace_back(createBallparkGeographicOffset(
                sourceCRS, targetCRS, dbContext));
        }
    }

    auto op = ConcatenatedOperation::createComputeMetadata(
        steps, disallowEmptyIntersection);
    op->setHasBallparkTransformation(!sameDatum);
    res.emplace_back(op);
    return res;
}

// ---------------------------------------------------------------------------

static bool hasIdentifiers(const CoordinateOperationNNPtr &op) {
    if (!op->identifiers().empty()) {
        return true;
    }
    auto concatenated = dynamic_cast<const ConcatenatedOperation *>(op.get());
    if (concatenated) {
        for (const auto &subOp : concatenated->operations()) {
            if (hasIdentifiers(subOp)) {
                return true;
            }
        }
    }
    return false;
}

// ---------------------------------------------------------------------------

static std::vector<crs::CRSNNPtr>
findCandidateGeodCRSForDatum(const io::AuthorityFactoryPtr &authFactory,
                             const crs::GeodeticCRS *crs,
                             const datum::GeodeticReferenceFrame *datum) {
    std::vector<crs::CRSNNPtr> candidates;
    assert(datum);
    const auto &ids = datum->identifiers();
    const auto &datumName = datum->nameStr();
    if (!ids.empty()) {
        for (const auto &id : ids) {
            const auto &authName = *(id->codeSpace());
            const auto &code = id->code();
            if (!authName.empty()) {
                const auto crsIds = crs->identifiers();
                const auto tmpFactory =
                    (crsIds.size() == 1 &&
                     *(crsIds.front()->codeSpace()) == authName)
                        ? io::AuthorityFactory::create(
                              authFactory->databaseContext(), authName)
                              .as_nullable()
                        : authFactory;
                auto l_candidates = tmpFactory->createGeodeticCRSFromDatum(
                    authName, code, std::string());
                for (const auto &candidate : l_candidates) {
                    candidates.emplace_back(candidate);
                }
            }
        }
    } else if (datumName != "unknown" && datumName != "unnamed") {
        auto matches = authFactory->createObjectsFromName(
            datumName,
            {io::AuthorityFactory::ObjectType::GEODETIC_REFERENCE_FRAME}, false,
            2);
        if (matches.size() == 1) {
            const auto &match = matches.front();
            if (datum->_isEquivalentTo(
                    match.get(), util::IComparable::Criterion::EQUIVALENT) &&
                !match->identifiers().empty()) {
                return findCandidateGeodCRSForDatum(
                    authFactory, crs,
                    dynamic_cast<const datum::GeodeticReferenceFrame *>(
                        match.get()));
            }
        }
    }
    return candidates;
}

// ---------------------------------------------------------------------------

void CoordinateOperationFactory::Private::setCRSs(
    CoordinateOperation *co, const crs::CRSNNPtr &sourceCRS,
    const crs::CRSNNPtr &targetCRS) {
    co->setCRSs(sourceCRS, targetCRS, nullptr);

    auto invCO = dynamic_cast<InverseCoordinateOperation *>(co);
    if (invCO) {
        invCO->forwardOperation()->setCRSs(targetCRS, sourceCRS, nullptr);
    }

    auto transf = dynamic_cast<Transformation *>(co);
    if (transf) {
        transf->inverseAsTransformation()->setCRSs(targetCRS, sourceCRS,
                                                   nullptr);
    }

    auto concat = dynamic_cast<ConcatenatedOperation *>(co);
    if (concat) {
        auto first = concat->operations().front().get();
        auto &firstTarget(first->targetCRS());
        if (firstTarget) {
            setCRSs(first, sourceCRS, NN_NO_CHECK(firstTarget));
        }
        auto last = concat->operations().back().get();
        auto &lastSource(last->sourceCRS());
        if (lastSource) {
            setCRSs(last, NN_NO_CHECK(lastSource), targetCRS);
        }
    }
}

// ---------------------------------------------------------------------------

static bool hasResultSetOnlyResultsWithPROJStep(
    const std::vector<CoordinateOperationNNPtr> &res) {
    for (const auto &op : res) {
        auto concat = dynamic_cast<const ConcatenatedOperation *>(op.get());
        if (concat) {
            bool hasPROJStep = false;
            const auto &steps = concat->operations();
            for (const auto &step : steps) {
                const auto &ids = step->identifiers();
                if (!ids.empty()) {
                    const auto &opAuthority = *(ids.front()->codeSpace());
                    if (opAuthority == "PROJ" ||
                        opAuthority == "INVERSE(PROJ)" ||
                        opAuthority == "DERIVED_FROM(PROJ)") {
                        hasPROJStep = true;
                        break;
                    }
                }
            }
            if (!hasPROJStep) {
                return false;
            }
        } else {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------

void CoordinateOperationFactory::Private::createOperationsWithDatumPivot(
    std::vector<CoordinateOperationNNPtr> &res, const crs::CRSNNPtr &sourceCRS,
    const crs::CRSNNPtr &targetCRS, const crs::GeodeticCRS *geodSrc,
    const crs::GeodeticCRS *geodDst, Private::Context &context) {

#ifdef TRACE_CREATE_OPERATIONS
    ENTER_BLOCK("createOperationsWithDatumPivot(" +
                objectAsStr(sourceCRS.get()) + "," +
                objectAsStr(targetCRS.get()) + ")");
#endif

    struct CreateOperationsWithDatumPivotAntiRecursion {
        Context &context;

        explicit CreateOperationsWithDatumPivotAntiRecursion(Context &contextIn)
            : context(contextIn) {
            assert(!context.inCreateOperationsWithDatumPivotAntiRecursion);
            context.inCreateOperationsWithDatumPivotAntiRecursion = true;
        }

        ~CreateOperationsWithDatumPivotAntiRecursion() {
            context.inCreateOperationsWithDatumPivotAntiRecursion = false;
        }
    };
    CreateOperationsWithDatumPivotAntiRecursion guard(context);

    const auto &authFactory = context.context->getAuthorityFactory();
    const auto &dbContext = authFactory->databaseContext();

    const auto candidatesSrcGeod(findCandidateGeodCRSForDatum(
        authFactory, geodSrc,
        geodSrc->datumNonNull(dbContext.as_nullable()).get()));
    const auto candidatesDstGeod(findCandidateGeodCRSForDatum(
        authFactory, geodDst,
        geodDst->datumNonNull(dbContext.as_nullable()).get()));

    const bool sourceAndTargetAre3D =
        geodSrc->coordinateSystem()->axisList().size() == 3 &&
        geodDst->coordinateSystem()->axisList().size() == 3;

    auto createTransformations = [&](const crs::CRSNNPtr &candidateSrcGeod,
                                     const crs::CRSNNPtr &candidateDstGeod,
                                     const CoordinateOperationNNPtr &opFirst,
                                     bool isNullFirst) {
        const auto opsSecond =
            createOperations(candidateSrcGeod, candidateDstGeod, context);
        const auto opsThird = createOperations(
            sourceAndTargetAre3D
                ? candidateDstGeod->promoteTo3D(std::string(), dbContext)
                : candidateDstGeod,
            targetCRS, context);
        assert(!opsThird.empty());
        const CoordinateOperationNNPtr &opThird(opsThird[0]);

        for (auto &opSecond : opsSecond) {
            // Check that it is not a transformation synthetized by
            // ourselves
            if (!hasIdentifiers(opSecond)) {
                continue;
            }
            // And even if it is a referenced transformation, check that
            // it is not a trivial one
            auto so = dynamic_cast<const SingleOperation *>(opSecond.get());
            if (so && isAxisOrderReversal(so->method()->getEPSGCode())) {
                continue;
            }

            std::vector<CoordinateOperationNNPtr> subOps;
            const bool isNullThird = isNullTransformation(opThird->nameStr());
            CoordinateOperationNNPtr opSecondCloned(
                (isNullFirst || isNullThird || sourceAndTargetAre3D)
                    ? opSecond->shallowClone()
                    : opSecond);
            if (isNullFirst || isNullThird) {
                if (opSecondCloned->identifiers().size() == 1 &&
                    (*opSecondCloned->identifiers()[0]->codeSpace())
                            .find("DERIVED_FROM") == std::string::npos) {
                    {
                        util::PropertyMap map;
                        addModifiedIdentifier(map, opSecondCloned.get(), false,
                                              true);
                        opSecondCloned->setProperties(map);
                    }
                    auto invCO = dynamic_cast<InverseCoordinateOperation *>(
                        opSecondCloned.get());
                    if (invCO) {
                        auto invCOForward = invCO->forwardOperation().get();
                        if (invCOForward->identifiers().size() == 1 &&
                            (*invCOForward->identifiers()[0]->codeSpace())
                                    .find("DERIVED_FROM") ==
                                std::string::npos) {
                            util::PropertyMap map;
                            addModifiedIdentifier(map, invCOForward, false,
                                                  true);
                            invCOForward->setProperties(map);
                        }
                    }
                }
            }
            if (sourceAndTargetAre3D) {

                // Force Helmert operations to use the 3D domain, even if the
                // ones we found in EPSG are advertized for the 2D domain.
                auto concat =
                    dynamic_cast<ConcatenatedOperation *>(opSecondCloned.get());
                if (concat) {
                    std::vector<CoordinateOperationNNPtr> newSteps;
                    for (const auto &step : concat->operations()) {
                        auto newStep = step->shallowClone();
                        setCRSs(newStep.get(),
                                newStep->sourceCRS()->promoteTo3D(std::string(),
                                                                  dbContext),
                                newStep->targetCRS()->promoteTo3D(std::string(),
                                                                  dbContext));
                        newSteps.emplace_back(newStep);
                    }
                    opSecondCloned =
                        ConcatenatedOperation::createComputeMetadata(
                            newSteps, disallowEmptyIntersection);
                } else {
                    setCRSs(opSecondCloned.get(),
                            opSecondCloned->sourceCRS()->promoteTo3D(
                                std::string(), dbContext),
                            opSecondCloned->targetCRS()->promoteTo3D(
                                std::string(), dbContext));
                }
            }
            if (isNullFirst) {
                auto oldTarget(NN_CHECK_ASSERT(opSecondCloned->targetCRS()));
                setCRSs(opSecondCloned.get(), sourceCRS, oldTarget);
            } else {
                subOps.emplace_back(opFirst);
            }
            if (isNullThird) {
                auto oldSource(NN_CHECK_ASSERT(opSecondCloned->sourceCRS()));
                setCRSs(opSecondCloned.get(), oldSource, targetCRS);
                subOps.emplace_back(opSecondCloned);
            } else {
                subOps.emplace_back(opSecondCloned);
                subOps.emplace_back(opThird);
            }
#ifdef TRACE_CREATE_OPERATIONS
            std::string debugStr;
            for (const auto &op : subOps) {
                if (!debugStr.empty()) {
                    debugStr += " + ";
                }
                debugStr += objectAsStr(op.get());
                debugStr += " (";
                debugStr += objectAsStr(op->sourceCRS().get());
                debugStr += "->";
                debugStr += objectAsStr(op->targetCRS().get());
                debugStr += ")";
            }
            logTrace("transformation " + debugStr);
#endif
            try {
                res.emplace_back(ConcatenatedOperation::createComputeMetadata(
                    subOps, disallowEmptyIntersection));
            } catch (const InvalidOperationEmptyIntersection &) {
            }
        }
    };

    // Start in priority with candidates that have exactly the same name as
    // the sourcCRS and targetCRS. Typically for the case of init=IGNF:XXXX

    // Transformation from IGNF:NTFP to IGNF:RGF93G,
    // using
    // NTF geographiques Paris (gr) vers NTF GEOGRAPHIQUES GREENWICH (DMS) +
    // NOUVELLE TRIANGULATION DE LA FRANCE (NTF) vers RGF93 (ETRS89)
    // that is using ntf_r93.gsb, is horribly dependent
    // of IGNF:RGF93G being returned before IGNF:RGF93GEO in candidatesDstGeod.
    // If RGF93GEO is returned before then we go through WGS84 and use
    // instead a Helmert transformation.
    // The below logic is thus quite fragile, and attempts at changing it
    // result in degraded results for other use cases...

    for (const auto &candidateSrcGeod : candidatesSrcGeod) {
        if (candidateSrcGeod->nameStr() == sourceCRS->nameStr()) {
            auto sourceSrcGeodModified(
                sourceAndTargetAre3D
                    ? candidateSrcGeod->promoteTo3D(std::string(), dbContext)
                    : candidateSrcGeod);
            for (const auto &candidateDstGeod : candidatesDstGeod) {
                if (candidateDstGeod->nameStr() == targetCRS->nameStr()) {
#ifdef TRACE_CREATE_OPERATIONS
                    ENTER_BLOCK("try " + objectAsStr(sourceCRS.get()) + "->" +
                                objectAsStr(candidateSrcGeod.get()) + "->" +
                                objectAsStr(candidateDstGeod.get()) + "->" +
                                objectAsStr(targetCRS.get()) + ")");
#endif
                    const auto opsFirst = createOperations(
                        sourceCRS, sourceSrcGeodModified, context);
                    assert(!opsFirst.empty());
                    const bool isNullFirst =
                        isNullTransformation(opsFirst[0]->nameStr());
                    createTransformations(candidateSrcGeod, candidateDstGeod,
                                          opsFirst[0], isNullFirst);
                    if (!res.empty()) {
                        if (hasResultSetOnlyResultsWithPROJStep(res)) {
                            continue;
                        }
                        return;
                    }
                }
            }
        }
    }

    for (const auto &candidateSrcGeod : candidatesSrcGeod) {
        const bool bSameSrcName =
            candidateSrcGeod->nameStr() == sourceCRS->nameStr();
#ifdef TRACE_CREATE_OPERATIONS
        ENTER_BLOCK("");
#endif
        auto sourceSrcGeodModified(
            sourceAndTargetAre3D
                ? candidateSrcGeod->promoteTo3D(std::string(), dbContext)
                : candidateSrcGeod);
        const auto opsFirst =
            createOperations(sourceCRS, sourceSrcGeodModified, context);
        assert(!opsFirst.empty());
        const bool isNullFirst = isNullTransformation(opsFirst[0]->nameStr());

        for (const auto &candidateDstGeod : candidatesDstGeod) {
            if (bSameSrcName &&
                candidateDstGeod->nameStr() == targetCRS->nameStr()) {
                continue;
            }

#ifdef TRACE_CREATE_OPERATIONS
            ENTER_BLOCK("try " + objectAsStr(sourceCRS.get()) + "->" +
                        objectAsStr(candidateSrcGeod.get()) + "->" +
                        objectAsStr(candidateDstGeod.get()) + "->" +
                        objectAsStr(targetCRS.get()) + ")");
#endif
            createTransformations(candidateSrcGeod, candidateDstGeod,
                                  opsFirst[0], isNullFirst);
            if (!res.empty() && !hasResultSetOnlyResultsWithPROJStep(res)) {
                return;
            }
        }
    }
}

// ---------------------------------------------------------------------------

static CoordinateOperationNNPtr
createBallparkGeocentricTranslation(const crs::CRSNNPtr &sourceCRS,
                                    const crs::CRSNNPtr &targetCRS) {
    std::string name(BALLPARK_GEOCENTRIC_TRANSLATION);
    name += " from ";
    name += sourceCRS->nameStr();
    name += " to ";
    name += targetCRS->nameStr();

    return util::nn_static_pointer_cast<CoordinateOperation>(
        Transformation::createGeocentricTranslations(
            util::PropertyMap()
                .set(common::IdentifiedObject::NAME_KEY, name)
                .set(common::ObjectUsage::DOMAIN_OF_VALIDITY_KEY,
                     metadata::Extent::WORLD),
            sourceCRS, targetCRS, 0.0, 0.0, 0.0, {}));
}

// ---------------------------------------------------------------------------

bool CoordinateOperationFactory::Private::hasPerfectAccuracyResult(
    const std::vector<CoordinateOperationNNPtr> &res, const Context &context) {
    auto resTmp = FilterResults(res, context.context, context.extent1,
                                context.extent2, true)
                      .getRes();
    for (const auto &op : resTmp) {
        const double acc = getAccuracy(op);
        if (acc == 0.0) {
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------

std::vector<CoordinateOperationNNPtr>
CoordinateOperationFactory::Private::createOperations(
    const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
    Private::Context &context) {

#ifdef TRACE_CREATE_OPERATIONS
    ENTER_BLOCK("createOperations(" + objectAsStr(sourceCRS.get()) + " --> " +
                objectAsStr(targetCRS.get()) + ")");
#endif

    std::vector<CoordinateOperationNNPtr> res;

    auto boundSrc = dynamic_cast<const crs::BoundCRS *>(sourceCRS.get());
    auto boundDst = dynamic_cast<const crs::BoundCRS *>(targetCRS.get());

    const auto &sourceProj4Ext = boundSrc
                                     ? boundSrc->baseCRS()->getExtensionProj4()
                                     : sourceCRS->getExtensionProj4();
    const auto &targetProj4Ext = boundDst
                                     ? boundDst->baseCRS()->getExtensionProj4()
                                     : targetCRS->getExtensionProj4();
    if (!sourceProj4Ext.empty() || !targetProj4Ext.empty()) {
        createOperationsFromProj4Ext(sourceCRS, targetCRS, boundSrc, boundDst,
                                     res);
        return res;
    }

    auto geodSrc = dynamic_cast<const crs::GeodeticCRS *>(sourceCRS.get());
    auto geodDst = dynamic_cast<const crs::GeodeticCRS *>(targetCRS.get());
    auto geogSrc = dynamic_cast<const crs::GeographicCRS *>(sourceCRS.get());
    auto geogDst = dynamic_cast<const crs::GeographicCRS *>(targetCRS.get());
    auto vertSrc = dynamic_cast<const crs::VerticalCRS *>(sourceCRS.get());
    auto vertDst = dynamic_cast<const crs::VerticalCRS *>(targetCRS.get());

    // First look-up if the registry provide us with operations.
    auto derivedSrc = dynamic_cast<const crs::DerivedCRS *>(sourceCRS.get());
    auto derivedDst = dynamic_cast<const crs::DerivedCRS *>(targetCRS.get());
    const auto &authFactory = context.context->getAuthorityFactory();
    if (authFactory &&
        (derivedSrc == nullptr ||
         !derivedSrc->baseCRS()->_isEquivalentTo(
             targetCRS.get(), util::IComparable::Criterion::EQUIVALENT)) &&
        (derivedDst == nullptr ||
         !derivedDst->baseCRS()->_isEquivalentTo(
             sourceCRS.get(), util::IComparable::Criterion::EQUIVALENT))) {

        if (createOperationsFromDatabase(sourceCRS, targetCRS, context, geodSrc,
                                         geodDst, geogSrc, geogDst, vertSrc,
                                         vertDst, res)) {
            return res;
        }
    }

    // Special case if both CRS are geodetic
    if (geodSrc && geodDst && !derivedSrc && !derivedDst) {
        createOperationsGeodToGeod(sourceCRS, targetCRS, context, geodSrc,
                                   geodDst, res);
        return res;
    }

    // If the source is a derived CRS, then chain the inverse of its
    // deriving conversion, with transforms from its baseCRS to the
    // targetCRS
    if (derivedSrc) {
        createOperationsDerivedTo(sourceCRS, targetCRS, context, derivedSrc,
                                  res);
        return res;
    }

    // reverse of previous case
    if (derivedDst) {
        return applyInverse(createOperations(targetCRS, sourceCRS, context));
    }

    // Order of comparison between the geogDst vs geodDst is impotant
    if (boundSrc && geogDst) {
        createOperationsBoundToGeog(sourceCRS, targetCRS, context, boundSrc,
                                    geogDst, res);
        return res;
    } else if (boundSrc && geodDst) {
        createOperationsToGeod(sourceCRS, targetCRS, context, geodDst, res);
        return res;
    }

    // reverse of previous case
    if (geodSrc && boundDst) {
        return applyInverse(createOperations(targetCRS, sourceCRS, context));
    }

    // vertCRS (as boundCRS with transformation to target vertCRS) to
    // vertCRS
    if (boundSrc && vertDst) {
        createOperationsBoundToVert(sourceCRS, targetCRS, context, boundSrc,
                                    vertDst, res);
        return res;
    }

    // reverse of previous case
    if (boundDst && vertSrc) {
        return applyInverse(createOperations(targetCRS, sourceCRS, context));
    }

    if (vertSrc && vertDst) {
        createOperationsVertToVert(sourceCRS, targetCRS, context, vertSrc,
                                   vertDst, res);
        return res;
    }

    // A bit odd case as we are comparing apples to oranges, but in case
    // the vertical unit differ, do something useful.
    if (vertSrc && geogDst) {
        createOperationsVertToGeog(sourceCRS, targetCRS, context, vertSrc,
                                   geogDst, res);
        return res;
    }

    // reverse of previous case
    if (vertDst && geogSrc) {
        return applyInverse(createOperations(targetCRS, sourceCRS, context));
    }

    // boundCRS to boundCRS
    if (boundSrc && boundDst) {
        createOperationsBoundToBound(sourceCRS, targetCRS, context, boundSrc,
                                     boundDst, res);
        return res;
    }

    auto compoundSrc = dynamic_cast<crs::CompoundCRS *>(sourceCRS.get());
    // Order of comparison between the geogDst vs geodDst is impotant
    if (compoundSrc && geogDst) {
        createOperationsCompoundToGeog(sourceCRS, targetCRS, context,
                                       compoundSrc, geogDst, res);
        return res;
    } else if (compoundSrc && geodDst) {
        createOperationsToGeod(sourceCRS, targetCRS, context, geodDst, res);
        return res;
    }

    // reverse of previous cases
    auto compoundDst = dynamic_cast<const crs::CompoundCRS *>(targetCRS.get());
    if (geodSrc && compoundDst) {
        return applyInverse(createOperations(targetCRS, sourceCRS, context));
    }

    if (compoundSrc && compoundDst) {
        createOperationsCompoundToCompound(sourceCRS, targetCRS, context,
                                           compoundSrc, compoundDst, res);
        return res;
    }

    // '+proj=longlat +ellps=GRS67 +nadgrids=@foo.gsb +type=crs' to
    // '+proj=longlat +ellps=GRS80 +nadgrids=@bar.gsb +geoidgrids=@bar.gtx
    // +type=crs'
    if (boundSrc && compoundDst) {
        createOperationsBoundToCompound(sourceCRS, targetCRS, context, boundSrc,
                                        compoundDst, res);
        return res;
    }

    // reverse of previous case
    if (boundDst && compoundSrc) {
        return applyInverse(createOperations(targetCRS, sourceCRS, context));
    }

    return res;
}

// ---------------------------------------------------------------------------

void CoordinateOperationFactory::Private::createOperationsFromProj4Ext(
    const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
    const crs::BoundCRS *boundSrc, const crs::BoundCRS *boundDst,
    std::vector<CoordinateOperationNNPtr> &res) {

    ENTER_FUNCTION();

    auto sourceProjExportable = dynamic_cast<const io::IPROJStringExportable *>(
        boundSrc ? boundSrc : sourceCRS.get());
    auto targetProjExportable = dynamic_cast<const io::IPROJStringExportable *>(
        boundDst ? boundDst : targetCRS.get());
    if (!sourceProjExportable) {
        throw InvalidOperation("Source CRS is not PROJ exportable");
    }
    if (!targetProjExportable) {
        throw InvalidOperation("Target CRS is not PROJ exportable");
    }
    auto projFormatter = io::PROJStringFormatter::create();
    projFormatter->setCRSExport(true);
    projFormatter->setLegacyCRSToCRSContext(true);
    projFormatter->startInversion();
    sourceProjExportable->_exportToPROJString(projFormatter.get());
    auto geogSrc = dynamic_cast<const crs::GeographicCRS *>(sourceCRS.get());
    if (geogSrc) {
        auto tmpFormatter = io::PROJStringFormatter::create();
        geogSrc->addAngularUnitConvertAndAxisSwap(tmpFormatter.get());
        projFormatter->ingestPROJString(tmpFormatter->toString());
    }

    projFormatter->stopInversion();

    targetProjExportable->_exportToPROJString(projFormatter.get());
    auto geogDst = dynamic_cast<const crs::GeographicCRS *>(targetCRS.get());
    if (geogDst) {
        auto tmpFormatter = io::PROJStringFormatter::create();
        geogDst->addAngularUnitConvertAndAxisSwap(tmpFormatter.get());
        projFormatter->ingestPROJString(tmpFormatter->toString());
    }

    const auto PROJString = projFormatter->toString();
    auto properties = util::PropertyMap().set(
        common::IdentifiedObject::NAME_KEY,
        buildTransfName(sourceCRS->nameStr(), targetCRS->nameStr()));
    res.emplace_back(SingleOperation::createPROJBased(
        properties, PROJString, sourceCRS, targetCRS, {}));
}

// ---------------------------------------------------------------------------

bool CoordinateOperationFactory::Private::createOperationsFromDatabase(
    const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
    Private::Context &context, const crs::GeodeticCRS *geodSrc,
    const crs::GeodeticCRS *geodDst, const crs::GeographicCRS *geogSrc,
    const crs::GeographicCRS *geogDst, const crs::VerticalCRS *vertSrc,
    const crs::VerticalCRS *vertDst,
    std::vector<CoordinateOperationNNPtr> &res) {

    ENTER_FUNCTION();

    if (geogSrc && vertDst) {
        createOperationsFromDatabase(targetCRS, sourceCRS, context, geodDst,
                                     geodSrc, geogDst, geogSrc, vertDst,
                                     vertSrc, res);
        res = applyInverse(res);
    } else if (geogDst && vertSrc) {
        res = applyInverse(createOperationsGeogToVertFromGeoid(
            targetCRS, sourceCRS, vertSrc, context));
        if (!res.empty()) {
            createOperationsVertToGeogBallpark(sourceCRS, targetCRS, context,
                                               vertSrc, geogDst, res);
        }
    }

    if (!res.empty()) {
        return true;
    }

    bool resFindDirectNonEmptyBeforeFiltering = false;
    res = findOpsInRegistryDirect(sourceCRS, targetCRS, context,
                                  resFindDirectNonEmptyBeforeFiltering);

    // If we get at least a result with perfect accuracy, do not
    // bother generating synthetic transforms.
    if (hasPerfectAccuracyResult(res, context)) {
        return true;
    }

    bool doFilterAndCheckPerfectOp = false;

    bool sameGeodeticDatum = false;

    if (vertSrc || vertDst) {
        if (res.empty()) {
            if (geogSrc &&
                geogSrc->coordinateSystem()->axisList().size() == 2 &&
                vertDst) {
                auto dbContext =
                    context.context->getAuthorityFactory()->databaseContext();
                auto resTmp = findOpsInRegistryDirect(
                    sourceCRS->promoteTo3D(std::string(), dbContext), targetCRS,
                    context, resFindDirectNonEmptyBeforeFiltering);
                for (auto &op : resTmp) {
                    auto newOp = op->shallowClone();
                    setCRSs(newOp.get(), sourceCRS, targetCRS);
                    res.emplace_back(newOp);
                }
            } else if (geogDst &&
                       geogDst->coordinateSystem()->axisList().size() == 2 &&
                       vertSrc) {
                auto dbContext =
                    context.context->getAuthorityFactory()->databaseContext();
                auto resTmp = findOpsInRegistryDirect(
                    sourceCRS, targetCRS->promoteTo3D(std::string(), dbContext),
                    context, resFindDirectNonEmptyBeforeFiltering);
                for (auto &op : resTmp) {
                    auto newOp = op->shallowClone();
                    setCRSs(newOp.get(), sourceCRS, targetCRS);
                    res.emplace_back(newOp);
                }
            }
        }
        if (res.empty()) {
            createOperationsFromDatabaseWithVertCRS(sourceCRS, targetCRS,
                                                    context, geogSrc, geogDst,
                                                    vertSrc, vertDst, res);
        }
    } else if (geodSrc && geodDst) {

        const auto &authFactory = context.context->getAuthorityFactory();
        const auto dbContext = authFactory->databaseContext().as_nullable();

        const auto srcDatum = geodSrc->datumNonNull(dbContext);
        const auto dstDatum = geodDst->datumNonNull(dbContext);
        sameGeodeticDatum = srcDatum->_isEquivalentTo(
            dstDatum.get(), util::IComparable::Criterion::EQUIVALENT);

        if (res.empty() && !sameGeodeticDatum &&
            !context.inCreateOperationsWithDatumPivotAntiRecursion) {
            // If we still didn't find a transformation, and that the source
            // and target are GeodeticCRS, then go through their underlying
            // datum to find potential transformations between other
            // GeodeticCRSs
            // that are made of those datum
            // The typical example is if transforming between two
            // GeographicCRS,
            // but transformations are only available between their
            // corresponding geocentric CRS.
            createOperationsWithDatumPivot(res, sourceCRS, targetCRS, geodSrc,
                                           geodDst, context);
            doFilterAndCheckPerfectOp = !res.empty();
        }
    }

    bool foundInstantiableOp = false;
    // FIXME: the limitation to .size() == 1 is just for the
    // -s EPSG:4959+5759 -t "EPSG:4959+7839" case
    // finding EPSG:7860 'NZVD2016 height to Auckland 1946
    // height (1)', which uses the EPSG:1071 'Vertical Offset by Grid
    // Interpolation (NZLVD)' method which is not currently implemented by PROJ
    // (cannot deal with .csv files)
    // Initially the test was written to iterate over for all operations of a
    // non-empty res, but this causes failures in the test suite when no grids
    // are installed at all. Ideally we should tweak the test suite to be
    // robust to that, or skip some tests.
    if (res.size() == 1) {
        try {
            res.front()->exportToPROJString(
                io::PROJStringFormatter::create().get());
            foundInstantiableOp = true;
        } catch (const std::exception &) {
        }
        if (!foundInstantiableOp) {
            resFindDirectNonEmptyBeforeFiltering = false;
        }
    } else if (res.size() > 1) {
        foundInstantiableOp = true;
    }

    // NAD27 to NAD83 has tens of results already. No need to look
    // for a pivot
    if (!sameGeodeticDatum &&
        (((res.empty() || !foundInstantiableOp) &&
          !resFindDirectNonEmptyBeforeFiltering &&
          context.context->getAllowUseIntermediateCRS() ==
              CoordinateOperationContext::IntermediateCRSUse::
                  IF_NO_DIRECT_TRANSFORMATION) ||
         context.context->getAllowUseIntermediateCRS() ==
             CoordinateOperationContext::IntermediateCRSUse::ALWAYS ||
         getenv("PROJ_FORCE_SEARCH_PIVOT"))) {
        auto resWithIntermediate = findsOpsInRegistryWithIntermediate(
            sourceCRS, targetCRS, context, false);
        res.insert(res.end(), resWithIntermediate.begin(),
                   resWithIntermediate.end());
        doFilterAndCheckPerfectOp = !res.empty();
    }

    if (res.empty() && !context.inCreateOperationsWithDatumPivotAntiRecursion &&
        !resFindDirectNonEmptyBeforeFiltering && geodSrc && geodDst &&
        !sameGeodeticDatum && context.context->getIntermediateCRS().empty() &&
        context.context->getAllowUseIntermediateCRS() !=
            CoordinateOperationContext::IntermediateCRSUse::NEVER) {
        // Currently triggered by "IG05/12 Intermediate CRS" to ITRF2014
        auto resWithIntermediate = findsOpsInRegistryWithIntermediate(
            sourceCRS, targetCRS, context, true);
        res.insert(res.end(), resWithIntermediate.begin(),
                   resWithIntermediate.end());
        doFilterAndCheckPerfectOp = !res.empty();
    }

    if (doFilterAndCheckPerfectOp) {
        // If we get at least a result with perfect accuracy, do not bother
        // generating synthetic transforms.
        if (hasPerfectAccuracyResult(res, context)) {
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------

static std::vector<crs::CRSNNPtr>
findCandidateVertCRSForDatum(const io::AuthorityFactoryPtr &authFactory,
                             const datum::VerticalReferenceFrame *datum) {
    std::vector<crs::CRSNNPtr> candidates;
    assert(datum);
    const auto &ids = datum->identifiers();
    const auto &datumName = datum->nameStr();
    if (!ids.empty()) {
        for (const auto &id : ids) {
            const auto &authName = *(id->codeSpace());
            const auto &code = id->code();
            if (!authName.empty()) {
                auto l_candidates =
                    authFactory->createVerticalCRSFromDatum(authName, code);
                for (const auto &candidate : l_candidates) {
                    candidates.emplace_back(candidate);
                }
            }
        }
    } else if (datumName != "unknown" && datumName != "unnamed") {
        auto matches = authFactory->createObjectsFromName(
            datumName,
            {io::AuthorityFactory::ObjectType::VERTICAL_REFERENCE_FRAME}, false,
            2);
        if (matches.size() == 1) {
            const auto &match = matches.front();
            if (datum->_isEquivalentTo(
                    match.get(), util::IComparable::Criterion::EQUIVALENT) &&
                !match->identifiers().empty()) {
                return findCandidateVertCRSForDatum(
                    authFactory,
                    dynamic_cast<const datum::VerticalReferenceFrame *>(
                        match.get()));
            }
        }
    }
    return candidates;
}

// ---------------------------------------------------------------------------

std::vector<CoordinateOperationNNPtr>
CoordinateOperationFactory::Private::createOperationsGeogToVertFromGeoid(
    const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
    const crs::VerticalCRS *vertDst, Private::Context &context) {

    ENTER_FUNCTION();

    const auto useTransf = [&sourceCRS, &targetCRS, &context,
                            vertDst](const CoordinateOperationNNPtr &op) {
        // If the source geographic CRS has a non-metre vertical unit, we need
        // to create an intermediate and operation to do the vertical unit
        // conversion from that vertical unit to the one of the geographic CRS
        // of the source of the operation
        const auto geogCRS =
            dynamic_cast<const crs::GeographicCRS *>(sourceCRS.get());
        assert(geogCRS);
        const auto &srcAxisList = geogCRS->coordinateSystem()->axisList();
        CoordinateOperationPtr opPtr;
        const auto opSourceCRSGeog =
            dynamic_cast<const crs::GeographicCRS *>(op->sourceCRS().get());
        // I assume opSourceCRSGeog should always be null in practice...
        if (opSourceCRSGeog && srcAxisList.size() == 3 &&
            srcAxisList[2]->unit().conversionToSI() != 1) {
            const auto &authFactory = context.context->getAuthorityFactory();
            const auto dbContext =
                authFactory ? authFactory->databaseContext().as_nullable()
                            : nullptr;
            auto tmpCRSWithSrcZ =
                opSourceCRSGeog->demoteTo2D(std::string(), dbContext)
                    ->promoteTo3D(std::string(), dbContext, srcAxisList[2]);

            std::vector<CoordinateOperationNNPtr> opsUnitConvert;
            createOperationsGeogToGeog(
                opsUnitConvert, tmpCRSWithSrcZ, NN_NO_CHECK(op->sourceCRS()),
                context,
                dynamic_cast<const crs::GeographicCRS *>(tmpCRSWithSrcZ.get()),
                opSourceCRSGeog);
            assert(opsUnitConvert.size() == 1);
            opPtr = opsUnitConvert.front().as_nullable();
        }

        std::vector<CoordinateOperationNNPtr> ops;
        if (opPtr)
            ops.emplace_back(NN_NO_CHECK(opPtr));
        ops.emplace_back(op);

        const auto targetOp =
            dynamic_cast<const crs::VerticalCRS *>(op->targetCRS().get());
        assert(targetOp);
        if (targetOp->_isEquivalentTo(
                vertDst, util::IComparable::Criterion::EQUIVALENT)) {
            auto ret = ConcatenatedOperation::createComputeMetadata(
                ops, disallowEmptyIntersection);
            return ret;
        }
        std::vector<CoordinateOperationNNPtr> tmp;
        createOperationsVertToVert(NN_NO_CHECK(op->targetCRS()), targetCRS,
                                   context, targetOp, vertDst, tmp);
        assert(!tmp.empty());
        ops.emplace_back(tmp.front());
        auto ret = ConcatenatedOperation::createComputeMetadata(
            ops, disallowEmptyIntersection);
        return ret;
    };

    const auto getProjGeoidTransformation =
        [&sourceCRS, &targetCRS, &vertDst,
         &context](const CoordinateOperationNNPtr &model,
                   const std::string &projFilename) {
            const auto getNameVertCRSMetre = [](const std::string &name) {
                if (name.empty())
                    return std::string("unnamed");
                auto ret(name);
                bool haveOriginalUnit = false;
                if (name.back() == ')') {
                    const auto pos = ret.rfind(" (");
                    if (pos != std::string::npos) {
                        haveOriginalUnit = true;
                        ret = ret.substr(0, pos);
                    }
                }
                const auto pos = ret.rfind(" depth");
                if (pos != std::string::npos) {
                    ret = ret.substr(0, pos) + " height";
                }
                if (!haveOriginalUnit) {
                    ret += " (metre)";
                }
                return ret;
            };

            const auto &axis = vertDst->coordinateSystem()->axisList()[0];
            const auto &authFactory = context.context->getAuthorityFactory();
            const auto dbContext =
                authFactory ? authFactory->databaseContext().as_nullable()
                            : nullptr;

            const auto geogSrcCRS =
                dynamic_cast<crs::GeographicCRS *>(
                    model->interpolationCRS().get())
                    ? NN_NO_CHECK(model->interpolationCRS())
                    : sourceCRS->demoteTo2D(std::string(), dbContext)
                          ->promoteTo3D(std::string(), dbContext);
            const auto vertCRSMetre =
                axis->unit() == common::UnitOfMeasure::METRE &&
                        axis->direction() == cs::AxisDirection::UP
                    ? targetCRS
                    : util::nn_static_pointer_cast<crs::CRS>(
                          crs::VerticalCRS::create(
                              util::PropertyMap().set(
                                  common::IdentifiedObject::NAME_KEY,
                                  getNameVertCRSMetre(targetCRS->nameStr())),
                              vertDst->datum(), vertDst->datumEnsemble(),
                              cs::VerticalCS::createGravityRelatedHeight(
                                  common::UnitOfMeasure::METRE)));
            const auto properties = util::PropertyMap().set(
                common::IdentifiedObject::NAME_KEY,
                buildOpName("Transformation", vertCRSMetre, geogSrcCRS));

            // Try to find a representative value for the accuracy of this grid
            // from the registered transformations.
            std::vector<metadata::PositionalAccuracyNNPtr> accuracies;
            const auto &modelAccuracies =
                model->coordinateOperationAccuracies();
            if (modelAccuracies.empty()) {
                if (authFactory) {
                    const auto transformationsForGrid =
                        io::DatabaseContext::getTransformationsForGridName(
                            authFactory->databaseContext(), projFilename);
                    double accuracy = -1;
                    for (const auto &transf : transformationsForGrid) {
                        accuracy = std::max(accuracy, getAccuracy(transf));
                    }
                    if (accuracy >= 0) {
                        accuracies.emplace_back(
                            metadata::PositionalAccuracy::create(
                                toString(accuracy)));
                    }
                }
            }

            return Transformation::createGravityRelatedHeightToGeographic3D(
                properties, vertCRSMetre, geogSrcCRS, nullptr, projFilename,
                !modelAccuracies.empty() ? modelAccuracies : accuracies);
        };

    std::vector<CoordinateOperationNNPtr> res;
    const auto &authFactory = context.context->getAuthorityFactory();
    if (authFactory) {
        const auto &models = vertDst->geoidModel();
        for (const auto &model : models) {
            const auto &modelName = model->nameStr();
            const auto transformations =
                starts_with(modelName, "PROJ ")
                    ? std::vector<
                          CoordinateOperationNNPtr>{getProjGeoidTransformation(
                          model, modelName.substr(strlen("PROJ ")))}
                    : authFactory->getTransformationsForGeoid(
                          modelName,
                          context.context->getUsePROJAlternativeGridNames());
            for (const auto &transf : transformations) {
                if (dynamic_cast<crs::GeographicCRS *>(
                        transf->sourceCRS().get()) &&
                    dynamic_cast<crs::VerticalCRS *>(
                        transf->targetCRS().get())) {
                    res.push_back(useTransf(transf));
                } else if (dynamic_cast<crs::GeographicCRS *>(
                               transf->targetCRS().get()) &&
                           dynamic_cast<crs::VerticalCRS *>(
                               transf->sourceCRS().get())) {
                    res.push_back(useTransf(transf->inverse()));
                }
            }
        }
    }

    return res;
}

// ---------------------------------------------------------------------------

std::vector<CoordinateOperationNNPtr> CoordinateOperationFactory::Private::
    createOperationsGeogToVertWithIntermediateVert(
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        const crs::VerticalCRS *vertDst, Private::Context &context) {

    ENTER_FUNCTION();

    std::vector<CoordinateOperationNNPtr> res;

    struct AntiRecursionGuard {
        Context &context;

        explicit AntiRecursionGuard(Context &contextIn) : context(contextIn) {
            assert(!context.inCreateOperationsGeogToVertWithIntermediateVert);
            context.inCreateOperationsGeogToVertWithIntermediateVert = true;
        }

        ~AntiRecursionGuard() {
            context.inCreateOperationsGeogToVertWithIntermediateVert = false;
        }
    };
    AntiRecursionGuard guard(context);
    const auto &authFactory = context.context->getAuthorityFactory();
    const auto dbContext = authFactory->databaseContext().as_nullable();

    auto candidatesVert = findCandidateVertCRSForDatum(
        authFactory, vertDst->datumNonNull(dbContext).get());
    for (const auto &candidateVert : candidatesVert) {
        auto resTmp = createOperations(sourceCRS, candidateVert, context);
        if (!resTmp.empty()) {
            const auto opsSecond =
                createOperations(candidateVert, targetCRS, context);
            if (!opsSecond.empty()) {
                // The transformation from candidateVert to targetCRS should
                // be just a unit change typically, so take only the first one,
                // which is likely/hopefully the only one.
                for (const auto &opFirst : resTmp) {
                    if (hasIdentifiers(opFirst)) {
                        if (candidateVert->_isEquivalentTo(
                                targetCRS.get(),
                                util::IComparable::Criterion::EQUIVALENT)) {
                            res.emplace_back(opFirst);
                        } else {
                            res.emplace_back(
                                ConcatenatedOperation::createComputeMetadata(
                                    {opFirst, opsSecond.front()},
                                    disallowEmptyIntersection));
                        }
                    }
                }
                if (!res.empty())
                    break;
            }
        }
    }

    return res;
}

// ---------------------------------------------------------------------------

std::vector<CoordinateOperationNNPtr> CoordinateOperationFactory::Private::
    createOperationsGeogToVertWithAlternativeGeog(
        const crs::CRSNNPtr &sourceCRS, // geographic CRS
        const crs::CRSNNPtr &targetCRS, // vertical CRS
        Private::Context &context) {

    ENTER_FUNCTION();

    std::vector<CoordinateOperationNNPtr> res;

    struct AntiRecursionGuard {
        Context &context;

        explicit AntiRecursionGuard(Context &contextIn) : context(contextIn) {
            assert(!context.inCreateOperationsGeogToVertWithAlternativeGeog);
            context.inCreateOperationsGeogToVertWithAlternativeGeog = true;
        }

        ~AntiRecursionGuard() {
            context.inCreateOperationsGeogToVertWithAlternativeGeog = false;
        }
    };
    AntiRecursionGuard guard(context);

    // Generally EPSG has operations from GeogCrs to VertCRS
    auto ops = findOpsInRegistryDirectTo(targetCRS, context);

    const auto geogCRS =
        dynamic_cast<const crs::GeographicCRS *>(sourceCRS.get());
    assert(geogCRS);
    const auto &srcAxisList = geogCRS->coordinateSystem()->axisList();
    for (const auto &op : ops) {
        const auto tmpCRS =
            dynamic_cast<const crs::GeographicCRS *>(op->sourceCRS().get());
        if (tmpCRS) {
            if (srcAxisList.size() == 3 &&
                srcAxisList[2]->unit().conversionToSI() != 1) {

                const auto &authFactory =
                    context.context->getAuthorityFactory();
                const auto dbContext =
                    authFactory->databaseContext().as_nullable();
                auto tmpCRSWithSrcZ =
                    tmpCRS->demoteTo2D(std::string(), dbContext)
                        ->promoteTo3D(std::string(), dbContext, srcAxisList[2]);

                std::vector<CoordinateOperationNNPtr> opsUnitConvert;
                createOperationsGeogToGeog(
                    opsUnitConvert, tmpCRSWithSrcZ,
                    NN_NO_CHECK(op->sourceCRS()), context,
                    dynamic_cast<const crs::GeographicCRS *>(
                        tmpCRSWithSrcZ.get()),
                    tmpCRS);
                assert(opsUnitConvert.size() == 1);
                auto concat = ConcatenatedOperation::createComputeMetadata(
                    {opsUnitConvert.front(), op}, disallowEmptyIntersection);
                res.emplace_back(concat);
            } else {
                res.emplace_back(op);
            }
        }
    }

    return res;
}

// ---------------------------------------------------------------------------

void CoordinateOperationFactory::Private::
    createOperationsFromDatabaseWithVertCRS(
        const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
        Private::Context &context, const crs::GeographicCRS *geogSrc,
        const crs::GeographicCRS *geogDst, const crs::VerticalCRS *vertSrc,
        const crs::VerticalCRS *vertDst,
        std::vector<CoordinateOperationNNPtr> &res) {

    // Typically to transform from "NAVD88 height (ftUS)" to a geog CRS
    // by using transformations of "NAVD88 height" (metre) to that geog CRS
    if (res.empty() &&
        !context.inCreateOperationsGeogToVertWithIntermediateVert && geogSrc &&
        vertDst) {
        res = createOperationsGeogToVertWithIntermediateVert(
            sourceCRS, targetCRS, vertDst, context);
    } else if (res.empty() &&
               !context.inCreateOperationsGeogToVertWithIntermediateVert &&
               geogDst && vertSrc) {
        res = applyInverse(createOperationsGeogToVertWithIntermediateVert(
            targetCRS, sourceCRS, vertSrc, context));
    }

    // NAD83 only exists in 2D version in EPSG, so if it has been
    // promoted to 3D, when researching a vertical to geog
    // transformation, try to down cast to 2D.
    const auto geog3DToVertTryThroughGeog2D =
        [&res, &context](const crs::GeographicCRS *geogSrcIn,
                         const crs::VerticalCRS *vertDstIn,
                         const crs::CRSNNPtr &targetCRSIn) {
            if (res.empty() && geogSrcIn && vertDstIn &&
                geogSrcIn->coordinateSystem()->axisList().size() == 3) {
                const auto &authFactory =
                    context.context->getAuthorityFactory();
                const auto dbContext =
                    authFactory ? authFactory->databaseContext().as_nullable()
                                : nullptr;
                const auto candidatesSrcGeod(findCandidateGeodCRSForDatum(
                    authFactory, geogSrcIn,
                    geogSrcIn->datumNonNull(dbContext).get()));
                for (const auto &candidate : candidatesSrcGeod) {
                    auto geogCandidate =
                        util::nn_dynamic_pointer_cast<crs::GeographicCRS>(
                            candidate);
                    if (geogCandidate &&
                        geogCandidate->coordinateSystem()->axisList().size() ==
                            2) {
                        bool ignored;
                        res = findOpsInRegistryDirect(
                            NN_NO_CHECK(geogCandidate), targetCRSIn, context,
                            ignored);
                        break;
                    }
                }
                return true;
            }
            return false;
        };

    if (geog3DToVertTryThroughGeog2D(geogSrc, vertDst, targetCRS)) {
        // do nothing
    } else if (geog3DToVertTryThroughGeog2D(geogDst, vertSrc, sourceCRS)) {
        res = applyInverse(res);
    }

    // There's no direct transformation from NAVD88 height to WGS84,
    // so try to research all transformations from NAVD88 to another
    // intermediate GeographicCRS.
    if (res.empty() &&
        !context.inCreateOperationsGeogToVertWithAlternativeGeog && geogSrc &&
        vertDst) {
        res = createOperationsGeogToVertWithAlternativeGeog(sourceCRS,
                                                            targetCRS, context);
    } else if (res.empty() &&
               !context.inCreateOperationsGeogToVertWithAlternativeGeog &&
               geogDst && vertSrc) {
        res = applyInverse(createOperationsGeogToVertWithAlternativeGeog(
            targetCRS, sourceCRS, context));
    }
}

// ---------------------------------------------------------------------------

void CoordinateOperationFactory::Private::createOperationsGeodToGeod(
    const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
    Private::Context &context, const crs::GeodeticCRS *geodSrc,
    const crs::GeodeticCRS *geodDst,
    std::vector<CoordinateOperationNNPtr> &res) {

    ENTER_FUNCTION();

    if (geodSrc->ellipsoid()->celestialBody() !=
        geodDst->ellipsoid()->celestialBody()) {
        throw util::UnsupportedOperationException(
            "Source and target ellipsoid do not belong to the same "
            "celestial body");
    }

    auto geogSrc = dynamic_cast<const crs::GeographicCRS *>(geodSrc);
    auto geogDst = dynamic_cast<const crs::GeographicCRS *>(geodDst);

    if (geogSrc && geogDst) {
        createOperationsGeogToGeog(res, sourceCRS, targetCRS, context, geogSrc,
                                   geogDst);
        return;
    }

    const bool isSrcGeocentric = geodSrc->isGeocentric();
    const bool isSrcGeographic = geogSrc != nullptr;
    const bool isTargetGeocentric = geodDst->isGeocentric();
    const bool isTargetGeographic = geogDst != nullptr;

    const auto IsSameDatum = [&context, &geodSrc, &geodDst]() {
        const auto &authFactory = context.context->getAuthorityFactory();
        const auto dbContext =
            authFactory ? authFactory->databaseContext().as_nullable()
                        : nullptr;

        return geodSrc->datumNonNull(dbContext)->_isEquivalentTo(
            geodDst->datumNonNull(dbContext).get(),
            util::IComparable::Criterion::EQUIVALENT);
    };

    if (((isSrcGeocentric && isTargetGeographic) ||
         (isSrcGeographic && isTargetGeocentric))) {

        // Same datum ?
        if (IsSameDatum()) {
            res.emplace_back(
                Conversion::createGeographicGeocentric(sourceCRS, targetCRS));
        } else if (isSrcGeocentric && geogDst) {
            std::string interm_crs_name(geogDst->nameStr());
            interm_crs_name += " (geocentric)";
            auto interm_crs =
                util::nn_static_pointer_cast<crs::CRS>(crs::GeodeticCRS::create(
                    addDomains(util::PropertyMap().set(
                                   common::IdentifiedObject::NAME_KEY,
                                   interm_crs_name),
                               geogDst),
                    geogDst->datum(), geogDst->datumEnsemble(),
                    NN_CHECK_ASSERT(
                        util::nn_dynamic_pointer_cast<cs::CartesianCS>(
                            geodSrc->coordinateSystem()))));
            auto opFirst =
                createBallparkGeocentricTranslation(sourceCRS, interm_crs);
            auto opSecond =
                Conversion::createGeographicGeocentric(interm_crs, targetCRS);
            res.emplace_back(ConcatenatedOperation::createComputeMetadata(
                {opFirst, opSecond}, disallowEmptyIntersection));
        } else {
            // Apply previous case in reverse way
            std::vector<CoordinateOperationNNPtr> resTmp;
            createOperationsGeodToGeod(targetCRS, sourceCRS, context, geodDst,
                                       geodSrc, resTmp);
            assert(resTmp.size() == 1);
            res.emplace_back(resTmp.front()->inverse());
        }

        return;
    }

    if (isSrcGeocentric && isTargetGeocentric) {
        if (sourceCRS->_isEquivalentTo(
                targetCRS.get(), util::IComparable::Criterion::EQUIVALENT) ||
            IsSameDatum()) {
            std::string name(NULL_GEOCENTRIC_TRANSLATION);
            name += " from ";
            name += sourceCRS->nameStr();
            name += " to ";
            name += targetCRS->nameStr();
            res.emplace_back(Transformation::createGeocentricTranslations(
                util::PropertyMap()
                    .set(common::IdentifiedObject::NAME_KEY, name)
                    .set(common::ObjectUsage::DOMAIN_OF_VALIDITY_KEY,
                         metadata::Extent::WORLD),
                sourceCRS, targetCRS, 0.0, 0.0, 0.0,
                {metadata::PositionalAccuracy::create("0")}));
        } else {
            res.emplace_back(
                createBallparkGeocentricTranslation(sourceCRS, targetCRS));
        }
        return;
    }

    // Transformation between two geodetic systems of unknown type
    // This should normally not be triggered with "standard" CRS
    res.emplace_back(createGeodToGeodPROJBased(sourceCRS, targetCRS));
}

// ---------------------------------------------------------------------------

void CoordinateOperationFactory::Private::createOperationsDerivedTo(
    const crs::CRSNNPtr & /*sourceCRS*/, const crs::CRSNNPtr &targetCRS,
    Private::Context &context, const crs::DerivedCRS *derivedSrc,
    std::vector<CoordinateOperationNNPtr> &res) {

    ENTER_FUNCTION();

    auto opFirst = derivedSrc->derivingConversion()->inverse();
    // Small optimization if the targetCRS is the baseCRS of the source
    // derivedCRS.
    if (derivedSrc->baseCRS()->_isEquivalentTo(
            targetCRS.get(), util::IComparable::Criterion::EQUIVALENT)) {
        res.emplace_back(opFirst);
        return;
    }
    auto opsSecond =
        createOperations(derivedSrc->baseCRS(), targetCRS, context);
    for (const auto &opSecond : opsSecond) {
        try {
            res.emplace_back(ConcatenatedOperation::createComputeMetadata(
                {opFirst, opSecond}, disallowEmptyIntersection));
        } catch (const InvalidOperationEmptyIntersection &) {
        }
    }
}

// ---------------------------------------------------------------------------

void CoordinateOperationFactory::Private::createOperationsBoundToGeog(
    const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
    Private::Context &context, const crs::BoundCRS *boundSrc,
    const crs::GeographicCRS *geogDst,
    std::vector<CoordinateOperationNNPtr> &res) {

    ENTER_FUNCTION();

    const auto &hubSrc = boundSrc->hubCRS();
    auto hubSrcGeog = dynamic_cast<const crs::GeographicCRS *>(hubSrc.get());
    auto geogCRSOfBaseOfBoundSrc = boundSrc->baseCRS()->extractGeographicCRS();
    {
        // If geogCRSOfBaseOfBoundSrc is a DerivedGeographicCRS, use its base
        // instead (if it is a GeographicCRS)
        auto derivedGeogCRS =
            std::dynamic_pointer_cast<crs::DerivedGeographicCRS>(
                geogCRSOfBaseOfBoundSrc);
        if (derivedGeogCRS) {
            auto baseCRS = std::dynamic_pointer_cast<crs::GeographicCRS>(
                derivedGeogCRS->baseCRS().as_nullable());
            if (baseCRS) {
                geogCRSOfBaseOfBoundSrc = baseCRS;
            }
        }
    }

    const auto &authFactory = context.context->getAuthorityFactory();
    const auto dbContext =
        authFactory ? authFactory->databaseContext().as_nullable() : nullptr;

    const auto geogDstDatum = geogDst->datumNonNull(dbContext);

    // If the underlying datum of the source is the same as the target, do
    // not consider the boundCRS at all, but just its base
    if (geogCRSOfBaseOfBoundSrc) {
        auto geogCRSOfBaseOfBoundSrcDatum =
            geogCRSOfBaseOfBoundSrc->datumNonNull(dbContext);
        if (geogCRSOfBaseOfBoundSrcDatum->_isEquivalentTo(
                geogDstDatum.get(), util::IComparable::Criterion::EQUIVALENT)) {
            res = createOperations(boundSrc->baseCRS(), targetCRS, context);
            return;
        }
    }

    bool triedBoundCrsToGeogCRSSameAsHubCRS = false;
    // Is it: boundCRS to a geogCRS that is the same as the hubCRS ?
    if (hubSrcGeog && geogCRSOfBaseOfBoundSrc &&
        (hubSrcGeog->_isEquivalentTo(
             geogDst, util::IComparable::Criterion::EQUIVALENT) ||
         hubSrcGeog->is2DPartOf3D(NN_NO_CHECK(geogDst), dbContext))) {
        triedBoundCrsToGeogCRSSameAsHubCRS = true;

        CoordinateOperationPtr opIntermediate;
        if (!geogCRSOfBaseOfBoundSrc->_isEquivalentTo(
                boundSrc->transformation()->sourceCRS().get(),
                util::IComparable::Criterion::EQUIVALENT)) {
            auto opsIntermediate = createOperations(
                NN_NO_CHECK(geogCRSOfBaseOfBoundSrc),
                boundSrc->transformation()->sourceCRS(), context);
            assert(!opsIntermediate.empty());
            opIntermediate = opsIntermediate.front();
        }

        if (boundSrc->baseCRS() == geogCRSOfBaseOfBoundSrc) {
            if (opIntermediate) {
                try {
                    res.emplace_back(
                        ConcatenatedOperation::createComputeMetadata(
                            {NN_NO_CHECK(opIntermediate),
                             boundSrc->transformation()},
                            disallowEmptyIntersection));
                } catch (const InvalidOperationEmptyIntersection &) {
                }
            } else {
                // Optimization to avoid creating a useless concatenated
                // operation
                res.emplace_back(boundSrc->transformation());
            }
            return;
        }
        auto opsFirst = createOperations(
            boundSrc->baseCRS(), NN_NO_CHECK(geogCRSOfBaseOfBoundSrc), context);
        if (!opsFirst.empty()) {
            for (const auto &opFirst : opsFirst) {
                try {
                    std::vector<CoordinateOperationNNPtr> subops;
                    subops.emplace_back(opFirst);
                    if (opIntermediate) {
                        subops.emplace_back(NN_NO_CHECK(opIntermediate));
                    }
                    subops.emplace_back(boundSrc->transformation());
                    res.emplace_back(
                        ConcatenatedOperation::createComputeMetadata(
                            subops, disallowEmptyIntersection));
                } catch (const InvalidOperationEmptyIntersection &) {
                }
            }
            if (!res.empty()) {
                return;
            }
        }
        // If the datum are equivalent, this is also fine
    } else if (geogCRSOfBaseOfBoundSrc && hubSrcGeog &&
               hubSrcGeog->datumNonNull(dbContext)->_isEquivalentTo(
                   geogDstDatum.get(),
                   util::IComparable::Criterion::EQUIVALENT)) {
        auto opsFirst = createOperations(
            boundSrc->baseCRS(), NN_NO_CHECK(geogCRSOfBaseOfBoundSrc), context);
        auto opsLast = createOperations(hubSrc, targetCRS, context);
        if (!opsFirst.empty() && !opsLast.empty()) {
            CoordinateOperationPtr opIntermediate;
            if (!geogCRSOfBaseOfBoundSrc->_isEquivalentTo(
                    boundSrc->transformation()->sourceCRS().get(),
                    util::IComparable::Criterion::EQUIVALENT)) {
                auto opsIntermediate = createOperations(
                    NN_NO_CHECK(geogCRSOfBaseOfBoundSrc),
                    boundSrc->transformation()->sourceCRS(), context);
                assert(!opsIntermediate.empty());
                opIntermediate = opsIntermediate.front();
            }
            for (const auto &opFirst : opsFirst) {
                for (const auto &opLast : opsLast) {
                    try {
                        std::vector<CoordinateOperationNNPtr> subops;
                        subops.emplace_back(opFirst);
                        if (opIntermediate) {
                            subops.emplace_back(NN_NO_CHECK(opIntermediate));
                        }
                        subops.emplace_back(boundSrc->transformation());
                        subops.emplace_back(opLast);
                        res.emplace_back(
                            ConcatenatedOperation::createComputeMetadata(
                                subops, disallowEmptyIntersection));
                    } catch (const InvalidOperationEmptyIntersection &) {
                    }
                }
            }
            if (!res.empty()) {
                return;
            }
        }
        // Consider WGS 84 and NAD83 as equivalent in that context if the
        // geogCRSOfBaseOfBoundSrc ellipsoid is Clarke66 (for NAD27)
        // Case of "+proj=latlong +ellps=clrk66
        // +nadgrids=ntv1_can.dat,conus"
        // to "+proj=latlong +datum=NAD83"
    } else if (geogCRSOfBaseOfBoundSrc && hubSrcGeog &&
               geogCRSOfBaseOfBoundSrc->ellipsoid()->_isEquivalentTo(
                   datum::Ellipsoid::CLARKE_1866.get(),
                   util::IComparable::Criterion::EQUIVALENT) &&
               hubSrcGeog->datumNonNull(dbContext)->_isEquivalentTo(
                   datum::GeodeticReferenceFrame::EPSG_6326.get(),
                   util::IComparable::Criterion::EQUIVALENT) &&
               geogDstDatum->_isEquivalentTo(
                   datum::GeodeticReferenceFrame::EPSG_6269.get(),
                   util::IComparable::Criterion::EQUIVALENT)) {
        auto nnGeogCRSOfBaseOfBoundSrc = NN_NO_CHECK(geogCRSOfBaseOfBoundSrc);
        if (boundSrc->baseCRS()->_isEquivalentTo(
                nnGeogCRSOfBaseOfBoundSrc.get(),
                util::IComparable::Criterion::EQUIVALENT)) {
            auto transf = boundSrc->transformation()->shallowClone();
            transf->setProperties(util::PropertyMap().set(
                common::IdentifiedObject::NAME_KEY,
                buildTransfName(boundSrc->baseCRS()->nameStr(),
                                targetCRS->nameStr())));
            transf->setCRSs(boundSrc->baseCRS(), targetCRS, nullptr);
            res.emplace_back(transf);
            return;
        } else {
            auto opsFirst = createOperations(
                boundSrc->baseCRS(), nnGeogCRSOfBaseOfBoundSrc, context);
            auto transf = boundSrc->transformation()->shallowClone();
            transf->setProperties(util::PropertyMap().set(
                common::IdentifiedObject::NAME_KEY,
                buildTransfName(nnGeogCRSOfBaseOfBoundSrc->nameStr(),
                                targetCRS->nameStr())));
            transf->setCRSs(nnGeogCRSOfBaseOfBoundSrc, targetCRS, nullptr);
            if (!opsFirst.empty()) {
                for (const auto &opFirst : opsFirst) {
                    try {
                        res.emplace_back(
                            ConcatenatedOperation::createComputeMetadata(
                                {opFirst, transf}, disallowEmptyIntersection));
                    } catch (const InvalidOperationEmptyIntersection &) {
                    }
                }
                if (!res.empty()) {
                    return;
                }
            }
        }
    }

    if (hubSrcGeog &&
        hubSrcGeog->_isEquivalentTo(geogDst,
                                    util::IComparable::Criterion::EQUIVALENT) &&
        dynamic_cast<const crs::VerticalCRS *>(boundSrc->baseCRS().get())) {
        auto transfSrc = boundSrc->transformation()->sourceCRS();
        if (dynamic_cast<const crs::VerticalCRS *>(transfSrc.get()) &&
            !boundSrc->baseCRS()->_isEquivalentTo(
                transfSrc.get(), util::IComparable::Criterion::EQUIVALENT)) {
            auto opsFirst =
                createOperations(boundSrc->baseCRS(), transfSrc, context);
            for (const auto &opFirst : opsFirst) {
                try {
                    res.emplace_back(
                        ConcatenatedOperation::createComputeMetadata(
                            {opFirst, boundSrc->transformation()},
                            disallowEmptyIntersection));
                } catch (const InvalidOperationEmptyIntersection &) {
                }
            }
            return;
        }

        res.emplace_back(boundSrc->transformation());
        return;
    }

    if (!triedBoundCrsToGeogCRSSameAsHubCRS && hubSrcGeog &&
        geogCRSOfBaseOfBoundSrc) {
        // This one should go to the above 'Is it: boundCRS to a geogCRS
        // that is the same as the hubCRS ?' case
        auto opsFirst = createOperations(sourceCRS, hubSrc, context);
        auto opsLast = createOperations(hubSrc, targetCRS, context);
        if (!opsFirst.empty() && !opsLast.empty()) {
            for (const auto &opFirst : opsFirst) {
                for (const auto &opLast : opsLast) {
                    // Exclude artificial transformations from the hub
                    // to the target CRS, if it is the only one.
                    if (opsLast.size() > 1 ||
                        !opLast->hasBallparkTransformation()) {
                        try {
                            res.emplace_back(
                                ConcatenatedOperation::createComputeMetadata(
                                    {opFirst, opLast},
                                    disallowEmptyIntersection));
                        } catch (const InvalidOperationEmptyIntersection &) {
                        }
                    } else {
                        // std::cerr << "excluded " << opLast->nameStr() <<
                        // std::endl;
                    }
                }
            }
            if (!res.empty()) {
                return;
            }
        }
    }

    auto vertCRSOfBaseOfBoundSrc =
        dynamic_cast<const crs::VerticalCRS *>(boundSrc->baseCRS().get());
    if (vertCRSOfBaseOfBoundSrc && hubSrcGeog) {
        auto opsFirst = createOperations(sourceCRS, hubSrc, context);
        if (context.skipHorizontalTransformation) {
            if (!opsFirst.empty()) {
                const auto &hubAxisList =
                    hubSrcGeog->coordinateSystem()->axisList();
                const auto &targetAxisList =
                    geogDst->coordinateSystem()->axisList();
                if (hubAxisList.size() == 3 && targetAxisList.size() == 3 &&
                    !hubAxisList[2]->_isEquivalentTo(
                        targetAxisList[2].get(),
                        util::IComparable::Criterion::EQUIVALENT)) {

                    const auto &srcAxis = hubAxisList[2];
                    const double convSrc = srcAxis->unit().conversionToSI();
                    const auto &dstAxis = targetAxisList[2];
                    const double convDst = dstAxis->unit().conversionToSI();
                    const bool srcIsUp =
                        srcAxis->direction() == cs::AxisDirection::UP;
                    const bool srcIsDown =
                        srcAxis->direction() == cs::AxisDirection::DOWN;
                    const bool dstIsUp =
                        dstAxis->direction() == cs::AxisDirection::UP;
                    const bool dstIsDown =
                        dstAxis->direction() == cs::AxisDirection::DOWN;
                    const bool heightDepthReversal =
                        ((srcIsUp && dstIsDown) || (srcIsDown && dstIsUp));

                    const double factor = convSrc / convDst;
                    auto conv = Conversion::createChangeVerticalUnit(
                        util::PropertyMap().set(
                            common::IdentifiedObject::NAME_KEY,
                            "Change of vertical unit"),
                        common::Scale(heightDepthReversal ? -factor : factor));

                    conv->setCRSs(
                        hubSrc,
                        hubSrc->demoteTo2D(std::string(), dbContext)
                            ->promoteTo3D(std::string(), dbContext, dstAxis),
                        nullptr);

                    for (const auto &op : opsFirst) {
                        try {
                            res.emplace_back(
                                ConcatenatedOperation::createComputeMetadata(
                                    {op, conv}, disallowEmptyIntersection));
                        } catch (const InvalidOperationEmptyIntersection &) {
                        }
                    }
                } else {
                    res = opsFirst;
                }
            }
            return;
        } else {
            auto opsSecond = createOperations(hubSrc, targetCRS, context);
            if (!opsFirst.empty() && !opsSecond.empty()) {
                for (const auto &opFirst : opsFirst) {
                    for (const auto &opLast : opsSecond) {
                        // Exclude artificial transformations from the hub
                        // to the target CRS
                        if (!opLast->hasBallparkTransformation()) {
                            try {
                                res.emplace_back(
                                    ConcatenatedOperation::
                                        createComputeMetadata(
                                            {opFirst, opLast},
                                            disallowEmptyIntersection));
                            } catch (
                                const InvalidOperationEmptyIntersection &) {
                            }
                        } else {
                            // std::cerr << "excluded " << opLast->nameStr() <<
                            // std::endl;
                        }
                    }
                }
                if (!res.empty()) {
                    return;
                }
            }
        }
    }

    res = createOperations(boundSrc->baseCRS(), targetCRS, context);
}

// ---------------------------------------------------------------------------

void CoordinateOperationFactory::Private::createOperationsBoundToVert(
    const crs::CRSNNPtr & /*sourceCRS*/, const crs::CRSNNPtr &targetCRS,
    Private::Context &context, const crs::BoundCRS *boundSrc,
    const crs::VerticalCRS *vertDst,
    std::vector<CoordinateOperationNNPtr> &res) {

    ENTER_FUNCTION();

    auto baseSrcVert =
        dynamic_cast<const crs::VerticalCRS *>(boundSrc->baseCRS().get());
    const auto &hubSrc = boundSrc->hubCRS();
    auto hubSrcVert = dynamic_cast<const crs::VerticalCRS *>(hubSrc.get());
    if (baseSrcVert && hubSrcVert &&
        vertDst->_isEquivalentTo(hubSrcVert,
                                 util::IComparable::Criterion::EQUIVALENT)) {
        res.emplace_back(boundSrc->transformation());
        return;
    }

    res = createOperations(boundSrc->baseCRS(), targetCRS, context);
}

// ---------------------------------------------------------------------------

void CoordinateOperationFactory::Private::createOperationsVertToVert(
    const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
    Private::Context &context, const crs::VerticalCRS *vertSrc,
    const crs::VerticalCRS *vertDst,
    std::vector<CoordinateOperationNNPtr> &res) {

    ENTER_FUNCTION();

    const auto &authFactory = context.context->getAuthorityFactory();
    const auto dbContext =
        authFactory ? authFactory->databaseContext().as_nullable() : nullptr;

    const auto srcDatum = vertSrc->datumNonNull(dbContext);
    const auto dstDatum = vertDst->datumNonNull(dbContext);
    const bool equivalentVDatum = srcDatum->_isEquivalentTo(
        dstDatum.get(), util::IComparable::Criterion::EQUIVALENT);

    const auto &srcAxis = vertSrc->coordinateSystem()->axisList()[0];
    const double convSrc = srcAxis->unit().conversionToSI();
    const auto &dstAxis = vertDst->coordinateSystem()->axisList()[0];
    const double convDst = dstAxis->unit().conversionToSI();
    const bool srcIsUp = srcAxis->direction() == cs::AxisDirection::UP;
    const bool srcIsDown = srcAxis->direction() == cs::AxisDirection::DOWN;
    const bool dstIsUp = dstAxis->direction() == cs::AxisDirection::UP;
    const bool dstIsDown = dstAxis->direction() == cs::AxisDirection::DOWN;
    const bool heightDepthReversal =
        ((srcIsUp && dstIsDown) || (srcIsDown && dstIsUp));

    const double factor = convSrc / convDst;
    if (!equivalentVDatum) {
        auto name = buildTransfName(sourceCRS->nameStr(), targetCRS->nameStr());
        name += BALLPARK_VERTICAL_TRANSFORMATION;
        auto conv = Transformation::createChangeVerticalUnit(
            util::PropertyMap().set(common::IdentifiedObject::NAME_KEY, name),
            sourceCRS, targetCRS,
            // In case of a height depth reversal, we should probably have
            // 2 steps instead of putting a negative factor...
            common::Scale(heightDepthReversal ? -factor : factor), {});
        conv->setHasBallparkTransformation(true);
        res.push_back(conv);
    } else if (convSrc != convDst || !heightDepthReversal) {
        auto name = buildConvName(sourceCRS->nameStr(), targetCRS->nameStr());
        auto conv = Conversion::createChangeVerticalUnit(
            util::PropertyMap().set(common::IdentifiedObject::NAME_KEY, name),
            // In case of a height depth reversal, we should probably have
            // 2 steps instead of putting a negative factor...
            common::Scale(heightDepthReversal ? -factor : factor));
        conv->setCRSs(sourceCRS, targetCRS, nullptr);
        res.push_back(conv);
    } else {
        auto name = buildConvName(sourceCRS->nameStr(), targetCRS->nameStr());
        auto conv = Conversion::createHeightDepthReversal(
            util::PropertyMap().set(common::IdentifiedObject::NAME_KEY, name));
        conv->setCRSs(sourceCRS, targetCRS, nullptr);
        res.push_back(conv);
    }
}

// ---------------------------------------------------------------------------

void CoordinateOperationFactory::Private::createOperationsVertToGeog(
    const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
    Private::Context &context, const crs::VerticalCRS *vertSrc,
    const crs::GeographicCRS *geogDst,
    std::vector<CoordinateOperationNNPtr> &res) {

    ENTER_FUNCTION();

    if (vertSrc->identifiers().empty()) {
        const auto &vertSrcName = vertSrc->nameStr();
        const auto &authFactory = context.context->getAuthorityFactory();
        if (authFactory != nullptr && vertSrcName != "unnamed" &&
            vertSrcName != "unknown") {
            auto matches = authFactory->createObjectsFromName(
                vertSrcName, {io::AuthorityFactory::ObjectType::VERTICAL_CRS},
                false, 2);
            if (matches.size() == 1) {
                const auto &match = matches.front();
                if (vertSrc->_isEquivalentTo(
                        match.get(),
                        util::IComparable::Criterion::EQUIVALENT) &&
                    !match->identifiers().empty()) {
                    auto resTmp = createOperations(
                        NN_NO_CHECK(
                            util::nn_dynamic_pointer_cast<crs::VerticalCRS>(
                                match)),
                        targetCRS, context);
                    res.insert(res.end(), resTmp.begin(), resTmp.end());
                    return;
                }
            }
        }
    }

    createOperationsVertToGeogBallpark(sourceCRS, targetCRS, context, vertSrc,
                                       geogDst, res);
}

// ---------------------------------------------------------------------------

void CoordinateOperationFactory::Private::createOperationsVertToGeogBallpark(
    const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
    Private::Context &, const crs::VerticalCRS *vertSrc,
    const crs::GeographicCRS *geogDst,
    std::vector<CoordinateOperationNNPtr> &res) {

    ENTER_FUNCTION();

    const auto &srcAxis = vertSrc->coordinateSystem()->axisList()[0];
    const double convSrc = srcAxis->unit().conversionToSI();
    double convDst = 1.0;
    const auto &geogAxis = geogDst->coordinateSystem()->axisList();
    bool dstIsUp = true;
    bool dstIsDown = false;
    if (geogAxis.size() == 3) {
        const auto &dstAxis = geogAxis[2];
        convDst = dstAxis->unit().conversionToSI();
        dstIsUp = dstAxis->direction() == cs::AxisDirection::UP;
        dstIsDown = dstAxis->direction() == cs::AxisDirection::DOWN;
    }
    const bool srcIsUp = srcAxis->direction() == cs::AxisDirection::UP;
    const bool srcIsDown = srcAxis->direction() == cs::AxisDirection::DOWN;
    const bool heightDepthReversal =
        ((srcIsUp && dstIsDown) || (srcIsDown && dstIsUp));

    const double factor = convSrc / convDst;

    const auto &sourceCRSExtent = getExtent(sourceCRS);
    const auto &targetCRSExtent = getExtent(targetCRS);
    const bool sameExtent =
        sourceCRSExtent && targetCRSExtent &&
        sourceCRSExtent->_isEquivalentTo(
            targetCRSExtent.get(), util::IComparable::Criterion::EQUIVALENT);

    util::PropertyMap map;
    map.set(common::IdentifiedObject::NAME_KEY,
            buildTransfName(sourceCRS->nameStr(), targetCRS->nameStr()) +
                BALLPARK_VERTICAL_TRANSFORMATION_NO_ELLIPSOID_VERT_HEIGHT)
        .set(common::ObjectUsage::DOMAIN_OF_VALIDITY_KEY,
             sameExtent ? NN_NO_CHECK(sourceCRSExtent)
                        : metadata::Extent::WORLD);

    auto conv = Transformation::createChangeVerticalUnit(
        map, sourceCRS, targetCRS,
        common::Scale(heightDepthReversal ? -factor : factor), {});
    conv->setHasBallparkTransformation(true);
    res.push_back(conv);
}

// ---------------------------------------------------------------------------

void CoordinateOperationFactory::Private::createOperationsBoundToBound(
    const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
    Private::Context &context, const crs::BoundCRS *boundSrc,
    const crs::BoundCRS *boundDst, std::vector<CoordinateOperationNNPtr> &res) {

    ENTER_FUNCTION();

    // BoundCRS to BoundCRS of horizontal CRS using the same (geographic) hub
    const auto &hubSrc = boundSrc->hubCRS();
    auto hubSrcGeog = dynamic_cast<const crs::GeographicCRS *>(hubSrc.get());
    const auto &hubDst = boundDst->hubCRS();
    auto hubDstGeog = dynamic_cast<const crs::GeographicCRS *>(hubDst.get());
    if (hubSrcGeog && hubDstGeog &&
        hubSrcGeog->_isEquivalentTo(hubDstGeog,
                                    util::IComparable::Criterion::EQUIVALENT)) {
        auto opsFirst = createOperations(sourceCRS, hubSrc, context);
        auto opsLast = createOperations(hubSrc, targetCRS, context);
        for (const auto &opFirst : opsFirst) {
            for (const auto &opLast : opsLast) {
                try {
                    std::vector<CoordinateOperationNNPtr> ops;
                    ops.push_back(opFirst);
                    ops.push_back(opLast);
                    res.emplace_back(
                        ConcatenatedOperation::createComputeMetadata(
                            ops, disallowEmptyIntersection));
                } catch (const InvalidOperationEmptyIntersection &) {
                }
            }
        }
        if (!res.empty()) {
            return;
        }
    }

    // BoundCRS to BoundCRS of vertical CRS using the same vertical datum
    // ==> ignore the bound transformation
    auto baseOfBoundSrcAsVertCRS =
        dynamic_cast<crs::VerticalCRS *>(boundSrc->baseCRS().get());
    auto baseOfBoundDstAsVertCRS =
        dynamic_cast<crs::VerticalCRS *>(boundDst->baseCRS().get());
    if (baseOfBoundSrcAsVertCRS && baseOfBoundDstAsVertCRS) {

        const auto &authFactory = context.context->getAuthorityFactory();
        const auto dbContext =
            authFactory ? authFactory->databaseContext().as_nullable()
                        : nullptr;

        const auto datumSrc = baseOfBoundSrcAsVertCRS->datumNonNull(dbContext);
        const auto datumDst = baseOfBoundDstAsVertCRS->datumNonNull(dbContext);
        if (datumSrc->nameStr() == datumDst->nameStr() &&
            (datumSrc->nameStr() != "unknown" ||
             boundSrc->transformation()->_isEquivalentTo(
                 boundDst->transformation().get(),
                 util::IComparable::Criterion::EQUIVALENT))) {
            res = createOperations(boundSrc->baseCRS(), boundDst->baseCRS(),
                                   context);
            return;
        }
    }

    // BoundCRS to BoundCRS of vertical CRS
    auto vertCRSOfBaseOfBoundSrc = boundSrc->baseCRS()->extractVerticalCRS();
    auto vertCRSOfBaseOfBoundDst = boundDst->baseCRS()->extractVerticalCRS();
    if (hubSrcGeog && hubDstGeog &&
        hubSrcGeog->_isEquivalentTo(hubDstGeog,
                                    util::IComparable::Criterion::EQUIVALENT) &&
        vertCRSOfBaseOfBoundSrc && vertCRSOfBaseOfBoundDst) {
        auto opsFirst = createOperations(sourceCRS, hubSrc, context);
        auto opsLast = createOperations(hubSrc, targetCRS, context);
        if (!opsFirst.empty() && !opsLast.empty()) {
            for (const auto &opFirst : opsFirst) {
                for (const auto &opLast : opsLast) {
                    try {
                        res.emplace_back(
                            ConcatenatedOperation::createComputeMetadata(
                                {opFirst, opLast}, disallowEmptyIntersection));
                    } catch (const InvalidOperationEmptyIntersection &) {
                    }
                }
            }
            if (!res.empty()) {
                return;
            }
        }
    }

    res = createOperations(boundSrc->baseCRS(), boundDst->baseCRS(), context);
}

// ---------------------------------------------------------------------------

static std::vector<CoordinateOperationNNPtr>
getOps(const CoordinateOperationNNPtr &op) {
    auto concatenated = dynamic_cast<const ConcatenatedOperation *>(op.get());
    if (concatenated)
        return concatenated->operations();
    return {op};
}

// ---------------------------------------------------------------------------

static std::string normalize2D3DInName(const std::string &s) {
    std::string out = s;
    const char *const patterns[] = {
        " (2D)",
        " (geographic3D horizontal)",
        " (geog2D)",
        " (geog3D)",
    };
    for (const char *pattern : patterns) {
        out = replaceAll(out, pattern, "");
    }
    return out;
}

// ---------------------------------------------------------------------------

static bool useCompatibleTransformationsForSameSourceTarget(
    const CoordinateOperationNNPtr &opA, const CoordinateOperationNNPtr &opB) {
    const auto subOpsA = getOps(opA);
    const auto subOpsB = getOps(opB);

    for (const auto &subOpA : subOpsA) {
        if (!dynamic_cast<const Transformation *>(subOpA.get()))
            continue;
        const auto subOpAName = normalize2D3DInName(subOpA->nameStr());
        const auto &subOpASourceCRSName = subOpA->sourceCRS()->nameStr();
        const auto &subOpATargetCRSName = subOpA->targetCRS()->nameStr();
        if (subOpASourceCRSName == "unknown" ||
            subOpATargetCRSName == "unknown")
            continue;
        for (const auto &subOpB : subOpsB) {
            if (!dynamic_cast<const Transformation *>(subOpB.get()))
                continue;
            const auto &subOpBSourceCRSName = subOpB->sourceCRS()->nameStr();
            const auto &subOpBTargetCRSName = subOpB->targetCRS()->nameStr();
            if (subOpBSourceCRSName == "unknown" ||
                subOpBTargetCRSName == "unknown")
                continue;

            if (subOpASourceCRSName == subOpBSourceCRSName &&
                subOpATargetCRSName == subOpBTargetCRSName) {
                const auto &subOpBName = normalize2D3DInName(subOpB->nameStr());
                if (starts_with(subOpAName, NULL_GEOGRAPHIC_OFFSET) &&
                    starts_with(subOpB->nameStr(), NULL_GEOGRAPHIC_OFFSET)) {
                    continue;
                }
                if (subOpAName != subOpBName) {
                    return false;
                }
            } else if (subOpASourceCRSName == subOpBTargetCRSName &&
                       subOpATargetCRSName == subOpBSourceCRSName) {
                const auto &subOpBName = subOpB->nameStr();
                if (starts_with(subOpAName, NULL_GEOGRAPHIC_OFFSET) &&
                    starts_with(subOpBName, NULL_GEOGRAPHIC_OFFSET)) {
                    continue;
                }

                if (subOpAName !=
                    normalize2D3DInName(subOpB->inverse()->nameStr())) {
                    return false;
                }
            }
        }
    }
    return true;
}

// ---------------------------------------------------------------------------

static crs::GeographicCRSPtr
getInterpolationGeogCRS(const CoordinateOperationNNPtr &verticalTransform,
                        const io::DatabaseContextPtr &dbContext) {
    crs::GeographicCRSPtr interpolationGeogCRS;
    auto transformationVerticalTransform =
        dynamic_cast<const Transformation *>(verticalTransform.get());
    if (transformationVerticalTransform == nullptr) {
        const auto concat = dynamic_cast<const ConcatenatedOperation *>(
            verticalTransform.get());
        if (concat) {
            const auto &steps = concat->operations();
            // Is this change of unit and/or height depth reversal +
            // transformation ?
            for (const auto &step : steps) {
                const auto transf =
                    dynamic_cast<const Transformation *>(step.get());
                if (transf) {
                    // Only support a single Transformation in the steps
                    if (transformationVerticalTransform != nullptr) {
                        transformationVerticalTransform = nullptr;
                        break;
                    }
                    transformationVerticalTransform = transf;
                }
            }
        }
    }
    if (transformationVerticalTransform &&
        !transformationVerticalTransform->hasBallparkTransformation()) {
        auto interpTransformCRS =
            transformationVerticalTransform->interpolationCRS();
        if (interpTransformCRS) {
            interpolationGeogCRS =
                std::dynamic_pointer_cast<crs::GeographicCRS>(
                    interpTransformCRS);
        } else {
            // If no explicit interpolation CRS, then
            // this will be the geographic CRS of the
            // vertical to geog transformation
            interpolationGeogCRS =
                std::dynamic_pointer_cast<crs::GeographicCRS>(
                    transformationVerticalTransform->targetCRS().as_nullable());
        }
    }

    if (interpolationGeogCRS) {
        if (interpolationGeogCRS->coordinateSystem()->axisList().size() == 3) {
            // We need to force the interpolation CRS, which
            // will
            // frequently be 3D, to 2D to avoid transformations
            // between source CRS and interpolation CRS to have
            // 3D terms.
            interpolationGeogCRS =
                interpolationGeogCRS->demoteTo2D(std::string(), dbContext)
                    .as_nullable();
        }
    }

    return interpolationGeogCRS;
}

// ---------------------------------------------------------------------------

void CoordinateOperationFactory::Private::createOperationsCompoundToGeog(
    const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
    Private::Context &context, const crs::CompoundCRS *compoundSrc,
    const crs::GeographicCRS *geogDst,
    std::vector<CoordinateOperationNNPtr> &res) {

    ENTER_FUNCTION();

    const auto &authFactory = context.context->getAuthorityFactory();
    const auto &componentsSrc = compoundSrc->componentReferenceSystems();
    if (!componentsSrc.empty()) {

        if (componentsSrc.size() == 2) {
            auto derivedHSrc =
                dynamic_cast<const crs::DerivedCRS *>(componentsSrc[0].get());
            if (derivedHSrc) {
                std::vector<crs::CRSNNPtr> intermComponents{
                    derivedHSrc->baseCRS(), componentsSrc[1]};
                auto properties = util::PropertyMap().set(
                    common::IdentifiedObject::NAME_KEY,
                    intermComponents[0]->nameStr() + " + " +
                        intermComponents[1]->nameStr());
                auto intermCompound =
                    crs::CompoundCRS::create(properties, intermComponents);
                auto opsFirst =
                    createOperations(sourceCRS, intermCompound, context);
                assert(!opsFirst.empty());
                auto opsLast =
                    createOperations(intermCompound, targetCRS, context);
                for (const auto &opLast : opsLast) {
                    try {
                        res.emplace_back(
                            ConcatenatedOperation::createComputeMetadata(
                                {opsFirst.front(), opLast},
                                disallowEmptyIntersection));
                    } catch (const std::exception &) {
                    }
                }
                return;
            }
        }

        std::vector<CoordinateOperationNNPtr> horizTransforms;
        auto srcGeogCRS = componentsSrc[0]->extractGeographicCRS();
        if (srcGeogCRS) {
            horizTransforms =
                createOperations(componentsSrc[0], targetCRS, context);
        }
        std::vector<CoordinateOperationNNPtr> verticalTransforms;

        const auto dbContext =
            authFactory ? authFactory->databaseContext().as_nullable()
                        : nullptr;
        if (componentsSrc.size() >= 2 &&
            componentsSrc[1]->extractVerticalCRS()) {

            struct SetSkipHorizontalTransform {
                Context &context;

                explicit SetSkipHorizontalTransform(Context &contextIn)
                    : context(contextIn) {
                    assert(!context.skipHorizontalTransformation);
                    context.skipHorizontalTransformation = true;
                }

                ~SetSkipHorizontalTransform() {
                    context.skipHorizontalTransformation = false;
                }
            };
            SetSkipHorizontalTransform setSkipHorizontalTransform(context);

            verticalTransforms = createOperations(
                componentsSrc[1],
                targetCRS->promoteTo3D(std::string(), dbContext), context);
            bool foundRegisteredTransformWithAllGridsAvailable = false;
            const auto gridAvailabilityUse =
                context.context->getGridAvailabilityUse();
            const bool ignoreMissingGrids =
                gridAvailabilityUse ==
                CoordinateOperationContext::GridAvailabilityUse::
                    IGNORE_GRID_AVAILABILITY;
            for (const auto &op : verticalTransforms) {
                if (hasIdentifiers(op) && dbContext) {
                    bool missingGrid = false;
                    if (!ignoreMissingGrids) {
                        const auto gridsNeeded = op->gridsNeeded(
                            dbContext,
                            gridAvailabilityUse ==
                                CoordinateOperationContext::
                                    GridAvailabilityUse::KNOWN_AVAILABLE);
                        for (const auto &gridDesc : gridsNeeded) {
                            if (!gridDesc.available) {
                                missingGrid = true;
                                break;
                            }
                        }
                    }
                    if (!missingGrid) {
                        foundRegisteredTransformWithAllGridsAvailable = true;
                        break;
                    }
                }
            }
            if (!foundRegisteredTransformWithAllGridsAvailable && srcGeogCRS &&
                !srcGeogCRS->_isEquivalentTo(
                    geogDst, util::IComparable::Criterion::EQUIVALENT) &&
                !srcGeogCRS->is2DPartOf3D(NN_NO_CHECK(geogDst), dbContext)) {
                auto geogCRSTmp =
                    NN_NO_CHECK(srcGeogCRS)
                        ->demoteTo2D(std::string(), dbContext)
                        ->promoteTo3D(
                            std::string(), dbContext,
                            geogDst->coordinateSystem()->axisList().size() == 3
                                ? geogDst->coordinateSystem()->axisList()[2]
                                : cs::VerticalCS::createGravityRelatedHeight(
                                      common::UnitOfMeasure::METRE)
                                      ->axisList()[0]);
                auto verticalTransformsTmp =
                    createOperations(componentsSrc[1], geogCRSTmp, context);
                bool foundRegisteredTransform = false;
                foundRegisteredTransformWithAllGridsAvailable = false;
                for (const auto &op : verticalTransformsTmp) {
                    if (hasIdentifiers(op) && dbContext) {
                        bool missingGrid = false;
                        if (!ignoreMissingGrids) {
                            const auto gridsNeeded = op->gridsNeeded(
                                dbContext,
                                gridAvailabilityUse ==
                                    CoordinateOperationContext::
                                        GridAvailabilityUse::KNOWN_AVAILABLE);
                            for (const auto &gridDesc : gridsNeeded) {
                                if (!gridDesc.available) {
                                    missingGrid = true;
                                    break;
                                }
                            }
                        }
                        foundRegisteredTransform = true;
                        if (!missingGrid) {
                            foundRegisteredTransformWithAllGridsAvailable =
                                true;
                            break;
                        }
                    }
                }
                if (foundRegisteredTransformWithAllGridsAvailable) {
                    verticalTransforms = verticalTransformsTmp;
                } else if (foundRegisteredTransform) {
                    verticalTransforms.insert(verticalTransforms.end(),
                                              verticalTransformsTmp.begin(),
                                              verticalTransformsTmp.end());
                }
            }
        }

        if (horizTransforms.empty() || verticalTransforms.empty()) {
            res = horizTransforms;
            return;
        }

        typedef std::pair<std::vector<CoordinateOperationNNPtr>,
                          std::vector<CoordinateOperationNNPtr>>
            PairOfTransforms;
        std::map<std::string, PairOfTransforms>
            cacheHorizToInterpAndInterpToTarget;

        for (const auto &verticalTransform : verticalTransforms) {
#ifdef TRACE_CREATE_OPERATIONS
            ENTER_BLOCK("Considering vertical transform " +
                        objectAsStr(verticalTransform.get()));
#endif
            crs::GeographicCRSPtr interpolationGeogCRS =
                getInterpolationGeogCRS(verticalTransform, dbContext);
            if (interpolationGeogCRS) {
#ifdef TRACE_CREATE_OPERATIONS
                logTrace("Using " + objectAsStr(interpolationGeogCRS.get()) +
                         " as interpolation CRS");
#endif
                std::vector<CoordinateOperationNNPtr> srcToInterpOps;
                std::vector<CoordinateOperationNNPtr> interpToTargetOps;

                std::string key;
                const auto &ids = interpolationGeogCRS->identifiers();
                if (!ids.empty()) {
                    key =
                        (*ids.front()->codeSpace()) + ':' + ids.front()->code();
                }

                const auto computeOpsToInterp = [&srcToInterpOps,
                                                 &interpToTargetOps,
                                                 &componentsSrc,
                                                 &interpolationGeogCRS,
                                                 &targetCRS, &geogDst,
                                                 &dbContext, &context]() {
                    // Do the sourceCRS to interpolation CRS in 2D only
                    // to avoid altering the orthometric elevation
                    srcToInterpOps = createOperations(
                        componentsSrc[0], NN_NO_CHECK(interpolationGeogCRS),
                        context);

                    // But do the interpolation CRS to targetCRS in 3D
                    // to have proper ellipsoid height transformation.
                    // We need to force the vertical axis of this 3D'ified
                    // interpolation CRS to be the same as the target CRS,
                    // to avoid potential double vertical unit conversion,
                    // as the vertical transformation already takes care of
                    // that.
                    auto interp3D =
                        interpolationGeogCRS
                            ->demoteTo2D(std::string(), dbContext)
                            ->promoteTo3D(
                                std::string(), dbContext,
                                geogDst->coordinateSystem()
                                            ->axisList()
                                            .size() == 3
                                    ? geogDst->coordinateSystem()->axisList()[2]
                                    : cs::VerticalCS::
                                          createGravityRelatedHeight(
                                              common::UnitOfMeasure::METRE)
                                              ->axisList()[0]);
                    interpToTargetOps =
                        createOperations(interp3D, targetCRS, context);
                };

                if (!key.empty()) {
                    auto iter = cacheHorizToInterpAndInterpToTarget.find(key);
                    if (iter == cacheHorizToInterpAndInterpToTarget.end()) {
#ifdef TRACE_CREATE_OPERATIONS
                        ENTER_BLOCK("looking for horizontal transformation "
                                    "from source to interpCRS and interpCRS to "
                                    "target");
#endif
                        computeOpsToInterp();
                        cacheHorizToInterpAndInterpToTarget[key] =
                            PairOfTransforms(srcToInterpOps, interpToTargetOps);
                    } else {
                        srcToInterpOps = iter->second.first;
                        interpToTargetOps = iter->second.second;
                    }
                } else {
#ifdef TRACE_CREATE_OPERATIONS
                    ENTER_BLOCK("looking for horizontal transformation "
                                "from source to interpCRS and interpCRS to "
                                "target");
#endif
                    computeOpsToInterp();
                }

#ifdef TRACE_CREATE_OPERATIONS
                ENTER_BLOCK("creating HorizVerticalHorizPROJBased operations");
#endif
                const bool srcAndTargetGeogAreSame =
                    componentsSrc[0]->isEquivalentTo(
                        targetCRS->demoteTo2D(std::string(), dbContext).get(),
                        util::IComparable::Criterion::EQUIVALENT);

                // Lambda to add to the set the name of geodetic datum of the
                // CRS
                const auto addDatumOfToSet = [&dbContext](
                                                 std::set<std::string> &set,
                                                 const crs::CRSNNPtr &crs) {
                    auto geodCRS = crs->extractGeodeticCRS();
                    if (geodCRS) {
                        set.insert(geodCRS->datumNonNull(dbContext)->nameStr());
                    }
                };

                // Lambda to return the set of names of geodetic datums used
                // by the source and target CRS of a list of operations.
                const auto makeDatumSet =
                    [&addDatumOfToSet](
                        const std::vector<CoordinateOperationNNPtr> &ops) {
                        std::set<std::string> datumSetOps;
                        for (const auto &subOp : ops) {
                            if (!dynamic_cast<const Transformation *>(
                                    subOp.get()))
                                continue;
                            addDatumOfToSet(datumSetOps,
                                            NN_NO_CHECK(subOp->sourceCRS()));
                            addDatumOfToSet(datumSetOps,
                                            NN_NO_CHECK(subOp->targetCRS()));
                        }
                        return datumSetOps;
                    };

                std::map<CoordinateOperation *, std::set<std::string>>
                    mapSetDatumsUsed;
                if (srcAndTargetGeogAreSame) {
                    // When the geographic CRS of the source and target, we
                    // want to make sure that the transformation from the
                    // source to the interpolation CRS uses the same datums as
                    // the one from the interpolation CRS to the target CRS.
                    // A simplistic view would be that the srcToInterp and
                    // interpToTarget should be the same, but they are
                    // subtelties, like interpToTarget being done in 3D, so with
                    // additional conversion steps, slightly different names in
                    // operations between 2D and 3D. The initial filter on
                    // checking that we use the same set of datum enable us
                    // to be confident we reject upfront geodetically-dubious
                    // operations.
                    for (const auto &op : srcToInterpOps) {
                        mapSetDatumsUsed[op.get()] = makeDatumSet(getOps(op));
                    }
                    for (const auto &op : interpToTargetOps) {
                        mapSetDatumsUsed[op.get()] = makeDatumSet(getOps(op));
                    }
                }

                for (const auto &srcToInterp : srcToInterpOps) {
                    for (const auto &interpToTarget : interpToTargetOps) {

                        if ((srcAndTargetGeogAreSame &&
                             mapSetDatumsUsed[srcToInterp.get()] !=
                                 mapSetDatumsUsed[interpToTarget.get()]) ||
                            !useCompatibleTransformationsForSameSourceTarget(
                                srcToInterp, interpToTarget)) {
#ifdef TRACE_CREATE_OPERATIONS
                            logTrace(
                                "Considering that '" + srcToInterp->nameStr() +
                                "' and '" + interpToTarget->nameStr() +
                                "' do not use consistent operations in the pre "
                                "and post-vertical transformation steps");
#endif
                            continue;
                        }

                        try {
                            auto op = createHorizVerticalHorizPROJBased(
                                sourceCRS, targetCRS, srcToInterp,
                                verticalTransform, interpToTarget,
                                interpolationGeogCRS, true);
                            res.emplace_back(op);
                        } catch (const std::exception &) {
                        }
                    }
                }
            } else {
                // This case is probably only correct if
                // verticalTransform and horizTransform are independent
                // and in particular that verticalTransform does not
                // involve a grid, because of the rather arbitrary order
                // horizontal then vertical applied
                for (const auto &horizTransform : horizTransforms) {
                    try {
                        auto op = createHorizVerticalPROJBased(
                            sourceCRS, targetCRS, horizTransform,
                            verticalTransform, disallowEmptyIntersection);
                        res.emplace_back(op);
                    } catch (const std::exception &) {
                    }
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------

void CoordinateOperationFactory::Private::createOperationsToGeod(
    const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
    Private::Context &context, const crs::GeodeticCRS *geodDst,
    std::vector<CoordinateOperationNNPtr> &res) {

    auto cs = cs::EllipsoidalCS::createLatitudeLongitudeEllipsoidalHeight(
        common::UnitOfMeasure::DEGREE, common::UnitOfMeasure::METRE);
    auto intermGeog3DCRS =
        util::nn_static_pointer_cast<crs::CRS>(crs::GeographicCRS::create(
            util::PropertyMap()
                .set(common::IdentifiedObject::NAME_KEY, geodDst->nameStr())
                .set(common::ObjectUsage::DOMAIN_OF_VALIDITY_KEY,
                     metadata::Extent::WORLD),
            geodDst->datum(), geodDst->datumEnsemble(), cs));
    auto sourceToGeog3DOps =
        createOperations(sourceCRS, intermGeog3DCRS, context);
    auto geog3DToTargetOps =
        createOperations(intermGeog3DCRS, targetCRS, context);
    if (!geog3DToTargetOps.empty()) {
        for (const auto &op : sourceToGeog3DOps) {
            auto newOp = op->shallowClone();
            setCRSs(newOp.get(), sourceCRS, intermGeog3DCRS);
            try {
                res.emplace_back(ConcatenatedOperation::createComputeMetadata(
                    {newOp, geog3DToTargetOps.front()},
                    disallowEmptyIntersection));
            } catch (const InvalidOperationEmptyIntersection &) {
            }
        }
    }
}

// ---------------------------------------------------------------------------

void CoordinateOperationFactory::Private::createOperationsCompoundToCompound(
    const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
    Private::Context &context, const crs::CompoundCRS *compoundSrc,
    const crs::CompoundCRS *compoundDst,
    std::vector<CoordinateOperationNNPtr> &res) {

    const auto &componentsSrc = compoundSrc->componentReferenceSystems();
    const auto &componentsDst = compoundDst->componentReferenceSystems();
    if (componentsSrc.empty() || componentsSrc.size() != componentsDst.size()) {
        return;
    }
    const auto srcGeog = componentsSrc[0]->extractGeographicCRS();
    const auto dstGeog = componentsDst[0]->extractGeographicCRS();
    if (srcGeog == nullptr || dstGeog == nullptr) {
        return;
    }

    std::vector<CoordinateOperationNNPtr> verticalTransforms;
    if (componentsSrc.size() >= 2 && componentsSrc[1]->extractVerticalCRS() &&
        componentsDst[1]->extractVerticalCRS()) {
        if (!componentsSrc[1]->_isEquivalentTo(componentsDst[1].get())) {
            verticalTransforms =
                createOperations(componentsSrc[1], componentsDst[1], context);
        }
    }

    // If we didn't find a non-ballpark transformation between
    // the 2 vertical CRS, then try through intermediate geographic CRS
    // For example
    // WGS 84 + EGM96 --> ETRS89 + Belfast height where
    // there is a geoid model for EGM96 referenced to WGS 84
    // and a geoid model for Belfast height referenced to ETRS89
    if (verticalTransforms.size() == 1 &&
        verticalTransforms.front()->hasBallparkTransformation()) {
        auto dbContext =
            context.context->getAuthorityFactory()->databaseContext();
        const auto intermGeogSrc =
            srcGeog->promoteTo3D(std::string(), dbContext);
        const bool intermGeogSrcIsSameAsIntermGeogDst =
            srcGeog->_isEquivalentTo(dstGeog.get());
        const auto intermGeogDst =
            intermGeogSrcIsSameAsIntermGeogDst
                ? intermGeogSrc
                : dstGeog->promoteTo3D(std::string(), dbContext);
        const auto opsSrcToGeog =
            createOperations(sourceCRS, intermGeogSrc, context);
        const auto opsGeogToTarget =
            createOperations(intermGeogDst, targetCRS, context);
        const bool hasNonTrivialSrcTransf =
            !opsSrcToGeog.empty() &&
            !opsSrcToGeog.front()->hasBallparkTransformation();
        const bool hasNonTrivialTargetTransf =
            !opsGeogToTarget.empty() &&
            !opsGeogToTarget.front()->hasBallparkTransformation();
        if (hasNonTrivialSrcTransf && hasNonTrivialTargetTransf) {
            const auto opsGeogSrcToGeogDst =
                createOperations(intermGeogSrc, intermGeogDst, context);
            for (const auto &op1 : opsSrcToGeog) {
                if (op1->hasBallparkTransformation()) {
                    // std::cerr << "excluded " << op1->nameStr() << std::endl;
                    continue;
                }
                for (const auto &op2 : opsGeogSrcToGeogDst) {
                    for (const auto &op3 : opsGeogToTarget) {
                        if (op3->hasBallparkTransformation()) {
                            // std::cerr << "excluded " << op3->nameStr() <<
                            // std::endl;
                            continue;
                        }
                        try {
                            res.emplace_back(
                                ConcatenatedOperation::createComputeMetadata(
                                    intermGeogSrcIsSameAsIntermGeogDst
                                        ? std::vector<
                                              CoordinateOperationNNPtr>{op1,
                                                                        op3}
                                        : std::vector<
                                              CoordinateOperationNNPtr>{op1,
                                                                        op2,
                                                                        op3},
                                    disallowEmptyIntersection));
                        } catch (const std::exception &) {
                        }
                    }
                }
            }
        }
        if (!res.empty()) {
            return;
        }
    }

    for (const auto &verticalTransform : verticalTransforms) {
        auto interpolationGeogCRS = NN_NO_CHECK(srcGeog);
        auto interpTransformCRS = verticalTransform->interpolationCRS();
        if (interpTransformCRS) {
            auto nn_interpTransformCRS = NN_NO_CHECK(interpTransformCRS);
            if (dynamic_cast<const crs::GeographicCRS *>(
                    nn_interpTransformCRS.get())) {
                interpolationGeogCRS = NN_NO_CHECK(
                    util::nn_dynamic_pointer_cast<crs::GeographicCRS>(
                        nn_interpTransformCRS));
            }
        } else {
            auto compSrc0BoundCrs =
                dynamic_cast<crs::BoundCRS *>(componentsSrc[0].get());
            auto compDst0BoundCrs =
                dynamic_cast<crs::BoundCRS *>(componentsDst[0].get());
            if (compSrc0BoundCrs && compDst0BoundCrs &&
                dynamic_cast<crs::GeographicCRS *>(
                    compSrc0BoundCrs->hubCRS().get()) &&
                compSrc0BoundCrs->hubCRS()->_isEquivalentTo(
                    compDst0BoundCrs->hubCRS().get())) {
                interpolationGeogCRS = NN_NO_CHECK(
                    util::nn_dynamic_pointer_cast<crs::GeographicCRS>(
                        compSrc0BoundCrs->hubCRS()));
            }
        }
        auto opSrcCRSToGeogCRS =
            createOperations(componentsSrc[0], interpolationGeogCRS, context);
        auto opGeogCRStoDstCRS =
            createOperations(interpolationGeogCRS, componentsDst[0], context);
        for (const auto &opSrc : opSrcCRSToGeogCRS) {
            for (const auto &opDst : opGeogCRStoDstCRS) {

                try {
                    auto op = createHorizVerticalHorizPROJBased(
                        sourceCRS, targetCRS, opSrc, verticalTransform, opDst,
                        interpolationGeogCRS, true);
                    res.emplace_back(op);
                } catch (const InvalidOperationEmptyIntersection &) {
                } catch (const io::FormattingException &) {
                }
            }
        }
    }

    if (verticalTransforms.empty()) {
        auto resTmp =
            createOperations(componentsSrc[0], componentsDst[0], context);
        for (const auto &op : resTmp) {
            auto opClone = op->shallowClone();
            setCRSs(opClone.get(), sourceCRS, targetCRS);
            res.emplace_back(opClone);
        }
    }
}

// ---------------------------------------------------------------------------

void CoordinateOperationFactory::Private::createOperationsBoundToCompound(
    const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
    Private::Context &context, const crs::BoundCRS *boundSrc,
    const crs::CompoundCRS *compoundDst,
    std::vector<CoordinateOperationNNPtr> &res) {

    const auto &authFactory = context.context->getAuthorityFactory();
    const auto dbContext =
        authFactory ? authFactory->databaseContext().as_nullable() : nullptr;

    const auto &componentsDst = compoundDst->componentReferenceSystems();
    if (!componentsDst.empty()) {
        auto compDst0BoundCrs =
            dynamic_cast<crs::BoundCRS *>(componentsDst[0].get());
        if (compDst0BoundCrs) {
            auto boundSrcHubAsGeogCRS =
                dynamic_cast<crs::GeographicCRS *>(boundSrc->hubCRS().get());
            auto compDst0BoundCrsHubAsGeogCRS =
                dynamic_cast<crs::GeographicCRS *>(
                    compDst0BoundCrs->hubCRS().get());
            if (boundSrcHubAsGeogCRS && compDst0BoundCrsHubAsGeogCRS) {
                const auto boundSrcHubAsGeogCRSDatum =
                    boundSrcHubAsGeogCRS->datumNonNull(dbContext);
                const auto compDst0BoundCrsHubAsGeogCRSDatum =
                    compDst0BoundCrsHubAsGeogCRS->datumNonNull(dbContext);
                if (boundSrcHubAsGeogCRSDatum->_isEquivalentTo(
                        compDst0BoundCrsHubAsGeogCRSDatum.get())) {
                    auto cs = cs::EllipsoidalCS::
                        createLatitudeLongitudeEllipsoidalHeight(
                            common::UnitOfMeasure::DEGREE,
                            common::UnitOfMeasure::METRE);
                    auto intermGeog3DCRS = util::nn_static_pointer_cast<
                        crs::CRS>(crs::GeographicCRS::create(
                        util::PropertyMap()
                            .set(common::IdentifiedObject::NAME_KEY,
                                 boundSrcHubAsGeogCRS->nameStr())
                            .set(common::ObjectUsage::DOMAIN_OF_VALIDITY_KEY,
                                 metadata::Extent::WORLD),
                        boundSrcHubAsGeogCRS->datum(),
                        boundSrcHubAsGeogCRS->datumEnsemble(), cs));
                    auto sourceToGeog3DOps =
                        createOperations(sourceCRS, intermGeog3DCRS, context);
                    auto geog3DToTargetOps =
                        createOperations(intermGeog3DCRS, targetCRS, context);
                    for (const auto &opSrc : sourceToGeog3DOps) {
                        for (const auto &opDst : geog3DToTargetOps) {
                            if (opSrc->targetCRS() && opDst->sourceCRS() &&
                                !opSrc->targetCRS()->_isEquivalentTo(
                                    opDst->sourceCRS().get())) {
                                // Shouldn't happen normally, but typically
                                // one of them can be 2D and the other 3D
                                // due to above createOperations() not
                                // exactly setting the expected source and
                                // target CRS.
                                // So create an adapter operation...
                                auto intermOps = createOperations(
                                    NN_NO_CHECK(opSrc->targetCRS()),
                                    NN_NO_CHECK(opDst->sourceCRS()), context);
                                if (!intermOps.empty()) {
                                    res.emplace_back(
                                        ConcatenatedOperation::
                                            createComputeMetadata(
                                                {opSrc, intermOps.front(),
                                                 opDst},
                                                disallowEmptyIntersection));
                                }
                            } else {
                                res.emplace_back(
                                    ConcatenatedOperation::
                                        createComputeMetadata(
                                            {opSrc, opDst},
                                            disallowEmptyIntersection));
                            }
                        }
                    }
                    return;
                }
            }
        }
    }

    // There might be better things to do, but for now just ignore the
    // transformation of the bound CRS
    res = createOperations(boundSrc->baseCRS(), targetCRS, context);
}
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Find a list of CoordinateOperation from sourceCRS to targetCRS.
 *
 * The operations are sorted with the most relevant ones first: by
 * descending
 * area (intersection of the transformation area with the area of interest,
 * or intersection of the transformation with the area of use of the CRS),
 * and
 * by increasing accuracy. Operations with unknown accuracy are sorted last,
 * whatever their area.
 *
 * When one of the source or target CRS has a vertical component but not the
 * other one, the one that has no vertical component is automatically promoted
 * to a 3D version, where its vertical axis is the ellipsoidal height in metres,
 * using the ellipsoid of the base geodetic CRS.
 *
 * @param sourceCRS source CRS.
 * @param targetCRS target CRS.
 * @param context Search context.
 * @return a list
 */
std::vector<CoordinateOperationNNPtr>
CoordinateOperationFactory::createOperations(
    const crs::CRSNNPtr &sourceCRS, const crs::CRSNNPtr &targetCRS,
    const CoordinateOperationContextNNPtr &context) const {

#ifdef TRACE_CREATE_OPERATIONS
    ENTER_FUNCTION();
#endif
    // Look if we are called on CRS that have a link to a 'canonical'
    // BoundCRS
    // If so, use that one as input
    const auto &srcBoundCRS = sourceCRS->canonicalBoundCRS();
    const auto &targetBoundCRS = targetCRS->canonicalBoundCRS();
    auto l_sourceCRS = srcBoundCRS ? NN_NO_CHECK(srcBoundCRS) : sourceCRS;
    auto l_targetCRS = targetBoundCRS ? NN_NO_CHECK(targetBoundCRS) : targetCRS;
    const auto &authFactory = context->getAuthorityFactory();

    metadata::ExtentPtr sourceCRSExtent;
    auto l_resolvedSourceCRS =
        crs::CRS::getResolvedCRS(l_sourceCRS, authFactory, sourceCRSExtent);
    metadata::ExtentPtr targetCRSExtent;
    auto l_resolvedTargetCRS =
        crs::CRS::getResolvedCRS(l_targetCRS, authFactory, targetCRSExtent);
    Private::Context contextPrivate(sourceCRSExtent, targetCRSExtent, context);

    if (context->getSourceAndTargetCRSExtentUse() ==
        CoordinateOperationContext::SourceTargetCRSExtentUse::INTERSECTION) {
        if (sourceCRSExtent && targetCRSExtent &&
            !sourceCRSExtent->intersects(NN_NO_CHECK(targetCRSExtent))) {
            return std::vector<CoordinateOperationNNPtr>();
        }
    }

    return filterAndSort(Private::createOperations(l_resolvedSourceCRS,
                                                   l_resolvedTargetCRS,
                                                   contextPrivate),
                         context, sourceCRSExtent, targetCRSExtent);
}

// ---------------------------------------------------------------------------

/** \brief Instantiate a CoordinateOperationFactory.
 */
CoordinateOperationFactoryNNPtr CoordinateOperationFactory::create() {
    return NN_NO_CHECK(
        CoordinateOperationFactory::make_unique<CoordinateOperationFactory>());
}

// ---------------------------------------------------------------------------

} // namespace operation

namespace crs {
// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress

crs::CRSNNPtr CRS::getResolvedCRS(const crs::CRSNNPtr &crs,
                                  const io::AuthorityFactoryPtr &authFactory,
                                  metadata::ExtentPtr &extentOut) {
    const auto &ids = crs->identifiers();
    const auto &name = crs->nameStr();

    bool approxExtent;
    extentOut = operation::getExtentPossiblySynthetized(crs, approxExtent);

    // We try to "identify" the provided CRS with the ones of the database,
    // but in a more restricted way that what identify() does.
    // If we get a match from id in priority, and from name as a fallback, and
    // that they are equivalent to the input CRS, then use the identified CRS.
    // Even if they aren't equivalent, we update extentOut with the one of the
    // identified CRS if our input one is absent/not reliable.

    const auto tryToIdentifyByName =
        [&crs, &name, &authFactory, approxExtent,
         &extentOut](io::AuthorityFactory::ObjectType objectType) {
            if (name != "unknown" && name != "unnamed") {
                auto matches = authFactory->createObjectsFromName(
                    name, {objectType}, false, 2);
                if (matches.size() == 1) {
                    const auto match =
                        util::nn_static_pointer_cast<crs::CRS>(matches.front());
                    if (approxExtent || !extentOut) {
                        extentOut = operation::getExtent(match);
                    }
                    if (match->isEquivalentTo(
                            crs.get(),
                            util::IComparable::Criterion::EQUIVALENT)) {
                        return match;
                    }
                }
            }
            return crs;
        };

    auto geogCRS = dynamic_cast<crs::GeographicCRS *>(crs.get());
    if (geogCRS && authFactory) {
        if (!ids.empty()) {
            const auto tmpAuthFactory = io::AuthorityFactory::create(
                authFactory->databaseContext(), *ids.front()->codeSpace());
            try {
                auto resolvedCrs(
                    tmpAuthFactory->createGeographicCRS(ids.front()->code()));
                if (approxExtent || !extentOut) {
                    extentOut = operation::getExtent(resolvedCrs);
                }
                if (resolvedCrs->isEquivalentTo(
                        crs.get(), util::IComparable::Criterion::EQUIVALENT)) {
                    return util::nn_static_pointer_cast<crs::CRS>(resolvedCrs);
                }
            } catch (const std::exception &) {
            }
        } else {
            return tryToIdentifyByName(
                geogCRS->coordinateSystem()->axisList().size() == 2
                    ? io::AuthorityFactory::ObjectType::GEOGRAPHIC_2D_CRS
                    : io::AuthorityFactory::ObjectType::GEOGRAPHIC_3D_CRS);
        }
    }

    auto projectedCrs = dynamic_cast<crs::ProjectedCRS *>(crs.get());
    if (projectedCrs && authFactory) {
        if (!ids.empty()) {
            const auto tmpAuthFactory = io::AuthorityFactory::create(
                authFactory->databaseContext(), *ids.front()->codeSpace());
            try {
                auto resolvedCrs(
                    tmpAuthFactory->createProjectedCRS(ids.front()->code()));
                if (approxExtent || !extentOut) {
                    extentOut = operation::getExtent(resolvedCrs);
                }
                if (resolvedCrs->isEquivalentTo(
                        crs.get(), util::IComparable::Criterion::EQUIVALENT)) {
                    return util::nn_static_pointer_cast<crs::CRS>(resolvedCrs);
                }
            } catch (const std::exception &) {
            }
        } else {
            return tryToIdentifyByName(
                io::AuthorityFactory::ObjectType::PROJECTED_CRS);
        }
    }

    auto compoundCrs = dynamic_cast<crs::CompoundCRS *>(crs.get());
    if (compoundCrs && authFactory) {
        if (!ids.empty()) {
            const auto tmpAuthFactory = io::AuthorityFactory::create(
                authFactory->databaseContext(), *ids.front()->codeSpace());
            try {
                auto resolvedCrs(
                    tmpAuthFactory->createCompoundCRS(ids.front()->code()));
                if (approxExtent || !extentOut) {
                    extentOut = operation::getExtent(resolvedCrs);
                }
                if (resolvedCrs->isEquivalentTo(
                        crs.get(), util::IComparable::Criterion::EQUIVALENT)) {
                    return util::nn_static_pointer_cast<crs::CRS>(resolvedCrs);
                }
            } catch (const std::exception &) {
            }
        } else {
            auto outCrs = tryToIdentifyByName(
                io::AuthorityFactory::ObjectType::COMPOUND_CRS);
            const auto &components = compoundCrs->componentReferenceSystems();
            if (outCrs.get() != crs.get()) {
                bool hasGeoid = false;
                if (components.size() == 2) {
                    auto vertCRS =
                        dynamic_cast<crs::VerticalCRS *>(components[1].get());
                    if (vertCRS && !vertCRS->geoidModel().empty()) {
                        hasGeoid = true;
                    }
                }
                if (!hasGeoid) {
                    return outCrs;
                }
            }
            if (approxExtent || !extentOut) {
                // If we still did not get a reliable extent, then try to
                // resolve the components of the compoundCRS, and take the
                // intersection of their extent.
                extentOut = metadata::ExtentPtr();
                for (const auto &component : components) {
                    metadata::ExtentPtr componentExtent;
                    getResolvedCRS(component, authFactory, componentExtent);
                    if (extentOut && componentExtent)
                        extentOut = extentOut->intersection(
                            NN_NO_CHECK(componentExtent));
                    else if (componentExtent)
                        extentOut = componentExtent;
                }
            }
        }
    }
    return crs;
}

//! @endcond

} // namespace crs
NS_PROJ_END
