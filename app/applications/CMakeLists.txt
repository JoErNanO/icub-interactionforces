# Copyright: 2013 Francesco Giovannini, iCub Facility - Istituto Italiano di Tecnologia
# Author: Francesco Giovannini
# CopyPolicy: Released under the terms of the GNU GPL v2.0.
# 

# Search for application file
file(GLOB APP_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/*.xml.template)

# Install apps 
yarp_install(FILES ${APP_SCRIPTS} DESTINATION ${ICUBCONTRIB_APPLICATIONS_TEMPLATES_INSTALL_DIR})
