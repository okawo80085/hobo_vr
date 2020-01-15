
//-----------------------------------------------------------------------------
// Purpose:controllerDriver
//-----------------------------------------------------------------------------
class CSampleControllerDriver : public vr::ITrackedDeviceServerDriver
{
public:
	CSampleControllerDriver(bool side)
	{
		m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
		m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;

		if (side) {
			m_sSerialNumber = "nut666";
		} else {
			m_sSerialNumber = "nut999";
		}
		m_sModelNumber = "MyController";

		poseController.poseTimeOffset = 0;
		poseController.poseIsValid = true;
		poseController.deviceIsConnected = true;
		poseController.qWorldFromDriverRotation = HmdQuaternion_Init(1, 0, 0, 0 );
		poseController.qDriverFromHeadRotation = HmdQuaternion_Init(1, 0, 0, 0 );
		poseController.vecPosition[0] = 0.;
		if (side) {
			poseController.vecPosition[1] = 1.;
		} else {
			poseController.vecPosition[1] = 1.1;
		}

		poseController.vecPosition[2] = 0.;
		handSide_ = side;
	}

	virtual ~CSampleControllerDriver()
	{
	}


	virtual EVRInitError Activate( vr::TrackedDeviceIndex_t unObjectId )
	{
		m_unObjectId = unObjectId;
		m_ulPropertyContainer = vr::VRProperties()->TrackedDeviceToPropertyContainer( m_unObjectId );

		vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, Prop_ModelNumber_String, m_sModelNumber.c_str() );
		vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, Prop_RenderModelName_String, m_sModelNumber.c_str() );

		// return a constant that's not 0 (invalid) or 1 (reserved for Oculus)
		vr::VRProperties()->SetUint64Property( m_ulPropertyContainer, Prop_CurrentUniverseId_Uint64, 2 );

		// even though we won't ever track we want to pretend to be the right hand so binding will work as expected
		if (handSide_) {
			vr::VRProperties()->SetInt32Property( m_ulPropertyContainer, Prop_ControllerRoleHint_Int32, TrackedControllerRole_RightHand );
		} else {
			vr::VRProperties()->SetInt32Property( m_ulPropertyContainer, Prop_ControllerRoleHint_Int32, TrackedControllerRole_LeftHand );
		}

		// this file tells the UI what to show the user for binding this controller as well as what default bindings should
		// be for legacy or other apps
		vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, Prop_InputProfilePath_String, "{sample}/input/mycontroller_profile.json" );

		// create all the input components
		vr::VRDriverInput()->CreateBooleanComponent( m_ulPropertyContainer, "/input/a/click", &m_compA );
		vr::VRDriverInput()->CreateBooleanComponent( m_ulPropertyContainer, "/input/b/click", &m_compB );
		vr::VRDriverInput()->CreateBooleanComponent( m_ulPropertyContainer, "/input/c/click", &m_compC );

		// create our haptic component
		vr::VRDriverInput()->CreateHapticComponent( m_ulPropertyContainer, "/output/haptic", &m_compHaptic );

		return VRInitError_None;
	}

	virtual void Deactivate()
	{
		m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
	}

	virtual void EnterStandby()
	{
	}

	void *GetComponent( const char *pchComponentNameAndVersion )
	{
		// override this to add a component to a driver
		return NULL;
	}

	virtual void PowerOff()
	{
	}

	/** debug request from a client */
	virtual void DebugRequest( const char *pchRequest, char *pchResponseBuffer, uint32_t unResponseBufferSize )
	{
		if ( unResponseBufferSize >= 1 )
			pchResponseBuffer[0] = 0;
	}

	virtual DriverPose_t GetPose()
	{
		poseController.result = TrackingResult_Running_OK;
		if (handSide_) {
			if ( (0x01 & GetAsyncKeyState( VK_UP )) != 0) {
				poseController.vecPosition[2] -= 0.01;
			}
			if ( (0x01 & GetAsyncKeyState( VK_DOWN )) != 0) {
				poseController.vecPosition[2] += 0.01;
			}

			if ( (0x01 & GetAsyncKeyState( VK_LEFT )) != 0) {
				poseController.vecPosition[0] -= 0.01;
			}
			if ( (0x01 & GetAsyncKeyState( VK_RIGHT )) != 0) {
				poseController.vecPosition[0] += 0.01;
			}

			if ( (0x01 & GetAsyncKeyState( VK_NUMPAD0 )) != 0) {
				poseController.vecPosition[1] -= 0.01;
			}
			if ( (0x01 & GetAsyncKeyState( VK_NUMPAD1 )) != 0) {
				poseController.vecPosition[1] += 0.01;
			}
		}
		return poseController;
	}


	void RunFrame()
	{
#if defined( _WINDOWS )
		// Your driver would read whatever hardware state is associated with its input components and pass that
		// in to UpdateBooleanComponent. This could happen in RunFrame or on a thread of your own that's reading USB
		// state. There's no need to update input state unless it changes, but it doesn't do any harm to do so.
		if (handSide_)
		{
			vr::VRDriverInput()->UpdateBooleanComponent( m_compA, (0x8000 & GetAsyncKeyState( 'V' )) != 0, 0 );
			vr::VRDriverInput()->UpdateBooleanComponent( m_compB, (0x8000 & GetAsyncKeyState( 'B' )) != 0, 0 );
			vr::VRDriverInput()->UpdateBooleanComponent( m_compC, (0x8000 & GetAsyncKeyState( 'C' )) != 0, 0 );
		}
#endif
		if ( m_unObjectId != vr::k_unTrackedDeviceIndexInvalid )
		{
			vr::VRServerDriverHost()->TrackedDevicePoseUpdated( m_unObjectId, GetPose(), sizeof( DriverPose_t ) );
		}
	}

	void ProcessEvent( const vr::VREvent_t & vrEvent )
	{
		switch ( vrEvent.eventType )
		{
		case vr::VREvent_Input_HapticVibration:
		{
			if ( vrEvent.data.hapticVibration.componentHandle == m_compHaptic )
			{
				// This is where you would send a signal to your hardware to trigger actual haptic feedback
				DriverLog( "BUZZ!\n" );
			}
		}
		break;
		}
	}


	std::string GetSerialNumber() const { return m_sSerialNumber; }

private:
	vr::TrackedDeviceIndex_t m_unObjectId;
	vr::PropertyContainerHandle_t m_ulPropertyContainer;

	vr::VRInputComponentHandle_t m_compA;
	vr::VRInputComponentHandle_t m_compB;
	vr::VRInputComponentHandle_t m_compC;
	vr::VRInputComponentHandle_t m_compHaptic;

	std::string m_sSerialNumber;
	std::string m_sModelNumber;

	DriverPose_t poseController;
	bool handSide_;

};
