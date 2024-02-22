#include "stdafx.h"

#include "GameFramework.h"
#include "Scene.h"
#include "Player.h"


CGameFramework::CGameFramework()	// O
{

	m_pdxgiFactory = NULL;
	m_pdxgiSwapChain = NULL;
	m_pd3dDevice = NULL;

	m_pd3dCommandAllocator = NULL;
	m_pd3dCommandQueue = NULL;
	m_pd3dPipelineState = NULL;
	m_pd3dCommandList = NULL;

	for (int i = 0; i < m_nSwapChainBuffers; ++i)
		m_ppd3dRenderTargetBuffers[i] = NULL;

	m_pd3dRtvDescriptorHeap = NULL;
	m_nRtvDescriptorIncrementSize = 0;

	m_pd3dDepthStencilBuffer = NULL;
	m_pd3dDsvDescriptorHeap = NULL;
	m_nDsvDescriptorIncrementSize = 0;

	m_nSwapChainBufferIndex = 0;

	m_hFenceEvent = NULL;
	m_pd3dFence = NULL;
	m_nFenceValue = 0;

	m_nWndClientWidth = FRAME_BUFFER_WIDTH;
	m_nWndClientHeight = FRAME_BUFFER_HEIGHT;

	_tcscpy_s(m_pszFrameRate, _T("LapProject ("));

	/*		따라하기 6번		 */
	for (int i = 0; i < m_nSwapChainBuffers; i++)
		m_nFenceValues[i] = 0;
	m_pScene = NULL;

}


CGameFramework::~CGameFramework()
{
}


/* 프레임 워크 관련 함수 */

/*   프레임 워크를 초기화 하는 함수 ( 주 윈도우가 생성되면 호출 )   */
bool CGameFramework::OnCreate(HINSTANCE hInstance, HWND hMainWnd)	// O
{

	m_hInstance = hInstance;
	m_hWnd = hMainWnd;

	/* Direct3D 디바이스 , 명령 Queue , 명령 List , 스왑체인 등을 생성하는 함수를 호출한다. */
	CreateDirect3DDevice();
	CreateCommandQueueAndList();
	CreateRtvAndDsvDescriptorHeaps();
	CreateSwapChain();

	/* Create Event ( lpEventAttributes , bManualReset , bInitialState , lpName ) : */
	/* lpEventAttributes : NULL - 생성된 이벤트 핸들은 자식 프로세스에 상속 X */
	/* bManualReset : FALSE - 자동으로 이벤트가 해제되는 이벤트 객체를 생성 */
	/* bInitialState : FALSE - 이벤트가 발생하지 않은 채로 생성 */
	/* IpName : 이벤트 객체에 사용할 이름 포인터 */
	/* return : 성공시 - 이벤트 객체의 핸들을 반환 , 실패시 - NULL 반환 */
	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	/* 렌더링할 객체( 게임 월드 객체)를 생성한다. */
	BuildObjects();

	return true;


}


void CGameFramework::OnDestroy()
{

	ReleaseObjects();				// 객체를 없앤다.

	::CloseHandle(m_hFenceEvent);	// 할당 받은 이벤트 핸들을 풀어준다.

#if defined(_DEBUG)
	if (m_pd3dDebugController) m_pd3dDebugController->Release();
#endif

	for (int i = 0; i < m_nSwapChainBuffers; ++i)
		if (m_ppd3dRenderTargetBuffers[i])
			m_ppd3dRenderTargetBuffers[i]->Release();
	if (m_pd3dRtvDescriptorHeap)	m_pd3dRtvDescriptorHeap->Release();

	if (m_pd3dDepthStencilBuffer)	m_pd3dDepthStencilBuffer->Release();
	if (m_pd3dDsvDescriptorHeap)	m_pd3dDsvDescriptorHeap->Release();

	if (m_pd3dCommandAllocator)		m_pd3dCommandAllocator->Release();
	if (m_pd3dCommandQueue)			m_pd3dCommandQueue->Release();
	if (m_pd3dCommandList)			m_pd3dCommandList->Release();

	if (m_pd3dFence)				m_pd3dFence->Release();

	m_pdxgiSwapChain->SetFullscreenState(FALSE, NULL);	// Window모드로 바꿔준다. 
	if (m_pdxgiSwapChain)			m_pdxgiSwapChain->Release();
	if (m_pd3dDevice)				m_pd3dDevice->Release();
	if (m_pdxgiFactory)				m_pdxgiFactory->Release();

}


/* 스왑체인 , 디바이스 , 서술자 힙 , 명령 큐 / 할당자 / 리스트를 생성하는 Create 함수 */

/*       스왑 체인 생성하는 함수       */
void CGameFramework::CreateSwapChain()						// O
{

	RECT rcClient;
	::GetClientRect(m_hWnd, &rcClient);
	m_nWndClientWidth = rcClient.right - rcClient.left;
	m_nWndClientHeight = rcClient.bottom - rcClient.top;

	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(dxgiSwapChainDesc));

	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	dxgiSwapChainDesc.BufferDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.BufferDesc.Height = m_nWndClientHeight;
	dxgiSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiSwapChainDesc.OutputWindow = m_hWnd;
	dxgiSwapChainDesc.SampleDesc.Count = 1;
	dxgiSwapChainDesc.SampleDesc.Quality = 0;
	dxgiSwapChainDesc.Windowed = TRUE;

#ifdef _WITH_ONLY_RESIZE_BACKBUFFES
	// 전체화면 모드에서 바탕화면 해상도를 ㅏㅂ꾸지 않고 후면 버퍼의 크기를 바탕화면 크기로 변경
	dxgiSwapChainDesc.Flags = 0;
#else
	// 전체체화면 모드에서 바탕화면의 해상도를 스왑체인의 크기에 맞게 변경
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
#endif

	HRESULT hResult = m_pdxgiFactory->CreateSwapChain(m_pd3dCommandQueue, &dxgiSwapChainDesc,
		(IDXGISwapChain**)&m_pdxgiSwapChain);

	m_pdxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);		// ALT + ENTER로 전체화면 전환하지 않을 꺼야
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();	// 현재 스왑체인의 후면 버퍼의 인덱스를 얻어온다..
}

/*   Direct3DDevice 인터페이스 생성 함수   */
void CGameFramework::CreateDirect3DDevice()		 // O 		
{

#if defined(_DEBUG)
	D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&m_pd3dDebugController);
	//m_pd3dDebugController->EnableDebugLayer();
#endif

	/*                     DXGI 팩토리를 생성한다.                     */

	/* CreateDXGIFactory1( REFIID riid , void** ppFactory ) */
	/* REFIID : ppFactory 매개 변수에서 참조하는 DXGIFactory객체의 GUID */
	/* void** : IDXGIFactory객체에 대한 포인트의 주소값 => [Out] 값이 저장되는 곳 */
	/* GUID : 인터페이스 클래스 식별자(ID) 를 나타내는 128-비트 정수 문자열 */
	/* __uuidof : 인터페이스 자료형 , 클래스 이름 , 인터페이스 포인터를 반환해주는 연산자 */
	/* IDXGIFactory : DXGI를 사용할 각종 인터페이스를 얻어오는 인터페이스이다. 그래서 반환 값에 **이 들어가 있다. */
	::CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&m_pdxgiFactory);


	/*     모든 하드웨어 어댑터에 대하여 특성 레벨 12.0을 지원하는 하드웨어 디바이스를 생성한다.     */

	IDXGIAdapter1* pd3dAdapter = NULL;

	/* EnumAdapters1( UINT Adapter , IDXGIAdapter1 **ppAdapter ) */
	/* UINT : 순서화한 Adapter의 인덱스 */
	/* IDXGIAdapter1 : [Out] UINT번째의 Adatper 포인터를 얻어올 IDXGIAdapter1 인터페이스의 포인터 주소 값 . 절대 NULL 사용 불가 */
	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != m_pdxgiFactory->EnumAdapters1(i, &pd3dAdapter); ++i)
	{
		/* DXGI_ADAPTER_DESC1 : DXGI를 사용할 Video Card 서술자*/
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;

		/* GetDesc1 : 어댑터의 DXGI 1.1 서술자를 얻어온다. */
		pd3dAdapter->GetDesc1(&dxgiAdapterDesc);

		/* & : 비트 연산자 and */
		/* DXGI_ADPATER_FLAG_SOFTWARE : 그래픽 카드가 아닌 운영체제에서 기본적으로 CPU를 통해 제공하는 렌더링 어댑터 */
		if (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)	// dxgiAdapterDesc.Flags에 DXGI_ADPATER_FLAG_SOFTWARE 비트가 포함되어 있어? 
			continue;

		/* CreateDevice( IUnknown* pAdapter , D3D_FEATURE_LEVEL MinimumFeaturelevel , const IID& riid , void** ppDevice ) */
		/* IUnknown : 디바이스를 만들 때 쓸 비디오 어댑터 포인터이다. NULL사용시 기본 Adapter 사용 */
		/* D3D_FEATURE_LEVEL : Adapter 포인터가 지원해야 하는 최소 FEATURE LEVEL 값이다. DirectX 12.0을 이용할 것이기에 12로 설정 */
		/* REFIID : 디바이스 인터페이스를 위한 GUID ( Globally Unique IDentifier ) */
		/* void** : [Out] 반환되는 Device 포인터를 받기위한 메모리 블럭 */
		/* Adapter가 DirectX3D를 지원하고 ID3D12Device를 성공적으로 만들어 m_pd3dDevice에 저장했다면 for문에서 나와줘  */
		/* Device : ? */
		if (SUCCEEDED(D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_12_0,
			__uuidof(ID3D12Device), (void**)&m_pd3dDevice)))
			break;
	}

	/*          특성 레벨 12.0을 지원하는 하드웨어 디바이스가 없으면 WARP 디바이스를 생성한다.          */
	/* WARP Device : 하드웨어적으로 D3D를 구현한 것은 아니지만 운영체제에서 Adapter를 Emulater 하고 그려준다. */

	if (!pd3dAdapter)
	{
		/* EnumWarpAdapter(const RFIID& riid , void** ppAdapter) */
		/* RIID : ppAdapter에서 참조할 IDXGIFactory4 객체의 GUID */
		/* ppAdapter : [Out] Adapter의 인터페이스 포인터의 주소값 . 절대 NULL사용 불가 */
		m_pdxgiFactory->EnumWarpAdapter(__uuidof(IDXGIFactory4), (void**)&pd3dAdapter);
		D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&m_pd3dDevice);
	}

	/*          디바이스가 지원하는 다중 샘플의 품질 수준을 확인          */

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS d3dMsaaQualityLevels;
	d3dMsaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dMsaaQualityLevels.SampleCount = 4;

	/* D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS : 품질 수준을 결정하기 위한 옵션을 지정한다. */
	/* D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE : 아무 옵션을 지정하지 않겠다. */
	/* D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_TILED_RESOURCE : 타일링된 리소스의 품질 수치를 정할 수 있다. */
	d3dMsaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	d3dMsaaQualityLevels.NumQualityLevels = 0;						// QualityLevel을 0으로 초기화

	/* CheckFeatureSupport(D3D_FEATURE feature, void* pFeatureSupportData, UINT FeatureSupportDataSize) */
	/* D3D_FEATURE : 지원을 요구하는 기능의 서술자 */
	/* void* : [In, Out] 기능 요구 서술자로 채워질 구조체 */
	/* UNIT : pFeatureSupportData의 Size */
	/* 현재 그래픽 카드에서 지원하는 기능들에 대한 정보를 얻어줘 */
	m_pd3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&d3dMsaaQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	m_nMsaa4xQualityLevels = d3dMsaaQualityLevels.NumQualityLevels;	// 그래픽 카드가 지원하는 다중 샘플 품질 수준을 확인한다.
	// 여기서는 창을 띄우는게 목표이기 때문에 0으로 설정 한?
	m_bMsaa4xEnable = (m_nMsaa4xQualityLevels > 1) ? true : false;

	/*          펜스를 생성하고 펜스 값을 1로 설정한다.          */

	/* CreateFence(UINT64 InitialValue, D3D_FENCE_FLAGS Flags, const IID& riid, void** ppFence) */
	/* UINT64 : Fence의 초기값 */
	/* Flags : 비트 OR 연산자를 사용하여 결합한 Fence의 옵션값 */
	/* RIID (Reference Interface ID) : ID3D12Fence 인터페이스의 GUID */
	/* void** : fence에 접근 하기위해 사용 할 ID3D12Fence 인터페이스의 포인터를 받기위한 메모리 블럭 포인터 */
	m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_pd3dFence);
	m_nFenceValue = 1;

	if (pd3dAdapter) pd3dAdapter->Release();

}

/* CommandQueue & List : GPU는 1개의 명령 Queue를 갖고있고 CPU가 CommandList를 넣어주면 GPU는 1프레임마다 순서대로 실행한다. */

/* CommandQueue & List 인터페이스를 생성하는 함수 */
void CGameFramework::CreateCommandQueueAndList()	// O
{
	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc;
	::ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	/* D3D12_COMMAND_QUEUE_FLAGS : GPU가 일하는 시간이 너무 늦어( Time Out )! Queue를 비우고 다음 Frame의 명령 List를 채울꺼야? */
	/* D3D12_COMMAND_QUEUE_FLAG_NONE : Time Out 이면 Queue를 비우고 다음 Frame의 명령 List를 채울꺼야 */
	/* D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT : 아니 GPU가 일을 끝낼때 까지 기다릴거야 */
	d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	/* D3D12_COMMAND_LIST_TYPE_DIRECT : GPU가 직접 사용 가능한 Queue를 만들거야 */
	d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	/* HRESULT : 함수의 결과를 나타내는 32비트 값 */
	/* Command Queue : GPU가 갖고 있는 1개 이상의 CommandList의 집합 */
	HRESULT hResult = m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc,	// 직접 (Direct) 명령 Queue를 생성한다.
		__uuidof(ID3D12CommandQueue), (void**)&m_pd3dCommandQueue);

	/* Command Allocator : 명령 할당자 , GPU 명령을 저장하기 위한 메모리를 할당한다. 디바이스는 명령 할당자를 통하여 명령 List를 생성한다. */
	hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		__uuidof(ID3D12CommandAllocator), (void**)&m_pd3dCommandAllocator);

	/* Command List : GPU가 실행할 명령들의 순서화된 집합, 생성될 때 Open상태로 생성된다. Open상태일 때에만 명령을 집어 넣을 수 있다. */
	/* 1개의 Allocator 에서 2개 이상의 List가 Open될 수 없다. */
	/* CrateCommandList(UINT nodeMask, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* pCommandAllocator, */
	/* ID3D12PipelineState* pInitialState, RIID riid, void** ppCommandList */
	/* UINT : 명령어 리스트를 생성할 대상 어댑터 하나의 GPU일 경우 0 */
	/* D3D12_COMMAND_LIST_TYPE : 명령어 리스트의 유형 */
	/* ID3D12CommandAllocator : 명령어 리스트를 생성할 명령 할당자 */
	/* ID3D12PipelineState : 현재 파이프 라인의 상태 , NULL 사용시 라이브러리가 자동으로 상태를 설정 */
	/* RIID : CommandList의 GUID */
	/* void** : [Out] CommandList에 접근하기위해 사용할 ID3DICommandList 인터페이스 포인터를 받기 위한 메모리 블럭 포인터 */
	hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dCommandAllocator, NULL,
		__uuidof(ID3D12CommandList), (void**)&m_pd3dCommandList);

	hResult = m_pd3dCommandList->Close();	// 초기 Open 되어있는 상태이므로 Close 필쑤 ! 

}

/*       RTV , RSV 서술자 힙을 생성하는 함수       */
void CGameFramework::CreateRtvAndDsvDescriptorHeaps()	// O
{
	// Rtv : Render Target View   
	// Dsv : Depth Stencil View 
	// DescriptorHeap : View , Descriptor를 저장하기 위한 연속적인 메모리 ( 배열 ) 
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	::ZeroMemory(&d3dDescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));

	/*       렌더 타겟 서술자 힙( 서술자의 개수는 스왑체인 버퍼의 개수)을 생성한다.       */

	d3dDescriptorHeapDesc.NumDescriptors = m_nSwapChainBuffers;				// 서술자의 개수 = 스왑체인 버퍼의 개수 
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	// RenderTargetView 타입의 Heap 
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// D3D12_DESCRIPTOR_HEAP_FLAG_NONE : CPU야 너가 정해줘
	d3dDescriptorHeapDesc.NodeMask = 0;								// 몇 번째 GPU야? 싱글 GPU = 0

	HRESULT hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc,// 서술자 Heap을 생성한다.
		__uuidof(ID3D12DescriptorHeap), (void**)&m_pd3dRtvDescriptorHeap);

	/*       렌더 타겟 서술자 힙의 원소의 크기를 저장       */

	/* GPU마다 서술자의 크기가 다르다 ! 다음 원소로 넘어가기 위해 힙의 원소 크기(IncrementSize)를 저장(Get) 해야한다. */
	m_nRtvDescriptorIncrementSize =
		m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	/*       깊이-스텐실 서술자 힙( 서술자의 개수는 1 )을 생성한다.       */

	/* 깊이-스텐실은 1개를 초기화 - 사용 - 초기화 - 사용 순으로 사용하기 때문에 1개면 충분하다. */
	d3dDescriptorHeapDesc.NumDescriptors = 1;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc,
		__uuidof(ID3D12DescriptorHeap), (void**)&m_pd3dDsvDescriptorHeap);

	/*       깊이-스텐실 서술자 힙의 원소의 크기를 저장       */

	m_nDsvDescriptorIncrementSize =
		m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);



}

/* 랜더 타겟 뷰 서술자를 생성하는 함수 */
void CGameFramework::CreateRenderTargetView()	// O
{
	HRESULT hResult;

	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle =
		m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	/*       스왑체인의 각 후면 버퍼에 대한 렌더 타겟 뷰를 생성한다.       */

	for (UINT i = 0; i < m_nSwapChainBuffers; ++i)
	{
		hResult = m_pdxgiSwapChain->GetBuffer(i,
			__uuidof(ID3D12Resource), (void**)&m_ppd3dRenderTargetBuffers[i]);
		m_pd3dDevice->CreateRenderTargetView(m_ppd3dRenderTargetBuffers[i], NULL,
			d3dRtvCPUDescriptorHandle);
		d3dRtvCPUDescriptorHandle.ptr += m_nRtvDescriptorIncrementSize;
	}

}

/* 깊이 - 스텐실 버퍼와 서술자를 생성하는 함수 */
void CGameFramework::CreateDepthStencilView()		// X
{

	/*       D3D12_RESOURCE 서술자를 채워준다.       */

	D3D12_RESOURCE_DESC d3dResourceDesc;
	/*	리소스를 2차원 텍스쳐로 사용 할거야 .	*/
	d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3dResourceDesc.Alignment = 0;
	d3dResourceDesc.Width = m_nWndClientWidth;
	d3dResourceDesc.Height = m_nWndClientHeight;
	d3dResourceDesc.DepthOrArraySize = 1;
	d3dResourceDesc.MipLevels = 1;
	d3dResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dResourceDesc.SampleDesc.Count = 1;
	d3dResourceDesc.SampleDesc.Quality = 0;
	d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	/*       */

	D3D12_HEAP_PROPERTIES d3dHeapProperties;
	::ZeroMemory(&d3dHeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	/* Type : 리소스에 CPU , GPU 접근 가능 여부를 설정 */
	d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;						// DEFAULT : GPU만 접근 가능 CPU는 접근 불가
	/* CPUPageProperty : 힙에 대한 CPU-페이지 속성 */
	d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;	// UNKNOWN : 운영체제야 너가 알아서 해줘 ㅎ
	/* HHeap의 리소스에 대한 CPU와 GPU의 대역폭 속성 */
	d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;		// UNKNOWN : 운영체제야 너가 알아서 해줘
	/* CreationNodeMask : GPU 어댑터 Index */
	d3dHeapProperties.CreationNodeMask = 1;									// 0 , 1 : 단일 GPU 어댑터
	/* 다중 어탭터 중에서 리소스를 볼 수 있는 집합을 나타내는 비트 */
	d3dHeapProperties.VisibleNodeMask = 1;

	/*       깊이 - 스텐실 버퍼를 생성한다.       */

	D3D12_CLEAR_VALUE d3dClearValue;
	d3dClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dClearValue.DepthStencil.Depth = 1.0f;
	d3dClearValue.DepthStencil.Stencil = 0;
	/* CreateCommittedResource(const D3D12_HEAP_PROPERTIES* pHeapProperties, D3D12_HEAP_FLAG Heapflags, const D3D12_RESOURCE_DESC* pDesc,
	D3D12_RESOURCE_STATES InitialResourceState, const D3D12_CLEAR_VALUE* pOptimizedClearValue, const IID& riidSource, void** ppvResouce) */

	/* CreateCommittedResource : Heap과 리소스를 생성하는 함수 Heap은 완전한 리소스를 포함할 수 있는 충분한 메모리를 갖고 있어야 한다. */
	/* D3D12_HEAP_PROPERTIES : Heap에 대한 정보 포인터 */
	/* D3D12_HEAP_FLAGS : 비트 OR연산으로 조합된 Heap 옵션 */
	/* D3D12_RESOURCE_DESC : 리소스 서술자 포인터 */
	/* D3D12_RESOURCE_STATE : 비트 OR연산으로 조합된 자원의 초기상태 */
	/* D3D12_CREAR_VALUE : 깊이 - 스텐실 버퍼의 초기값을 설정한다. */
	/* REFIID : Resource 인터페이스의 GUID */
	/* void** : 요구하여 생성된 리소스 객체의 포인터를 받을 메모리 포인터 */
	m_pd3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE,
		&d3dResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &d3dClearValue,
		__uuidof(ID3D12Resource), (void**)&m_pd3dDepthStencilBuffer);

	/*       깊이 - 스텐실 뷰를 생성한다.       */

	/* GetCPUDescriptorHandleForHeapStart : 힙의 시작을 나타내는 CPU 서술자 핸들을 가져온다. */
	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle =
		m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	/* CreateDepthStencilView( ID3D12Resource* pResource, const D3D12_DEPTH_STENCIL_VIEW_DESC* pDesc,
	D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor */
	/* ID3D12Resource : 깊이 - 스텐실을 나타내기 위한 Resource 오브젝트의 포인터 */
	/* D3D12_DEPTH_STENCIL_VIEW_DESC : 깊이 - 스텐실 View 서술자포인터이다. NULL사용시 초기 default서술자 사용 */
	/* D3D12_CPU_DESCRIPTOR_HANDLE : 깊이 - 스텐실 View를 붙잡을 힙의 시작을 나타내는 CPU 서술자 핸들 */
	/* 리소스 데이터에 접근 하기위한 깊이 - 스텐실 View를 생성한다. */

	m_pd3dDevice->CreateDepthStencilView(m_pd3dDepthStencilBuffer, NULL, d3dDsvCPUDescriptorHandle);


}




/* 렌더링할 메쉬와 객체 관련 함수 */

/*   렌더링할 객체를 생성하는 함수   */
void CGameFramework::BuildObjects()
{

	m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);

	m_pScene = new CScene();

	if (m_pScene) m_pScene->BuildObjects(m_pd3dDevice, m_pd3dCommandList);

	CAirplanePlayer* pAirplanePlayer = new CAirplanePlayer(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature());

	m_pPlayer = pAirplanePlayer;
	m_pCamera = m_pPlayer->GetCamera();
	m_pd3dCommandList->Close();

	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	WaitForGpuComplete();

	if (m_pScene) m_pScene->ReleaseUploadBuffers();

	m_GameTimer.Reset();

}

/*   렌더링할 객체를 소멸시키는 함수   */
void CGameFramework::ReleaseObjects()
{
	if (m_pScene) m_pScene->ReleaseObjects();
	if (m_pScene) delete m_pScene;

}


/* 사용자 입력 , 애니메이션 , 렌더링 관련 함수 */

/*    사용자 입력을 처리하는 함수    */
void CGameFramework::ProcessInput()
{
	static UCHAR pKeyBuffer[256];

	DWORD dwDirection = 0;

	/*
		키보드의 상태 정보를 반환한다. 화살표 키(‘→’, ‘←’, ‘↑’, ‘↓’)를 누르면 플레이어를 오른쪽/왼쪽(로컬 x-축), 앞/ 뒤(로컬 z-축)로 이동한다.
		‘Page Up’과 ‘Page Down’ 키를 누르면 플레이어를 위/아래(로컬 y-축)로 이동한다.
	*/
	if (::GetKeyboardState(pKeyBuffer))
	{
		if (pKeyBuffer[VK_UP] & 0xF0) dwDirection |= DIR_FORWARD;
		if (pKeyBuffer[VK_DOWN] & 0xF0) dwDirection |= DIR_BACKWARD;
		if (pKeyBuffer[VK_LEFT] & 0xF0) dwDirection |= DIR_LEFT;
		if (pKeyBuffer[VK_RIGHT] & 0xF0) dwDirection |= DIR_RIGHT;
		if (pKeyBuffer[VK_PRIOR] & 0xF0) dwDirection |= DIR_UP;
		if (pKeyBuffer[VK_NEXT] & 0xF0) dwDirection |= DIR_DOWN;
	}

	float cxDelta = 0.0f, cyDelta = 0.0f;
	POINT ptCursorPos;

	/*
		마우스를 캡쳐했으면 마우스가 얼마만큼 이동하였는 가를 계산한다.
		마우스 왼쪽 또는 오른쪽 버튼이 눌러질 때의 메시지(WM_LBUTTONDOWN, WM_RBUTTONDOWN)를 처리할 때 마우스를 캡쳐하였다.
		그러므로 마우스가 캡쳐된 것은 마우스 버튼이 눌려진 상태를 의미한다.
		마우스 버튼이 눌려진 상태에서 마우스를 좌우 또는 상하로 움직이면 플 레이어를 x-축 또는 y-축으로 회전한다.
	*/
	if (::GetCapture() == m_hWnd) {
		//마우스 커서를 화면에서 없앤다(보이지 않게 한다). 
		::SetCursor(NULL);

		//현재 마우스 커서의 위치를 가져온다. 
		::GetCursorPos(&ptCursorPos);

		//마우스 버튼이 눌린 상태에서 마우스가 움직인 양을 구한다. 
		cxDelta = (float)(ptCursorPos.x - m_ptOldCursorPos.x) / 3.0f;
		cyDelta = (float)(ptCursorPos.y - m_ptOldCursorPos.y) / 3.0f;

		//마우스 커서의 위치를 마우스가 눌려졌던 위치로 설정한다. 
		::SetCursorPos(m_ptOldCursorPos.x, m_ptOldCursorPos.y);
	}

	//마우스 또는 키 입력이 있으면 플레이어를 이동하거나(dwDirection) 회전한다(cxDelta 또는 cyDelta). 
	if ((dwDirection != 0) || (cxDelta != 0.0f) || (cyDelta != 0.0f))
	{
		if (cxDelta || cyDelta)
		{
			/*
				cxDelta는 y-축의 회전을 나타내고 cyDelta는 x-축의 회전을 나타낸다.
				오른쪽 마우스 버튼이 눌려진 경우 cxDelta는 z-축의 회전을 나타낸다.
			*/
			if (pKeyBuffer[VK_RBUTTON] & 0xF0)
				m_pPlayer->Rotate(cyDelta, 0.0f, -cxDelta);

			else
				m_pPlayer->Rotate(cyDelta, cxDelta, 0.0f);
		}
		/*
			플레이어를 dwDirection 방향으로 이동한다(실제로는 속도 벡터를 변경한다).
			이동 거리는 시간에 비례하도록 한다.
			플레이어의 이동 속력은 (50/초)로 가정한다.
		*/
		if (dwDirection) m_pPlayer->Move(dwDirection, 50.0f * m_GameTimer.GetTimeElapsed(), true);
	}

	//플레이어를 실제로 이동하고 카메라를 갱신한다. 중력과 마찰력의 영향을 속도 벡터에 적용한다.
	m_pPlayer->Update(m_GameTimer.GetTimeElapsed());

}

/* 오브젝트들의 움직임을 처리하는 함수 */
void CGameFramework::AnimateObjects()
{
	if (m_pScene) m_pScene->AnimateObjects(m_GameTimer.GetTimeElapsed());
}

/*  오브젝트들을 렌더링 해주는 함수  */
void CGameFramework::FrameAdvance()
{
	m_GameTimer.Tick(0.0f);

	ProcessInput();
	AnimateObjects();

	HRESULT hResult = m_pd3dCommandAllocator->Reset();

	hResult = m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);

	D3D12_RESOURCE_BARRIER d3dResourceBarrier;

	::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
	d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3dResourceBarrier.Transition.pResource = m_ppd3dRenderTargetBuffers[m_nSwapChainBufferIndex];
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	d3dRtvCPUDescriptorHandle.ptr += (m_nSwapChainBufferIndex * m_nRtvDescriptorIncrementSize);

	float pfClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	m_pd3dCommandList->ClearRenderTargetView(d3dRtvCPUDescriptorHandle, pfClearColor/*Colors::Azure*/, 0, NULL);

	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	m_pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
	m_pd3dCommandList->OMSetRenderTargets(1, &d3dRtvCPUDescriptorHandle, TRUE, &d3dDsvCPUDescriptorHandle);

	if (m_pScene)
		m_pScene->Render(m_pd3dCommandList, m_pCamera);

	//3인칭 카메라일 때 플레이어가 항상 보이도록 렌더링한다.
#ifdef _WITH_PLAYER_TOP 
	//렌더 타겟은 그대로 두고 깊이 버퍼를 1.0으로 지우고 플레이어를 렌더링하면 플레이어는 무조건 그려질 것이다. 
	m_pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
#endif 
	//3인칭 카메라일 때 플레이어를 렌더링한다. 
	if (m_pPlayer) m_pPlayer->Render(m_pd3dCommandList, m_pCamera);

	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	hResult = m_pd3dCommandList->Close();

	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	WaitForGpuComplete();

	m_pdxgiSwapChain->Present(0, 0);

	MoveToNextFrame();

	m_GameTimer.GetFrameRate(m_pszFrameRate + 12, 37); ::SetWindowText(m_hWnd, m_pszFrameRate);

}


/* CPU & GPU 동기화 관련 함수 */

/*        GPU가 리소스를 다 사용할 때 까지 CPU가 기다리게 하는 함수.        */
void CGameFramework::WaitForGpuComplete()
{
	UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);
	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}

}


/* 윈도우의 메시지 ( 키보드 마우스 입력 ) 처리 관련 함수 */

/*         마우스 입력을 처리하는 함수        */
void CGameFramework::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{

	switch (nMessageID)
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		//마우스 캡쳐를 하고 현재 마우스 위치를 가져온다. 
		::SetCapture(hWnd);
		::GetCursorPos(&m_ptOldCursorPos);
		break;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		//마우스 캡쳐를 해제한다. 
		::ReleaseCapture();
		break;

	}

}

/*         키보드 입력을 처리하는 함수         */
void CGameFramework::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_ESCAPE:
			::PostQuitMessage(0);
			break;

		case VK_RETURN:
			break;

		case VK_F8:
			break;

		case VK_F9:
		{
			BOOL bFullScreenState = FALSE;

			m_pdxgiSwapChain->GetFullscreenState(&bFullScreenState, NULL);

			if (!bFullScreenState)
			{
				DXGI_MODE_DESC dxgiTargetParameters;

				dxgiTargetParameters.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				dxgiTargetParameters.Width = m_nWndClientWidth;
				dxgiTargetParameters.Height = m_nWndClientHeight;
				dxgiTargetParameters.RefreshRate.Numerator = 60;
				dxgiTargetParameters.RefreshRate.Denominator = 1;
				dxgiTargetParameters.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
				dxgiTargetParameters.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
				m_pdxgiSwapChain->ResizeTarget(&dxgiTargetParameters);
			}
			m_pdxgiSwapChain->SetFullscreenState(!bFullScreenState, NULL);

			OnResizeBackBuffers();

			break;
		}
		/*
		‘F1’ 키를 누르면 1인칭 카메라,
		‘F2’ 키를 누르면 스페이스-쉽 카메라로 변경한다,
		‘F3’ 키를 누르면 3인칭 카메라 로 변경한다.
		*/
		case VK_F1:
		case VK_F2:
		case VK_F3:
			m_pCamera = m_pPlayer->ChangeCamera((DWORD)(wParam - VK_F1 + 1), m_GameTimer.GetTimeElapsed());
			break;

		default:
			break;
		}
		break;

	default:
		break;
	}
}

void CGameFramework::OnResizeBackBuffers()
{
	WaitForGpuComplete();	// 백버퍼는 윈도우 사이즈 , 풀스크린으로 되어있기 떄문에 GPU가 모든 그림을 다 그릴 때 까지 기다린다.

	m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);	// CommandList를 비우고 재사용 할거야

	/* 혹시 랜더타겟 버퍼가 남아있다면 메모리를 해제한다. */
	for (int i = 0; i < m_nSwapChainBuffers; i++)
		if (m_ppd3dRenderTargetBuffers[i])
			m_ppd3dRenderTargetBuffers[i]->Release();

	if (m_pd3dDepthStencilBuffer)
		m_pd3dDepthStencilBuffer->Release();

#ifdef _WITH_ONLY_RESIZE_BACKBUFFERS
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	m_pdxgiSwapChain->GetDesc(&dxgiSwapChainDesc);
	m_pdxgiSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
	m_nSwapChainBufferIndex = 0;
#else
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	m_pdxgiSwapChain->GetDesc(&dxgiSwapChainDesc);
	m_pdxgiSwapChain->ResizeBuffers(m_nSwapChainBuffers, m_nWndClientWidth,
		m_nWndClientHeight, dxgiSwapChainDesc.BufferDesc.Format, dxgiSwapChainDesc.Flags);
#endif

	CreateRenderTargetView();
	CreateDepthStencilView();

	m_pd3dCommandList->Close();

	/* 여기서 만든 커맨드 들을 리스트화 시킨다. */
	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	/* 화면 전환이 이루어져서 다시 그리는걸 기다린다. */
	WaitForGpuComplete();
}


/*  */

LRESULT CALLBACK CGameFramework::OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
		/* 윈도우가 생성 될 떄 , 윈도우 크기가 변경될 떄 호출된다. */
	case WM_SIZE:
		m_nWndClientWidth = LOWORD(lParam);
		m_nWndClientHeight = HIWORD(lParam);

		OnResizeBackBuffers();
		break;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
		OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
		break;
	}

	return(0);

}

/*		따라하기 6번		 */

void CGameFramework::MoveToNextFrame()
{
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();
	UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);
	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}


}