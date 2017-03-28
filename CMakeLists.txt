cmake_minimum_required(VERSION 3.6)

find_package(PkgConfig REQUIRED)

set(SCROLLBACK_LINES 4096)
set(WORD_CHARS "-A-Za-z0-9:./?%&#_=+@~")
set(SCROLLBAR FALSE)

set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
configure_file("${SOURCE_DIR}/config.h.in" "${SOURCE_DIR}/config.h")

# Set CMAKE_LIB_INSTALL_DIR if not defined
include(GNUInstallDirs)

if (${CMAKE_SYSTEM} MATCHES "^Linux*")
    message("Target platform: Linux")
    add_definitions("-DLinux")
elseif (${CMAKE_SYSTEM} MATCHES "^FreeBSD*")
    message("Target platform: FreeBSD")
    add_definitions("-DBSD")
elseif (${CMAKE_SYSTEM} MATCHES "^Darwin*")
    message("Target platform: macOS/OSX")
    add_definitions("-DOSX")
else ()
    message("Unknown target OS, supposing Linux. Detected OS is: " ${CMAKE_SYSTEM})
    add_definitions("-DLinux")
endif ()

add_definitions("-DUDEBUG=5")

include_directories(${GTK_INCLUDE_DIRS})

pkg_search_module(VTE REQUIRED vte)

if (VTE_FOUND)
    add_definitions(-DHAVE_LIBVTE)
    link_directories(${VTE_LIBRARY_DIRS})
    include_directories(${VTE_INCLUDE_DIRS})
else ()
    message("Fatal error -- VTE wasn't found...")
endif ()

add_executable(tinyterm "${SOURCE_DIR}/tinyterm.c")
target_link_libraries(tinyterm ${VTE_LIBRARIES})

install(TARGETS tinyterm
        RUNTIME DESTINATION bin)

install(FILES "${SOURCE_DIR}/tinyterm.png"
        DESTINATION share/pixmaps)
