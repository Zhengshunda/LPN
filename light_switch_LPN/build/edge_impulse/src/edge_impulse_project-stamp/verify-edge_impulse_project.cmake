# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

if("D:/bluemesh/tflite/tflite-v2/light_switch_LPN/11212-v1.zip" STREQUAL "")
  message(FATAL_ERROR "LOCAL can't be empty")
endif()

if(NOT EXISTS "D:/bluemesh/tflite/tflite-v2/light_switch_LPN/11212-v1.zip")
  message(FATAL_ERROR "File not found: D:/bluemesh/tflite/tflite-v2/light_switch_LPN/11212-v1.zip")
endif()

if("" STREQUAL "")
  message(WARNING "File will not be verified since no URL_HASH specified")
  return()
endif()

if("" STREQUAL "")
  message(FATAL_ERROR "EXPECT_VALUE can't be empty")
endif()

message(STATUS "verifying file...
     file='D:/bluemesh/tflite/tflite-v2/light_switch_LPN/11212-v1.zip'")

file("" "D:/bluemesh/tflite/tflite-v2/light_switch_LPN/11212-v1.zip" actual_value)

if(NOT "${actual_value}" STREQUAL "")
  message(FATAL_ERROR "error:  hash of
  D:/bluemesh/tflite/tflite-v2/light_switch_LPN/11212-v1.zip
does not match expected value
  expected: ''
    actual: '${actual_value}'
")
endif()

message(STATUS "verifying file... done")
