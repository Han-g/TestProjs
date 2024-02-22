#pragma once
#include "Timer.h"

class CScene;
class CGameTimer;
class CGameObject;
class CPlayer;
class CCamera;

class CGameFramework
{
private:

	/* 핸들 관련 변수 */
	HINSTANCE					m_hInstance;
	HWND						m_hWnd;

	/* 클라이언트 사이즈 변수 */
	int							m_nWndClientWidth;
	int							m_nWndClientHeight;

	/* 인터페이스 포인터 관련 변수 */
	IDXGIFactory4*				m_pdxgiFactory;		// DXGI 팩토리 인터페이스에 대한 포인터
	IDXGISwapChain3*			m_pdxgiSwapChain;	// 스왑 체인 인터페이스에 대한 포인터이다. 주로 디스플레이를 제어하기 위해 필요
	ID3D12Device*				m_pd3dDevice;		// Direct3D 디바이스 인터페이스에 대한 포인터이다. 주로 리소스를 생성하기 위하여 필요하다.

	/* 다중샘플링 관련 변수 */
/* MSAA 다중 샘플링을 활성화하고 다중 샘플링 레벨을 설정 */

/* 샘플링이란 ? 해상도에 비해 너비 , 높이를 x배 씩 해준 도화지에서 그리고 */
/* 클라이언트 영역에 렌더링 해줄 때 x = 2 기준 4배로 늘어난 Pixel의 평균 색상을 구하여 */
/* 하나로 만들어주는 안티앨리어싱 기법 */
	bool						m_bMsaa4xEnable = false;	// 샘플링 상태
	UINT						m_nMsaa4xQualityLevels = 0;	// 샘플링 품질 레벨 0 이면 사용 안함 

	/* 스왑 체인 후면 버퍼 관련 변수 */

	static const UINT			m_nSwapChainBuffers = 2;	// 스왑 체인의 후면 버퍼의 개수
	UINT						m_nSwapChainBufferIndex;	// 스왐 체인의 후면 버퍼 인덱스 

	/* 렌더 타겟 버퍼 , 서술자 인터페이스 관련 변수 */

/* 서술자( Descriptor ) : 리소스에 대해 설명하기 위한 것 */
/* DescriptorHeap : 서술자 배열 */
	ID3D12Resource*				m_ppd3dRenderTargetBuffers[m_nSwapChainBuffers];	// 렌더 타겟 버퍼
	ID3D12DescriptorHeap*		m_pd3dRtvDescriptorHeap;							// 서술자 힙 인터페이스 포인터
	UINT						m_nRtvDescriptorIncrementSize;						// 렌더 타겟 서술자 원소의 크기

	/* 깊이-스텐실 버퍼관련 변수 */

	ID3D12Resource*				m_pd3dDepthStencilBuffer;		// 깊이 - 스텐실 버퍼
	ID3D12DescriptorHeap*		m_pd3dDsvDescriptorHeap;		// 서술자 힙 인터페이스 포인터
	UINT						m_nDsvDescriptorIncrementSize;	// 깊이 - 스텐실 서술자 원소의 크기

	/* 명령 큐 , 명령 할당자 , 명령 리스트 인터페이스 관련 변수 */

/* CommandQueue & List : GPU는 1개의 명령 Queue를 갖고있고 CPU가 CommandList를 넣어주면 GPU는 1프레임마다 순서대로 실행한다. */
/* Allocator : 명령 할당자 - 명령 리스트의 메모리를 할당 받는다. */
	ID3D12CommandQueue*			m_pd3dCommandQueue;		// 명령 큐 인터페이스 포인터
	ID3D12CommandAllocator*		m_pd3dCommandAllocator;	// 명령 할당자 인터페이스 포인터
	ID3D12GraphicsCommandList*	m_pd3dCommandList;		// 명령 리스트 인터페이스 포인터

	/* Fence 관련 변수 */

/* Fence( 울타리 ) : CPU와 GPU의 동기화를 해주기 위한 울타리 역할을 한다. */
/* Handle : Physical 메모리의 주소를 나타내는 포인터의 개념 */
	ID3D12Fence*				m_pd3dFence;	// 인터페이스 포인터 
	UINT64						m_nFenceValue;	// 펜스의 값 ( 초기 값 )
	HANDLE						m_hFenceEvent;	// 이벤트 핸들

	/* 후면 버퍼마다 현재의 펜스값을 관리하기 위한 멤버 변수 */
	UINT64						m_nFenceValues[m_nSwapChainBuffers];

	CScene* m_pScene;
	CCamera* m_pCamera = NULL;

#if defined(_DEBUG)
	/* Debug Layer Interfaces */
	ID3D12Debug* m_pd3dDebugController;
#endif

	/* 렌더링 파이프라인 관련 변수 */

/* 렌더링 파이프라인 :
	GPU를 사용하여 리소스 ( 정점 / 인덱스 , 텍스쳐 ) 를 2D 이미지로 렌더링 하는 과정 */
	/* 정확히는 모니터에 출력 전까지의 과정 */
	ID3D12PipelineState* m_pd3dPipelineState; // 파이프 라인의 상태를 나타내는 변수 

	CGameTimer					m_GameTimer;
	_TCHAR						m_pszFrameRate[50];

	//플레이어 객체에 대한 포인터이다.
	CPlayer* m_pPlayer = NULL;

	//마지막으로 마우스 버튼을 클릭할 때의 마우스 커서의 위치이다. 
	POINT m_ptOldCursorPos;

public:

	CGameFramework();
	~CGameFramework();

	/* 프레임 워크 관련 함수 */
	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);	// 프레임 워크를 초기화 하는 함수 ( 주 윈도우가 생성되면 호출 )

	void OnDestroy();									// 윈도우가 닫힐 때 호출 되는 함수 

	/* 스왑체인 , 디바이스 , 서술자 힙 , 명령 큐 / 할당자 / 리스트를 생성하는 Create 함수 */

	void CreateSwapChain();					// 스왑 체인 생성
	void CreateDirect3DDevice();			// Direct3DDevice 인터페이스 생성 함수 
	// Rtv : Render Target View   
	// Dsv : Depth Stencil View 
	void CreateRtvAndDsvDescriptorHeaps();
	void CreateCommandQueueAndList();		// CommandQueue & List 인터페이스를 생성하는 함수
	void CreateRenderTargetView();			// 랜더 타겟 뷰 서술자를 생성하는 함수 
	void CreateDepthStencilView();			// 깊이 - 스텐실 버퍼와 서술자를 생성하는 함수

	/* 렌더링할 메쉬와 객체 관련 함수 */

	void BuildObjects();	// 렌더링할 객체를 생성하는 함수
	void ReleaseObjects();	// 렌더링할 객체를 소멸시키는 함수

	/* 사용자 입력 , 애니메이션 , 렌더링 관련 함수 */

	void ProcessInput();	// 사용자 입력을 처리하는 함수
	void AnimateObjects();	// 오브젝트들의 움직임을 처리하는 함수
	void FrameAdvance();	// 오브젝트들을 렌더링 해주는 함수

	/* CPU & GPU 동기화 관련 함수 */

	void WaitForGpuComplete();	// GPU가 일을 끝날 때 까지 CPU가 기다리게 하는 함수.

	/* 윈도우의 메시지 ( 키보드 마우스 입력 등 ) 처리 관련 함수 */

/* 마우스 입력을 처리하는 함수 */
	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	/* 키보드 입력을 처리하는 함수 */
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	/* 전체화면 전환 함수 */

	void OnResizeBackBuffers();

	/* 윈도우 입력( 전체화면 전환 등 )을 처리하는 함수 */

	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);


	/* 프레임 관련 함수 */
	void MoveToNextFrame();
};

