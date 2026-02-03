####
# cmake/toolchain/zephyr.cmake:
#
# A toolchain for Zephyr + F Prime integration. It performs the following:
# 1. Sets the FPRIME_PLATFORM to "Zephyr"
# 3. Supplies a function `register_fprime_zephyr_deployment` that registers a deployment/zephyr application
####
set(FPRIME_PLATFORM Zephyr)
set(EMPTY_SOURCE "${CMAKE_CURRENT_LIST_DIR}/empty.cpp")
####
# Function `register_fprime_zephyr_deployment`:
#
# Zephyr expects to build the application in a specific target called `app`. However,
# F Prime's CMake system attaches deployment information to the deployment executable, which
# cannot link in the Zephyr context.
#
# This function provides an alternate deployment registration that allows both build systems
# to work together.
#
# The F Prime deployment is satisfied by:
#  1. `fprime-zephyr-deployment` is created as a library with a single empty source
#  2. `fprime-zephyr-deployment` is registered is set as a "Deployment"
#  3. F Prime custom deployment targets are attached to `fprime-zephyr-deployment`
# 
# Zephyr's `app` target is satisfied by:
# 1. Adding supplied SOURCES to the `app` target
# 2. Registering supplied and default implementations to the `app` target
# 4. Linking `fprime-zephyr-deployment` to the `app` target
#
# This is all wrapped up in a bow by adding a custom target that takes the place of the deployment target
# and builds the final zephyr target and installs the files to the install directory.
#
# Args: 
#    BUILD_TARGET_NAME: name of the build target to register
#    ARGN: see `register_fprime_deployment` for details
###
function(register_fprime_zephyr_deployment)
    # Cannot run until the F Prime build system is loaded
    include(utilities)
    set(BUILD_TARGET_NAME "${FPRIME_CURRENT_MODULE}")
    cmake_parse_arguments(INTERNAL "EXCLUDE_FROM_ALL" "" "CHOOSES_IMPLEMENTATIONS;SOURCES;HEADERS;DEPENDS" ${ARGN})
    list(LENGTH INTERNAL_UNPARSED_ARGUMENTS UNPARSED_COUNT)
    fprime_cmake_ASSERT("Failed to parse: ${INTERNAL_UNPARSED_ARGUMENTS}" UNPARSED_COUNT LESS 2)
    if (INTERNAL_UNPARSED_ARGUMENTS)
        list(LENGTH INTERNAL_UNPARSED_ARGUMENTS UNKNOWN_LENGTH)
        fprime_cmake_ASSERT("Unknown argument supplied: ${INTERNAL_UNPARSED_ARGUMENTS}"  UNKNOWN_LENGTH EQUAL 1) 
        set(BUILD_TARGET_NAME "${INTERNAL_UNPARSED_ARGUMENTS}")
    endif()
    # Register an interface library substituted as the F Prime deployment for the purposes of setting up the fprime
    # deployment specific targets. We cannot use "app" here, because it was defined in another CMake directory
    # and so we use a specific target name.
    fprime_cmake_ASSERT("Zephyr supports only one deployment per project" NOT TARGET fprime-zephyr-deployment)
    add_library(fprime-zephyr-deployment "${EMPTY_SOURCE}")
    fprime_target_dependencies(fprime-zephyr-deployment PUBLIC ${INTERNAL_DEPENDS})
    set_target_properties(fprime-zephyr-deployment PROPERTIES
        FPRIME_TYPE Deployment
    )
    fprime_attach_custom_targets(fprime-zephyr-deployment)

    # Set up app using the supplied sources and implementations
    target_sources(app PRIVATE ${INTERNAL_SOURCES})
    fprime_target_implementations(app ${INTERNAL_CHOOSES_IMPLEMENTATIONS})
    target_link_libraries(app PRIVATE fprime-zephyr-deployment)


    # Add a custom target built by this module to do the final zephyr install. This will be named based on what
    # should have been the deployment name, will depend on the zephyr-final binary, and will perform the installation
    # of the outputs.
    install(
        DIRECTORY ${CMAKE_BINARY_DIR}/zephyr/
        COMPONENT fprime-zephyr-binaries
        DESTINATION ${CMAKE_INSTALL_PREFIX}
        FILES_MATCHING PATTERN "zephyr/zephyr.*"
        REGEX "zephyr/[^z]" EXCLUDE
    )
    add_custom_target(${BUILD_TARGET_NAME} ALL DEPENDS zephyr_final fprime-zephyr-deployment_dictionary
        COMMAND "${CMAKE_COMMAND}"
            -DCMAKE_INSTALL_COMPONENT=fprime-zephyr-binaries -P ${CMAKE_BINARY_DIR}/cmake_install.cmake
    
    )
    # Integration with fprime-util is now done via special methods that write-out special metadata. Since we've
    # defined a new way of registering deployments, we must also replicate the target integration metadata.
    include(fprime-util)
    fprime_util_metadata_add_build_target("${BUILD_TARGET_NAME}")
endfunction()

# Patch in std-atomic implementations
if (BOARD STREQUAL "rpi_pico" OR FPRIME_ZEPHYR_USE_STD_ATOMIC_FIX)
    include_directories(BEFORE "${CMAKE_CURRENT_LIST_DIR}/../../fprime-zephyr/Os/StdAtomic")
endif()
