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

#include "coordinateoperation_internal.hpp"
#include "coordinateoperation_private.hpp"
#include "esriparammappings.hpp"
#include "operationmethod_private.hpp"
#include "oputils.hpp"
#include "parammappings.hpp"
#include "vectorofvaluesparams.hpp"

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
struct Transformation::Private {

    TransformationPtr forwardOperation_{};

    static TransformationNNPtr registerInv(const Transformation *thisIn,
                                           TransformationNNPtr invTransform);
};
//! @endcond

// ---------------------------------------------------------------------------

Transformation::Transformation(
    const crs::CRSNNPtr &sourceCRSIn, const crs::CRSNNPtr &targetCRSIn,
    const crs::CRSPtr &interpolationCRSIn, const OperationMethodNNPtr &methodIn,
    const std::vector<GeneralParameterValueNNPtr> &values,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies)
    : SingleOperation(methodIn), d(internal::make_unique<Private>()) {
    setParameterValues(values);
    setCRSs(sourceCRSIn, targetCRSIn, interpolationCRSIn);
    setAccuracies(accuracies);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
Transformation::~Transformation() = default;
//! @endcond

// ---------------------------------------------------------------------------

Transformation::Transformation(const Transformation &other)
    : CoordinateOperation(other), SingleOperation(other),
      d(internal::make_unique<Private>(*other.d)) {}

// ---------------------------------------------------------------------------

/** \brief Return the source crs::CRS of the transformation.
 *
 * @return the source CRS.
 */
const crs::CRSNNPtr &Transformation::sourceCRS() PROJ_PURE_DEFN {
    return CoordinateOperation::getPrivate()->strongRef_->sourceCRS_;
}

// ---------------------------------------------------------------------------

/** \brief Return the target crs::CRS of the transformation.
 *
 * @return the target CRS.
 */
const crs::CRSNNPtr &Transformation::targetCRS() PROJ_PURE_DEFN {
    return CoordinateOperation::getPrivate()->strongRef_->targetCRS_;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
TransformationNNPtr Transformation::shallowClone() const {
    auto transf = Transformation::nn_make_shared<Transformation>(*this);
    transf->assignSelf(transf);
    transf->setCRSs(this, false);
    if (transf->d->forwardOperation_) {
        transf->d->forwardOperation_ =
            transf->d->forwardOperation_->shallowClone().as_nullable();
    }
    return transf;
}

CoordinateOperationNNPtr Transformation::_shallowClone() const {
    return util::nn_static_pointer_cast<CoordinateOperation>(shallowClone());
}

// ---------------------------------------------------------------------------

TransformationNNPtr
Transformation::promoteTo3D(const std::string &,
                            const io::DatabaseContextPtr &dbContext) const {
    auto transf = shallowClone();
    transf->setCRSs(sourceCRS()->promoteTo3D(std::string(), dbContext),
                    targetCRS()->promoteTo3D(std::string(), dbContext),
                    interpolationCRS());
    return transf;
}

// ---------------------------------------------------------------------------

TransformationNNPtr
Transformation::demoteTo2D(const std::string &,
                           const io::DatabaseContextPtr &dbContext) const {
    auto transf = shallowClone();
    transf->setCRSs(sourceCRS()->demoteTo2D(std::string(), dbContext),
                    targetCRS()->demoteTo2D(std::string(), dbContext),
                    interpolationCRS());
    return transf;
}

//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
/** \brief Return the TOWGS84 parameters of the transformation.
 *
 * If this transformation uses Coordinate Frame Rotation, Position Vector
 * transformation or Geocentric translations, a vector of 7 double values
 * using the Position Vector convention (EPSG:9606) is returned. Those values
 * can be used as the value of the WKT1 TOWGS84 parameter or
 * PROJ +towgs84 parameter.
 *
 * @return a vector of 7 values if valid, otherwise a io::FormattingException
 * is thrown.
 * @throws io::FormattingException
 */
std::vector<double>
Transformation::getTOWGS84Parameters() const // throw(io::FormattingException)
{
    // GDAL WKT1 assumes EPSG:9606 / Position Vector convention

    bool sevenParamsTransform = false;
    bool threeParamsTransform = false;
    bool invertRotSigns = false;
    const auto &l_method = method();
    const auto &methodName = l_method->nameStr();
    const int methodEPSGCode = l_method->getEPSGCode();
    const auto paramCount = parameterValues().size();
    if ((paramCount == 7 &&
         ci_find(methodName, "Coordinate Frame") != std::string::npos) ||
        methodEPSGCode == EPSG_CODE_METHOD_COORDINATE_FRAME_GEOCENTRIC ||
        methodEPSGCode == EPSG_CODE_METHOD_COORDINATE_FRAME_GEOGRAPHIC_2D ||
        methodEPSGCode == EPSG_CODE_METHOD_COORDINATE_FRAME_GEOGRAPHIC_3D) {
        sevenParamsTransform = true;
        invertRotSigns = true;
    } else if ((paramCount == 7 &&
                ci_find(methodName, "Position Vector") != std::string::npos) ||
               methodEPSGCode == EPSG_CODE_METHOD_POSITION_VECTOR_GEOCENTRIC ||
               methodEPSGCode ==
                   EPSG_CODE_METHOD_POSITION_VECTOR_GEOGRAPHIC_2D ||
               methodEPSGCode ==
                   EPSG_CODE_METHOD_POSITION_VECTOR_GEOGRAPHIC_3D) {
        sevenParamsTransform = true;
        invertRotSigns = false;
    } else if ((paramCount == 3 &&
                ci_find(methodName, "Geocentric translations") !=
                    std::string::npos) ||
               methodEPSGCode ==
                   EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_GEOCENTRIC ||
               methodEPSGCode ==
                   EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_GEOGRAPHIC_2D ||
               methodEPSGCode ==
                   EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_GEOGRAPHIC_3D) {
        threeParamsTransform = true;
    }

    if (threeParamsTransform || sevenParamsTransform) {
        std::vector<double> params(7, 0.0);
        bool foundX = false;
        bool foundY = false;
        bool foundZ = false;
        bool foundRotX = false;
        bool foundRotY = false;
        bool foundRotZ = false;
        bool foundScale = false;
        const double rotSign = invertRotSigns ? -1.0 : 1.0;

        const auto fixNegativeZero = [](double x) {
            if (x == 0.0)
                return 0.0;
            return x;
        };

        for (const auto &genOpParamvalue : parameterValues()) {
            auto opParamvalue = dynamic_cast<const OperationParameterValue *>(
                genOpParamvalue.get());
            if (opParamvalue) {
                const auto &parameter = opParamvalue->parameter();
                const auto epsg_code = parameter->getEPSGCode();
                const auto &l_parameterValue = opParamvalue->parameterValue();
                if (l_parameterValue->type() == ParameterValue::Type::MEASURE) {
                    const auto &measure = l_parameterValue->value();
                    if (epsg_code == EPSG_CODE_PARAMETER_X_AXIS_TRANSLATION) {
                        params[0] = measure.getSIValue();
                        foundX = true;
                    } else if (epsg_code ==
                               EPSG_CODE_PARAMETER_Y_AXIS_TRANSLATION) {
                        params[1] = measure.getSIValue();
                        foundY = true;
                    } else if (epsg_code ==
                               EPSG_CODE_PARAMETER_Z_AXIS_TRANSLATION) {
                        params[2] = measure.getSIValue();
                        foundZ = true;
                    } else if (epsg_code ==
                               EPSG_CODE_PARAMETER_X_AXIS_ROTATION) {
                        params[3] = fixNegativeZero(
                            rotSign * measure.convertToUnit(
                                          common::UnitOfMeasure::ARC_SECOND));
                        foundRotX = true;
                    } else if (epsg_code ==
                               EPSG_CODE_PARAMETER_Y_AXIS_ROTATION) {
                        params[4] = fixNegativeZero(
                            rotSign * measure.convertToUnit(
                                          common::UnitOfMeasure::ARC_SECOND));
                        foundRotY = true;
                    } else if (epsg_code ==
                               EPSG_CODE_PARAMETER_Z_AXIS_ROTATION) {
                        params[5] = fixNegativeZero(
                            rotSign * measure.convertToUnit(
                                          common::UnitOfMeasure::ARC_SECOND));
                        foundRotZ = true;
                    } else if (epsg_code ==
                               EPSG_CODE_PARAMETER_SCALE_DIFFERENCE) {
                        params[6] = measure.convertToUnit(
                            common::UnitOfMeasure::PARTS_PER_MILLION);
                        foundScale = true;
                    }
                }
            }
        }
        if (foundX && foundY && foundZ &&
            (threeParamsTransform ||
             (foundRotX && foundRotY && foundRotZ && foundScale))) {
            return params;
        } else {
            throw io::FormattingException(
                "Missing required parameter values in transformation");
        }
    }

#if 0
    if (methodEPSGCode == EPSG_CODE_METHOD_GEOGRAPHIC2D_OFFSETS ||
        methodEPSGCode == EPSG_CODE_METHOD_GEOGRAPHIC3D_OFFSETS) {
        auto offsetLat =
            parameterValueMeasure(EPSG_CODE_PARAMETER_LATITUDE_OFFSET);
        auto offsetLong =
            parameterValueMeasure(EPSG_CODE_PARAMETER_LONGITUDE_OFFSET);

        auto offsetHeight =
            parameterValueMeasure(EPSG_CODE_PARAMETER_VERTICAL_OFFSET);

        if (offsetLat.getSIValue() == 0.0 && offsetLong.getSIValue() == 0.0 &&
            offsetHeight.getSIValue() == 0.0) {
            std::vector<double> params(7, 0.0);
            return params;
        }
    }
#endif

    throw io::FormattingException(
        "Transformation cannot be formatted as WKT1 TOWGS84 parameters");
}
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Instantiate a transformation from a vector of GeneralParameterValue.
 *
 * @param properties See \ref general_properties. At minimum the name should be
 * defined.
 * @param sourceCRSIn Source CRS.
 * @param targetCRSIn Target CRS.
 * @param interpolationCRSIn Interpolation CRS (might be null)
 * @param methodIn Operation method.
 * @param values Vector of GeneralOperationParameterNNPtr.
 * @param accuracies Vector of positional accuracy (might be empty).
 * @return new Transformation.
 * @throws InvalidOperation
 */
TransformationNNPtr Transformation::create(
    const util::PropertyMap &properties, const crs::CRSNNPtr &sourceCRSIn,
    const crs::CRSNNPtr &targetCRSIn, const crs::CRSPtr &interpolationCRSIn,
    const OperationMethodNNPtr &methodIn,
    const std::vector<GeneralParameterValueNNPtr> &values,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {
    if (methodIn->parameters().size() != values.size()) {
        throw InvalidOperation(
            "Inconsistent number of parameters and parameter values");
    }
    auto transf = Transformation::nn_make_shared<Transformation>(
        sourceCRSIn, targetCRSIn, interpolationCRSIn, methodIn, values,
        accuracies);
    transf->assignSelf(transf);
    transf->setProperties(properties);
    std::string name;
    if (properties.getStringValue(common::IdentifiedObject::NAME_KEY, name) &&
        ci_find(name, "ballpark") != std::string::npos) {
        transf->setHasBallparkTransformation(true);
    }
    return transf;
}

// ---------------------------------------------------------------------------

/** \brief Instantiate a transformation and its OperationMethod.
 *
 * @param propertiesTransformation The \ref general_properties of the
 * Transformation.
 * At minimum the name should be defined.
 * @param sourceCRSIn Source CRS.
 * @param targetCRSIn Target CRS.
 * @param interpolationCRSIn Interpolation CRS (might be null)
 * @param propertiesOperationMethod The \ref general_properties of the
 * OperationMethod.
 * At minimum the name should be defined.
 * @param parameters Vector of parameters of the operation method.
 * @param values Vector of ParameterValueNNPtr. Constraint:
 * values.size() == parameters.size()
 * @param accuracies Vector of positional accuracy (might be empty).
 * @return new Transformation.
 * @throws InvalidOperation
 */
TransformationNNPtr
Transformation::create(const util::PropertyMap &propertiesTransformation,
                       const crs::CRSNNPtr &sourceCRSIn,
                       const crs::CRSNNPtr &targetCRSIn,
                       const crs::CRSPtr &interpolationCRSIn,
                       const util::PropertyMap &propertiesOperationMethod,
                       const std::vector<OperationParameterNNPtr> &parameters,
                       const std::vector<ParameterValueNNPtr> &values,
                       const std::vector<metadata::PositionalAccuracyNNPtr>
                           &accuracies) // throw InvalidOperation
{
    OperationMethodNNPtr op(
        OperationMethod::create(propertiesOperationMethod, parameters));

    if (parameters.size() != values.size()) {
        throw InvalidOperation(
            "Inconsistent number of parameters and parameter values");
    }
    std::vector<GeneralParameterValueNNPtr> generalParameterValues;
    generalParameterValues.reserve(values.size());
    for (size_t i = 0; i < values.size(); i++) {
        generalParameterValues.push_back(
            OperationParameterValue::create(parameters[i], values[i]));
    }
    return create(propertiesTransformation, sourceCRSIn, targetCRSIn,
                  interpolationCRSIn, op, generalParameterValues, accuracies);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress

// ---------------------------------------------------------------------------

static TransformationNNPtr createSevenParamsTransform(
    const util::PropertyMap &properties,
    const util::PropertyMap &methodProperties, const crs::CRSNNPtr &sourceCRSIn,
    const crs::CRSNNPtr &targetCRSIn, double translationXMetre,
    double translationYMetre, double translationZMetre,
    double rotationXArcSecond, double rotationYArcSecond,
    double rotationZArcSecond, double scaleDifferencePPM,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {
    return Transformation::create(
        properties, sourceCRSIn, targetCRSIn, nullptr, methodProperties,
        VectorOfParameters{
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_X_AXIS_TRANSLATION),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_Y_AXIS_TRANSLATION),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_Z_AXIS_TRANSLATION),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_X_AXIS_ROTATION),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_Y_AXIS_ROTATION),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_Z_AXIS_ROTATION),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_SCALE_DIFFERENCE),
        },
        createParams(common::Length(translationXMetre),
                     common::Length(translationYMetre),
                     common::Length(translationZMetre),
                     common::Angle(rotationXArcSecond,
                                   common::UnitOfMeasure::ARC_SECOND),
                     common::Angle(rotationYArcSecond,
                                   common::UnitOfMeasure::ARC_SECOND),
                     common::Angle(rotationZArcSecond,
                                   common::UnitOfMeasure::ARC_SECOND),
                     common::Scale(scaleDifferencePPM,
                                   common::UnitOfMeasure::PARTS_PER_MILLION)),
        accuracies);
}

// ---------------------------------------------------------------------------

static void getTransformationType(const crs::CRSNNPtr &sourceCRSIn,
                                  const crs::CRSNNPtr &targetCRSIn,
                                  bool &isGeocentric, bool &isGeog2D,
                                  bool &isGeog3D) {
    auto sourceCRSGeod =
        dynamic_cast<const crs::GeodeticCRS *>(sourceCRSIn.get());
    auto targetCRSGeod =
        dynamic_cast<const crs::GeodeticCRS *>(targetCRSIn.get());
    isGeocentric = sourceCRSGeod && sourceCRSGeod->isGeocentric() &&
                   targetCRSGeod && targetCRSGeod->isGeocentric();
    if (isGeocentric) {
        isGeog2D = false;
        isGeog3D = false;
        return;
    }
    isGeocentric = false;

    auto sourceCRSGeog =
        dynamic_cast<const crs::GeographicCRS *>(sourceCRSIn.get());
    auto targetCRSGeog =
        dynamic_cast<const crs::GeographicCRS *>(targetCRSIn.get());
    if (!sourceCRSGeog || !targetCRSGeog) {
        throw InvalidOperation("Inconsistent CRS type");
    }
    const auto nSrcAxisCount =
        sourceCRSGeog->coordinateSystem()->axisList().size();
    const auto nTargetAxisCount =
        targetCRSGeog->coordinateSystem()->axisList().size();
    isGeog2D = nSrcAxisCount == 2 && nTargetAxisCount == 2;
    isGeog3D = !isGeog2D && nSrcAxisCount >= 2 && nTargetAxisCount >= 2;
}

// ---------------------------------------------------------------------------

static int
useOperationMethodEPSGCodeIfPresent(const util::PropertyMap &properties,
                                    int nDefaultOperationMethodEPSGCode) {
    const auto *operationMethodEPSGCode =
        properties.get("OPERATION_METHOD_EPSG_CODE");
    if (operationMethodEPSGCode) {
        const auto boxedValue = dynamic_cast<const util::BoxedValue *>(
            (*operationMethodEPSGCode).get());
        if (boxedValue &&
            boxedValue->type() == util::BoxedValue::Type::INTEGER) {
            return boxedValue->integerValue();
        }
    }
    return nDefaultOperationMethodEPSGCode;
}
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Instantiate a transformation with Geocentric Translations method.
 *
 * @param properties See \ref general_properties of the Transformation.
 * At minimum the name should be defined.
 * @param sourceCRSIn Source CRS.
 * @param targetCRSIn Target CRS.
 * @param translationXMetre Value of the Translation_X parameter (in metre).
 * @param translationYMetre Value of the Translation_Y parameter (in metre).
 * @param translationZMetre Value of the Translation_Z parameter (in metre).
 * @param accuracies Vector of positional accuracy (might be empty).
 * @return new Transformation.
 */
TransformationNNPtr Transformation::createGeocentricTranslations(
    const util::PropertyMap &properties, const crs::CRSNNPtr &sourceCRSIn,
    const crs::CRSNNPtr &targetCRSIn, double translationXMetre,
    double translationYMetre, double translationZMetre,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {
    bool isGeocentric;
    bool isGeog2D;
    bool isGeog3D;
    getTransformationType(sourceCRSIn, targetCRSIn, isGeocentric, isGeog2D,
                          isGeog3D);
    return create(
        properties, sourceCRSIn, targetCRSIn, nullptr,
        createMethodMapNameEPSGCode(useOperationMethodEPSGCodeIfPresent(
            properties,
            isGeocentric
                ? EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_GEOCENTRIC
                : isGeog2D
                      ? EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_GEOGRAPHIC_2D
                      : EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_GEOGRAPHIC_3D)),
        VectorOfParameters{
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_X_AXIS_TRANSLATION),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_Y_AXIS_TRANSLATION),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_Z_AXIS_TRANSLATION),
        },
        createParams(common::Length(translationXMetre),
                     common::Length(translationYMetre),
                     common::Length(translationZMetre)),
        accuracies);
}

// ---------------------------------------------------------------------------

/** \brief Instantiate a transformation with Position vector transformation
 * method.
 *
 * This is similar to createCoordinateFrameRotation(), except that the sign of
 * the rotation terms is inverted.
 *
 * @param properties See \ref general_properties of the Transformation.
 * At minimum the name should be defined.
 * @param sourceCRSIn Source CRS.
 * @param targetCRSIn Target CRS.
 * @param translationXMetre Value of the Translation_X parameter (in metre).
 * @param translationYMetre Value of the Translation_Y parameter (in metre).
 * @param translationZMetre Value of the Translation_Z parameter (in metre).
 * @param rotationXArcSecond Value of the Rotation_X parameter (in
 * arc-second).
 * @param rotationYArcSecond Value of the Rotation_Y parameter (in
 * arc-second).
 * @param rotationZArcSecond Value of the Rotation_Z parameter (in
 * arc-second).
 * @param scaleDifferencePPM Value of the Scale_Difference parameter (in
 * parts-per-million).
 * @param accuracies Vector of positional accuracy (might be empty).
 * @return new Transformation.
 */
TransformationNNPtr Transformation::createPositionVector(
    const util::PropertyMap &properties, const crs::CRSNNPtr &sourceCRSIn,
    const crs::CRSNNPtr &targetCRSIn, double translationXMetre,
    double translationYMetre, double translationZMetre,
    double rotationXArcSecond, double rotationYArcSecond,
    double rotationZArcSecond, double scaleDifferencePPM,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {

    bool isGeocentric;
    bool isGeog2D;
    bool isGeog3D;
    getTransformationType(sourceCRSIn, targetCRSIn, isGeocentric, isGeog2D,
                          isGeog3D);
    return createSevenParamsTransform(
        properties,
        createMethodMapNameEPSGCode(useOperationMethodEPSGCodeIfPresent(
            properties,
            isGeocentric
                ? EPSG_CODE_METHOD_POSITION_VECTOR_GEOCENTRIC
                : isGeog2D ? EPSG_CODE_METHOD_POSITION_VECTOR_GEOGRAPHIC_2D
                           : EPSG_CODE_METHOD_POSITION_VECTOR_GEOGRAPHIC_3D)),
        sourceCRSIn, targetCRSIn, translationXMetre, translationYMetre,
        translationZMetre, rotationXArcSecond, rotationYArcSecond,
        rotationZArcSecond, scaleDifferencePPM, accuracies);
}

// ---------------------------------------------------------------------------

/** \brief Instantiate a transformation with Coordinate Frame Rotation method.
 *
 * This is similar to createPositionVector(), except that the sign of
 * the rotation terms is inverted.
 *
 * @param properties See \ref general_properties of the Transformation.
 * At minimum the name should be defined.
 * @param sourceCRSIn Source CRS.
 * @param targetCRSIn Target CRS.
 * @param translationXMetre Value of the Translation_X parameter (in metre).
 * @param translationYMetre Value of the Translation_Y parameter (in metre).
 * @param translationZMetre Value of the Translation_Z parameter (in metre).
 * @param rotationXArcSecond Value of the Rotation_X parameter (in
 * arc-second).
 * @param rotationYArcSecond Value of the Rotation_Y parameter (in
 * arc-second).
 * @param rotationZArcSecond Value of the Rotation_Z parameter (in
 * arc-second).
 * @param scaleDifferencePPM Value of the Scale_Difference parameter (in
 * parts-per-million).
 * @param accuracies Vector of positional accuracy (might be empty).
 * @return new Transformation.
 */
TransformationNNPtr Transformation::createCoordinateFrameRotation(
    const util::PropertyMap &properties, const crs::CRSNNPtr &sourceCRSIn,
    const crs::CRSNNPtr &targetCRSIn, double translationXMetre,
    double translationYMetre, double translationZMetre,
    double rotationXArcSecond, double rotationYArcSecond,
    double rotationZArcSecond, double scaleDifferencePPM,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {
    bool isGeocentric;
    bool isGeog2D;
    bool isGeog3D;
    getTransformationType(sourceCRSIn, targetCRSIn, isGeocentric, isGeog2D,
                          isGeog3D);
    return createSevenParamsTransform(
        properties,
        createMethodMapNameEPSGCode(useOperationMethodEPSGCodeIfPresent(
            properties,
            isGeocentric
                ? EPSG_CODE_METHOD_COORDINATE_FRAME_GEOCENTRIC
                : isGeog2D ? EPSG_CODE_METHOD_COORDINATE_FRAME_GEOGRAPHIC_2D
                           : EPSG_CODE_METHOD_COORDINATE_FRAME_GEOGRAPHIC_3D)),
        sourceCRSIn, targetCRSIn, translationXMetre, translationYMetre,
        translationZMetre, rotationXArcSecond, rotationYArcSecond,
        rotationZArcSecond, scaleDifferencePPM, accuracies);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
static TransformationNNPtr createFifteenParamsTransform(
    const util::PropertyMap &properties,
    const util::PropertyMap &methodProperties, const crs::CRSNNPtr &sourceCRSIn,
    const crs::CRSNNPtr &targetCRSIn, double translationXMetre,
    double translationYMetre, double translationZMetre,
    double rotationXArcSecond, double rotationYArcSecond,
    double rotationZArcSecond, double scaleDifferencePPM,
    double rateTranslationX, double rateTranslationY, double rateTranslationZ,
    double rateRotationX, double rateRotationY, double rateRotationZ,
    double rateScaleDifference, double referenceEpochYear,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {
    return Transformation::create(
        properties, sourceCRSIn, targetCRSIn, nullptr, methodProperties,
        VectorOfParameters{
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_X_AXIS_TRANSLATION),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_Y_AXIS_TRANSLATION),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_Z_AXIS_TRANSLATION),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_X_AXIS_ROTATION),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_Y_AXIS_ROTATION),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_Z_AXIS_ROTATION),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_SCALE_DIFFERENCE),

            createOpParamNameEPSGCode(
                EPSG_CODE_PARAMETER_RATE_X_AXIS_TRANSLATION),
            createOpParamNameEPSGCode(
                EPSG_CODE_PARAMETER_RATE_Y_AXIS_TRANSLATION),
            createOpParamNameEPSGCode(
                EPSG_CODE_PARAMETER_RATE_Z_AXIS_TRANSLATION),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_RATE_X_AXIS_ROTATION),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_RATE_Y_AXIS_ROTATION),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_RATE_Z_AXIS_ROTATION),
            createOpParamNameEPSGCode(
                EPSG_CODE_PARAMETER_RATE_SCALE_DIFFERENCE),

            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_REFERENCE_EPOCH),
        },
        VectorOfValues{
            common::Length(translationXMetre),
            common::Length(translationYMetre),
            common::Length(translationZMetre),
            common::Angle(rotationXArcSecond,
                          common::UnitOfMeasure::ARC_SECOND),
            common::Angle(rotationYArcSecond,
                          common::UnitOfMeasure::ARC_SECOND),
            common::Angle(rotationZArcSecond,
                          common::UnitOfMeasure::ARC_SECOND),
            common::Scale(scaleDifferencePPM,
                          common::UnitOfMeasure::PARTS_PER_MILLION),
            common::Measure(rateTranslationX,
                            common::UnitOfMeasure::METRE_PER_YEAR),
            common::Measure(rateTranslationY,
                            common::UnitOfMeasure::METRE_PER_YEAR),
            common::Measure(rateTranslationZ,
                            common::UnitOfMeasure::METRE_PER_YEAR),
            common::Measure(rateRotationX,
                            common::UnitOfMeasure::ARC_SECOND_PER_YEAR),
            common::Measure(rateRotationY,
                            common::UnitOfMeasure::ARC_SECOND_PER_YEAR),
            common::Measure(rateRotationZ,
                            common::UnitOfMeasure::ARC_SECOND_PER_YEAR),
            common::Measure(rateScaleDifference,
                            common::UnitOfMeasure::PPM_PER_YEAR),
            common::Measure(referenceEpochYear, common::UnitOfMeasure::YEAR),
        },
        accuracies);
}
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Instantiate a transformation with Time Dependent position vector
 * transformation method.
 *
 * This is similar to createTimeDependentCoordinateFrameRotation(), except that
 * the sign of
 * the rotation terms is inverted.
 *
 * This method is defined as [EPSG:1053]
 * (https://www.epsg-registry.org/export.htm?gml=urn:ogc:def:method:EPSG::1053)
 *
 * @param properties See \ref general_properties of the Transformation.
 * At minimum the name should be defined.
 * @param sourceCRSIn Source CRS.
 * @param targetCRSIn Target CRS.
 * @param translationXMetre Value of the Translation_X parameter (in metre).
 * @param translationYMetre Value of the Translation_Y parameter (in metre).
 * @param translationZMetre Value of the Translation_Z parameter (in metre).
 * @param rotationXArcSecond Value of the Rotation_X parameter (in
 * arc-second).
 * @param rotationYArcSecond Value of the Rotation_Y parameter (in
 * arc-second).
 * @param rotationZArcSecond Value of the Rotation_Z parameter (in
 * arc-second).
 * @param scaleDifferencePPM Value of the Scale_Difference parameter (in
 * parts-per-million).
 * @param rateTranslationX Value of the rate of change of X-axis translation (in
 * metre/year)
 * @param rateTranslationY Value of the rate of change of Y-axis translation (in
 * metre/year)
 * @param rateTranslationZ Value of the rate of change of Z-axis translation (in
 * metre/year)
 * @param rateRotationX Value of the rate of change of X-axis rotation (in
 * arc-second/year)
 * @param rateRotationY Value of the rate of change of Y-axis rotation (in
 * arc-second/year)
 * @param rateRotationZ Value of the rate of change of Z-axis rotation (in
 * arc-second/year)
 * @param rateScaleDifference Value of the rate of change of scale difference
 * (in PPM/year)
 * @param referenceEpochYear Parameter reference epoch (in decimal year)
 * @param accuracies Vector of positional accuracy (might be empty).
 * @return new Transformation.
 */
TransformationNNPtr Transformation::createTimeDependentPositionVector(
    const util::PropertyMap &properties, const crs::CRSNNPtr &sourceCRSIn,
    const crs::CRSNNPtr &targetCRSIn, double translationXMetre,
    double translationYMetre, double translationZMetre,
    double rotationXArcSecond, double rotationYArcSecond,
    double rotationZArcSecond, double scaleDifferencePPM,
    double rateTranslationX, double rateTranslationY, double rateTranslationZ,
    double rateRotationX, double rateRotationY, double rateRotationZ,
    double rateScaleDifference, double referenceEpochYear,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {
    bool isGeocentric;
    bool isGeog2D;
    bool isGeog3D;
    getTransformationType(sourceCRSIn, targetCRSIn, isGeocentric, isGeog2D,
                          isGeog3D);
    return createFifteenParamsTransform(
        properties,
        createMethodMapNameEPSGCode(useOperationMethodEPSGCodeIfPresent(
            properties,
            isGeocentric
                ? EPSG_CODE_METHOD_TIME_DEPENDENT_POSITION_VECTOR_GEOCENTRIC
                : isGeog2D
                      ? EPSG_CODE_METHOD_TIME_DEPENDENT_POSITION_VECTOR_GEOGRAPHIC_2D
                      : EPSG_CODE_METHOD_TIME_DEPENDENT_POSITION_VECTOR_GEOGRAPHIC_3D)),
        sourceCRSIn, targetCRSIn, translationXMetre, translationYMetre,
        translationZMetre, rotationXArcSecond, rotationYArcSecond,
        rotationZArcSecond, scaleDifferencePPM, rateTranslationX,
        rateTranslationY, rateTranslationZ, rateRotationX, rateRotationY,
        rateRotationZ, rateScaleDifference, referenceEpochYear, accuracies);
}

// ---------------------------------------------------------------------------

/** \brief Instantiate a transformation with Time Dependent Position coordinate
 * frame rotation transformation method.
 *
 * This is similar to createTimeDependentPositionVector(), except that the sign
 * of
 * the rotation terms is inverted.
 *
 * This method is defined as [EPSG:1056]
 * (https://www.epsg-registry.org/export.htm?gml=urn:ogc:def:method:EPSG::1056)
 *
 * @param properties See \ref general_properties of the Transformation.
 * At minimum the name should be defined.
 * @param sourceCRSIn Source CRS.
 * @param targetCRSIn Target CRS.
 * @param translationXMetre Value of the Translation_X parameter (in metre).
 * @param translationYMetre Value of the Translation_Y parameter (in metre).
 * @param translationZMetre Value of the Translation_Z parameter (in metre).
 * @param rotationXArcSecond Value of the Rotation_X parameter (in
 * arc-second).
 * @param rotationYArcSecond Value of the Rotation_Y parameter (in
 * arc-second).
 * @param rotationZArcSecond Value of the Rotation_Z parameter (in
 * arc-second).
 * @param scaleDifferencePPM Value of the Scale_Difference parameter (in
 * parts-per-million).
 * @param rateTranslationX Value of the rate of change of X-axis translation (in
 * metre/year)
 * @param rateTranslationY Value of the rate of change of Y-axis translation (in
 * metre/year)
 * @param rateTranslationZ Value of the rate of change of Z-axis translation (in
 * metre/year)
 * @param rateRotationX Value of the rate of change of X-axis rotation (in
 * arc-second/year)
 * @param rateRotationY Value of the rate of change of Y-axis rotation (in
 * arc-second/year)
 * @param rateRotationZ Value of the rate of change of Z-axis rotation (in
 * arc-second/year)
 * @param rateScaleDifference Value of the rate of change of scale difference
 * (in PPM/year)
 * @param referenceEpochYear Parameter reference epoch (in decimal year)
 * @param accuracies Vector of positional accuracy (might be empty).
 * @return new Transformation.
 * @throws InvalidOperation
 */
TransformationNNPtr Transformation::createTimeDependentCoordinateFrameRotation(
    const util::PropertyMap &properties, const crs::CRSNNPtr &sourceCRSIn,
    const crs::CRSNNPtr &targetCRSIn, double translationXMetre,
    double translationYMetre, double translationZMetre,
    double rotationXArcSecond, double rotationYArcSecond,
    double rotationZArcSecond, double scaleDifferencePPM,
    double rateTranslationX, double rateTranslationY, double rateTranslationZ,
    double rateRotationX, double rateRotationY, double rateRotationZ,
    double rateScaleDifference, double referenceEpochYear,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {

    bool isGeocentric;
    bool isGeog2D;
    bool isGeog3D;
    getTransformationType(sourceCRSIn, targetCRSIn, isGeocentric, isGeog2D,
                          isGeog3D);
    return createFifteenParamsTransform(
        properties,
        createMethodMapNameEPSGCode(useOperationMethodEPSGCodeIfPresent(
            properties,
            isGeocentric
                ? EPSG_CODE_METHOD_TIME_DEPENDENT_COORDINATE_FRAME_GEOCENTRIC
                : isGeog2D
                      ? EPSG_CODE_METHOD_TIME_DEPENDENT_COORDINATE_FRAME_GEOGRAPHIC_2D
                      : EPSG_CODE_METHOD_TIME_DEPENDENT_COORDINATE_FRAME_GEOGRAPHIC_3D)),
        sourceCRSIn, targetCRSIn, translationXMetre, translationYMetre,
        translationZMetre, rotationXArcSecond, rotationYArcSecond,
        rotationZArcSecond, scaleDifferencePPM, rateTranslationX,
        rateTranslationY, rateTranslationZ, rateRotationX, rateRotationY,
        rateRotationZ, rateScaleDifference, referenceEpochYear, accuracies);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
static TransformationNNPtr _createMolodensky(
    const util::PropertyMap &properties, const crs::CRSNNPtr &sourceCRSIn,
    const crs::CRSNNPtr &targetCRSIn, int methodEPSGCode,
    double translationXMetre, double translationYMetre,
    double translationZMetre, double semiMajorAxisDifferenceMetre,
    double flattingDifference,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {
    return Transformation::create(
        properties, sourceCRSIn, targetCRSIn, nullptr,
        createMethodMapNameEPSGCode(methodEPSGCode),
        VectorOfParameters{
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_X_AXIS_TRANSLATION),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_Y_AXIS_TRANSLATION),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_Z_AXIS_TRANSLATION),
            createOpParamNameEPSGCode(
                EPSG_CODE_PARAMETER_SEMI_MAJOR_AXIS_DIFFERENCE),
            createOpParamNameEPSGCode(
                EPSG_CODE_PARAMETER_FLATTENING_DIFFERENCE),
        },
        createParams(
            common::Length(translationXMetre),
            common::Length(translationYMetre),
            common::Length(translationZMetre),
            common::Length(semiMajorAxisDifferenceMetre),
            common::Measure(flattingDifference, common::UnitOfMeasure::NONE)),
        accuracies);
}
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Instantiate a transformation with Molodensky method.
 *
 * @see createAbridgedMolodensky() for a related method.
 *
 * This method is defined as [EPSG:9604]
 * (https://www.epsg-registry.org/export.htm?gml=urn:ogc:def:method:EPSG::9604)
 *
 * @param properties See \ref general_properties of the Transformation.
 * At minimum the name should be defined.
 * @param sourceCRSIn Source CRS.
 * @param targetCRSIn Target CRS.
 * @param translationXMetre Value of the Translation_X parameter (in metre).
 * @param translationYMetre Value of the Translation_Y parameter (in metre).
 * @param translationZMetre Value of the Translation_Z parameter (in metre).
 * @param semiMajorAxisDifferenceMetre The difference between the semi-major
 * axis values of the ellipsoids used in the target and source CRS (in metre).
 * @param flattingDifference The difference  between the flattening values of
 * the ellipsoids used in the target and source CRS.
 * @param accuracies Vector of positional accuracy (might be empty).
 * @return new Transformation.
 * @throws InvalidOperation
 */
TransformationNNPtr Transformation::createMolodensky(
    const util::PropertyMap &properties, const crs::CRSNNPtr &sourceCRSIn,
    const crs::CRSNNPtr &targetCRSIn, double translationXMetre,
    double translationYMetre, double translationZMetre,
    double semiMajorAxisDifferenceMetre, double flattingDifference,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {
    return _createMolodensky(
        properties, sourceCRSIn, targetCRSIn, EPSG_CODE_METHOD_MOLODENSKY,
        translationXMetre, translationYMetre, translationZMetre,
        semiMajorAxisDifferenceMetre, flattingDifference, accuracies);
}

// ---------------------------------------------------------------------------

/** \brief Instantiate a transformation with Abridged Molodensky method.
 *
 * @see createdMolodensky() for a related method.
 *
 * This method is defined as [EPSG:9605]
 * (https://www.epsg-registry.org/export.htm?gml=urn:ogc:def:method:EPSG::9605)
 *
 * @param properties See \ref general_properties of the Transformation.
 * At minimum the name should be defined.
 * @param sourceCRSIn Source CRS.
 * @param targetCRSIn Target CRS.
 * @param translationXMetre Value of the Translation_X parameter (in metre).
 * @param translationYMetre Value of the Translation_Y parameter (in metre).
 * @param translationZMetre Value of the Translation_Z parameter (in metre).
 * @param semiMajorAxisDifferenceMetre The difference between the semi-major
 * axis values of the ellipsoids used in the target and source CRS (in metre).
 * @param flattingDifference The difference  between the flattening values of
 * the ellipsoids used in the target and source CRS.
 * @param accuracies Vector of positional accuracy (might be empty).
 * @return new Transformation.
 * @throws InvalidOperation
 */
TransformationNNPtr Transformation::createAbridgedMolodensky(
    const util::PropertyMap &properties, const crs::CRSNNPtr &sourceCRSIn,
    const crs::CRSNNPtr &targetCRSIn, double translationXMetre,
    double translationYMetre, double translationZMetre,
    double semiMajorAxisDifferenceMetre, double flattingDifference,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {
    return _createMolodensky(properties, sourceCRSIn, targetCRSIn,
                             EPSG_CODE_METHOD_ABRIDGED_MOLODENSKY,
                             translationXMetre, translationYMetre,
                             translationZMetre, semiMajorAxisDifferenceMetre,
                             flattingDifference, accuracies);
}

// ---------------------------------------------------------------------------

/** \brief Instantiate a transformation from TOWGS84 parameters.
 *
 * This is a helper of createPositionVector() with the source CRS being the
 * GeographicCRS of sourceCRSIn, and the target CRS being EPSG:4326
 *
 * @param sourceCRSIn Source CRS.
 * @param TOWGS84Parameters The vector of 3 double values (Translation_X,_Y,_Z)
 * or 7 double values (Translation_X,_Y,_Z, Rotation_X,_Y,_Z, Scale_Difference)
 * passed to createPositionVector()
 * @return new Transformation.
 * @throws InvalidOperation
 */
TransformationNNPtr Transformation::createTOWGS84(
    const crs::CRSNNPtr &sourceCRSIn,
    const std::vector<double> &TOWGS84Parameters) // throw InvalidOperation
{
    if (TOWGS84Parameters.size() != 3 && TOWGS84Parameters.size() != 7) {
        throw InvalidOperation(
            "Invalid number of elements in TOWGS84Parameters");
    }

    crs::CRSPtr transformSourceCRS = sourceCRSIn->extractGeodeticCRS();
    if (!transformSourceCRS) {
        throw InvalidOperation(
            "Cannot find GeodeticCRS in sourceCRS of TOWGS84 transformation");
    }

    util::PropertyMap properties;
    properties.set(common::IdentifiedObject::NAME_KEY,
                   concat("Transformation from ", transformSourceCRS->nameStr(),
                          " to WGS84"));

    auto targetCRS =
        dynamic_cast<const crs::GeographicCRS *>(transformSourceCRS.get())
            ? util::nn_static_pointer_cast<crs::CRS>(
                  crs::GeographicCRS::EPSG_4326)
            : util::nn_static_pointer_cast<crs::CRS>(
                  crs::GeodeticCRS::EPSG_4978);

    if (TOWGS84Parameters.size() == 3) {
        return createGeocentricTranslations(
            properties, NN_NO_CHECK(transformSourceCRS), targetCRS,
            TOWGS84Parameters[0], TOWGS84Parameters[1], TOWGS84Parameters[2],
            {});
    }

    return createPositionVector(properties, NN_NO_CHECK(transformSourceCRS),
                                targetCRS, TOWGS84Parameters[0],
                                TOWGS84Parameters[1], TOWGS84Parameters[2],
                                TOWGS84Parameters[3], TOWGS84Parameters[4],
                                TOWGS84Parameters[5], TOWGS84Parameters[6], {});
}

// ---------------------------------------------------------------------------
/** \brief Instantiate a transformation with NTv2 method.
 *
 * @param properties See \ref general_properties of the Transformation.
 * At minimum the name should be defined.
 * @param sourceCRSIn Source CRS.
 * @param targetCRSIn Target CRS.
 * @param filename NTv2 filename.
 * @param accuracies Vector of positional accuracy (might be empty).
 * @return new Transformation.
 */
TransformationNNPtr Transformation::createNTv2(
    const util::PropertyMap &properties, const crs::CRSNNPtr &sourceCRSIn,
    const crs::CRSNNPtr &targetCRSIn, const std::string &filename,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {

    return create(properties, sourceCRSIn, targetCRSIn, nullptr,
                  createMethodMapNameEPSGCode(EPSG_CODE_METHOD_NTV2),
                  VectorOfParameters{createOpParamNameEPSGCode(
                      EPSG_CODE_PARAMETER_LATITUDE_LONGITUDE_DIFFERENCE_FILE)},
                  VectorOfValues{ParameterValue::createFilename(filename)},
                  accuracies);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
static TransformationNNPtr _createGravityRelatedHeightToGeographic3D(
    const util::PropertyMap &properties, bool inverse,
    const crs::CRSNNPtr &sourceCRSIn, const crs::CRSNNPtr &targetCRSIn,
    const crs::CRSPtr &interpolationCRSIn, const std::string &filename,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {

    return Transformation::create(
        properties, sourceCRSIn, targetCRSIn, interpolationCRSIn,
        util::PropertyMap().set(
            common::IdentifiedObject::NAME_KEY,
            inverse ? INVERSE_OF + PROJ_WKT2_NAME_METHOD_HEIGHT_TO_GEOG3D
                    : PROJ_WKT2_NAME_METHOD_HEIGHT_TO_GEOG3D),
        VectorOfParameters{createOpParamNameEPSGCode(
            EPSG_CODE_PARAMETER_GEOID_CORRECTION_FILENAME)},
        VectorOfValues{ParameterValue::createFilename(filename)}, accuracies);
}
//! @endcond

// ---------------------------------------------------------------------------
/** \brief Instantiate a transformation from GravityRelatedHeight to
 * Geographic3D
 *
 * @param properties See \ref general_properties of the Transformation.
 * At minimum the name should be defined.
 * @param sourceCRSIn Source CRS.
 * @param targetCRSIn Target CRS.
 * @param interpolationCRSIn Interpolation CRS. (might be null)
 * @param filename GRID filename.
 * @param accuracies Vector of positional accuracy (might be empty).
 * @return new Transformation.
 */
TransformationNNPtr Transformation::createGravityRelatedHeightToGeographic3D(
    const util::PropertyMap &properties, const crs::CRSNNPtr &sourceCRSIn,
    const crs::CRSNNPtr &targetCRSIn, const crs::CRSPtr &interpolationCRSIn,
    const std::string &filename,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {

    return _createGravityRelatedHeightToGeographic3D(
        properties, false, sourceCRSIn, targetCRSIn, interpolationCRSIn,
        filename, accuracies);
}

// ---------------------------------------------------------------------------

/** \brief Instantiate a transformation with method VERTCON
 *
 * @param properties See \ref general_properties of the Transformation.
 * At minimum the name should be defined.
 * @param sourceCRSIn Source CRS.
 * @param targetCRSIn Target CRS.
 * @param filename GRID filename.
 * @param accuracies Vector of positional accuracy (might be empty).
 * @return new Transformation.
 */
TransformationNNPtr Transformation::createVERTCON(
    const util::PropertyMap &properties, const crs::CRSNNPtr &sourceCRSIn,
    const crs::CRSNNPtr &targetCRSIn, const std::string &filename,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {

    return create(properties, sourceCRSIn, targetCRSIn, nullptr,
                  createMethodMapNameEPSGCode(EPSG_CODE_METHOD_VERTCON),
                  VectorOfParameters{createOpParamNameEPSGCode(
                      EPSG_CODE_PARAMETER_VERTICAL_OFFSET_FILE)},
                  VectorOfValues{ParameterValue::createFilename(filename)},
                  accuracies);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
static inline std::vector<metadata::PositionalAccuracyNNPtr>
buildAccuracyZero() {
    return std::vector<metadata::PositionalAccuracyNNPtr>{
        metadata::PositionalAccuracy::create("0")};
}

// ---------------------------------------------------------------------------

//! @endcond

/** \brief Instantiate a transformation with method Longitude rotation
 *
 * This method is defined as [EPSG:9601]
 * (https://www.epsg-registry.org/export.htm?gml=urn:ogc:def:method:EPSG::9601)
 * *
 * @param properties See \ref general_properties of the Transformation.
 * At minimum the name should be defined.
 * @param sourceCRSIn Source CRS.
 * @param targetCRSIn Target CRS.
 * @param offset Longitude offset to add.
 * @return new Transformation.
 */
TransformationNNPtr Transformation::createLongitudeRotation(
    const util::PropertyMap &properties, const crs::CRSNNPtr &sourceCRSIn,
    const crs::CRSNNPtr &targetCRSIn, const common::Angle &offset) {

    return create(
        properties, sourceCRSIn, targetCRSIn, nullptr,
        createMethodMapNameEPSGCode(EPSG_CODE_METHOD_LONGITUDE_ROTATION),
        VectorOfParameters{
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_LONGITUDE_OFFSET)},
        VectorOfValues{ParameterValue::create(offset)}, buildAccuracyZero());
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
bool Transformation::isLongitudeRotation() const {
    return method()->getEPSGCode() == EPSG_CODE_METHOD_LONGITUDE_ROTATION;
}

//! @endcond

// ---------------------------------------------------------------------------

/** \brief Instantiate a transformation with method Geographic 2D offsets
 *
 * This method is defined as [EPSG:9619]
 * (https://www.epsg-registry.org/export.htm?gml=urn:ogc:def:method:EPSG::9619)
 * *
 * @param properties See \ref general_properties of the Transformation.
 * At minimum the name should be defined.
 * @param sourceCRSIn Source CRS.
 * @param targetCRSIn Target CRS.
 * @param offsetLat Latitude offset to add.
 * @param offsetLon Longitude offset to add.
 * @param accuracies Vector of positional accuracy (might be empty).
 * @return new Transformation.
 */
TransformationNNPtr Transformation::createGeographic2DOffsets(
    const util::PropertyMap &properties, const crs::CRSNNPtr &sourceCRSIn,
    const crs::CRSNNPtr &targetCRSIn, const common::Angle &offsetLat,
    const common::Angle &offsetLon,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {
    return create(
        properties, sourceCRSIn, targetCRSIn, nullptr,
        createMethodMapNameEPSGCode(EPSG_CODE_METHOD_GEOGRAPHIC2D_OFFSETS),
        VectorOfParameters{
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_LATITUDE_OFFSET),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_LONGITUDE_OFFSET)},
        VectorOfValues{offsetLat, offsetLon}, accuracies);
}

// ---------------------------------------------------------------------------

/** \brief Instantiate a transformation with method Geographic 3D offsets
 *
 * This method is defined as [EPSG:9660]
 * (https://www.epsg-registry.org/export.htm?gml=urn:ogc:def:method:EPSG::9660)
 * *
 * @param properties See \ref general_properties of the Transformation.
 * At minimum the name should be defined.
 * @param sourceCRSIn Source CRS.
 * @param targetCRSIn Target CRS.
 * @param offsetLat Latitude offset to add.
 * @param offsetLon Longitude offset to add.
 * @param offsetHeight Height offset to add.
 * @param accuracies Vector of positional accuracy (might be empty).
 * @return new Transformation.
 */
TransformationNNPtr Transformation::createGeographic3DOffsets(
    const util::PropertyMap &properties, const crs::CRSNNPtr &sourceCRSIn,
    const crs::CRSNNPtr &targetCRSIn, const common::Angle &offsetLat,
    const common::Angle &offsetLon, const common::Length &offsetHeight,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {
    return create(
        properties, sourceCRSIn, targetCRSIn, nullptr,
        createMethodMapNameEPSGCode(EPSG_CODE_METHOD_GEOGRAPHIC3D_OFFSETS),
        VectorOfParameters{
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_LATITUDE_OFFSET),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_LONGITUDE_OFFSET),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_VERTICAL_OFFSET)},
        VectorOfValues{offsetLat, offsetLon, offsetHeight}, accuracies);
}

// ---------------------------------------------------------------------------

/** \brief Instantiate a transformation with method Geographic 2D with
 * height
 * offsets
 *
 * This method is defined as [EPSG:9618]
 * (https://www.epsg-registry.org/export.htm?gml=urn:ogc:def:method:EPSG::9618)
 * *
 * @param properties See \ref general_properties of the Transformation.
 * At minimum the name should be defined.
 * @param sourceCRSIn Source CRS.
 * @param targetCRSIn Target CRS.
 * @param offsetLat Latitude offset to add.
 * @param offsetLon Longitude offset to add.
 * @param offsetHeight Geoid undulation to add.
 * @param accuracies Vector of positional accuracy (might be empty).
 * @return new Transformation.
 */
TransformationNNPtr Transformation::createGeographic2DWithHeightOffsets(
    const util::PropertyMap &properties, const crs::CRSNNPtr &sourceCRSIn,
    const crs::CRSNNPtr &targetCRSIn, const common::Angle &offsetLat,
    const common::Angle &offsetLon, const common::Length &offsetHeight,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {
    return create(
        properties, sourceCRSIn, targetCRSIn, nullptr,
        createMethodMapNameEPSGCode(
            EPSG_CODE_METHOD_GEOGRAPHIC2D_WITH_HEIGHT_OFFSETS),
        VectorOfParameters{
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_LATITUDE_OFFSET),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_LONGITUDE_OFFSET),
            createOpParamNameEPSGCode(EPSG_CODE_PARAMETER_GEOID_UNDULATION)},
        VectorOfValues{offsetLat, offsetLon, offsetHeight}, accuracies);
}

// ---------------------------------------------------------------------------

/** \brief Instantiate a transformation with method Vertical Offset.
 *
 * This method is defined as [EPSG:9616]
 * (https://www.epsg-registry.org/export.htm?gml=urn:ogc:def:method:EPSG::9616)
 * *
 * @param properties See \ref general_properties of the Transformation.
 * At minimum the name should be defined.
 * @param sourceCRSIn Source CRS.
 * @param targetCRSIn Target CRS.
 * @param offsetHeight Geoid undulation to add.
 * @param accuracies Vector of positional accuracy (might be empty).
 * @return new Transformation.
 */
TransformationNNPtr Transformation::createVerticalOffset(
    const util::PropertyMap &properties, const crs::CRSNNPtr &sourceCRSIn,
    const crs::CRSNNPtr &targetCRSIn, const common::Length &offsetHeight,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {
    return create(properties, sourceCRSIn, targetCRSIn, nullptr,
                  createMethodMapNameEPSGCode(EPSG_CODE_METHOD_VERTICAL_OFFSET),
                  VectorOfParameters{createOpParamNameEPSGCode(
                      EPSG_CODE_PARAMETER_VERTICAL_OFFSET)},
                  VectorOfValues{offsetHeight}, accuracies);
}

// ---------------------------------------------------------------------------

/** \brief Instantiate a transformation based on the Change of Vertical Unit
 * method.
 *
 * This method is defined as [EPSG:1069]
 * (https://www.epsg-registry.org/export.htm?gml=urn:ogc:def:method:EPSG::1069)
 *
 * @param properties See \ref general_properties of the conversion. If the name
 * is not provided, it is automatically set.
 * @param sourceCRSIn Source CRS.
 * @param targetCRSIn Target CRS.
 * @param factor Conversion factor
 * @param accuracies Vector of positional accuracy (might be empty).
 * @return a new Transformation.
 */
TransformationNNPtr Transformation::createChangeVerticalUnit(
    const util::PropertyMap &properties, const crs::CRSNNPtr &sourceCRSIn,
    const crs::CRSNNPtr &targetCRSIn, const common::Scale &factor,
    const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {
    return create(
        properties, sourceCRSIn, targetCRSIn, nullptr,
        createMethodMapNameEPSGCode(EPSG_CODE_METHOD_CHANGE_VERTICAL_UNIT),
        VectorOfParameters{
            createOpParamNameEPSGCode(
                EPSG_CODE_PARAMETER_UNIT_CONVERSION_SCALAR),
        },
        VectorOfValues{
            factor,
        },
        accuracies);
}

// ---------------------------------------------------------------------------

// to avoid -0...
static double negate(double val) {
    if (val != 0) {
        return -val;
    }
    return 0.0;
}

// ---------------------------------------------------------------------------

static CoordinateOperationPtr
createApproximateInverseIfPossible(const Transformation *op) {
    bool sevenParamsTransform = false;
    bool fifteenParamsTransform = false;
    const auto &method = op->method();
    const auto &methodName = method->nameStr();
    const int methodEPSGCode = method->getEPSGCode();
    const auto paramCount = op->parameterValues().size();
    const bool isPositionVector =
        ci_find(methodName, "Position Vector") != std::string::npos;
    const bool isCoordinateFrame =
        ci_find(methodName, "Coordinate Frame") != std::string::npos;

    // See end of "2.4.3.3 Helmert 7-parameter transformations"
    // in EPSG 7-2 guidance
    // For practical purposes, the inverse of 7- or 15-parameters Helmert
    // can be obtained by using the forward method with all parameters
    // negated
    // (except reference epoch!)
    // So for WKT export use that. But for PROJ string, we use the +inv flag
    // so as to get "perfect" round-tripability.
    if ((paramCount == 7 && isCoordinateFrame &&
         !isTimeDependent(methodName)) ||
        methodEPSGCode == EPSG_CODE_METHOD_COORDINATE_FRAME_GEOCENTRIC ||
        methodEPSGCode == EPSG_CODE_METHOD_COORDINATE_FRAME_GEOGRAPHIC_2D ||
        methodEPSGCode == EPSG_CODE_METHOD_COORDINATE_FRAME_GEOGRAPHIC_3D) {
        sevenParamsTransform = true;
    } else if (
        (paramCount == 15 && isCoordinateFrame &&
         isTimeDependent(methodName)) ||
        methodEPSGCode ==
            EPSG_CODE_METHOD_TIME_DEPENDENT_COORDINATE_FRAME_GEOCENTRIC ||
        methodEPSGCode ==
            EPSG_CODE_METHOD_TIME_DEPENDENT_COORDINATE_FRAME_GEOGRAPHIC_2D ||
        methodEPSGCode ==
            EPSG_CODE_METHOD_TIME_DEPENDENT_COORDINATE_FRAME_GEOGRAPHIC_3D) {
        fifteenParamsTransform = true;
    } else if ((paramCount == 7 && isPositionVector &&
                !isTimeDependent(methodName)) ||
               methodEPSGCode == EPSG_CODE_METHOD_POSITION_VECTOR_GEOCENTRIC ||
               methodEPSGCode ==
                   EPSG_CODE_METHOD_POSITION_VECTOR_GEOGRAPHIC_2D ||
               methodEPSGCode ==
                   EPSG_CODE_METHOD_POSITION_VECTOR_GEOGRAPHIC_3D) {
        sevenParamsTransform = true;
    } else if (
        (paramCount == 15 && isPositionVector && isTimeDependent(methodName)) ||
        methodEPSGCode ==
            EPSG_CODE_METHOD_TIME_DEPENDENT_POSITION_VECTOR_GEOCENTRIC ||
        methodEPSGCode ==
            EPSG_CODE_METHOD_TIME_DEPENDENT_POSITION_VECTOR_GEOGRAPHIC_2D ||
        methodEPSGCode ==
            EPSG_CODE_METHOD_TIME_DEPENDENT_POSITION_VECTOR_GEOGRAPHIC_3D) {
        fifteenParamsTransform = true;
    }
    if (sevenParamsTransform || fifteenParamsTransform) {
        double neg_x = negate(op->parameterValueNumericAsSI(
            EPSG_CODE_PARAMETER_X_AXIS_TRANSLATION));
        double neg_y = negate(op->parameterValueNumericAsSI(
            EPSG_CODE_PARAMETER_Y_AXIS_TRANSLATION));
        double neg_z = negate(op->parameterValueNumericAsSI(
            EPSG_CODE_PARAMETER_Z_AXIS_TRANSLATION));
        double neg_rx = negate(
            op->parameterValueNumeric(EPSG_CODE_PARAMETER_X_AXIS_ROTATION,
                                      common::UnitOfMeasure::ARC_SECOND));
        double neg_ry = negate(
            op->parameterValueNumeric(EPSG_CODE_PARAMETER_Y_AXIS_ROTATION,
                                      common::UnitOfMeasure::ARC_SECOND));
        double neg_rz = negate(
            op->parameterValueNumeric(EPSG_CODE_PARAMETER_Z_AXIS_ROTATION,
                                      common::UnitOfMeasure::ARC_SECOND));
        double neg_scaleDiff = negate(op->parameterValueNumeric(
            EPSG_CODE_PARAMETER_SCALE_DIFFERENCE,
            common::UnitOfMeasure::PARTS_PER_MILLION));
        auto methodProperties = util::PropertyMap().set(
            common::IdentifiedObject::NAME_KEY, methodName);
        int method_epsg_code = method->getEPSGCode();
        if (method_epsg_code) {
            methodProperties
                .set(metadata::Identifier::CODESPACE_KEY,
                     metadata::Identifier::EPSG)
                .set(metadata::Identifier::CODE_KEY, method_epsg_code);
        }
        if (fifteenParamsTransform) {
            double neg_rate_x = negate(op->parameterValueNumeric(
                EPSG_CODE_PARAMETER_RATE_X_AXIS_TRANSLATION,
                common::UnitOfMeasure::METRE_PER_YEAR));
            double neg_rate_y = negate(op->parameterValueNumeric(
                EPSG_CODE_PARAMETER_RATE_Y_AXIS_TRANSLATION,
                common::UnitOfMeasure::METRE_PER_YEAR));
            double neg_rate_z = negate(op->parameterValueNumeric(
                EPSG_CODE_PARAMETER_RATE_Z_AXIS_TRANSLATION,
                common::UnitOfMeasure::METRE_PER_YEAR));
            double neg_rate_rx = negate(op->parameterValueNumeric(
                EPSG_CODE_PARAMETER_RATE_X_AXIS_ROTATION,
                common::UnitOfMeasure::ARC_SECOND_PER_YEAR));
            double neg_rate_ry = negate(op->parameterValueNumeric(
                EPSG_CODE_PARAMETER_RATE_Y_AXIS_ROTATION,
                common::UnitOfMeasure::ARC_SECOND_PER_YEAR));
            double neg_rate_rz = negate(op->parameterValueNumeric(
                EPSG_CODE_PARAMETER_RATE_Z_AXIS_ROTATION,
                common::UnitOfMeasure::ARC_SECOND_PER_YEAR));
            double neg_rate_scaleDiff = negate(op->parameterValueNumeric(
                EPSG_CODE_PARAMETER_RATE_SCALE_DIFFERENCE,
                common::UnitOfMeasure::PPM_PER_YEAR));
            double referenceEpochYear =
                op->parameterValueNumeric(EPSG_CODE_PARAMETER_REFERENCE_EPOCH,
                                          common::UnitOfMeasure::YEAR);
            return util::nn_static_pointer_cast<CoordinateOperation>(
                       createFifteenParamsTransform(
                           createPropertiesForInverse(op, false, true),
                           methodProperties, op->targetCRS(), op->sourceCRS(),
                           neg_x, neg_y, neg_z, neg_rx, neg_ry, neg_rz,
                           neg_scaleDiff, neg_rate_x, neg_rate_y, neg_rate_z,
                           neg_rate_rx, neg_rate_ry, neg_rate_rz,
                           neg_rate_scaleDiff, referenceEpochYear,
                           op->coordinateOperationAccuracies()))
                .as_nullable();
        } else {
            return util::nn_static_pointer_cast<CoordinateOperation>(
                       createSevenParamsTransform(
                           createPropertiesForInverse(op, false, true),
                           methodProperties, op->targetCRS(), op->sourceCRS(),
                           neg_x, neg_y, neg_z, neg_rx, neg_ry, neg_rz,
                           neg_scaleDiff, op->coordinateOperationAccuracies()))
                .as_nullable();
        }
    }

    return nullptr;
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
TransformationNNPtr
Transformation::Private::registerInv(const Transformation *thisIn,
                                     TransformationNNPtr invTransform) {
    invTransform->d->forwardOperation_ = thisIn->shallowClone().as_nullable();
    invTransform->setHasBallparkTransformation(
        thisIn->hasBallparkTransformation());
    return invTransform;
}
//! @endcond

// ---------------------------------------------------------------------------

CoordinateOperationNNPtr Transformation::inverse() const {
    return inverseAsTransformation();
}

// ---------------------------------------------------------------------------

TransformationNNPtr Transformation::inverseAsTransformation() const {

    if (d->forwardOperation_) {
        return NN_NO_CHECK(d->forwardOperation_);
    }
    const auto &l_method = method();
    const auto &methodName = l_method->nameStr();
    const int methodEPSGCode = l_method->getEPSGCode();
    const auto &l_sourceCRS = sourceCRS();
    const auto &l_targetCRS = targetCRS();

    // For geocentric translation, the inverse is exactly the negation of
    // the parameters.
    if (ci_find(methodName, "Geocentric translations") != std::string::npos ||
        methodEPSGCode == EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_GEOCENTRIC ||
        methodEPSGCode ==
            EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_GEOGRAPHIC_2D ||
        methodEPSGCode ==
            EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_GEOGRAPHIC_3D) {
        double x =
            parameterValueNumericAsSI(EPSG_CODE_PARAMETER_X_AXIS_TRANSLATION);
        double y =
            parameterValueNumericAsSI(EPSG_CODE_PARAMETER_Y_AXIS_TRANSLATION);
        double z =
            parameterValueNumericAsSI(EPSG_CODE_PARAMETER_Z_AXIS_TRANSLATION);
        auto properties = createPropertiesForInverse(this, false, false);
        return Private::registerInv(
            this, create(properties, l_targetCRS, l_sourceCRS, nullptr,
                         createMethodMapNameEPSGCode(
                             useOperationMethodEPSGCodeIfPresent(
                                 properties, methodEPSGCode)),
                         VectorOfParameters{
                             createOpParamNameEPSGCode(
                                 EPSG_CODE_PARAMETER_X_AXIS_TRANSLATION),
                             createOpParamNameEPSGCode(
                                 EPSG_CODE_PARAMETER_Y_AXIS_TRANSLATION),
                             createOpParamNameEPSGCode(
                                 EPSG_CODE_PARAMETER_Z_AXIS_TRANSLATION),
                         },
                         createParams(common::Length(negate(x)),
                                      common::Length(negate(y)),
                                      common::Length(negate(z))),
                         coordinateOperationAccuracies()));
    }

    if (methodEPSGCode == EPSG_CODE_METHOD_MOLODENSKY ||
        methodEPSGCode == EPSG_CODE_METHOD_ABRIDGED_MOLODENSKY) {
        double x =
            parameterValueNumericAsSI(EPSG_CODE_PARAMETER_X_AXIS_TRANSLATION);
        double y =
            parameterValueNumericAsSI(EPSG_CODE_PARAMETER_Y_AXIS_TRANSLATION);
        double z =
            parameterValueNumericAsSI(EPSG_CODE_PARAMETER_Z_AXIS_TRANSLATION);
        double da = parameterValueNumericAsSI(
            EPSG_CODE_PARAMETER_SEMI_MAJOR_AXIS_DIFFERENCE);
        double df = parameterValueNumericAsSI(
            EPSG_CODE_PARAMETER_FLATTENING_DIFFERENCE);

        if (methodEPSGCode == EPSG_CODE_METHOD_ABRIDGED_MOLODENSKY) {
            return Private::registerInv(
                this,
                createAbridgedMolodensky(
                    createPropertiesForInverse(this, false, false), l_targetCRS,
                    l_sourceCRS, negate(x), negate(y), negate(z), negate(da),
                    negate(df), coordinateOperationAccuracies()));
        } else {
            return Private::registerInv(
                this,
                createMolodensky(createPropertiesForInverse(this, false, false),
                                 l_targetCRS, l_sourceCRS, negate(x), negate(y),
                                 negate(z), negate(da), negate(df),
                                 coordinateOperationAccuracies()));
        }
    }

    if (isLongitudeRotation()) {
        auto offset =
            parameterValueMeasure(EPSG_CODE_PARAMETER_LONGITUDE_OFFSET);
        const common::Angle newOffset(negate(offset.value()), offset.unit());
        return Private::registerInv(
            this, createLongitudeRotation(
                      createPropertiesForInverse(this, false, false),
                      l_targetCRS, l_sourceCRS, newOffset));
    }

    if (methodEPSGCode == EPSG_CODE_METHOD_GEOGRAPHIC2D_OFFSETS) {
        auto offsetLat =
            parameterValueMeasure(EPSG_CODE_PARAMETER_LATITUDE_OFFSET);
        const common::Angle newOffsetLat(negate(offsetLat.value()),
                                         offsetLat.unit());

        auto offsetLong =
            parameterValueMeasure(EPSG_CODE_PARAMETER_LONGITUDE_OFFSET);
        const common::Angle newOffsetLong(negate(offsetLong.value()),
                                          offsetLong.unit());

        return Private::registerInv(
            this, createGeographic2DOffsets(
                      createPropertiesForInverse(this, false, false),
                      l_targetCRS, l_sourceCRS, newOffsetLat, newOffsetLong,
                      coordinateOperationAccuracies()));
    }

    if (methodEPSGCode == EPSG_CODE_METHOD_GEOGRAPHIC3D_OFFSETS) {
        auto offsetLat =
            parameterValueMeasure(EPSG_CODE_PARAMETER_LATITUDE_OFFSET);
        const common::Angle newOffsetLat(negate(offsetLat.value()),
                                         offsetLat.unit());

        auto offsetLong =
            parameterValueMeasure(EPSG_CODE_PARAMETER_LONGITUDE_OFFSET);
        const common::Angle newOffsetLong(negate(offsetLong.value()),
                                          offsetLong.unit());

        auto offsetHeight =
            parameterValueMeasure(EPSG_CODE_PARAMETER_VERTICAL_OFFSET);
        const common::Length newOffsetHeight(negate(offsetHeight.value()),
                                             offsetHeight.unit());

        return Private::registerInv(
            this, createGeographic3DOffsets(
                      createPropertiesForInverse(this, false, false),
                      l_targetCRS, l_sourceCRS, newOffsetLat, newOffsetLong,
                      newOffsetHeight, coordinateOperationAccuracies()));
    }

    if (methodEPSGCode == EPSG_CODE_METHOD_GEOGRAPHIC2D_WITH_HEIGHT_OFFSETS) {
        auto offsetLat =
            parameterValueMeasure(EPSG_CODE_PARAMETER_LATITUDE_OFFSET);
        const common::Angle newOffsetLat(negate(offsetLat.value()),
                                         offsetLat.unit());

        auto offsetLong =
            parameterValueMeasure(EPSG_CODE_PARAMETER_LONGITUDE_OFFSET);
        const common::Angle newOffsetLong(negate(offsetLong.value()),
                                          offsetLong.unit());

        auto offsetHeight =
            parameterValueMeasure(EPSG_CODE_PARAMETER_GEOID_UNDULATION);
        const common::Length newOffsetHeight(negate(offsetHeight.value()),
                                             offsetHeight.unit());

        return Private::registerInv(
            this, createGeographic2DWithHeightOffsets(
                      createPropertiesForInverse(this, false, false),
                      l_targetCRS, l_sourceCRS, newOffsetLat, newOffsetLong,
                      newOffsetHeight, coordinateOperationAccuracies()));
    }

    if (methodEPSGCode == EPSG_CODE_METHOD_VERTICAL_OFFSET) {

        auto offsetHeight =
            parameterValueMeasure(EPSG_CODE_PARAMETER_VERTICAL_OFFSET);
        const common::Length newOffsetHeight(negate(offsetHeight.value()),
                                             offsetHeight.unit());

        return Private::registerInv(
            this,
            createVerticalOffset(createPropertiesForInverse(this, false, false),
                                 l_targetCRS, l_sourceCRS, newOffsetHeight,
                                 coordinateOperationAccuracies()));
    }

    if (methodEPSGCode == EPSG_CODE_METHOD_CHANGE_VERTICAL_UNIT) {
        const double convFactor = parameterValueNumericAsSI(
            EPSG_CODE_PARAMETER_UNIT_CONVERSION_SCALAR);
        return Private::registerInv(
            this, createChangeVerticalUnit(
                      createPropertiesForInverse(this, false, false),
                      l_targetCRS, l_sourceCRS, common::Scale(1.0 / convFactor),
                      coordinateOperationAccuracies()));
    }

#ifdef notdef
    // We don't need that currently, but we might...
    if (methodEPSGCode == EPSG_CODE_METHOD_HEIGHT_DEPTH_REVERSAL) {
        return Private::registerInv(
            this,
            createHeightDepthReversal(
                createPropertiesForInverse(this, false, false), l_targetCRS,
                l_sourceCRS, coordinateOperationAccuracies()));
    }
#endif

    return InverseTransformation::create(NN_NO_CHECK(
        util::nn_dynamic_pointer_cast<Transformation>(shared_from_this())));
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress

// ---------------------------------------------------------------------------

InverseTransformation::InverseTransformation(const TransformationNNPtr &forward)
    : Transformation(
          forward->targetCRS(), forward->sourceCRS(),
          forward->interpolationCRS(),
          OperationMethod::create(createPropertiesForInverse(forward->method()),
                                  forward->method()->parameters()),
          forward->parameterValues(), forward->coordinateOperationAccuracies()),
      InverseCoordinateOperation(forward, true) {
    setPropertiesFromForward();
}

// ---------------------------------------------------------------------------

InverseTransformation::~InverseTransformation() = default;

// ---------------------------------------------------------------------------

TransformationNNPtr
InverseTransformation::create(const TransformationNNPtr &forward) {
    auto conv = util::nn_make_shared<InverseTransformation>(forward);
    conv->assignSelf(conv);
    return conv;
}

// ---------------------------------------------------------------------------

TransformationNNPtr InverseTransformation::inverseAsTransformation() const {
    return NN_NO_CHECK(
        util::nn_dynamic_pointer_cast<Transformation>(forwardOperation_));
}

// ---------------------------------------------------------------------------

void InverseTransformation::_exportToWKT(io::WKTFormatter *formatter) const {

    auto approxInverse = createApproximateInverseIfPossible(
        util::nn_dynamic_pointer_cast<Transformation>(forwardOperation_).get());
    if (approxInverse) {
        approxInverse->_exportToWKT(formatter);
    } else {
        Transformation::_exportToWKT(formatter);
    }
}

// ---------------------------------------------------------------------------

CoordinateOperationNNPtr InverseTransformation::_shallowClone() const {
    auto op = InverseTransformation::nn_make_shared<InverseTransformation>(
        inverseAsTransformation()->shallowClone());
    op->assignSelf(op);
    op->setCRSs(this, false);
    return util::nn_static_pointer_cast<CoordinateOperation>(op);
}

//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void Transformation::_exportToWKT(io::WKTFormatter *formatter) const {
    exportTransformationToWKT(formatter);
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void Transformation::_exportToJSON(
    io::JSONFormatter *formatter) const // throw(FormattingException)
{
    auto writer = formatter->writer();
    auto objectContext(formatter->MakeObjectContext(
        formatter->abridgedTransformation() ? "AbridgedTransformation"
                                            : "Transformation",
        !identifiers().empty()));

    writer->AddObjKey("name");
    auto l_name = nameStr();
    if (l_name.empty()) {
        writer->Add("unnamed");
    } else {
        writer->Add(l_name);
    }

    if (!formatter->abridgedTransformation()) {
        writer->AddObjKey("source_crs");
        formatter->setAllowIDInImmediateChild();
        sourceCRS()->_exportToJSON(formatter);

        writer->AddObjKey("target_crs");
        formatter->setAllowIDInImmediateChild();
        targetCRS()->_exportToJSON(formatter);

        const auto &l_interpolationCRS = interpolationCRS();
        if (l_interpolationCRS) {
            writer->AddObjKey("interpolation_crs");
            formatter->setAllowIDInImmediateChild();
            l_interpolationCRS->_exportToJSON(formatter);
        }
    }

    writer->AddObjKey("method");
    formatter->setOmitTypeInImmediateChild();
    formatter->setAllowIDInImmediateChild();
    method()->_exportToJSON(formatter);

    writer->AddObjKey("parameters");
    {
        auto parametersContext(writer->MakeArrayContext(false));
        for (const auto &genOpParamvalue : parameterValues()) {
            formatter->setAllowIDInImmediateChild();
            formatter->setOmitTypeInImmediateChild();
            genOpParamvalue->_exportToJSON(formatter);
        }
    }

    if (!formatter->abridgedTransformation()) {
        if (!coordinateOperationAccuracies().empty()) {
            writer->AddObjKey("accuracy");
            writer->Add(coordinateOperationAccuracies()[0]->value());
        }
    }

    if (formatter->abridgedTransformation()) {
        if (formatter->outputId()) {
            formatID(formatter);
        }
    } else {
        ObjectUsage::baseExportToJSON(formatter);
    }
}

//! @endcond

//! @cond Doxygen_Suppress
static const std::string nullString;

static const std::string &_getNTv2Filename(const Transformation *op,
                                           bool allowInverse) {

    const auto &l_method = op->method();
    if (l_method->getEPSGCode() == EPSG_CODE_METHOD_NTV2 ||
        (allowInverse &&
         ci_equal(l_method->nameStr(), INVERSE_OF + EPSG_NAME_METHOD_NTV2))) {
        const auto &fileParameter = op->parameterValue(
            EPSG_NAME_PARAMETER_LATITUDE_LONGITUDE_DIFFERENCE_FILE,
            EPSG_CODE_PARAMETER_LATITUDE_LONGITUDE_DIFFERENCE_FILE);
        if (fileParameter &&
            fileParameter->type() == ParameterValue::Type::FILENAME) {
            return fileParameter->valueFile();
        }
    }
    return nullString;
}
//! @endcond

// ---------------------------------------------------------------------------
//! @cond Doxygen_Suppress
const std::string &Transformation::getNTv2Filename() const {

    return _getNTv2Filename(this, false);
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
static const std::string &_getNTv1Filename(const Transformation *op,
                                           bool allowInverse) {

    const auto &l_method = op->method();
    const auto &methodName = l_method->nameStr();
    if (l_method->getEPSGCode() == EPSG_CODE_METHOD_NTV1 ||
        (allowInverse &&
         ci_equal(methodName, INVERSE_OF + EPSG_NAME_METHOD_NTV1))) {
        const auto &fileParameter = op->parameterValue(
            EPSG_NAME_PARAMETER_LATITUDE_LONGITUDE_DIFFERENCE_FILE,
            EPSG_CODE_PARAMETER_LATITUDE_LONGITUDE_DIFFERENCE_FILE);
        if (fileParameter &&
            fileParameter->type() == ParameterValue::Type::FILENAME) {
            return fileParameter->valueFile();
        }
    }
    return nullString;
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
static const std::string &_getCTABLE2Filename(const Transformation *op,
                                              bool allowInverse) {
    const auto &l_method = op->method();
    const auto &methodName = l_method->nameStr();
    if (ci_equal(methodName, PROJ_WKT2_NAME_METHOD_CTABLE2) ||
        (allowInverse &&
         ci_equal(methodName, INVERSE_OF + PROJ_WKT2_NAME_METHOD_CTABLE2))) {
        const auto &fileParameter = op->parameterValue(
            EPSG_NAME_PARAMETER_LATITUDE_LONGITUDE_DIFFERENCE_FILE,
            EPSG_CODE_PARAMETER_LATITUDE_LONGITUDE_DIFFERENCE_FILE);
        if (fileParameter &&
            fileParameter->type() == ParameterValue::Type::FILENAME) {
            return fileParameter->valueFile();
        }
    }
    return nullString;
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
static const std::string &
_getHorizontalShiftGTIFFFilename(const Transformation *op, bool allowInverse) {
    const auto &l_method = op->method();
    const auto &methodName = l_method->nameStr();
    if (ci_equal(methodName, PROJ_WKT2_NAME_METHOD_HORIZONTAL_SHIFT_GTIFF) ||
        (allowInverse &&
         ci_equal(methodName,
                  INVERSE_OF + PROJ_WKT2_NAME_METHOD_HORIZONTAL_SHIFT_GTIFF))) {
        const auto &fileParameter = op->parameterValue(
            EPSG_NAME_PARAMETER_LATITUDE_LONGITUDE_DIFFERENCE_FILE,
            EPSG_CODE_PARAMETER_LATITUDE_LONGITUDE_DIFFERENCE_FILE);
        if (fileParameter &&
            fileParameter->type() == ParameterValue::Type::FILENAME) {
            return fileParameter->valueFile();
        }
    }
    return nullString;
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
static const std::string &
_getGeocentricTranslationFilename(const Transformation *op, bool allowInverse) {

    const auto &l_method = op->method();
    const auto &methodName = l_method->nameStr();
    if (l_method->getEPSGCode() ==
            EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_BY_GRID_INTERPOLATION_IGN ||
        (allowInverse &&
         ci_equal(
             methodName,
             INVERSE_OF +
                 EPSG_NAME_METHOD_GEOCENTRIC_TRANSLATION_BY_GRID_INTERPOLATION_IGN))) {
        const auto &fileParameter =
            op->parameterValue(EPSG_NAME_PARAMETER_GEOCENTRIC_TRANSLATION_FILE,
                               EPSG_CODE_PARAMETER_GEOCENTRIC_TRANSLATION_FILE);
        if (fileParameter &&
            fileParameter->type() == ParameterValue::Type::FILENAME) {
            return fileParameter->valueFile();
        }
    }
    return nullString;
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
static const std::string &
_getHeightToGeographic3DFilename(const Transformation *op, bool allowInverse) {

    const auto &methodName = op->method()->nameStr();

    if (ci_equal(methodName, PROJ_WKT2_NAME_METHOD_HEIGHT_TO_GEOG3D) ||
        (allowInverse &&
         ci_equal(methodName,
                  INVERSE_OF + PROJ_WKT2_NAME_METHOD_HEIGHT_TO_GEOG3D))) {
        const auto &fileParameter =
            op->parameterValue(EPSG_NAME_PARAMETER_GEOID_CORRECTION_FILENAME,
                               EPSG_CODE_PARAMETER_GEOID_CORRECTION_FILENAME);
        if (fileParameter &&
            fileParameter->type() == ParameterValue::Type::FILENAME) {
            return fileParameter->valueFile();
        }
    }
    return nullString;
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
static bool
isGeographic3DToGravityRelatedHeight(const OperationMethodNNPtr &method,
                                     bool allowInverse) {
    const auto &methodName = method->nameStr();
    static const char *const methodCodes[] = {
        "1025", // Geographic3D to GravityRelatedHeight (EGM2008)
        "1030", // Geographic3D to GravityRelatedHeight (NZgeoid)
        "1045", // Geographic3D to GravityRelatedHeight (OSGM02-Ire)
        "1047", // Geographic3D to GravityRelatedHeight (Gravsoft)
        "1048", // Geographic3D to GravityRelatedHeight (Ausgeoid v2)
        "1050", // Geographic3D to GravityRelatedHeight (CI)
        "1059", // Geographic3D to GravityRelatedHeight (PNG)
        "1088", // Geog3D to Geog2D+GravityRelatedHeight (gtx)
        "1060", // Geographic3D to GravityRelatedHeight (CGG2013)
        "1072", // Geographic3D to GravityRelatedHeight (OSGM15-Ire)
        "1073", // Geographic3D to GravityRelatedHeight (IGN2009)
        "1081", // Geographic3D to GravityRelatedHeight (BEV AT)
        "1083", // Geog3D to Geog2D+Vertical (AUSGeoid v2)
        "1089", // Geog3D to Geog2D+GravityRelatedHeight (BEV AT)
        "1090", // Geog3D to Geog2D+GravityRelatedHeight (CGG 2013)
        "1091", // Geog3D to Geog2D+GravityRelatedHeight (CI)
        "1092", // Geog3D to Geog2D+GravityRelatedHeight (EGM2008)
        "1093", // Geog3D to Geog2D+GravityRelatedHeight (Gravsoft)
        "1094", // Geog3D to Geog2D+GravityRelatedHeight (IGN1997)
        "1095", // Geog3D to Geog2D+GravityRelatedHeight (IGN2009)
        "1096", // Geog3D to Geog2D+GravityRelatedHeight (OSGM15-Ire)
        "1097", // Geog3D to Geog2D+GravityRelatedHeight (OSGM-GB)
        "1098", // Geog3D to Geog2D+GravityRelatedHeight (SA 2010)
        "1100", // Geog3D to Geog2D+GravityRelatedHeight (PL txt)
        "1103", // Geog3D to Geog2D+GravityRelatedHeight (EGM)
        "1105", // Geog3D to Geog2D+GravityRelatedHeight (ITAL2005)
        "9661", // Geographic3D to GravityRelatedHeight (EGM)
        "9662", // Geographic3D to GravityRelatedHeight (Ausgeoid98)
        "9663", // Geographic3D to GravityRelatedHeight (OSGM-GB)
        "9664", // Geographic3D to GravityRelatedHeight (IGN1997)
        "9665", // Geographic3D to GravityRelatedHeight (US .gtx)
        "9635", // Geog3D to Geog2D+GravityRelatedHeight (US .gtx)
    };

    if (ci_find(methodName, "Geographic3D to GravityRelatedHeight") == 0) {
        return true;
    }
    if (allowInverse &&
        ci_find(methodName,
                INVERSE_OF + "Geographic3D to GravityRelatedHeight") == 0) {
        return true;
    }

    for (const auto &code : methodCodes) {
        for (const auto &idSrc : method->identifiers()) {
            const auto &srcAuthName = *(idSrc->codeSpace());
            const auto &srcCode = idSrc->code();
            if (ci_equal(srcAuthName, "EPSG") && srcCode == code) {
                return true;
            }
            if (allowInverse && ci_equal(srcAuthName, "INVERSE(EPSG)") &&
                srcCode == code) {
                return true;
            }
        }
    }
    return false;
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
const std::string &Transformation::getHeightToGeographic3DFilename() const {

    const std::string &ret = _getHeightToGeographic3DFilename(this, false);
    if (!ret.empty())
        return ret;
    if (isGeographic3DToGravityRelatedHeight(method(), false)) {
        const auto &fileParameter =
            parameterValue(EPSG_NAME_PARAMETER_GEOID_CORRECTION_FILENAME,
                           EPSG_CODE_PARAMETER_GEOID_CORRECTION_FILENAME);
        if (fileParameter &&
            fileParameter->type() == ParameterValue::Type::FILENAME) {
            return fileParameter->valueFile();
        }
    }
    return nullString;
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
static util::PropertyMap
createSimilarPropertiesMethod(common::IdentifiedObjectNNPtr obj) {
    util::PropertyMap map;

    const std::string &forwardName = obj->nameStr();
    if (!forwardName.empty()) {
        map.set(common::IdentifiedObject::NAME_KEY, forwardName);
    }

    {
        auto ar = util::ArrayOfBaseObject::create();
        for (const auto &idSrc : obj->identifiers()) {
            const auto &srcAuthName = *(idSrc->codeSpace());
            const auto &srcCode = idSrc->code();
            auto idsProp = util::PropertyMap().set(
                metadata::Identifier::CODESPACE_KEY, srcAuthName);
            ar->add(metadata::Identifier::create(srcCode, idsProp));
        }
        if (!ar->empty()) {
            map.set(common::IdentifiedObject::IDENTIFIERS_KEY, ar);
        }
    }

    return map;
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
static util::PropertyMap
createSimilarPropertiesTransformation(TransformationNNPtr obj) {
    util::PropertyMap map;

    // The domain(s) are unchanged
    addDomains(map, obj.get());

    const std::string &forwardName = obj->nameStr();
    if (!forwardName.empty()) {
        map.set(common::IdentifiedObject::NAME_KEY, forwardName);
    }

    const std::string &remarks = obj->remarks();
    if (!remarks.empty()) {
        map.set(common::IdentifiedObject::REMARKS_KEY, remarks);
    }

    addModifiedIdentifier(map, obj.get(), false, true);

    return map;
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
static TransformationNNPtr
createNTv1(const util::PropertyMap &properties,
           const crs::CRSNNPtr &sourceCRSIn, const crs::CRSNNPtr &targetCRSIn,
           const std::string &filename,
           const std::vector<metadata::PositionalAccuracyNNPtr> &accuracies) {
    return Transformation::create(
        properties, sourceCRSIn, targetCRSIn, nullptr,
        createMethodMapNameEPSGCode(EPSG_CODE_METHOD_NTV1),
        {OperationParameter::create(
            util::PropertyMap()
                .set(common::IdentifiedObject::NAME_KEY,
                     EPSG_NAME_PARAMETER_LATITUDE_LONGITUDE_DIFFERENCE_FILE)
                .set(metadata::Identifier::CODESPACE_KEY,
                     metadata::Identifier::EPSG)
                .set(metadata::Identifier::CODE_KEY,
                     EPSG_CODE_PARAMETER_LATITUDE_LONGITUDE_DIFFERENCE_FILE))},
        {ParameterValue::createFilename(filename)}, accuracies);
}
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Return an equivalent transformation to the current one, but using
 * PROJ alternative grid names.
 */
TransformationNNPtr Transformation::substitutePROJAlternativeGridNames(
    io::DatabaseContextNNPtr databaseContext) const {
    auto self = NN_NO_CHECK(std::dynamic_pointer_cast<Transformation>(
        shared_from_this().as_nullable()));

    const auto &l_method = method();
    const int methodEPSGCode = l_method->getEPSGCode();

    std::string projFilename;
    std::string projGridFormat;
    bool inverseDirection = false;

    const auto &NTv1Filename = _getNTv1Filename(this, false);
    const auto &NTv2Filename = _getNTv2Filename(this, false);
    std::string lasFilename;
    if (methodEPSGCode == EPSG_CODE_METHOD_NADCON) {
        const auto &latitudeFileParameter =
            parameterValue(EPSG_NAME_PARAMETER_LATITUDE_DIFFERENCE_FILE,
                           EPSG_CODE_PARAMETER_LATITUDE_DIFFERENCE_FILE);
        const auto &longitudeFileParameter =
            parameterValue(EPSG_NAME_PARAMETER_LONGITUDE_DIFFERENCE_FILE,
                           EPSG_CODE_PARAMETER_LONGITUDE_DIFFERENCE_FILE);
        if (latitudeFileParameter &&
            latitudeFileParameter->type() == ParameterValue::Type::FILENAME &&
            longitudeFileParameter &&
            longitudeFileParameter->type() == ParameterValue::Type::FILENAME) {
            lasFilename = latitudeFileParameter->valueFile();
        }
    }
    const auto &horizontalGridName =
        !NTv1Filename.empty()
            ? NTv1Filename
            : !NTv2Filename.empty() ? NTv2Filename : lasFilename;

    if (!horizontalGridName.empty() && databaseContext->lookForGridAlternative(
                                           horizontalGridName, projFilename,
                                           projGridFormat, inverseDirection)) {

        if (horizontalGridName == projFilename) {
            if (inverseDirection) {
                throw util::UnsupportedOperationException(
                    "Inverse direction for " + projFilename + " not supported");
            }
            return self;
        }

        const auto &l_sourceCRS = sourceCRS();
        const auto &l_targetCRS = targetCRS();
        const auto &l_accuracies = coordinateOperationAccuracies();
        if (projGridFormat == "GTiff") {
            auto parameters =
                std::vector<OperationParameterNNPtr>{createOpParamNameEPSGCode(
                    EPSG_CODE_PARAMETER_LATITUDE_LONGITUDE_DIFFERENCE_FILE)};
            auto methodProperties = util::PropertyMap().set(
                common::IdentifiedObject::NAME_KEY,
                PROJ_WKT2_NAME_METHOD_HORIZONTAL_SHIFT_GTIFF);
            auto values = std::vector<ParameterValueNNPtr>{
                ParameterValue::createFilename(projFilename)};
            if (inverseDirection) {
                return create(createPropertiesForInverse(
                                  self.as_nullable().get(), true, false),
                              l_targetCRS, l_sourceCRS, nullptr,
                              methodProperties, parameters, values,
                              l_accuracies)
                    ->inverseAsTransformation();

            } else {
                return create(createSimilarPropertiesTransformation(self),
                              l_sourceCRS, l_targetCRS, nullptr,
                              methodProperties, parameters, values,
                              l_accuracies);
            }
        } else if (projGridFormat == "NTv1") {
            if (inverseDirection) {
                return createNTv1(createPropertiesForInverse(
                                      self.as_nullable().get(), true, false),
                                  l_targetCRS, l_sourceCRS, projFilename,
                                  l_accuracies)
                    ->inverseAsTransformation();
            } else {
                return createNTv1(createSimilarPropertiesTransformation(self),
                                  l_sourceCRS, l_targetCRS, projFilename,
                                  l_accuracies);
            }
        } else if (projGridFormat == "NTv2") {
            if (inverseDirection) {
                return createNTv2(createPropertiesForInverse(
                                      self.as_nullable().get(), true, false),
                                  l_targetCRS, l_sourceCRS, projFilename,
                                  l_accuracies)
                    ->inverseAsTransformation();
            } else {
                return createNTv2(createSimilarPropertiesTransformation(self),
                                  l_sourceCRS, l_targetCRS, projFilename,
                                  l_accuracies);
            }
        } else if (projGridFormat == "CTable2") {
            auto parameters =
                std::vector<OperationParameterNNPtr>{createOpParamNameEPSGCode(
                    EPSG_CODE_PARAMETER_LATITUDE_LONGITUDE_DIFFERENCE_FILE)};
            auto methodProperties =
                util::PropertyMap().set(common::IdentifiedObject::NAME_KEY,
                                        PROJ_WKT2_NAME_METHOD_CTABLE2);
            auto values = std::vector<ParameterValueNNPtr>{
                ParameterValue::createFilename(projFilename)};
            if (inverseDirection) {
                return create(createPropertiesForInverse(
                                  self.as_nullable().get(), true, false),
                              l_targetCRS, l_sourceCRS, nullptr,
                              methodProperties, parameters, values,
                              l_accuracies)
                    ->inverseAsTransformation();

            } else {
                return create(createSimilarPropertiesTransformation(self),
                              l_sourceCRS, l_targetCRS, nullptr,
                              methodProperties, parameters, values,
                              l_accuracies);
            }
        }
    }

    if (isGeographic3DToGravityRelatedHeight(method(), false)) {
        const auto &fileParameter =
            parameterValue(EPSG_NAME_PARAMETER_GEOID_CORRECTION_FILENAME,
                           EPSG_CODE_PARAMETER_GEOID_CORRECTION_FILENAME);
        if (fileParameter &&
            fileParameter->type() == ParameterValue::Type::FILENAME) {
            auto filename = fileParameter->valueFile();
            if (databaseContext->lookForGridAlternative(
                    filename, projFilename, projGridFormat, inverseDirection)) {

                if (inverseDirection) {
                    throw util::UnsupportedOperationException(
                        "Inverse direction for "
                        "Geographic3DToGravityRelatedHeight not supported");
                }

                if (filename == projFilename) {
                    return self;
                }

                auto parameters = std::vector<OperationParameterNNPtr>{
                    createOpParamNameEPSGCode(
                        EPSG_CODE_PARAMETER_GEOID_CORRECTION_FILENAME)};
#ifdef disabled_for_now
                if (inverseDirection) {
                    return create(
                               createPropertiesForInverse(
                                   self.as_nullable().get(), true, false),
                               targetCRS(), sourceCRS(), nullptr,
                               createSimilarPropertiesMethod(method()),
                               parameters,
                               {ParameterValue::createFilename(projFilename)},
                               coordinateOperationAccuracies())
                        ->inverseAsTransformation();
                } else
#endif
                {
                    return create(
                        createSimilarPropertiesTransformation(self),
                        sourceCRS(), targetCRS(), nullptr,
                        createSimilarPropertiesMethod(method()), parameters,
                        {ParameterValue::createFilename(projFilename)},
                        coordinateOperationAccuracies());
                }
            }
        }
    }

    const auto &geocentricTranslationFilename =
        _getGeocentricTranslationFilename(this, false);
    if (!geocentricTranslationFilename.empty()) {
        if (databaseContext->lookForGridAlternative(
                geocentricTranslationFilename, projFilename, projGridFormat,
                inverseDirection)) {

            if (inverseDirection) {
                throw util::UnsupportedOperationException(
                    "Inverse direction for "
                    "GeocentricTranslation not supported");
            }

            if (geocentricTranslationFilename == projFilename) {
                return self;
            }

            auto parameters =
                std::vector<OperationParameterNNPtr>{createOpParamNameEPSGCode(
                    EPSG_CODE_PARAMETER_GEOCENTRIC_TRANSLATION_FILE)};
            return create(createSimilarPropertiesTransformation(self),
                          sourceCRS(), targetCRS(), interpolationCRS(),
                          createSimilarPropertiesMethod(method()), parameters,
                          {ParameterValue::createFilename(projFilename)},
                          coordinateOperationAccuracies());
        }
    }

    if (methodEPSGCode == EPSG_CODE_METHOD_VERTCON ||
        methodEPSGCode == EPSG_CODE_METHOD_VERTICALGRID_NZLVD ||
        methodEPSGCode == EPSG_CODE_METHOD_VERTICALGRID_BEV_AT ||
        methodEPSGCode == EPSG_CODE_METHOD_VERTICALGRID_GTX ||
        methodEPSGCode == EPSG_CODE_METHOD_VERTICALGRID_PL_TXT) {
        auto fileParameter =
            parameterValue(EPSG_NAME_PARAMETER_VERTICAL_OFFSET_FILE,
                           EPSG_CODE_PARAMETER_VERTICAL_OFFSET_FILE);
        if (fileParameter &&
            fileParameter->type() == ParameterValue::Type::FILENAME) {

            auto filename = fileParameter->valueFile();
            if (databaseContext->lookForGridAlternative(
                    filename, projFilename, projGridFormat, inverseDirection)) {

                if (filename == projFilename) {
                    if (inverseDirection) {
                        throw util::UnsupportedOperationException(
                            "Inverse direction for " + projFilename +
                            " not supported");
                    }
                    return self;
                }

                auto parameters = std::vector<OperationParameterNNPtr>{
                    createOpParamNameEPSGCode(
                        EPSG_CODE_PARAMETER_VERTICAL_OFFSET_FILE)};
                if (inverseDirection) {
                    return create(
                               createPropertiesForInverse(
                                   self.as_nullable().get(), true, false),
                               targetCRS(), sourceCRS(), nullptr,
                               createSimilarPropertiesMethod(method()),
                               parameters,
                               {ParameterValue::createFilename(projFilename)},
                               coordinateOperationAccuracies())
                        ->inverseAsTransformation();
                } else {
                    return create(
                        createSimilarPropertiesTransformation(self),
                        sourceCRS(), targetCRS(), nullptr,
                        createSimilarPropertiesMethod(method()), parameters,
                        {ParameterValue::createFilename(projFilename)},
                        coordinateOperationAccuracies());
                }
            }
        }
    }

    return self;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress

static void ThrowExceptionNotGeodeticGeographic(const char *trfrm_name) {
    throw io::FormattingException(concat("Can apply ", std::string(trfrm_name),
                                         " only to GeodeticCRS / "
                                         "GeographicCRS"));
}

// ---------------------------------------------------------------------------

// If crs is a geographic CRS, or a compound CRS of a geographic CRS,
// or a compoundCRS of a bound CRS of a geographic CRS, return that
// geographic CRS
static crs::GeographicCRSPtr
extractGeographicCRSIfGeographicCRSOrEquivalent(const crs::CRSNNPtr &crs) {
    auto geogCRS = util::nn_dynamic_pointer_cast<crs::GeographicCRS>(crs);
    if (!geogCRS) {
        auto compoundCRS = util::nn_dynamic_pointer_cast<crs::CompoundCRS>(crs);
        if (compoundCRS) {
            const auto &components = compoundCRS->componentReferenceSystems();
            if (!components.empty()) {
                geogCRS = util::nn_dynamic_pointer_cast<crs::GeographicCRS>(
                    components[0]);
                if (!geogCRS) {
                    auto boundCRS =
                        util::nn_dynamic_pointer_cast<crs::BoundCRS>(
                            components[0]);
                    if (boundCRS) {
                        geogCRS =
                            util::nn_dynamic_pointer_cast<crs::GeographicCRS>(
                                boundCRS->baseCRS());
                    }
                }
            }
        } else {
            auto boundCRS = util::nn_dynamic_pointer_cast<crs::BoundCRS>(crs);
            if (boundCRS) {
                geogCRS = util::nn_dynamic_pointer_cast<crs::GeographicCRS>(
                    boundCRS->baseCRS());
            }
        }
    }
    return geogCRS;
}

// ---------------------------------------------------------------------------

static void setupPROJGeodeticSourceCRS(io::PROJStringFormatter *formatter,
                                       const crs::CRSNNPtr &crs, bool addPushV3,
                                       const char *trfrm_name) {
    auto sourceCRSGeog = extractGeographicCRSIfGeographicCRSOrEquivalent(crs);
    if (sourceCRSGeog) {
        formatter->startInversion();
        sourceCRSGeog->_exportToPROJString(formatter);
        formatter->stopInversion();
        if (util::isOfExactType<crs::DerivedGeographicCRS>(
                *(sourceCRSGeog.get()))) {
            // The export of a DerivedGeographicCRS in non-CRS mode adds
            // unit conversion and axis swapping. We must compensate for that
            formatter->startInversion();
            sourceCRSGeog->addAngularUnitConvertAndAxisSwap(formatter);
            formatter->stopInversion();
        }

        if (addPushV3) {
            formatter->addStep("push");
            formatter->addParam("v_3");
        }

        formatter->addStep("cart");
        sourceCRSGeog->ellipsoid()->_exportToPROJString(formatter);
    } else {
        auto sourceCRSGeod = dynamic_cast<const crs::GeodeticCRS *>(crs.get());
        if (!sourceCRSGeod) {
            ThrowExceptionNotGeodeticGeographic(trfrm_name);
        }
        formatter->startInversion();
        sourceCRSGeod->addGeocentricUnitConversionIntoPROJString(formatter);
        formatter->stopInversion();
    }
}
// ---------------------------------------------------------------------------

static void setupPROJGeodeticTargetCRS(io::PROJStringFormatter *formatter,
                                       const crs::CRSNNPtr &crs, bool addPopV3,
                                       const char *trfrm_name) {
    auto targetCRSGeog = extractGeographicCRSIfGeographicCRSOrEquivalent(crs);
    if (targetCRSGeog) {
        formatter->addStep("cart");
        formatter->setCurrentStepInverted(true);
        targetCRSGeog->ellipsoid()->_exportToPROJString(formatter);

        if (addPopV3) {
            formatter->addStep("pop");
            formatter->addParam("v_3");
        }
        if (util::isOfExactType<crs::DerivedGeographicCRS>(
                *(targetCRSGeog.get()))) {
            // The export of a DerivedGeographicCRS in non-CRS mode adds
            // unit conversion and axis swapping. We must compensate for that
            targetCRSGeog->addAngularUnitConvertAndAxisSwap(formatter);
        }
        targetCRSGeog->_exportToPROJString(formatter);
    } else {
        auto targetCRSGeod = dynamic_cast<const crs::GeodeticCRS *>(crs.get());
        if (!targetCRSGeod) {
            ThrowExceptionNotGeodeticGeographic(trfrm_name);
        }
        targetCRSGeod->addGeocentricUnitConversionIntoPROJString(formatter);
    }
}

//! @endcond

// ---------------------------------------------------------------------------

void Transformation::_exportToPROJString(
    io::PROJStringFormatter *formatter) const // throw(FormattingException)
{
    if (formatter->convention() ==
        io::PROJStringFormatter::Convention::PROJ_4) {
        throw io::FormattingException(
            "Transformation cannot be exported as a PROJ.4 string");
    }

    formatter->setCoordinateOperationOptimizations(true);

    bool positionVectorConvention = true;
    bool sevenParamsTransform = false;
    bool threeParamsTransform = false;
    bool fifteenParamsTransform = false;
    const auto &l_method = method();
    const int methodEPSGCode = l_method->getEPSGCode();
    const auto &methodName = l_method->nameStr();
    const auto paramCount = parameterValues().size();
    const bool l_isTimeDependent = isTimeDependent(methodName);
    const bool isPositionVector =
        ci_find(methodName, "Position Vector") != std::string::npos ||
        ci_find(methodName, "PV") != std::string::npos;
    const bool isCoordinateFrame =
        ci_find(methodName, "Coordinate Frame") != std::string::npos ||
        ci_find(methodName, "CF") != std::string::npos;
    if ((paramCount == 7 && isCoordinateFrame && !l_isTimeDependent) ||
        methodEPSGCode == EPSG_CODE_METHOD_COORDINATE_FRAME_GEOCENTRIC ||
        methodEPSGCode == EPSG_CODE_METHOD_COORDINATE_FRAME_GEOGRAPHIC_2D ||
        methodEPSGCode == EPSG_CODE_METHOD_COORDINATE_FRAME_GEOGRAPHIC_3D) {
        positionVectorConvention = false;
        sevenParamsTransform = true;
    } else if (
        (paramCount == 15 && isCoordinateFrame && l_isTimeDependent) ||
        methodEPSGCode ==
            EPSG_CODE_METHOD_TIME_DEPENDENT_COORDINATE_FRAME_GEOCENTRIC ||
        methodEPSGCode ==
            EPSG_CODE_METHOD_TIME_DEPENDENT_COORDINATE_FRAME_GEOGRAPHIC_2D ||
        methodEPSGCode ==
            EPSG_CODE_METHOD_TIME_DEPENDENT_COORDINATE_FRAME_GEOGRAPHIC_3D) {
        positionVectorConvention = false;
        fifteenParamsTransform = true;
    } else if ((paramCount == 7 && isPositionVector && !l_isTimeDependent) ||
               methodEPSGCode == EPSG_CODE_METHOD_POSITION_VECTOR_GEOCENTRIC ||
               methodEPSGCode ==
                   EPSG_CODE_METHOD_POSITION_VECTOR_GEOGRAPHIC_2D ||
               methodEPSGCode ==
                   EPSG_CODE_METHOD_POSITION_VECTOR_GEOGRAPHIC_3D) {
        sevenParamsTransform = true;
    } else if (
        (paramCount == 15 && isPositionVector && l_isTimeDependent) ||
        methodEPSGCode ==
            EPSG_CODE_METHOD_TIME_DEPENDENT_POSITION_VECTOR_GEOCENTRIC ||
        methodEPSGCode ==
            EPSG_CODE_METHOD_TIME_DEPENDENT_POSITION_VECTOR_GEOGRAPHIC_2D ||
        methodEPSGCode ==
            EPSG_CODE_METHOD_TIME_DEPENDENT_POSITION_VECTOR_GEOGRAPHIC_3D) {
        fifteenParamsTransform = true;
    } else if ((paramCount == 3 &&
                ci_find(methodName, "Geocentric translations") !=
                    std::string::npos) ||
               methodEPSGCode ==
                   EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_GEOCENTRIC ||
               methodEPSGCode ==
                   EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_GEOGRAPHIC_2D ||
               methodEPSGCode ==
                   EPSG_CODE_METHOD_GEOCENTRIC_TRANSLATION_GEOGRAPHIC_3D) {
        threeParamsTransform = true;
    }
    if (threeParamsTransform || sevenParamsTransform ||
        fifteenParamsTransform) {
        double x =
            parameterValueNumericAsSI(EPSG_CODE_PARAMETER_X_AXIS_TRANSLATION);
        double y =
            parameterValueNumericAsSI(EPSG_CODE_PARAMETER_Y_AXIS_TRANSLATION);
        double z =
            parameterValueNumericAsSI(EPSG_CODE_PARAMETER_Z_AXIS_TRANSLATION);

        auto sourceCRSGeog =
            dynamic_cast<const crs::GeographicCRS *>(sourceCRS().get());
        auto targetCRSGeog =
            dynamic_cast<const crs::GeographicCRS *>(targetCRS().get());
        const bool addPushPopV3 =
            ((sourceCRSGeog &&
              sourceCRSGeog->coordinateSystem()->axisList().size() == 2) ||
             (targetCRSGeog &&
              targetCRSGeog->coordinateSystem()->axisList().size() == 2));

        setupPROJGeodeticSourceCRS(formatter, sourceCRS(), addPushPopV3,
                                   "Helmert");

        formatter->addStep("helmert");
        formatter->addParam("x", x);
        formatter->addParam("y", y);
        formatter->addParam("z", z);
        if (sevenParamsTransform || fifteenParamsTransform) {
            double rx =
                parameterValueNumeric(EPSG_CODE_PARAMETER_X_AXIS_ROTATION,
                                      common::UnitOfMeasure::ARC_SECOND);
            double ry =
                parameterValueNumeric(EPSG_CODE_PARAMETER_Y_AXIS_ROTATION,
                                      common::UnitOfMeasure::ARC_SECOND);
            double rz =
                parameterValueNumeric(EPSG_CODE_PARAMETER_Z_AXIS_ROTATION,
                                      common::UnitOfMeasure::ARC_SECOND);
            double scaleDiff =
                parameterValueNumeric(EPSG_CODE_PARAMETER_SCALE_DIFFERENCE,
                                      common::UnitOfMeasure::PARTS_PER_MILLION);
            formatter->addParam("rx", rx);
            formatter->addParam("ry", ry);
            formatter->addParam("rz", rz);
            formatter->addParam("s", scaleDiff);
            if (fifteenParamsTransform) {
                double rate_x = parameterValueNumeric(
                    EPSG_CODE_PARAMETER_RATE_X_AXIS_TRANSLATION,
                    common::UnitOfMeasure::METRE_PER_YEAR);
                double rate_y = parameterValueNumeric(
                    EPSG_CODE_PARAMETER_RATE_Y_AXIS_TRANSLATION,
                    common::UnitOfMeasure::METRE_PER_YEAR);
                double rate_z = parameterValueNumeric(
                    EPSG_CODE_PARAMETER_RATE_Z_AXIS_TRANSLATION,
                    common::UnitOfMeasure::METRE_PER_YEAR);
                double rate_rx = parameterValueNumeric(
                    EPSG_CODE_PARAMETER_RATE_X_AXIS_ROTATION,
                    common::UnitOfMeasure::ARC_SECOND_PER_YEAR);
                double rate_ry = parameterValueNumeric(
                    EPSG_CODE_PARAMETER_RATE_Y_AXIS_ROTATION,
                    common::UnitOfMeasure::ARC_SECOND_PER_YEAR);
                double rate_rz = parameterValueNumeric(
                    EPSG_CODE_PARAMETER_RATE_Z_AXIS_ROTATION,
                    common::UnitOfMeasure::ARC_SECOND_PER_YEAR);
                double rate_scaleDiff = parameterValueNumeric(
                    EPSG_CODE_PARAMETER_RATE_SCALE_DIFFERENCE,
                    common::UnitOfMeasure::PPM_PER_YEAR);
                double referenceEpochYear =
                    parameterValueNumeric(EPSG_CODE_PARAMETER_REFERENCE_EPOCH,
                                          common::UnitOfMeasure::YEAR);
                formatter->addParam("dx", rate_x);
                formatter->addParam("dy", rate_y);
                formatter->addParam("dz", rate_z);
                formatter->addParam("drx", rate_rx);
                formatter->addParam("dry", rate_ry);
                formatter->addParam("drz", rate_rz);
                formatter->addParam("ds", rate_scaleDiff);
                formatter->addParam("t_epoch", referenceEpochYear);
            }
            if (positionVectorConvention) {
                formatter->addParam("convention", "position_vector");
            } else {
                formatter->addParam("convention", "coordinate_frame");
            }
        }

        setupPROJGeodeticTargetCRS(formatter, targetCRS(), addPushPopV3,
                                   "Helmert");

        return;
    }

    if (methodEPSGCode == EPSG_CODE_METHOD_MOLODENSKY_BADEKAS_CF_GEOCENTRIC ||
        methodEPSGCode == EPSG_CODE_METHOD_MOLODENSKY_BADEKAS_PV_GEOCENTRIC ||
        methodEPSGCode ==
            EPSG_CODE_METHOD_MOLODENSKY_BADEKAS_CF_GEOGRAPHIC_3D ||
        methodEPSGCode ==
            EPSG_CODE_METHOD_MOLODENSKY_BADEKAS_PV_GEOGRAPHIC_3D ||
        methodEPSGCode ==
            EPSG_CODE_METHOD_MOLODENSKY_BADEKAS_CF_GEOGRAPHIC_2D ||
        methodEPSGCode ==
            EPSG_CODE_METHOD_MOLODENSKY_BADEKAS_PV_GEOGRAPHIC_2D) {

        positionVectorConvention =
            isPositionVector ||
            methodEPSGCode ==
                EPSG_CODE_METHOD_MOLODENSKY_BADEKAS_PV_GEOCENTRIC ||
            methodEPSGCode ==
                EPSG_CODE_METHOD_MOLODENSKY_BADEKAS_PV_GEOGRAPHIC_3D ||
            methodEPSGCode ==
                EPSG_CODE_METHOD_MOLODENSKY_BADEKAS_PV_GEOGRAPHIC_2D;

        double x =
            parameterValueNumericAsSI(EPSG_CODE_PARAMETER_X_AXIS_TRANSLATION);
        double y =
            parameterValueNumericAsSI(EPSG_CODE_PARAMETER_Y_AXIS_TRANSLATION);
        double z =
            parameterValueNumericAsSI(EPSG_CODE_PARAMETER_Z_AXIS_TRANSLATION);
        double rx = parameterValueNumeric(EPSG_CODE_PARAMETER_X_AXIS_ROTATION,
                                          common::UnitOfMeasure::ARC_SECOND);
        double ry = parameterValueNumeric(EPSG_CODE_PARAMETER_Y_AXIS_ROTATION,
                                          common::UnitOfMeasure::ARC_SECOND);
        double rz = parameterValueNumeric(EPSG_CODE_PARAMETER_Z_AXIS_ROTATION,
                                          common::UnitOfMeasure::ARC_SECOND);
        double scaleDiff =
            parameterValueNumeric(EPSG_CODE_PARAMETER_SCALE_DIFFERENCE,
                                  common::UnitOfMeasure::PARTS_PER_MILLION);

        double px = parameterValueNumericAsSI(
            EPSG_CODE_PARAMETER_ORDINATE_1_EVAL_POINT);
        double py = parameterValueNumericAsSI(
            EPSG_CODE_PARAMETER_ORDINATE_2_EVAL_POINT);
        double pz = parameterValueNumericAsSI(
            EPSG_CODE_PARAMETER_ORDINATE_3_EVAL_POINT);

        bool addPushPopV3 =
            (methodEPSGCode ==
                 EPSG_CODE_METHOD_MOLODENSKY_BADEKAS_PV_GEOGRAPHIC_2D ||
             methodEPSGCode ==
                 EPSG_CODE_METHOD_MOLODENSKY_BADEKAS_CF_GEOGRAPHIC_2D);

        setupPROJGeodeticSourceCRS(formatter, sourceCRS(), addPushPopV3,
                                   "Molodensky-Badekas");

        formatter->addStep("molobadekas");
        formatter->addParam("x", x);
        formatter->addParam("y", y);
        formatter->addParam("z", z);
        formatter->addParam("rx", rx);
        formatter->addParam("ry", ry);
        formatter->addParam("rz", rz);
        formatter->addParam("s", scaleDiff);
        formatter->addParam("px", px);
        formatter->addParam("py", py);
        formatter->addParam("pz", pz);
        if (positionVectorConvention) {
            formatter->addParam("convention", "position_vector");
        } else {
            formatter->addParam("convention", "coordinate_frame");
        }

        setupPROJGeodeticTargetCRS(formatter, targetCRS(), addPushPopV3,
                                   "Molodensky-Badekas");

        return;
    }

    if (methodEPSGCode == EPSG_CODE_METHOD_MOLODENSKY ||
        methodEPSGCode == EPSG_CODE_METHOD_ABRIDGED_MOLODENSKY) {
        double x =
            parameterValueNumericAsSI(EPSG_CODE_PARAMETER_X_AXIS_TRANSLATION);
        double y =
            parameterValueNumericAsSI(EPSG_CODE_PARAMETER_Y_AXIS_TRANSLATION);
        double z =
            parameterValueNumericAsSI(EPSG_CODE_PARAMETER_Z_AXIS_TRANSLATION);
        double da = parameterValueNumericAsSI(
            EPSG_CODE_PARAMETER_SEMI_MAJOR_AXIS_DIFFERENCE);
        double df = parameterValueNumericAsSI(
            EPSG_CODE_PARAMETER_FLATTENING_DIFFERENCE);

        auto sourceCRSGeog =
            dynamic_cast<const crs::GeographicCRS *>(sourceCRS().get());
        if (!sourceCRSGeog) {
            throw io::FormattingException(
                "Can apply Molodensky only to GeographicCRS");
        }

        auto targetCRSGeog =
            dynamic_cast<const crs::GeographicCRS *>(targetCRS().get());
        if (!targetCRSGeog) {
            throw io::FormattingException(
                "Can apply Molodensky only to GeographicCRS");
        }

        formatter->startInversion();
        sourceCRSGeog->_exportToPROJString(formatter);
        formatter->stopInversion();

        formatter->addStep("molodensky");
        sourceCRSGeog->ellipsoid()->_exportToPROJString(formatter);
        formatter->addParam("dx", x);
        formatter->addParam("dy", y);
        formatter->addParam("dz", z);
        formatter->addParam("da", da);
        formatter->addParam("df", df);

        if (ci_find(methodName, "Abridged") != std::string::npos ||
            methodEPSGCode == EPSG_CODE_METHOD_ABRIDGED_MOLODENSKY) {
            formatter->addParam("abridged");
        }

        targetCRSGeog->_exportToPROJString(formatter);

        return;
    }

    if (methodEPSGCode == EPSG_CODE_METHOD_GEOGRAPHIC2D_OFFSETS) {
        double offsetLat =
            parameterValueNumeric(EPSG_CODE_PARAMETER_LATITUDE_OFFSET,
                                  common::UnitOfMeasure::ARC_SECOND);
        double offsetLong =
            parameterValueNumeric(EPSG_CODE_PARAMETER_LONGITUDE_OFFSET,
                                  common::UnitOfMeasure::ARC_SECOND);

        auto sourceCRSGeog =
            extractGeographicCRSIfGeographicCRSOrEquivalent(sourceCRS());
        if (!sourceCRSGeog) {
            throw io::FormattingException(
                "Can apply Geographic 2D offsets only to GeographicCRS");
        }

        auto targetCRSGeog =
            extractGeographicCRSIfGeographicCRSOrEquivalent(targetCRS());
        if (!targetCRSGeog) {
            throw io::FormattingException(
                "Can apply Geographic 2D offsets only to GeographicCRS");
        }

        formatter->startInversion();
        sourceCRSGeog->addAngularUnitConvertAndAxisSwap(formatter);
        formatter->stopInversion();

        if (offsetLat != 0.0 || offsetLong != 0.0) {
            formatter->addStep("geogoffset");
            formatter->addParam("dlat", offsetLat);
            formatter->addParam("dlon", offsetLong);
        }

        targetCRSGeog->addAngularUnitConvertAndAxisSwap(formatter);

        return;
    }

    if (methodEPSGCode == EPSG_CODE_METHOD_GEOGRAPHIC3D_OFFSETS) {
        double offsetLat =
            parameterValueNumeric(EPSG_CODE_PARAMETER_LATITUDE_OFFSET,
                                  common::UnitOfMeasure::ARC_SECOND);
        double offsetLong =
            parameterValueNumeric(EPSG_CODE_PARAMETER_LONGITUDE_OFFSET,
                                  common::UnitOfMeasure::ARC_SECOND);
        double offsetHeight =
            parameterValueNumericAsSI(EPSG_CODE_PARAMETER_VERTICAL_OFFSET);

        auto sourceCRSGeog =
            dynamic_cast<const crs::GeographicCRS *>(sourceCRS().get());
        if (!sourceCRSGeog) {
            throw io::FormattingException(
                "Can apply Geographic 3D offsets only to GeographicCRS");
        }

        auto targetCRSGeog =
            dynamic_cast<const crs::GeographicCRS *>(targetCRS().get());
        if (!targetCRSGeog) {
            throw io::FormattingException(
                "Can apply Geographic 3D offsets only to GeographicCRS");
        }

        formatter->startInversion();
        sourceCRSGeog->addAngularUnitConvertAndAxisSwap(formatter);
        formatter->stopInversion();

        if (offsetLat != 0.0 || offsetLong != 0.0 || offsetHeight != 0.0) {
            formatter->addStep("geogoffset");
            formatter->addParam("dlat", offsetLat);
            formatter->addParam("dlon", offsetLong);
            formatter->addParam("dh", offsetHeight);
        }

        targetCRSGeog->addAngularUnitConvertAndAxisSwap(formatter);

        return;
    }

    if (methodEPSGCode == EPSG_CODE_METHOD_GEOGRAPHIC2D_WITH_HEIGHT_OFFSETS) {
        double offsetLat =
            parameterValueNumeric(EPSG_CODE_PARAMETER_LATITUDE_OFFSET,
                                  common::UnitOfMeasure::ARC_SECOND);
        double offsetLong =
            parameterValueNumeric(EPSG_CODE_PARAMETER_LONGITUDE_OFFSET,
                                  common::UnitOfMeasure::ARC_SECOND);
        double offsetHeight =
            parameterValueNumericAsSI(EPSG_CODE_PARAMETER_GEOID_UNDULATION);

        auto sourceCRSGeog =
            dynamic_cast<const crs::GeographicCRS *>(sourceCRS().get());
        if (!sourceCRSGeog) {
            auto sourceCRSCompound =
                dynamic_cast<const crs::CompoundCRS *>(sourceCRS().get());
            if (sourceCRSCompound) {
                sourceCRSGeog = sourceCRSCompound->extractGeographicCRS().get();
            }
            if (!sourceCRSGeog) {
                throw io::FormattingException("Can apply Geographic 2D with "
                                              "height offsets only to "
                                              "GeographicCRS / CompoundCRS");
            }
        }

        auto targetCRSGeog =
            dynamic_cast<const crs::GeographicCRS *>(targetCRS().get());
        if (!targetCRSGeog) {
            auto targetCRSCompound =
                dynamic_cast<const crs::CompoundCRS *>(targetCRS().get());
            if (targetCRSCompound) {
                targetCRSGeog = targetCRSCompound->extractGeographicCRS().get();
            }
            if (!targetCRSGeog) {
                throw io::FormattingException("Can apply Geographic 2D with "
                                              "height offsets only to "
                                              "GeographicCRS / CompoundCRS");
            }
        }

        formatter->startInversion();
        sourceCRSGeog->addAngularUnitConvertAndAxisSwap(formatter);
        formatter->stopInversion();

        if (offsetLat != 0.0 || offsetLong != 0.0 || offsetHeight != 0.0) {
            formatter->addStep("geogoffset");
            formatter->addParam("dlat", offsetLat);
            formatter->addParam("dlon", offsetLong);
            formatter->addParam("dh", offsetHeight);
        }

        targetCRSGeog->addAngularUnitConvertAndAxisSwap(formatter);

        return;
    }

    if (methodEPSGCode == EPSG_CODE_METHOD_VERTICAL_OFFSET) {

        const crs::CRS *srcCRS = sourceCRS().get();
        const crs::CRS *tgtCRS = targetCRS().get();

        const auto sourceCRSCompound =
            dynamic_cast<const crs::CompoundCRS *>(srcCRS);
        const auto targetCRSCompound =
            dynamic_cast<const crs::CompoundCRS *>(tgtCRS);
        if (sourceCRSCompound && targetCRSCompound &&
            sourceCRSCompound->componentReferenceSystems()[0]->_isEquivalentTo(
                targetCRSCompound->componentReferenceSystems()[0].get(),
                util::IComparable::Criterion::EQUIVALENT)) {
            srcCRS = sourceCRSCompound->componentReferenceSystems()[1].get();
            tgtCRS = targetCRSCompound->componentReferenceSystems()[1].get();
        }

        auto sourceCRSVert = dynamic_cast<const crs::VerticalCRS *>(srcCRS);
        if (!sourceCRSVert) {
            throw io::FormattingException(
                "Can apply Vertical offset only to VerticalCRS");
        }

        auto targetCRSVert = dynamic_cast<const crs::VerticalCRS *>(tgtCRS);
        if (!targetCRSVert) {
            throw io::FormattingException(
                "Can apply Vertical offset only to VerticalCRS");
        }

        auto offsetHeight =
            parameterValueNumericAsSI(EPSG_CODE_PARAMETER_VERTICAL_OFFSET);

        formatter->startInversion();
        sourceCRSVert->addLinearUnitConvert(formatter);
        formatter->stopInversion();

        formatter->addStep("geogoffset");
        formatter->addParam("dh", offsetHeight);

        targetCRSVert->addLinearUnitConvert(formatter);

        return;
    }

    // Substitute grid names with PROJ friendly names.
    if (formatter->databaseContext()) {
        auto alternate = substitutePROJAlternativeGridNames(
            NN_NO_CHECK(formatter->databaseContext()));
        auto self = NN_NO_CHECK(std::dynamic_pointer_cast<Transformation>(
            shared_from_this().as_nullable()));

        if (alternate != self) {
            alternate->_exportToPROJString(formatter);
            return;
        }
    }

    const bool isMethodInverseOf = starts_with(methodName, INVERSE_OF);

    const auto &NTv1Filename = _getNTv1Filename(this, true);
    const auto &NTv2Filename = _getNTv2Filename(this, true);
    const auto &CTABLE2Filename = _getCTABLE2Filename(this, true);
    const auto &HorizontalShiftGTIFFFilename =
        _getHorizontalShiftGTIFFFilename(this, true);
    const auto &hGridShiftFilename =
        !HorizontalShiftGTIFFFilename.empty()
            ? HorizontalShiftGTIFFFilename
            : !NTv1Filename.empty()
                  ? NTv1Filename
                  : !NTv2Filename.empty() ? NTv2Filename : CTABLE2Filename;
    if (!hGridShiftFilename.empty()) {
        auto sourceCRSGeog =
            extractGeographicCRSIfGeographicCRSOrEquivalent(sourceCRS());
        if (!sourceCRSGeog) {
            throw io::FormattingException(
                concat("Can apply ", methodName, " only to GeographicCRS"));
        }

        auto targetCRSGeog =
            extractGeographicCRSIfGeographicCRSOrEquivalent(targetCRS());
        if (!targetCRSGeog) {
            throw io::FormattingException(
                concat("Can apply ", methodName, " only to GeographicCRS"));
        }

        formatter->startInversion();
        sourceCRSGeog->addAngularUnitConvertAndAxisSwap(formatter);
        formatter->stopInversion();

        if (isMethodInverseOf) {
            formatter->startInversion();
        }
        formatter->addStep("hgridshift");
        formatter->addParam("grids", hGridShiftFilename);
        if (isMethodInverseOf) {
            formatter->stopInversion();
        }

        targetCRSGeog->addAngularUnitConvertAndAxisSwap(formatter);

        return;
    }

    const auto &geocentricTranslationFilename =
        _getGeocentricTranslationFilename(this, true);
    if (!geocentricTranslationFilename.empty()) {
        auto sourceCRSGeog =
            dynamic_cast<const crs::GeographicCRS *>(sourceCRS().get());
        if (!sourceCRSGeog) {
            throw io::FormattingException(
                concat("Can apply ", methodName, " only to GeographicCRS"));
        }

        auto targetCRSGeog =
            dynamic_cast<const crs::GeographicCRS *>(targetCRS().get());
        if (!targetCRSGeog) {
            throw io::FormattingException(
                concat("Can apply ", methodName, " only to GeographicCRS"));
        }

        const auto &interpCRS = interpolationCRS();
        if (!interpCRS) {
            throw io::FormattingException(
                "InterpolationCRS required "
                "for"
                " " EPSG_NAME_METHOD_GEOCENTRIC_TRANSLATION_BY_GRID_INTERPOLATION_IGN);
        }
        const bool interpIsSrc = interpCRS->_isEquivalentTo(
            sourceCRS().get(), util::IComparable::Criterion::EQUIVALENT);
        const bool interpIsTarget = interpCRS->_isEquivalentTo(
            targetCRS().get(), util::IComparable::Criterion::EQUIVALENT);
        if (!interpIsSrc && !interpIsTarget) {
            throw io::FormattingException(
                "For"
                " " EPSG_NAME_METHOD_GEOCENTRIC_TRANSLATION_BY_GRID_INTERPOLATION_IGN
                ", interpolation CRS should be the source or target CRS");
        }

        formatter->startInversion();
        sourceCRSGeog->addAngularUnitConvertAndAxisSwap(formatter);
        formatter->stopInversion();

        if (isMethodInverseOf) {
            formatter->startInversion();
        }

        formatter->addStep("push");
        formatter->addParam("v_3");

        formatter->addStep("cart");
        sourceCRSGeog->ellipsoid()->_exportToPROJString(formatter);

        formatter->addStep("xyzgridshift");
        formatter->addParam("grids", geocentricTranslationFilename);
        formatter->addParam("grid_ref",
                            interpIsTarget ? "output_crs" : "input_crs");
        (interpIsTarget ? targetCRSGeog : sourceCRSGeog)
            ->ellipsoid()
            ->_exportToPROJString(formatter);

        formatter->startInversion();
        formatter->addStep("cart");
        targetCRSGeog->ellipsoid()->_exportToPROJString(formatter);
        formatter->stopInversion();

        formatter->addStep("pop");
        formatter->addParam("v_3");

        if (isMethodInverseOf) {
            formatter->stopInversion();
        }

        targetCRSGeog->addAngularUnitConvertAndAxisSwap(formatter);

        return;
    }

    const auto &heightFilename = _getHeightToGeographic3DFilename(this, true);
    if (!heightFilename.empty()) {
        auto targetCRSGeog =
            extractGeographicCRSIfGeographicCRSOrEquivalent(targetCRS());
        if (!targetCRSGeog) {
            throw io::FormattingException(
                concat("Can apply ", methodName, " only to GeographicCRS"));
        }

        if (!formatter->omitHorizontalConversionInVertTransformation()) {
            formatter->startInversion();
            formatter->pushOmitZUnitConversion();
            targetCRSGeog->addAngularUnitConvertAndAxisSwap(formatter);
            formatter->popOmitZUnitConversion();
            formatter->stopInversion();
        }

        if (isMethodInverseOf) {
            formatter->startInversion();
        }
        formatter->addStep("vgridshift");
        formatter->addParam("grids", heightFilename);
        formatter->addParam("multiplier", 1.0);
        if (isMethodInverseOf) {
            formatter->stopInversion();
        }

        if (!formatter->omitHorizontalConversionInVertTransformation()) {
            formatter->pushOmitZUnitConversion();
            targetCRSGeog->addAngularUnitConvertAndAxisSwap(formatter);
            formatter->popOmitZUnitConversion();
        }

        return;
    }

    if (isGeographic3DToGravityRelatedHeight(method(), true)) {
        auto fileParameter =
            parameterValue(EPSG_NAME_PARAMETER_GEOID_CORRECTION_FILENAME,
                           EPSG_CODE_PARAMETER_GEOID_CORRECTION_FILENAME);
        if (fileParameter &&
            fileParameter->type() == ParameterValue::Type::FILENAME) {
            auto filename = fileParameter->valueFile();

            auto sourceCRSGeog =
                extractGeographicCRSIfGeographicCRSOrEquivalent(sourceCRS());
            if (!sourceCRSGeog) {
                throw io::FormattingException(
                    concat("Can apply ", methodName, " only to GeographicCRS"));
            }

            if (!formatter->omitHorizontalConversionInVertTransformation()) {
                formatter->startInversion();
                formatter->pushOmitZUnitConversion();
                sourceCRSGeog->addAngularUnitConvertAndAxisSwap(formatter);
                formatter->popOmitZUnitConversion();
                formatter->stopInversion();
            }

            bool doInversion = isMethodInverseOf;
            // The EPSG Geog3DToHeight is the reverse convention of PROJ !
            doInversion = !doInversion;
            if (doInversion) {
                formatter->startInversion();
            }
            formatter->addStep("vgridshift");
            formatter->addParam("grids", filename);
            formatter->addParam("multiplier", 1.0);
            if (doInversion) {
                formatter->stopInversion();
            }

            if (!formatter->omitHorizontalConversionInVertTransformation()) {
                formatter->pushOmitZUnitConversion();
                sourceCRSGeog->addAngularUnitConvertAndAxisSwap(formatter);
                formatter->popOmitZUnitConversion();
            }

            return;
        }
    }

    if (methodEPSGCode == EPSG_CODE_METHOD_VERTCON) {
        auto fileParameter =
            parameterValue(EPSG_NAME_PARAMETER_VERTICAL_OFFSET_FILE,
                           EPSG_CODE_PARAMETER_VERTICAL_OFFSET_FILE);
        if (fileParameter &&
            fileParameter->type() == ParameterValue::Type::FILENAME) {
            formatter->addStep("vgridshift");
            formatter->addParam("grids", fileParameter->valueFile());
            if (fileParameter->valueFile().find(".tif") != std::string::npos) {
                formatter->addParam("multiplier", 1.0);
            } else {
                // The vertcon grids go from NGVD 29 to NAVD 88, with units
                // in millimeter (see
                // https://github.com/OSGeo/proj.4/issues/1071), for gtx files
                formatter->addParam("multiplier", 0.001);
            }
            return;
        }
    }

    if (methodEPSGCode == EPSG_CODE_METHOD_VERTICALGRID_NZLVD ||
        methodEPSGCode == EPSG_CODE_METHOD_VERTICALGRID_BEV_AT ||
        methodEPSGCode == EPSG_CODE_METHOD_VERTICALGRID_GTX ||
        methodEPSGCode == EPSG_CODE_METHOD_VERTICALGRID_PL_TXT) {
        auto fileParameter =
            parameterValue(EPSG_NAME_PARAMETER_VERTICAL_OFFSET_FILE,
                           EPSG_CODE_PARAMETER_VERTICAL_OFFSET_FILE);
        if (fileParameter &&
            fileParameter->type() == ParameterValue::Type::FILENAME) {
            formatter->addStep("vgridshift");
            formatter->addParam("grids", fileParameter->valueFile());
            formatter->addParam("multiplier", 1.0);
            return;
        }
    }

    if (isLongitudeRotation()) {
        double offsetDeg =
            parameterValueNumeric(EPSG_CODE_PARAMETER_LONGITUDE_OFFSET,
                                  common::UnitOfMeasure::DEGREE);

        auto sourceCRSGeog =
            dynamic_cast<const crs::GeographicCRS *>(sourceCRS().get());
        if (!sourceCRSGeog) {
            throw io::FormattingException(
                concat("Can apply ", methodName, " only to GeographicCRS"));
        }

        auto targetCRSGeog =
            dynamic_cast<const crs::GeographicCRS *>(targetCRS().get());
        if (!targetCRSGeog) {
            throw io::FormattingException(
                concat("Can apply ", methodName + " only to GeographicCRS"));
        }

        if (!sourceCRSGeog->ellipsoid()->_isEquivalentTo(
                targetCRSGeog->ellipsoid().get(),
                util::IComparable::Criterion::EQUIVALENT)) {
            // This is arguable if we should check this...
            throw io::FormattingException("Can apply Longitude rotation "
                                          "only to SRS with same "
                                          "ellipsoid");
        }

        formatter->startInversion();
        sourceCRSGeog->addAngularUnitConvertAndAxisSwap(formatter);
        formatter->stopInversion();

        bool done = false;
        if (offsetDeg != 0.0) {
            // Optimization: as we are doing nominally a +step=inv,
            // if the negation of the offset value is a well-known name,
            // then use forward case with this name.
            auto projPMName = datum::PrimeMeridian::getPROJStringWellKnownName(
                common::Angle(-offsetDeg));
            if (!projPMName.empty()) {
                done = true;
                formatter->addStep("longlat");
                sourceCRSGeog->ellipsoid()->_exportToPROJString(formatter);
                formatter->addParam("pm", projPMName);
            }
        }
        if (!done) {
            // To actually add the offset, we must use the reverse longlat
            // operation.
            formatter->startInversion();
            formatter->addStep("longlat");
            sourceCRSGeog->ellipsoid()->_exportToPROJString(formatter);
            datum::PrimeMeridian::create(util::PropertyMap(),
                                         common::Angle(offsetDeg))
                ->_exportToPROJString(formatter);
            formatter->stopInversion();
        }

        targetCRSGeog->addAngularUnitConvertAndAxisSwap(formatter);

        return;
    }

    if (exportToPROJStringGeneric(formatter)) {
        return;
    }

    throw io::FormattingException("Unimplemented");
}

} // namespace operation
NS_PROJ_END
