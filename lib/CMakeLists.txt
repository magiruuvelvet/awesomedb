set(CURRENT_TARGET "libawesomedb")
set(CURRENT_TARGET_NAME "libawesomedb++")
set(CURRENT_TARGET_INTERFACE "awesomedb")

message(STATUS "Configuring ${CURRENT_TARGET}...")

CreateTarget(${CURRENT_TARGET} STATIC ${CURRENT_TARGET_NAME} 20)

# Qt
find_package(Qt5Core REQUIRED)
find_package(Qt5Sql REQUIRED)

# Qt deprecated warnings
target_compile_definitions(${CURRENT_TARGET} PRIVATE -DQT_DEPRECATED_WARNINGS)
target_compile_definitions(${CURRENT_TARGET} PRIVATE -DQT_DISABLE_DEPRECATED_BEFORE=0x060000)

# disable Qt foreach macro
target_compile_definitions(${CURRENT_TARGET} PRIVATE -DQT_NO_FOREACH)

target_link_libraries(${CURRENT_TARGET}
    PRIVATE
        Qt5::Sql
    PUBLIC
        Qt5::Core
        fmt
)

add_library(${CURRENT_TARGET_INTERFACE} INTERFACE)
target_include_directories(${CURRENT_TARGET_INTERFACE} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${CURRENT_TARGET_INTERFACE} INTERFACE ${CURRENT_TARGET})

message(STATUS "Configured ${CURRENT_TARGET}.")
