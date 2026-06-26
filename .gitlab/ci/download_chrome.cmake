cmake_minimum_required(VERSION 3.12)

# Input variables.
set(chrome_version "138.0.7204.183")
set(chrome_baseurl "https://vtk.org/files/support/chrome/")
# Uncomment to test newer releases before mirroring at vtk.org
# set(chrome_baseurl "https://storage.googleapis.com/chrome-for-testing-public")

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  set(chrome_platform "win64")
  set(chrome_ext "zip")
  set(chrome_hash "10a9fa695a9e4c05b336aebdc553f92c2351b2e9b962b00dc00612c8dcaacd68")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "linux")
  set(chrome_platform "linux64")
  set(chrome_ext "zip")
  set(chrome_hash "085393c89646c06141c7f2eb2d8a4620a5bc86b2df897ca3d1b256e8b222175a")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_arm64")
  set(chrome_platform "mac64")
  set(chrome_ext "zip")
  set(chrome_hash "2e83ec74a87928bdcd3425ba90901bceb2be6c5ffa6d9ebcac77f1dd3d1dd768")
  set(chrome_file "chrome-mac-arm64.zip")
  set(chrome_dir ".gitlab/chrome-mac-arm64")
else ()
  message(FATAL_ERROR
      "Unknown platform for chrome")
endif ()
set(chrome_url "${chrome_baseurl}/${chrome_version}/${chrome_platform}")
if (NOT DEFINED chrome_file)
  set(chrome_file "chrome-${chrome_platform}.${chrome_ext}")
endif()
if (NOT DEFINED chrome_dir)
  set(chrome_dir ".gitlab/chrome-${chrome_platform}")
endif ()
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
  "${chrome_dir}"
  ".gitlab/chrome")
