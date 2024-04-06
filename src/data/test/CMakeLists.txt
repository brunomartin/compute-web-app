cmake_minimum_required(VERSION 3.10)

project(CwaDataTest)

configure_file(run_test.sh ${CMAKE_BINARY_DIR} COPYONLY)

add_executable(CwaDataTestMtRing data_test_mt_ring.cpp)
target_link_libraries(CwaDataTestMtRing CwaData)

add_test(NAME CwaDataTestMtRingSocket
  COMMAND ${CMAKE_COMMAND}
    -DCMD=$<TARGET_FILE:CwaDataTestMtRing>
    -DARGS=--asio
    -DWORKING_DIRECTORY=$<TARGET_FILE_DIR:CwaDataTestMtRing>
  -P ${CMAKE_CURRENT_SOURCE_DIR}/test_mt_ring.cmake
)

add_test(NAME CwaDataTestMtRingFile
  COMMAND ${CMAKE_COMMAND}
    -DCMD=$<TARGET_FILE:CwaDataTestMtRing>
    -DARGS=--file
    -DWORKING_DIRECTORY=$<TARGET_FILE_DIR:CwaDataTestMtRing>
  -P ${CMAKE_CURRENT_SOURCE_DIR}/test_mt_ring.cmake
)

add_executable(CwaDataTestMpRingServer data_test_mp_ring_server.cpp)
target_link_libraries(CwaDataTestMpRingServer CwaData)

add_executable(CwaDataTestMpRingClient data_test_mp_ring_client.cpp)
target_link_libraries(CwaDataTestMpRingClient CwaData)

add_test(NAME CwaDataTestMpRingSocket
  COMMAND ${CMAKE_COMMAND}
    -DCMD_CLIENT=$<TARGET_FILE:CwaDataTestMpRingClient>
    -DCMD_SERVER=$<TARGET_FILE:CwaDataTestMpRingServer>
    -DWORKING_DIRECTORY=$<TARGET_FILE_DIR:CwaDataTestMpRingServer>
  -DARGS=--asio
  -P ${CMAKE_CURRENT_SOURCE_DIR}/test_mp_ring.cmake
)

add_test(NAME CwaDataTestMpRingFile
  COMMAND ${CMAKE_COMMAND}
    -DCMD_CLIENT=$<TARGET_FILE:CwaDataTestMpRingClient>
    -DCMD_SERVER=$<TARGET_FILE:CwaDataTestMpRingServer>
    -DWORKING_DIRECTORY=$<TARGET_FILE_DIR:CwaDataTestMpRingServer>
  -DARGS=--file
  -P ${CMAKE_CURRENT_SOURCE_DIR}/test_mp_ring.cmake
)
