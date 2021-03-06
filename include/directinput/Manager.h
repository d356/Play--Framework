#ifndef _DIRECTINPUTMANAGER_H_
#define _DIRECTINPUTMANAGER_H_

#include "DirectInput.h"
#include <memory>
#include <list>
#include <map>
#include <unordered_map>
#include "win32/GuidUtils.h"
#include "Device.h"

namespace Framework
{
	namespace DirectInput
	{
		class CManager
		{
		public:
			typedef std::function<void (const GUID&, uint32, uint32)> InputEventHandler;

									CManager();
			virtual					~CManager();

			uint32					RegisterInputEventHandler(const InputEventHandler&);
			void					UnregisterInputEventHandler(uint32);

			void					CreateKeyboard(HWND);
			void					CreateJoysticks(HWND);
			bool					GetDeviceInfo(const GUID&, DIDEVICEINSTANCE*);
			bool					GetDeviceObjectInfo(const GUID&, uint32, DIDEVICEOBJECTINSTANCE*);

		private:
			typedef std::shared_ptr<CDevice> DevicePtr;
			typedef std::map<GUID, DevicePtr> DeviceList;
			typedef std::list<GUID> JoystickInstanceList;
			typedef std::unordered_map<uint32, InputEventHandler> InputEventHandlerMap;

			void					CallInputEventHandlers(const GUID&, uint32, uint32);

			DWORD					UpdateThreadProc();
			static DWORD CALLBACK	UpdateThreadProcStub(void*);

			static BOOL CALLBACK	EnumDevicesCallback(LPCDIDEVICEINSTANCE, LPVOID);
			BOOL					EnumDevicesCallbackImpl(LPCDIDEVICEINSTANCE);

			LPDIRECTINPUT8			m_directInput;
			JoystickInstanceList	m_joystickInstances;
			DeviceList				m_devices;

			InputEventHandlerMap	m_inputEventHandlers;
			uint32					m_nextInputEventHandlerId;

			HANDLE					m_updateThreadHandle;
			bool					m_updateThreadOver;
			CRITICAL_SECTION		m_updateMutex;
		};
	}
}

#endif
