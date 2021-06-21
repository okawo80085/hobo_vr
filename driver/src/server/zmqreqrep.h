#ifndef ZMQPUBSUB_H
#define ZMQPUBSUB_H

#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <thread>

#include "protobufs/controller.pb.h"
#include <string_view>
#include <algorithm>

#include "zmq.hpp"
#include "zmq_addon.hpp"
#include <condition_variable>

namespace hobovr {

	/// <summary>
	/// ZMQ Rep Server. Holds all data transmitted to it through rep and re-sends it when requested, if available.
	/// </summary>
	class ZmqServer {
	public:
		/// <summary>
		/// ZMQ Rep Server. Holds all data transmitted to it through rep and re-sends it when requested, if available.
		/// </summary>
		/// <param name="_ctx">ZMQ Context</param>
		/// <param name="protocol">ZMQ Protocol to bind to</param>
		/// <param name="port">Port to bind to</param>
		/// <param name="addr">IP Address to bind to</param>
		ZmqServer(zmq::context_t& _ctx, char* protocol = "tcp", int port = 6969, char* addr = "127.0.0.1");

		/// <summary>
		/// ZMQ Rep Server. Holds all data transmitted to it through rep and re-sends it when requested, if available.
		/// <para>This initialization method will create a new zmq context and bind the server to "tcp://127.0.0.1:6969"</para>
		/// </summary>
		ZmqServer();

		~ZmqServer();

		/// <summary>
		/// Opens sockets and starts the ZMQ server thread so it can react to requests.
		/// </summary>
		void start();

		/// <summary>
		/// Closes sockets and stops the thread. Should automatically be called upon class destruction, but calling manually allows for restarting.
		/// </summary>
		void stop();
	protected:
		virtual bool on_packet(std::string& msg) { return false; };

		std::condition_variable cv;
		std::mutex cv_m;

	private:
		void close();
		void open();
		static void internal_thread_enter(ZmqServer* ptr);
		bool send(const char* msg, zmq::send_flags flags = zmq::send_flags::none);  // should only be used for replies
		void internal_thread();

		bool thread_reset = false;
		bool thread_keep_alive = false;
		bool is_open;
		std::thread* thread_pointer = nullptr;
		std::string address;
		zmq::socket_t rep;
		zmq::context_t& ctx;
	};


	/// <summary>
	/// ZMQ Req Client. Requests data from the server at a set rate. (~8ms for controllers/headsets, ~16ms for trackers, ~100ms for status updates) If the data doesn't exist, waits until next poll.
	/// </summary>
	class ZmqClient {
	public:
		/// <summary>
		/// ZMQ Req Client. Requests data from the server at a set rate. (~8ms for controllers/headsets, ~16ms for trackers, ~100ms for status updates) If the data doesn't exist, waits until next poll.
		/// </summary>
		/// <param name="_ctx">ZMQ Context</param>
		/// <param name="protocol">ZMQ Protocol to bind to</param>
		/// <param name="port">Port to bind to</param>
		/// <param name="addr">IP Address to bind to</param>
		ZmqClient(zmq::context_t& _ctx, char* protocol = "tcp", int port = 6969, char* addr = "127.0.01");
		ZmqClient();
		~ZmqClient();

		void start();

		void stop();

		bool send(const char* msg);

	protected:
		virtual bool on_packet(std::string& packet) { return false; };

	private:
		static void internal_thread_enter(ZmqClient* ptr);
		void close();
		void internal_thread();
		void open();

		bool thread_reset = false;
		bool thread_keep_alive = false;
		bool is_open;
		std::thread* thread_pointer = nullptr;
		std::string address;
		zmq::socket_t req;
		zmq::context_t& ctx;

	};



}
#endif