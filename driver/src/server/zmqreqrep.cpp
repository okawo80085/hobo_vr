#include "zmqpubsub.h"
#include<iostream>

namespace hobovr {

	/***********
	* ZMQ Server
	************/

	ZmqServer::ZmqServer(zmq::context_t& _ctx, char* protocol, int port, char* addr) :
		ctx(_ctx)
	{
		std::stringstream ss; ss << protocol << "://" << addr << ":" << port;
		address = ss.str();
	}

	ZmqServer::ZmqServer() :ctx(zmq::context_t(1)) {
		std::stringstream ss; ss << "tcp" << "://" << "127.0.0.1" << ":" << "6969";
		address = ss.str();
	}

	ZmqServer::~ZmqServer() {
		this->stop();
	}

	void ZmqServer::start() {
		thread_keep_alive = true;
		//this->send("Hello");

		this->thread_pointer = new std::thread(this->internal_thread_enter, this);

		if (!this->thread_pointer || !thread_keep_alive) {
			// log failed to create recv thread
			// printf("thread start error\n");
			this->close();
#ifdef DRIVERLOG_H
			DriverLog("receiver thread start error\n");
#endif
			throw std::runtime_error("failed to crate receiver thread or thread already exited");
		}
	}

	void ZmqServer::stop() {
		this->close();
		thread_keep_alive = false;
		if (this->thread_pointer) {
			this->thread_pointer->join();
			delete this->thread_pointer;
			this->thread_pointer = nullptr;
		}
	}
	void ZmqServer::close() {
		if (is_open) {
			//bool res = this->send("CLOSE\n");
			rep.setsockopt(ZMQ_LINGER, 0);
			rep.close();
		}
		is_open = false;
	}

	ZmqServer::open() {
		rep = zmq::socket_t(ctx, zmq::socket_type::rep);
		rep.bind(address);
		is_open = true;
	}

	bool ZmqServer::send(const char* msg, zmq::send_flags flags) {
		zmq::send_result_t r = rep.send(zmq::buffer(std::string_view(msg)), flags);
		return true && r;
	}

	void ZmqServer::internal_thread() {
		std::cout << "started c++ server thread" << std::endl;
		while (thread_keep_alive) {
			thread_reset = false;
#ifdef DRIVERLOG_H
			DriverLog("receiver thread started\n");
#endif
			std::cout << "in c++ server thread" << std::endl;
			while (thread_keep_alive && !thread_reset) {
				std::cout << "in c++ server inner thread" << std::endl;
				zmq::message_t request;
				std::cout << "polled in c++ inner thread" << std::endl;
				try {
					bool got_req = rep.recv(&request);
					std::cout << "got_req:" << got_req << std::endl;
					std::cout << "request:" << request.str() << std::endl;

					zmq::message_t reply(3);
					memcpy((void*)reply.data(), "Ack", 3);
					rep.send(reply);

					//no callback class needed since there will only be one server per steamvr

					/*if (got_req) {
						const fbs::Controller* controller = fbs::GetController(receiveMessage.data());
						//todo: add quit command to controller message
						std::cout << "got data" << std::endl;
						std::cout << controller->pose()->pos()->x() << std::endl;

						if (!thread_reset)
							m_pCallback->on_packet(controller);
					}*/

					std::this_thread::sleep_for(std::chrono::microseconds(1));
				}
				catch (...) {
#ifdef DRIVERLOG_H
					DriverLog("receiver thread error");
#endif
					break;
				}
			}

			// log end of recv thread
#ifdef DRIVERLOG_H
			DriverLog("receiver thread ended\n");
#endif
		}
		thread_keep_alive = false;
	}

	void ZmqServer::internal_thread_enter(ZmqServer* ptr) {
		ptr->internal_thread();
	}

	/***********
	* ZMQ Client
	************/

	ZmqClient::ZmqClient(zmq::context_t& _ctx, char* protocol, int port, char* addr) :
		ctx(_ctx)
	{
		std::stringstream ss; ss << protocol << "://" << addr << ":" << port;
		address = ss.str();
	}

	ZmqClient::ZmqClient() :ctx(zmq::context_t(1)) {
		std::stringstream ss; ss << "tcp" << "://" << "127.0.0.1" << ":" << "6969";
		address = ss.str();
	}

	ZmqClient::~ZmqClient() {
		this->stop();
	}

	void ZmqClient::start() {
		thread_keep_alive = true;

		this->thread_pointer = new std::thread(this->internal_thread_enter, this);

		if (!this->thread_pointer || !thread_keep_alive) {
			// log failed to create recv thread
			// printf("thread start error\n");
			this->close();
#ifdef DRIVERLOG_H
			DriverLog("receiver thread start error\n");
#endif
			throw std::runtime_error("failed to crate receiver thread or thread already exited");
		}
	}

	void ZmqClient::stop() {
		this->close();
		thread_keep_alive = false;
		m_pCallback = &m_NullCallback;
		if (this->thread_pointer) {
			this->thread_pointer->join();
			delete this->thread_pointer;
			this->thread_pointer = nullptr;
		}
	}

	void ZmqClient::close() {
		if (is_open) {
			bool res = this->send("CLOSE\n");
			publisher.setsockopt(ZMQ_LINGER, 0);
			subscriber.setsockopt(ZMQ_LINGER, 0);
			publisher.close();
			subscriber.close();
		}
		is_open = false;
	}

	void ZmqClient::open() {
		req = zmq::socket_t(ctx, zmq::socket_type::req);
		req.connect(ss.str());
		is_open = true;
	}

	bool ZmqClient::send(const char* msg) {
		zmq::send_result_t r2 = publisher.send(zmq::buffer(std::string_view(msg)));
		return r2&&true;
	}

	void ZmqClient::internal_thread() {
		std::cout << "started c++ thread" << std::endl;
		while (thread_keep_alive) {
			thread_reset = false;
#ifdef DRIVERLOG_H
			DriverLog("receiver thread started\n");
#endif
			std::cout << "in c++ thread" << std::endl;
			while (thread_keep_alive && !thread_reset) {
				std::cout << "in c++ inner thread" << std::endl;
				zmq::message_t topic;
				zmq::message_t receiveMessage;
				std::cout << "polled in c++ inner thread" << std::endl;
				try {
					std::cout << "polled data" << std::endl;
					bool got_data = req.recv(&receiveMessage);
					std::cout << "got_data: " << got_data << std::endl;
					std::cout << "receiveMessage: " << receiveMessage.str() << std::endl;


					if (got_data) {
						if (!thread_reset)
							this->on_packet(receiveMessage.to_string());
					}

					std::this_thread::sleep_for(std::chrono::microseconds(1));
				}
				catch (...) {
#ifdef DRIVERLOG_H
					DriverLog("receiver thread error");
#endif
					break;
				}
			}

			// log end of recv thread
#ifdef DRIVERLOG_H
			DriverLog("receiver thread ended\n");
#endif
		}
		thread_keep_alive = false;
	}

	void ZmqClient::internal_thread_enter(ZmqClient* ptr) {
		ptr->internal_thread();
	}

}