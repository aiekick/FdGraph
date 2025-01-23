cmake_minimum_required(VERSION 3.20)

set(PROJECT Common)

enable_language(C CXX)
project(${PROJECT} CXX)

###########################################################
###########################################################
###########################################################

# Common sources
file(GLOB_RECURSE COMMON_SOURCES ${CMAKE_SOURCE_DIR}/common/*.*)
source_group(TREE ${CMAKE_SOURCE_DIR}/common PREFIX common FILES ${COMMON_SOURCES})

if (MSVC)
	# Doc Sources
	file(GLOB_RECURSE DOC_SOURCES ${CMAKE_SOURCE_DIR}/doc/*.*)
	source_group(TREE ${CMAKE_SOURCE_DIR}/doc PREFIX doc FILES ${DOC_SOURCES})
	set_property(SOURCE ${DOC_SOURCES} PROPERTY VS_SETTINGS "ExcludedFromBuild=true")
endif()

# cmake Sources for have it the visual studio tree of the App
file(GLOB_RECURSE PROJECT_CMAKE_FILES ${CMAKE_SOURCE_DIR}/cmake/*.cmake)
source_group(TREE ${CMAKE_SOURCE_DIR}/cmake PREFIX cmake FILES ${PROJECT_CMAKE_FILES})

file(GLOB IMGUI_IMPL_SOURCES 
	${CMAKE_SOURCE_DIR}/3rdparty/imguipack/3rdparty/imgui_docking/backends/imgui_impl_glfw.cpp
	${CMAKE_SOURCE_DIR}/3rdparty/imguipack/3rdparty/imgui_docking/backends/imgui_impl_glfw.h
	${CMAKE_SOURCE_DIR}/3rdparty/imguipack/3rdparty/imgui_docking/backends/imgui_impl_opengl3.cpp
	${CMAKE_SOURCE_DIR}/3rdparty/imguipack/3rdparty/imgui_docking/backends/imgui_impl_opengl3.h
	${CMAKE_SOURCE_DIR}/3rdparty/imguipack/3rdparty/imgui_docking/misc/freetype/imgui_freetype.cpp
	${CMAKE_SOURCE_DIR}/3rdparty/imguipack/3rdparty/imgui_docking/misc/freetype/imgui_freetype.h)
source_group(common/ImGuiImpl FILES ${IMGUI_IMPL_SOURCES})

add_library(${PROJECT} STATIC 
	${DOC_SOURCES}
	${COMMON_SOURCES}
	${EZ_LIBS_SOURCES}
	${IMGUI_IMPL_SOURCES}
	${PROJECT_CMAKE_FILES}
)

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

target_link_libraries(${PROJECT} PUBLIC
	${IMGUIPACK_LIBRARIES}
	${OPENGL_LIBRARIES}
	${GLFW_LIBRARIES}
	${GLAD_LIBRARIES}
)
	
set_target_properties(${PROJECT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${FINAL_BIN_DIR}")
set_target_properties(${PROJECT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${FINAL_BIN_DIR}")
set_target_properties(${PROJECT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${FINAL_BIN_DIR}")
set_target_properties(${PROJECT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FINAL_BIN_DIR}")

########################################################
##### TESTS ############################################
########################################################

if(USE_BUILDING_OF_TESTS)
	enable_testing()
endif()
