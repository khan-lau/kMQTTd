project (kMQTTd CXX)

# cmake -DCMAKE_CXX_COMPILER=clang -DCMAKE_BUILD_TYPE=DEBUG ..
# 区分debug和release
# 在工程目录下，cmake -DCMAKE_BUILD_TYPE=DEBUG(RELEASE)，再执行make
# cmake -G "Xcode" -DCMAKE_OSX_ARCHITECTURES=x86_64;i386 ..  创建xcode工程
# cmake -G "Visual Studio 14 2015 Win64" ..   创建vc工程

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# SET(CMAKE_OSX_DEPLOYMENT_TARGET 10.11)

# MESSAGE("CMAKE_OSX_DEPLOYMENT_TARGET: "  ${CMAKE_OSX_DEPLOYMENT_TARGET})
MESSAGE( "CMAKE_CURRENT_SOURCE_DIR: "    ${CMAKE_CURRENT_SOURCE_DIR} ) # CMakeList.txt所在的目录
MESSAGE( "CMAKE_CURRENT_BINARY_DIR: "    ${CMAKE_CURRENT_BINARY_DIR} ) #外部编译时，指的是target目录，内部编译时，指的是顶级目录
MESSAGE( "CMAKE_CURRENT_LIST_DIR: "      ${CMAKE_CURRENT_LIST_DIR} )   #CMakeList.txt的完整路径
# MESSAGE( "CMAKE_CURRENT_LIST_LINE: "     ${CMAKE_CURRENT_LIST_LINE} )  #当前所在的行
MESSAGE( "CMAKE_MODULE_PATH: "           ${CMAKE_MODULE_PATH} )        #如果工程复杂，可能需要编写一些cmake模块，这里通过SET指定这个变量
MESSAGE( "LIBRARY_OUTPUT_DIR: "          ${LIBRARY_OUTPUT_DIR} )       #库和可执行的最终存放目录
MESSAGE( "BINARY_OUTPUT_DIR: "           ${BINARY_OUTPUT_DIR} )
MESSAGE( "PROJECT_NAME: "                ${PROJECT_NAME} )

# 是否支持c++17
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++20" COMPILER_SUPPORTS_CXX20)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
CHECK_CXX_COMPILER_FLAG("-std=c++1z" COMPILER_SUPPORTS_CXX1Z)
CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
CHECK_CXX_COMPILER_FLAG("-std=c++1y" COMPILER_SUPPORTS_CXX1Y)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)

if(COMPILER_SUPPORTS_CXX14) ## 判断是否支持c++14
    add_definitions("-std=c++14")
	MESSAGE(STATUS "The compiler ${CMAKE_CXX_COMPILER} has C++14 support.")
else()
    MESSAGE(STATUS "ERROR - The compiler ${CMAKE_CXX_COMPILER} has no C++14 support. Please use a different C++ compiler.")
	RETURN()
endif()


SET( BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/boost_1_67_0/build" )

#设置头文件的引用路径
SET( BOOST_HDRS "${BOOST_ROOT}/include" )
#设置引用库路径
SET( BOOST_LIB_PATCH "${BOOST_ROOT}/lib" )

FIND_PATH( BOOST_INCLUDE_DIR
		NAMES boost/config.hpp ${BOOST_HDRS}
        HINTS ${BOOST_HDRS} ${BOOST_HDRS}
        PATH_SUFFIXES boost_1_67_0 )
MESSAGE("found BOOST_INCLUDE_DIR:" ${BOOST_INCLUDE_DIR})
# ADD_DEFINITIONS( -DBoost_USE_STATIC_LIBS=ON )
find_package( Boost 1.67 REQUIRED COMPONENTS system date_time regex )
if(NOT Boost_LIBRARIES)  ## 如果未找到boost
    MESSAGE(STATUS "ERROR - Boost Not found.")
	RETURN()
endif()

INCLUDE_DIRECTORIES(
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/mqtt"
	"${BOOST_INCLUDE_DIR}"
)

LINK_DIRECTORIES(
	"${BOOST_LIB_PATCH}"
)

SET( CMAKE_VERBOSE_MAKEFILE  on )
SET( CMAKE_CXX_COMPILER      "clang" )
SET( CMAKE_CXX_FLAGS         "-Wall" )
SET( CMAKE_CXX_FLAGS_DEBUG   "-g3" )
SET( CMAKE_CXX_FLAGS_RELEASE "-O2" )

SET( CMAKE_C_COMPILER        "clang" )
SET( CMAKE_C_FLAGS           "-Wall" )
SET( CMAKE_C_FLAGS_DEBUG     "-g3" )
SET( CMAKE_C_FLAGS_RELEASE   "-O2" )

# 指定编译32bit或64bit程序
SET( CMAKE_C_FLAGS           "${CMAKE_C_FLAGS} -m64 " )
SET( CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -m64 " )
MESSAGE("Build 64bit Program.")


SET(SRC_LIST src/mqtt/VarTypes.h
	src/mqtt/Protocol.hpp
	src/mqtt/Protocol.cpp
	src/mqtt/Channel.hpp

	src/CppUtils.hpp
	src/Global.hpp
	src/MqttException.hpp
	src/Client.hpp
	src/MqttServer.hpp
	src/MqttClient.hpp
	src/ChTree.hpp
	src/MqttSession.hpp

	# src/kMQTTd.cpp
	# src/kMQTTc.cpp
	)

# ADD_SUBDIRECTORY(lib)
ADD_EXECUTABLE("kMQTTd" src/kMQTTd.cpp ${SRC_LIST})
ADD_EXECUTABLE("kMQTTc" src/kMQTTc.cpp ${SRC_LIST})


IF (BOOST_INCLUDE_DIR AND Boost_LIBRARIES)
	TARGET_LINK_LIBRARIES( "kMQTTd" ${Boost_LIBRARIES} stdc++ pthread)
	TARGET_LINK_LIBRARIES( "kMQTTc" ${Boost_LIBRARIES} stdc++ pthread)
ENDIF (BOOST_INCLUDE_DIR AND Boost_LIBRARIES)

##### Install target #####
INSTALL (TARGETS kMQTTd kMQTTc
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib/static
)
INSTALL (DIRECTORY conf DESTINATION bin)
INSTALL (FILES README.md LICENSE DESTINATION bin)


##### Uninstall target #####
if(NOT TARGET uninstall)
	CONFIGURE_FILE(
		"${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.in.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
		IMMEDIATE @ONLY
	)
  	add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()


