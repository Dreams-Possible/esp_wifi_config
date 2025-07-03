#all c file in this directory
file(GLOB SRCS "*.c")

#register component
idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS "."
)

#support littlefs
littlefs_create_partition_image(storage ../littlefs FLASH_IN_PROJECT)
