list(APPEND BOOST_COMPONENTS context system thread)

if(WIN32 AND NOT STATIC_LINK)
  list(APPEND BOOST_COMPONENTS date_time)
endif()

if(BUILD_SERVER)
  list(APPEND BOOST_COMPONENTS filesystem program_options)
endif()

if(BUILD_TEST)
  list(APPEND BOOST_COMPONENTS unit_test_framework)
endif()

# Unify the library types of the dependencies
if(UNIX)
  if(STATIC_LINK)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
  elseif(APPLE)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .dylib)
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES .so)
  endif()
endif()

set(Boost_USE_STATIC_LIBS ${STATIC_LINK})
set(Boost_NO_SYSTEM_PATHS ${ENABLE_CONAN})

find_package(Boost 1.77.0 REQUIRED COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
find_package(MbedTLS 2.7.0 REQUIRED)
find_package(Sodium 1.0.12 REQUIRED)
find_package(MaxmindDB 1.3.0 REQUIRED)
find_package(Rapidjson 1.1.0 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
