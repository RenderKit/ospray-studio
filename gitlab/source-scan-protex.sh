#!/bin/bash
## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

BDSTOOL=$PROTEX_PATH/bin/bdstool
SRC_PATH=$CI_PROJECT_DIR/

export _JAVA_OPTIONS=-Duser.home=$PROTEX_PATH/home

# enter source code directory before scanning
cd $SRC_PATH

$BDSTOOL new-project --server $PROTEX_OSPRAY_STUDIO_URL $PROTEX_OSPRAY_STUDIO_PROJECT |& tee ip_protex.log
if grep -q "fail\|error\|fatal\|not found" ip_protex.log; then
    exit 1
fi

$BDSTOOL analyze --server $PROTEX_OSPRAY_STUDIO_URL |& tee -a ip_protex.log
if grep -q "fail\|error\|fatal\|not found" ip_protex.log; then
    exit 1
fi

if grep -E "^Files pending identification: [0-9]+$" ip_protex.log; then
    echo "Protex scan FAILED!"
    exit 1
fi

echo "Protex scan PASSED!"
exit 0
