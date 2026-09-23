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
### Content Browser: Static Mesh 우클릭 → **Mesh Island Splitter**
- **Split into Islands...**: 옵션 다이얼로그를 연다. 분할 모드, 용접 허용오차, 병합 거리, 피벗, 네이밍(접미사·시작 인덱스·자릿수·하위 폴더),
  레벨 액터 치환을 설정하고 **Analyze**로 결과를 미리 본 뒤 **Split**. 원본과 같은 폴더(또는 하위 폴더)에 `<원본>_Part_NN` 애셋을 생성한다. 원본 애셋은 변경되지 않음
- **Split with Last Options**: 다이얼로그 없이 마지막으로 사용한 옵션으로 바로 분할
- **Analyze Islands**: 드라이 런. 마지막 옵션으로 메시마다 아일랜드/파트 개수를 알림과 Output Log로 보고

옵션은 프로젝트별 사용자 설정(`EditorPerProjectUserSettings.ini`)에 저장되어 다음 세션에도 유지된다.

### 레벨 액터 치환
- 옵션의 **Replace Actors In Level**을 켜면, 분할 후 현재 레벨에서 원본 메시를 쓰는 Static Mesh Actor를 모두 파트 액터들로 치환한다
- 레벨 뷰포트/아웃라이너에서 액터 선택 → 우클릭 → **Split and Replace with Islands...**: 선택한 액터만 치환
- 원본 액터를 복제해 메시만 바꾸므로 모빌리티·콜리전·렌더 설정·태그·레이어·부모 어태치먼트가 유지되고,
  머티리얼 오버라이드는 슬롯 이름으로 다시 매핑된다. 파트는 원본 라벨 이름의 아웃라이너 폴더로 묶이며, 전체 작업은 Ctrl+Z로 되돌릴 수 있다
- 블루프린트나 AStaticMeshActor 파생 클래스는 치환하지 않는다 (알림에 건너뛴 수 표시)

### 일괄 처리
Content Browser 폴더 우클릭 → **Split Static Meshes in Folder...**: 하위 폴더를 포함한 모든 Static Mesh를 같은 다이얼로그로 처리한다.
분할할 것이 없는 메시는 건너뛰며 진행 창에서 취소할 수 있다.

### 콘솔 / 스크립트
콘솔 커맨드 `MIS.AnalyzeSelected` / `MIS.SplitSelected` / `MIS.SplitSelectedActors`는 기본 설정에서 시작해 `key=value` 인자로만 설정을 바꾼다
(예: `MIS.SplitSelected mode=connectivity pivot=center folder=Parts replace=1`).
키 목록은 [MISConsoleCommands.cpp](Source/MeshIslandSplitter/Private/MISConsoleCommands.cpp) 참고.
Blueprint/Python은 `UMISSplitterLibrary` (`SplitStaticMesh`, `SplitStaticMeshes`, `GetLevelActorsUsingMesh`, `ReplaceActorsWithParts`)를 사용한다.

## 패키징
`Scripts\BuildPlugin.bat` (기본 엔진 경로: `C:\Program Files\Epic Games\UE_5.8`)

## 로드맵
- [x] 1. 저장소/모듈 스캐폴드
- [x] 2. 분할 코어 (연결성/근접/머티리얼 분할, 파트 애셋 생성, 콘솔 커맨드·Blueprint API)
- [x] 3. Content Browser 우클릭 메뉴 연동 (Analyze Islands / Split into Islands, 결과 알림)
- [x] 4. 옵션 UI (용접 허용오차, 피벗, 네이밍)
- [x] 5. 레벨 액터 치환 / 일괄 처리

자세한 설계는 [Docs/DESIGN.md](Docs/DESIGN.md) 참고.

## 라이선스
이 플러그인의 소스 코드는 [MIT License](LICENSE)로 배포됩니다.

Unreal Engine 및 관련 로고는 Epic Games, Inc.의 상표입니다.
이 플러그인을 사용하려면 Unreal Engine이 필요하며, 엔진 자체는 [Unreal Engine EULA](https://www.unrealengine.com/eula)의 적용을 받습니다.
이 저장소에는 엔진 소스 코드가 포함되어 있지 않습니다.

## 기여
이슈와 PR 환영합니다. 기여한 코드는 동일한 MIT License로 배포되는 데 동의한 것으로 간주합니다.
엔진 소스 코드를 복사해 넣는 PR은 EULA 문제로 받을 수 없습니다.
