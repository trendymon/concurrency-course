#include <tests_support/blocking.hpp>

namespace tests::support {

BlockingTcpClient::BlockingTcpClient(const std::string& host, uint16_t port)
    : resolver_(io_context_), socket_(io_context_) {
  ConnectTo(host, port);
}

void BlockingTcpClient::Send(asio::const_buffer buffer) {
  asio::write(socket_, buffer);
}

size_t BlockingTcpClient::Receive(asio::mutable_buffer buffer) {
  return asio::read(socket_, buffer);
}

std::string BlockingTcpClient::Receive(size_t bytes) {
  std::string reply;
  reply.resize(bytes);
  size_t bytes_read = Receive({reply.data(), bytes});
  reply.resize(bytes_read);
  return reply;
}

void BlockingTcpClient::ConnectTo(const std::string& host, uint16_t port) {
  auto address = resolver_.resolve(host, std::to_string(port));
  asio::connect(socket_, address);
}

}  // namespace tests::support
