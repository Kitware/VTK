
/*
* Copyright (c) 2007, Sandia Corporation
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the Sandia Corporation nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Sandia Corporation ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Sandia Corporation BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <vtkFileExtensionMimeTypeStrategy.h>
#include <vtkObjectFactory.h>
#include <vtkStdString.h>

#include <boost/algorithm/string.hpp>

vtkCxxRevisionMacro(vtkFileExtensionMimeTypeStrategy, "1.1");
vtkStandardNewMacro(vtkFileExtensionMimeTypeStrategy);

class vtkFileExtensionMimeTypeStrategy::implementation
{
public:
  static bool Lookup(const vtkStdString& path, const vtkStdString& suffix, const vtkStdString& mime_type, vtkStdString& result)
  {
    if(boost::iends_with(path, suffix))
      {
      result = mime_type;
      return true;
      }
      
    return false;
  }
};

vtkFileExtensionMimeTypeStrategy::vtkFileExtensionMimeTypeStrategy()
{
}

vtkFileExtensionMimeTypeStrategy::~vtkFileExtensionMimeTypeStrategy()
{
}

void vtkFileExtensionMimeTypeStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkStdString vtkFileExtensionMimeTypeStrategy::Lookup(const vtkStdString& path)
{
  vtkStdString mime_type;

  if(implementation::Lookup(path, ".ai", "application/postscript", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".bmp", "image/bmp", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".c", "text/plain", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".cpp", "text/plain", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".css", "text/css", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".csv", "text/csv", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".cxx", "text/plain", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".dll", "application/octet-stream", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".doc", "application/msword", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".dot", "application/msword", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".dvi", "application/x-dvi", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".eps", "application/postscript", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".exe", "application/octet-stream", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".gz", "application/x-gzip", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".gif", "image/gif", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".h", "text/plain", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".hpp", "text/plain", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".htm", "text/html", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".html", "text/html", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".hxx", "text/plain", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".ico", "image/x-icon", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".jpe", "image/jpeg", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".jpeg", "image/jpeg", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".jpg", "image/jpeg", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".js", "application/x-javascript", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".latex", "application/x-latex", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".lib", "application/octet-stream", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".pdf", "application/pdf", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".png", "image/png", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".ppt", "application/vnd.ms-powerpoint", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".ps", "application/postscript", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".shtml", "text/html", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".tcl", "application/x-tcl", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".tif", "image/tiff", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".tiff", "image/tiff", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".txt", "text/plain", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".xls", "application/vnd.ms-excel", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".xml", "text/xml", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".zip", "application/zip", mime_type)) return mime_type;

  return vtkStdString();
}

