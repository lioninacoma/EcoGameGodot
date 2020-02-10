#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <iostream>

#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

#include "constants.h"

using namespace std;

namespace godot {

	class ThreadPool {
	private:
		boost::asio::io_service ioService;
		boost::asio::io_service::work work;
		boost::thread_group threadpool;
		
		static int activeTasks;
	public:
		static ThreadPool* get() {
			static ThreadPool* pool = new ThreadPool();
			return pool;
		};
		static ThreadPool* getNav() {
			static ThreadPool* poolNav = new ThreadPool(NAV_POOL_SIZE);
			return poolNav;
		};
		ThreadPool() : ThreadPool(POOL_SIZE) {};
		explicit ThreadPool(size_t size) : work(ioService) {
			for (size_t i = 0; i < size; ++i) {
				threadpool.create_thread(
					boost::bind(&boost::asio::io_service::run, &ioService));
			}
		};
		~ThreadPool() {
			ioService.stop();
			threadpool.join_all();
		};
		template <typename LegacyCompletionHandler>
		void submitTask(BOOST_ASIO_MOVE_ARG(LegacyCompletionHandler) handler) {
			ioService.post(handler);
		};
	};
}

#endif