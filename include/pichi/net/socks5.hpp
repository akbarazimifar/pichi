#ifndef PICHI_NET_SOCKS5_HPP
#define PICHI_NET_SOCKS5_HPP

#include <optional>
#include <pichi/common/adapter.hpp>
#include <pichi/common/literals.hpp>
#include <string>
#include <unordered_map>

namespace pichi::net {

template <typename Stream> class Socks5Ingress : public Ingress {
private:
  using Credentials = std::unordered_map<std::string, std::string>;

  void authenticate(Yield);

public:
  template <typename... Args>
  Socks5Ingress(Credentials credentials, Args &&... args)
      : stream_{std::forward<Args>(args)...}, credentials_{
                                                  std::move(credentials)} {}

  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;
  void close(Yield) override;
  bool readable() const override;
  bool writable() const override;
  Endpoint readRemote(Yield) override;
  void confirm(Yield) override;
  void disconnect(std::exception_ptr, Yield) override;

private:
  Stream stream_;
  Credentials credentials_;
};

template <typename Stream> class Socks5Egress : public Egress {
private:
  using Credential = std::pair<std::string, std::string>;

  void authenticate(Yield);

public:
  template <typename... Args>
  Socks5Egress(std::optional<Credential> credential, Args &&... args)
      : stream_{std::forward<Args>(args)...}, credential_{
                                                  std::move(credential)} {}

  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;
  void close(Yield) override;
  bool readable() const override;
  bool writable() const override;
  void connect(Endpoint const &remote, ResolveResults next, Yield) override;

private:
  Stream stream_;
  std::optional<Credential> credential_;
};

} // namespace pichi::net

#endif // PICHI_NET_SOCKS5_HPP
