#include "SPSCData.h"

struct TestParams {
  float fvalue = 0.0f;
  int64_t timestamp = 0;
  int64_t counter = 0;

  bool equals(const TestParams &rhs) const {
    return fvalue == rhs.fvalue && timestamp == rhs.timestamp &&
           counter == rhs.counter;
  }

  void update(int64_t newCounter) {
    counter = newCounter;
    fvalue = std::rand() * (1.0f / RAND_MAX);
    timestamp =
        std::chrono::high_resolution_clock::now().time_since_epoch().count();
  }
};

class Test {
public:
  void processingTask() {
    double wait = 10;
    std::this_thread::sleep_for(std::chrono::milliseconds((long)(wait)));
  }

  void test_single_thread() {}

  void test_multi_thread() {
    using namespace std::chrono;

    SPSCData<TestParams> data;

    static constexpr size_t iterations = 100000;
    std::mutex m;
    std::condition_variable cv;
    bool ready = false;

    std::thread twriter([&]() {
      {
        std::unique_lock lk(m);
        cv.wait(lk, [&] { return ready; });
      }

      {
        std::scoped_lock lock(m);
        std::cout << "writer started " << std::endl;
      }

      int64_t counter = 0;
      auto startTime = steady_clock::now();
      for (size_t i = 0; i < iterations; i++) {
        WriteScope writeScope(&data);
        writeScope->update(++counter);
        std::this_thread::sleep_for(1us);
      }

      {
        std::scoped_lock lock(m);
        std::cout << "writer finished " << std::endl;
      }
    });

    std::thread treader([&]() {
      {
        std::unique_lock lk(m);
        cv.wait(lk, [&] { return ready; });
      }

      {
        std::scoped_lock lock(m);
        std::cout << "reader started " << std::endl;
      }

      TestParams prevParams;
      int numReads = 0;
      for (size_t i = 0; i < iterations; i++) {
        ReadScope readScope(&data);

        if (readScope.get()) {
          if (readScope->equals(prevParams)) {
            std::cout << "ERROR: new data equals the old one!" << std::endl;
          } else if (prevParams.counter >= readScope->counter) {
            std::cout << "ERROR: unexpected counter value "
                      << readScope->counter
                      << "! Prev value: " << prevParams.counter << std::endl;
          }

          prevParams = *readScope.get();
          numReads++;
        }

        std::this_thread::sleep_for(1us);
      }

      {
        std::scoped_lock lock(m);
        std::cout << "reader finished " << std::endl;
        std::cout << "numReads: " << numReads << std::endl;
      }
    });

    {
      std::lock_guard lk(m);
      ready = true;
      std::cout << "data ready for processing\n";
    }

    cv.notify_all();

    twriter.join();
    treader.join();
  }
};

int main() {
  auto start = std::chrono::steady_clock::now();

  Test t;
  t.test_single_thread();
  t.test_multi_thread();

  auto end = std::chrono::steady_clock::now();
  auto diff =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  std::cout << "test" << std::endl;
  std::cout << "took " << diff.count() << " us" << std::endl;

  return 0;
}