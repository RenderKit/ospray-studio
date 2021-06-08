
if(OPENEXR_FOUND)
    return()
endif()

## Look for any available version
message(STATUS "Looking for OpenEXR...")
find_package(OpenEXR QUIET)

if(NOT DEFINED OPENEXR_VERSION)
    set(OPENEXR_VERSION 3.0.4)
endif()

if(OPENEXR_FOUND)
    message(STATUS "Found OpenEXR")
else()
    ## Download and build if not found
    set(_ARCHIVE_EXT "zip")

    if(NOT DEFINED OPENEXR_URL)
        set(OPENEXR_URL "https://github.com/AcademySoftwareFoundation/openexr/archive/refs/tags/v${OPENEXR_VERSION}.${_ARCHIVE_EXT}")
        set(IMATH_URL "https://github.com/AcademySoftwareFoundation/Imath/archive/refs/tags/v${OPENEXR_VERSION}.${_ARCHIVE_EXT}")
    endif()

    message(STATUS "Downloading OpenEXR ${OPENEXR_URL}...")

    include(FetchContent)

    FetchContent_Declare(
        imath
        URL "${IMATH_URL}"
    )
    ## Bypass FetchContent_MakeAvailable() shortcut to disable install
    FetchContent_GetProperties(imath)
    if(NOT imath_POPULATED)
        FetchContent_Populate(imath)
        ## the subdir will still be built since targets depend on it, but it won't be installed
        add_subdirectory(${imath_SOURCE_DIR} ${imath_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()
    
    FetchContent_Declare(
        openexr
        URL "${OPENEXR_URL}"
    )
    ## Bypass FetchContent_MakeAvailable() shortcut to disable install
    FetchContent_GetProperties(openexr)
    if(NOT openexr_POPULATED)
        FetchContent_Populate(openexr)
        ## the subdir will still be built since targets depend on it, but it won't be installed
        add_subdirectory(${openexr_SOURCE_DIR} ${openexr_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()

    unset(${_ARCHIVE_EXT})
endif()
