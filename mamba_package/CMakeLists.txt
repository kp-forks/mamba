# Copyright (c) 2019, QuantStack and Mamba Contributors
#
# Distributed under the terms of the BSD 3-Clause License.
#
# The full license is in the file LICENSE, distributed with this software.

cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0025 NEW) # Introduced in cmake 3.0
cmake_policy(SET CMP0077 NEW) # Introduced in cmake 3.13

project(mamba-package)
include(GNUInstallDirs)

# Source files
# ============

set(
    MAMBA_PACKAGE_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/package.cpp
)

set(MAMBA_PACKAGE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/package.hpp)

# Dependencies
# ============

if(NOT TARGET mamba::libmamba)
    find_package(libmamba REQUIRED)
endif()

# Build definition
# ================

add_executable(mamba-package ${MAMBA_PACKAGE_SRCS} ${MAMBA_PACKAGE_HEADERS})
mamba_target_add_compile_warnings(mamba-package WARNING_AS_ERROR ${MAMBA_WARNING_AS_ERROR})

target_link_libraries(mamba-package PRIVATE mamba::libmamba)

target_compile_features(mamba-package PUBLIC cxx_std_20)
set_target_properties(
    mamba-package
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

install(
    TARGETS mamba-package
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
