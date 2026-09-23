# Design Notes

## 목표
하나의 Static Mesh 안에 서로 물리적으로 떨어진 덩어리(island)가 여러 개 있을 때,
이를 자동 감지하여 island 별로 별도의 Static Mesh 에셋으로 분리한다. (에디터 전용)

## "물리적으로 분리됨"의 정의
- 토폴로지 연결: MeshDescription의 Vertex(위치 정점)를 공유하는 삼각형끼리 연결.
  UV/노멀 seam은 VertexInstance만 갈라지므로 연결이 유지됨.
- FBX 임포트 등에서 같은 위치에 정점이 중복(unwelded)되어 있으면 실제로는 붙어 있어도
  별개 island로 판정되므로, 위치 허용오차(`WeldTolerance`) 기반 용접을 옵션으로 제공한다
  (기본 켜짐, 0.01cm). 용접은 그룹 판정에만 쓰이며 출력 지오메트리는 용접하지 않는다.

## 분할 모드
| 모드 | 동작 |
|---|---|
| Connectivity | (용접된) 정점을 공유하는 삼각형끼리 한 파트. 텍스트 메시는 글자마다 파트가 됨 |
| Proximity (기본) | 최근접 삼각형 간 거리가 `MergeDistance` 이내인 연결성 island를 병합. 단어는 한 파트로 유지 |
| MaterialSlot | 토폴로지와 무관하게 머티리얼 슬롯(폴리곤 그룹)당 한 파트 |

- `MergeDistance`는 절대값(cm) 또는 원본 바운즈 대각선 대비 %로 지정.
- `bRespectMaterialBoundary`: 서로 다른 머티리얼의 삼각형을 통해서는 병합하지 않음
  (보석 → 라벨 텍스트 같은 연쇄 병합 방지).

## 파이프라인
1. UStaticMesh LOD0 → `FMeshDescription`
2. 위치/삼각형/머티리얼 id를 엔진 독립 입력(`MIS::FCoreMeshInput`)으로 추출
3. 분할 코어(`MISSplitCore`, 엔진 비의존)
   - (옵션) 공간 해시 기반 정점 용접 → Union-Find로 연결성 island 계산
   - Proximity: island AABB로 후보를 거른 뒤 삼각형-삼각형 최소 거리로 병합 판정
   - 그룹 번호는 삼각형 등장 순서대로 매겨 결정적(deterministic)으로 유지
4. 그룹별로 원본 삼각형을 새 `FMeshDescription`에 복사
   (노멀·탄젠트·바이노멀 부호·컬러·모든 UV 채널·엣지 강도·머티리얼 슬롯 이름 유지)
5. 원본 옆에 `<원본><PartSuffix><NN>` 이름으로 새 UStaticMesh 에셋 생성.
   머티리얼은 슬롯 이름으로 매칭하고, 빌드 설정·Nanite·라이트맵 설정을 원본에서 복사.
   원본 애셋은 수정하지 않는다.

GeometryProcessing(`FDynamicMesh3`/`FMeshConnectedComponents`)은 쓰지 않는다.
근접 병합과 머티리얼 경계 옵션이 필요하고, 코어를 엔진 없이 단위 테스트하기 위해서다
(`Tests/CoreStandalone`, CI: `.github/workflows/core-tests.yml`).

## 진입점
- Content Browser: Static Mesh 우클릭 → Mesh Island Splitter (아래 "에디터 UI" 참고)
- `FMISMeshSplitter::Analyze` / `Split` (C++)
- 폴더 우클릭 → Split Static Meshes in Folder... (일괄 처리, 5단계)
- 레벨 액터 우클릭 → Split and Replace with Islands... (선택 액터 치환, 5단계)
- `FMISActorReplacer::FindActorsUsingMesh` / `ReplaceActors` (C++)
- `UMISSplitterLibrary::AnalyzeStaticMesh` / `SplitStaticMesh` / `SplitStaticMeshes` / `GetLevelActorsUsingMesh` / `ReplaceActorsWithParts` (Blueprint, Python)
- 콘솔: `MIS.AnalyzeSelected`, `MIS.SplitSelected`, `MIS.SplitSelectedActors`
  (`mode= dist= pct= weld= pivot= matboundary= suffix= start= digits= folder= replace= keep=`)

## 에디터 UI (3단계)
- `UToolMenus` 시작 콜백에서 `UE::ContentBrowser::ExtendToolMenu_AssetContextMenu(UStaticMesh)`를 확장,
  "Asset Actions" 섹션에 **Mesh Island Splitter** 서브메뉴(Analyze Islands / Split into Islands)를 추가.
- 선택 목록은 `UContentBrowserAssetContextMenuContext`에서 가져오며, 실제 작업은 콘솔 커맨드와 공유하는
  `MISEditorActions`(Private)가 수행: 로그 + 에디터 알림, 분할 후 생성된 애셋으로 Content Browser 동기화.
- 메뉴 소유자는 모듈 인스턴스이며 `ShutdownModule`에서 `UToolMenus::UnregisterOwner`로 해제.
- 메뉴 항목: Split into Islands...(옵션 다이얼로그) / Split with Last Options / Analyze Islands.

## 옵션 UI (4단계)
- `UMISSplitOptions`(Private, `config=EditorPerProjectUserSettings`)의 CDO를 `IDetailsView`로 직접 편집하는 모달 다이얼로그(`MISSplitDialog`).
  `FMISSplitSettings`와 `FMISReplaceSettings`를 `ShowOnlyInnerProperties`로 펼쳐 보여주므로 UPROPERTY 메타(EditCondition, Units, Clamp)가 그대로 UI가 된다.
- 다이얼로그를 닫으면(취소 포함) `SaveConfig()`로 저장 → 다음 세션과 "Split with Last Options"/"Analyze Islands"가 같은 옵션을 사용.
- Analyze 버튼은 다이얼로그 안에 결과를 표시한다. 일괄 처리 시 앞쪽 50개 메시만 미리 본다.
- 네이밍: `<원본><PartSuffix><StartIndex+N, IndexDigits 자리>`, `OutputSubfolder`가 있으면 원본 폴더 아래 하위 폴더에 생성.
  경로는 `FMISMeshSplitter::ResolveOutputPath`에서 검증한다.
- 콘솔 커맨드는 저장된 옵션을 쓰지 않고 항상 기본값 + 인자로 동작한다 (재현 가능한 테스트용).

## 레벨 액터 치환 / 일괄 처리 (5단계)
- 치환 대상은 클래스가 정확히 `AStaticMeshActor`인 액터뿐. 파생 클래스·블루프린트는 메시만 바꾸면 동작이 달라질 수 있어 제외하고 개수만 보고한다.
- 파트마다 `UEditorActorSubsystem::DuplicateActor`로 원본을 복제(원본과 같은 레벨)한 뒤 메시·트랜스폼·오버라이드만 바꾼다.
  트랜스폼은 `FTransform(PivotOffset) * 원본 트랜스폼`이므로 어떤 피벗 모드에서도 원본과 정확히 겹친다.
- 머티리얼 오버라이드는 파트 슬롯 이름 → 원본 슬롯 인덱스로 다시 매핑한다.
- 원본 삭제 시 원본에 붙어 있던 자식 액터는 첫 번째 파트로 옮긴다(월드 트랜스폼 유지). 메시당 하나의 `FScopedTransaction`으로 묶여 Undo 가능.
  애셋 생성 자체는 트랜잭션에 포함되지 않는다 (Undo해도 파트 애셋은 남음).
- 일괄 처리: 폴더 경로들에서 Asset Registry로 Static Mesh를 재귀 수집·로드하여 같은 `SplitMeshes` 경로로 처리.
  파트가 2개 미만인 메시는 실패가 아닌 "건너뜀"으로 집계하고(`Split`의 `bOutNothingToSplit`), 진행 창에서 취소할 수 있다.

## 결정된 사항
- 피벗: `KeepOriginal`(기본) / `BoundsCenter` / `BoundsBottomCenter`.
  이동량은 `FMISSplitPart::PivotOffset`으로 반환한다.
- LOD: LOD0만 분할한다.
- 결과 파트가 2개 미만이면 아무것도 생성하지 않는다.

## 미결 사항
- LOD1+ 분할, 원본 콜리전(BodySetup) 및 섹션별 설정(그림자/콜리전) 복사
- 대형 island 쌍의 근접 판정 가속(삼각형 단위 공간 해시/BVH)
- 파트 다수 생성 시 일괄 빌드
- 블루프린트/파생 클래스 액터, 인스턴스드 스태틱 메시(ISM/HISM) 컴포넌트 치환
- 레벨 치환 대상이 현재 에디터 월드(로드된 레벨)로 한정됨. 월드 파티션에서 언로드된 셀의 액터는 치환되지 않음
