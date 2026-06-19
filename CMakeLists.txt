cmake_minimum_required(VERSION 3.10.2)
project(uurmoo)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(Boost_USE_MULTITHREADED ON)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
cmake_policy(SET CMP0167 NEW)
find_package(Boost REQUIRED)
find_package(CURL)

set(ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/tgbot-cpp EXCLUDE_FROM_ALL)

add_executable(uurmoo src/main.cpp src/daily_scheduler.cpp src/message_queue.cpp src/utils.cpp)

target_include_directories(uurmoo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dotenv-cpp/include/laserpants/dotenv
)

if (CURL_FOUND)
    target_compile_definitions(uurmoo PRIVATE HAVE_CURL)
endif()

target_link_libraries(uurmoo PRIVATE
    TgBot
    ${CMAKE_THREAD_LIBS_INIT}
    ${OPENSSL_LIBRARIES}
)

if (TARGET Boost::system)
    target_link_libraries(uurmoo PRIVATE Boost::system)
endif()

if (CURL_FOUND)
    target_link_libraries(uurmoo PRIVATE ${CURL_LIBRARIES})
endif()
