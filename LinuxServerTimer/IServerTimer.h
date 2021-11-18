

#pragma once

/// <summary>
/// 服务器定时器类型
/// </summary>
enum ServerTimerType
{
	ServerTimerType_Libevent = 1,
	ServerTimerType_Epollfd = 2,
	ServerTimerType_Sleep = 3,
	ServerTimerType_Asio = 4,
};


/// <summary>
/// 服务器定时器监听器接口
/// </summary>
class IServerTimerListener
{
public:
	virtual ~IServerTimerListener() {}

	/// <summary>
	/// 定时器回调
	/// </summary>
	virtual void OnTimer(unsigned int iTimerID, unsigned int iElapse) = 0;
};


/// <summary>
///	服务器定时器接口
/// </summary>
class IServerTimer
{
public:
	virtual ~IServerTimer() {}

	/// <summary>
	///	获取服务器定时器的类型
	/// </summary>
	/// <returns>请查看 ServerTimerType</returns>
	virtual ServerTimerType GetType() const = 0;

	/// <summary>
	///  注册服务器定时器监听器
	/// </summary>
	/// <param name="pListener">服务器定时器监听器指针</param>
	virtual void RegisterListener(IServerTimerListener* pListener) = 0;

	/// <summary>
	/// 启动服务器定时器
	/// </summary>
	virtual void Start() = 0;

	/// <summary>
	/// 停止服务器定时器
	/// </summary>
	virtual void Stop() = 0;

	/// <summary>
	/// 设置一个定时器
	/// </summary>
	/// <param name="iTimerID">定时器ID</param>
	/// <param name="iElapse">定时时间,毫秒单位,大于100ms</param>
	/// <param name="bShootOnce">是否只响应一次,true为响应一次,false为无限次数响应</param>
	virtual void SetTimer(unsigned int iTimerID, unsigned int iElapse, bool bShootOnce = true) = 0;

	/// <summary>
	/// 杀掉一个定时器
	/// </summary>
	/// <param name="iTimerID">定时器ID</param>
	virtual void KillTimer(unsigned int iTimerID) = 0;

	/// <summary>
	/// 杀掉所有的定时器
	/// </summary>
	virtual void KillAllTimer() = 0;
};

/// <summary>
/// 创建服务器定时器实例
/// </summary>
/// <param name="type">定时器类型</param>
/// <returns>销毁定时器实例的接口指针</returns>
extern IServerTimer* CreateServerTimer(ServerTimerType type);

/// <summary>
/// 销毁定时器实例
/// </summary>
/// <param name="pIServerTimer">定时器实例的接口指针</param>
extern void DestoryServerTimer(IServerTimer*& pIServerTimer);
