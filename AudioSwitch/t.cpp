#include <Mmdeviceapi.h>
#include <Audioclient.h>
#include <Audiopolicy.h>
#include <windows.h>
#include <stdio.h>
#include <locale.h>
#include <conio.h>
#include <Functiondiscoverykeys_devpkey.h>

#define evt_OnlyMix		1	// 只有立体声混音
#define evt_MixandMP	2	// 立体声混音+麦克风输入
#define evt_OnlyMP		3	// 只有麦克风输入

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);

/// <summary>
///	设置音频终端设备
/// </summary>
/// <param name="ID">音频终端设备ID</param>
/// <param name="state">启用/禁用:1/0</param>
INT SetEPDev(LPWSTR ID, UINT state)
{
	HKEY hKey;
	WCHAR devPath[1000] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\MMDevices\\Audio\\Capture\\";
	DWORD devSta = state ? 0x00000001 : 0x10000001;

	lstrcat(devPath, ID);
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, devPath, NULL, KEY_SET_VALUE, &hKey);
	RegSetValueEx(hKey, L"DeviceState", NULL, REG_DWORD, (BYTE*)&devSta, 4);
	RegCloseKey(hKey);

	return S_OK;
}
/// <summary>
///	处理音频输入设备
/// </summary>
/// <param name="pCollection">设备集合</param>
/// <param name="evt">事件编号
/// 默认只遍历，返回是否可用
/// </param>
INT Proc(IMMDeviceCollection *pCollection, INT evt = 0)
{
	IMMDevice *pEndpoint = NULL;
	IPropertyStore *pProps = NULL;
	LPWSTR pwszID = NULL;
	LPWSTR partID = NULL;
	DWORD pdwstate = NULL;
	BOOL Enable = FALSE;
	HRESULT hr = S_OK;

	// 获取音频输入设备数量
	UINT  count;
	hr = pCollection->GetCount(&count);
	for (UINT i = 0; i < count; i++)
	{
		// 获取单个音频设备
		hr = pCollection->Item(i, &pEndpoint);
		// 获取单个音频设备ID
		hr = pEndpoint->GetId(&pwszID);
		partID = wcsstr(pwszID, L".{") + 1;
		// 获取单个音频设备配置信息
		hr = pEndpoint->OpenPropertyStore(STGM_READ, &pProps);
		// 根据配置信息设备名称
		PROPVARIANT varName;
		PropVariantInit(&varName);
		hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
		switch (evt)
		{
		case 0:
			if (!wcsstr(varName.pwszVal, L"立体声混音"))
				Enable = TRUE;
			// 获取单个音频设备状态
			hr = pEndpoint->GetState(&pdwstate);
			wprintf(L"%ls %ls\n", pdwstate == DEVICE_STATE_ACTIVE ? L"ON " : L"OFF", varName.pwszVal);
			break;
		case evt_OnlyMix:
			if (wcsstr(varName.pwszVal, L"立体声混音"))
				SetEPDev(partID, 1);
			else
				SetEPDev(partID, 0);
			break;
		case evt_MixandMP:
			SetEPDev(partID, 1);
			break;
		case evt_OnlyMP:
			if (wcsstr(varName.pwszVal, L"立体声混音"))
				SetEPDev(partID, 0);
			else
				SetEPDev(partID, 1);
			break;
		default:
			break;
		}
		// 清空获取的音频设备ID
		CoTaskMemFree(pwszID);
		pwszID = NULL;
		// 清空获取的音频设备名称
		PropVariantClear(&varName);
		// 清除音频设备、信息
		pEndpoint->Release();
		pProps->Release();
	}
	// 初始确认不可使用(无立体声混音)
	if (!evt && !Enable)
		return S_FALSE;
	return S_OK;
}
INT main(VOID)
{
	IMMDeviceEnumerator *pEnumerator = NULL;
	IMMDeviceCollection *pCollection = NULL;
	CHAR ch;
	HRESULT hr = S_OK;

	setlocale(LC_ALL, "");
	CoInitialize(NULL);
	hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
	// 遍历音频输入终端
	pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE | DEVICE_STATE_DISABLED, &pCollection);

	while (TRUE)
	{
		if (Proc(pCollection))
		{
			printf("\n\n声卡不支持立体声混音！\n任意键退出……");
			_getch();
			ch = '0';
		}
		else
		{
			printf("\n\n选择：\n1: 只有立体声混音\n2: 立体声混音+麦克风输入\n3: 只有麦克风输入\n4: 刷新音频输入终端设备列表\n其它: 退出");
			ch = _getch();
		}
		ch -= '0';
		if (ch >= evt_OnlyMix && ch <= evt_OnlyMP)
			Proc(pCollection, ch);
		else if (ch == 4)
		{
			pCollection->Release();
			pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE | DEVICE_STATE_DISABLED, &pCollection);
		}
		else
			break;
		system("cls");
	}

	// 清除遍历器和集合
	pEnumerator->Release();
	pCollection->Release();

	return 0;
}