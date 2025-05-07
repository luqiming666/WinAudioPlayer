//
// CAudioDeviceListener.h
//

#ifndef __H_CAudioDeviceListener__
#define __H_CAudioDeviceListener__

#include <iostream>
#include <Mmdeviceapi.h>

class IAudioDeviceListener {
public:
    virtual HRESULT OnAudioDeviceChanged(LPCWSTR pwstrDeviceId) = 0;
};

class CAudioDeviceListener : public IMMNotificationClient
{
private:
	LONG m_cRef;
    IAudioDeviceListener* m_pListener;

public:
    CAudioDeviceListener() : m_cRef(1), m_pListener(nullptr) {}
	~CAudioDeviceListener() {}

    void SetDeviceListener(IAudioDeviceListener* pListener) { m_pListener = pListener; }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_cRef); }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG ulRef = InterlockedDecrement(&m_cRef);
        //if (0 == ulRef) delete this;
        return ulRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID** ppvInterface) override {
        if (IID_IUnknown == riid || __uuidof(IMMNotificationClient) == riid) {
            *ppvInterface = static_cast<IMMNotificationClient*>(this);
            AddRef();
            return S_OK;
        }
        else {
            *ppvInterface = nullptr;
            return E_NOINTERFACE;
        }
    }

    // 设备状态变化回调
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) override {
        std::wstring deviceId(pwstrDeviceId);

        if (dwNewState == DEVICE_STATE_ACTIVE) {
            std::wcout << L"设备插入: " << deviceId << std::endl;
            if (m_pListener) m_pListener->OnAudioDeviceChanged(pwstrDeviceId);
        }
        else if (dwNewState == DEVICE_STATE_UNPLUGGED) {
            std::wcout << L"设备拔出: " << deviceId << std::endl;
            if (m_pListener) m_pListener->OnAudioDeviceChanged(pwstrDeviceId);
        }

        return S_OK;
    }

    // 其他接口方法（未实现）
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key) override { return S_OK; }
};

#endif // __H_CAudioDeviceListener__