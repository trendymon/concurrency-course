#pragma once

#include <cstdlib>
#include <string>
#include <utility>

#include <asio.hpp>

namespace tests::support {

class BlockingTcpClient {
 public:
  BlockingTcpClient(const std::string& host, uint16_t port);

  void Send(asio::const_buffer buffer);

  void Send(const std::string& data) {
    Send(asio::const_buffer{data.data(), data.size()});
  }

  size_t Receive(asio::mutable_buffer buffer);

  std::string Receive(size_t bytes);

 private:
  void ConnectTo(const std::string& host, uint16_t port);

 private:
  asio::io_context io_context_;
  asio::ip::tcp::resolver resolver_;
  asio::ip::tcp::socket socket_;
};

}  // namespace tests::support
