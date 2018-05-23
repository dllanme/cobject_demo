#pragma once
#pragma warning(disable:4099)


#include "dNetObject.h"
#include "dBuffObject.h"
#include "dTimeObject.h"



////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
#define _dNETTEST_


#define dNETDEF_WSVER				0x0202
#define dNETDEF_MAXBUFFSIZE			(5 * dKB)
#define dNETDEF_MOASTSIZE			65535


#define dNETDEF_TIMEOUT				(3 * dSECMILL)
#define dNETDEF_DEFAULTPORT			1030
#define dNETDEF_NATIVEPACK			false
#define dNETDEF_PORTTHDENABLE		true
#define dNETDEF_MAXCONNECT			256
#define dNETDEF_INSIDECOMMAND		true
#define dNETDEF_REVERSESENDMODE		false

#define dNETDEF_THREADMODECONNECT	false
#define dNETDEF_THREADMODERECEIVE	false

#define dNETDEF_RWTIMEOUT			(dSECMILL * 15)
#define dNETDEF_CONNECTTIMEOUT		(dSECMILL * 5)
#define dNETDEF_RECONNECTTIME		(dSECMILL * 10)
#define dNETDEF_DISCONNECTTIME		(dSECMILL / 10)
#define dNETDEF_READTIME			(dSECMILL / 100)
#define dNETDEF_ONLINETIME			(dSECMILL * 20)
#define dNETDEF_CHECK_VALIDTIME		(dSECMILL)
#define dNETDEF_CHECKMATCTIME		(dSECMILL * 10)
#define dNETDEF_THREADWAITEXIT		(dSECMILL)
#define dNETDEF_SHORTWAIT			(10)


#define dNETDEF_WAIT_ID				1
#define dNETDEF_NOWAIT_ID			0


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
enum tagdNETBaseErrorEnum{

	dnetbaseerrorNONE,
	dnetbaseerrorSUCCEED,
	dnetbaseerrorFAIL,
	dnetbaseerrorERROR,

	dnetbaseerrorTIMEOUT_READ,
	dnetbaseerrorTIMEOUT_WRITE,
	dnetbaseerrorTIMEOUT_OTHER,

	dnetbaseerrorCONNECT_OUT,

};

typedef enum tagdNETBuffTypeEnum{
	dnetNATIVE,
	dnetBUFF,
	dnetPACK,
};

enum tagdNETInsideCmdEnum{
	dnetinsidecmdNONE,
	dnetinsidecmdCONNECT,
	dnetinsidecmdONLINE,
	dnetinsidecmdCLOSE,
};

typedef struct tagdNETClientInfoRec{
	bool				check_connect;
	dNETCONNECTINFO		connect_info;
	dSIZE				peer;
	dBYTE*				buff;
	dNETPACK*			pack;
	TdDataMatc			data_matc;
	dSIZE				reply_time;
	TdMutexObject		mutex;
	tagdNETClientInfoRec(){
		check_connect = false;
		buff = NULL;
		peer = NULL;
		pack = NULL;
		reply_time = 0;
	}
	//~tagdNETClientInfoRec(){
	//	if(buff)
	//		free(buff);
	//}
	void recreate_mutex(){
		mutex.ReCreateMutex(connect_info.netaddr.getstring());
	}
} dNETClientInfo, dNETCLIENTINFO;



////////////////////////////////////////////////////////////////////////////////////////////////////////

//#include "E:/develop2/sigslot/sigslot.h"
//using namespace sigslot;

#define dNETBASE TdNETBase

class TdNETBase/* : public has_slots<>*/
{
public:
	//static dSIZE NetConnectID;
	static bool CheckOpenListenPort(int port){
		return false;
	}
	static bool WaitEvent(HANDLE waitEvent, int waitTime = 1000){
		return (WAIT_OBJECT_0 == WaitForSingleObject(waitEvent, waitTime));
	}
	static bool UIWaitEvent(HANDLE waitEvent, int waitTime = 1000)
	{
		BOOL bRet;
		MSG msg;
		int startTick = ::GetTickCount();
		while((bRet = ::GetMessage(&msg, NULL, 0, 0)) != 0){
			if(::GetTickCount() - startTick >= (dSIZE)waitTime)
				break;
			if(WAIT_OBJECT_0 == WaitForSingleObject(waitEvent, 10))
				return true;
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		return false;
	}

public:
	TdNETBase(){
		if(!TdNetFrame::IsNetStartup){
			TdNetFrame::IsNetStartup = true;
			WSADATA wsa_data;
			WSAStartup(dNETDEF_WSVER, &wsa_data);
		}
		m_Data = NULL;
		m_Timeout = dNETDEF_RWTIMEOUT;
		m_ConnectTimeout = dNETDEF_CONNECTTIMEOUT;
		m_DefaultPort = dNETDEF_DEFAULTPORT;
		m_NativePack = dNETDEF_NATIVEPACK;
		memset(&m_BandAddr, 0, sizeof(dNETADDR));
		m_ErrorNumber = 0;
		m_Connected = false;
		m_ConnectTime = 0;
		m_SendOnlineInterval = dNETDEF_ONLINETIME;
	}
	virtual ~TdNETBase(){}

	void SetData(dSIZE value)							{m_Data = value;}
	void SetTimeout(dSIZE value)						{m_Timeout = value;}
	void SetConnectTimeout(dSIZE value)					{m_ConnectTimeout = value;}
	void SetDefaultPort(int value)						{m_DefaultPort = value;}
	void SetNativePack(bool value)						{m_NativePack = value;}
	void SetBandAddr(dNETADDR value)					{memcpy(&m_BandAddr, &value, sizeof(dNETADDR));}

	void SetConnectAddr(dNETADDR value)					{dNET::AutoGetDNS(value); if(!IsConnected()) memcpy(&m_ConnectAddr, &value, sizeof(dNETADDR));}
	void SetSendOnlineInterval(dSIZE value)				{m_SendOnlineInterval = value;}

	dSIZE	 GetData()									{return m_Data;}
	dSIZE    GetTimeout()								{return m_Timeout;}
	dNETADDR GetBandAddr()								{return m_BandAddr;}
	dNETADDR GetConnectAddr()							{return m_ConnectAddr;}

	bool IsConnected()									{return m_Connected;}
	bool IsNativePack()									{return m_NativePack;}

	int   GetDefaultPort()								{return m_DefaultPort;}
	dTIME GetConnectTime()								{return m_ConnectTime;}
	int   GetErrorNumber()								{return m_ErrorNumber;}

protected:
	dSIZE		m_Data;
	dSIZE		m_Timeout;
	dSIZE		m_ConnectTimeout;
	int			m_DefaultPort;
    bool		m_NativePack;
	dNETADDR	m_BandAddr;

	int			m_ErrorNumber;
	bool		m_Connected;
	dTIME		m_ConnectTime;

	dNETADDR	m_ConnectAddr;
	dSIZE		m_SendOnlineInterval;

};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class TdNETBasePort : public TdNETBase{
public:
	TdNETBasePort(){
	}
	virtual ~TdNETBasePort(){}

	virtual bool OpenPort(){
		if(!m_Connected){
			if(openPort()){
				m_Connected = true;
				m_ConnectTime = dCURRENTTIME;
			}
		}
		return m_Connected;
	}
	virtual bool ClosePort(){
		if(m_Connected)
			m_Connected = !closePort();
		return !m_Connected;
	}
	bool IsOpendPort()						{return m_Connected;}

protected:
	virtual bool openPort() = 0;
	virtual bool closePort() = 0;

    virtual void OnEventAccept(dNETCONNECTINFO* connect_info, bool* stop)	{}
    virtual void OnEventClose(dNETCONNECTINFO* connect_info)				{}

    virtual void OnEventReceiveNative(dNETCONNECTINFO* connect_info, dBYTE* buff, dSIZE size, dBYTE* reply_buff, dSIZE* reply_size, bool* stop)	{}
    virtual void OnEventReceiveBuffer(dNETCONNECTINFO* connect_info, dBYTE* buff, dSIZE size, dBYTE* reply_buff, dSIZE* reply_size, bool* stop)	{}
    virtual void OnEventReceivePack(dNETCONNECTINFO* connect_info, dNETPACK* pack, dNETPACK* reply_pack, bool* reply, bool* stop)				{}

    virtual void OnEventError(dNETCONNECTINFO* connect_info)	{}

	virtual void OnEventConnect(bool succeed, bool* reconnect)	{}
	virtual void OnEventDisConnect(bool* reconnect)				{}
    virtual void OnEventSendData(dSIZE size)					{}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class TdNETBasePortThread : public TdNETBasePort
{
public:
	TdNETBasePortThread(){
		m_PortThdHandle = NULL;
		m_PortThdID = NULL;
		m_PortThdStop = NULL;
		m_PortThdFinish = NULL;
		m_PortThdEnable = dNETDEF_PORTTHDENABLE;
		m_PortMutex.ReCreateMutex(dFormat("%d_mutex", this));

		m_fnOnPortThreadProc.Assign(this, &TdNETBasePortThread::OnPortThreadProc);
	}
	virtual ~TdNETBasePortThread(){
		StopPortThread();
		ClosePort();
	}

	virtual bool OpenPort(){
		if(m_Connected)
			return true;
		if(!TdNETBasePort::OpenPort())
			return false;
		if(m_PortThdEnable)
			StartPortThread();
		return true;
	}
	virtual bool ClosePort(){
		if(!m_Connected)
			return true;
		onNotifyStopPortThread();
		if(!StopPortThread())
			return false;
		return TdNETBasePort::ClosePort();
	}

	bool StartPortThread(){
		if(IsExistPortThread())
			return true;
		m_PortThdStop = CreateEvent(NULL, TRUE, FALSE, NULL);
		m_PortThdFinish = CreateEvent(NULL, TRUE, FALSE, NULL);
		m_PortThdHandle = (HANDLE)_beginthreadex(NULL, 0, m_fnOnPortThreadProc, NULL, CREATE_SUSPENDED, &m_PortThdID);
		::ResumeThread(m_PortThdHandle);
		return IsExistPortThread();
	}
	bool StopPortThread(){
		if(!IsExistPortThread())
			return true;
		SetEvent(m_PortThdStop);
		if(WAIT_OBJECT_0 != WaitForSingleObject(m_PortThdFinish, dNETDEF_THREADWAITEXIT))
			return false;
		CloseHandle(m_PortThdHandle);
		CloseHandle(m_PortThdStop);
		CloseHandle(m_PortThdFinish);
		m_PortThdHandle = NULL;
		return true;
	}
	void NotifyStopPortThread(){
		if(IsExistPortThread())
			onNotifyStopPortThread();
	}

	void SetPortThreadEnable(bool value)	{if(!IsOpendPort()) m_PortThdEnable = value;}
	void SetThreadModeEnable(bool value)	{SetPortThreadEnable(value);}

	bool IsPortThreadEnable()				{return m_PortThdEnable;}
	bool IsThreadModeEnable()				{return IsPortThreadEnable();}

	bool IsExistPortThread()				{return NULL != m_PortThdHandle;}

protected:
	HANDLE			m_PortThdHandle;
	unsigned		m_PortThdID;
	HANDLE			m_PortThdStop;
	HANDLE			m_PortThdFinish;
	bool			m_PortThdEnable;
	TdMutexObject	m_PortMutex;

	virtual bool doWaitStopPortThread(int wait = 1)	{return (WAIT_OBJECT_0 == WaitForSingleObject(m_PortThdStop, wait));}
	virtual void onNotifyStopPortThread()			{SetEvent(m_PortThdStop);}
	virtual void onPortThread() = 0;

private:
	typedef unsigned (TdNETBasePortThread::*CLASSTHREADPROC)(LPVOID);
	TdCallback<dTHREADPROC, TdNETBasePortThread, CLASSTHREADPROC> m_fnOnPortThreadProc;
	unsigned OnPortThreadProc(LPVOID lpParam){
		onPortThread();
		SetEvent(m_PortThdFinish);
		return 0;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class TdNETBaseServer : public TdNETBasePortThread
{
public:
	TdNETBaseServer(){
		m_PortThdEnable = true;
		m_MaxConnect = dNETDEF_MAXCONNECT;
		m_ResponseInsideCommand = dNETDEF_INSIDECOMMAND;
	}

	virtual bool StartListen()	{return OpenPort();}
	virtual bool StopListen()	{return ClosePort();}

	virtual bool  DisConnectAll() = 0;
	virtual bool  DisConnect(dSIZE Index) = 0;
	virtual int   DisConnectTimeout(dSIZE timeOut = dNETDEF_TIMEOUT) = 0;

	virtual dSIZE GetConnectCount() = 0;
	virtual void  GetConnectInfo(dNETCONNECTINFO* buff, const dSIZE size, dSIZE& count) = 0;
	virtual bool  GetConnectInfo(dNETCONNECTINFO& ConnectInfo, const dSIZE Index) = 0;
	virtual int   IndexOfConnectInfo(const dNETADDR& addr) = 0;
	
	void SetPortThreadEnable(bool value)		{}
	void SetMaxConnect(dSIZE value)				{m_MaxConnect = value;}
	void SetResponseInsideCommand(bool value)	{m_ResponseInsideCommand = value;}

	bool IsListen()								{return m_Connected;}

protected:
	dSIZE m_MaxConnect;
	bool  m_ResponseInsideCommand;

	//virtual void doSend(dSIZE size)											{OnEventSendData(size);}
	virtual void onAccept(dNETCONNECTINFO* connect_info, bool* stop)																		{OnEventAccept(connect_info, stop);}
	virtual void onClose(dNETCONNECTINFO* connect_info)																						{OnEventClose(connect_info);};
	virtual void onError(dNETCONNECTINFO* connect_info)																						{OnEventError(connect_info);};
	virtual void onReceiveNative(dNETCONNECTINFO* connect_info, dBYTE* buff, dSIZE size, dBYTE* reply_buff, dSIZE* reply_size, bool* stop)	{OnEventReceiveNative(connect_info, buff, size, reply_buff, reply_size, stop);}
	virtual void onReceiveBuff(dNETCONNECTINFO* connect_info, dBYTE* buff, dSIZE size, dBYTE* reply_buff, dSIZE* reply_size, bool* stop)	{OnEventReceiveBuffer(connect_info, buff, size, reply_buff, reply_size, stop);}
	virtual void onReceivePack(dNETCONNECTINFO* connect_info, dNETPACK* pack, dNETPACK* reply_pack, bool* reply, bool* stop)				{OnEventReceivePack(connect_info, pack, reply_pack, reply, stop);}

	virtual bool doResponseInsideCommand(dNETCLIENTINFO* client_info, dBYTE* buff, dSIZE size, bool* stop, bool& reply)						{return false;}
	
	//virtual void doPortReverseSend(dNETCLIENTINFO* client_info, bool* stop);

};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class TdNETBaseClient : public TdNETBasePortThread
{
public:
	TdNETBaseClient(){
		//m_SendOnlineInterval = dNETDEF_ONLINETIME;
		m_ThreadModeConnect = dNETDEF_THREADMODECONNECT;
		m_ThreadModeReceive = dNETDEF_THREADMODERECEIVE;
		m_WaitConnectEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	}
	~TdNETBaseClient(){
		CloseHandle(m_WaitConnectEvent);
	}

	virtual bool ConnectServer()	{return OpenPort();}
	virtual bool DisConnectServer()	{return ClosePort();}
	virtual bool WaitForConnectServer(dSIZE timeOut = 0){
		if(IsConnectedServer())
			return true;
		if(timeOut > 0)
			SetTimeout(timeOut);
		//if(!m_ThreadModeConnect)
		//	return ConnectServer();
		ResetEvent(m_WaitConnectEvent);
		if(ConnectServer() && WaitEvent(m_WaitConnectEvent, GetTimeout()))
			return true;
		DisConnectServer();
		return false;
	}

	virtual bool PackSend(int command, bool autoThread = false, dOBJECT* buff = NULL, dSIZE buffSize = 0, long lParam = 0, DWORD wParam = 0){
		dNETPACK pack(command);
		pack.cmd.lparam = lParam;
		pack.cmd.wparam = wParam;
		if(buff && buffSize > 0)
			pack.addbuff(buff, buffSize);
		return SendPack(pack, autoThread);
	}
	virtual bool PackTransfers(dNETPACK& rPack, dSIZE checkReceiveBuffSize, int command, dOBJECT* buff = NULL, dSIZE buffSize = 0, long lParam = 0, DWORD wParam = 0, int wait = dNETDEF_TIMEOUT){
		if(!PackSend(command, false, buff, buffSize, lParam, wParam))
			return false;
		if(!ReceivePack(rPack, wait))
			return false;
		if(checkReceiveBuffSize > 0)
			return rPack.cmd.buffsize == checkReceiveBuffSize;
		return true;
	}
	virtual bool TransfersPack(const dNETPACK& spack, dNETPACK& rpack, int wait = dNETDEF_TIMEOUT){
		return SendPack(spack, false) && ReceivePack(rpack, wait);
	}

	virtual bool SendPack(const dNETPACK& pack, bool autoThread = true) = 0;
	virtual int  SendBuff(const dBYTE* buff, dSIZE size, bool autoThread = true) = 0;
	virtual int  SendNative(const dBYTE* buff, dSIZE size, bool autoThread = true) = 0;

	virtual bool ReceivePack(dNETPACK& pack, int wait = dNETDEF_TIMEOUT) = 0;
	virtual int  ReceiveBuff(dBYTE* buff, dSIZE size, int wait = dNETDEF_TIMEOUT, dSIZE* data_size = NULL) = 0;
	virtual int  ReceiveNative(dBYTE* buff, dSIZE size, int wait = dNETDEF_TIMEOUT, dSIZE* data_size = NULL) = 0;

	virtual int  GetRecvDataSize()	{return 0;}
	virtual int  GetSendDataSize()	{return 0;}

	//void SetConnectAddr(dNETADDR value)			{if(!IsConnectedServer()) memcpy(&m_ConnectAddr, &value, sizeof(dNETADDR));}
	//void SetSendOnlineInterval(dSIZE value)		{m_SendOnlineInterval = value;}

	bool IsConnectedServer()					{return m_Connected;}
	bool IsThreadModeConnect()					{return m_ThreadModeConnect;}
	bool IsThreadModeReceive()					{return m_ThreadModeReceive;}

	void SetThreadModeConnect(bool value)		{if(!IsOpendPort()) m_ThreadModeConnect = value;}
	void SetThreadModeReceive(bool value)		{m_ThreadModeReceive = value;}

protected:
	bool		m_ThreadModeConnect;
	bool		m_ThreadModeReceive;
	HANDLE		m_WaitConnectEvent;

	virtual void onSend(dSIZE size)											{OnEventSendData(size);}
	virtual void onConnect(bool succeed, bool* reconnect)					{if(succeed) SetEvent(m_WaitConnectEvent); OnEventConnect(succeed, reconnect);}
	virtual void onDisConnect(bool* reconnect)								{OnEventDisConnect(reconnect);}
	virtual void onError()													{OnEventError(NULL);}
	virtual void onReceiveNative(dBYTE* buff, dSIZE size)					{OnEventReceiveNative(NULL, buff, size, NULL, NULL, NULL);}
	virtual void onReceiveBuff(dBYTE* buff, dSIZE size)						{OnEventReceiveBuffer(NULL, buff, size, NULL, NULL, NULL);}
	virtual void onReceivePack(dNETPACK* pack)								{OnEventReceivePack(NULL, pack, NULL, NULL, NULL);}


};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class TdNETBaseAddrs{
public:
	void AddrsClear(bool addrsInclude = true){
		TdAddrList* addrs = &m_AddrsInclude;
		if(!addrsInclude)
			addrs = &m_AddrsBlock;
		addrs->Clear();
	}
	void AddrsAdd(const dNETADDR& addr, bool addrsInclude = true){
		TdAddrList* addrs = &m_AddrsInclude;
		if(!addrsInclude)
			addrs = &m_AddrsBlock;
		addrs->AddAddr(&addr);
	}
	void AddrsRemove(const dNETADDR& addr, bool addrsInclude = true){
		TdAddrList* addrs = &m_AddrsInclude;
		if(!addrsInclude)
			addrs = &m_AddrsBlock;
		int index = addrs->IndexOfAddr(&addr);
		addrs->Delete(index);
	}

protected:
	TdAddrList			m_AddrsInclude;
	TdAddrList			m_AddrsBlock;

	bool doCheckAddr(const dNETADDR* addr){
		if(m_AddrsInclude.GetCount() > 0)
			return m_AddrsInclude.ExistsIP(addr->addr);
		if(m_AddrsBlock.GetCount() > 0)
			return !m_AddrsBlock.ExistsIP(addr->addr);
		return true;
	}
};

class TdNETBaseClients{
protected:
	TdSafeList	m_ClientList;

	dNETCLIENTINFO*	findClient(dSIZE peer){
		dNETCLIENTINFO* item;
		for(dSIZE i = 0; i < m_ClientList.GetCount(); i++){
			item = (dNETCLIENTINFO*)m_ClientList.At(i);
			if(peer == item->peer)
				return item;
		}
		return NULL;
	}
	dNETCLIENTINFO*	findClient(const dNETADDR& addr){
		dNETCLIENTINFO* item;
		for(dSIZE i = 0; i < m_ClientList.GetCount(); i++){
			item = (dNETCLIENTINFO*)m_ClientList.At(i);
			if(item->connect_info.netaddr.issame(addr))
				return item;
		}
		return NULL;
	}
	int indexOfClient(const dNETADDR& addr){
		dNETCLIENTINFO* item;
		for(dSIZE i = 0; i < m_ClientList.GetCount(); i++){
			item = (dNETCLIENTINFO*)m_ClientList.At(i);
			if(item->connect_info.netaddr.issame(addr))
				return i;
		}
		return dNULL;
	}
	dSIZE getConnectCount()	{return m_ClientList.GetCount();}
	void getConnectInfo(dNETCONNECTINFO* buff, const dSIZE size, dSIZE& count){
		count = getConnectCount();
		if(count > size)
			count = size;
		for(dSIZE i = 0; i < count; i++)
			getConnectInfo(buff[i], i);
	}
	bool getConnectInfo(dNETCONNECTINFO& ConnectInfo, const dSIZE Index){
		dNETCLIENTINFO* info = (dNETCLIENTINFO*)m_ClientList.At(Index);
		if(!info)
			return false;
		memcpy(&ConnectInfo, &info->connect_info, sizeof(dNETCONNECTINFO));
		return true;
	}
	int indexOfConnectInfo(const dNETADDR& addr){
		return indexOfClient(addr);
	}

};

class TdNETBaseReverseSendList{
public:
	TdNETBaseReverseSendList(){
		m_ReverseSendMode = dNETDEF_REVERSESENDMODE;
		m_RSendList.setMaxListSize(dMAXSIZE);
	}
	virtual ~TdNETBaseReverseSendList(){
		m_RSendList.clearList();
	}
	
	void SetReverseSendMode(bool value)			{m_ReverseSendMode = value;}

protected:
	bool			m_ReverseSendMode;
	TdBuffExtList	m_RSendList;

	bool addBuffToRSL(const dBYTE* buff, dSIZE size, dSIZE id = 0, dSIZE data = 0){
		return m_RSendList.addBuff(buff, size, id, data);
	}
	bool doRSLFetchBuff(){
		dBUFFITEMEXT* bie = m_RSendList.fetchBuff();
		if(bie){
			onRSLFetchBuff(bie->buff, bie->size, bie->id, bie->addtime);
			delete bie;
			return true;
		}
		return false;
	}
	
	
	virtual void onRSLFetchBuff(const dBYTE* buff, dSIZE size, dSIZE id, dSIZE data) {}

};

#define TSSTATSEC  3
class TdNETBaseTrafficStatistics{
public:
	TdNETBaseTrafficStatistics()			{tsClear();}
	void TSCheckOnSec()						{tsCheckOnSec();}

	double	GetTSRecvRate()					{return m_TSRecvRate;}
	double	GetTSSendRate()					{return m_TSSendRate;}
	dSIZE	GetTSRecvPackTotal()			{return m_TSRecvPackTotal;}
	dSIZE	GetTSSendPackTotal()			{return m_TSSendPackTotal;}
	double	GetTSRecvRatePrevious()			{return m_TSRecvRatePrevious;}
	double	GetTSSendRatePrevious()			{return m_TSSendRatePrevious;}
	dSIZE	GetTSRecvPackTotalPrevious()	{return m_TSRecvPackTotalPrevious;}
	dSIZE	GetTSSendPackTotalPrevious()	{return m_TSSendPackTotalPrevious;}

protected:
	dSIZE	m_TSRecvLastTick;
	dSIZE	m_TSSendLastTick;
	dSIZE	m_TSRecvTotal;
	dSIZE	m_TSSendTotal;
	double	m_TSRecvRate;
	double	m_TSSendRate;
	dSIZE	m_TSRecvPackTotal;
	dSIZE	m_TSSendPackTotal;
	double	m_TSRecvRatePrevious;
	double	m_TSSendRatePrevious;
	dSIZE	m_TSRecvPackTotalPrevious;
	dSIZE	m_TSSendPackTotalPrevious;

	void tsClear(){
		m_TSRecvLastTick = 0;
		m_TSSendLastTick = 0;
		m_TSRecvTotal = 0;
		m_TSSendTotal = 0;
		m_TSRecvRate = 0;
		m_TSSendRate = 0;
		m_TSRecvPackTotal = 0;
		m_TSSendPackTotal = 0;
		m_TSRecvRatePrevious = 0;
		m_TSSendRatePrevious = 0;
		m_TSRecvPackTotalPrevious = 0;
		m_TSSendPackTotalPrevious = 0;
	}
	void tsWriteRecvTraffic(dSIZE size){
		dSIZE tickCurrent = dTICKCOUNT;
		dSIZE millTotal = tickCurrent - m_TSRecvLastTick;
		m_TSRecvTotal += size;
		m_TSRecvPackTotal++;
		if(tickCurrent - m_TSRecvLastTick >= dSECMILL){
			m_TSRecvRatePrevious = m_TSRecvRate;
			m_TSRecvPackTotalPrevious = m_TSRecvPackTotal;

			m_TSRecvRate = (double)m_TSRecvTotal / millTotal * dSECMILL;
			m_TSRecvLastTick = tickCurrent;
			m_TSRecvTotal = 0;
			m_TSRecvPackTotal = 0;
		}
	}
	void tsWriteSendTraffic(dSIZE size){
		dSIZE tickCurrent = dTICKCOUNT;
		dSIZE millTotal = tickCurrent - m_TSSendLastTick;
		m_TSSendTotal += size;
		m_TSSendPackTotal++;
		if(tickCurrent - m_TSSendLastTick >= dSECMILL){
			m_TSSendRatePrevious = m_TSSendRate;
			m_TSSendPackTotalPrevious = m_TSSendPackTotal;

			m_TSSendRate = (double)m_TSSendTotal / millTotal * dSECMILL;
			m_TSSendLastTick = tickCurrent;
			m_TSSendTotal = 0;
			m_TSSendPackTotal = 0;
		}
	}
	void tsCheckOnSec(){
		if(dTICKCOUNT - m_TSRecvLastTick >= TSSTATSEC * dSECMILL){
			m_TSRecvLastTick = dTICKCOUNT;
			m_TSRecvTotal = 0;
			m_TSRecvRate = 0;
			m_TSRecvPackTotal = 0;
			m_TSRecvRatePrevious = 0;
			m_TSRecvPackTotalPrevious = 0;
		}
		if(dTICKCOUNT - m_TSSendLastTick >= TSSTATSEC * dSECMILL){
			m_TSSendLastTick = dTICKCOUNT;
			m_TSSendTotal = 0;
			m_TSSendRate = 0;
			m_TSSendPackTotal = 0;
			m_TSSendRatePrevious = 0;
			m_TSSendPackTotalPrevious = 0;
		}
	}
};

//class TdNETBaseServerFetchThread : public TdFetchBuffThread, public TdNETBaseServer
//{
//public:
//	virtual bool OpenPort(){
//		if(!TdNETBaseServer::OpenPort())
//			return false;
//		if(m_FetchThdEnable)
//			StartFetchThread();
//		return true;
//	}
//	virtual bool ClosePort(){
//		if(!StopFetchThread())
//			return false;
//		return TdNETBasePortThread::ClosePort();
//	}
//protected:
//	virtual void onFetchBuff(dBUFFITEM* item){}
//
//};

//class TdNETBaseServerFrame : public TdNETBaseServer, public TdFetchBuffThread, public TdNETBaseAddrs, public TdNETBaseClients
class TdNETBaseServerFrame : public TdNETBaseServer, public TdNETBaseReverseSendList, public TdNETBaseAddrs, public TdNETBaseClients, public TdNETBaseTrafficStatistics
{
public:
	//virtual bool OpenPort(){
	//	if(!TdNETBaseServer::OpenPort())
	//		return false;
	//	if(m_FetchThdEnable)
	//		StartFetchThread();
	//	return true;
	//}
	//virtual bool ClosePort(){
	//	if(!StopFetchThread())
	//		return false;
	//	return TdNETBasePortThread::ClosePort();
	//}

	virtual bool SendPack(const dNETPACK& pack, int connectIndex)			{
		dNETCLIENTINFO* clientInfo = (dNETCLIENTINFO*)m_ClientList.At(connectIndex);
		if(clientInfo)
			return sendPack(clientInfo, pack);
		return false;
	}
	virtual int  SendBuff(const dBYTE* buff, dSIZE size, int connectIndex)	{
		dNETCLIENTINFO* clientInfo = (dNETCLIENTINFO*)m_ClientList.At(connectIndex);
		if(clientInfo)
			return sendBuff(clientInfo, buff, size);
		return false;
	}
	virtual int  SendNative(const dBYTE* buff, dSIZE size, int connectIndex){
		dNETCLIENTINFO* clientInfo = (dNETCLIENTINFO*)m_ClientList.At(connectIndex);
		if(clientInfo)
			return sendNative(clientInfo, buff, size);
		return false;
	}

	virtual bool DisConnectAll(){
		//TdListBase removeList;
		//dNETCLIENTINFO* info;
		//dSIZE count = GetConnectCount();
		//for(dSIZE i  = 0; i < count; i++){
		//	info = (dNETCLIENTINFO*)m_ClientList.At(i);
		//	removeList.Add(info);
		//}
		//dSIZE index;
		//for(dSIZE i = 0; i < removeList.GetCount(); i++){
		//	index = m_ClientList.IndexOf(removeList.At(i));
		//	DisConnect(index);
		//}
		//return 0 == m_ClientList.GetCount();
		dSIZE count = GetConnectCount();
		for(dSIZE i = 0; i < count; i++)
			DisConnect(0);
		return 0 == m_ClientList.GetCount();
	}
	virtual int DisConnectTimeout(dSIZE timeOut = dTIMEOUT){
		if(!IsOpendPort())
			return 0;
		if(0 == timeOut)
			timeOut = GetTimeout();
		dTIME curTIME = dCURRENTTIME;
		int result = 0;
		TdListBase removeList;
		dSIZE count = GetConnectCount();
		dNETCLIENTINFO* info;
		for(dSIZE i  = 0; i < count; i++){
			info = (dNETCLIENTINFO*)m_ClientList.At(i);
			if(info){
				if(timeOut <= dTIMEFRAME::TimeSpan(curTIME, info->connect_info.online, dtimeMILLSEC))
					removeList.Add(info);
			}
		}
		dSIZE index;
		for(dSIZE i = 0; i < removeList.GetCount(); i++){
			index = m_ClientList.IndexOf(removeList.At(i));
			if(DisConnect(index))
				result++;
		}
		return result;
	}

	virtual dSIZE GetConnectCount()														{return getConnectCount();}
	virtual void  GetConnectInfo(dNETCONNECTINFO* buff, const dSIZE size, dSIZE& count)	{getConnectInfo(buff, size, count);}
	virtual bool  GetConnectInfo(dNETCONNECTINFO& ConnectInfo, const dSIZE Index)		{return getConnectInfo(ConnectInfo, Index);}
	virtual int   IndexOfConnectInfo(const dNETADDR& addr)								{return indexOfConnectInfo(addr);}
	
protected:
	//virtual void onFetchBuff(dBUFFITEM* item){}

	virtual bool sendPack(dNETCLIENTINFO* client_info, const dNETPACK& pack) = 0;
	virtual int  sendBuff(dNETCLIENTINFO* client_info, const dBYTE* buff, dSIZE size) = 0;
	virtual int  sendNative(dNETCLIENTINFO* client_info, const dBYTE* buff, dSIZE size) = 0;

};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class TdNETBaseClientFrame : public TdNETBaseClient, public TdBuffListFrame
{
public:
	TdNETBaseClientFrame(){
		m_InsideConnected = false;
		SetNoCheckMaxID(dNETDEF_NOWAIT_ID);
	}
	virtual ~TdNETBaseClientFrame(){
		m_DataMatc.ClearAllList();
	}

	virtual bool SendPack(const dNETPACK& pack, bool autoThread = true){
		if(!checkSendStatus(autoThread, TdDataMatc::CountOfPackConvertDBIS(pack)))
			return false;
		dDATABUFFITEMS dbis;
		TdDataMatc::PackConvertDBIS(pack, dbis);
		//int dbis_size = TdDataMatc::GetDBISSendSize(dbis);
		return doSendDBIS(dbis, autoThread) == TdDataMatc::GetDBISSendSize(dbis);
	}
	virtual int SendBuff(const dBYTE* buff, dSIZE size, bool autoThread = true){
		if(!checkSendStatus(autoThread, TdDataMatc::CountOfBuffConvertDBIS(size)))
			return false;
		dDATABUFFITEMS dbis;
		TdDataMatc::BuffConvertDBIS(buff, size, dbis);
		return doSendDBIS(dbis, autoThread);
	}
	virtual int SendNative(const dBYTE* buff, dSIZE size, bool autoThread = true){
		if(!checkSendStatus(autoThread, 1))
			return false;
		if(!autoThread)
			return doSendNative(buff, size);
		addBuffItem(buff, size, dNETDEF_WAIT_ID);
		return 0;
	}
	virtual bool ReceivePack(dNETPACK& pack, int wait = dNETDEF_TIMEOUT){
		if(!checkPortStatus(true) || (IsExistPortThread() && m_ThreadModeReceive))
			return FALSE;
		dBUFFITEM* bi = doReceiveBI(wait);
		if(!bi){
			return false;
		}
		return TdDataMatc::BIConvertPack(bi, pack);
	}
	virtual int ReceiveBuff(dBYTE* buff, dSIZE size, int wait = dNETDEF_TIMEOUT, dSIZE* data_size = NULL){
		if(!checkPortStatus(true) || (IsExistPortThread() && m_ThreadModeReceive))
			return FALSE;
		dBUFFITEM* bi = doReceiveBI(wait);
		int result = 0;
		if(bi){
			if(data_size)
				*data_size = bi->size;
			if(size >= bi->size){
				memcpy(buff, bi->buff, bi->size);
				result = bi->size;
			}
		}
		return result;
	}
	virtual int ReceiveNative(dBYTE* buff, dSIZE size, int wait = dNETDEF_TIMEOUT, dSIZE* data_size = NULL){
		if(!checkPortStatus(true) || (IsExistPortThread() && m_ThreadModeReceive))
			return FALSE;
		return doReceiveNative(buff, size, wait, data_size);
	}

	bool IsInsideConnected()								{return m_InsideConnected;}

protected:
	TdDataMatc		m_DataMatc;
	bool			m_InsideConnected;
	dSIZE			m_LastOnlineTime;

	virtual bool checkPortStatus(bool check_receive)		{return m_InsideConnected;}
	virtual bool checkPortOnline(){
		if(!m_InsideConnected)
			return false;
		if(0 >= m_SendOnlineInterval || dTICKCOUNT - m_LastOnlineTime < m_SendOnlineInterval)
			return true;
		return doCheckOnline();
	}
	virtual bool checkSendStatus(bool& autoThread, int packCount = 1){
		if(!checkPortStatus(false) || !m_InsideConnected)
			return false;
			//不存在发送线程时，强制使用立即发送
		if(!IsExistPortThread())
			autoThread = false;
		//当接收模式下，或当发送列队中内容时 强制使用线程发送
		else if(IsExistPortThread() && (m_ThreadModeReceive || getBuffListCount() > 0))
			autoThread = true;
		if(autoThread && !CheckAddBuffList(packCount))
			return false;
		if(!autoThread)
			if(!checkPortOnline())
				return false;
		return true;
	}

protected:
	virtual int			doSendNative(const dBYTE* buff, dSIZE size) = 0;
	virtual int			doReceiveNative(dBYTE* buff, dSIZE size, int wait = dNETDEF_TIMEOUT, dSIZE* data_size = NULL) = 0;
	virtual dBUFFITEM*	doReceiveBI(int wait = dNETDEF_TIMEOUT) = 0;
	virtual bool		doCheckOnline() = 0;

	virtual int doSendDBIS(const dDATABUFFITEMS& dbis, bool autoThread = true){
		int result = 0;
		for(dSIZE i = 0; i < dbis.size(); i++){
			if(autoThread)
				addBuffItem(dbis[i].bytes, dbis[i].dbisize(), (i == dbis.size() - 1) ? dNETDEF_WAIT_ID : dNETDEF_NOWAIT_ID);
			else
				result += doSendNative(dbis[i].bytes, dbis[i].dbisize()); 
		}
		return result;
	}

protected:
	virtual void onPortThread(){}

};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TdNETBaseMultiClientFrame : public TdNETBase, public TdNETBaseTrafficStatistics{
public:
	TdNETBaseMultiClientFrame(int clientCount = 1, const char* groupID = NULL){
		m_ClientCreated = false;
		m_ClientCount = clientCount;
		m_ClientLast = 0;
		if(groupID)
			strcpy(m_GroupID, groupID);
		else
			strcpy(m_GroupID, "");
	}
	virtual ~TdNETBaseMultiClientFrame()	{ReleaseClients();}
	
	void SetClientCount(int value)			{if(!IsClientCreated()) m_ClientCount = value;}
	void SetGroupID(const char* value)		{if(value) strcpy(m_GroupID, value);}
	bool CheckGroupID(const char* id)		{return 0 == strcmp(m_GroupID, id);}
	CString GetGroupID()					{return m_GroupID;}
	
	int ClientsConnect(){
		if(0 >= m_ClientCount)
			return 0;
		if(!m_ClientCreated){
			for(int i = 0; i < m_ClientCount; i++)
				m_ClientList.Add(createSocket());
			m_ClientLock.ReCreateMutex(dFormat("clients_mutex_%s_%d", m_GroupID, this));
		}
		m_ClientCreated = true;
		m_ClientLast = 0;
		int succeedTotal = 0;
		for(int i = 0; i < m_ClientCount; i++){
			TdNETBaseClientFrame* client = (TdNETBaseClientFrame*)m_ClientList.At(i);
			if(!client->IsConnectedServer()){
				client->SetData(i);
				client->SetDefaultPort(m_DefaultPort);
				client->SetConnectAddr(dNET::NetAddr(m_ConnectAddr.addr, m_ConnectAddr.port+i));
				client->SetTimeout(m_Timeout);
				client->SetBuffListMax(dKB);
				client->SetThreadModeConnect(true);
				client->SetThreadModeReceive(false);
				client->SetThreadModeEnable(true);
				if(client->ConnectServer())
					succeedTotal++;
			}
			else
				succeedTotal++;
		}
		return succeedTotal;
	}
	void ReleaseClients(){
		if(!m_ClientCreated)
			return;
		m_ClientLock.Lock();
		for(dSIZE i = 0; i < m_ClientList.GetCount(); i++){
			TdNETBaseClientFrame* client = (TdNETBaseClientFrame*)m_ClientList.At(i);
			client->DisConnectServer();
			releaseSocket(client);
		}
		m_ClientList.Clear();
		m_ClientCreated = false;
		m_ClientCount = 0;
		m_ClientLock.UnLock();
	}

	bool BroadcastPack(const dNETPACK& pack, bool autoThread = true){
		if(!m_ClientCreated)
			return false;
		bool result = true;
		m_ClientLock.Lock();
		for(int i = 0; i < m_ClientCount; i++){
			TdNETBaseClientFrame* client = (TdNETBaseClientFrame*)m_ClientList.At(i);
			tsWriteSendTraffic(pack.get_pack_size());
			if(!client->SendPack(pack, autoThread))
				result = false;
		}
		m_ClientLock.UnLock();
		return result;
	}
	bool SendPack(const dNETPACK& pack, bool autoThread = true, bool autoSwitchClient = false){
		if(!m_ClientCreated)
			return false;
		bool result = false;
		
		m_ClientLock.Lock();

		if(autoSwitchClient){
			m_ClientLast++;
			if(m_ClientLast < 0 || m_ClientLast >= m_ClientCount)
				m_ClientLast = 0;
		}
			//SwitchClient();
		TdNETBaseClientFrame* client = (TdNETBaseClientFrame*)m_ClientList.At(m_ClientLast);
		if(client){
			if(client->IsInsideConnected()){
				tsWriteSendTraffic(pack.get_pack_size());
				result = client->SendPack(pack, autoThread);
			}
		}

		m_ClientLock.UnLock();
		return result;
	}
	void SwitchClient(){
		m_ClientLast++;
		if(m_ClientLast < 0 || m_ClientLast >= m_ClientCount)
			m_ClientLast = 0;
	}

	bool IsClientCreated()	{return m_ClientCreated;}

protected:
	bool					m_ClientCreated;
	int						m_ClientCount;
	TdListBase				m_ClientList;
	int						m_ClientLast;
	TdMutexObject			m_ClientLock;
	char					m_GroupID[dNORSIZE];

protected:
	virtual TdNETBaseClientFrame*	createSocket() = 0;
	virtual void					releaseSocket(TdNETBaseClientFrame* client) = 0;
};
