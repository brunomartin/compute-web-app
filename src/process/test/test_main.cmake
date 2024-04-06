cmake_minimum_required(VERSION 3.10)

if(${CMAKE_VERSION} VERSION_LESS "3.17.0")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${WORKING_DIRECTORY}/process_mt_input
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${WORKING_DIRECTORY}/process_mt_output
    COMMAND ${CMAKE_COMMAND} -E remove -f ${WORKING_DIRECTORY}/process_mt_supplier.json
    COMMAND ${CMAKE_COMMAND} -E remove -f ${WORKING_DIRECTORY}/process_mt_supplier.json.lock
    COMMAND ${CMAKE_COMMAND} -E remove -f ${WORKING_DIRECTORY}/process_mt_aggregator.json
    COMMAND ${CMAKE_COMMAND} -E remove -f ${WORKING_DIRECTORY}/process_mt_aggregator.json.lock
  )
else()
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E rm -Rf ${WORKING_DIRECTORY}/process_mt_input
    COMMAND ${CMAKE_COMMAND} -E rm -Rf ${WORKING_DIRECTORY}/process_mt_output
    COMMAND ${CMAKE_COMMAND} -E rm -Rf ${WORKING_DIRECTORY}/process_mt_supplier.json
    COMMAND ${CMAKE_COMMAND} -E rm -Rf ${WORKING_DIRECTORY}/process_mt_supplier.json.lock
    COMMAND ${CMAKE_COMMAND} -E rm -Rf ${WORKING_DIRECTORY}/process_mt_aggregator.json
    COMMAND ${CMAKE_COMMAND} -E rm -Rf ${WORKING_DIRECTORY}/process_mt_aggregator.json.lock
  )
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E make_directory ${WORKING_DIRECTORY}/process_mt_input
  COMMAND ${CMAKE_COMMAND} -E make_directory ${WORKING_DIRECTORY}/process_mt_output
)

execute_process(
  COMMAND ${CMD} --worker-type ${WORKER_TYPE} --cwa-sup ${SUP_TYPE} --cwa-agg ${AGG_TYPE}
    --log-file process_main.log
  WORKING_DIRECTORY ${WORKING_DIRECTORY}
  RESULTS_VARIABLE CMD_RESULT
  TIMEOUT 20
)

# look for results of all commands
set(EXPECTED_RESULT 0)

if(NOT(CMD_RESULT STREQUAL EXPECTED_RESULT))
  message(FATAL_ERROR "Result is ${CMD_RESULT}")
endif()
