// Copyright (c) 2026 DrunkenPoy
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "MISMeshSplitter.h"
#include "MISSplitSettings.h"

class AActor;
class AStaticMeshActor;
class UStaticMesh;
class UWorld;

/**
 * 레벨에 배치된 원본 스태틱 메시 액터를 분할 파트 액터들로 치환한다 (5단계).
 *
 * 원본 액터를 파트 수만큼 복제한 뒤 메시·트랜스폼·머티리얼 오버라이드만 바꾸므로
 * 모빌리티, 콜리전, 렌더 설정, 태그, 레이어, 부모 어태치먼트가 그대로 유지된다.
 * 머티리얼 오버라이드는 슬롯 이름으로 다시 매핑한다. 전체 작업은 Undo 가능한 트랜잭션 하나로 묶인다.
 */
class MESHISLANDSPLITTER_API FMISActorReplacer
{
public:
	/**
	 * World에서 Source를 사용하는 AStaticMeshActor를 찾는다.
	 * 정확히 AStaticMeshActor 클래스인 액터만 대상이며 (파생 클래스·블루프린트는 치환 시 동작이 바뀔 수 있어 제외),
	 * 제외된 액터 수는 OutNumSkipped로 반환한다.
	 */
	static TArray<AStaticMeshActor*> FindActorsUsingMesh(UWorld* World, const UStaticMesh* Source, int32* OutNumSkipped = nullptr);

	/** Actors 중 치환 가능한 것(정확히 AStaticMeshActor이고 Source를 사용하는 액터)만 골라낸다. */
	static TArray<AStaticMeshActor*> FilterActorsUsingMesh(TConstArrayView<AActor*> Actors, const UStaticMesh* Source, int32* OutNumSkipped = nullptr);

	/**
	 * 각 원본 액터 자리에 파트 액터들을 배치한다. Parts는 FMISMeshSplitter::Split의 결과여야 한다
	 * (PivotOffset으로 파트 위치를 복원한다). 새로 만든 액터를 OutNewActors에 추가하고 치환한 원본 액터 수를 반환한다.
	 */
	static int32 ReplaceActors(
		const UStaticMesh* Source,
		TConstArrayView<FMISSplitPart> Parts,
		TConstArrayView<AStaticMeshActor*> Actors,
		const FMISReplaceSettings& Settings,
		TArray<AActor*>& OutNewActors);
};
