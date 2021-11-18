
#include "stdinc.h"
#include "IServerTimer.h"
#include "LibeventServerTimer.h"


using namespace std;

class CServer : public IServerTimerListener
{
public:
	CServer()
	{
		cout << "Hello CMake." << endl;

		evutil_gettimeofday(&g_lasttime, nullptr);

		pITimer = CreateServerTimer(ServerTimerType_Asio);
		pITimer->RegisterListener(this);
	}
	~CServer()
	{
		DestoryServerTimer(pITimer);
	}

	int Run()
	{
		pITimer->Start();

		unsigned int iTimerID = 99;
		unsigned int iElapse = 1000;
		pITimer->SetTimer(iTimerID, iElapse);
		iTimerID = 88;
		pITimer->SetTimer(iTimerID, iElapse, false);
		
		while (1)
		{
			usleep(1000000);
		}

		return 0;
	}

protected:
	virtual void OnTimer(unsigned int iTimerID, unsigned int iElapse)
	{
		struct timeval nowTime, difference;
		double elapsed;
		evutil_gettimeofday(&nowTime, nullptr);
		evutil_timersub(&nowTime, &g_lasttime, &difference);
		elapsed = difference.tv_sec + (difference.tv_usec / 1.0e6);

		struct tm* info = gmtime(&nowTime.tv_sec);

		printf("[%d] called at %2d:%02d:%02d: %.3f seconds elapsed.\n",
			iTimerID, 
			(info->tm_hour + 8) % 24, info->tm_min, info->tm_sec,
			elapsed);

		g_lasttime = nowTime;
	}

private:
	IServerTimer* pITimer;
	struct timeval g_lasttime;
};


int main()
{
	CServer app;
	return app.Run();
}
