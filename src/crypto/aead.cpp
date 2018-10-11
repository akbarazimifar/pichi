#include <pichi/asserts.hpp>
#include <pichi/crypto/aead.hpp>
#include <pichi/crypto/hash.hpp>
#include <sodium.h>

using namespace std;

namespace pichi::crypto {

template <CryptoMethod method>
static void initialize(AeadContext<method>& ctx, ConstBuffer<uint8_t> ikm,
                       ConstBuffer<uint8_t> salt)
{
  assertTrue(ikm.size() == KEY_SIZE<method>, PichiError::CRYPTO_ERROR);
  assertTrue(salt.size() == IV_SIZE<method>, PichiError::CRYPTO_ERROR);
  if constexpr (helpers::isGcm<method>()) {
    auto skey = vector<uint8_t>(KEY_SIZE<method>, 0);
    hkdf<HashAlgorithm::SHA1>(skey, ikm, salt);
    mbedtls_gcm_init(&ctx);
    assertTrue(mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, skey.data(), skey.size() * 8) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else if constexpr (helpers::isSodiumAead<method>()) {
    ctx.resize(KEY_SIZE<method>);
    hkdf<HashAlgorithm::SHA1>(ctx, ikm, salt);
  }
  else
    static_assert(helpers::DependentFalse<method>::value);
}

template <CryptoMethod method> static void release(AeadContext<method>& ctx)
{
  if constexpr (helpers::isGcm<method>())
    mbedtls_gcm_free(&ctx);
  else
    static_assert(helpers::isSodiumAead<method>());
}

template <CryptoMethod method>
static void encrypt(AeadContext<method>& ctx, ConstBuffer<uint8_t> nonce,
                    ConstBuffer<uint8_t> plain, MutableBuffer<uint8_t> cipher)
{
  assertTrue(nonce.size() == NONCE_SIZE<method>, PichiError::CRYPTO_ERROR);
  assertTrue(cipher.size() >= plain.size() + TAG_SIZE<method>, PichiError::CRYPTO_ERROR);
  if constexpr (helpers::isGcm<method>()) {
    assertTrue(mbedtls_gcm_crypt_and_tag(&ctx, MBEDTLS_GCM_ENCRYPT, plain.size(), nonce.data(),
                                         nonce.size(), nullptr, 0, plain.data(), cipher.data(),
                                         TAG_SIZE<method>, cipher.data() + plain.size()) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else if constexpr (method == CryptoMethod::CHACHA20_IETF_POLY1305) {
    auto clen = static_cast<unsigned long long>(plain.size() + TAG_SIZE<method>);
    assertTrue(crypto_aead_chacha20poly1305_ietf_encrypt(cipher.data(), &clen, plain.data(),
                                                         plain.size(), nullptr, 0, nullptr,
                                                         nonce.data(), ctx.data()) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else if constexpr (method == CryptoMethod::XCHACHA20_IETF_POLY1305) {
    auto clen = static_cast<unsigned long long>(plain.size() + TAG_SIZE<method>);
    assertTrue(crypto_aead_xchacha20poly1305_ietf_encrypt(cipher.data(), &clen, plain.data(),
                                                          plain.size(), nullptr, 0, nullptr,
                                                          nonce.data(), ctx.data()) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else
    static_assert(helpers::DependentFalse<method>::value);
}

template <CryptoMethod method>
static void decrypt(AeadContext<method>& ctx, ConstBuffer<uint8_t> nonce,
                    ConstBuffer<uint8_t> cipher, MutableBuffer<uint8_t> plain)
{
  assertTrue(nonce.size() == NONCE_SIZE<method>, PichiError::CRYPTO_ERROR);
  assertTrue(plain.size() + TAG_SIZE<method> >= cipher.size(), PichiError::CRYPTO_ERROR);
  if constexpr (helpers::isGcm<method>()) {
    assertTrue(mbedtls_gcm_auth_decrypt(&ctx, cipher.size() - TAG_SIZE<method>, nonce.data(),
                                        nonce.size(), nullptr, 0,
                                        cipher.data() + cipher.size() - TAG_SIZE<method>,
                                        TAG_SIZE<method>, cipher.data(), plain.data()) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else if constexpr (method == CryptoMethod::CHACHA20_IETF_POLY1305) {
    auto mlen = 0ull;
    assertTrue(crypto_aead_chacha20poly1305_ietf_decrypt(plain.data(), &mlen, nullptr,
                                                         cipher.data(), cipher.size(), nullptr, 0,
                                                         nonce.data(), ctx.data()) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else if constexpr (method == CryptoMethod::XCHACHA20_IETF_POLY1305) {
    auto mlen = 0ull;
    assertTrue(crypto_aead_xchacha20poly1305_ietf_decrypt(plain.data(), &mlen, nullptr,
                                                          cipher.data(), cipher.size(), nullptr, 0,
                                                          nonce.data(), ctx.data()) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else
    static_assert(helpers::DependentFalse<method>::value);
}

template <CryptoMethod method>
AeadEncryptor<method>::AeadEncryptor(ConstBuffer<uint8_t> key, ConstBuffer<uint8_t> salt)
  : nonce_(NONCE_SIZE<method>, 0), salt_{salt.begin(), salt.end()}
{
  if (salt_.empty()) {
    salt_.resize(IV_SIZE<method>);
    randombytes_buf(salt_.data(), salt_.size());
  }
  assertTrue(salt_.size() == IV_SIZE<method>, PichiError::CRYPTO_ERROR);
  initialize<method>(ctx_, key, salt_);
}

template <CryptoMethod method> AeadEncryptor<method>::~AeadEncryptor() { release<method>(ctx_); }

template <CryptoMethod method> ConstBuffer<uint8_t> AeadEncryptor<method>::getIv() const
{
  return salt_;
}

template <CryptoMethod method>
size_t AeadEncryptor<method>::encrypt(ConstBuffer<uint8_t> plain, MutableBuffer<uint8_t> cipher)
{
  assertTrue(plain.size() <= 0x3fff, PichiError::CRYPTO_ERROR);
  assertTrue(cipher.size() >= plain.size() + TAG_SIZE<method>, PichiError::CRYPTO_ERROR);
  pichi::crypto::encrypt<method>(ctx_, nonce_, plain, cipher);
  sodium_increment(nonce_.data(), nonce_.size());
  return plain.size() + TAG_SIZE<method>;
}

template class AeadEncryptor<CryptoMethod::AES_128_GCM>;
template class AeadEncryptor<CryptoMethod::AES_192_GCM>;
template class AeadEncryptor<CryptoMethod::AES_256_GCM>;
template class AeadEncryptor<CryptoMethod::CHACHA20_IETF_POLY1305>;
template class AeadEncryptor<CryptoMethod::XCHACHA20_IETF_POLY1305>;

template <CryptoMethod method>
AeadDecryptor<method>::AeadDecryptor(ConstBuffer<uint8_t> key)
  : okm_{key.begin(), key.end()}, nonce_(NONCE_SIZE<method>, 0)
{
}

template <CryptoMethod method> AeadDecryptor<method>::~AeadDecryptor()
{
  if (initialized_) release<method>(ctx_);
}

template <CryptoMethod method> size_t AeadDecryptor<method>::getIvSize() const
{
  return IV_SIZE<method>;
}

template <CryptoMethod method> void AeadDecryptor<method>::setIv(ConstBuffer<uint8_t> iv)
{
  assertTrue(iv.size() == IV_SIZE<method>, PichiError::CRYPTO_ERROR);
  assertFalse(okm_.empty(), PichiError::CRYPTO_ERROR);
  initialize<method>(ctx_, okm_, iv);
  initialized_ = true;
  okm_.clear();
}

template <CryptoMethod method>
size_t AeadDecryptor<method>::decrypt(ConstBuffer<uint8_t> cipher, MutableBuffer<uint8_t> plain)
{
  assertTrue(okm_.empty(), PichiError::CRYPTO_ERROR);
  assertTrue(cipher.size() > TAG_SIZE<method>, PichiError::CRYPTO_ERROR);
  assertTrue(plain.size() >= cipher.size() - TAG_SIZE<method>, PichiError::CRYPTO_ERROR);
  pichi::crypto::decrypt<method>(ctx_, nonce_, cipher, plain);
  sodium_increment(nonce_.data(), nonce_.size());
  return cipher.size() - TAG_SIZE<method>;
}

template class AeadDecryptor<CryptoMethod::AES_128_GCM>;
template class AeadDecryptor<CryptoMethod::AES_192_GCM>;
template class AeadDecryptor<CryptoMethod::AES_256_GCM>;
template class AeadDecryptor<CryptoMethod::CHACHA20_IETF_POLY1305>;
template class AeadDecryptor<CryptoMethod::XCHACHA20_IETF_POLY1305>;

} // namespace pichi::crypto
