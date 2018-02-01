cmake_minimum_required(VERSION 2.8)
cmake_policy(VERSION 2.8)

project(libpicobt C)
set(CMAKE_BUILD_TYPE Debug)
set(PROJECT_VERSION 0.0.2)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/")
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")
endif()

INCLUDE(GNUInstallDirs)


find_package(Check)
find_package(Threads)


# build libpicobt
file(GLOB SOURCES "src/*.c")
include_directories(include)
add_library(picobt SHARED ${SOURCES})
add_library(picobt_static STATIC ${SOURCES})
set_target_properties(picobt PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION "1"
)

# and link with the OS-specific Bluetooth library
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	target_link_libraries(picobt picobt_static ws2_32 Bthprops)
	set_target_properties(picobt_static PROPERTIES OUTPUT_NAME picobt)
else()
	target_link_libraries(picobt picobt_static bluetooth)
	set_target_properties(picobt_static PROPERTIES OUTPUT_NAME picobt)
endif()

# build test programs
add_executable(services "examples/services.c")
target_link_libraries(services picobt)

add_executable(discovery "examples/discovery.c")
target_link_libraries(discovery picobt)

add_executable(devicelist "examples/devicelist.c")
target_link_libraries(devicelist picobt)

add_executable(client-port "examples/client-port.c")
target_link_libraries(client-port picobt)

add_executable(server-port "examples/server-port.c")
target_link_libraries(server-port picobt)

add_executable(client-service "examples/client-service.c")
target_link_libraries(client-service picobt)

add_executable(server-service "examples/server-service.c")
target_link_libraries(server-service picobt)

# build tests with libcheck
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	file(GLOB SOURCES_TEST "tests/*.c")
	list(REMOVE_ITEM SOURCES_TEST ${CMAKE_SOURCE_DIR}/tests/test_main.c)
	include_directories(include)
	add_library(picobt_suites STATIC ${SOURCES_TEST})
	
	file(GLOB SOURCES_MOCK "tests/mock/*.c")
	add_library(picobt_mock SHARED ${SOURCES_MOCK})

	add_executable(tests_main "tests/test_main.c")
	target_link_libraries(tests_main picobt_suites picobt_mock picobt ${CHECK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif()

# On 64-bit Fedora, the library directory is lib64
# so we can let cmake define it for us
#set(PKG_CONFIG_LIBDIR "\${prefix}/lib")
set(PKG_CONFIG_INCLUDEDIR "\${prefix}/include/picobt")
set(PKG_CONFIG_LIBS	"-L\${libdir} -lpicobt -lbluetooth")
set(PKG_CONFIG_CFLAGS "-I\${includedir}")

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/pkg-config.pc.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
)

# Separate files into components: dev and run
install(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc" DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig COMPONENT dev)
install(DIRECTORY include/picobt DESTINATION include COMPONENT dev)
install(TARGETS picobt DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT run)
install(TARGETS picobt_static DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT dev)

# PACKAGING

# Set up general packaging information
set(CPACK_PACKAGE_NAME "libpicobt")
set(CPACK_PACKAGE_VENDOR "mypico.org")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Seb Aebischer")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Platform-neutral Bluetooth API")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_PATCH})

# Create two packages: runtime and development
set(CPACK_COMPONENTS_ALL run dev)
set(CPACK_COMPONENT_RUN_DISPLAY_NAME "Runtime")
set(CPACK_COMPONENT_DEV_DISPLAY_NAME "Development")

# Set human-readable descriptions
set(CPACK_COMPONENT_RUN_DESCRIPTION "Runtime shared libraries")
set(CPACK_COMPONENT_DEV_DESCRIPTION "Development static libraries and headers")

# Ensure the dev package also installs the runtime package
# The following line should set a dependency between packages but appears to have no effect for DEB
set(CPACK_COMPONENT_DEV_DEPENDS run)

# The following bug in CMake prevents automatic dependency generation
# http://public.kitware.com/Bug/view.php?id=13488
# This is fixed in CMake 3.3, but the runner has CMake 3.0.2
# Our workaround is to specify the dependencies explicitly
#set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_RUN_PACKAGE_DEPENDS "libbluetooth3 (>= 4.91), libc6 (>= 2.14)")
# The following line is needed either way
set(CPACK_DEBIAN_DEV_PACKAGE_DEPENDS "${CPACK_PACKAGE_NAME}-run (>= ${CPACK_PACKAGE_VERSION})")

# The above lines only work in cmake 3.3 and above. The following is a temporary fix in the meantime
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libbluetooth3 (>= 4.91), libc6 (>= 2.14)")

# Generate the architecture variable
# This should happen automatically; it's not clear why it doesn't
if (NOT DEB_PACKAGE${COMPONENT}_ARCH)
	execute_process(COMMAND
		dpkg-architecture -qDEB_BUILD_ARCH
		OUTPUT_VARIABLE CURRENT_ARCH
	)
	string(STRIP "${CURRENT_ARCH}" CURRENT_ARCH)
	set(DEB_PACKAGE${COMPONENT}_ARCH "${CURRENT_ARCH}")
endif()

# Set up the output package name to include the architecture
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}_${CPACK_PACKAGE_VERSION}_${DEB_PACKAGE${COMPONENT}_ARCH}")
SET(CPACK_OUTPUT_FILE_PREFIX packages)

# we want to generate debian packages split into components
set(CPACK_GENERATOR "DEB")
set(CPACK_DEB_COMPONENT_INSTALL ON)

# All of the set(CPACK) statements must precede the inclusion of CPack
include(CPack)
