# VTK WASM regression test suite architecture

The VTK Emscripten test utilities utilize a custom protocol designed to address the file I/O requirements of WebAssembly (WASM) unit tests. This protocol allows C++ unit tests to preload files into the sandbox environment or dump files from the sandbox to the server hosting the WASM module. If you prefer to use a Node.js server instead of the Python server employed in VTK, this documentation will guide you through implementing the necessary routes.

## Arguments

The command line for a unit test is set by the CMake functions in vtkModuleTesting.cmake file. It looks like
"python" "Testing/WebAssembly/runner.py" --engine="/path/to/engine" "/path/to/vtkModuleNameCxxTests.js_file" "TestName" "arg1" "arg2" ... "argn"

## Routes:

1. `/`:

    **Request description**: Initial GET request for HTML.

    **Expected response**: server must respond with the contents of a HTML page generated from `templates/index.html` or 404 if it does not exist.

2. `/filename.{js,wasm}`:

    **Request description**: Request for a file with .js or .wasm extension.

    **Expected response**: server must respond with the contents of the JS/WASM file from the build/bin directory or 404 if it does not exist.

3. `/favicon.ico`:

    **Request description**: Request for an icon file.

    **Expected response**: server must respond with the contents of an icon file from the build/bin directory or 404 if it does not exist.

4. `/preload?file=/path/to/file.ext`:

    **Request description**: wasm unit test initiates a GET with the full path of a file in its query parameters.

    **Expected response**: server must respond with the contents of the file or 404 if a file does not exist.

5. `/console_output`:

    **Request description**: wasm unit test initiates a POST with text from `cout` and `cerr` streams in POST content.

    **Expected response**: 'OK'

6. `/dump?file=/path/to/file.ext`:

    **Request description**: wasm unit test initiates a POST with the full path of a file in its query parameters and sends the entire contents of the output file in the POST content.

    **Expected response**: 'OK' or 'Invalid query for /dump' if query parameters are invalid.

7. `/exit`:

    **Request description**: wasm unit test initiates a POST with the exit code of the C++ unit test in the POST content.

    **Expected response**: 'close-window' or 'Invalid exit code! Expected an integer, got {received_exit_code}' if exit code cannot be parsed into an integer.

## Reference Client-side implementation in VTK:

1. [vtkEmscriptenTestUtilities.h](https://gitlab.kitware.com/vtk/vtk/-/blob/master/Testing/Core/vtkEmscriptenTestUtilities.h)
2. [vtkEmscriptenTestUtilities.cxx](https://gitlab.kitware.com/vtk/vtk/-/blob/master/Testing/Core/vtkEmscriptenTestUtilities.cxx)
3. [vtkEmscriptenTestUtilities.js](https://gitlab.kitware.com/vtk/vtk/-/blob/master/Testing/Core/vtkEmscriptenTestUtilities.js)

## Reference Server-side implementation in VTK:

1. The `vtkTestHTTPHandler.translate_path` method implements the `/preload`, `/filename.{js,wasm}` and `/favicon.ico` routes.
2. The `vtkTestHTTPHandler.do_POST` method implements routes like `/dump`, `/console_output` and `/exit`. These are triggered when the wasm application wants to write a file to disk through `fwrite`, print messages to `cout`/`cerr` and `exit(code)` respectively.
