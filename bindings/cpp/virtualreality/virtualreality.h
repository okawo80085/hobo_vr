#pragma once
#ifndef __HOBO_VR_LIB
#define __HOBO_VR_LIB

#include <unordered_map>
#include <chrono>
#include <thread>
#include <vector>
#include <functional>
#include <string>

#include <stdio.h>
#include <stdarg.h>

namespace hvr {
/*
c++ bindings for hobo_vr posers

right now includes templates only, no tracking
*/

// version
static const uint32_t k_nHvrVersionMajor = 0;
static const uint32_t k_nHvrVersionMinor = 1;
static const uint32_t k_nHvrVersionBuild = 1;
static const std::string k_nHvrVersionJoke = "solid snake";

// logging

static vr::ostream s_oLogStream& = NULL;

#if !defined( WIN32)
#define vsnprintf_s vsnprintf
#endif

bool ChangeLogStreams(std::ostream& newStream) {
    s_oLogStream = newStream;
    return s_oLogStream != NULL;
}

ChangeLogStreams(std::cout); // use stdout by default

static void LogVarArgs( const char *pMsgFormat, va_list args )
{
    char buf[1024];
    vsnprintf_s( buf, sizeof(buf), pMsgFormat, args );

    if (s_oLogStream)
        s_oLogStream << buf;
}


void Log( const char *pMsgFormat, ... )
{
    va_list args;
    va_start( args, pMsgFormat );

    DriverLogVarArgs( pMsgFormat, args );

    va_end(args);
}


void DebugLog( const char *pMsgFormat, ... )
{
#ifdef _DEBUG
    va_list args;
    va_start( args, pMsgFormat );

    DriverLogVarArgs( pMsgFormat, args );

    va_end(args);
#endif
}

// constants
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

// yes, all controller inputs need to be floats cuz thats what the receiver is expecting 🙃
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

    const char type_id() {return 'p';}
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

    const char type_id() {return 'c';}
};

struct TrackerPose : Pose {
    const char type_id() {return 't';}
};

#pragma pack(pop)

struct KeepAliveTrigger {
    bool is_alive;
    std::chrono::nanoseconds sleep_delay;

    KeepAliveTrigger(bool alive, std::chrono::nanoseconds sleep_del): is_alive(alive), sleep_delay(sleep_del) {}
}

#ifdef WIN32
#include <locale>
#include <codecvt>

std::wstring s2w(std::string wide_utf16_source_string){
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::string narrow = converter.to_bytes(wide_utf16_source_string);
    std::wstring wide = converter.from_bytes(narrow_utf8_source_string);
    return wide;
}

#endif

// base template class
class PoserTemplateBase : public Callback {
private:
    std::vector<std::function<void()>> m_vThreads;
    bool m_bStarted = false;

protected:
    std::unordered_map<std::string, KeepAliveTrigger> m_mThreadRegistry;
    char m_chpLastReadBuff[4096];

    std::shared_prt<SocketObj> m_spSockComm;
    std::shared_prt<SocketObj> m_spManagerSockComm;

public:
    PoserTemplateBase(
        std::string addr="127.0.0.1",
        int port=6969,
        std::chrono::nanoseconds send_delay=1e+07ns,) {

        #ifdef WIN32
        m_spSockComm = std::make_shared<SocketObj>(s2w(addr), port);
        m_spManagerSockComm = std::make_shared<SocketObj>(s2w(addr), port);
        #else
        m_spSockComm = std::make_shared<SocketObj>(addr, port);
        m_spManagerSockComm = std::make_shared<SocketObj>(addr, port);
        #endif

        m_spSockComm->m_sIdMessage = g_sPoserIdMsg+"\n";
        m_spSockComm->setCallback(this);
        m_spManagerSockComm->m_sIdMessage = g_sManagerIdMsg+"\n";        
        // m_spManagerSockComm->setCallback(this);

        register_member_thread(std::bind(&PoserTemplateBase::close, this), "close", 1e+08ns); // register the close thread first
        register_member_thread(std::bind(&PoserTemplateBase::send, this), "send", send_delay); // provides a trigger for the send thread in derived classes
    }

    bool Start() {
        // connect to server, send id messages
        Log("connecting to server...\n");
        m_spSockComm->start();
        m_spManagerSockComm->start();
        m_bStarted = true;
        return false;
    }

    virtual void send() = 0; // lmao, override this

    void OnPacket(char* buff, int len) {
        m_chpLastReadBuff = buff; // just store the received buffer
    }

    void close() {
        while (m_mThreadRegistry["close"].is_alive) {
            // cli and close thread logic, i'll add it when i get around to adding docopt

            Log("for testing purposes this poser will exit in 10 seconds\n");
            std::this_thread::sleep_for(std::chrono::seconds(10)); // for now just wait and break;
            break;

            std::this_thread::sleep_for(m_mThreadRegistry["close"].sleep_delay); // thread sleep
        }
        m_mThreadRegistry["close"].is_alive = false; // signal that this thread is done and is about to exit
        Log("closing...\n");
        // send thread stop signals
        for (auto& i : m_mThreadRegistry) {
            if (i.second.is_alive) {
                i.second.is_alive = false;
                Log("stop command sent to '%s'...\n", i.first.c_str());
            } else {
                Log("thread using trigger '%s' already stopped\n", i.first.c_str());
            }
        }
        Log("finished\n");
    }

    void main() {
        if (!m_bStarted) {
            // log, throw
            Log("startup error: main() called before Start()\n");
            throw std::runtime_error("main() called before Start()");
        }

        // spin up the registered threads
        std::vector<std::thread> l_vRunning;
        for (auto i : m_vThreads)
            l_vRunning.push_back(std::thread(i));

        l_vRunning[0].join(); // wait for the close thread to finish

        // join the threads
        for (int i=1; i<l_vRunning.length(); i++) {
            if (l_vRunning[i].joinable()) {
                l_vRunning[i].join();
            }
        }

        try {
            m_spSockComm->stop();
        } catch(...) {
            Log("failed to close main socket connection\n");
        }

        try {
            m_spManagerSockComm->stop();
        } catch(...) {
            Log("failed to close manager socket connection\n");
        }
    }

    // register new threads
    void register_member_thread(std::function<void()> member_func, std::string trigger_name, std::chrono::nanoseconds sleep_delay) {
        if (!m_mThreadRegistry.contains(trigger_name)) {
            m_mThreadRegistry[trigger_name] = KeepAliveTrigger(true, sleep_delay);
            m_vThreads.push_back(member_func);
        } else {
            // log, throw
            Log("register error: trigger '%s' already registered\n", trigger_name.c_str());
            throw std::runtime_error("trigger already registered");
        }
    }
};

// derived posers
class UduPoserTemplate: public PoserTemplateBase {
protected:
    std::vector<Pose> m_vPoses;

public:
    UduPoserTemplate( std::string udu_string,
        std::string addr="127.0.0.1",
        int port=6969,
        std::chrono::nanoseconds send_delay=1e+07ns,):PoserTemplateBase(addr, port, send_delay) {
        //temp
        m_vPoses.push_back(Pose);
        m_vPoses.push_back(ControllerPose);
        m_vPoses.push_back(ControllerPose);
    }

    void send() {
        while (m_mThreadRegistry["send"].is_alive) {
            try {
                for (auto i : m_vPoses)
                    m_spSockComm->send2(i._to_pchar(), i.len_bytes());

                m_spSockComm->send2(g_sMessageTerminator);

                std::this_thread::sleep_for(m_mThreadRegistry["send"].sleep_delay);
            } catch (...) {
                Log("send thread: failed, exiting\n");
                break;
            }

        }
    }
}

}; // namespace vr

#endif // __HOBO_VR_LIB