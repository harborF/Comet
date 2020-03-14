project(CometServer)
cmake_minimum_required(VERSION 2.8)


#########################################################################################
# Root folders
set(ProjectRootDir ${${PROJECT_NAME}_SOURCE_DIR}/)
set(ProjectBinaryDir ${${PROJECT_NAME}_BINARY_DIR}/)

# Source Subfolders
set(AclRootDir /usr/local/include/acl)
SET(EXECUTABLE_OUTPUT_PATH ${ProjectRootDir}/bin)
SET(LIBRARY_OUTPUT_PATH ${ProjectRootDir}/lib)

#-DUSE_THRIFT=on
#thrift -r --gen cpp -o ./ StateSvr.thrift

OPTION(USE_THRIFT "Option description" ON)
IF(USE_THRIFT)
	ADD_DEFINITIONS(-DUSE_THRIFT)
	MESSAGE("--use thrift on")

	set(ProtosDir ./src/thrift_protos/gen-cpp)
	set(SRC_proxy ./src/thrift_protos/thrift_proxy.cpp)
	SET(SRC_protos ${ProtosDir}/StateSvr_constants.cpp ${ProtosDir}/StateSvr.cpp ${ProtosDir}/StateSvr_types.cpp)
ENDIF(USE_THRIFT)

# All following targets should search these directories for headers
include_directories( 
  ${ProjectRootDir}/src
	${AclRootDir}/dist/include
	${AclRootDir}/dist/include/acl
	${AclRootDir}/lib_fiber/c/include
	${AclRootDir}/lib_fiber/cpp/include
	/usr/local/include/thrift
	${ProtosDir}
  ${ProjectRootDir}/src/thrift_protos
)

LINK_DIRECTORIES(
	${AclRootDir}/dist/lib/linux64
	${AclRootDir}/lib_fiber/lib
)
ADD_DEFINITIONS(-DLINUX2 -D_USE_FAST_MACRO -Wno-deprecated)

aux_source_directory(./src/common SRC_common)
aux_source_directory(./src/server SRC_main)

aux_source_directory(./src/logic SRC_logic)
add_executable("master_server" ${SRC_logic} ${SRC_main} ${SRC_common} ${SRC_protos} ${SRC_proxy})
SET(CMAKE_CXX_FLAGS "-g -O3 -pthread")
SET_TARGET_PROPERTIES("master_server" PROPERTIES LINKER_LANGUAGE CXX)
IF(USE_THRIFT)
	target_link_libraries("master_server" thrift)
ENDIF(USE_THRIFT)
target_link_libraries("master_server" _fiber _fiber_cpp _acl_cpp _protocol _acl dl z jsoncpp curlpp curl)  


aux_source_directory(./src/client SRC_client)
add_executable("acl_client" ${SRC_client} ${SRC_common})
SET(CMAKE_CXX_FLAGS " -std=c++11 -g -O3 -pthread")
SET_TARGET_PROPERTIES("acl_client" PROPERTIES LINKER_LANGUAGE CXX)
IF(USE_THRIFT)
	target_link_libraries("acl_client" thrift)
ENDIF(USE_THRIFT)
target_link_libraries("acl_client" _fiber _fiber_cpp _acl_cpp _protocol _acl dl z jsoncpp curlpp curl)

IF(USE_THRIFT)
	aux_source_directory(./src/thrift_svr SRC_thrift)
	add_executable("thrift_server" ${SRC_thrift} ${SRC_protos})
	SET(CMAKE_CXX_FLAGS " -std=c++11 -g -O3 -pthread")
	SET_TARGET_PROPERTIES("thrift_server" PROPERTIES LINKER_LANGUAGE CXX)
	target_link_libraries("thrift_server" thrift thriftnb event _acl_cpp _protocol _acl jsoncpp)  
ENDIF(USE_THRIFT)
