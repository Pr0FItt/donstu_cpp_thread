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

  // thread are starting
  std::vector<std::thread> threads;
  threads.reserve(COUNT_THREADS);

  for (int i = 0; i < COUNT_THREADS; ++i) {
    threads.emplace_back(funcThread, std::cref(args[i]), std::ref(logger));
  }

  // wait for stop all threads
  for (auto& t : threads) {
    if (t.joinable()) t.join();
  }
  
  std::cout << "final g_counter = " << g_counter
		<< " (expected " << COUNT_THREADS * 100000 << ")\n";
  // close file automatically
  logger.writeLine("main: all threads finished, file closed\n");
  return 0;
}
