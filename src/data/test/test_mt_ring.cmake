cmake_minimum_required(VERSION 3.10)

if(${CMAKE_VERSION} VERSION_LESS "3.17.0")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${WORKING_DIRECTORY}/mt_ring_input
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${WORKING_DIRECTORY}/mt_ring_output
    COMMAND ${CMAKE_COMMAND} -E remove -f ${WORKING_DIRECTORY}/mt_ring_supplier.json
    COMMAND ${CMAKE_COMMAND} -E remove -f ${WORKING_DIRECTORY}/mt_ring_supplier.json.lock
    COMMAND ${CMAKE_COMMAND} -E remove -f ${WORKING_DIRECTORY}/mt_ring_aggregator.json
    COMMAND ${CMAKE_COMMAND} -E remove -f ${WORKING_DIRECTORY}/mt_ring_aggregator.json.lock
  )
else()
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E rm -Rf ${WORKING_DIRECTORY}/mt_ring_input
    COMMAND ${CMAKE_COMMAND} -E rm -Rf ${WORKING_DIRECTORY}/mt_ring_output
    COMMAND ${CMAKE_COMMAND} -E rm -Rf ${WORKING_DIRECTORY}/mt_ring_supplier.json
    COMMAND ${CMAKE_COMMAND} -E rm -Rf ${WORKING_DIRECTORY}/mt_ring_supplier.json.lock
    COMMAND ${CMAKE_COMMAND} -E rm -Rf ${WORKING_DIRECTORY}/mt_ring_aggregator.json
    COMMAND ${CMAKE_COMMAND} -E rm -Rf ${WORKING_DIRECTORY}/mt_ring_aggregator.json.lock
  )
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E make_directory ${WORKING_DIRECTORY}/mt_ring_input
  COMMAND ${CMAKE_COMMAND} -E make_directory ${WORKING_DIRECTORY}/mt_ring_output
)

execute_process(
  COMMAND ${CMD} ${ARGS} --log-file mt_ring.log
  WORKING_DIRECTORY ${WORKING_DIRECTORY}
  RESULTS_VARIABLE CMD_RESULT
  TIMEOUT 20
)

# look for results of all commands
set(EXPECTED_RESULT 0)

if(NOT(CMD_RESULT STREQUAL EXPECTED_RESULT))
  message(FATAL_ERROR "Result is ${CMD_RESULT}")
endif()
