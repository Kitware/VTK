// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkURILoader_h
#define vtkURILoader_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkResourceStream.h" // For vtkResourceStream
#include "vtkSmartPointer.h"   // For vtkSmartPointer
#include "vtkURI.h"            // For vtkURI

#include <cstdint> // for std::int32_t
#include <cstdlib> // for std::size_t
#include <memory>  // for std::unique_ptr
#include <string>  // for std::string

VTK_ABI_NAMESPACE_BEGIN

/**
 * @brief Helper class for readers and importer that need to load more than one resource
 *
 * ## Introduction
 *
 * `vtkURILoader` is a class that will load URIs,
 * giving `vtkResourceStream`s from resolved resource.
 * URI logic (parsing, resolution) is implemented in `vtkURI`.
 *
 * Some formats, such as GLTF, may refer to external resources through URIs. This URI loader
 * can be used to solve this problem.
 *
 * `vtkURILoader` provides "file" and "data" URIs loading.
 * "file" URIs loading only supports localhost.
 * More schemes may be supported in future versions.
 *
 * ## Base URI
 *
 * Base URI, specified in [RFC3986#5](https://datatracker.ietf.org/doc/html/rfc3986#section-5),
 * is a concept that enable URIs to refer to relative resources from a base resource.
 * For example, you can have a file that needs to look for another file next to it.
 * The best way to implement this, is to have the first file as the base URI, and the referenced
 * file as a relative reference.
 *
 * To set a local file as base URI, you should use `SetBaseFileName`, and for a local directory,
 * you should use `SetBaseDirectory`. These functions generate a "file" URI from given path.
 * For example, calling `SetBaseFileName(".")` will generate the following URI:
 * `"file:///<absolute-current-working-directory>/."`.
 * These functions should be used because they handle some platform specific details, such as adding
 * a `/` at the beginning of the path on Windows, percent-encoding, resolving relative paths,
 * "." and "..", checking that path exists and actual filesystem entry type is coherent.
 *
 * If the loader has no base URI, it can only load full URIs. See `vtkURI` for more information.
 *
 * ## Basic usage
 *
 * Here is a basic example of `vtkURILoader` usage:
 * ```cpp
 * vtkNew<vtkURILoader> loader;
 * loader->SetBaseFileName("."); // Set current working directory as the base URI
 * // This Load call will parse the string to a vtkURI. In that case, the URI only has a path.
 * // Then it will be resolved from base URI. In that case, the current working directory:
 * // "file:///<cwd>/." + "example.txt" == "file:///<cwd>/example.txt"
 * // Then it will call the `DoLoad` function. This DoLoad function will check URI scheme,
 * // here "file", and call the right loading function. In that case LoadFile will be called.
 * // LoadFile will create a vtkFileResourceStream and open it on URI path.
 * auto stream = loader->Load("example.txt");
 * // stream is opened on ./example.txt... Or it is null, in case of error.
 *
 * // When loading a full URI, base URI is ignored (see vtkURI::Resolve and RFC specs)
 * auto other = loader->Load("data:;base64,AAAA");
 * // other is a vtkMemoryResourceStream on the decoded base64 data. Here, 3 bytes, all equal to 0.
 * ```
 *
 * Note that in previous example, `loader->Load()` actually returns a `vtkResourceStream`,
 * the real type can be accessed through `SafeDownCast`.
 *
 * ## Usage in readers
 *
 * When implementing a reader, you should use `vtkURILoader` if the format can contain URIs.
 * Here are the global guidelines of URI loader support in a reader:
 * - The function should be named `SetURILoader`.
 * - Depending on the format, the reader may require an URI loader, or just optionally use it.
 * - The reader may use a default constructed URI loader by default. This would enable full URI
 *   loading, such as "data" URIs.
 * - When reading from a file name, using `SetFileName` function, the reader should internally
 *   open a `vtkFileResourceStream` on the file and create a `vtkURILoader` with a base URI set to
 *   `FileName`, then use the same code as the resource stream based reading. This prevents
 *   code duplication.
 *
 * ## Extension
 *
 * `vtkURILoader::DoLoad` is responsible of actually loading a full URI.
 * It is a virtual function, so it can be reimplemented to let the user support additional schemes.
 * In case you want do support additional scheme, URI scheme and host should be case-insensitive
 * as specified in [RFC3986#6.2.2.1](https://datatracker.ietf.org/doc/html/rfc3986#section-6.2.2.1).
 *
 * `vtkURILoader::LoadFile` and `vtkURILoader::LoadData` are the actual implementation of "file"
 * and "data" URI loading.
 */
class VTKIOCORE_EXPORT vtkURILoader : public vtkObject
{
  struct vtkInternals;

public:
  vtkTypeMacro(vtkURILoader, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkURILoader* New();

  ///@{
  /**
   * @brief Set base URI to use
   *
   * URI must be an absolute URI.
   * It will be used by `Resolve` and `Load(const vtkURI*)` functions
   * to resolve relative references.
   *
   * It may be `nullptr`. `HasBaseURI` is a more explicit way to check that.
   *
   * `SetBaseURI(const std::string& uri)` parses the URI from a string
   * and then do the same as `SetBaseURI(vtkURI* uri)`.
   *
   * Base URI is manipulated as a non-const `vtkURI` because loader keep a owning reference on it.
   *
   * @param uri A string to parse, or an existing URI. Empty string is the same as nullptr.
   * Existing URI Reference count will be increased.
   * @return `true` if uri is a suitable base URI (i.e. is `absolute`).
   */
  bool SetBaseURI(const std::string& uri);
  bool SetBaseURI(vtkURI* uri);
  ///@}

  /**
   * @brief Get base URI
   * @return the pointer on base URI, may be nullptr.
   */
  vtkURI* GetBaseURI() const;

  /**
   * Check if loader as a base URI
   */
  bool HasBaseURI() const { return this->GetBaseURI() != nullptr; }

  /**
   * @brief Higher level way to set the base URI to an existing file
   *
   * This generates a file URI on the absolute path of the specified filepath.
   * `filepath` must refer to an existing file.
   *
   * @param filepath File path to use as base URI.
   * `filepath` may be relative, it will be automatically transformed into an absolute path.
   * @return true if filename can be resolved, false otherwise
   */
  bool SetBaseFileName(VTK_FILEPATH const std::string& filepath);

  /**
   * @brief Higher level way to set the base URI to an existing directory
   *
   * This generates a file URI on the absolute path of the specified directory `"."` file.
   * `path` must refer to an existing directory.
   *
   * @param dirpath File path to use as base URI.
   * `dirpath` may be relative, it will be automatically transformed into an absolute path.
   * @return true if path can be resolved, false otherwise
   */
  bool SetBaseDirectory(VTK_FILEPATH const std::string& dirpath);

  /**
   * @brief Resolve URI from base URI
   * @return vtkURI::Resolve(this->GetBaseURI(), uri);
   */
  vtkSmartPointer<vtkURI> Resolve(const vtkURI* uri);

  /**
   * @brief Load a resource referenced by an URI
   *
   * Perform as if by calling `this->Load(uri.data(), uri.size())`.
   *
   * @param uri URI string representation, may be empty.
   * @return A `vtkResourceStream` on the loaded resource on success, nullptr otherwise.
   */
  vtkSmartPointer<vtkResourceStream> Load(const std::string& uri)
  {
    return this->Load(uri.data(), uri.size());
  }

  /**
   * @brief Load a resource referenced by an URI
   *
   * Try to parse  an URI from given string using `vtkURI::Parse`.
   * If parsing fails, returns nullptr immediately, otherwise,
   * performs as if by calling `Load(const vtkURI*)` with parsed URI.
   *
   * @param uri An URI string representation, may be `nullptr` if size is `0`.
   * @param size Size of `uri` string, may be `0`.
   * @return A `vtkResourceStream` on the loaded resource on success, nullptr otherwise.
   */
  vtkSmartPointer<vtkResourceStream> Load(const char* uri, std::size_t size);

  /**
   * Try to resolve given URI from base URI, using `vtkURI::Resolve`.
   * If resolution fails, returns nullptr.
   * Otherwise, performs as if by calling `LoadResolved(const vtkURI*)` with resolved URI.
   *
   * @param uri A `vtkURI`, must be a relative reference.
   * @return A `vtkResourceStream` on the loaded resource on success, nullptr otherwise.
   */
  vtkSmartPointer<vtkResourceStream> Load(const vtkURI* uri);

  /**
   * @brief Load a resource from a full URI
   *
   * Checks if URI is suitable for loading (i.e. is a full URI), then calls `DoLoad(uri)`.
   *
   * @param uri A `vtkURI`, must be a full URI
   * @return A `vtkResourceStream` on the loaded resource on success, nullptr otherwise.
   */
  vtkSmartPointer<vtkResourceStream> LoadResolved(const vtkURI* uri);

protected:
  /**
   * @brief Constructor
   *
   * Default constructed vtkURILoader has no base URI.
   */
  vtkURILoader();
  ~vtkURILoader() override;
  vtkURILoader(const vtkURILoader&) = delete;
  vtkURILoader& operator=(const vtkURILoader&) = delete;

  /**
   * @brief Load a resource from a full URI
   *
   * Dispatch, depending on uri scheme:
   * - `LoadFile` if scheme == "file"
   * - `LoadData` if scheme == "data"
   * If scheme is not one of these, returns nullptr and generates an error.
   *
   * This function is virtual and may be reimplemented to support additional schemes
   * or disable some schemes.
   *
   * @param uri A `vtkURI`, must be a full URI
   * @return A `vtkResourceStream` on the loaded resource on success, nullptr otherwise.
   */
  virtual vtkSmartPointer<vtkResourceStream> DoLoad(const vtkURI& uri);

  /**
   * @brief Load a resource from a file URI
   *
   * Current implementation only supports localhost authority.
   * Returned stream is a `vtkFileResourceStream` on URI path.
   * Query and fragment are ignored.
   *
   * @param uri A `vtkURI`, must be a full file URI
   * @return A `vtkResourceStream` on the loaded resource on success, nullptr otherwise.
   */
  vtkSmartPointer<vtkResourceStream> LoadFile(const vtkURI& uri);

  /**
   * @brief Load a resource from a data URI
   *
   * Authority is ignored.
   * Supports raw (percent-encoded) and base64-encoded data URI.
   * If an error occurs during data decoding, returns nullptr.
   * Query and fragment are ignored.
   *
   * Returned stream is a `vtkMemoryResourceStream` on decoded data. The streams owns the data.
   *
   * @param uri A `vtkURI`, must be a full data URI
   * @return A `vtkResourceStream` on the loaded resource on success, nullptr otherwise.
   */
  vtkSmartPointer<vtkResourceStream> LoadData(const vtkURI& uri);

private:
  std::unique_ptr<vtkInternals> Impl;
};

VTK_ABI_NAMESPACE_END

#endif
