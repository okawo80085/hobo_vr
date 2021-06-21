#ifndef HOBOVR_SERVER__H
#define HOBOVR_SERVER__H


namespace hobovr {
	// le version
	static const uint32_t k_nHobovrVersionMajor = 1;
	static const uint32_t k_nHobovrVersionMinor = 0;
	static const uint32_t k_nHobovrVersionBuild = 0;
	static const std::string k_sHobovrVersionGG = "Multi Hobo Drifting";

} // namespace hobovr

class ServerDriverHoboVR : public vr::IServerTrackedDeviceProvider, public hobovr::ZmqServer {
public:
	ServerDriverHoboVR() {}

	/*******************************
	* IServerTrackedDeviceProvider *
	*******************************/

	virtual vr::EVRInitError Init(vr::IVRDriverContext* pDriverContext);
	virtual void Cleanup();
	virtual const char* const* GetInterfaceVersions() {
		return vr::k_InterfaceVersions;
	} // IServerTrackedDeviceProvider
	virtual bool ShouldBlockStandbyMode() { return false; }
	virtual void EnterStandby() {}
	virtual void LeaveStandby() {}
	virtual void RunFrame();

	/********************
	* hobovr::ZmqServer *
	********************/
	bool on_packet(std::string& msg);

	void non_vr_init();

private:
	void slow_internal_thread();
	static void slow_internal_thread_enter(ServerDriverHoboVR* ptr) {
		ptr->slow_internal_thread();
	}
	void UpdateServerDeviceList();

	std::vector<hobovr::HobovrDeviceElement*> m_vDevices;
	std::vector<hobovr::HobovrDeviceElement*> m_vStandbyDevices;

	bool m_bDeviceListSyncEvent = false;

	// slower thread stuff
	bool is_slow_internal_thread_alive;
	std::thread* slow_internal_thread_ptr;
};

#endif // !HOBOVR_SERVER__H
