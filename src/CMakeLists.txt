# CMakeLists.txt for the actual program

set(HOLGOL_SERVER_SOURCES
	basewss.cpp
	main.cpp
	holgolwss.cpp
	consoleinteraction.cpp
	${PROJECT_SOURCE_DIR}/include/basewss.h
	${PROJECT_SOURCE_DIR}/include/holgolwss.h
	${PROJECT_SOURCE_DIR}/include/consoleinteraction.h
)

add_executable(holgol_server ${HOLGOL_SERVER_SOURCES})

target_include_directories(holgol_server PRIVATE ${PROJECT_SOURCE_DIR}/vendor/websocketpp)

target_include_directories(holgol_server PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(holgol_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(holgol_server PRIVATE ${PROJECT_SOURCE_DIR}/include)

# PCH the common include
# target_precompile_headers(holgol_server PUBLIC ${PROJECT_SOURCE_DIR}/include/common.h)

# Enforce C++17 support
set_property(TARGET holgol_server PROPERTY CXX_STANDARD 17)
set_property(TARGET holgol_server PROPERTY CXX_STANDARD_REQUIRED ON)

# Link with any needed libraries
target_link_libraries(holgol_server ${CMAKE_THREAD_LIBS_INIT} Boost::system Boost::json)
