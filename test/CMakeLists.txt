set(UNIT_TEST signalRecorderTest)

include_directories(${${PROJECT_NAME}_SOURCE_DIR}/src)
include_directories(${${PROJECT_NAME}_SOURCE_DIR}/test)
add_executable(${UNIT_TEST} SignalRecorderTest.cpp SignalRecorderTest.h)
target_link_libraries(${UNIT_TEST} ${PROJECT_NAME} Qt5::Core Qt5::Test)
add_custom_command(
    TARGET ${UNIT_TEST}
    COMMENT "Run tests"
    POST_BUILD
    COMMAND ${UNIT_TEST}
)
