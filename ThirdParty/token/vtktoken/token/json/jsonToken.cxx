//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
#include "token/json/jsonToken.h"

token_BEGIN_NAMESPACE

void to_json(json& j, const Token& t)
{
  if (t.hasData())
  {
    j = t.data();
  }
  else
  {
    j = t.getId();
  }
}

void from_json(const json& j, Token& t)
{
  if (j.is_string())
  {
    t = Token(j.get<std::string>());
  }
  else
  {
    t = Token(j.get<Hash>());
  }
}

token_CLOSE_NAMESPACE
