#!/bin/bash
mv ${DEPLOY_DIR}/antifurto ${DEPLOY_DIR}/antifurto.old
cp ${CMAKE_CURRENT_BINARY_DIR}/antifurto ${DEPLOY_DIR}/
cp ${CMAKE_CURRENT_BINARY_DIR}/frontend/antifurto.fcgi ${DEPLOY_DIR}/
cp ${CMAKE_CURRENT_BINARY_DIR}/send_mail.sh ${DEPLOY_DIR}/
#cp ${CMAKE_CURRENT_SOURCE_DIR}/lib/yowsup/src/yowsup-cli ${DEPLOY_DIR}/
#cp -r ${CMAKE_CURRENT_SOURCE_DIR}/lib/yowsup/src/Yowsup ${DEPLOY_DIR}/
#cp -r ${CMAKE_CURRENT_SOURCE_DIR}/lib/yowsup/src/Examples ${DEPLOY_DIR}/
#cp ${CMAKE_CURRENT_SOURCE_DIR}/lib/dropbox_uploader/dropbox_uploader.sh ${DEPLOY_DIR}/
#cp test/antifurto_pi_test ${DEPLOY_DIR}/
