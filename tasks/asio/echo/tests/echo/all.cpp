#include <wheels/test/test_framework.hpp>

#include <echo/server.hpp>

#include <tests_support/blocking.hpp>
#include <tests_support/buffer.hpp>
#include <tests_support/generator.hpp>

#include <algorithm>
#include <chrono>
#include <list>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

using namespace tests::support;

////////////////////////////////////////////////////////////////////////////////

static const uint16_t kServerPort = 51423;

void LaunchEchoServer() {
  std::thread([]() {
    echo::ServeForever(kServerPort);
  }).detach();
}

BlockingTcpClient MakeEchoClient() {
  return {"localhost", kServerPort};
}

std::string Echo(BlockingTcpClient& client, const std::string& data) {
  client.Send(data);
  return client.Receive(data.length());
}

////////////////////////////////////////////////////////////////////////////////

TEST_SUITE(EchoServer) {
  SIMPLE_TEST(ServerLaunched) {
  }

  SIMPLE_TEST(Hello) {
    auto client = MakeEchoClient();
    ASSERT_EQ(Echo(client, "Hello"), "Hello");
  }

  SIMPLE_TEST(HelloWorld) {
    auto client = MakeEchoClient();

    client.Send("Hello");
    std::this_thread::sleep_for(1s);
    client.Send(", World");
    ASSERT_EQ(client.Receive(12), "Hello, World");

    // No extra data from server
    ASSERT_EQ(Echo(client, "!"), "!");
  }

  SIMPLE_TEST(TwoClients) {
    auto alice = MakeEchoClient();
    auto bob = MakeEchoClient();

    static const std::string& kAliceMessage = "Hi, I am Alice!";
    static const std::string& kBobMessage = "Hi, I am Bob!";

    alice.Send(kAliceMessage);
    bob.Send(kBobMessage);

    std::this_thread::sleep_for(1s);

    ASSERT_EQ(alice.Receive(kAliceMessage.length()), kAliceMessage);
    ASSERT_EQ(bob.Receive(kBobMessage.length()), kBobMessage);
  }

  SIMPLE_TEST(ConcurrentClients) {
    static const size_t kClients = 17;

    std::list<BlockingTcpClient> clients;
    for (size_t i = 0; i < kClients; ++i) {
      clients.emplace_back("localhost", kServerPort);
    }

    static const std::string kMessage = "Merry Cristmas!";

    for (auto& client : clients) {
      client.Send(kMessage);
    }

    for (auto& client : clients) {
      ASSERT_EQ(client.Receive(kMessage.length()), kMessage);
    }
  }

  class MessageBuffer {
   public:
    size_t Size() const {
      return size_;
    }

    void Append(const char* buf, size_t bytes) {
      buf_.write(buf, bytes);
      size_ += bytes;
    }

    void Append(asio::const_buffer buffer) {
      Append((const char*)buffer.data(), buffer.size());
    }

    std::string ToString() const {
      return buf_.str();
    }

   private:
    std::ostringstream buf_;
    size_t size_ = 0;
  };

  SIMPLE_TEST(DelayedReads) {
    static const size_t kBytesToSend = 16 * 1024 * 1024;
    static const size_t kChunkLimit = 1024 * 1024;

    auto client = MakeEchoClient();
    MessageBuffer sent, received;

    auto write = [&]() {
      DataGenerator generator(kBytesToSend);

      std::vector<char> write_buf(kChunkLimit);

      while (generator.HasMore()) {
        size_t bytes = generator.NextChunk(write_buf.data(), kChunkLimit);
        asio::const_buffer chunk(write_buf.data(), bytes);

        client.Send(chunk);
        sent.Append(chunk);
      }
    };

    auto read = [&]() {
      std::this_thread::sleep_for(3s);

      std::vector<char> read_buf(kChunkLimit);

      while (received.Size() < kBytesToSend) {
        size_t bytes_read = client.Receive({read_buf.data(), kChunkLimit});
        received.Append(read_buf.data(), bytes_read);
      }
    };

    std::thread writer(write);
    std::thread reader(read);

    writer.join();
    reader.join();

    ASSERT_EQ(sent.Size(), kBytesToSend);
    ASSERT_EQ(sent.ToString(), received.ToString());
  }

  class MessageGenerator {
   public:
    MessageGenerator(size_t seed) : twister_(seed) {
    }

    std::string Next() {
      message_ = message_ + std::to_string(twister_()) + message_;
      return message_;
    }

   private:
    std::mt19937 twister_;
    std::string message_;
  };

  TEST(GrowingMessages,
       wheels::test::TestOptions().TimeLimit(15s).AdaptTLToSanitizer()) {
    static const size_t kIterations = 17;

    auto run_client = [&](size_t thread_index) {
      MessageGenerator generator(thread_index);

      for (size_t i = 0; i < kIterations; ++i) {
        auto message = generator.Next();
        auto client = MakeEchoClient();
        ASSERT_EQ(Echo(client, message), message);
        std::this_thread::sleep_for(100ms);
      }
    };

    static const size_t kThreads = 7;

    std::vector<std::thread> threads;
    for (size_t i = 0; i < kThreads; ++i) {
      threads.emplace_back(run_client, i);
    }

    for (auto& thread : threads) {
      thread.join();
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, const char* argv[]) {
  LaunchEchoServer();

  wheels::test::RunTestsMain(wheels::test::ListAllTests(), argc, argv);
}
