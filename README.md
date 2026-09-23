# Mesh Island Splitter (UE 5.8)

Static Mesh 안의 물리적으로 분리된 덩어리(island)를 자동 감지해 개별 Static Mesh로 분할하는 **에디터 전용** 플러그인.

## 요구 사항
- Unreal Engine 5.8
- Win64 (Mac/Linux 빌드는 미검증)

## 개발 환경 세팅
1. 호스트용 빈 C++ 프로젝트 생성 (예: `D:\UE\MISHost`)
2. `Scripts\LinkToProject.bat "D:\UE\MISHost"` 실행 → `Plugins\MeshIslandSplitter` 정션 생성
3. `.uproject` 우클릭 → Generate Visual Studio project files → 빌드

## 사용법
Content Browser에서 Static Mesh를 선택(복수 선택 가능) → 우클릭 → **Mesh Island Splitter**
- **Analyze Islands**: 드라이 런. 메시마다 아일랜드/파트 개수를 알림과 Output Log로 보고하며 아무것도 생성하지 않음
- **Split into Islands**: 원본과 같은 폴더에 `<원본>_Part_NN` 애셋을 생성하고 Content Browser에서 선택. 원본은 변경되지 않음

현재는 기본 설정(Proximity, 병합 거리 1cm, 용접 0.01cm, 원본 피벗)을 사용한다. 설정을 바꿔 테스트하려면 콘솔 커맨드
`MIS.AnalyzeSelected` / `MIS.SplitSelected`에 `key=value` 인자를 넘긴다 (예: `MIS.SplitSelected mode=connectivity pivot=center`).
키 목록은 [MISConsoleCommands.cpp](Source/MeshIslandSplitter/Private/MISConsoleCommands.cpp) 참고.

## 패키징
`Scripts\BuildPlugin.bat` (기본 엔진 경로: `C:\Program Files\Epic Games\UE_5.8`)

## 로드맵
- [x] 1. 저장소/모듈 스캐폴드
- [x] 2. 분할 코어 (MeshDescription ↔ DynamicMesh, 연결 컴포넌트)
- [x] 3. Content Browser 우클릭 메뉴 연동
- [ ] 4. 옵션 UI (용접 허용오차, 피벗, 네이밍)
- [ ] 5. 레벨 액터 치환 / 일괄 처리

자세한 설계는 [Docs/DESIGN.md](Docs/DESIGN.md) 참고.

## 라이선스
이 플러그인의 소스 코드는 [MIT License](LICENSE)로 배포됩니다.

Unreal Engine 및 관련 로고는 Epic Games, Inc.의 상표입니다.
이 플러그인을 사용하려면 Unreal Engine이 필요하며, 엔진 자체는 [Unreal Engine EULA](https://www.unrealengine.com/eula)의 적용을 받습니다.
이 저장소에는 엔진 소스 코드가 포함되어 있지 않습니다.

## 기여
이슈와 PR 환영합니다. 기여한 코드는 동일한 MIT License로 배포되는 데 동의한 것으로 간주합니다.
엔진 소스 코드를 복사해 넣는 PR은 EULA 문제로 받을 수 없습니다.
