
if(OpenImageIO_FOUND)
    return()
endif()

## Look for any available version
message(STATUS "Looking for OpenImageIO...")
find_package(OpenImageIO QUIET)

if(NOT DEFINED OIIO_VERSION)
    set(OIIO_VERSION 2.2.15.1)
endif()

if(OpenImageIO_FOUND)
    message(STATUS "Found OpenImageIO")
else()
    ## Download and build if not found
    set(_ARCHIVE_EXT "zip")

    if(NOT DEFINED OIIO_URL)
        set(OIIO_URL "https://github.com/OpenImageIO/oiio/archive/refs/tags/v${OIIO_VERSION}.${_ARCHIVE_EXT}")
    endif()

    message(STATUS "Downloading OpenImageIO ${OIIO_URL}...")

    include(FetchContent)

    FetchContent_Declare(
        oiio
        URL "${OIIO_URL}"
    )

    ## Bypass FetchContent_MakeAvailable() shortcut to disable install
    FetchContent_GetProperties(oiio)
    if(NOT oiio_POPULATED)
        FetchContent_Populate(oiio)
        ## the subdir will still be built since targets depend on it, but it won't be installed
        add_subdirectory(${oiio_SOURCE_DIR} ${oiio_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()
    
    unset(${_ARCHIVE_EXT})
endif()
