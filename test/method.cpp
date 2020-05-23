#define BOOST_TEST_MODULE pichi method test

#include "utils.hpp"
#include <boost/test/unit_test.hpp>
#include <pichi/common.hpp>
#include <pichi/crypto/method.hpp>

using namespace pichi;
using namespace pichi::crypto;

BOOST_AUTO_TEST_SUITE(METHOD)

BOOST_AUTO_TEST_CASE(KEY_SIZE_Test)
{
  BOOST_CHECK_EQUAL(16_sz, KEY_SIZE<CryptoMethod::RC4_MD5>);
  BOOST_CHECK_EQUAL(16_sz, KEY_SIZE<CryptoMethod::BF_CFB>);
  BOOST_CHECK_EQUAL(16_sz, KEY_SIZE<CryptoMethod::AES_128_CTR>);
  BOOST_CHECK_EQUAL(24_sz, KEY_SIZE<CryptoMethod::AES_192_CTR>);
  BOOST_CHECK_EQUAL(32_sz, KEY_SIZE<CryptoMethod::AES_256_CTR>);
  BOOST_CHECK_EQUAL(16_sz, KEY_SIZE<CryptoMethod::AES_128_CFB>);
  BOOST_CHECK_EQUAL(24_sz, KEY_SIZE<CryptoMethod::AES_192_CFB>);
  BOOST_CHECK_EQUAL(32_sz, KEY_SIZE<CryptoMethod::AES_256_CFB>);
  BOOST_CHECK_EQUAL(16_sz, KEY_SIZE<CryptoMethod::CAMELLIA_128_CFB>);
  BOOST_CHECK_EQUAL(24_sz, KEY_SIZE<CryptoMethod::CAMELLIA_192_CFB>);
  BOOST_CHECK_EQUAL(32_sz, KEY_SIZE<CryptoMethod::CAMELLIA_256_CFB>);
  BOOST_CHECK_EQUAL(32_sz, KEY_SIZE<CryptoMethod::CHACHA20>);
  BOOST_CHECK_EQUAL(32_sz, KEY_SIZE<CryptoMethod::SALSA20>);
  BOOST_CHECK_EQUAL(32_sz, KEY_SIZE<CryptoMethod::CHACHA20_IETF>);
  BOOST_CHECK_EQUAL(16_sz, KEY_SIZE<CryptoMethod::AES_128_GCM>);
  BOOST_CHECK_EQUAL(24_sz, KEY_SIZE<CryptoMethod::AES_192_GCM>);
  BOOST_CHECK_EQUAL(32_sz, KEY_SIZE<CryptoMethod::AES_256_GCM>);
  BOOST_CHECK_EQUAL(32_sz, KEY_SIZE<CryptoMethod::CHACHA20_IETF_POLY1305>);
  BOOST_CHECK_EQUAL(32_sz, KEY_SIZE<CryptoMethod::XCHACHA20_IETF_POLY1305>);
}

BOOST_AUTO_TEST_CASE(IV_SIZE_Test)
{
  BOOST_CHECK_EQUAL(16_sz, IV_SIZE<CryptoMethod::RC4_MD5>);
  BOOST_CHECK_EQUAL(8_sz, IV_SIZE<CryptoMethod::BF_CFB>);
  BOOST_CHECK_EQUAL(16_sz, IV_SIZE<CryptoMethod::AES_128_CTR>);
  BOOST_CHECK_EQUAL(16_sz, IV_SIZE<CryptoMethod::AES_192_CTR>);
  BOOST_CHECK_EQUAL(16_sz, IV_SIZE<CryptoMethod::AES_256_CTR>);
  BOOST_CHECK_EQUAL(16_sz, IV_SIZE<CryptoMethod::AES_128_CFB>);
  BOOST_CHECK_EQUAL(16_sz, IV_SIZE<CryptoMethod::AES_192_CFB>);
  BOOST_CHECK_EQUAL(16_sz, IV_SIZE<CryptoMethod::AES_256_CFB>);
  BOOST_CHECK_EQUAL(16_sz, IV_SIZE<CryptoMethod::CAMELLIA_128_CFB>);
  BOOST_CHECK_EQUAL(16_sz, IV_SIZE<CryptoMethod::CAMELLIA_192_CFB>);
  BOOST_CHECK_EQUAL(16_sz, IV_SIZE<CryptoMethod::CAMELLIA_256_CFB>);
  BOOST_CHECK_EQUAL(8_sz, IV_SIZE<CryptoMethod::CHACHA20>);
  BOOST_CHECK_EQUAL(8_sz, IV_SIZE<CryptoMethod::SALSA20>);
  BOOST_CHECK_EQUAL(12_sz, IV_SIZE<CryptoMethod::CHACHA20_IETF>);
  BOOST_CHECK_EQUAL(16_sz, IV_SIZE<CryptoMethod::AES_128_GCM>);
  BOOST_CHECK_EQUAL(24_sz, IV_SIZE<CryptoMethod::AES_192_GCM>);
  BOOST_CHECK_EQUAL(32_sz, IV_SIZE<CryptoMethod::AES_256_GCM>);
  BOOST_CHECK_EQUAL(32_sz, IV_SIZE<CryptoMethod::CHACHA20_IETF_POLY1305>);
  BOOST_CHECK_EQUAL(32_sz, IV_SIZE<CryptoMethod::XCHACHA20_IETF_POLY1305>);
}

BOOST_AUTO_TEST_CASE(NONCE_SIZE_Test)
{
  BOOST_CHECK_EQUAL(12_sz, NONCE_SIZE<CryptoMethod::AES_128_GCM>);
  BOOST_CHECK_EQUAL(12_sz, NONCE_SIZE<CryptoMethod::AES_192_GCM>);
  BOOST_CHECK_EQUAL(12_sz, NONCE_SIZE<CryptoMethod::AES_256_GCM>);
  BOOST_CHECK_EQUAL(12_sz, NONCE_SIZE<CryptoMethod::CHACHA20_IETF_POLY1305>);
  BOOST_CHECK_EQUAL(24_sz, NONCE_SIZE<CryptoMethod::XCHACHA20_IETF_POLY1305>);
}

BOOST_AUTO_TEST_CASE(TAG_SIZE_Test)
{
  BOOST_CHECK_EQUAL(16_sz, TAG_SIZE<CryptoMethod::AES_128_GCM>);
  BOOST_CHECK_EQUAL(16_sz, TAG_SIZE<CryptoMethod::AES_192_GCM>);
  BOOST_CHECK_EQUAL(16_sz, TAG_SIZE<CryptoMethod::AES_256_GCM>);
  BOOST_CHECK_EQUAL(16_sz, TAG_SIZE<CryptoMethod::CHACHA20_IETF_POLY1305>);
  BOOST_CHECK_EQUAL(16_sz, TAG_SIZE<CryptoMethod::XCHACHA20_IETF_POLY1305>);
}

BOOST_AUTO_TEST_SUITE_END()
