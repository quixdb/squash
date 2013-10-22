find_package (Doxygen)

set (SQUASH_DOXYGEN_PLUGIN_DOCS)
foreach (plugin ${squash_enabled_plugins})
  set (SQUASH_DOXYGEN_PLUGIN_DOCS
    "${SQUASH_DOXYGEN_PLUGIN_DOCS} ${SQUASH_TOP_SRCDIR}/plugins/${plugin}/${plugin}.md")
endforeach ()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
  ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

if (DOXYGEN_FOUND)
  add_custom_target (doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif ()
