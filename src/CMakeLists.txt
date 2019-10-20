set(SOURCES QSignalRecorder.cpp QSignalRecorder.h shared_export.h)

add_definitions(-DENABLE_SHARED_LIBRARY)

add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Core)
