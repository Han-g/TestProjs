#pragma once


class CCamera;
class CGameObject;
class CPlayer;

//게임 객체의 정보를 셰이더에게 넘겨주기 위한 구조체(상수 버퍼)이다. 
struct CB_GAMEOBJECT_INFO {

	XMFLOAT4X4 m_xmf4x4World;

};

//인스턴스 정보(게임 객체의 월드 변환 행렬과 객체의 색상)를 위한 구조체이다. 
struct VS_VB_INSTANCE {

	XMFLOAT4X4 m_xmf4x4Transform;
	XMFLOAT4 m_xmcColor;

};


//셰이더 소스 코드를 컴파일하고 그래픽스 상태 객체를 생성한다. 
class CShader {

public:
	CShader() {};
	virtual ~CShader();

private:
	int						m_nReferences = 0;

public:
	void AddRef() { m_nReferences++; }
	void Release() { if (--m_nReferences <= 0) delete this; }

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState();
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob** ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob** ppd3dShaderBlob);

	D3D12_SHADER_BYTECODE CompileShaderFromFile(WCHAR* pszFileName, LPCSTR pszShaderName, LPCSTR pszShaderProfile, ID3DBlob** ppd3dShaderBlob);

	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature);
	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseShaderVariables();
	virtual void UpdateShaderVariable(ID3D12GraphicsCommandList* pd3dCommandList, XMFLOAT4X4* pxmf4x4World);
	virtual void OnPrepareRender(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);

protected: //셰이더가 포함하는 게임 객체들의 리스트(배열)이다. 
	ID3D12PipelineState** m_ppd3dPipelineStates = NULL;
	int						m_nPipelineStates = 0;

};

class CPlayerShader : public CShader {

public:
	CPlayerShader();
	virtual ~CPlayerShader();

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob** ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob** ppd3dShaderBlob);
	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature);

};

//“CObjectsShader” 클래스는 게임 객체들을 포함하는 셰이더 객체이다. 
class CObjectsShader : public CShader {

public:
	CObjectsShader() {};
	virtual ~CObjectsShader() {};

	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void AnimateObjects(float fTimeElapsed);
	virtual void ReleaseObjects();

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob** ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob** ppd3dShaderBlob);

	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature);

	virtual void ReleaseUploadBuffers();

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);

protected:
	CGameObject** m_ppObjects = NULL;
	int m_nObjects = 0;

};


/*

	따라하기 10번에서는 객체가 셰이더 객체와 메쉬에 대한 포인터를 갖고 있는 구조이다.
	그리고 Scene객체는 게임 객체들의 배열을 갖고 있으며 Scene을 렌더링 하기 위해
	게임 객체들을 순서대로 렌더링 한다.
	각 게임 객체를 렌더링 하기위해 매번 셰이더 객체를 렌더링하고 메쉬 정보를 설정해야 한다.

	이러한 구조에서는 각 객체를 렌더링 하기위해 Device의 상태를 변경해야 한다.
	씬을 구성하는 갳체가 아주 많은 경우 상태 변경을 위한 시간이 늘어날 수 있으므로 프레임 레이트가 떨어질 수 있다.

*/

/*

	따라하기 11번에서는 셰이더 객체가 게임 객체들의 배열을 가지고 있는 구조를 가지고 있다.
	셰이더 객체가 메쉬에 대한 포인터를 가질 수도 있다.
	그리고 씬 객체는 셰이더 객체들의 배열을 가지고 있으며 씬을 렌더링 하기 위해서 셰이더 객체들을 순서대로 렌더링 한다.

	각 셰이더 객체를 랜더링 하기 위해서 셰이더 정보를 한번만 설정하고 게임 객체들의 배열에 포함된 모든 객체들을 순서대로 렌더링하면 된다.

	이러한 구조에서는 씬을 렌더링 하기 위해 셰이더 정보를 객체의 개수만큼 설정하지 않아도 된다.
	= 디바이스 상태 변화가 적어질 수 있다.

*/

////////////////////////////////////////////////////

class CInstancingShader : public CObjectsShader {

public:
	CInstancingShader();
	virtual ~CInstancingShader();

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob** ppd3dShaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob** ppd3dShaderBlob);

	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12RootSignature* pd3dGraphicsRootSignature);
	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);

	virtual void ReleaseShaderVariables();
	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);

	virtual void ReleaseObjects() {};
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);

protected:
	//인스턴스 정점 버퍼와 정점 버퍼 뷰이다. 
	ID3D12Resource* m_pd3dcbGameObjects = NULL;
	VS_VB_INSTANCE* m_pcbMappedGameObjects = NULL;
	D3D12_VERTEX_BUFFER_VIEW m_d3dInstancingBufferView;

};
