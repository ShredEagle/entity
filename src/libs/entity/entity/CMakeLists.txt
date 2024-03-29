set(TARGET_NAME entity)

set(${TARGET_NAME}_HEADERS
    Archetype.h
    ArchetypeStore.h
    Component.h
    Entity.h
    EntityManager.h
    HandleKey.h
    Query.h
    QueryStore.h
    Wrap.h

    detail/CloningPointer.h
    detail/HandledStore.h
    detail/Invoker.h
    detail/QueryBackend.h
)

set(${TARGET_NAME}_SOURCES
    Archetype.cpp
    ArchetypeStore.cpp
    Entity.cpp
    EntityManager.cpp
    Query.cpp
    QueryStore.cpp
)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR}
             FILES ${${TARGET_NAME}_HEADERS} ${${TARGET_NAME}_SOURCES}
)

add_library(${TARGET_NAME}
            ${${TARGET_NAME}_SOURCES}
            ${${TARGET_NAME}_HEADERS}
)

add_library(ad::${TARGET_NAME} ALIAS ${TARGET_NAME})

cmc_target_current_include_directory(${TARGET_NAME})

option(BUILD_ENTITY_SANITIZE "Build entity with sanitize assertions enabled." OFF)
if(BUILD_ENTITY_SANITIZE)
    target_compile_definitions(${TARGET_NAME} PUBLIC "ENTITY_SANITIZE")
endif()

set_target_properties(${TARGET_NAME} PROPERTIES
                      VERSION "${${PROJECT_NAME}_VERSION}"
)

cmc_cpp_all_warnings_as_errors(${TARGET_NAME} ENABLED ${BUILD_CONF_WarningAsError})

cmc_cpp_sanitizer(${TARGET_NAME} ${BUILD_CONF_Sanitizer})



##
## Dependencies
##

cmc_find_dependencies()

target_link_libraries(${TARGET_NAME}
    PUBLIC ad::handy
)


##
## Install
##

install(TARGETS ${TARGET_NAME} EXPORT ${TARGET_NAME}Targets)

include(cmc-install)
# Install the headers preserving the directory hierarchy
cmc_install_with_folders(FILES ${${TARGET_NAME}_HEADERS}
                         DESTINATION include/${TARGET_NAME}/${TARGET_NAME}
)
# Prepare CMake package in both build and install trees
cmc_install_packageconfig(${TARGET_NAME} ${TARGET_NAME}Targets ${PROJECT_NAME}
                          FIND_FILE "CMakeFinds.cmake.in"
                          NAMESPACE ad::
)
