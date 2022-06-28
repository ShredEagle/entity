add_subdirectory(libs/entity/entity)

option (BUILD_tests "Build 'tests' application" true)
if(BUILD_tests)
    add_subdirectory(apps/entity_tests)
endif()
