

#pragma once

/// <summary>
/// ��������ʱ������
/// </summary>
enum ServerTimerType
{
	ServerTimerType_Libevent = 1,
	ServerTimerType_Epollfd = 2,
	ServerTimerType_Sleep = 3,
	ServerTimerType_Asio = 4,
};


/// <summary>
/// ��������ʱ���������ӿ�
/// </summary>
class IServerTimerListener
{
public:
	virtual ~IServerTimerListener() {}

	/// <summary>
	/// ��ʱ���ص�
	/// </summary>
	virtual void OnTimer(unsigned int iTimerID, unsigned int iElapse) = 0;
};


/// <summary>
///	��������ʱ���ӿ�
/// </summary>
class IServerTimer
{
public:
	virtual ~IServerTimer() {}

	/// <summary>
	///	��ȡ��������ʱ��������
	/// </summary>
	/// <returns>��鿴 ServerTimerType</returns>
	virtual ServerTimerType GetType() const = 0;

	/// <summary>
	///  ע���������ʱ��������
	/// </summary>
	/// <param name="pListener">��������ʱ��������ָ��</param>
	virtual void RegisterListener(IServerTimerListener* pListener) = 0;

	/// <summary>
	/// ������������ʱ��
	/// </summary>
	virtual void Start() = 0;

	/// <summary>
	/// ֹͣ��������ʱ��
	/// </summary>
	virtual void Stop() = 0;

	/// <summary>
	/// ����һ����ʱ��
	/// </summary>
	/// <param name="iTimerID">��ʱ��ID</param>
	/// <param name="iElapse">��ʱʱ��,���뵥λ,����100ms</param>
	/// <param name="bShootOnce">�Ƿ�ֻ��Ӧһ��,trueΪ��Ӧһ��,falseΪ���޴�����Ӧ</param>
	virtual void SetTimer(unsigned int iTimerID, unsigned int iElapse, bool bShootOnce = true) = 0;

	/// <summary>
	/// ɱ��һ����ʱ��
	/// </summary>
	/// <param name="iTimerID">��ʱ��ID</param>
	virtual void KillTimer(unsigned int iTimerID) = 0;

	/// <summary>
	/// ɱ�����еĶ�ʱ��
	/// </summary>
	virtual void KillAllTimer() = 0;
};

/// <summary>
/// ������������ʱ��ʵ��
/// </summary>
/// <param name="type">��ʱ������</param>
/// <returns>���ٶ�ʱ��ʵ���Ľӿ�ָ��</returns>
extern IServerTimer* CreateServerTimer(ServerTimerType type);

/// <summary>
/// ���ٶ�ʱ��ʵ��
/// </summary>
/// <param name="pIServerTimer">��ʱ��ʵ���Ľӿ�ָ��</param>
extern void DestoryServerTimer(IServerTimer*& pIServerTimer);
