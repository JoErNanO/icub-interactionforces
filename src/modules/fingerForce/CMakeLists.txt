# Copyright: 2013 Francesco Giovannini, iCub Facility - Istituto Italiano di Tecnologia
# Author: Francesco Giovannini
# CopyPolicy: Released under the terms of the GNU GPL v2.0.
# 

#
# The fingerForce module.
#
set(MODULENAME fingerForce)

###################
## The included source code
###################
set(SRC_HEADERS 
    include/FingerForceModule.h
    idl/include/${MODULENAME}_IDLServer.h
)

set(SRC_FILES main.cpp 
    FingerForceModule.cpp
    idl/src/${MODULENAME}_IDLServer.cpp
)

# Search for thrift files
set(IDL ${MODULENAME}.thrift)
yarp_idl_to_dir(${IDL} ${CMAKE_CURRENT_SOURCE_DIR}/idl)
#file(GLOB TMP_GEN_IDL ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
#file(COPY ${TMP_GEN_IDL} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR})
#file(REMOVE_RECURSE ${CMAKE_CURRENT_SOURCE_DIR}/src)
###################


###################
## The include directory 
###################
include_directories(include/)
include_directories(idl/include/)
###################


###################
## The executable
###################
source_group("Source Files" FILES ${SRC_FILES})
source_group("Header Files" FILES ${SRC_HEADERS})
source_group("IDL Files"    FILES ${IDL})


add_executable(${MODULENAME} ${SRC_FILES} ${SRC_HEADERS} ${IDL})
target_link_libraries(${MODULENAME} ${YARP_LIBRARIES} icubmod)

if(WIN32)
    install(TARGETS ${MODULENAME} DESTINATION bin/${CMAKE_BUILD_TYPE})
else(WIN32)
    install(TARGETS ${MODULENAME} DESTINATION bin)
endif(WIN32)
###################
