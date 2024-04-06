cmake_minimum_required(VERSION 3.10)

if(${CMAKE_VERSION} VERSION_LESS "3.17.0")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${WORKING_DIRECTORY}/mp_ring_input
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${WORKING_DIRECTORY}/mp_ring_output
    COMMAND ${CMAKE_COMMAND} -E remove -f ${WORKING_DIRECTORY}/mp_ring_supplier.json
    COMMAND ${CMAKE_COMMAND} -E remove -f ${WORKING_DIRECTORY}/mp_ring_supplier.json.lock
    COMMAND ${CMAKE_COMMAND} -E remove -f ${WORKING_DIRECTORY}/mp_ring_aggregator.json
    COMMAND ${CMAKE_COMMAND} -E remove -f ${WORKING_DIRECTORY}/mp_ring_aggregator.json.lock
  )
else()
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E rm -Rf ${WORKING_DIRECTORY}/mp_ring_input
    COMMAND ${CMAKE_COMMAND} -E rm -Rf ${WORKING_DIRECTORY}/mp_ring_output
    COMMAND ${CMAKE_COMMAND} -E rm -Rf ${WORKING_DIRECTORY}/mp_ring_supplier.json
    COMMAND ${CMAKE_COMMAND} -E rm -Rf ${WORKING_DIRECTORY}/mp_ring_supplier.json.lock
    COMMAND ${CMAKE_COMMAND} -E rm -Rf ${WORKING_DIRECTORY}/mp_ring_aggregator.json
    COMMAND ${CMAKE_COMMAND} -E rm -Rf ${WORKING_DIRECTORY}/mp_ring_aggregator.json.lock
  )
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E make_directory ${WORKING_DIRECTORY}/mp_ring_input
  COMMAND ${CMAKE_COMMAND} -E make_directory ${WORKING_DIRECTORY}/mp_ring_output
)

if(${CMAKE_VERSION} VERSION_LESS "3.19.0")

  execute_process(
    COMMAND ${CMD_CLIENT} ${ARGS} --log-file mp_ring_client_1.log
    COMMAND ${CMD_CLIENT} ${ARGS} --log-file mp_ring_client_2.log
    COMMAND ${CMD_CLIENT} ${ARGS} --log-file mp_ring_client_3.log
    COMMAND ${CMD_CLIENT} ${ARGS} --log-file mp_ring_client_4.log
    COMMAND ${CMD_CLIENT} ${ARGS} --log-file mp_ring_client_5.log
    COMMAND ${CMD_CLIENT} ${ARGS} --log-file mp_ring_client_6.log
    COMMAND ${CMD_SERVER} ${ARGS} --log-file mp_ring_server.log
    WORKING_DIRECTORY ${WORKING_DIRECTORY}
    RESULTS_VARIABLE CMD_RESULT
    TIMEOUT 20
  )

  # look for results of all commands
  set(EXPECTED_RESULT 0 0 0 0 0 0 0)

  if(NOT(CMD_RESULT STREQUAL EXPECTED_RESULT))
    message(FATAL_ERROR "Result is ${CMD_RESULT}")
  endif()

  # look only server result, other may fails for unknown reason for now
  # list(GET CMD_RESULT -1 CM_SERVER_RESULT)

  # if(NOT(${CM_SERVER_RESULT} EQUAL 0))
  #   message(FATAL_ERROR "Result is ${CMD_RESULT}")
  # endif()

else()

  execute_process(
    COMMAND ${CMD_CLIENT} ${ARGS} --log-file mp_ring_client_1.log
    COMMAND ${CMD_CLIENT} ${ARGS} --log-file mp_ring_client_2.log
    COMMAND ${CMD_CLIENT} ${ARGS} --log-file mp_ring_client_3.log
    COMMAND ${CMD_CLIENT} ${ARGS} --log-file mp_ring_client_4.log
    COMMAND ${CMD_CLIENT} ${ARGS} --log-file mp_ring_client_5.log
    COMMAND ${CMD_CLIENT} ${ARGS} --log-file mp_ring_client_6.log
    COMMAND ${CMD_SERVER} ${ARGS} --log-file mp_ring_server.log
    WORKING_DIRECTORY ${WORKING_DIRECTORY}
    RESULTS_VARIABLE CMD_RESULT
    COMMAND_ERROR_IS_FATAL ANY # if any of the command fails, execute process fails
    TIMEOUT 20
  )

endif()
