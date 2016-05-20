ADD_SUBDIRECTORY(module)

ADD_EXECUTABLE(playground playground.cc)
TARGET_LINK_LIBRARIES(playground phosphor)
