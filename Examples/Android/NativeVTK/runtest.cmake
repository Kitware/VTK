# test script for running an android test and collecting the output
# this script is probably brittle as well peanut brittle. ots of
# hardcoded paths etc.  Feel free to make it more generic as more devices
# platforms are tested.

# adb and sleep must be in the path
# the valid image must be on the device

#load the app
execute_process(
  COMMAND adb install -r NativeVTK-debug.apk
  WORKING_DIRECTORY ${WORKINGDIR}
  OUTPUT_VARIABLE CMD_OUTPUT
  ERROR_VARIABLE CMD_ERROR
  RESULT_VARIABLE CMD_RESULT
  )
if(CMD_RESULT)
  message(FATAL_ERROR "Error running adb install with output ${CMD_OUTPUT} and errors ${CMD_ERROR}")
else()
  message("Success running adb install")
endif()

# run the app
execute_process(
  COMMAND adb shell am start -n com.kitware.NativeVTK/android.app.NativeActivity --es VTKTesting Testing
  WORKING_DIRECTORY ${WORKINGDIR}
  OUTPUT_VARIABLE CMD_OUTPUT
  ERROR_VARIABLE CMD_ERROR
  RESULT_VARIABLE CMD_RESULT
  )
if(CMD_RESULT)
  message(FATAL_ERROR "Error running adb shell am start with output ${CMD_OUTPUT} and errors ${CMD_ERROR}")
else()
  message("Success running adb shell am start")
endif()

#sleep for a few sec
execute_process(COMMAND sleep 10)

# get the valid image
execute_process(
  COMMAND adb pull /storage/emulated/legacy/Android/data/com.kitware.NativeVTK/files/NativeVTKResult.png
  WORKING_DIRECTORY ${WORKINGDIR}
  OUTPUT_VARIABLE CMD_OUTPUT
  ERROR_VARIABLE CMD_ERROR
  RESULT_VARIABLE CMD_RESULT
  )
if(CMD_RESULT)
  message(FATAL_ERROR "Error pulling result file back with output ${CMD_OUTPUT} and errors ${CMD_ERROR}")
else()
  message("Success running adb pull ...")
endif()

# get the output text
execute_process(
  COMMAND adb pull /storage/emulated/legacy/Android/data/com.kitware.NativeVTK/files/NativeVTKResult.txt
  WORKING_DIRECTORY ${WORKINGDIR}
  OUTPUT_VARIABLE CMD_OUTPUT
  ERROR_VARIABLE CMD_ERROR
  RESULT_VARIABLE CMD_RESULT
  )
if(CMD_RESULT)
  message(FATAL_ERROR "Error pulling result file text with output ${CMD_OUTPUT} and errors ${CMD_ERROR}")
else()
  file(READ NativeVTKResult.txt TextResult)
  message("${TextResult}")
endif()
