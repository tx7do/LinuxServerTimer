#pragma once


/// 服务器定时器类型
enum ServerTimerType
{
	ServerTimerType_Libevent = 1,// libevent定时器
	ServerTimerType_Epollfd = 2,// epollfd定时器
	ServerTimerType_Sleep = 3,// Sleep定时器
	ServerTimerType_Asio = 4,// Asio定时器
};

/// 服务器定时器监听器接口
class IServerTimerListener
{
public:
	virtual ~IServerTimerListener() = default;

	/// 定时器回调
	/// @param iTimerID 定时器ID
	/// @param iElapse 定时器触发时间
	virtual void OnTimer(unsigned int iTimerID, unsigned int iElapse) = 0;
};

///	服务器定时器接口
class IServerTimer
{
public:
	virtual ~IServerTimer() = default;

	///	获取服务器定时器的类型
	/// @return 服务器定时器的类型
	virtual ServerTimerType GetType() const = 0;

	/// 注册服务器定时器监听器
	/// @param pListener 服务器定时器监听器指针
	virtual void RegisterListener(IServerTimerListener* pListener) = 0;

	/// 启动服务器定时器
	virtual void Start() = 0;

	/// 停止服务器定时器
	virtual void Stop() = 0;

	/// 设置一个定时器
	/// @param[in] iTimerID 定时器ID
	/// @param[in] iElapse 定时器触发时间，毫秒单位，大于100ms
	/// @param[in] bShootOnce 是否只触发一次，true为触发一次,false为无限次触发
	virtual void SetTimer(unsigned int iTimerID, unsigned int iElapse, bool bShootOnce) = 0;

	/// 杀掉一个定时器
	/// @param iTimerID 定时器ID
	virtual void KillTimer(unsigned int iTimerID) = 0;

	/// 杀掉所有的定时器
	virtual void KillAllTimer() = 0;
};

/// 创建服务器定时器实例
/// @param type 定时器类型
/// @return 销毁定时器实例的接口指针
extern IServerTimer* CreateServerTimer(ServerTimerType type);

/// 销毁定时器实例
/// @param pIServerTimer 定时器实例的接口指针
extern void DestroyServerTimer(IServerTimer*& pIServerTimer);
