#include <iostream>
#include "virtualreality.h"

#include <string>

class MyPoser : public hvr::UduPoserTemplate {
public:
	MyPoser(std::string udu) : UduPoserTemplate(udu) {
		hvr::Log("MyPoser started\n");

		m_vPoses[0]->loc = { 0, 0, 0 };
		m_vPoses[0]->rot = { 1, 0, 0 , 0 };
		m_vPoses[0]->vel = { 0, 0, 0 };
		m_vPoses[0]->ang_vel = { 0, 0,0 };

		register_member_thread(std::bind(&MyPoser::my_thread, this), "my_thread", std::chrono::nanoseconds(10000000));
	}

	void my_thread() {
		hvr::Log("my_thread started");
		while (m_mThreadRegistry["my_thread"].is_alive) {
			m_vPoses[0]->loc.x += 0.01;

			std::this_thread::sleep_for(m_mThreadRegistry["my_thread"].sleep_delay); // thread sleep
		}
		m_mThreadRegistry["my_thread"].is_alive = false; // signal death
		hvr::Log("my_thread ended");
	}
};

int main() {
	MyPoser pp("lmao");
	if (pp.Start());
		pp.Main();

	return 0;
}