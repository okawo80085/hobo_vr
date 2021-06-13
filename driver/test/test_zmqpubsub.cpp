#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"
#include "../src/networking/zmqpubsub.h"
#include <chrono>
#include <thread>
namespace vrs = VrSubscriber;

unsigned int Factorial(unsigned int number) {
    return number <= 1 ? number : Factorial(number - 1) * number;
}

TEST_CASE("pubsub", "[pubsub]") {
    SECTION("init1") {
        auto v = vrs::ControllerReceiver();
    }

    SECTION("init2") {
        auto c = zmq::context_t(1);
        auto v = vrs::ControllerReceiver(c, "tcp", 6969, "127.0.0.1");
    }

    SECTION("start") {
        auto v = vrs::ControllerReceiver();
        v.start();
        v.stop();
    }

    SECTION("stop") {
        auto v = vrs::ControllerReceiver();
        v.stop();
    }

    SECTION("close_me") {
        auto v = vrs::ControllerReceiver();
    }

    SECTION("send") {
        auto v = vrs::ControllerReceiver();
        v.send("hi");
    }

    //catch isn't thread safe, or zmq safe really, so we'll ignore this definitely handled exception.
    // This is testing closing the port and timing out on a blocking receive, which throws an exception as it should according to zmq, which is caught in the my_thread try catch block. 
    // However, Catch2 tried to figure out what that exception is and tries to create a new pointer at an illegal location, repetedly, forever.
    /*SECTION("thread") {
        auto v = vrs::ControllerReceiver();
        v.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        REQUIRE_THROWS(v.stop());
    }*/
}