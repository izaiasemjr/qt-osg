cmake_minimum_required(VERSION 2.8.11)
project(qtosg)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Gui OpenGL)
find_package(OpenSceneGraph REQUIRED COMPONENTS osgDB osgGA osgUtil osgViewer)

include_directories(${OPENSCENEGRAPH_INCLUDE_DIRS})


# ------------ Add 3MF -------------------------
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Extern/lib3mf/Cpp)
if (WIN32)
	# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/Lib)
	link_directories(${CMAKE_CURRENT_SOURCE_DIR}/Extern/lib3mf/shared/windows10)
	link_libraries(lib3mf)
	link_directories(${CMAKE_CURRENT_BINARY_DIR})
	get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
	get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
	find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
else()
	# Unix prefixes the name of the library with "lib" anyway
	link_directories(${CMAKE_CURRENT_SOURCE_DIR}/Extern/lib3mf/shared/ubuntu20)
	link_libraries(3mf)
	link_directories(${CMAKE_CURRENT_BINARY_DIR})
endif()
#-----------------------------------------------------------------


set(SOURCES
    main.cpp
    )

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    ${OPENSCENEGRAPH_LIBRARIES}
    Qt5::Core
    Qt5::Gui
    Qt5::OpenGL
    )
