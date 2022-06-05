
#include "stdinc.h"
#include "IServerTimer.h"
#include "LibeventServerTimer.h"
#include <chrono>
#include <iomanip>
#include "stopwatch.h"


using namespace std;


class CServer final : public IServerTimerListener
{
public:
	CServer()
	{
		cout << "Hello CMake." << endl;
		pITimer = CreateServerTimer(ServerTimerType_Asio);
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

		unsigned int iTimerID = 99;
		unsigned int iElapse = 1000;
		pITimer->SetTimer(iTimerID, iElapse, true);
		iTimerID = 88;
		pITimer->SetTimer(iTimerID, iElapse, false);

		for (;;)
		{
			std::cout << "waiting for thread done." << std::endl;
			std::this_thread::sleep_for(chrono::seconds{ 1000 });
			break;
		}

		return 0;
	}

protected:
	void OnTimer(unsigned int iTimerID, unsigned int iElapse) final
	{
		auto now = chrono::system_clock::now();
		auto now_time = chrono::system_clock::to_time_t(now);

		auto elapsed = duration_cast<double>(_stopwatch.tick());

		std::cout << "[" << iTimerID << "] OnTimer: " << " "
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
