find_package(OpenSSL REQUIRED QUIET)

find_file(OPENSSL_BASE_H openssl/base.h PATHS ${OPENSSL_INCLUDE_DIR} REQUIRED)
file(STRINGS ${OPENSSL_BASE_H} VERSION_LINE REGEX "^#define BORINGSSL_API_VERSION [0-9]+$")

if(VERSION_LINE)
  string(REGEX REPLACE "^#define BORINGSSL_API_VERSION ([0-9]+)$" "\\1" OPENSSL_VERSION ${VERSION_LINE})
  unset(VERSION_LINE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BoringSSL
  REQUIRED_VARS OPENSSL_INCLUDE_DIR OPENSSL_LIBRARIES
  OPENSSL_CRYPTO_LIBRARY OPENSSL_CRYPTO_LIBRARIES
  OPENSSL_SSL_LIBRARY OPENSSL_SSL_LIBRARIES
  VERSION_VAR OPENSSL_VERSION
)