# Design Notes

## 목표
하나의 Static Mesh 안에 서로 물리적으로 떨어진 덩어리(island)가 여러 개 있을 때,
이를 자동 감지하여 island 별로 별도의 Static Mesh 에셋으로 분리한다. (에디터 전용)

## "물리적으로 분리됨"의 정의 (결정 필요)
- 토폴로지 연결: MeshDescription의 Vertex(위치 정점)를 공유하는 삼각형끼리 연결.
  UV/노멀 seam은 VertexInstance만 갈라지므로 연결이 유지됨.
- 문제: FBX 임포트 등에서 같은 위치에 정점이 중복(unwelded)되어 있으면
  실제로는 붙어 있는데 별개 island로 판정될 수 있음.
- 대안: 위치 허용오차(tolerance) 기반 용접 후 연결 판정 → 옵션으로 제공.

## 파이프라인 (초안)
1. UStaticMesh (LOD0) → FMeshDescription
2. FMeshDescription → FDynamicMesh3
3. (옵션) tolerance 기반 정점 용접
4. 연결 컴포넌트 계산 (FMeshConnectedComponents)
5. 컴포넌트별 서브메시 추출 → FMeshDescription → 새 UStaticMesh 에셋 생성
6. 머티리얼 슬롯/섹션, UV, 피벗 처리

## 미결 사항
- 피벗: 원본 유지 vs island 바운드 중심
- LOD1+ / Nanite / 콜리전 처리
- 원본 대체(레벨 액터 교체) 여부

## 에디터 UI (3단계)
- `UToolMenus` 시작 콜백에서 `UE::ContentBrowser::ExtendToolMenu_AssetContextMenu(UStaticMesh)`를 확장,
  "Asset Actions" 섹션에 **Mesh Island Splitter** 서브메뉴(Analyze Islands / Split into Islands)를 추가.
- 선택 목록은 `UContentBrowserAssetContextMenuContext`에서 가져오며, 실제 작업은 콘솔 커맨드와 공유하는
  `MISEditorActions`(Private)가 수행: 로그 + 에디터 알림, 분할 후 생성된 애셋으로 Content Browser 동기화.
- 메뉴 소유자는 모듈 인스턴스이며 `ShutdownModule`에서 `UToolMenus::UnregisterOwner`로 해제.
- 설정은 아직 `FMISSplitSettings` 기본값 고정 → 4단계에서 옵션 다이얼로그로 대체.
