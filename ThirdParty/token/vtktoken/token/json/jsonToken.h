//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
#ifndef token_json_jsonToken_h
#define token_json_jsonToken_h

#include "token/Token.h"

// XXX(kitware): Use VTK's version, whether internal or external.
// #include "nlohmann/json.hpp"
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)

// Define how managed string tokens are serialized.
token_BEGIN_NAMESPACE
using json = nlohmann::json;

TOKEN_EXPORT void to_json(json&, const Token&);

TOKEN_EXPORT void from_json(const json&, Token&);

token_CLOSE_NAMESPACE

#endif
