add_library(PLATFORM STATIC)
target_sources(PLATFORM PUBLIC platform.c)
target_include_directories(PLATFORM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})