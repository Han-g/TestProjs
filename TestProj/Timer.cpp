#include "stdafx.h"
#include "Timer.h"


CGameTimer::CGameTimer()
{
	/* QueryPerformanceFrequency(LARGE_INTERGER* lpFrequency */
	/* lpFrequency : 현재 초당 Performance Counter 빈도를 받는다. */
	/* Performance Counter : 주파수, 시스템 부팅시 고정 되어 있으며 모든 프로그램이 일관하다. */
	/* 따라서 빈도는 응용 프로그램 초기화 시에만 쿼리해야 결과를 얻을 수 있다. */
	if (::QueryPerformanceFrequency((LARGE_INTEGER*)&m_nPerformanceFrequency))
	{
		m_bHardwareHasPerformanceCounter = TRUE;
		::QueryPerformanceCounter((LARGE_INTEGER*)&m_nLastTime);
		m_fTimeScale = 1.0f / m_nPerformanceFrequency;
	}

	else
	{
		m_bHardwareHasPerformanceCounter = FALSE;
		m_nLastTime = ::timeGetTime(); // 윈도우가 시작하고 나서의 경과한 시간을 받는다.
		m_fTimeScale = 0.001f;
	}

	m_nSampleCount = 0;
	m_nCurrentFrameRate = 0;
	m_nFramesPerSecond = 0;
	m_fFPSTimeElapsed = 0.0f;

}


CGameTimer::~CGameTimer()
{
}


/*           Tick           */

/* 컴퓨터가 성능 카운터 하드웨어를 가지고 있으면 카운터와 주파수를 이용하여 현재 시간을 갱신, */
/* 없으면 멀티미디어 타이머를 이용한다. */

void CGameTimer::Tick(float fLockFPS)
{

	if (m_bHardwareHasPerformanceCounter)
		::QueryPerformanceCounter((LARGE_INTEGER*)&m_nCurrentTime);	// 카운터와 주파수를 이용하여 현재 시간을 갱신

	else
		m_nCurrentTime = ::timeGetTime();

	/* 마지막으로 이 함수를 호출한 이후 경과한 시간을 계산한다. */
	/* 현재 시간 - 마지막 프레임의 시간 * ? */
	float fTimeElapsed = (m_nCurrentTime - m_nLastTime) * m_fTimeScale;

	if (fLockFPS > 0.0f)
	{
		/* fLockFPS가 0보다 크면 이 시간만큼 호출한 함수를 기다리게 할거야 : FPS를 맞추기 위해서 */
		while (fTimeElapsed < (1.0f / fLockFPS))
		{
			if (m_bHardwareHasPerformanceCounter)
				::QueryPerformanceCounter((LARGE_INTEGER*)&m_nCurrentTime);

			else
				m_nCurrentTime = ::timeGetTime();

			fTimeElapsed = (m_nCurrentTime - m_nLastTime) * m_fTimeScale;
		}

	}

	/* 다음에 쓰기위해 프레임 레이트 처리가 끝난 시간을 저장해 둘거야. */
	m_nLastTime = m_nCurrentTime;

	if (fabsf(fTimeElapsed - m_fTimeElapsed) < 1.0f)
	{
		/* memmove( void* dest, const void* src, size_t n*/
		/* dest : 복제될 메모리 영역 */
		/* src  : 복제할 메모리 영역 */
		/* n    : 복제할 데이터의 바이트 수 */

		::memmove(&m_fFrameTime[1], m_fFrameTime, (MAX_SAMPLE_COUNT - 1) * sizeof(float)); // 0 ~ 49 -> 1 ~ 50
		m_fFrameTime[0] = fTimeElapsed;	// 0에 삽입

		if (m_nSampleCount < MAX_SAMPLE_COUNT)
			m_nSampleCount++;	// 누적된 프레임의 수가 최대 프레임 보다 적으면 ++
	}

	/* 초당 프레임 수를 1증가 시키고 현재 프레임 처리 시간을 누적하여 저장한다. */

	m_nFramesPerSecond++;
	m_fFPSTimeElapsed += fTimeElapsed;

	if (m_fFPSTimeElapsed > 1.0f)	// 1초가 지날 경우
	{
		m_nCurrentFrameRate = m_nFramesPerSecond;
		m_nFramesPerSecond = 0;
		m_fFPSTimeElapsed = 0.0f;
	}

	/* 누적된 프레임 처리 시간의 평균을 구하여 프레임 처리 시간을 구한다. */

	m_fTimeElapsed = 0.0f;

	for (ULONG i = 0; i < m_nSampleCount; i++)
		m_fTimeElapsed += m_fFrameTime[i];

	if (m_nSampleCount > 0)
		m_fTimeElapsed /= m_nSampleCount;

}


/*           GetFrameRate           */
/* 마지막 프레임 레이트를 문자열과 정수형으로 반환 */

unsigned long CGameTimer::GetFrameRate(LPTSTR lpszString, int nCharacters)
{

	if (lpszString)
	{
		/* _itow_s(int value, wchar_t* buffer, size_t sizeInCharacters, int radix */
		/* value  : 변환할 숫자 */
		/* buffer : 변환의 결과로 채워질 곳 */
		/* sizeInCharacters : 문자 버퍼의 크기 */
		/* radix : 기본 value, 2 ~ 36범위 */
		/* 정수형을 문자형으로 변환 해주는 함수 */
		_itow_s(m_nCurrentFrameRate, lpszString, nCharacters, 10);

		/* wcscat_s(char* strDestination, size_t numberOfElements, const wchar_t* strSource) */
		/* strDestination : null로 끝나는 문자열 버퍼 */
		/* numberOfElements : 대상 문자열 버퍼의 크기 */
		/* strSource : null 종료 소스 문자열 버퍼 */
		/* 문자열을 더 하는 함수 */
		wcscat_s(lpszString, nCharacters, _T(" FPS)"));
	}

	return(m_nCurrentFrameRate);

}

float CGameTimer::GetTimeElapsed()
{

	return(m_fTimeElapsed);

}

void CGameTimer::Reset()
{
	__int64 nPerformanceCounter;
	::QueryPerformanceCounter((LARGE_INTEGER*)&nPerformanceCounter);

	/* 기존 , 마지막 시간을 현재 시각으로 초기화 */
	m_nBaseTime = nPerformanceCounter;
	m_nLastTime = nPerformanceCounter;
	m_nStopTime = 0;
	m_bStopped = false;

}

void CGameTimer::Start()
{

	__int64 nPerformanceCounter;
	::QueryPerformanceCounter((LARGE_INTEGER*)&nPerformanceCounter);

	if (m_bStopped)
	{
		m_nPausedTime += (nPerformanceCounter - m_nStopTime);
		m_nLastTime = nPerformanceCounter;
		m_nStopTime = 0;
		m_bStopped = false;
	}

}

void CGameTimer::Stop()
{

	if (!m_bStopped)
	{
		::QueryPerformanceCounter((LARGE_INTEGER*)&m_nStopTime);
		m_bStopped = true;
	}

}

/* 게임이 실행된 총 시간을 얻는 함수 */

float CGameTimer::GetTotalTime()
{

	if (m_bStopped)
		return(float(((m_nStopTime - m_nPausedTime) - m_nBaseTime) * m_fTimeScale));

	return (float(((m_nCurrentTime - m_nPausedTime) - m_nBaseTime) * m_fTimeScale));

}

