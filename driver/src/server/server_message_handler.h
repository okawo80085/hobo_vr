#ifndef SERVER_MESSAGE_HANDLER_H
#define SERVER_MESSAGE_HANDLER_H

#include "protobufs/message.pb.h"

class ServerMessageHandler {
public:
	ServerMessageHandler() {};
	~ServerMessageHandler() {};

	void handle_message(VRProto::VRMessage& msg) {
		switch (msg.VRUnion_case()) {

		case VRProto::VRMessage::kRequest:
		{
			std::cout << "VRProto::VRMessage::kRequest\n";
			VRProto::Request req = msg.request();
			std::cout << "created request\n";

			switch (req.type()) {
			case VRProto::RequestTypeDeviceList:
			{
				std::cout << "VRProto::RequestTypeDeviceList\n";
				reply = VRProto::VRMessage();
				std::cout << "created reply\n";

				VRProto::DeviceList current_device_list_copy = *(current_device_list.get());
				reply.set_allocated_device_list(current_device_list.release());
				current_device_list = std::make_unique<VRProto::DeviceList>(current_device_list_copy);

				std::cout << "set_allocated_device_list\n";
				break;
			}
			case VRProto::RequestTypeController:
			{
				std::cout << "VRProto::RequestTypeController\n";
				reply = VRProto::VRMessage();
				std::cout << "created reply\n";

				VRProto::Controller current_controller_copy = *(current_controller.get());
				reply.set_allocated_controller(current_controller.release());
				current_controller = std::make_unique<VRProto::Controller>(current_controller_copy);

				std::cout << "set_allocated_controller\n";
				break;
			}
			}

			break;
		}
		case VRProto::VRMessage::kDeviceList:
		{
			std::cout << "VRProto::VRMessage::kDeviceList\n";
			const VRProto::DeviceList& dev = msg.device_list();
			std::cout << "created device list\n";
			current_device_list = std::make_unique<VRProto::DeviceList>(dev);
			std::cout << "copied device list\n";
			reply = VRProto::VRMessage();
			std::cout << "created reply\n";
			reply.set_ack(true);
			std::cout << "set ack\n";
			break;
		}
		case VRProto::VRMessage::kController:
		{
			std::cout << "VRProto::VRMessage::kController\n";
			const VRProto::Controller& con = msg.controller();
			std::cout << "created controller\n";
			current_controller = std::make_unique<VRProto::Controller>(con);
			std::cout << "copied controller\n";
			reply = VRProto::VRMessage();
			std::cout << "created reply\n";
			reply.set_ack(true);
			std::cout << "set ack\n";
			break;
		}
		default:
			break;
		}
	}

	//const pointers are used to say these will not change and to not copy data, speeding up stuff a lot
	VRProto::VRMessage reply;

	std::unique_ptr<VRProto::DeviceList> current_device_list;
	std::unique_ptr <VRProto::Controller> current_controller;
private:


};

#endif // !SERVER_MESSAGE_HANDLER_H
