#include <iostream>
#include <vector>
#include <thread>
#include <sstream>
#include "threadfuncs.h"

int main() {
  about();

  // Open log file
  Logger logger("output.log");

  {
	std::ostringstream oss;
	oss << "main: pid = " << getThreadID()
	    << ", opened file: 'output.log'\n";
	logger.writeLine(oss.str());
  }

  // args for threads
  std::vector<ThreadArgs> args(COUNT_THREADS);

  for (int i = 0; i < COUNT_THREADS; ++i) {
	std::ostringstream oss;
	oss << "T" << i;
	args[i].id = 1 + i;
	args[i].tag = oss.str();
  }


//futures and promises
  std::vector<std::promise<std::string>> promises(COUNT_THREADS);
  std::vector<std::future<std::string>> futures;
  futures.reserve(COUNT_THREADS);
  for (int i = 0; i < COUNT_THREADS; ++i) {
	futures.push_back(promises[i].get_future());
  }

  std::vector<std::thread> threads;
  threads.reserve(COUNT_THREADS);
  for (int i = 0; i < COUNT_THREADS; ++i) {
	threads.emplace_back(
		funcThread,
		std::cref(args[i]),
		std::ref(logger),
		std::move(promises[i])
	);
  }

  for (auto& t : threads) {
	if (t.joinable()) t.join();
  }

  for (int i = 0; i < COUNT_THREADS; ++i) {
	std::cout << "main: thread " << i
		<< " back: " << futures[i].get() << "\n";
  }

  // wait for stop all threads
  for (auto& t : threads) {
    if (t.joinable()) t.join();
  }

  {
	std::thread prod(producer, std::ref(logger), 10);
	std::thread cons(consumer, std::ref(logger));
	prod.join();
	cons.join();
  }
  
  std::cout << "final g_counter = " << g_counter
		<< " (expected " << COUNT_THREADS * 100000 << ")\n";
  // close file automatically
  logger.writeLine("main: all threads finished, file closed\n");
  return 0;
}
