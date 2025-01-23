cmake_minimum_required(VERSION 3.20)

set(PROJECT FdGraphClient)

enable_language(C CXX)
project(${PROJECT} CXX)

###########################################################
###########################################################
###########################################################

# Main sources
file(GLOB MAIN_SOURCES ${CMAKE_SOURCE_DIR}/main.cpp)
source_group(main FILES ${MAIN_SOURCES})

# App Sources
file(GLOB_RECURSE SRC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.*)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC_SOURCES})

add_executable(${PROJECT}
	${MAIN_SOURCES}
	${SRC_SOURCES}
)

if (USE_SHARED_LIBS)
	target_compile_definitions(${PROJECT} PRIVATE BUILD_IMGUI_PACK_SHARED_LIBS)	
endif()

set_target_properties(${PROJECT} PROPERTIES OUTPUT_NAME "${PROJECT}_${CMAKE_SYSTEM_NAME}_${ARCH}")

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}")
install(TARGETS ${PROJECT} DESTINATION ${CMAKE_INSTALL_PREFIX})

include_directories(
	${CMAKE_SOURCE_DIR}/3rdparty
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/common
	${IMGUIPACK_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIR}
	${PLOG_INCLUDE_DIR}
	${GLFW_INCLUDE_DIR}
	${GLAD_INCLUDE_DIR}
)

set(LIBS Common)
if(WIN32)
	set(LIBS ${LIBS})
endif()

target_link_libraries(${PROJECT}
	${LIBS}
)


set_target_properties(${PROJECT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${FINAL_BIN_DIR}")
set_target_properties(${PROJECT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${FINAL_BIN_DIR}")
set_target_properties(${PROJECT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${FINAL_BIN_DIR}")
set_target_properties(${PROJECT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FINAL_BIN_DIR}")


########################################################
################# BUILD INCREMENT TOOL #################
########################################################

add_custom_command(
	TARGET ${PROJECT} PRE_BUILD
	COMMAND BuildInc ${PROJECT} $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/client/${PROJECT}Build.h> -ff ${CMAKE_CURRENT_SOURCE_DIR}/common/res/big.flf --no-help)

########################################################
##### TESTS ############################################
########################################################

if(USE_BUILDING_OF_TESTS)
	enable_testing()
endif()
