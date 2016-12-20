cmake_minimum_required(VERSION 3.2)
project(GlPractice)

set(EX_PROJ_SOURCE_DIR ${CMAKE_BINARY_DIR}/deps/Source)
set(EX_PROJ_BUILD_DIR ${CMAKE_BINARY_DIR}/deps/Build)

find_package(OpenGL REQUIRED) 
include_directories(${OPENGL_INCLUDE_DIR})
if(UNIX)
    find_package(X11 REQUIRED)
endif(UNIX)

configure_file(deps-CMakeLists.txt deps/CMakeLists.txt COPYONLY)
execute_process(COMMAND ${CMAKE_COMMAND} . WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/deps)
execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/deps)

include_directories(${EX_PROJ_SOURCE_DIR}/STB_IMAGE_EX
					${EX_PROJ_SOURCE_DIR}/GLM_EX/glm
                    ${EX_PROJ_SOURCE_DIR}/GLFW_EX/include
                    ${EX_PROJ_SOURCE_DIR}/GLEW_EX/include)

add_subdirectory(${CMAKE_BINARY_DIR}/../src)
add_subdirectory(${CMAKE_BINARY_DIR}/../include)

set(ABSOLUTE_SRC_FILES "")
set(ABSOLUTE_HDR_FILES "")
foreach(filen ${SRC_FILES})
	list(APPEND ABSOLUTE_SRC_FILES "src/${filen}")
endforeach(filen)
foreach(filen ${HDR_FILES})
	list(APPEND ABSOLUTE_HDR_FILES "include/${filen}")
endforeach(filen)
include_directories(${CMAKE_BINARY_DIR}/../include)


add_executable(GLPractice ${ABSOLUTE_SRC_FILES} ${ABSOLUTE_HDR_FILES})

set(XRENDER_LIBS Xrandr Xcursor Xinerama Xxf86vm X11)

add_definitions(-DGLEW_STATIC)
if(WIN32)
	# set_target_properties(StarEngine PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
	# set_target_properties(StarEngine PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
	# set_target_properties(StarEngine PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
	# set_target_properties(StarEngine PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE")
	# set_target_properties(StarEngine PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:windows")
	# set_target_properties(StarEngine PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:windows")
    target_link_libraries(GLPractice debug ${EX_PROJ_BUILD_DIR}/GLFW_EX/src/Debug/glfw3${CMAKE_STATIC_LIBRARY_SUFFIX})
    target_link_libraries(GLPractice debug ${EX_PROJ_BUILD_DIR}/GLEW_EX/lib/Debug/glewd${CMAKE_STATIC_LIBRARY_SUFFIX})
    target_link_libraries(GLPractice optimized ${EX_PROJ_BUILD_DIR}/GLFW_EX/src/Release/glfw3${CMAKE_STATIC_LIBRARY_SUFFIX})
    target_link_libraries(GLPractice optimized ${EX_PROJ_BUILD_DIR}/GLEW_EX/lib/Release/glew${CMAKE_STATIC_LIBRARY_SUFFIX})
    target_link_libraries(GLPractice ${OPENGL_LIBRARIES})
	add_custom_command(TARGET GLPractice POST_BUILD
		COMMAND XCOPY \"$(SolutionDir)..\\res" \"$(TargetDir)res\\" /S /Y /I)
endif(WIN32)
if(UNIX)
    target_link_libraries(GLPractice ${EX_PROJ_BUILD_DIR}/GLFW_EX/src/libglfw3${CMAKE_STATIC_LIBRARY_SUFFIX})
    target_link_libraries(GLPractice ${EX_PROJ_BUILD_DIR}/GLEW_EX/lib/libglew${CMAKE_STATIC_LIBRARY_SUFFIX})
    target_link_libraries(GLPractice ${OPENGL_LIBRARIES} ${XRENDER_LIBS} dl pthread)
endif(UNIX)