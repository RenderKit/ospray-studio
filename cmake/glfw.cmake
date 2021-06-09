
if(glfw3_FOUND)
    return()
endif()

## Look for any available version
message(STATUS "Looking for glfw...")
find_package(glfw3 QUIET)

if(NOT DEFINED GLFW_VERSION)
    set(GLFW_VERSION 3.3.4)
endif()

if(glfw3_FOUND)
    message(STATUS "Found glfw3")
else()
    ## Download and build if not found
    set(_ARCHIVE_EXT "zip")

    if(NOT DEFINED GLFW_URL)
        set(GLFW_URL "https://github.com/glfw/glfw/releases/download/${GLFW_VERSION}/glfw-${GLFW_VERSION}.${_ARCHIVE_EXT}")
    endif()

    message(STATUS "Downloading glfw ${GLFW_URL}...")

    include(FetchContent)

    FetchContent_Declare(
        glfw
        URL "${GLFW_URL}"
    )
    ## Bypass FetchContent_MakeAvailable() shortcut to disable install
    FetchContent_GetProperties(glfw)
    if(NOT glfw_POPULATED)
        FetchContent_Populate(glfw)
        ## the subdir will still be built since targets depend on it, but it won't be installed
        add_subdirectory(${glfw_SOURCE_DIR} ${glfw_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()

    add_library(glfw::glfw ALIAS glfw)

    unset(${_ARCHIVE_EXT})

endif()
