#pragma once
#ifndef _HOBO_VR_LIB_
#define _HOBO_VR_LIB_

#include <unordered_map>
#include <chrono>
#include <thread>
#include <vector>
#include <functional>

namespace vr {
/*
c++ bindings for hobo_vr posers

right now includes only templates, no tracking
*/

static const std::string g_sMessageTerminator = "\t\r\n"; // has to be exactly 3 characters
static const std::string g_sPoserIdMsg   = "holla"; // has to be exactly 5 characters
static const std::string g_sManagerIdMsg = "monky"; // has to be exactly 5 characters

// device pose objects
#pragma pack(push, 1)
struct Quat
{
    float w, x, y, z;
};

struct Vec3
{
    float x, y, z;
};

// yes, all controller inputs need to be floats cuz thats what the receiver is expecting
struct Ctrl {
    float grip;
    float system;
    float menu;
    float trackpad_click;
    float trigger_value;
    float trackpad_x;
    float trackpad_y;
    float trackpad_touch;
    float trigger_click;
};

struct Pose
{
    Vec3 pos;
    Quat rot;
    Vec3 vel;
    Vec3 ang_vel;

    int len() {
        return 13;
    }
    int len_bytes() {
        return 52; // 13*sizeof(float)
    }

    const char* _to_pchar() {
        char* ret = (char*)this;
        return ret;
    }

    const char type_id() {return 'a';}
};

struct ControllerPose : Pose
{
    Ctrl inputs;

    int len() {
        return 22;
    }
    int len_bytes() {
        return 88; // 22*sizeof(float)
    }

    const char type_id() {return 'b';}
};

#pragma pack(pop)

struct KeepAliveTrigger {
    bool is_alive;
    std::chrono::nanoseconds sleep_delay;

    KeepAliveTrigger(bool alive, std::chrono::nanoseconds sleep_del): is_alive(alive), sleep_delay(sleep_del) {}
}

// base template class
class PoserTemplateBase {
private:
    std::vector<std::function<void()>> m_vThreads;
    std::thread m_tCloseThread;

protected:
    int m_iPort;
    std::string m_sAddr;
    std::unordered_map<std::string, KeepAliveTrigger> m_mThreadRegistry;
    char* m_chpLastReadBuff = "";

public:
    PoserTemplateBase(
        std::string addr="127.0.0.1",
        int port=6969,
        std::chrono::nanoseconds send_delay=1e+07ns,):, m_sAddr(addr), m_iPort(port) {

        register_member_thread(std::bind(&PoserTemplateBase::close, this), "close", 1e+08ns); // register the close thread first
        register_member_thread(std::bind(&PoserTemplateBase::send, this), "close", send_delay);
    }

    bool Start() {
        // connect to server, send id messages
        // TODO: actually code this part retard
        return false;
    }

    virtual void send() = 0; // override this haha

    void close() {
        while (m_mThreadRegistry["close"].is_alive) {
            // cli and close thread logic, i'll add it when i get around to adding docopt

            std::this_thread::sleep_for(std::chrono::seconds(10)); // for now just wait and break;
            break;

            std::this_thread::sleep_for(m_mThreadRegistry["close"].sleep_delay);
        }
        m_mThreadRegistry["close"].is_alive = false;
    }

    void main() {
        // spin up the registered threads
        std::vector<std::thread> l_vRunning;
        for (auto i : m_vThreads)
            l_vRunning.push_back(std::thread(i));

        l_vRunning[0].join(); // wait for the close thread to finish

        // close the threads
        for (int i=1; i<l_vRunning.length(); i++) {
            if (l_vRunning[i].joinable())
                l_vRunning[i].join();
        }
    }

    void register_member_thread(std::function<void()> member_func, std::string trigger_name, std::chrono::nanoseconds sleep_delay) {
        if (!m_mThreadRegistry.contains(trigger_name)) {
            m_mThreadRegistry[trigger_name] = KeepAliveTrigger(true, sleep_delay);
            m_vThreads.push_back(member_func);
        } else {
            throw std::runtime_error("trigger already registered");
        }
    }
};

}; // namespace vr

#endif // _HOBO_VR_LIB_