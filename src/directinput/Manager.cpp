#include "directinput/Manager.h"
#include "directinput/Keyboard.h"
#include "directinput/Joystick.h"
#include <stdexcept>

using namespace Framework::DirectInput;

CManager::CManager()
: m_updateThreadHandle(NULL)
, m_updateThreadOver(false)
, m_nextInputEventHandlerId(1)
{
	if(FAILED(DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, reinterpret_cast<void**>(&m_directInput), NULL)))
	{
		throw std::runtime_error("Couldn't create DirectInput8");
	}
	m_directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, &CManager::EnumDevicesCallback, this, DIEDFL_ATTACHEDONLY);
	{
		DWORD threadId = 0;
		InitializeCriticalSection(&m_updateMutex);
		m_updateThreadHandle = CreateThread(NULL, NULL, &CManager::UpdateThreadProcStub, this, NULL, &threadId);
	}
}

CManager::~CManager()
{
	m_updateThreadOver = true;
	WaitForSingleObject(m_updateThreadHandle, INFINITE);
	DeleteCriticalSection(&m_updateMutex);
	m_devices.clear();
	if(m_directInput != NULL)
	{
		m_directInput->Release();
	}
}

uint32 CManager::RegisterInputEventHandler(const InputEventHandler& inputEventHandler)
{
	uint32 eventHandlerId = m_nextInputEventHandlerId++;
	EnterCriticalSection(&m_updateMutex);
	{
		m_inputEventHandlers[eventHandlerId] = inputEventHandler;
	}
	LeaveCriticalSection(&m_updateMutex);
	return eventHandlerId;
}

void CManager::UnregisterInputEventHandler(uint32 eventHandlerId)
{
	EnterCriticalSection(&m_updateMutex);
	{
		auto inputEventHandlerIterator = m_inputEventHandlers.find(eventHandlerId);
		assert(inputEventHandlerIterator != std::end(m_inputEventHandlers));
		m_inputEventHandlers.erase(inputEventHandlerIterator);
	}
	LeaveCriticalSection(&m_updateMutex);
}

void CManager::CreateKeyboard(HWND window)
{
	LPDIRECTINPUTDEVICE8 device;
	if(FAILED(m_directInput->CreateDevice(GUID_SysKeyboard, &device, NULL)))
	{
		throw std::runtime_error("Couldn't create device.");
	}
	EnterCriticalSection(&m_updateMutex);
	{
		m_devices[GUID_SysKeyboard] = DevicePtr(new CKeyboard(device, window));
	}
	LeaveCriticalSection(&m_updateMutex);
}

void CManager::CreateJoysticks(HWND window)
{
	for(auto joystickIterator(std::begin(m_joystickInstances));
		std::end(m_joystickInstances) != joystickIterator; joystickIterator++)
	{
		LPDIRECTINPUTDEVICE8 device;
		const auto& deviceGuid(*joystickIterator);
		if(FAILED(m_directInput->CreateDevice(deviceGuid, &device, NULL)))
		{
			continue;
		}
		EnterCriticalSection(&m_updateMutex);
		{
			m_devices[deviceGuid] = DevicePtr(new CJoystick(device, window));
		}
		LeaveCriticalSection(&m_updateMutex);
	}
}

bool CManager::GetDeviceInfo(const GUID& deviceId, DIDEVICEINSTANCE* deviceInfo)
{
	if(!deviceInfo) return false;
	auto deviceIterator(m_devices.find(deviceId));
	if(deviceIterator == std::end(m_devices)) return false;
	auto& device = deviceIterator->second;
	return device->GetInfo(deviceInfo);
}

bool CManager::GetDeviceObjectInfo(const GUID& deviceId, uint32 id, DIDEVICEOBJECTINSTANCE* objectInfo)
{
	if(!objectInfo) return false;
	auto deviceIterator(m_devices.find(deviceId));
	if(deviceIterator == std::end(m_devices)) return false;
	auto& device = deviceIterator->second;
	return device->GetObjectInfo(id, objectInfo);
}

BOOL CALLBACK CManager::EnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
	return static_cast<CManager*>(pvRef)->EnumDevicesCallbackImpl(lpddi);
}

BOOL CManager::EnumDevicesCallbackImpl(LPCDIDEVICEINSTANCE lpddi)
{
	m_joystickInstances.push_back(lpddi->guidInstance);
	return DIENUM_CONTINUE;
}

void CManager::CallInputEventHandlers(const GUID& device, uint32 id, uint32 value)
{
	for(auto inputEventHandlerIterator(std::begin(m_inputEventHandlers));
		inputEventHandlerIterator != std::end(m_inputEventHandlers); inputEventHandlerIterator++)
	{
		inputEventHandlerIterator->second(device, id, value);
	}
}

DWORD CManager::UpdateThreadProc()
{
	auto inputEventHandler = std::bind(&CManager::CallInputEventHandlers, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	while(!m_updateThreadOver)
	{
		EnterCriticalSection(&m_updateMutex);
		{
			for(auto deviceIterator(std::begin(m_devices));
				deviceIterator != std::end(m_devices); deviceIterator++)
			{
				auto& device = deviceIterator->second;
				device->ProcessEvents(inputEventHandler);
			}
		}
		LeaveCriticalSection(&m_updateMutex);
		Sleep(16);
	}
	return 0;
}

DWORD WINAPI CManager::UpdateThreadProcStub(void* param)
{
	return reinterpret_cast<CManager*>(param)->UpdateThreadProc();
}
