#pragma once
#ifndef __HOBO_VR_LIB
#define __HOBO_VR_LIB

#include <unordered_map>
#include <chrono>
#include <thread>
#include <vector>
#include <functional>
#include <string>
#include <iostream>
#include <cmath>

#include <stdio.h>
#include <stdarg.h>

namespace hvr {

// hobovr_log.h

static std::ostream* s_oLogStream = &std::cout;

#if !defined( WIN32)
#define vsnprintf_s vsnprintf
#endif

bool ChangeLogStreams(std::ostream* const newStream) {
    s_oLogStream = newStream;
    return s_oLogStream != nullptr;
}

static void LogVarArgs( const char *pMsgFormat, va_list args )
{
    char buf[1024];
    vsnprintf_s( buf, sizeof(buf), pMsgFormat, args );

    if (s_oLogStream)
        *s_oLogStream << buf;
}


void Log( const char *pMsgFormat, ... )
{
    va_list args;
    va_start( args, pMsgFormat );

    LogVarArgs( pMsgFormat, args );

    va_end(args);
}


void DebugLog( const char *pMsgFormat, ... )
{
#ifdef _DEBUG
    va_list args;
    va_start( args, pMsgFormat );

    LogVarArgs( pMsgFormat, args );

    va_end(args);
#endif
}

};

#ifdef _WIN32
#include "virtualreality_receiver_win.h"
#endif // yes it's only for windows for now

namespace hvr 
{
/*
c++ bindings for hobo_vr posers

right now includes templates only, no tracking
*/

// version
static const uint32_t k_nHvrVersionMajor = 0;
static const uint32_t k_nHvrVersionMinor = 1;
static const uint32_t k_nHvrVersionBuild = 1;
static const std::string k_nHvrVersionJoke = "solid snake";

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
public:
    Vec3 loc = {0, 0, 0};
    Quat rot = {1, 0, 0, 0};
    Vec3 vel = {0, 0, 0};
    Vec3 ang_vel = {0, 0, 0};

    virtual int len() {
        return 13;
    }
    virtual int len_bytes() {
        return 52; // 13*sizeof(float)
    }

    virtual int __get_garbage_size() {
        return sizeof(Pose) - len_bytes();
    }

    virtual const char* _to_pchar() {
        char* ret = (char*)this;
        return ret + __get_garbage_size();
    }

    virtual const char type_id() { return 'p'; }

    // cuz fuck you, only use these methods on controllers
    virtual Ctrl& updateInputs() { Ctrl res({ 0 });  return res; }
    virtual void updateInputs(Ctrl ni) {}
};

struct ControllerPose: Pose
{
    Ctrl inputs;

    Ctrl& updateInputs() {
        return inputs;
    }

    void updateInputs(Ctrl ni) {
        inputs = ni;
    }

    int len() {
        return 22;
    }
    int len_bytes() {
        return 88; // 22*sizeof(float)
    }

    int __get_garbage_size() {
        return sizeof(ControllerPose) - len_bytes();
    }

    const char type_id() { return 'c'; }
};

struct TrackerPose: Pose {
    const char type_id() { return 't'; }
};

// the only allowed manager communication type 
struct ManagerPacket {
    uint32_t msg_type;
    uint32_t data[129];

    int len_bytes() {
        return 520; // 130*sizeof(int)
    }

    const char* _to_pchar() {
        char* ret = (char*)this;
        return ret;
    }
};

#pragma pack(pop)

struct KeepAliveTrigger {
    bool is_alive;
    std::chrono::nanoseconds sleep_delay;

    KeepAliveTrigger(bool alive=true, std::chrono::nanoseconds sleep_del= std::chrono::nanoseconds(100000000)) : is_alive(alive), sleep_delay(sleep_del) {}
};

// get command line arguments from :istream: using the rules provided in :cli_setts:
template <class CharT, class Traits = std::char_traits<CharT>>
std::vector<std::pair<std::string, std::string>> get_cli_args(std::basic_istream<CharT, Traits>& istream, const std::string cli_setts) {
    Log("> ");
    char buff[2048];
    istream.getline(buff, 2048);
    std::string text(buff);
    std::regex rgx("-{1,2}[a-z]+");
    std::regex rgx2("-{1,2}[a-z]+[ ]*");
    auto keys = utilz::get_rgx_vector(text, rgx);
    auto vals = utilz::split_by_rgx(text, rgx2);
    std::vector<std::pair<std::string, std::string>> out;
    for (int i=0; i< (keys.size() == 1 ? 1 : min(keys.size(), vals.size()-1)); i++){
        std::pair<std::string, std::string> temp;
        if (vals.size() > 1)
            temp = {keys[i], vals[i+1]};
        else
            temp = {keys[i], ""};

        out.push_back(temp);
    }
    #ifdef _DEBUG
    for (auto i : out)
        DebugLog("('%s', '%s')\n", i.first.c_str(), i.second.c_str());
    DebugLog("%d, %d\n", keys.size(), vals.size());
    #endif

    std::regex rgx3("Options:[ ]*\n");
    auto temp = utilz::split_by_rgx(cli_setts, rgx3)[1];
    for (auto i : out) {
        if (utilz::first_rgx_match(temp, std::regex(i.first)) == "")
            throw std::runtime_error("unknown key");
    }

    return out;
}

// base template class
class PoserTemplateBase : public utilz::Callback {
private:
    std::vector<std::function<void()>> m_vThreads;
    bool m_bStarted = false;

protected:
    std::unordered_map<std::string, KeepAliveTrigger> m_mThreadRegistry;
    char m_chpLastReadBuff[4096];

    std::shared_ptr<utilz::SocketObj> m_spSockComm;
    std::shared_ptr<utilz::SocketObj> m_spManagerSockComm;

    const std::string CLI_STRING = 
R"(hobo_vr poser

Usage: poser [-h | --help] [options]

Options:
    -h, --help      shows this message
    -q, --quit      exits the poser
)";

public:
    PoserTemplateBase(
        std::string addr="127.0.0.1",
        int port=6969,
        std::chrono::nanoseconds send_delay=std::chrono::nanoseconds(10000000)) {

        m_spSockComm = std::make_shared<utilz::SocketObj>(addr, port);
        m_spManagerSockComm = std::make_shared<utilz::SocketObj>(addr, port);

        m_spSockComm->m_sIdMessage = g_sPoserIdMsg+"\n";
        m_spSockComm->setCallback(this);
        m_spManagerSockComm->m_sIdMessage = g_sManagerIdMsg+"\n";        
        // m_spManagerSockComm->setCallback(this);

        register_member_thread(std::bind(&PoserTemplateBase::close, this), "close", std::chrono::nanoseconds(100000000)); // register the close thread first
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

    // send a manager message
    void _send_manager(ManagerPacket msg) {
        m_spManagerSockComm->send2(msg._to_pchar(), msg.len_bytes());
    }

    virtual void _cli_arg_map(std::pair<std::string, std::string>) {}

    virtual void send() = 0; // lmao, override this

    void OnPacket(char* buff, int len) {
        strncpy_s(m_chpLastReadBuff, buff, 4096); // just store the received buffer
    }

    void close() {
        Log("%s\n", CLI_STRING.c_str());

        while (m_mThreadRegistry["close"].is_alive) {
            try {
                std::vector<std::pair<std::string, std::string>> cli_args= get_cli_args(std::cin, CLI_STRING); // the oldest trick in the book, TODO: write get_cli_args

                if (std::find_if(cli_args.begin(), cli_args.end(), [](std::pair<std::string, std::string> x)->bool{return x.first == "--quit";}) != cli_args.end() ||
                    std::find_if(cli_args.begin(), cli_args.end(), [](std::pair<std::string, std::string> x)->bool{return x.first == "-q";}) != cli_args.end())
                    break;

                if (std::find_if(cli_args.begin(), cli_args.end(), [](std::pair<std::string, std::string> x)->bool{return x.first == "--help";}) != cli_args.end() ||
                    std::find_if(cli_args.begin(), cli_args.end(), [](std::pair<std::string, std::string> x)->bool{return x.first == "-h";}) != cli_args.end())
                    Log("%s\n", CLI_STRING.c_str());

                // im not sure what im surprised more about
                // the fact that i didn't have an aneurysm writing those 2 if statements
                // or that they work

                for (auto& i : cli_args) {
                    _cli_arg_map(i);
                }

            } catch(...) {
                Log("failed to parse command, try again\n");
                Log("%s\n", CLI_STRING.c_str());
            }

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

    void Main() {
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
        for (int i=1; i<l_vRunning.size(); i++) {
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
        if (!m_mThreadRegistry.count(trigger_name)) {
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
    std::vector<Pose*> m_vPoses; // NEVER modify it yourself

public:
    UduPoserTemplate( std::string udu_string,
        std::string addr="127.0.0.1",
        int port=6969,
        std::chrono::nanoseconds send_delay=std::chrono::nanoseconds(10000000)):PoserTemplateBase(addr, port, send_delay) {
        std::regex rgx2("([htc][ ])*([htc]$)");
        if (utilz::first_rgx_match(udu_string, rgx2) != udu_string){
            Log("invalid udu string\n");
            throw std::runtime_error("invalid udu string");
        }

        std::regex rgx("[htc]");
        auto udu_vector = utilz::get_rgx_vector(udu_string, rgx);

        for (auto i : udu_vector) {
            if (i == "h")
                m_vPoses.push_back(new Pose());
            else if (i == "c")
                m_vPoses.push_back(new ControllerPose());
            else if (i == "t")
                m_vPoses.push_back(new TrackerPose());
        }

        Log("total of %d new device(s) were added, ready for start\n", m_vPoses.size());
    }

    ~UduPoserTemplate() {
        for (auto& i : m_vPoses)
            delete i;
    }

    void send() {
        while (m_mThreadRegistry["send"].is_alive) {
            try {
                for (auto i : m_vPoses)
                    m_spSockComm->send2(i->_to_pchar(), i->len_bytes());

                m_spSockComm->send2(g_sMessageTerminator.c_str());

                std::this_thread::sleep_for(m_mThreadRegistry["send"].sleep_delay);
            } catch (...) {
                Log("send thread: failed, exiting\n");
                break;
            }

        }
    }
};

}; // namespace hvr

#endif // __HOBO_VR_LIB