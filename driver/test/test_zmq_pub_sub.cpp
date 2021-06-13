#include <pybind11/pybind11.h>
#include "../src/networking/zmqpubsub.h"

namespace py = pybind11;
namespace vrs = VrSubscriber;

PYBIND11_MODULE(vrsubscriber, m) {
	py::class_<vrs::ControllerReceiver>(m, "vrsubscriber")
		.def(py::init<>())
		.def("start", &vrs::ControllerReceiver::start)
		.def("stop", &vrs::ControllerReceiver::stop)
		.def("close_me", &vrs::ControllerReceiver::close_me)
		.def("send", &vrs::ControllerReceiver::send)
		.def("my_thread", &vrs::ControllerReceiver::my_thread)
		;
	py::class_<vrs::ZmqServer>(m, "vrserver")
		.def(py::init<>())
		.def("start", &vrs::ZmqServer::start)
		.def("stop", &vrs::ZmqServer::stop)
		.def("close_me", &vrs::ZmqServer::close_me)
		.def("send", &vrs::ZmqServer::send)
		.def("my_thread", &vrs::ZmqServer::my_thread)
		;
}