/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Tokenizer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkSystemIncludes.h"

#include "Tokenizer.h"

Tokenizer::Tokenizer(const char *s, const char *delim)
  : FullString(s), Delim(delim)
{
  this->Reset();
}

Tokenizer::Tokenizer(const std::string &s, const char *delim)
  : FullString(s), Delim(delim)
{
  this->Reset();
}

std::string Tokenizer::GetNextToken()
{
  if (this->HasMoreTokens())
    {
    std::string::size_type token_start = this->Position;
    std::string::size_type token_end
      = this->FullString.find_first_of(this->Delim, token_start);
    this->Position = this->FullString.find_first_not_of(this->Delim, token_end);

    if (token_end != std::string::npos)
      {
      return this->FullString.substr(token_start, token_end-token_start);
      }
    else
      {
      return this->FullString.substr(token_start);
      }
    }
  else
    {
    return std::string();
    }
}

std::string Tokenizer::GetRemainingString() const
{
  if (this->HasMoreTokens())
    {
    return this->FullString.substr(this->Position);
    }
  else
    {
    return std::string();
    }
}

bool Tokenizer::HasMoreTokens() const
{
  return (this->Position != std::string::npos);
}

void Tokenizer::Reset()
{
  this->Position = this->FullString.find_first_not_of(this->Delim);
}
