cmake_minimum_required(VERSION 2.8)

find_program(CppCheck_EXECUTABLE cppcheck DOC "path to the ccppcheck executable")
mark_as_advanced(CppCheck_EXECUTABLE)

if(CppCheck_EXECUTABLE)
  execute_process(COMMAND ${CppCheck_EXECUTABLE} --version
    OUTPUT_VARIABLE CppCheck_version_output
    ERROR_VARIABLE  CppCheck_version_error
    RESULT_VARIABLE CppCheck_version_result
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  if("${CppCheck_version_result}" EQUAL 0)
    string(REGEX REPLACE "^Cppcheck ([^ ]+)"
      "\\1"
      CppCheck_VERSION "${CppCheck_version_output}")
  endif()
endif(CppCheck_EXECUTABLE)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(CppCheck
  REQUIRED_VARS CppCheck_EXECUTABLE
  VERSION_VAR CppCheck_VERSION)

if(CppCheck_FOUND)
  add_custom_target(cppcheck)
endif(CppCheck_FOUND)

function(cppcheck)
  if(CppCheck_FOUND)
    set (options FORCE FATAL)
    set (oneValueArgs)
    set (multiValueArgs TARGET DEFINE INCLUDE_DIRECTORIES ENABLE)
    cmake_parse_arguments(CPPCHECK "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    foreach(tgt ${CPPCHECK_TARGET})
      set(cppcheck_cmd "${CppCheck_EXECUTABLE}" "-q")

      get_property(SOURCES TARGET "${tgt}" PROPERTY SOURCES)
      get_property(SOURCE_DIR TARGET "${tgt}" PROPERTY SOURCE_DIR)
      get_property(COMPILE_DEFINITIONS TARGET "${tgt}" PROPERTY COMPILE_DEFINITIONS)
      get_property(DIR_COMPILE_DEFINITIONS DIRECTORY "${SOURCE_DIR}" PROPERTY COMPILE_DEFINITIONS)
      get_property(INCLUDE_DIRECTORIES TARGET "${tgt}" PROPERTY INCLUDE_DIRECTORIES)
      get_property(DIR_INCLUDE_DIRECTORIES DIRECTORY "${SOURCE_DIR}" PROPERTY INCLUDE_DIRECTORIES)
      get_property(C_STANDARD TARGET "${tgt}" PROPERTY C_STANDARD)

      if(CPPCHECK_FORCE)
        list(APPEND cppcheck_cmd "-f")
      endif()

      if(CPPCHECK_FATAL)
        list(APPEND cppcheck_cmd "--error-exitcode=1")
      endif()

      if(C_STANDARD)
        list(APPEND cppcheck_cmd "--std=c${C_STANDARD}")
      endif(C_STANDARD)

      foreach(enable ${CPPCHECK_ENABLE})
        list(APPEND cppcheck_cmd "--enable=${enable}")
      endforeach()

      foreach(def ${COMPILE_DEFINITIONS} ${DIR_COMPILE_DEFINITIONS} ${CPPCHECK_DEFINE})
        list(APPEND cppcheck_cmd "-D${def}")
      endforeach()

      foreach(inc ${INCLUDE_DIRECTORIES} ${DIR_INCLUDE_DIRECTORIES} ${CPPCHECK_INCLUDE_DIRECTORIES})
        if(NOT "${inc}" STREQUAL "/usr/include")
          list(APPEND cppcheck_cmd "-I${inc}")
        endif()
      endforeach()

      if(WIN32)
        list(APPEND cppcheck_cmd "--platform=win64")
      else()
        list(APPEND cppcheck_cmd "--platform=unix64")
      endif()

      add_custom_target("${tgt}-cppcheck" COMMAND
        ${cppcheck_cmd} ${SOURCES}
        WORKING_DIRECTORY "${SOURCE_DIR}")

      add_dependencies(cppcheck "${tgt}-cppcheck")
    endforeach(tgt)
  endif()
endfunction(cppcheck)
