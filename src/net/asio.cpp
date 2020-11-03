#include <pichi/common/config.hpp>
// Include config.hpp first
#include <array>
#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/spawn2.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/write.hpp>
#include <pichi/api/balancer.hpp>
#include <pichi/api/ingress_holder.hpp>
#include <pichi/common/adapter.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/crypto/key.hpp>
#include <pichi/net/asio.hpp>
#include <pichi/net/direct.hpp>
#include <pichi/net/http.hpp>
#include <pichi/net/reject.hpp>
#include <pichi/net/socks5.hpp>
#include <pichi/net/ssaead.hpp>
#include <pichi/net/ssstream.hpp>
#include <pichi/net/stream.hpp>
#include <pichi/net/trojan.hpp>
#include <pichi/net/tunnel.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/ingress.hpp>

#ifdef DEPRECATED_RFC2818_CLASS
#include <boost/asio/ssl/host_name_verification.hpp>
#else  // DEPRECATED_RFC2818_CLASS
#include <boost/asio/ssl/rfc2818_verification.hpp>
#endif  // DEPRECATED_RFC2818_CLASS

using namespace std;
namespace asio = boost::asio;
namespace ip = asio::ip;
namespace sys = boost::system;
using ip::tcp;
using TcpSocket = tcp::socket;

namespace pichi::net {

namespace ssl = asio::ssl;

using TLSStream = pichi::net::TlsStream<TcpSocket>;

static auto createTlsContext(vo::TlsIngressOption const& option)
{
  auto ctx = ssl::context{ssl::context::tls_server};
  ctx.use_certificate_chain_file(option.certFile_);
  ctx.use_private_key_file(option.keyFile_, ssl::context::pem);
  return ctx;
}

static auto createTlsContext(vo::TlsEgressOption const& option, string const& serverName)
{
  // TODO SNI not supported yet
  auto ctx = ssl::context{ssl::context::tls_client};
  if (option.insecure_) {
    ctx.set_verify_mode(ssl::context::verify_none);
    return ctx;
  }

  ctx.set_verify_mode(ssl::context::verify_peer);
  if (option.caFile_.has_value())
    ctx.load_verify_file(*option.caFile_);
  else {
    ctx.set_default_verify_paths();
#ifdef DEPRECATED_RFC2818_CLASS
    ctx.set_verify_callback(ssl::host_name_verification{option.serverName_.value_or(serverName)});
#else   // DEPRECATED_RFC2818_CLASS
    ctx.set_verify_callback(ssl::rfc2818_verification{option.serverName_.value_or(serverName)});
#endif  // DEPRECATED_RFC2818_CLASS
  }
  return ctx;
}

static optional<function<bool(string const&, string const&)>>
    genAuthenticator(optional<vo::Ingress::Credential> const& credential)
{
  if (credential.has_value())
    return [&all = get<vo::UpIngressCredential>(*credential).credential_](auto&& u, auto&& p) {
      auto it = all.find(u);
      return it != cend(all) && it->second == p;
    };
  return {};
}

template <typename Socket>
unique_ptr<Ingress> makeShadowsocksIngress(Socket&& s, vo::ShadowsocksOption const& option)
{
  auto container = array<uint8_t, 1024>{0};
  auto psk = MutableBuffer<uint8_t>{container};
  psk = {container,
         crypto::generateKey(option.method_, ConstBuffer<uint8_t>{option.password_}, container)};
  switch (option.method_) {
  case CryptoMethod::RC4_MD5:
    return make_unique<SSStreamAdapter<CryptoMethod::RC4_MD5, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::BF_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::BF_CFB, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::AES_128_CTR:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_128_CTR, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::AES_192_CTR:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_192_CTR, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::AES_256_CTR:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_256_CTR, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::AES_128_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_128_CFB, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::AES_192_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_192_CFB, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::AES_256_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_256_CFB, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::CAMELLIA_128_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::CAMELLIA_128_CFB, Socket>>(psk,
                                                                                forward<Socket>(s));
  case CryptoMethod::CAMELLIA_192_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::CAMELLIA_192_CFB, Socket>>(psk,
                                                                                forward<Socket>(s));
  case CryptoMethod::CAMELLIA_256_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::CAMELLIA_256_CFB, Socket>>(psk,
                                                                                forward<Socket>(s));
  case CryptoMethod::CHACHA20:
    return make_unique<SSStreamAdapter<CryptoMethod::CHACHA20, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::SALSA20:
    return make_unique<SSStreamAdapter<CryptoMethod::SALSA20, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::CHACHA20_IETF:
    return make_unique<SSStreamAdapter<CryptoMethod::CHACHA20_IETF, Socket>>(psk,
                                                                             forward<Socket>(s));
  case CryptoMethod::AES_128_GCM:
    return make_unique<SSAeadAdapter<CryptoMethod::AES_128_GCM, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::AES_192_GCM:
    return make_unique<SSAeadAdapter<CryptoMethod::AES_192_GCM, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::AES_256_GCM:
    return make_unique<SSAeadAdapter<CryptoMethod::AES_256_GCM, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::CHACHA20_IETF_POLY1305:
    return make_unique<SSAeadAdapter<CryptoMethod::CHACHA20_IETF_POLY1305, Socket>>(
        psk, forward<Socket>(s));
  case CryptoMethod::XCHACHA20_IETF_POLY1305:
    return make_unique<SSAeadAdapter<CryptoMethod::XCHACHA20_IETF_POLY1305, Socket>>(
        psk, forward<Socket>(s));
  default:
    fail(PichiError::BAD_PROTO);
  }
}

static unique_ptr<Egress> makeShadowsocksEgress(vo::ShadowsocksOption const& option,
                                                asio::io_context& io)
{
  auto container = array<uint8_t, 1024>{};
  auto len = crypto::generateKey(option.method_, ConstBuffer<uint8_t>{option.password_}, container);
  auto psk = MutableBuffer<uint8_t>{container, len};

  switch (option.method_) {
  case CryptoMethod::RC4_MD5:
    return make_unique<SSStreamAdapter<CryptoMethod::RC4_MD5, TcpSocket>>(psk, io);
  case CryptoMethod::BF_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::BF_CFB, TcpSocket>>(psk, io);
  case CryptoMethod::AES_128_CTR:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_128_CTR, TcpSocket>>(psk, io);
  case CryptoMethod::AES_192_CTR:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_192_CTR, TcpSocket>>(psk, io);
  case CryptoMethod::AES_256_CTR:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_256_CTR, TcpSocket>>(psk, io);
  case CryptoMethod::AES_128_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_128_CFB, TcpSocket>>(psk, io);
  case CryptoMethod::AES_192_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_192_CFB, TcpSocket>>(psk, io);
  case CryptoMethod::AES_256_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_256_CFB, TcpSocket>>(psk, io);
  case CryptoMethod::CAMELLIA_128_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::CAMELLIA_128_CFB, TcpSocket>>(psk, io);
  case CryptoMethod::CAMELLIA_192_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::CAMELLIA_192_CFB, TcpSocket>>(psk, io);
  case CryptoMethod::CAMELLIA_256_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::CAMELLIA_256_CFB, TcpSocket>>(psk, io);
  case CryptoMethod::CHACHA20:
    return make_unique<SSStreamAdapter<CryptoMethod::CHACHA20, TcpSocket>>(psk, io);
  case CryptoMethod::SALSA20:
    return make_unique<SSStreamAdapter<CryptoMethod::SALSA20, TcpSocket>>(psk, io);
  case CryptoMethod::CHACHA20_IETF:
    return make_unique<SSStreamAdapter<CryptoMethod::CHACHA20_IETF, TcpSocket>>(psk, io);
  case CryptoMethod::AES_128_GCM:
    return make_unique<SSAeadAdapter<CryptoMethod::AES_128_GCM, TcpSocket>>(psk, io);
  case CryptoMethod::AES_192_GCM:
    return make_unique<SSAeadAdapter<CryptoMethod::AES_192_GCM, TcpSocket>>(psk, io);
  case CryptoMethod::AES_256_GCM:
    return make_unique<SSAeadAdapter<CryptoMethod::AES_256_GCM, TcpSocket>>(psk, io);
  case CryptoMethod::CHACHA20_IETF_POLY1305:
    return make_unique<SSAeadAdapter<CryptoMethod::CHACHA20_IETF_POLY1305, TcpSocket>>(psk, io);
  case CryptoMethod::XCHACHA20_IETF_POLY1305:
    return make_unique<SSAeadAdapter<CryptoMethod::XCHACHA20_IETF_POLY1305, TcpSocket>>(psk, io);
  default:
    fail(PichiError::BAD_PROTO);
  }
}

template <template <typename> typename Ingress, typename Socket>
unique_ptr<pichi::Ingress> makeHttpOrSocks5Ingress(vo::Ingress const& vo, Socket&& s)
{
  if (vo.tls_.has_value())
    return make_unique<Ingress<TLSStream>>(genAuthenticator(vo.credential_),
                                           createTlsContext(*vo.tls_), forward<Socket>(s));
  else
    return make_unique<Ingress<TcpSocket>>(genAuthenticator(vo.credential_), forward<Socket>(s));
}

template <template <typename> typename Egress>
unique_ptr<pichi::Egress> makeHttpOrSocks5Egress(vo::Egress const& vo, asio::io_context& io)
{
  using NoCredential = optional<pair<string, string>>;
  auto cred = vo.credential_.has_value() ? get<vo::UpEgressCredential>(*vo.credential_).credential_
                                         : NoCredential{};
  if (vo.tls_.has_value())
    return make_unique<Egress<TLSStream>>(move(cred), createTlsContext(*vo.tls_, vo.server_->host_),
                                          io);
  else
    return make_unique<Egress<TcpSocket>>(move(cred), io);
}

static auto makeRejectEgress(vo::RejectOption const& option, asio::io_context& io)
{
  switch (option.mode_) {
  case DelayMode::RANDOM:
    return make_unique<RejectEgress>(io);
  case DelayMode::FIXED:
    return make_unique<RejectEgress>(io, *option.delay_);
  default:
    fail(PichiError::BAD_PROTO);
  }
}

template <typename Stream, typename Yield> void connect(ResolveResults next, Stream& s, Yield yield)
{
#ifdef BUILD_TEST
  if constexpr (is_same_v<Stream, TestStream>) {
    suppressC4100(next);
    suppressC4100(yield);
    s.connect();
    return;
  }
  else {
#endif  // BUILD_TEST
    asyncConnect(s, next, yield);
#ifdef BUILD_TEST
  }
#endif  // BUILD_TEST
  if constexpr (IsTlsStreamV<Stream>) {
    s.async_handshake(ssl::stream_base::client, yield);
  }
}

using crypto::generateKey;

template <typename Stream, typename Yield>
void read(Stream& s, MutableBuffer<uint8_t> buf, Yield yield)
{
#ifdef BUILD_TEST
  if constexpr (is_same_v<Stream, TestStream>)
    asio::read(s, asio::buffer(buf));
  else
#endif  // BUILD_TEST
    asio::async_read(s, asio::buffer(buf), yield);
}

template <typename Stream, typename Yield>
size_t readSome(Stream& s, MutableBuffer<uint8_t> buf, Yield yield)
{
#ifdef BUILD_TEST
  if constexpr (is_same_v<Stream, TestStream>) {
    return s.read_some(asio::buffer(buf));
  }
  else
#endif  // BUILD_TEST
    return s.async_read_some(asio::buffer(buf), yield);
}

template <typename Stream, typename Yield>
void write(Stream& s, ConstBuffer<uint8_t> buf, Yield yield)
{
#ifdef BUILD_TEST
  if constexpr (is_same_v<Stream, TestStream>)
    asio::write(s, asio::buffer(buf));
  else
#endif  // BUILD_TEST
    asio::async_write(s, asio::buffer(buf), yield);
}

template <typename Stream, typename Yield> void close(Stream& s, Yield yield)
{
  // This function is supposed to be 'noexcept' because it's always invoked in
  // the desturctors.
  // TODO log it
  auto ec = sys::error_code{};
  if constexpr (IsTlsStreamV<Stream>) {
    s.async_shutdown(yield[ec]);
    s.close(ec);
  }
  else {
    suppressC4100(yield);
    s.close(ec);
  }
}

template <typename Socket> unique_ptr<Ingress> makeIngress(api::IngressHolder& holder, Socket&& s)
{
  auto& vo = holder.vo_;
  switch (vo.type_) {
  case AdapterType::TROJAN:
    return make_unique<TrojanIngress<TLSStream>>(
        get<vo::TrojanOption>(*vo.opt_).remote_,
        cbegin(get<vo::TrojanIngressCredential>(*vo.credential_).credential_),
        cend(get<vo::TrojanIngressCredential>(*vo.credential_).credential_),
        createTlsContext(*vo.tls_), forward<Socket>(s));
  case AdapterType::HTTP:
    return makeHttpOrSocks5Ingress<HttpIngress>(vo, forward<Socket>(s));
  case AdapterType::SOCKS5:
    return makeHttpOrSocks5Ingress<Socks5Ingress>(vo, forward<Socket>(s));
  case AdapterType::SS:
    return makeShadowsocksIngress(forward<Socket>(s), get<vo::ShadowsocksOption>(*vo.opt_));
  case AdapterType::TUNNEL:
    return make_unique<TunnelIngress<api::IngressIterator, Socket>>(*holder.balancer_,
                                                                    forward<Socket>(s));
  default:
    fail(PichiError::BAD_PROTO);
  }
}

unique_ptr<Egress> makeEgress(vo::Egress const& vo, asio::io_context& io)
{
  switch (vo.type_) {
  case AdapterType::TROJAN:
    return make_unique<TrojanEgress<TLSStream>>(
        get<vo::TrojanEgressCredential>(*vo.credential_).credential_,
        createTlsContext(*vo.tls_, vo.server_->host_), io);
  case AdapterType::HTTP:
    return makeHttpOrSocks5Egress<HttpEgress>(vo, io);
  case AdapterType::SOCKS5:
    return makeHttpOrSocks5Egress<Socks5Egress>(vo, io);
  case AdapterType::DIRECT:
    return make_unique<DirectAdapter>(io);
  case AdapterType::REJECT:
    return makeRejectEgress(get<vo::RejectOption>(*vo.opt_), io);
  case AdapterType::SS:
    return makeShadowsocksEgress(get<vo::ShadowsocksOption>(*vo.opt_), io);
  default:
    fail(PichiError::BAD_PROTO);
  }
}

using Yield = asio::yield_context;

template void connect<>(ResolveResults, TcpSocket&, Yield);
template void read<>(TcpSocket&, MutableBuffer<uint8_t>, Yield);
template size_t readSome<>(TcpSocket&, MutableBuffer<uint8_t>, Yield);
template void write<>(TcpSocket&, ConstBuffer<uint8_t>, Yield);
template void close<>(TcpSocket&, Yield);

template void connect<>(ResolveResults, TLSStream&, Yield);
template void read<>(TLSStream&, MutableBuffer<uint8_t>, Yield);
template size_t readSome<>(TLSStream&, MutableBuffer<uint8_t>, Yield);
template void write<>(TLSStream&, ConstBuffer<uint8_t>, Yield);
template void close<>(TLSStream&, Yield);

#ifdef BUILD_TEST
template void connect<>(ResolveResults, TestStream&, Yield);
template void read<>(TestStream&, MutableBuffer<uint8_t>, Yield);
template size_t readSome<>(TestStream&, MutableBuffer<uint8_t>, Yield);
template void write<>(TestStream&, ConstBuffer<uint8_t>, Yield);
template void close<>(TestStream&, Yield);
#endif  // BUILD_TEST

template unique_ptr<Ingress> makeIngress<>(api::IngressHolder&, TcpSocket&&);

}  // namespace pichi::net
