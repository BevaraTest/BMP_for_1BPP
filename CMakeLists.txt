cmake_minimum_required(VERSION 3.7)
project(Filters)

include(filters.cmake)

add_definitions(-fpic)

SET(FILTER_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/BMP1BPP_filter.c
)

SET(FILTER_LIB
)

SET(FILTER_INC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_filter(BMP1BPP
        "${FILTER_SRC}"
        "${FILTER_LIB}"
        []
        ""
        "${FILTER_INC}"
        ""
        "1")
