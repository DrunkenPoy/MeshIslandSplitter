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

## 진입점 (2단계 기준)
- `FMISMeshSplitter::Analyze` / `Split` (C++)
- `UMISSplitterLibrary::AnalyzeStaticMesh` / `SplitStaticMesh` (Blueprint, Python)
- 콘솔: `MIS.AnalyzeSelected`, `MIS.SplitSelected` (`mode= dist= pct= weld= pivot= matboundary=`)

## 결정된 사항
- 피벗: `KeepOriginal`(기본) / `BoundsCenter` / `BoundsBottomCenter`.
  이동량은 `FMISSplitPart::PivotOffset`으로 반환한다.
- LOD: LOD0만 분할한다.
- 결과 파트가 2개 미만이면 아무것도 생성하지 않는다.

## 미결 사항
- LOD1+ 분할, 원본 콜리전(BodySetup) 및 섹션별 설정(그림자/콜리전) 복사
- 대형 island 쌍의 근접 판정 가속(삼각형 단위 공간 해시/BVH)
- 파트 다수 생성 시 일괄 빌드
- 원본 대체(레벨 액터 교체) 여부 → 5단계
