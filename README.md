# DirectX 12 Rendering Framework

C++와 HLSL로 장치·명령 제출·메시·카메라·셰이더를 연결한 렌더링 학습 프로젝트.

| 항목 | 내용 |
|---|---|
| 저장소 이름 | TestProjs |
| 기술 | C++, DirectX 12, HLSL, Win32 |
| 빌드 설정 | Visual Studio, v143, Windows SDK 10.0 |
| 현재 기본 장면 | 색상 정점을 사용하는 회전 큐브 |

> 문서 기준: 2026-11-01. 구현 상태는 소스 검토 기준이며, 이 문서 갱신 과정에서 빌드·플레이·성능 실험을 새로 실행하지 않았습니다.

## 1. 프로젝트 소개

DirectX 12의 초기화부터 한 프레임의 명령 기록·제출·화면 표시까지 직접 연결하는 프로젝트입니다. GameFramework, Scene, Camera, Mesh, Shader, GameObject로 코드를 구성합니다.

현재 기본 장면은 회전 큐브 한 개입니다. 텍스처·조명·그림자·인스턴싱은 이 README에서 현재 구현 기능으로 소개하지 않습니다.

## 2. 실행·시연 방법

- 성능 결과: **미측정**

1. Windows에서 v143 도구 집합과 Windows SDK를 준비합니다.
2. [TestProjs.sln](https://github.com/Han-g/TestProjs/blob/master/TestProjs.sln)을 엽니다.
3. 사용 환경의 DirectX 12 지원을 확인하고 x64 구성으로 빌드합니다.
4. 실행 작업 디렉터리에서 [TestProj/Shaders.hlsl](https://github.com/Han-g/TestProjs/blob/master/TestProj/Shaders.hlsl)을 찾을 수 있도록 설정합니다.
5. 기본 장면의 큐브 회전과 화면 표시를 확인합니다.

Debug 빌드는 D3D12 디버그 계층의 설치·가용 여부를 확인합니다. 위 절차의 새 환경 빌드와 실행은 아직 검증하지 않았습니다. 현재 입력 처리 함수 일부는 비어 있으므로 자유 시점 카메라를 구현 완료로 소개하지 않습니다.

## 3. 현재 구현된 기능

| 영역 | 소스에서 확인한 구현 |
|---|---|
| 초기화 | DXGI Factory, D3D12 Device, SwapChain |
| 명령 | CommandQueue, CommandAllocator, CommandList |
| 렌더 타깃 | RTV, 깊이·스텐실 버퍼, 자원 상태 전환 |
| 동기화 | Fence와 이벤트를 이용한 GPU 완료 대기 |
| 장면 | Scene, GameObject, 회전 객체 |
| 메시 | 정점·인덱스 버퍼, 큐브 메시 |
| 셰이더 | HLSL 컴파일, RootSignature, PSO, 상수 버퍼 |
| 카메라 데이터 | View·Projection 행렬 및 셰이더 전달 |

## 4. 담당 범위와 사용 자료

Windows SDK와 DirectX 12 API를 사용합니다.

개인 작성·수정 범위와 기반 예제의 출처는 별도 작업 기록으로 정리할 예정입니다. 이 문서의 기능 목록은 현재 저장소에 포함된 렌더링 코드를 기준으로 합니다.

## 5. 핵심 구조와 설계

```text
Win32 메시지 / 게임 루프
  → GameFramework::FrameAdvance
  → 입력·애니메이션 갱신
  → CommandAllocator / CommandList 준비
  → 렌더 타깃 상태 전환·초기화
  → Scene → GameObject → Mesh / Shader
  → 명령 제출·Fence 대기
  → Present
```

| 파일 | 역할 |
|---|---|
| [GameFramework.cpp](https://github.com/Han-g/TestProjs/blob/master/TestProj/GameFramework.cpp) | 장치·프레임·스왑 체인·GPU 동기화 |
| [Scene.cpp](https://github.com/Han-g/TestProjs/blob/master/TestProj/Scene.cpp) | 객체 구성과 장면 렌더링 |
| [Mesh.cpp](https://github.com/Han-g/TestProjs/blob/master/TestProj/Mesh.cpp) | 정점·인덱스 버퍼와 그리기 |
| [Shader.cpp](https://github.com/Han-g/TestProjs/blob/master/TestProj/Shader.cpp) | 셰이더 컴파일과 PSO |
| [Shaders.hlsl](https://github.com/Han-g/TestProjs/blob/master/TestProj/Shaders.hlsl) | 월드·뷰·투영 변환과 정점 색상 출력 |

## 6. 문제 해결과 검증 기록

| 관찰 사항 | 개선 방향 | 현재 상태 |
|---|---|---|
| 셰이더 컴파일 결과를 검사하지 않고 Blob 사용 | HRESULT·오류 Blob 확인 후 실패 경로 처리 | 정적 확인, 수정 전 |
| 명령 제출 뒤 매 프레임 GPU 완료 대기 | 프레임별 CommandAllocator·상수 버퍼·Fence 관리 | 개선 계획 |
| 수동 COM 자원 해제 | ComPtr 적용과 자원 소유권 정리 | 개선 계획 |
| 하나의 기본 장면 | 객체 수를 바꿀 수 있는 재현 가능한 데모 | 개선 계획 |
| 성능 수치 | 동일 해상도·객체 수·VSync 조건의 CPU/GPU 시간 | 미측정 |

GPU 대기 호출만 삭제하면 CommandAllocator나 상수 버퍼의 재사용 시점이 잘못될 수 있습니다. 프레임별 자원 수명과 Fence 검사를 함께 설계해야 합니다.

## 7. 현재 제한과 다음 작업

- [ ] 셰이더·장치 생성 실패를 확인 가능한 오류 메시지로 처리
- [ ] COM 자원과 업로드 자원의 소유권·해제 시점 정리
- [ ] 프레임별 자원 구조를 적용하고 재사용 시점에만 대기
- [ ] 실제 입력 처리와 조작 가능한 카메라 추가
- [ ] 텍스처와 기본 조명으로 작은 데모 장면 완성
- [ ] 같은 메시의 개별 드로우와 인스턴싱 비교
- [ ] 동일 조건의 CPU·GPU 프레임 시간과 draw call 수 기록

---

저장소: [TestProjs](https://github.com/Han-g/TestProjs)
