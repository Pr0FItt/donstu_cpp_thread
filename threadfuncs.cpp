// threadfuncs.cpp
#include "threadfuncs.h"
#include <thread>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <syscall.h>
//#include <windows.h>
#include <sys/types.h>

std::atomic<int> g_counter{0};

Logger::Logger(const std::string& filename)
  : file_(filename, std::ios::out | std::ios::trunc)
{
  if (!file_.is_open()) {
    throw std::runtime_error("Cannot open log file: " + filename);
  }
}

Logger::~Logger() {
  // std::ofstream close file here automatically
}

bool Logger::writeLine(const std::string& msg) {
  std::lock_guard<std::mutex> lock(mutex_);
  file_ << msg;
  file_.flush();
  if (!file_) {
    std::cerr << "write failed: " << msg << "\n";
    return false;
  }
  return true;
}

pid_t getThreadID() {
  return static_cast<pid_t>(::syscall(SYS_gettid));
  //return GetCurrentThreadId();
}

void about() {
  std::cout << "std::thread example\n";
}

void funcThread(const ThreadArgs& args, Logger& logger, std::promise<std::string> resultPromise) {
  	int completed = 0;
	for (int i = 0; i < COUNT_ITERATIONS; ++i) {
    std::ostringstream oss;

    oss << "[tag = " << args.tag
        << "] pid = "  << ::getpid()
        << " ppid = "  << ::getppid()
        << " tid = "   << getThreadID()
	<< " std_id = " << std::this_thread::get_id()
        << " iter = "  << i
        << "\n";
   if(!logger.writeLine(oss.str())) {
	std::cerr << "Thread tag=" << args.tag
		<< ": failed to write log line, stopping loop\n";
	break;
   }
	++completed;
    // imitation of useful work
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
	for (int i = 0; i < 100000; ++i) {
		++g_counter;
	}
	std::ostringstream result;
	result << args.tag << ": completed " << completed << " iterations";
	resultPromise.set_value(result.str());
}

  namespace {
	std::mutex		 pc_mutex;
	std:: condition_variable pc_cv;
	int			 pc_value = 0;
	bool 			 pc_has_value = false;
	bool			 pc_done = false;
  }

  void producer(Logger& logger, int count) {
	for (int i = 1; i <= count; ++i) {
		{
			std::unique_lock<std::mutex> lock(pc_mutex);
			pc_cv.wait(lock, [] {return !pc_has_value; });

			pc_value = i;
			pc_has_value = true;
		}
		pc_cv.notify_all();

		std::ostringstream oss;
		oss << "producer: put value = " << i << "\n";
		logger.writeLine(oss.str());
	}

	{
		std::lock_guard<std::mutex> lock(pc_mutex);
		pc_done = true;
	}
	pc_cv.notify_all();
  }

  void consumer(Logger& logger) {
	while (true) {
		std::unique_lock<std::mutex> lock(pc_mutex);
		pc_cv.wait(lock, [] {return pc_has_value || pc_done; });

		if (!pc_has_value && pc_done) {
			return;
		}
		
		int value = pc_value;
		pc_has_value = false;
		lock.unlock();
		pc_cv.notify_all();

		std::ostringstream oss;
		oss << "consumer: got value = " << value << "\n";
		logger.writeLine(oss.str());
	}
  }
