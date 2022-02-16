## Copyright 2020-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

if(OPENEXR_FOUND)
    return()
endif()

find_package(OpenEXR 2 REQUIRED)
