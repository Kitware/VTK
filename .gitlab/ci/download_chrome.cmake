cmake_minimum_required(VERSION 3.12)

# Input variables.
set(chrome_version "125.0.6422.60")
set(chrome_baseurl "https://vtk.org/files/support")
# Uncomment to test newer releases before mirroring at vtk.org
# set(chrome_baseurl "https://storage.googleapis.com/chrome-for-testing-public")

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  set(chrome_platform "win64")
  set(chrome_ext "zip")
  set(chrome_hash "ea51cd37d28747820046b989ed178dd39e0133c2474a95bec2853c154ecae6de")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "linux")
  set(chrome_platform "linux64")
  set(chrome_ext "zip")
  set(chrome_hash "ffb9fe5f0ad490716d403c04bf176a80b35726ef399b6b895301e435e7753d6f")
else ()
  message(FATAL_ERROR
      "Unknown platform for chrome")
endif ()
set(chrome_url "${chrome_baseurl}/chrome/${chrome_version}/${chrome_platform}")
set(chrome_file "chrome-${chrome_platform}.${chrome_ext}")

# Download the file.
file(DOWNLOAD
  "${chrome_url}/${chrome_file}"
  ".gitlab/${chrome_file}"
  STATUS download_status
  EXPECTED_HASH "SHA256=${chrome_hash}")

# Check the download status.
list(GET download_status 0 res)
if (res)
  list(GET download_status 1 err)
  message(FATAL_ERROR
    "Failed to download ${chrome_file}: ${err}")
endif ()

# Extract the file.
execute_process(
  COMMAND
    "${CMAKE_COMMAND}"
    -E tar
    xf "${chrome_file}"
  WORKING_DIRECTORY ".gitlab"
  RESULT_VARIABLE res
  ERROR_VARIABLE err
  ERROR_STRIP_TRAILING_WHITESPACE)
if (res)
  message(FATAL_ERROR
    "Failed to extract ${chrome_file}: ${err}")
endif ()

# Move to a predictable prefix.
file(RENAME
  ".gitlab/chrome-${chrome_platform}"
  ".gitlab/chrome")
