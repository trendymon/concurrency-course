#include <echo/server.hpp>

#include <asio.hpp>

#include <memory>
#include <utility>

using asio::ip::tcp;

namespace echo {

////////////////////////////////////////////////////////////////////////////////

class Session : public std::enable_shared_from_this<Session> {
 private:
  static const size_t kBufferSize = 1024;

 public:
  explicit Session(tcp::socket socket) : socket_(std::move(socket)) {
  }

  void Start() {
    ReadChunk();
  }

 private:
  void ReadChunk() {
    auto self = shared_from_this();
    socket_.async_read_some(
        asio::buffer(buffer_, kBufferSize),
        [this, self](std::error_code error_code, std::size_t bytes_read) {
          if (!error_code) {
            WriteChunk(bytes_read);
          }
        });
  }

  void WriteChunk(size_t bytes) {
    auto self = shared_from_this();
    asio::async_write(
        socket_, asio::buffer(buffer_, bytes),
        [self](std::error_code /*error_code*/, std::size_t /*bytes*/) {
          // Write completed!
        });
  }

 private:
  tcp::socket socket_;
  char buffer_[kBufferSize];
};

class Server {
 public:
  Server(asio::io_context& io_context, uint16_t port)
      : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
  }

  void Start() {
    AcceptClient();
  }

 private:
  void AcceptClient() {
    acceptor_.async_accept([](std::error_code error_code, tcp::socket client) {
      if (!error_code) {
        std::make_shared<Session>(std::move(client))->Start();
      }
    });
  }

 private:
  tcp::acceptor acceptor_;
};

////////////////////////////////////////////////////////////////////////////////

void ServeForever(uint16_t port) {
  asio::io_context event_loop;

  Server server(event_loop, port);
  server.Start();

  // Invoke handlers
  event_loop.run();
}

}  // namespace echo
