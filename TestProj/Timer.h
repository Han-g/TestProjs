#pragma once

const ULONG MAX_SAMPLE_COUNT = 50; // 50회의 프레임 처리시간을 누적하여 평균한다.

class CGameTimer
{
public:
	CGameTimer();
	virtual ~CGameTimer();

	void Tick(float fLockFPS = 0.0f);					// 타이머의 시간을 갱신한다.
	void Start();
	void Stop();
	void Reset();

	unsigned long GetFrameRate(LPTSTR lpszString = NULL, int nCharacters = 0);

	float GetTimeElapsed();							// 프레임의 평균 경과 시간을 반환한다.
	float GetTotalTime();								// 프레임 레이트를 반환한다.

private:

	/* Performance Counter */
	/* 카운터 데이터는 시스템 병목 현상을 파악하고 시스템 및 응용프로그램 성능을 세부적으로 조정 */
	/* 운영 체제, 네트워크 및 장치는 응용 프로그램에서 소비 할 수 있는 카운터 데이터를 제공 */
	/* 시스템의 성능을 그래픽으로 보여준다. */

	bool			m_bHardwareHasPerformanceCounter;	// 컴퓨터가 Performance Counter를 갖고 있는가?
	bool			m_bStopped;

	float			m_fTimeScale;						// Scale Counter의 양
	float			m_fTimeElapsed;						// 마지막 프레임 이후 지나간 시간
	float			m_fFPSTimeElapsed;					// 프레임 레이트 계산 소요 시간
	float			m_fFrameTime[MAX_SAMPLE_COUNT];		// 프레임 시간을 누적하기 위한 배열

	__int64			m_nCurrentTime;						// 현재의 시간
	__int64			m_nLastTime;						// 마지막 프레임의 시간
	__int64			m_nBaseTime;
	__int64			m_nPausedTime;
	__int64			m_nStopTime;
	__int64			m_nPerformanceFrequency;			// 컴퓨터의 Performance 빈도

	ULONG			m_nSampleCount;						// 누적된 프레임의 횟수

	unsigned long	m_nCurrentFrameRate;				// 현재의 프레임 레이트
	unsigned long	m_nFramesPerSecond;					// 초당 프레임 수
};

