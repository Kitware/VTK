// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Define utility functions to preload and dump a file.
 * This file gets injected as a `--pre-js` file.
 */

var vtkEmscriptenTestUtilities = {
  /**
   * Fetches a file `hostPath` from server and copies webassembly memory.
   * Here is how you can call this from C/C++ code:
   * ```cpp
   * #include "vtkEmscriptenTestUtilities.h" // for PreloadDescriptor and vtkPreloadDataFileIntoMemory
   * vtkEmscriptenTestUtilities::PreloadDescriptor* payload = nullptr;
   * payload = reinterpret_cast<vtkEmscriptenTestUtilities::PreloadDescriptor*>(vtkPreloadDataFileIntoMemory("/path/to/server/directory/file.png"));
   * ```
   * @param {*} hostPath A valid path in the server"s filesystem.
   */
  vtkPreloadDataFileIntoMemory__deps: ['malloc'],
  vtkPreloadDataFileIntoMemory__sig: "pp",
  vtkPreloadDataFileIntoMemory: (hostPathFile) => {
    hostPathFile = UTF8ToString(hostPathFile);
    const req = new XMLHttpRequest;
    req.overrideMimeType('text/plain; charset=x-user-defined');
    req.open("GET", `preload?file=${hostPathFile}`, false);
    req.send(null);
    if (!(req.status >= 200 && req.status < 300)) {
      return 0;
    } else if (req.response !== undefined) {
      let data = Uint8Array.from(req.response, c => c.charCodeAt(0));
      const fileContentPtr = _malloc(data.length);
      HEAPU8.set(data, fileContentPtr);
      const payloadPtr = _malloc(4 + {{{ POINTER_SIZE }}});
      {{{ makeSetValue('payloadPtr', 0, 'fileContentPtr', '*') }}}
      {{{ makeSetValue('payloadPtr', `${POINTER_SIZE}`, 'data.length', '*') }}}
      return payloadPtr;
    }
  },

  /**
   * Helps VTK C++ unit tests write a file in the server"s file system.
   * Here is how you can call this from C/C++ code:
   * ```cpp
   * #include "vtkEmscriptenTestUtilities.h" // for vtkDumpFile
   * vtkDumpFile("/path/to/server/directory/file.png", bytes, number_of_bytes);
   * ```
   * @param {*} hostPath A valid path in the server"s filesystem.
   * @param {*} data A Uint8Array representing the contents of the file which will be dumped out in the server"s filesystem.
   */
  vtkDumpFile__sig: "vppp",
  vtkDumpFile: (hostPathFile, data, nbytes) => {
    hostPathFile = UTF8ToString(hostPathFile);
    const req = new XMLHttpRequest;
    req.open("POST", `dump?file=${hostPathFile}`);
    const byteArray = HEAPU8.subarray(data, data + nbytes);
    req.send(new Uint8Array(byteArray));
  },

  /**
   * Send the exit code to the server.
   */
  vtkPostExitCode__sig: "vi",
  vtkPostExitCode: (code) => {
    // this function is defined in Testing/WebAssembly/templates/index.html
    finalize(code)
  },
};

mergeInto(LibraryManager.library, vtkEmscriptenTestUtilities);
