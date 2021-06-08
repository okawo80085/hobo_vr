#include <vector>
#include <string>
#include <sstream>

#include "zmq.hpp"
#include "zmq_addon.hpp"

namespace VrSubscriber{
	/// <summary>
	/// Driver Receiver class.
	/// 
	/// ZMQ PubSub Nodes:
	///		Receives  <- DriverSender
	///		Published -> DriverReceiver
	/// </summary>
	class DriverReceiver {
		std::vector<std::string> m_vsDevice_list;
		std::vector<int> m_viEps;
		std::string m_sIdMessage = "hello\n";

		DriverReceiver(B& _buffer_getter, zmq::context_t* ctx, char* protocol="tcp", int port = 6969, char* addr = "127.0.01") :
			buffer_getter(_buffer_getter), 
			subscriber(*ctx, zmq::socket_type::sub),
			publisher(*ctx, zmq::socket_type::pub)
		{
			std::stringstream ss; ss << protocol << "://" << addr << ":" << port;

			// We'll be publishing to and receiving from the same location:
			subscriber.connect(ss.str());
			publisher.bind(ss.str());

			subscriber.set(zmq::sockopt::subscribe, "DriverSender");

			auto star = buffer_getter(receiveMessage.data());
		}

		~DriverReceiver() {
			this->stop();
		}

		void start() {
			m_bThreadKeepAlive = true;
			this->send("Hello");

			this->m_pMyTread = new std::thread(this->my_thread_enter, this);

			if (!this->m_pMyTread || !m_bThreadKeepAlive) {
				// log failed to create recv thread
				// printf("thread start error\n");
				this->close_me();
#ifdef DRIVERLOG_H
				DriverLog("receiver thread start error\n");
#endif
				throw std::runtime_error("failed to crate receiver thread or thread already exited");
			}
		}

		int send(const char* msg) {
			publisher.send(zmq::str_buffer("DriverReceiver"), zmq::send_flags::sndmore);
			publisher.send(zmq::str_buffer(msg));
		}

	private:
		std::thread* m_pMyTread = nullptr;
		B& buffer_getter;
		zmq::socket_t subscriber;
		zmq::socket_t publisher;
		
	};
}