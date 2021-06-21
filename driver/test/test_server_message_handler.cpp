#include "catch.hpp"
#include "../src/server/server_message_handler.h"
#include <vector>
#include <string>
#include <memory>

std::unique_ptr<ServerMessageHandler> setup_devicelist_msg() {
    std::cout << "setup_devicelist_msg\n";

    auto s = std::make_unique<ServerMessageHandler>();
    std::cout << "created server message handler\n";

    auto devlist = std::make_unique<VRProto::DeviceList>();
    std::vector<VRProto::Device> devices;
    std::cout << "created device list\n";

    VRProto::Device* dev1 = devlist->add_devices();
    dev1->set_type(VRProto::DeviceTypeHMD);
    dev1->set_uid("hobovr_HMD");
    std::cout << "created hmd\n";

    VRProto::Device* dev2 = devlist->add_devices();
    dev2->set_type(VRProto::DeviceTypeController);
    dev2->set_uid("hobovr_left_controller");
    dev2->set_left_hand(true);
    std::cout << "created left controller\n";

    VRProto::Device* dev3 = devlist->add_devices();
    dev3->set_type(VRProto::DeviceTypeController);
    dev3->set_uid("hobovr_right_controller");
    dev3->set_left_hand(false);
    std::cout << "created right controller\n";

    VRProto::VRMessage msg = VRProto::VRMessage();
    msg.set_allocated_device_list(devlist.release());
    std::cout << "set_allocated_device_list\n";

    s->handle_message(msg);
    std::cout << "handeled message\n";

    return s;
}

std::unique_ptr <ServerMessageHandler> setup_controller_msg() {
    std::cout << "setup_controller_msg\n";

    auto s = std::make_unique < ServerMessageHandler>();
    std::cout << "created server message handler\n";

    auto con = std::make_unique<VRProto::Controller>();
    std::cout << "created controller\n";

    auto pose = std::make_unique<VRProto::Pose>();
    std::cout << "created pose\n";

    auto pos = std::make_unique<VRProto::Vec3>();
    std::cout << "created pos\n";
    pos->set_x(1);
    pos->set_y(2);
    pos->set_z(3);
    std::cout << "set pos\n";

    auto quat = std::make_unique<VRProto::Vec4>();
    std::cout << "created quat\n";
    quat->set_x(4);
    quat->set_y(5);
    quat->set_z(6);
    quat->set_w(7);
    std::cout << "set quat\n";

    pose->set_allocated_pos(pos.release());
    std::cout << "set allocated pos\n";
    pose->set_allocated_quat(quat.release());
    std::cout << "set allocated quat\n";

    auto buttons = std::make_unique<VRProto::ByteDict>();
    std::cout << "created buttons\n";

    (*buttons->mutable_entries())[0] = 0.0;
    (*buttons->mutable_entries())[1] = 0.3;
    (*buttons->mutable_entries())[2] = 0.6;
    (*buttons->mutable_entries())[3] = 0.9;
    (*buttons->mutable_entries())[4] = 1.0;
    (*buttons->mutable_entries())[5] = 1.5;
    std::cout << "set buttons\n";

    con->set_allocated_pose(pose.release());
    std::cout << "set allocated pose\n";
    con->set_allocated_buttons(buttons.release());
    std::cout << "set allocated buttons\n";

    VRProto::VRMessage msg = VRProto::VRMessage();
    msg.set_allocated_controller(con.release());
    std::cout << "set_allocated_controller\n";

    s->handle_message(msg);
    std::cout << "handeled message\n";

    return s;
}

TEST_CASE("server_message_handler", "[server_message_handler]") {
    SECTION("init") {
        auto s = ServerMessageHandler();
    }

    SECTION("handle kDeviceList") {
        auto s = setup_devicelist_msg();

        REQUIRE(s->current_device_list->devices()[0].type() == VRProto::DeviceTypeHMD);
        REQUIRE(s->current_device_list->devices()[0].uid() == "hobovr_HMD");

        REQUIRE(s->current_device_list->devices()[1].type() == VRProto::DeviceTypeController);
        REQUIRE(s->current_device_list->devices()[1].uid() == "hobovr_left_controller");
        REQUIRE(s->current_device_list->devices()[1].left_hand() == true);

        REQUIRE(s->current_device_list->devices()[2].type() == VRProto::DeviceTypeController);
        REQUIRE(s->current_device_list->devices()[2].uid() == "hobovr_right_controller");
        REQUIRE(s->current_device_list->devices()[2].left_hand() == false);

        REQUIRE(s->reply.has_ack());
        REQUIRE(s->reply.ack() == true);
    }

    SECTION("handle kController") {
        auto s = setup_controller_msg();

        REQUIRE(s->current_controller->pose().pos().x() == 1);
        REQUIRE(s->current_controller->pose().pos().y() == 2);
        REQUIRE(s->current_controller->pose().pos().z() == 3);

        REQUIRE(s->current_controller->pose().quat().x() == 4);
        REQUIRE(s->current_controller->pose().quat().y() == 5);
        REQUIRE(s->current_controller->pose().quat().z() == 6);
        REQUIRE(s->current_controller->pose().quat().w() == 7);

        REQUIRE(s->current_controller->buttons().entries().at(0) == 0.0f);
        REQUIRE(s->current_controller->buttons().entries().at(1) == 0.3f);
        REQUIRE(s->current_controller->buttons().entries().at(2) == 0.6f);
        REQUIRE(s->current_controller->buttons().entries().at(3) == 0.9f);
        REQUIRE(s->current_controller->buttons().entries().at(4) == 1.0f);
        REQUIRE(s->current_controller->buttons().entries().at(5) == 1.5f);

        REQUIRE(s->reply.has_ack());
        REQUIRE(s->reply.ack() == true);
    }

    SECTION("handle kRequest RequestTypeDeviceList") {
        auto s = setup_devicelist_msg();

        auto req = std::make_unique<VRProto::Request>();
        req->set_type(VRProto::RequestTypeDeviceList);

        auto msg = std::make_unique <VRProto::VRMessage>();
        msg->set_allocated_request(req.release());

        s->handle_message(*msg);
        std::cout << "handeled message RequestTypeDeviceList\n";

        REQUIRE(s->reply.has_device_list());
        std::cout << "has_device_list\n";

        REQUIRE(s->reply.device_list().devices()[0].type() == VRProto::DeviceTypeHMD);
        std::cout << "device 0 type\n";
        REQUIRE(s->reply.device_list().devices()[0].uid() == "hobovr_HMD");
        std::cout << "device 0 uid\n";

        REQUIRE(s->reply.device_list().devices()[1].type() == VRProto::DeviceTypeController);
        REQUIRE(s->reply.device_list().devices()[1].uid() == "hobovr_left_controller");
        REQUIRE(s->reply.device_list().devices()[1].left_hand() == true);
        std::cout << "device 1\n";


        REQUIRE(s->reply.device_list().devices()[2].type() == VRProto::DeviceTypeController);
        REQUIRE(s->reply.device_list().devices()[2].uid() == "hobovr_right_controller");
        REQUIRE(s->reply.device_list().devices()[2].left_hand() == false);
        std::cout << "device 2\n";
    }

    SECTION("handle kRequest RequestTypeController") {
        std::cout << "handele kRequest RequestTypeController\n";
        auto s = setup_controller_msg();
        std::cout << "setup_controller_msg\n";

        auto req = std::make_unique<VRProto::Request>();
        req->set_type(VRProto::RequestTypeController);

        VRProto::VRMessage msg = VRProto::VRMessage();
        msg.set_allocated_request(req.release());

        s->handle_message(msg);
        std::cout << "handeled message RequestTypeController\n";

        REQUIRE(s->reply.has_controller());
        std::cout << "has_controller\n";

        REQUIRE(s->reply.controller().pose().pos().x() == 1);
        REQUIRE(s->reply.controller().pose().pos().y() == 2);
        REQUIRE(s->reply.controller().pose().pos().z() == 3);

        REQUIRE(s->reply.controller().pose().quat().x() == 4);
        REQUIRE(s->reply.controller().pose().quat().y() == 5);
        REQUIRE(s->reply.controller().pose().quat().z() == 6);
        REQUIRE(s->reply.controller().pose().quat().w() == 7);

        REQUIRE(s->reply.controller().buttons().entries().at(0) == 0.0f);
        REQUIRE(s->reply.controller().buttons().entries().at(1) == 0.3f);
        REQUIRE(s->reply.controller().buttons().entries().at(2) == 0.6f);
        REQUIRE(s->reply.controller().buttons().entries().at(3) == 0.9f);
        REQUIRE(s->reply.controller().buttons().entries().at(4) == 1.0f);
        REQUIRE(s->reply.controller().buttons().entries().at(5) == 1.5f);
    }
}