prefix=${CMAKE_INSTALL_PREFIX}
exec_prefix=${CMAKE_INSTALL_PREFIX}
libdir=${CMAKE_INSTALL_PREFIX}/lib
includedir=${CMAKE_INSTALL_PREFIX}/include

Name: ${PROJECT_NAME}
Description: A library to provide bluetooth functions for the Pico Project 
URL: http://mypico.org
Version: ${PROJECT_VERSION}
Libs: ${PKG_CONFIG_LIBS}
Libs.private: 
Cflags: ${PKG_CONFIG_CFLAGS}

