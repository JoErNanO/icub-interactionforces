# Copyright: 2013 Francesco Giovannini, iCub Facility - Istituto Italiano di Tecnologia
# Author: Francesco Giovannini
# CopyPolicy: Released under the terms of the GNU GPL v2.0.
# 


set(CONTEXTNAME interactionForces)

# Search for xml files
file(GLOB APP_CONF ${CMAKE_CURRENT_SOURCE_DIR}/portscope/*.xml)
yarp_install(FILES ${APP_CONF} DESTINATION ${ICUBCONTRIB_CONTEXTS_INSTALL_DIR}/${CONTEXTNAME})
