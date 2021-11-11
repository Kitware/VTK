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

#include "coordinateoperation_internal.hpp"
#include "coordinateoperation_private.hpp"
#include "operationmethod_private.hpp"
#include "oputils.hpp"
#include "parammappings.hpp"

// PROJ include order is sensitive
// clang-format off
#include "proj.h"
#include "proj_internal.h" // M_PI
// clang-format on
#include "proj_constants.h"
#include "proj_json_streaming_writer.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace NS_PROJ::internal;

// ---------------------------------------------------------------------------

NS_PROJ_START
namespace operation {

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress

InvalidOperationEmptyIntersection::InvalidOperationEmptyIntersection(
    const std::string &message)
    : InvalidOperation(message) {}

InvalidOperationEmptyIntersection::InvalidOperationEmptyIntersection(
    const InvalidOperationEmptyIntersection &) = default;

InvalidOperationEmptyIntersection::~InvalidOperationEmptyIntersection() =
    default;

//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress

// ---------------------------------------------------------------------------

GridDescription::GridDescription()
    : shortName{}, fullName{}, packageName{}, url{}, directDownload(false),
      openLicense(false), available(false) {}

GridDescription::~GridDescription() = default;

GridDescription::GridDescription(const GridDescription &) = default;

GridDescription::GridDescription(GridDescription &&other) noexcept
    : shortName(std::move(other.shortName)),
      fullName(std::move(other.fullName)),
      packageName(std::move(other.packageName)), url(std::move(other.url)),
      directDownload(other.directDownload), openLicense(other.openLicense),
      available(other.available) {}

//! @endcond

// ---------------------------------------------------------------------------

CoordinateOperation::CoordinateOperation()
    : d(internal::make_unique<Private>()) {}

// ---------------------------------------------------------------------------

CoordinateOperation::CoordinateOperation(const CoordinateOperation &other)
    : ObjectUsage(other), d(internal::make_unique<Private>(*other.d)) {}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
CoordinateOperation::~CoordinateOperation() = default;
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Return the version of the coordinate transformation (i.e.
 * instantiation
 * due to the stochastic nature of the parameters).
 *
 * Mandatory when describing a coordinate transformation or point motion
 * operation, and should not be supplied for a coordinate conversion.
 *
 * @return version or empty.
 */
const util::optional<std::string> &
CoordinateOperation::operationVersion() const {
    return d->operationVersion_;
}

// ---------------------------------------------------------------------------

/** \brief Return estimate(s) of the impact of this coordinate operation on
 * point accuracy.
 *
 * Gives position error estimates for target coordinates of this coordinate
 * operation, assuming no errors in source coordinates.
 *
 * @return estimate(s) or empty vector.
 */
const std::vector<metadata::PositionalAccuracyNNPtr> &
CoordinateOperation::coordinateOperationAccuracies() const {
    return d->coordinateOperationAccuracies_;
}

// ---------------------------------------------------------------------------

/** \brief Return the source CRS of this coordinate operation.
 *
 * This should not be null, expect for of a derivingConversion of a DerivedCRS
 * when the owning DerivedCRS has been destroyed.
 *
 * @return source CRS, or null.
 */
const crs::CRSPtr CoordinateOperation::sourceCRS() const {
    return d->sourceCRSWeak_.lock();
}

// ---------------------------------------------------------------------------

/** \brief Return the target CRS of this coordinate operation.
 *
 * This should not be null, expect for of a derivingConversion of a DerivedCRS
 * when the owning DerivedCRS has been destroyed.
 *
 * @return target CRS, or null.
 */
const crs::CRSPtr CoordinateOperation::targetCRS() const {
    return d->targetCRSWeak_.lock();
}

// ---------------------------------------------------------------------------

/** \brief Return the interpolation CRS of this coordinate operation.
 *
 * @return interpolation CRS, or null.
 */
const crs::CRSPtr &CoordinateOperation::interpolationCRS() const {
    return d->interpolationCRS_;
}

// ---------------------------------------------------------------------------

/** \brief Return the source epoch of coordinates.
 *
 * @return source epoch of coordinates, or empty.
 */
const util::optional<common::DataEpoch> &
CoordinateOperation::sourceCoordinateEpoch() const {
    return d->sourceCoordinateEpoch_;
}

// ---------------------------------------------------------------------------

/** \brief Return the target epoch of coordinates.
 *
 * @return target epoch of coordinates, or empty.
 */
const util::optional<common::DataEpoch> &
CoordinateOperation::targetCoordinateEpoch() const {
    return d->targetCoordinateEpoch_;
}

// ---------------------------------------------------------------------------

void CoordinateOperation::setWeakSourceTargetCRS(
    std::weak_ptr<crs::CRS> sourceCRSIn, std::weak_ptr<crs::CRS> targetCRSIn) {
    d->sourceCRSWeak_ = sourceCRSIn;
    d->targetCRSWeak_ = targetCRSIn;
}

// ---------------------------------------------------------------------------

void CoordinateOperation::setCRSs(const crs::CRSNNPtr &sourceCRSIn,
                                  const crs::CRSNNPtr &targetCRSIn,
                                  const crs::CRSPtr &interpolationCRSIn) {
    d->strongRef_ =
        internal::make_unique<Private::CRSStrongRef>(sourceCRSIn, targetCRSIn);
    d->sourceCRSWeak_ = sourceCRSIn.as_nullable();
    d->targetCRSWeak_ = targetCRSIn.as_nullable();
    d->interpolationCRS_ = interpolationCRSIn;
}
// ---------------------------------------------------------------------------

void CoordinateOperation::setCRSs(const CoordinateOperation *in,
                                  bool inverseSourceTarget) {
    auto l_sourceCRS = in->sourceCRS();
    auto l_targetCRS = in->targetCRS();
    if (l_sourceCRS && l_targetCRS) {
        auto nn_sourceCRS = NN_NO_CHECK(l_sourceCRS);
        auto nn_targetCRS = NN_NO_CHECK(l_targetCRS);
        if (inverseSourceTarget) {
            setCRSs(nn_targetCRS, nn_sourceCRS, in->interpolationCRS());
        } else {
            setCRSs(nn_sourceCRS, nn_targetCRS, in->interpolationCRS());
        }
    }
}

// ---------------------------------------------------------------------------

void CoordinateOperation::setAccuracies(
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {
    d->coordinateOperationAccuracies_ = accuracies;
}

// ---------------------------------------------------------------------------

/** \brief Return whether a coordinate operation can be instantiated as
 * a PROJ pipeline, checking in particular that referenced grids are
 * available.
 */
bool CoordinateOperation::isPROJInstantiable(
    const io::DatabaseContextPtr &databaseContext,
    bool considerKnownGridsAsAvailable) const {
    try {
        exportToPROJString(io::PROJStringFormatter::create().get());
    } catch (const std::exception &) {
        return false;
    }
    for (const auto &gridDesc :
         gridsNeeded(databaseContext, considerKnownGridsAsAvailable)) {
        if (!gridDesc.available) {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------

/** \brief Return whether a coordinate operation has a "ballpark"
 * transformation,
 * that is a very approximate one, due to lack of more accurate transformations.
 *
 * Typically a null geographic offset between two horizontal datum, or a
 * null vertical offset (or limited to unit changes) between two vertical
 * datum. Errors of several tens to one hundred meters might be expected,
 * compared to more accurate transformations.
 */
bool CoordinateOperation::hasBallparkTransformation() const {
    return d->hasBallparkTransformation_;
}

// ---------------------------------------------------------------------------

void CoordinateOperation::setHasBallparkTransformation(bool b) {
    d->hasBallparkTransformation_ = b;
}

// ---------------------------------------------------------------------------

void CoordinateOperation::setProperties(
    const util::PropertyMap &properties) // throw(InvalidValueTypeException)
{
    ObjectUsage::setProperties(properties);
    properties.getStringValue(OPERATION_VERSION_KEY, d->operationVersion_);
}

// ---------------------------------------------------------------------------

/** \brief Return a variation of the current coordinate operation whose axis
 * order is the one expected for visualization purposes.
 */
CoordinateOperationNNPtr
CoordinateOperation::normalizeForVisualization() const {
    auto l_sourceCRS = sourceCRS();
    auto l_targetCRS = targetCRS();
    if (!l_sourceCRS || !l_targetCRS) {
        throw util::UnsupportedOperationException(
            "Cannot retrieve source or target CRS");
    }
    const bool swapSource =
        l_sourceCRS->mustAxisOrderBeSwitchedForVisualization();
    const bool swapTarget =
        l_targetCRS->mustAxisOrderBeSwitchedForVisualization();
    auto l_this = NN_NO_CHECK(std::dynamic_pointer_cast<CoordinateOperation>(
        shared_from_this().as_nullable()));
    if (!swapSource && !swapTarget) {
        return l_this;
    }
    std::vector<CoordinateOperationNNPtr> subOps;
    if (swapSource) {
        auto op = Conversion::createAxisOrderReversal(false);
        op->setCRSs(l_sourceCRS->normalizeForVisualization(),
                    NN_NO_CHECK(l_sourceCRS), nullptr);
        subOps.emplace_back(op);
    }
    subOps.emplace_back(l_this);
    if (swapTarget) {
        auto op = Conversion::createAxisOrderReversal(false);
        op->setCRSs(NN_NO_CHECK(l_targetCRS),
                    l_targetCRS->normalizeForVisualization(), nullptr);
        subOps.emplace_back(op);
    }
    return util::nn_static_pointer_cast<CoordinateOperation>(
        ConcatenatedOperation::createComputeMetadata(subOps, true));
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
CoordinateOperationNNPtr CoordinateOperation::shallowClone() const {
    return _shallowClone();
}
//! @endcond

// ---------------------------------------------------------------------------

OperationMethod::OperationMethod() : d(internal::make_unique<Private>()) {}

// ---------------------------------------------------------------------------

OperationMethod::OperationMethod(const OperationMethod &other)
    : IdentifiedObject(other), d(internal::make_unique<Private>(*other.d)) {}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
OperationMethod::~OperationMethod() = default;
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Return the formula(s) or procedure used by this coordinate operation
 * method.
 *
 * This may be a reference to a publication (in which case use
 * formulaCitation()).
 *
 * Note that the operation method may not be analytic, in which case this
 * attribute references or contains the procedure, not an analytic formula.
 *
 * @return the formula, or empty.
 */
const util::optional<std::string> &OperationMethod::formula() PROJ_PURE_DEFN {
    return d->formula_;
}

// ---------------------------------------------------------------------------

/** \brief Return a reference to a publication giving the formula(s) or
 * procedure
 * used by the coordinate operation method.
 *
 * @return the formula citation, or empty.
 */
const util::optional<metadata::Citation> &
OperationMethod::formulaCitation() PROJ_PURE_DEFN {
    return d->formulaCitation_;
}

// ---------------------------------------------------------------------------

/** \brief Return the parameters of this operation method.
 *
 * @return the parameters.
 */
const std::vector<GeneralOperationParameterNNPtr> &
OperationMethod::parameters() PROJ_PURE_DEFN {
    return d->parameters_;
}

// ---------------------------------------------------------------------------

/** \brief Instantiate a operation method from a vector of
 * GeneralOperationParameter.
 *
 * @param properties See \ref general_properties. At minimum the name should be
 * defined.
 * @param parameters Vector of GeneralOperationParameterNNPtr.
 * @return a new OperationMethod.
 */
OperationMethodNNPtr OperationMethod::create(
    const util::PropertyMap &properties,
    const std::vector<GeneralOperationParameterNNPtr> &parameters) {
    OperationMethodNNPtr method(
        OperationMethod::nn_make_shared<OperationMethod>());
    method->assignSelf(method);
    method->setProperties(properties);
    method->d->parameters_ = parameters;
    properties.getStringValue("proj_method", method->d->projMethodOverride_);
    return method;
}

// ---------------------------------------------------------------------------

/** \brief Instantiate a operation method from a vector of OperationParameter.
 *
 * @param properties See \ref general_properties. At minimum the name should be
 * defined.
 * @param parameters Vector of OperationParameterNNPtr.
 * @return a new OperationMethod.
 */
OperationMethodNNPtr OperationMethod::create(
    const util::PropertyMap &properties,
    const std::vector<OperationParameterNNPtr> &parameters) {
    std::vector<GeneralOperationParameterNNPtr> parametersGeneral;
    parametersGeneral.reserve(parameters.size());
    for (const auto &p : parameters) {
        parametersGeneral.push_back(p);
    }
    return create(properties, parametersGeneral);
}

// ---------------------------------------------------------------------------

/** \brief Return the EPSG code, either directly, or through the name
 * @return code, or 0 if not found
 */
int OperationMethod::getEPSGCode() PROJ_PURE_DEFN {
    int epsg_code = IdentifiedObject::getEPSGCode();
    if (epsg_code == 0) {
        auto l_name = nameStr();
        if (ends_with(l_name, " (3D)")) {
            l_name.resize(l_name.size() - strlen(" (3D)"));
        }
        size_t nMethodNameCodes = 0;
        const auto methodNameCodes = getMethodNameCodes(nMethodNameCodes);
        for (size_t i = 0; i < nMethodNameCodes; ++i) {
            const auto &tuple = methodNameCodes[i];
            if (metadata::Identifier::isEquivalentName(l_name.c_str(),
                                                       tuple.name)) {
                return tuple.epsg_code;
            }
        }
    }
    return epsg_code;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void OperationMethod::_exportToWKT(io::WKTFormatter *formatter) const {
    const bool isWKT2 = formatter->version() == io::WKTFormatter::Version::WKT2;
    formatter->startNode(isWKT2 ? io::WKTConstants::METHOD
                                : io::WKTConstants::PROJECTION,
                         !identifiers().empty());
    std::string l_name(nameStr());
    if (!isWKT2) {
        const MethodMapping *mapping = getMapping(this);
        if (mapping == nullptr) {
            l_name = replaceAll(l_name, " ", "_");
        } else {
            if (l_name ==
                PROJ_WKT2_NAME_METHOD_GEOSTATIONARY_SATELLITE_SWEEP_X) {
                l_name = "Geostationary_Satellite";
            } else {
                if (mapping->wkt1_name == nullptr) {
                    throw io::FormattingException(
                        std::string("Unsupported conversion method: ") +
                        mapping->wkt2_name);
                }
                l_name = mapping->wkt1_name;
            }
        }
    }
    formatter->addQuotedString(l_name);
    if (formatter->outputId()) {
        formatID(formatter);
    }
    formatter->endNode();
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void OperationMethod::_exportToJSON(
    io::JSONFormatter *formatter) const // throw(FormattingException)
{
    auto writer = formatter->writer();
    auto objectContext(formatter->MakeObjectContext("OperationMethod",
                                                    !identifiers().empty()));

    writer->AddObjKey("name");
    writer->Add(nameStr());

    if (formatter->outputId()) {
        formatID(formatter);
    }
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
bool OperationMethod::_isEquivalentTo(
    const util::IComparable *other, util::IComparable::Criterion criterion,
    const io::DatabaseContextPtr &dbContext) const {
    auto otherOM = dynamic_cast<const OperationMethod *>(other);
    if (otherOM == nullptr ||
        !IdentifiedObject::_isEquivalentTo(other, criterion, dbContext)) {
        return false;
    }
    // TODO test formula and formulaCitation
    const auto &params = parameters();
    const auto &otherParams = otherOM->parameters();
    const auto paramsSize = params.size();
    if (paramsSize != otherParams.size()) {
        return false;
    }
    if (criterion == util::IComparable::Criterion::STRICT) {
        for (size_t i = 0; i < paramsSize; i++) {
            if (!params[i]->_isEquivalentTo(otherParams[i].get(), criterion,
                                            dbContext)) {
                return false;
            }
        }
    } else {
        std::vector<bool> candidateIndices(paramsSize, true);
        for (size_t i = 0; i < paramsSize; i++) {
            bool found = false;
            for (size_t j = 0; j < paramsSize; j++) {
                if (candidateIndices[j] &&
                    params[i]->_isEquivalentTo(otherParams[j].get(), criterion,
                                               dbContext)) {
                    candidateIndices[j] = false;
                    found = true;
                    break;
                }
            }
            if (!found) {
                return false;
            }
        }
    }
    return true;
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct GeneralParameterValue::Private {};
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
GeneralParameterValue::GeneralParameterValue() : d(nullptr) {}

// ---------------------------------------------------------------------------

GeneralParameterValue::GeneralParameterValue(const GeneralParameterValue &)
    : d(nullptr) {}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
GeneralParameterValue::~GeneralParameterValue() = default;
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct OperationParameterValue::Private {
    OperationParameterNNPtr parameter;
    ParameterValueNNPtr parameterValue;

    Private(const OperationParameterNNPtr &parameterIn,
            const ParameterValueNNPtr &valueIn)
        : parameter(parameterIn), parameterValue(valueIn) {}
};
//! @endcond

// ---------------------------------------------------------------------------

OperationParameterValue::OperationParameterValue(
    const OperationParameterValue &other)
    : GeneralParameterValue(other),
      d(internal::make_unique<Private>(*other.d)) {}

// ---------------------------------------------------------------------------

OperationParameterValue::OperationParameterValue(
    const OperationParameterNNPtr &parameterIn,
    const ParameterValueNNPtr &valueIn)
    : GeneralParameterValue(),
      d(internal::make_unique<Private>(parameterIn, valueIn)) {}

// ---------------------------------------------------------------------------

/** \brief Instantiate a OperationParameterValue.
 *
 * @param parameterIn Parameter (definition).
 * @param valueIn Parameter value.
 * @return a new OperationParameterValue.
 */
OperationParameterValueNNPtr
OperationParameterValue::create(const OperationParameterNNPtr &parameterIn,
                                const ParameterValueNNPtr &valueIn) {
    return OperationParameterValue::nn_make_shared<OperationParameterValue>(
        parameterIn, valueIn);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
OperationParameterValue::~OperationParameterValue() = default;
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Return the parameter (definition)
 *
 * @return the parameter (definition).
 */
const OperationParameterNNPtr &
OperationParameterValue::parameter() PROJ_PURE_DEFN {
    return d->parameter;
}

// ---------------------------------------------------------------------------

/** \brief Return the parameter value.
 *
 * @return the parameter value.
 */
const ParameterValueNNPtr &
OperationParameterValue::parameterValue() PROJ_PURE_DEFN {
    return d->parameterValue;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void OperationParameterValue::_exportToWKT(
    // cppcheck-suppress passedByValue
    io::WKTFormatter *formatter) const {
    _exportToWKT(formatter, nullptr);
}

void OperationParameterValue::_exportToWKT(io::WKTFormatter *formatter,
                                           const MethodMapping *mapping) const {
    const ParamMapping *paramMapping =
        mapping ? getMapping(mapping, d->parameter) : nullptr;
    if (paramMapping && paramMapping->wkt1_name == nullptr) {
        return;
    }
    const bool isWKT2 = formatter->version() == io::WKTFormatter::Version::WKT2;
    if (isWKT2 && parameterValue()->type() == ParameterValue::Type::FILENAME) {
        formatter->startNode(io::WKTConstants::PARAMETERFILE,
                             !parameter()->identifiers().empty());
    } else {
        formatter->startNode(io::WKTConstants::PARAMETER,
                             !parameter()->identifiers().empty());
    }
    if (paramMapping) {
        formatter->addQuotedString(paramMapping->wkt1_name);
    } else {
        formatter->addQuotedString(parameter()->nameStr());
    }
    parameterValue()->_exportToWKT(formatter);
    if (formatter->outputId()) {
        parameter()->formatID(formatter);
    }
    formatter->endNode();
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void OperationParameterValue::_exportToJSON(
    io::JSONFormatter *formatter) const {
    auto writer = formatter->writer();
    auto objectContext(formatter->MakeObjectContext(
        "ParameterValue", !parameter()->identifiers().empty()));

    writer->AddObjKey("name");
    writer->Add(parameter()->nameStr());

    const auto &l_value(parameterValue());
    if (l_value->type() == ParameterValue::Type::MEASURE) {
        writer->AddObjKey("value");
        writer->Add(l_value->value().value(), 15);
        writer->AddObjKey("unit");
        const auto &l_unit(l_value->value().unit());
        if (l_unit == common::UnitOfMeasure::METRE ||
            l_unit == common::UnitOfMeasure::DEGREE ||
            l_unit == common::UnitOfMeasure::SCALE_UNITY) {
            writer->Add(l_unit.name());
        } else {
            l_unit._exportToJSON(formatter);
        }
    } else if (l_value->type() == ParameterValue::Type::FILENAME) {
        writer->AddObjKey("value");
        writer->Add(l_value->valueFile());
    }

    if (formatter->outputId()) {
        parameter()->formatID(formatter);
    }
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress

/** Utility method used on WKT2 import to convert from abridged transformation
 * to "normal" transformation parameters.
 */
bool OperationParameterValue::convertFromAbridged(
    const std::string &paramName, double &val,
    const common::UnitOfMeasure *&unit, int &paramEPSGCode) {
    if (metadata::Identifier::isEquivalentName(
            paramName.c_str(), EPSG_NAME_PARAMETER_X_AXIS_TRANSLATION) ||
        paramEPSGCode == EPSG_CODE_PARAMETER_X_AXIS_TRANSLATION) {
        unit = &common::UnitOfMeasure::METRE;
        paramEPSGCode = EPSG_CODE_PARAMETER_X_AXIS_TRANSLATION;
        return true;
    } else if (metadata::Identifier::isEquivalentName(
                   paramName.c_str(), EPSG_NAME_PARAMETER_Y_AXIS_TRANSLATION) ||
               paramEPSGCode == EPSG_CODE_PARAMETER_Y_AXIS_TRANSLATION) {
        unit = &common::UnitOfMeasure::METRE;
        paramEPSGCode = EPSG_CODE_PARAMETER_Y_AXIS_TRANSLATION;
        return true;
    } else if (metadata::Identifier::isEquivalentName(
                   paramName.c_str(), EPSG_NAME_PARAMETER_Z_AXIS_TRANSLATION) ||
               paramEPSGCode == EPSG_CODE_PARAMETER_Z_AXIS_TRANSLATION) {
        unit = &common::UnitOfMeasure::METRE;
        paramEPSGCode = EPSG_CODE_PARAMETER_Z_AXIS_TRANSLATION;
        return true;
    } else if (metadata::Identifier::isEquivalentName(
                   paramName.c_str(), EPSG_NAME_PARAMETER_X_AXIS_ROTATION) ||
               paramEPSGCode == EPSG_CODE_PARAMETER_X_AXIS_ROTATION) {
        unit = &common::UnitOfMeasure::ARC_SECOND;
        paramEPSGCode = EPSG_CODE_PARAMETER_X_AXIS_ROTATION;
        return true;
    } else if (metadata::Identifier::isEquivalentName(
                   paramName.c_str(), EPSG_NAME_PARAMETER_Y_AXIS_ROTATION) ||
               paramEPSGCode == EPSG_CODE_PARAMETER_Y_AXIS_ROTATION) {
        unit = &common::UnitOfMeasure::ARC_SECOND;
        paramEPSGCode = EPSG_CODE_PARAMETER_Y_AXIS_ROTATION;
        return true;

    } else if (metadata::Identifier::isEquivalentName(
                   paramName.c_str(), EPSG_NAME_PARAMETER_Z_AXIS_ROTATION) ||
               paramEPSGCode == EPSG_CODE_PARAMETER_Z_AXIS_ROTATION) {
        unit = &common::UnitOfMeasure::ARC_SECOND;
        paramEPSGCode = EPSG_CODE_PARAMETER_Z_AXIS_ROTATION;
        return true;

    } else if (metadata::Identifier::isEquivalentName(
                   paramName.c_str(), EPSG_NAME_PARAMETER_SCALE_DIFFERENCE) ||
               paramEPSGCode == EPSG_CODE_PARAMETER_SCALE_DIFFERENCE) {
        val = (val - 1.0) * 1e6;
        unit = &common::UnitOfMeasure::PARTS_PER_MILLION;
        paramEPSGCode = EPSG_CODE_PARAMETER_SCALE_DIFFERENCE;
        return true;
    }
    return false;
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
bool OperationParameterValue::_isEquivalentTo(
    const util::IComparable *other, util::IComparable::Criterion criterion,
    const io::DatabaseContextPtr &dbContext) const {
    auto otherOPV = dynamic_cast<const OperationParameterValue *>(other);
    if (otherOPV == nullptr) {
        return false;
    }
    if (!d->parameter->_isEquivalentTo(otherOPV->d->parameter.get(), criterion,
                                       dbContext)) {
        return false;
    }
    if (criterion == util::IComparable::Criterion::STRICT) {
        return d->parameterValue->_isEquivalentTo(
            otherOPV->d->parameterValue.get(), criterion);
    }
    if (d->parameterValue->_isEquivalentTo(otherOPV->d->parameterValue.get(),
                                           criterion, dbContext)) {
        return true;
    }
    if (d->parameter->getEPSGCode() ==
            EPSG_CODE_PARAMETER_AZIMUTH_INITIAL_LINE ||
        d->parameter->getEPSGCode() ==
            EPSG_CODE_PARAMETER_ANGLE_RECTIFIED_TO_SKEW_GRID) {
        if (parameterValue()->type() == ParameterValue::Type::MEASURE &&
            otherOPV->parameterValue()->type() ==
                ParameterValue::Type::MEASURE) {
            const double a = std::fmod(parameterValue()->value().convertToUnit(
                                           common::UnitOfMeasure::DEGREE) +
                                           360.0,
                                       360.0);
            const double b =
                std::fmod(otherOPV->parameterValue()->value().convertToUnit(
                              common::UnitOfMeasure::DEGREE) +
                              360.0,
                          360.0);
            return std::fabs(a - b) <= 1e-10 * std::fabs(a);
        }
    }
    return false;
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct GeneralOperationParameter::Private {};
//! @endcond

// ---------------------------------------------------------------------------

GeneralOperationParameter::GeneralOperationParameter() : d(nullptr) {}

// ---------------------------------------------------------------------------

GeneralOperationParameter::GeneralOperationParameter(
    const GeneralOperationParameter &other)
    : IdentifiedObject(other), d(nullptr) {}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
GeneralOperationParameter::~GeneralOperationParameter() = default;
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct OperationParameter::Private {};
//! @endcond

// ---------------------------------------------------------------------------

OperationParameter::OperationParameter() : d(nullptr) {}

// ---------------------------------------------------------------------------

OperationParameter::OperationParameter(const OperationParameter &other)
    : GeneralOperationParameter(other), d(nullptr) {}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
OperationParameter::~OperationParameter() = default;
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Instantiate a OperationParameter.
 *
 * @param properties See \ref general_properties. At minimum the name should be
 * defined.
 * @return a new OperationParameter.
 */
OperationParameterNNPtr
OperationParameter::create(const util::PropertyMap &properties) {
    OperationParameterNNPtr op(
        OperationParameter::nn_make_shared<OperationParameter>());
    op->assignSelf(op);
    op->setProperties(properties);
    return op;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
bool OperationParameter::_isEquivalentTo(
    const util::IComparable *other, util::IComparable::Criterion criterion,
    const io::DatabaseContextPtr &dbContext) const {
    auto otherOP = dynamic_cast<const OperationParameter *>(other);
    if (otherOP == nullptr) {
        return false;
    }
    if (criterion == util::IComparable::Criterion::STRICT) {
        return IdentifiedObject::_isEquivalentTo(other, criterion, dbContext);
    }
    if (IdentifiedObject::_isEquivalentTo(other, criterion, dbContext)) {
        return true;
    }
    auto l_epsgCode = getEPSGCode();
    return l_epsgCode != 0 && l_epsgCode == otherOP->getEPSGCode();
}
//! @endcond

// ---------------------------------------------------------------------------

void OperationParameter::_exportToWKT(io::WKTFormatter *) const {}

// ---------------------------------------------------------------------------

/** \brief Return the name of a parameter designed by its EPSG code
 * @return name, or nullptr if not found
 */
const char *OperationParameter::getNameForEPSGCode(int epsg_code) noexcept {
    size_t nParamNameCodes = 0;
    const auto paramNameCodes = getParamNameCodes(nParamNameCodes);
    for (size_t i = 0; i < nParamNameCodes; ++i) {
        const auto &tuple = paramNameCodes[i];
        if (tuple.epsg_code == epsg_code) {
            return tuple.name;
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------------------

/** \brief Return the EPSG code, either directly, or through the name
 * @return code, or 0 if not found
 */
int OperationParameter::getEPSGCode() PROJ_PURE_DEFN {
    int epsg_code = IdentifiedObject::getEPSGCode();
    if (epsg_code == 0) {
        const auto &l_name = nameStr();
        size_t nParamNameCodes = 0;
        const auto paramNameCodes = getParamNameCodes(nParamNameCodes);
        for (size_t i = 0; i < nParamNameCodes; ++i) {
            const auto &tuple = paramNameCodes[i];
            if (metadata::Identifier::isEquivalentName(l_name.c_str(),
                                                       tuple.name)) {
                return tuple.epsg_code;
            }
        }
        if (metadata::Identifier::isEquivalentName(l_name.c_str(),
                                                   "Latitude of origin")) {
            return EPSG_CODE_PARAMETER_LATITUDE_OF_NATURAL_ORIGIN;
        }
        if (metadata::Identifier::isEquivalentName(l_name.c_str(),
                                                   "Scale factor")) {
            return EPSG_CODE_PARAMETER_SCALE_FACTOR_AT_NATURAL_ORIGIN;
        }
    }
    return epsg_code;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct SingleOperation::Private {
    std::vector<GeneralParameterValueNNPtr> parameterValues_{};
    OperationMethodNNPtr method_;

    explicit Private(const OperationMethodNNPtr &methodIn)
        : method_(methodIn) {}
};
//! @endcond

// ---------------------------------------------------------------------------

SingleOperation::SingleOperation(const OperationMethodNNPtr &methodIn)
    : d(internal::make_unique<Private>(methodIn)) {}

// ---------------------------------------------------------------------------

SingleOperation::SingleOperation(const SingleOperation &other)
    :
#if !defined(COMPILER_WARNS_ABOUT_ABSTRACT_VBASE_INIT)
      CoordinateOperation(other),
#endif
      d(internal::make_unique<Private>(*other.d)) {
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
SingleOperation::~SingleOperation() = default;
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Return the parameter values.
 *
 * @return the parameter values.
 */
const std::vector<GeneralParameterValueNNPtr> &
SingleOperation::parameterValues() PROJ_PURE_DEFN {
    return d->parameterValues_;
}

// ---------------------------------------------------------------------------

/** \brief Return the operation method associated to the operation.
 *
 * @return the operation method.
 */
const OperationMethodNNPtr &SingleOperation::method() PROJ_PURE_DEFN {
    return d->method_;
}

// ---------------------------------------------------------------------------

void SingleOperation::setParameterValues(
    const std::vector<GeneralParameterValueNNPtr> &values) {
    d->parameterValues_ = values;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
static const ParameterValuePtr nullParameterValue;
//! @endcond

/** \brief Return the parameter value corresponding to a parameter name or
 * EPSG code
 *
 * @param paramName the parameter name (or empty, in which case epsg_code
 *                  should be non zero)
 * @param epsg_code the parameter EPSG code (possibly zero)
 * @return the value, or nullptr if not found.
 */
const ParameterValuePtr &
SingleOperation::parameterValue(const std::string &paramName,
                                int epsg_code) const noexcept {
    if (epsg_code) {
        for (const auto &genOpParamvalue : parameterValues()) {
            auto opParamvalue = dynamic_cast<const OperationParameterValue *>(
                genOpParamvalue.get());
            if (opParamvalue) {
                const auto &parameter = opParamvalue->parameter();
                if (parameter->getEPSGCode() == epsg_code) {
                    return opParamvalue->parameterValue();
                }
            }
        }
    }
    for (const auto &genOpParamvalue : parameterValues()) {
        auto opParamvalue = dynamic_cast<const OperationParameterValue *>(
            genOpParamvalue.get());
        if (opParamvalue) {
            const auto &parameter = opParamvalue->parameter();
            if (metadata::Identifier::isEquivalentName(
                    paramName.c_str(), parameter->nameStr().c_str())) {
                return opParamvalue->parameterValue();
            }
        }
    }
    for (const auto &genOpParamvalue : parameterValues()) {
        auto opParamvalue = dynamic_cast<const OperationParameterValue *>(
            genOpParamvalue.get());
        if (opParamvalue) {
            const auto &parameter = opParamvalue->parameter();
            if (areEquivalentParameters(paramName, parameter->nameStr())) {
                return opParamvalue->parameterValue();
            }
        }
    }
    return nullParameterValue;
}

// ---------------------------------------------------------------------------

/** \brief Return the parameter value corresponding to a EPSG code
 *
 * @param epsg_code the parameter EPSG code
 * @return the value, or nullptr if not found.
 */
const ParameterValuePtr &
SingleOperation::parameterValue(int epsg_code) const noexcept {
    for (const auto &genOpParamvalue : parameterValues()) {
        auto opParamvalue = dynamic_cast<const OperationParameterValue *>(
            genOpParamvalue.get());
        if (opParamvalue) {
            const auto &parameter = opParamvalue->parameter();
            if (parameter->getEPSGCode() == epsg_code) {
                return opParamvalue->parameterValue();
            }
        }
    }
    return nullParameterValue;
}

// ---------------------------------------------------------------------------

/** \brief Return the parameter value, as a measure, corresponding to a
 * parameter name or EPSG code
 *
 * @param paramName the parameter name (or empty, in which case epsg_code
 *                  should be non zero)
 * @param epsg_code the parameter EPSG code (possibly zero)
 * @return the measure, or the empty Measure() object if not found.
 */
const common::Measure &
SingleOperation::parameterValueMeasure(const std::string &paramName,
                                       int epsg_code) const noexcept {
    const auto &val = parameterValue(paramName, epsg_code);
    if (val && val->type() == ParameterValue::Type::MEASURE) {
        return val->value();
    }
    return nullMeasure;
}

/** \brief Return the parameter value, as a measure, corresponding to a
 * EPSG code
 *
 * @param epsg_code the parameter EPSG code
 * @return the measure, or the empty Measure() object if not found.
 */
const common::Measure &
SingleOperation::parameterValueMeasure(int epsg_code) const noexcept {
    const auto &val = parameterValue(epsg_code);
    if (val && val->type() == ParameterValue::Type::MEASURE) {
        return val->value();
    }
    return nullMeasure;
}

//! @cond Doxygen_Suppress

double
SingleOperation::parameterValueNumericAsSI(int epsg_code) const noexcept {
    const auto &val = parameterValue(epsg_code);
    if (val && val->type() == ParameterValue::Type::MEASURE) {
        return val->value().getSIValue();
    }
    return 0.0;
}

double SingleOperation::parameterValueNumeric(
    int epsg_code, const common::UnitOfMeasure &targetUnit) const noexcept {
    const auto &val = parameterValue(epsg_code);
    if (val && val->type() == ParameterValue::Type::MEASURE) {
        return val->value().convertToUnit(targetUnit);
    }
    return 0.0;
}

double SingleOperation::parameterValueNumeric(
    const char *param_name,
    const common::UnitOfMeasure &targetUnit) const noexcept {
    const auto &val = parameterValue(param_name, 0);
    if (val && val->type() == ParameterValue::Type::MEASURE) {
        return val->value().convertToUnit(targetUnit);
    }
    return 0.0;
}

//! @endcond
// ---------------------------------------------------------------------------

/** \brief Instantiate a PROJ-based single operation.
 *
 * \note The operation might internally be a pipeline chaining several
 * operations.
 * The use of the SingleOperation modeling here is mostly to be able to get
 * the PROJ string as a parameter.
 *
 * @param properties Properties
 * @param PROJString the PROJ string.
 * @param sourceCRS source CRS (might be null).
 * @param targetCRS target CRS (might be null).
 * @param accuracies Vector of positional accuracy (might be empty).
 * @return the new instance
 */
SingleOperationNNPtr SingleOperation::createPROJBased(
    const util::PropertyMap &properties, const std::string &PROJString,
    const crs::CRSPtr &sourceCRS, const crs::CRSPtr &targetCRS,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {
    return util::nn_static_pointer_cast<SingleOperation>(
        PROJBasedOperation::create(properties, PROJString, sourceCRS, targetCRS,
                                   accuracies));
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
bool SingleOperation::_isEquivalentTo(
    const util::IComparable *other, util::IComparable::Criterion criterion,
    const io::DatabaseContextPtr &dbContext) const {
    return _isEquivalentTo(other, criterion, dbContext, false);
}

bool SingleOperation::_isEquivalentTo(const util::IComparable *other,
                                      util::IComparable::Criterion criterion,
                                      const io::DatabaseContextPtr &dbContext,
                                      bool inOtherDirection) const {

    auto otherSO = dynamic_cast<const SingleOperation *>(other);
    if (otherSO == nullptr ||
        (criterion == util::IComparable::Criterion::STRICT &&
         !ObjectUsage::_isEquivalentTo(other, criterion, dbContext))) {
        return false;
    }

    const int methodEPSGCode = d->method_->getEPSGCode();
    const int otherMethodEPSGCode = otherSO->d->method_->getEPSGCode();

    bool equivalentMethods =
        (criterion == util::IComparable::Criterion::EQUIVALENT &&
         methodEPSGCode != 0 && methodEPSGCode == otherMethodEPSGCode) ||
        d->method_->_isEquivalentTo(otherSO->d->method_.get(), criterion,
                                    dbContext);
    if (!equivalentMethods &&
        criterion == util::IComparable::Criterion::EQUIVALENT) {
        if ((methodEPSGCode == EPSG_CODE_METHOD_LAMBERT_AZIMUTHAL_EQUAL_AREA &&
             otherMethodEPSGCode ==
                 EPSG_CODE_METHOD_LAMBERT_AZIMUTHAL_EQUAL_AREA_SPHERICAL) ||
            (otherMethodEPSGCode ==
                 EPSG_CODE_METHOD_LAMBERT_AZIMUTHAL_EQUAL_AREA &&
             methodEPSGCode ==
                 EPSG_CODE_METHOD_LAMBERT_AZIMUTHAL_EQUAL_AREA_SPHERICAL) ||
            (methodEPSGCode ==
                 EPSG_CODE_METHOD_LAMBERT_CYLINDRICAL_EQUAL_AREA &&
             otherMethodEPSGCode ==
                 EPSG_CODE_METHOD_LAMBERT_CYLINDRICAL_EQUAL_AREA_SPHERICAL) ||
            (otherMethodEPSGCode ==
                 EPSG_CODE_METHOD_LAMBERT_CYLINDRICAL_EQUAL_AREA &&
             methodEPSGCode ==
                 EPSG_CODE_METHOD_LAMBERT_CYLINDRICAL_EQUAL_AREA_SPHERICAL) ||
            (methodEPSGCode == EPSG_CODE_METHOD_EQUIDISTANT_CYLINDRICAL &&
             otherMethodEPSGCode ==
                 EPSG_CODE_METHOD_EQUIDISTANT_CYLINDRICAL_SPHERICAL) ||
            (otherMethodEPSGCode == EPSG_CODE_METHOD_EQUIDISTANT_CYLINDRICAL &&
             methodEPSGCode ==
                 EPSG_CODE_METHOD_EQUIDISTANT_CYLINDRICAL_SPHERICAL)) {
            auto geodCRS =
                dynamic_cast<const crs::GeodeticCRS *>(sourceCRS().get());
            auto otherGeodCRS = dynamic_cast<const crs::GeodeticCRS *>(
                otherSO->sourceCRS().get());
            if (geodCRS && otherGeodCRS && geodCRS->ellipsoid()->isSphere() &&
                otherGeodCRS->ellipsoid()->isSphere()) {
                equivalentMethods = true;
            }
        }
    }

    if (!equivalentMethods) {
        if (criterion == util::IComparable::Criterion::EQUIVALENT) {

            const auto isTOWGS84Transf = [](int code) {
                return code ==
                           EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_GEOCENTRIC ||
                       code == EPSG_CODE_METHOD_POSITION_VECTOR_GEOCENTRIC ||
                       code == EPSG_CODE_METHOD_COORDINATE_FRAME_GEOCENTRIC ||
                       code ==
                           EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_GEOGRAPHIC_2D ||
                       code == EPSG_CODE_METHOD_POSITION_VECTOR_GEOGRAPHIC_2D ||
                       code ==
                           EPSG_CODE_METHOD_COORDINATE_FRAME_GEOGRAPHIC_2D ||
                       code ==
                           EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_GEOGRAPHIC_3D ||
                       code == EPSG_CODE_METHOD_POSITION_VECTOR_GEOGRAPHIC_3D ||
                       code == EPSG_CODE_METHOD_COORDINATE_FRAME_GEOGRAPHIC_3D;
            };

            // Translation vs (PV or CF)
            // or different PV vs CF convention
            if (isTOWGS84Transf(methodEPSGCode) &&
                isTOWGS84Transf(otherMethodEPSGCode)) {
                auto transf = static_cast<const Transformation *>(this);
                auto otherTransf = static_cast<const Transformation *>(otherSO);
                auto params = transf->getTOWGS84Parameters();
                auto otherParams = otherTransf->getTOWGS84Parameters();
                assert(params.size() == 7);
                assert(otherParams.size() == 7);
                for (size_t i = 0; i < 7; i++) {
                    if (std::fabs(params[i] - otherParams[i]) >
                        1e-10 * std::fabs(params[i])) {
                        return false;
                    }
                }
                return true;
            }

            // _1SP methods can sometimes be equivalent to _2SP ones
            // Check it by using convertToOtherMethod()
            if (methodEPSGCode ==
                    EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_1SP &&
                otherMethodEPSGCode ==
                    EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_2SP) {
                // Convert from 2SP to 1SP as the other direction has more
                // degree of liberties.
                return otherSO->_isEquivalentTo(this, criterion, dbContext);
            } else if ((methodEPSGCode == EPSG_CODE_METHOD_MERCATOR_VARIANT_A &&
                        otherMethodEPSGCode ==
                            EPSG_CODE_METHOD_MERCATOR_VARIANT_B) ||
                       (methodEPSGCode == EPSG_CODE_METHOD_MERCATOR_VARIANT_B &&
                        otherMethodEPSGCode ==
                            EPSG_CODE_METHOD_MERCATOR_VARIANT_A) ||
                       (methodEPSGCode ==
                            EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_2SP &&
                        otherMethodEPSGCode ==
                            EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_1SP)) {
                auto conv = dynamic_cast<const Conversion *>(this);
                if (conv) {
                    auto eqConv =
                        conv->convertToOtherMethod(otherMethodEPSGCode);
                    if (eqConv) {
                        return eqConv->_isEquivalentTo(other, criterion,
                                                       dbContext);
                    }
                }
            }
        }

        return false;
    }

    const auto &values = d->parameterValues_;
    const auto &otherValues = otherSO->d->parameterValues_;
    const auto valuesSize = values.size();
    const auto otherValuesSize = otherValues.size();
    if (criterion == util::IComparable::Criterion::STRICT) {
        if (valuesSize != otherValuesSize) {
            return false;
        }
        for (size_t i = 0; i < valuesSize; i++) {
            if (!values[i]->_isEquivalentTo(otherValues[i].get(), criterion,
                                            dbContext)) {
                return false;
            }
        }
        return true;
    }

    std::vector<bool> candidateIndices(otherValuesSize, true);
    bool equivalent = true;
    bool foundMissingArgs = valuesSize != otherValuesSize;

    for (size_t i = 0; equivalent && i < valuesSize; i++) {
        auto opParamvalue =
            dynamic_cast<const OperationParameterValue *>(values[i].get());
        if (!opParamvalue)
            return false;

        equivalent = false;
        bool sameNameDifferentValue = false;
        for (size_t j = 0; j < otherValuesSize; j++) {
            if (candidateIndices[j] &&
                values[i]->_isEquivalentTo(otherValues[j].get(), criterion,
                                           dbContext)) {
                candidateIndices[j] = false;
                equivalent = true;
                break;
            } else if (candidateIndices[j]) {
                auto otherOpParamvalue =
                    dynamic_cast<const OperationParameterValue *>(
                        otherValues[j].get());
                if (!otherOpParamvalue)
                    return false;
                sameNameDifferentValue =
                    opParamvalue->parameter()->_isEquivalentTo(
                        otherOpParamvalue->parameter().get(), criterion,
                        dbContext);
                if (sameNameDifferentValue) {
                    candidateIndices[j] = false;
                    break;
                }
            }
        }

        if (!equivalent &&
            methodEPSGCode == EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_2SP) {
            // For LCC_2SP, the standard parallels can be switched and
            // this will result in the same result.
            const int paramEPSGCode = opParamvalue->parameter()->getEPSGCode();
            if (paramEPSGCode ==
                    EPSG_CODE_PARAMETER_LATITUDE_1ST_STD_PARALLEL ||
                paramEPSGCode ==
                    EPSG_CODE_PARAMETER_LATITUDE_2ND_STD_PARALLEL) {
                auto value_1st = parameterValue(
                    EPSG_CODE_PARAMETER_LATITUDE_1ST_STD_PARALLEL);
                auto value_2nd = parameterValue(
                    EPSG_CODE_PARAMETER_LATITUDE_2ND_STD_PARALLEL);
                if (value_1st && value_2nd) {
                    equivalent =
                        value_1st->_isEquivalentTo(
                            otherSO
                                ->parameterValue(
                                    EPSG_CODE_PARAMETER_LATITUDE_2ND_STD_PARALLEL)
                                .get(),
                            criterion, dbContext) &&
                        value_2nd->_isEquivalentTo(
                            otherSO
                                ->parameterValue(
                                    EPSG_CODE_PARAMETER_LATITUDE_1ST_STD_PARALLEL)
                                .get(),
                            criterion, dbContext);
                }
            }
        }

        if (equivalent) {
            continue;
        }

        if (sameNameDifferentValue) {
            break;
        }

        // If there are parameters in this method not found in the other one,
        // check that they are set to a default neutral value, that is 1
        // for scale, and 0 otherwise.
        foundMissingArgs = true;
        const auto &value = opParamvalue->parameterValue();
        if (value->type() != ParameterValue::Type::MEASURE) {
            break;
        }
        if (value->value().unit().type() ==
            common::UnitOfMeasure::Type::SCALE) {
            equivalent = value->value().getSIValue() == 1.0;
        } else {
            equivalent = value->value().getSIValue() == 0.0;
        }
    }

    // In the case the arguments don't perfectly match, try the reverse
    // check.
    if (equivalent && foundMissingArgs && !inOtherDirection) {
        return otherSO->_isEquivalentTo(this, criterion, dbContext, true);
    }

    // Equivalent formulations of 2SP can have different parameters
    // Then convert to 1SP and compare.
    if (!equivalent &&
        methodEPSGCode == EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_2SP) {
        auto conv = dynamic_cast<const Conversion *>(this);
        auto otherConv = dynamic_cast<const Conversion *>(other);
        if (conv && otherConv) {
            auto thisAs1SP = conv->convertToOtherMethod(
                EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_1SP);
            auto otherAs1SP = otherConv->convertToOtherMethod(
                EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_1SP);
            if (thisAs1SP && otherAs1SP) {
                equivalent = thisAs1SP->_isEquivalentTo(otherAs1SP.get(),
                                                        criterion, dbContext);
            }
        }
    }
    return equivalent;
}
//! @endcond

// ---------------------------------------------------------------------------

std::set<GridDescription>
SingleOperation::gridsNeeded(const io::DatabaseContextPtr &databaseContext,
                             bool considerKnownGridsAsAvailable) const {
    std::set<GridDescription> res;
    for (const auto &genOpParamvalue : parameterValues()) {
        auto opParamvalue = dynamic_cast<const OperationParameterValue *>(
            genOpParamvalue.get());
        if (opParamvalue) {
            const auto &value = opParamvalue->parameterValue();
            if (value->type() == ParameterValue::Type::FILENAME) {
                const auto gridNames = split(value->valueFile(), ",");
                for (const auto &gridName : gridNames) {
                    GridDescription desc;
                    desc.shortName = gridName;
                    if (databaseContext) {
                        databaseContext->lookForGridInfo(
                            desc.shortName, considerKnownGridsAsAvailable,
                            desc.fullName, desc.packageName, desc.url,
                            desc.directDownload, desc.openLicense,
                            desc.available);
                    }
                    res.insert(desc);
                }
            }
        }
    }
    return res;
}

// ---------------------------------------------------------------------------

/** \brief Validate the parameters used by a coordinate operation.
 *
 * Return whether the method is known or not, or a list of missing or extra
 * parameters for the operations recognized by this implementation.
 */
std::list<std::string> SingleOperation::validateParameters() const {
    std::list<std::string> res;

    const auto &l_method = method();
    const auto &methodName = l_method->nameStr();
    const auto methodEPSGCode = l_method->getEPSGCode();

    const auto findMapping = [methodEPSGCode, &methodName](
                                 const MethodMapping *mappings,
                                 size_t mappingCount) -> const MethodMapping * {
        if (methodEPSGCode != 0) {
            for (size_t i = 0; i < mappingCount; ++i) {
                const auto &mapping = mappings[i];
                if (methodEPSGCode == mapping.epsg_code) {
                    return &mapping;
                }
            }
        }
        for (size_t i = 0; i < mappingCount; ++i) {
            const auto &mapping = mappings[i];
            if (metadata::Identifier::isEquivalentName(mapping.wkt2_name,
                                                       methodName.c_str())) {
                return &mapping;
            }
        }
        return nullptr;
    };

    size_t nProjectionMethodMappings = 0;
    const auto projectionMethodMappings =
        getProjectionMethodMappings(nProjectionMethodMappings);
    const MethodMapping *methodMapping =
        findMapping(projectionMethodMappings, nProjectionMethodMappings);
    if (methodMapping == nullptr) {
        size_t nOtherMethodMappings = 0;
        const auto otherMethodMappings =
            getOtherMethodMappings(nOtherMethodMappings);
        methodMapping = findMapping(otherMethodMappings, nOtherMethodMappings);
    }
    if (!methodMapping) {
        res.emplace_back("Unknown method " + methodName);
        return res;
    }
    if (methodMapping->wkt2_name != methodName) {
        if (metadata::Identifier::isEquivalentName(methodMapping->wkt2_name,
                                                   methodName.c_str())) {
            std::string msg("Method name ");
            msg += methodName;
            msg += " is equivalent to official ";
            msg += methodMapping->wkt2_name;
            msg += " but not strictly equal";
            res.emplace_back(msg);
        } else {
            std::string msg("Method name ");
            msg += methodName;
            msg += ", matched to ";
            msg += methodMapping->wkt2_name;
            msg += ", through its EPSG code has not an equivalent name";
            res.emplace_back(msg);
        }
    }
    if (methodEPSGCode != 0 && methodEPSGCode != methodMapping->epsg_code) {
        std::string msg("Method of EPSG code ");
        msg += toString(methodEPSGCode);
        msg += " does not match official code (";
        msg += toString(methodMapping->epsg_code);
        msg += ')';
        res.emplace_back(msg);
    }

    // Check if expected parameters are found
    for (int i = 0;
         methodMapping->params && methodMapping->params[i] != nullptr; ++i) {
        const auto *paramMapping = methodMapping->params[i];

        const OperationParameterValue *opv = nullptr;
        for (const auto &genOpParamvalue : parameterValues()) {
            auto opParamvalue = dynamic_cast<const OperationParameterValue *>(
                genOpParamvalue.get());
            if (opParamvalue) {
                const auto &parameter = opParamvalue->parameter();
                if ((paramMapping->epsg_code != 0 &&
                     parameter->getEPSGCode() == paramMapping->epsg_code) ||
                    ci_equal(parameter->nameStr(), paramMapping->wkt2_name)) {
                    opv = opParamvalue;
                    break;
                }
            }
        }

        if (!opv) {
            if ((methodEPSGCode == EPSG_CODE_METHOD_EQUIDISTANT_CYLINDRICAL ||
                 methodEPSGCode ==
                     EPSG_CODE_METHOD_EQUIDISTANT_CYLINDRICAL_SPHERICAL) &&
                paramMapping == &paramLatitudeNatOrigin) {
                // extension of EPSG used by GDAL/PROJ, so we should not
                // warn on its absence.
                continue;
            }
            std::string msg("Cannot find expected parameter ");
            msg += paramMapping->wkt2_name;
            res.emplace_back(msg);
            continue;
        }
        const auto &parameter = opv->parameter();
        if (paramMapping->wkt2_name != parameter->nameStr()) {
            if (ci_equal(parameter->nameStr(), paramMapping->wkt2_name)) {
                std::string msg("Parameter name ");
                msg += parameter->nameStr();
                msg += " is equivalent to official ";
                msg += paramMapping->wkt2_name;
                msg += " but not strictly equal";
                res.emplace_back(msg);
            } else {
                std::string msg("Parameter name ");
                msg += parameter->nameStr();
                msg += ", matched to ";
                msg += paramMapping->wkt2_name;
                msg += ", through its EPSG code has not an equivalent name";
                res.emplace_back(msg);
            }
        }
        const auto paramEPSGCode = parameter->getEPSGCode();
        if (paramEPSGCode != 0 && paramEPSGCode != paramMapping->epsg_code) {
            std::string msg("Parameter of EPSG code ");
            msg += toString(paramEPSGCode);
            msg += " does not match official code (";
            msg += toString(paramMapping->epsg_code);
            msg += ')';
            res.emplace_back(msg);
        }
    }

    // Check if there are extra parameters
    for (const auto &genOpParamvalue : parameterValues()) {
        auto opParamvalue = dynamic_cast<const OperationParameterValue *>(
            genOpParamvalue.get());
        if (opParamvalue) {
            const auto &parameter = opParamvalue->parameter();
            if (!getMapping(methodMapping, parameter)) {
                std::string msg("Parameter ");
                msg += parameter->nameStr();
                msg += " found but not expected for this method";
                res.emplace_back(msg);
            }
        }
    }

    return res;
} // namespace operation

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct ParameterValue::Private {
    ParameterValue::Type type_{ParameterValue::Type::STRING};
    std::unique_ptr<common::Measure> measure_{};
    std::unique_ptr<std::string> stringValue_{};
    int integerValue_{};
    bool booleanValue_{};

    explicit Private(const common::Measure &valueIn)
        : type_(ParameterValue::Type::MEASURE),
          measure_(internal::make_unique<common::Measure>(valueIn)) {}

    Private(const std::string &stringValueIn, ParameterValue::Type typeIn)
        : type_(typeIn),
          stringValue_(internal::make_unique<std::string>(stringValueIn)) {}

    explicit Private(int integerValueIn)
        : type_(ParameterValue::Type::INTEGER), integerValue_(integerValueIn) {}

    explicit Private(bool booleanValueIn)
        : type_(ParameterValue::Type::BOOLEAN), booleanValue_(booleanValueIn) {}
};
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
ParameterValue::~ParameterValue() = default;
//! @endcond

// ---------------------------------------------------------------------------

ParameterValue::ParameterValue(const common::Measure &measureIn)
    : d(internal::make_unique<Private>(measureIn)) {}

// ---------------------------------------------------------------------------

ParameterValue::ParameterValue(const std::string &stringValueIn,
                               ParameterValue::Type typeIn)
    : d(internal::make_unique<Private>(stringValueIn, typeIn)) {}

// ---------------------------------------------------------------------------

ParameterValue::ParameterValue(int integerValueIn)
    : d(internal::make_unique<Private>(integerValueIn)) {}

// ---------------------------------------------------------------------------

ParameterValue::ParameterValue(bool booleanValueIn)
    : d(internal::make_unique<Private>(booleanValueIn)) {}

// ---------------------------------------------------------------------------

/** \brief Instantiate a ParameterValue from a Measure (i.e. a value associated
 * with a
 * unit)
 *
 * @return a new ParameterValue.
 */
ParameterValueNNPtr ParameterValue::create(const common::Measure &measureIn) {
    return ParameterValue::nn_make_shared<ParameterValue>(measureIn);
}

// ---------------------------------------------------------------------------

/** \brief Instantiate a ParameterValue from a string value.
 *
 * @return a new ParameterValue.
 */
ParameterValueNNPtr ParameterValue::create(const char *stringValueIn) {
    return ParameterValue::nn_make_shared<ParameterValue>(
        std::string(stringValueIn), ParameterValue::Type::STRING);
}

// ---------------------------------------------------------------------------

/** \brief Instantiate a ParameterValue from a string value.
 *
 * @return a new ParameterValue.
 */
ParameterValueNNPtr ParameterValue::create(const std::string &stringValueIn) {
    return ParameterValue::nn_make_shared<ParameterValue>(
        stringValueIn, ParameterValue::Type::STRING);
}

// ---------------------------------------------------------------------------

/** \brief Instantiate a ParameterValue from a filename.
 *
 * @return a new ParameterValue.
 */
ParameterValueNNPtr
ParameterValue::createFilename(const std::string &stringValueIn) {
    return ParameterValue::nn_make_shared<ParameterValue>(
        stringValueIn, ParameterValue::Type::FILENAME);
}

// ---------------------------------------------------------------------------

/** \brief Instantiate a ParameterValue from a integer value.
 *
 * @return a new ParameterValue.
 */
ParameterValueNNPtr ParameterValue::create(int integerValueIn) {
    return ParameterValue::nn_make_shared<ParameterValue>(integerValueIn);
}

// ---------------------------------------------------------------------------

/** \brief Instantiate a ParameterValue from a boolean value.
 *
 * @return a new ParameterValue.
 */
ParameterValueNNPtr ParameterValue::create(bool booleanValueIn) {
    return ParameterValue::nn_make_shared<ParameterValue>(booleanValueIn);
}

// ---------------------------------------------------------------------------

/** \brief Returns the type of a parameter value.
 *
 * @return the type.
 */
const ParameterValue::Type &ParameterValue::type() PROJ_PURE_DEFN {
    return d->type_;
}

// ---------------------------------------------------------------------------

/** \brief Returns the value as a Measure (assumes type() == Type::MEASURE)
 * @return the value as a Measure.
 */
const common::Measure &ParameterValue::value() PROJ_PURE_DEFN {
    return *d->measure_;
}

// ---------------------------------------------------------------------------

/** \brief Returns the value as a string (assumes type() == Type::STRING)
 * @return the value as a string.
 */
const std::string &ParameterValue::stringValue() PROJ_PURE_DEFN {
    return *d->stringValue_;
}

// ---------------------------------------------------------------------------

/** \brief Returns the value as a filename (assumes type() == Type::FILENAME)
 * @return the value as a filename.
 */
const std::string &ParameterValue::valueFile() PROJ_PURE_DEFN {
    return *d->stringValue_;
}

// ---------------------------------------------------------------------------

/** \brief Returns the value as a integer (assumes type() == Type::INTEGER)
 * @return the value as a integer.
 */
int ParameterValue::integerValue() PROJ_PURE_DEFN { return d->integerValue_; }

// ---------------------------------------------------------------------------

/** \brief Returns the value as a boolean (assumes type() == Type::BOOLEAN)
 * @return the value as a boolean.
 */
bool ParameterValue::booleanValue() PROJ_PURE_DEFN { return d->booleanValue_; }

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void ParameterValue::_exportToWKT(io::WKTFormatter *formatter) const {
    const bool isWKT2 = formatter->version() == io::WKTFormatter::Version::WKT2;

    const auto &l_type = type();
    if (l_type == Type::MEASURE) {
        const auto &l_value = value();
        if (formatter->abridgedTransformation()) {
            const auto &unit = l_value.unit();
            const auto &unitType = unit.type();
            if (unitType == common::UnitOfMeasure::Type::LINEAR) {
                formatter->add(l_value.getSIValue());
            } else if (unitType == common::UnitOfMeasure::Type::ANGULAR) {
                formatter->add(
                    l_value.convertToUnit(common::UnitOfMeasure::ARC_SECOND));
            } else if (unit == common::UnitOfMeasure::PARTS_PER_MILLION) {
                formatter->add(1.0 + l_value.value() * 1e-6);
            } else {
                formatter->add(l_value.value());
            }
        } else {
            const auto &unit = l_value.unit();
            if (isWKT2) {
                formatter->add(l_value.value());
            } else {
                // In WKT1, as we don't output the natural unit, output to the
                // registered linear / angular unit.
                const auto &unitType = unit.type();
                if (unitType == common::UnitOfMeasure::Type::LINEAR) {
                    const auto &targetUnit = *(formatter->axisLinearUnit());
                    if (targetUnit.conversionToSI() == 0.0) {
                        throw io::FormattingException(
                            "cannot convert value to target linear unit");
                    }
                    formatter->add(l_value.convertToUnit(targetUnit));
                } else if (unitType == common::UnitOfMeasure::Type::ANGULAR) {
                    const auto &targetUnit = *(formatter->axisAngularUnit());
                    if (targetUnit.conversionToSI() == 0.0) {
                        throw io::FormattingException(
                            "cannot convert value to target angular unit");
                    }
                    formatter->add(l_value.convertToUnit(targetUnit));
                } else {
                    formatter->add(l_value.getSIValue());
                }
            }
            if (isWKT2 && unit != common::UnitOfMeasure::NONE) {
                if (!formatter
                         ->primeMeridianOrParameterUnitOmittedIfSameAsAxis() ||
                    (unit != common::UnitOfMeasure::SCALE_UNITY &&
                     unit != *(formatter->axisLinearUnit()) &&
                     unit != *(formatter->axisAngularUnit()))) {
                    unit._exportToWKT(formatter);
                }
            }
        }
    } else if (l_type == Type::STRING || l_type == Type::FILENAME) {
        formatter->addQuotedString(stringValue());
    } else if (l_type == Type::INTEGER) {
        formatter->add(integerValue());
    } else {
        throw io::FormattingException("boolean parameter value not handled");
    }
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
bool ParameterValue::_isEquivalentTo(const util::IComparable *other,
                                     util::IComparable::Criterion criterion,
                                     const io::DatabaseContextPtr &) const {
    auto otherPV = dynamic_cast<const ParameterValue *>(other);
    if (otherPV == nullptr) {
        return false;
    }
    if (type() != otherPV->type()) {
        return false;
    }
    switch (type()) {
    case Type::MEASURE: {
        return value()._isEquivalentTo(otherPV->value(), criterion, 2e-10);
    }

    case Type::STRING:
    case Type::FILENAME: {
        return stringValue() == otherPV->stringValue();
    }

    case Type::INTEGER: {
        return integerValue() == otherPV->integerValue();
    }

    case Type::BOOLEAN: {
        return booleanValue() == otherPV->booleanValue();
    }

    default: {
        assert(false);
        break;
    }
    }
    return true;
}
//! @endcond

//! @cond Doxygen_Suppress
// ---------------------------------------------------------------------------

InvalidOperation::InvalidOperation(const char *message) : Exception(message) {}

// ---------------------------------------------------------------------------

InvalidOperation::InvalidOperation(const std::string &message)
    : Exception(message) {}

// ---------------------------------------------------------------------------

InvalidOperation::InvalidOperation(const InvalidOperation &) = default;

// ---------------------------------------------------------------------------

InvalidOperation::~InvalidOperation() = default;
//! @endcond

// ---------------------------------------------------------------------------

void SingleOperation::exportTransformationToWKT(
    io::WKTFormatter *formatter) const {
    const bool isWKT2 = formatter->version() == io::WKTFormatter::Version::WKT2;
    if (!isWKT2) {
        throw io::FormattingException(
            "Transformation can only be exported to WKT2");
    }

    if (formatter->abridgedTransformation()) {
        formatter->startNode(io::WKTConstants::ABRIDGEDTRANSFORMATION,
                             !identifiers().empty());
    } else {
        formatter->startNode(io::WKTConstants::COORDINATEOPERATION,
                             !identifiers().empty());
    }

    formatter->addQuotedString(nameStr());

    if (formatter->use2019Keywords()) {
        const auto &version = operationVersion();
        if (version.has_value()) {
            formatter->startNode(io::WKTConstants::VERSION, false);
            formatter->addQuotedString(*version);
            formatter->endNode();
        }
    }

    if (!formatter->abridgedTransformation()) {
        exportSourceCRSAndTargetCRSToWKT(this, formatter);
    }

    method()->_exportToWKT(formatter);

    for (const auto &paramValue : parameterValues()) {
        paramValue->_exportToWKT(formatter, nullptr);
    }

    if (!formatter->abridgedTransformation()) {
        if (interpolationCRS()) {
            formatter->startNode(io::WKTConstants::INTERPOLATIONCRS, false);
            interpolationCRS()->_exportToWKT(formatter);
            formatter->endNode();
        }

        if (!coordinateOperationAccuracies().empty()) {
            formatter->startNode(io::WKTConstants::OPERATIONACCURACY, false);
            formatter->add(coordinateOperationAccuracies()[0]->value());
            formatter->endNode();
        }
    }

    ObjectUsage::baseExportToWKT(formatter);
    formatter->endNode();
}

// ---------------------------------------------------------------------------

bool SingleOperation::exportToPROJStringGeneric(
    io::PROJStringFormatter *formatter) const {
    const int methodEPSGCode = method()->getEPSGCode();

    if (methodEPSGCode == EPSG_CODE_METHOD_AFFINE_PARAMETRIC_TRANSFORMATION) {
        const double A0 = parameterValueMeasure(EPSG_CODE_PARAMETER_A0).value();
        const double A1 = parameterValueMeasure(EPSG_CODE_PARAMETER_A1).value();
        const double A2 = parameterValueMeasure(EPSG_CODE_PARAMETER_A2).value();
        const double B0 = parameterValueMeasure(EPSG_CODE_PARAMETER_B0).value();
        const double B1 = parameterValueMeasure(EPSG_CODE_PARAMETER_B1).value();
        const double B2 = parameterValueMeasure(EPSG_CODE_PARAMETER_B2).value();

        // Do not mess with axis unit and order for that transformation

        formatter->addStep("affine");
        formatter->addParam("xoff", A0);
        formatter->addParam("s11", A1);
        formatter->addParam("s12", A2);
        formatter->addParam("yoff", B0);
        formatter->addParam("s21", B1);
        formatter->addParam("s22", B2);

        return true;
    }

    if (isAxisOrderReversal(methodEPSGCode)) {
        formatter->addStep("axisswap");
        formatter->addParam("order", "2,1");
        auto sourceCRSGeog =
            dynamic_cast<const crs::GeographicCRS *>(sourceCRS().get());
        auto targetCRSGeog =
            dynamic_cast<const crs::GeographicCRS *>(targetCRS().get());
        if (sourceCRSGeog && targetCRSGeog) {
            const auto &unitSrc =
                sourceCRSGeog->coordinateSystem()->axisList()[0]->unit();
            const auto &unitDst =
                targetCRSGeog->coordinateSystem()->axisList()[0]->unit();
            if (!unitSrc._isEquivalentTo(
                    unitDst, util::IComparable::Criterion::EQUIVALENT)) {
                formatter->addStep("unitconvert");
                auto projUnit = unitSrc.exportToPROJString();
                if (projUnit.empty()) {
                    formatter->addParam("xy_in", unitSrc.conversionToSI());
                } else {
                    formatter->addParam("xy_in", projUnit);
                }
                projUnit = unitDst.exportToPROJString();
                if (projUnit.empty()) {
                    formatter->addParam("xy_out", unitDst.conversionToSI());
                } else {
                    formatter->addParam("xy_out", projUnit);
                }
            }
        }
        return true;
    }

    if (methodEPSGCode == EPSG_CODE_METHOD_GEOGRAPHIC_GEOCENTRIC) {

        auto sourceCRSGeod =
            dynamic_cast<const crs::GeodeticCRS *>(sourceCRS().get());
        auto targetCRSGeod =
            dynamic_cast<const crs::GeodeticCRS *>(targetCRS().get());
        if (sourceCRSGeod && targetCRSGeod) {
            auto sourceCRSGeog =
                dynamic_cast<const crs::GeographicCRS *>(sourceCRSGeod);
            auto targetCRSGeog =
                dynamic_cast<const crs::GeographicCRS *>(targetCRSGeod);
            bool isSrcGeocentric = sourceCRSGeod->isGeocentric();
            bool isSrcGeographic = sourceCRSGeog != nullptr;
            bool isTargetGeocentric = targetCRSGeod->isGeocentric();
            bool isTargetGeographic = targetCRSGeog != nullptr;
            if ((isSrcGeocentric && isTargetGeographic) ||
                (isSrcGeographic && isTargetGeocentric)) {

                formatter->startInversion();
                sourceCRSGeod->_exportToPROJString(formatter);
                formatter->stopInversion();

                targetCRSGeod->_exportToPROJString(formatter);

                return true;
            }
        }

        throw io::FormattingException("Invalid nature of source and/or "
                                      "targetCRS for Geographic/Geocentric "
                                      "conversion");
    }

    if (methodEPSGCode == EPSG_CODE_METHOD_CHANGE_VERTICAL_UNIT) {
        double convFactor = parameterValueNumericAsSI(
            EPSG_CODE_PARAMETER_UNIT_CONVERSION_SCALAR);
        auto uom = common::UnitOfMeasure(std::string(), convFactor,
                                         common::UnitOfMeasure::Type::LINEAR)
                       .exportToPROJString();
        auto reverse_uom =
            common::UnitOfMeasure(std::string(), 1.0 / convFactor,
                                  common::UnitOfMeasure::Type::LINEAR)
                .exportToPROJString();
        if (uom == "m") {
            // do nothing
        } else if (!uom.empty()) {
            formatter->addStep("unitconvert");
            formatter->addParam("z_in", uom);
            formatter->addParam("z_out", "m");
        } else if (!reverse_uom.empty()) {
            formatter->addStep("unitconvert");
            formatter->addParam("z_in", "m");
            formatter->addParam("z_out", reverse_uom);
        } else {
            formatter->addStep("affine");
            formatter->addParam("s33", convFactor);
        }
        return true;
    }

    if (methodEPSGCode == EPSG_CODE_METHOD_HEIGHT_DEPTH_REVERSAL) {
        formatter->addStep("axisswap");
        formatter->addParam("order", "1,2,-3");
        return true;
    }

    const char *prefix = "PROJ-based operation method: ";
    if (starts_with(method()->nameStr(), prefix)) {
        auto projString = method()->nameStr().substr(strlen(prefix));
        try {
            formatter->ingestPROJString(projString);
            return true;
        } catch (const io::ParsingException &e) {
            throw io::FormattingException(
                std::string("ingestPROJString() failed: ") + e.what());
        }
    }

    return false;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress

InverseCoordinateOperation::~InverseCoordinateOperation() = default;

// ---------------------------------------------------------------------------

InverseCoordinateOperation::InverseCoordinateOperation(
    const CoordinateOperationNNPtr &forwardOperationIn,
    bool wktSupportsInversion)
    : forwardOperation_(forwardOperationIn),
      wktSupportsInversion_(wktSupportsInversion) {}

// ---------------------------------------------------------------------------

void InverseCoordinateOperation::setPropertiesFromForward() {
    setProperties(
        createPropertiesForInverse(forwardOperation_.get(), false, false));
    setAccuracies(forwardOperation_->coordinateOperationAccuracies());
    if (forwardOperation_->sourceCRS() && forwardOperation_->targetCRS()) {
        setCRSs(forwardOperation_.get(), true);
    }
    setHasBallparkTransformation(
        forwardOperation_->hasBallparkTransformation());
}

// ---------------------------------------------------------------------------

CoordinateOperationNNPtr InverseCoordinateOperation::inverse() const {
    return forwardOperation_;
}

// ---------------------------------------------------------------------------

void InverseCoordinateOperation::_exportToPROJString(
    io::PROJStringFormatter *formatter) const {
    formatter->startInversion();
    forwardOperation_->_exportToPROJString(formatter);
    formatter->stopInversion();
}

// ---------------------------------------------------------------------------

bool InverseCoordinateOperation::_isEquivalentTo(
    const util::IComparable *other, util::IComparable::Criterion criterion,
    const io::DatabaseContextPtr &dbContext) const {
    auto otherICO = dynamic_cast<const InverseCoordinateOperation *>(other);
    if (otherICO == nullptr ||
        !ObjectUsage::_isEquivalentTo(other, criterion, dbContext)) {
        return false;
    }
    return inverse()->_isEquivalentTo(otherICO->inverse().get(), criterion,
                                      dbContext);
}

//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
PointMotionOperation::~PointMotionOperation() = default;
//! @endcond

// ---------------------------------------------------------------------------

} // namespace operation

NS_PROJ_END
