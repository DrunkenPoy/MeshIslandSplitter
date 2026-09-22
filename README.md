# Mesh Island Splitter (UE 5.8)

Static Mesh 안의 물리적으로 분리된 덩어리(island)를 자동 감지해 개별 Static Mesh로 분할하는 **에디터 전용** 플러그인.

## 요구 사항
- Unreal Engine 5.8
- Win64 (Mac/Linux 빌드는 미검증)

## 개발 환경 세팅
1. 호스트용 빈 C++ 프로젝트 생성 (예: `D:\UE\MISHost`)
2. `Scripts\LinkToProject.bat "D:\UE\MISHost"` 실행 → `Plugins\MeshIslandSplitter` 정션 생성
3. `.uproject` 우클릭 → Generate Visual Studio project files → 빌드

## 패키징
`Scripts\BuildPlugin.bat` (기본 엔진 경로: `C:\Program Files\Epic Games\UE_5.8`)

## 로드맵
- [x] 1. 저장소/모듈 스캐폴드
- [ ] 2. 분할 코어 (MeshDescription ↔ DynamicMesh, 연결 컴포넌트)
- [ ] 3. Content Browser 우클릭 메뉴 연동
- [ ] 4. 옵션 UI (용접 허용오차, 피벗, 네이밍)
- [ ] 5. 레벨 액터 치환 / 일괄 처리

자세한 설계는 [Docs/DESIGN.md](Docs/DESIGN.md) 참고.
