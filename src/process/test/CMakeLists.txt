cmake_minimum_required(VERSION 3.10)

project(CwaProcessTest)

add_executable(CwaProcessTestVariant test_variant.cpp)
target_link_libraries(CwaProcessTestVariant CwaProcess)
add_test(NAME CwaProcessTestVariant COMMAND CwaProcessTestVariant) 

add_executable(CwaProcessTestParameter test_parameter.cpp)
target_link_libraries(CwaProcessTestParameter CwaProcess)
add_test(NAME CwaProcessTestParameter COMMAND CwaProcessTestParameter) 

add_executable(CwaProcessTestDefinition test_definition.cpp)
target_link_libraries(CwaProcessTestDefinition CwaProcess)
add_test(NAME CwaProcessTestDefinition COMMAND CwaProcessTestDefinition)

configure_file(run_test.sh ${CMAKE_BINARY_DIR}/run_process_test.sh COPYONLY)

set(
  WORKER_SOURCES
  test_worker.h
  test_worker.cpp
)

add_executable(CwaProcessTestWorkerApp test_worker_app.cpp ${WORKER_SOURCES})
target_link_libraries(CwaProcessTestWorkerApp CwaProcess)

add_executable(CwaProcessTestMainApp test_main_app.cpp ${WORKER_SOURCES})
target_link_libraries(CwaProcessTestMainApp CwaProcess)
add_dependencies(CwaProcessTestMainApp CwaProcessTestWorkerApp)

# Add multi thread tests
# supplier socket, aggregator socket
add_test(NAME CwaProcessTestMainMtSocketSocket
  COMMAND ${CMAKE_COMMAND}
    -DCMD=$<TARGET_FILE:CwaProcessTestMainApp>
    -DWORKER_TYPE=mt
    -DSUP_TYPE=socket
    -DAGG_TYPE=socket
    -DWORKING_DIRECTORY=$<TARGET_FILE_DIR:CwaProcessTestMainApp>
  -P ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cmake
)

# supplier socket, aggregator file
add_test(NAME CwaProcessTestMainMtSocketFile
  COMMAND ${CMAKE_COMMAND}
    -DCMD=$<TARGET_FILE:CwaProcessTestMainApp>
    -DWORKER_TYPE=mt
    -DSUP_TYPE=socket
    -DAGG_TYPE=file
    -DWORKING_DIRECTORY=$<TARGET_FILE_DIR:CwaProcessTestMainApp>
  -P ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cmake
)

# supplier file, aggregator socket
add_test(NAME CwaProcessTestMainMtFileSocket
  COMMAND ${CMAKE_COMMAND}
    -DCMD=$<TARGET_FILE:CwaProcessTestMainApp>
    -DWORKER_TYPE=mt
    -DSUP_TYPE=file
    -DAGG_TYPE=socket
    -DWORKING_DIRECTORY=$<TARGET_FILE_DIR:CwaProcessTestMainApp>
  -P ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cmake
)

# supplier file, aggregator file
add_test(NAME CwaProcessTestMainMtFileFile
  COMMAND ${CMAKE_COMMAND}
    -DCMD=$<TARGET_FILE:CwaProcessTestMainApp>
    -DWORKER_TYPE=mt
    -DSUP_TYPE=file
    -DAGG_TYPE=file
    -DWORKING_DIRECTORY=$<TARGET_FILE_DIR:CwaProcessTestMainApp>
  -P ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cmake
)

# Add multi process tests
# supplier socket, aggregator socket
add_test(NAME CwaProcessTestMainMpSocketSocket
  COMMAND ${CMAKE_COMMAND}
    -DCMD=$<TARGET_FILE:CwaProcessTestMainApp>
    -DWORKER_TYPE=mp
    -DSUP_TYPE=socket
    -DAGG_TYPE=socket
    -DWORKING_DIRECTORY=$<TARGET_FILE_DIR:CwaProcessTestMainApp>
  -P ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cmake
)

# supplier socket, aggregator file
add_test(NAME CwaProcessTestMainMpSocketFile
  COMMAND ${CMAKE_COMMAND}
    -DCMD=$<TARGET_FILE:CwaProcessTestMainApp>
    -DWORKER_TYPE=mp
    -DSUP_TYPE=socket
    -DAGG_TYPE=file
    -DWORKING_DIRECTORY=$<TARGET_FILE_DIR:CwaProcessTestMainApp>
  -P ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cmake
)

# supplier file, aggregator socket
add_test(NAME CwaProcessTestMainMpFileSocket
  COMMAND ${CMAKE_COMMAND}
    -DCMD=$<TARGET_FILE:CwaProcessTestMainApp>
    -DWORKER_TYPE=mp
    -DSUP_TYPE=file
    -DAGG_TYPE=socket
    -DWORKING_DIRECTORY=$<TARGET_FILE_DIR:CwaProcessTestMainApp>
  -P ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cmake
)

# supplier file, aggregator file
add_test(NAME CwaProcessTestMainMpFileFile
  COMMAND ${CMAKE_COMMAND}
    -DCMD=$<TARGET_FILE:CwaProcessTestMainApp>
    -DWORKER_TYPE=mp
    -DSUP_TYPE=file
    -DAGG_TYPE=file
    -DWORKING_DIRECTORY=$<TARGET_FILE_DIR:CwaProcessTestMainApp>
  -P ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cmake
)
