#include <iostream>
#include <gtest/gtest.h>
#include "../future/future.h"

using namespace ray;

template<typename... Args>
inline void Print(Args&&... args) {
  (void)std::initializer_list<int>{(std::cout << std::forward<Args>(args) << ' ', 0)...};
  std::cout << "\n";
}

TEST(future_then, basic_then)
{
  Promise<int> promise;
  auto future = promise.GetFuture();
  auto f = future.Then([](int x){
    Print(std::this_thread::get_id());
    return x+2;
  }).Then([](int y){
    Print(std::this_thread::get_id());
    return y+2;
  }).Then([](int z){
    Print(std::this_thread::get_id());
    return z+2;
  });

  promise.SetValue(2);
  EXPECT_EQ(f.Get(), 8);
}

TEST(future_then, async_then)
{
  auto future = Async([]{return 2;}).Then([](int x){return x+2;}).Then([](int x){ return x+2; });
  EXPECT_EQ(future.Get(), 6);
}

TEST(when_any, any)
{
  std::vector<std::thread> threads;
  std::vector<Promise<int> > pmv(8);
  for (auto& pm : pmv) {
    std::thread t([&pm]{
      static int val = 10;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      pm.SetValue(val++);
    });
    threads.emplace_back(std::move(t));
  }

  std::vector<Future<int> > futures;
  for (auto& pm : pmv) {
    futures.emplace_back(pm.GetFuture());
  }

  auto fany = WhenAny(std::begin(futures), std::end(futures));
  fany.Then([]( Try<std::pair<size_t, int>> result) {//not support now
    std::cerr << "Result " << result.Value().first << " = " << result.Value().second << std::endl;
    EXPECT_LT(result.Value().second, 18);
  });

  for (auto& t : threads)
    t.join();
}

TEST(future_then, then_void)
{
  Promise<int> promise;
  auto future = promise.GetFuture();
  auto f = future.Then([](int x){
    EXPECT_EQ(x, 1);
  });

  promise.SetValue(1);
  f.Get();
}

TEST(future_exception, async_ommit_exception) {
  auto future = Async([] {
    throw std::runtime_error("");
    return 1;
  });

  auto f = future
               .Then([](Try<int> t) {
                 if (t.HasException()) {
                   std::cout << "has exception\n";
                 }

                 return 42;
               })
               .Then([](int i) {
                 return i + 2;
               });

  EXPECT_EQ(f.Get(), 44);
}

TEST(future_exception, async_exception) {
  auto future = Async([] {
    throw std::runtime_error("");
    return 1;
  });

  auto f = future
               .Then([](Try<int> t) {
                 if (t.HasException()) {
                   std::cout << "has exception\n";
                 }

                 return t + 42;
               })
               .Then([](int i) { return i + 2; });

  EXPECT_THROW(f.Get(), std::exception);
}

TEST(future_exception, value_ommit_exception){
  Promise<int> promise;
  auto future = promise.GetFuture();
  auto f = future.Then([](int x){
    throw std::runtime_error("error");
    return x+2;
  }).Then([](Try<int> y){
    if(y.HasException()){
      std::cout<<"has exception\n";
    }

    return 2;
  });

  promise.SetValue(1);

  EXPECT_EQ(f.Get(), 2);
}

TEST(future_exception, value_exception){
  Promise<int> promise;
  auto future = promise.GetFuture();
  auto f = future.Then([](int x){
    throw std::runtime_error("error");
    return x+2;
  }).Then([](int y){
    return y+2;
  });

  promise.SetValue(1);

  EXPECT_THROW(f.Get(), std::exception);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
