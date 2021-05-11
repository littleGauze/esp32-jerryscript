
set(JERRY_DIR "./jerryscript")

idf_component_register(SRCS ${JERRY_DIR}/targets/esp-idf INCLUDE_DIRS
  "."
  ${JERRY_DIR}/jerry-core/include
  ${JERRY_DIR}/jerry-ext/include
  ${JERRY_DIR}/jerry-port/default/include
)

add_prebuilt_library(libjerry-core lib/libjerry-core.a REQUIRES newlib PRIV_REQUIRES ${COMPONET_NAME})
add_prebuilt_library(libjerry-ext lib/libjerry-ext.a PRIV_REQUIRES ${COMPONET_NAME})
add_prebuilt_library(libjerry-port-default lib/libjerry-port-default.a PRIV_REQUIRES ${COMPONET_NAME})

target_link_libraries(${COMPONENT_LIB} INTERFACE libjerry-core)
target_link_libraries(${COMPONENT_LIB} INTERFACE libjerry-ext)
target_link_libraries(${COMPONENT_LIB} INTERFACE libjerry-port-default)
