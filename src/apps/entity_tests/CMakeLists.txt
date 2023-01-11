string(TOLOWER ${PROJECT_NAME} _lower_project_name)
set(TARGET_NAME ${_lower_project_name}_tests)

set(${TARGET_NAME}_HEADERS
    catch.hpp

    Components_helpers.h
    Inspector.h
)

set(${TARGET_NAME}_SOURCES
    Phase_tests.cpp
    Query_tests.cpp
    QueryIteration_tests.cpp
    QueryEvent_tests.cpp
    StateBackup_tests.cpp
    Usage_tests.cpp
    main.cpp
)

add_executable(${TARGET_NAME}
               ${${TARGET_NAME}_HEADERS}
               ${${TARGET_NAME}_SOURCES}
)

target_link_libraries(${TARGET_NAME}
    PRIVATE
        ad::entity
)

cmc_cpp_all_warnings_as_errors(${TARGET_NAME} ENABLED ${BUILD_CONF_WarningAsError})

cmc_cpp_sanitizer(${TARGET_NAME} ${BUILD_CONF_Sanitizer})

set_target_properties(${TARGET_NAME} PROPERTIES
                      VERSION "${${PROJECT_NAME}_VERSION}"
)

install(TARGETS ${TARGET_NAME})
