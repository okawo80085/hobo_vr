#include <iostream>

#include "ref/receiver.h"

int main()
{
    
    SockReceiver::DriverReceiver tt(57);

    tt.start(); // start recv thread

    for (int i=0; i<60; i++) {
        auto a = tt.get_pose(); // get pose vector

        std::cout << "message: [";
        for (auto j : a) {
            std::cout << j << ' ';
        }
        std::cout << "] " << a.size() << "\n";

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    tt.stop();
    return 0;
}