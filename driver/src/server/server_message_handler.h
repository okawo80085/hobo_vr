#ifndef SERVER_MESSAGE_HANDLER_H
#define SERVER_MESSAGE_HANDLER_H

#include "flatbuffers/message_generated.h"

class ServerMessageHandler{
public:
	ServerMessageHandler() {};
	~ServerMessageHandler() {};

	void handle_message(fbs::Message& msg) {
		switch (msg.message_type()) {

		case fbs::Msg::Msg_Request:
			fbs::Request* req = msg.message_as_Request();

			switch (req.type()) {
			case fbs::RequestType::RequestType_DeviceListRequest:
				reply = current_device_list; // todo: won't work. Have to build the message back up and add the right type.
				break;
			case fbs::RequestType::RequestType_ControllerRequest:
				reply = current_controller; // todo: won't work. Have to build the message back up and add the right type.
				break;
			}

			break;
		case fbs::Msg::Msg_DeviceList:
			fbs::Msg_DeviceList* dev = msg.message_as_DeviceList();
			current_device_list = dev;
			reply = nullptr;
			break;
		case fbs::Msg::Msg_Controller:
			fbs::Msg_DeviceList* con = msg.message_as_DeviceList();
			current_controller = con;
			reply = nullptr;
		default:
			break;
		}
	}

	fbs::Message* reply;

	fbs::Msg_DeviceList* current_device_list;
	fbs::Msg_Controller* current_controller;
private:


};

#endif // !SERVER_MESSAGE_HANDLER_H
