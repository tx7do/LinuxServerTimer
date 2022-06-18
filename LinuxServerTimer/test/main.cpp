
#include "IServerTimer.h"
#include <chrono>
#include <thread>
#include <iomanip>
#include "stopwatch.h"


using namespace std;


class CServer final : public IServerTimerListener
{
public:
	CServer()
	{
		std::cout << "Main Thread ID:" << std::this_thread::get_id() << std::endl;
		pITimer = CreateServerTimer(ServerTimerType_Epollfd);
		pITimer->RegisterListener(this);
	}
	~CServer() final
	{
		DestroyServerTimer(pITimer);
	}

	int Run()
	{
		_stopwatch.reset();

		std::cout << "begin start timer." << std::endl;
		pITimer->Start();
		std::cout << "finished start timer." << std::endl;

		timerid_t iTimerID = 99;
		elapse_t iElapse = 2000;
		pITimer->SetTimer(iTimerID, iElapse, true);

		std::this_thread::sleep_for(chrono::seconds{ 10 });
		iTimerID = 88;
		pITimer->SetTimer(iTimerID, iElapse, false);

		for (;;)
		{
			std::cout << "main thread waiting for thread done." << std::endl;
			std::this_thread::sleep_for(chrono::seconds{ 1000 });
			break;
		}

		return 0;
	}

protected:
	void OnTimer(timerid_t iTimerID, elapse_t iElapse) final
	{
		auto now = chrono::system_clock::now();
		auto now_time = chrono::system_clock::to_time_t(now);

		auto elapsed = duration_cast<double>(_stopwatch.tick());

		std::cout << "[" << std::this_thread::get_id() << "]"
				  << "[" << iTimerID << "] OnTimer: " << " "
				  << std::put_time(std::localtime(&now_time), "%T ") << " "
				  << setiosflags(ios::fixed) << setprecision(2) << elapsed
				  << " seconds elapsed." << std::endl;
	}

private:
	IServerTimer* pITimer = nullptr;
	StopWatch<> _stopwatch;
};

int main()
{
	CServer app;
	return app.Run();
}
