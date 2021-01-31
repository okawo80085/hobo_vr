#include <iostream>
#include "virtualreality.h"

#include <string>
#include <cmath>

class MyPoser : public hvr::UduPoserTemplate {
public:
	MyPoser(std::string udu) : UduPoserTemplate(udu) {
		hvr::Log("MyPoser started\n");

		m_vPoses[0]->loc = { 0.0, 0.0, 0.0 };
		m_vPoses[0]->rot = { 1.0, 0.0, 0.0, 0.0 };
		m_vPoses[0]->vel = { 0.0, 0.0, 0.0 };
		m_vPoses[0]->ang_vel = { 0.0, 0.0, 0.0 };

		m_vPoses[1]->loc = { -0.1, 0.0, 0.0 };
		m_vPoses[1]->rot = { 1.0, 0.0, 0.0, 0.0 };
		m_vPoses[1]->vel = { 0.0, 0.0, 0.0 };
		m_vPoses[1]->ang_vel = { 0.0, 0.0, 0.0 };
		m_vPoses[1]->inputs = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

		m_vPoses[2]->loc = { 0.1, 0.0, 0.0 };
		m_vPoses[2]->rot = { 1.0, 0.0, 0.0, 0.0 };
		m_vPoses[2]->vel = { 0.0, 0.0, 0.0 };
		m_vPoses[2]->ang_vel = { 0.0, 0.0, 0.0 };
		m_vPoses[2]->inputs = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

		register_member_thread(std::bind(&MyPoser::my_thread, this), "my_thread", std::chrono::nanoseconds(10000000));
	}

	void my_thread() {
		float h = 0;
		while (m_mThreadRegistry["my_thread"].is_alive) {
			m_vPoses[0]->loc.x = sin(h);
			m_vPoses[0]->rot = { 1.0, 0.0, 0.0, 0.0 }; // steamvr is really sensitive to stale orientation at startup

			h += 0.01;

			std::this_thread::sleep_for(m_mThreadRegistry["my_thread"].sleep_delay); // thread sleep
		}
		m_mThreadRegistry["my_thread"].is_alive = false; // signal death
	}
};

int main() {
	MyPoser pp("h c c");
	if (pp.Start());
		pp.Main();

	return 0;
}