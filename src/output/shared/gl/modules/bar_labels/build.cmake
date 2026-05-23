# Project default
option(GL_MODULES "GL_MODULES" ON)

# Xorg
if(GL_MODULES)
    # OpenGL/GLEW
    find_library(GLEW glew32)
    pkg_check_modules(GLEW QUIET glew)
    if(GLEW OR GLEW_FOUND)
        add_library(gl_bar_labels SHARED "${XAVA_MODULE_DIR}/main.c"
                                          "${XAVA_MODULE_DIR}/label_renderer.c"
                                          "${XAVA_MODULE_DIR}/peak_detector.c"
                                          "${XAVA_MODULE_DIR}/note_naming.c"
                                          "${XAVA_MODULE_DIR}/../../util/shader.c"
                                          "${XAVA_MODULE_DIR}/../../util/misc.c"
                                          "${GLOBAL_FUNCTION_SOURCES}")
        target_link_directories(gl_bar_labels PRIVATE
            "${GLEW_LIBRARY_DIRS}")
        target_include_directories(gl_bar_labels PRIVATE
            "${GLEW_INCLUDE_DIRS}"
            "${CMAKE_SOURCE_DIR}/lib/stb")

        if(WINDOWS OR MINGW OR MSVC OR CYGWIN)
            target_link_libraries(gl_bar_labels xava-shared "-lglew32 -lopengl32")
        else()
            target_link_libraries(gl_bar_labels xava-shared "${GLEW_LIBRARIES}")
        endif()
        target_compile_definitions(gl_bar_labels PUBLIC -DGL)

        set_target_properties(gl_bar_labels PROPERTIES PREFIX "")
        set_target_properties(gl_bar_labels PROPERTIES IMPORT_PREFIX "")
        set_target_properties(gl_bar_labels PROPERTIES OUTPUT_NAME "gl/module/bar_labels/module")

        # this copies the dlls for mr. windows
        #find_and_copy_dlls(gl_bar_labels)

        configure_file("${XAVA_MODULE_DIR}/config.ini"    gl/module/bar_labels/config.ini   COPYONLY)

        install(TARGETS gl_bar_labels RENAME module DESTINATION share/xava/gl/module/bar_labels/)
        install(FILES "${CMAKE_BINARY_DIR}/gl/module/bar_labels/config.ini"    DESTINATION share/xava/gl/module/bar_labels/)

        # Maybe GL license?
    else()
        message(WARNING "GLEW library not found; \"gl_bar_labels\" won't build")
    endif()
endif()
