// Copyright (c) 2026 DrunkenPoy
// SPDX-License-Identifier: MIT

#include "MISSplitDialog.h"
#include "MISActorReplacer.h"
#include "MISMeshSplitter.h"
#include "MISSplitOptions.h"

#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "IDetailsView.h"
#include "Misc/ScopedSlowTask.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "MISSplitDialog"

namespace MISSplitDialogImpl
{
	/** 미리보기(Analyze)는 대량 일괄 처리에서 느려지지 않도록 앞쪽 메시 일부만 분석한다. */
	constexpr int32 MaxPreviewMeshes = 50;

	class SMISSplitDialog : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SMISSplitDialog) {}
			SLATE_ARGUMENT(FText, Description)
			SLATE_ARGUMENT(MISSplitDialog::EScope, Scope)
			SLATE_ARGUMENT(TArray<TWeakObjectPtr<UStaticMesh>>, Meshes)
			SLATE_ARGUMENT(TWeakPtr<SWindow>, ParentWindow)
		SLATE_END_ARGS()

		bool bConfirmed = false;

		void Construct(const FArguments& InArgs)
		{
			Scope = InArgs._Scope;
			Meshes = InArgs._Meshes;
			ParentWindow = InArgs._ParentWindow;

			FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
			FDetailsViewArgs DetailsArgs;
			DetailsArgs.bAllowSearch = false;
			DetailsArgs.bHideSelectionTip = true;
			DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
			DetailsView = PropertyEditor.CreateDetailView(DetailsArgs);
			if (Scope == MISSplitDialog::EScope::SelectedActors)
			{
				// 선택한 액터는 항상 치환되므로 "레벨 전체 치환" 옵션은 의미가 없다
				DetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& PropertyAndParent)
				{
					return PropertyAndParent.Property.GetFName() != GET_MEMBER_NAME_CHECKED(UMISSplitOptions, bReplaceActorsInLevel);
				}));
			}
			DetailsView->SetObject(&UMISSplitOptions::Get());

			ChildSlot
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
				.Padding(8.0f)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 8.0f)
					[
						SNew(STextBlock)
						.Text(InArgs._Description)
						.AutoWrapText(true)
					]

					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						DetailsView.ToSharedRef()
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 8.0f)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
						.Padding(6.0f)
						[
							SNew(SBox)
							.MaxDesiredHeight(140.0f)
							[
								SNew(SScrollBox)
								+ SScrollBox::Slot()
								[
									SNew(STextBlock)
									.Text_Lambda([this]() { return AnalysisText; })
									.AutoWrapText(true)
								]
							]
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.Text(LOCTEXT("ResetButton", "Reset to Defaults"))
							.ToolTipText(LOCTEXT("ResetTooltip", "Restore all options to their default values."))
							.OnClicked(this, &SMISSplitDialog::OnReset)
						]

						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.HAlign(HAlign_Right)
						[
							SNew(SUniformGridPanel)
							.SlotPadding(FMargin(4.0f, 0.0f))

							+ SUniformGridPanel::Slot(0, 0)
							[
								SNew(SButton)
								.HAlign(HAlign_Center)
								.Text(LOCTEXT("AnalyzeButton", "Analyze"))
								.ToolTipText(LOCTEXT("AnalyzeButtonTooltip", "Preview island and part counts with the current options. Nothing is created."))
								.OnClicked(this, &SMISSplitDialog::OnAnalyze)
							]

							+ SUniformGridPanel::Slot(1, 0)
							[
								SNew(SButton)
								.HAlign(HAlign_Center)
								.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
								.Text(LOCTEXT("SplitButton", "Split"))
								.OnClicked(this, &SMISSplitDialog::OnSplit)
							]

							+ SUniformGridPanel::Slot(2, 0)
							[
								SNew(SButton)
								.HAlign(HAlign_Center)
								.Text(LOCTEXT("CancelButton", "Cancel"))
								.OnClicked(this, &SMISSplitDialog::OnCancel)
							]
						]
					]
				]
			];

			AnalysisText = LOCTEXT("AnalysisPlaceholder", "Press Analyze to preview how many parts each mesh would produce.");
		}

	private:
		MISSplitDialog::EScope Scope = MISSplitDialog::EScope::Assets;
		TArray<TWeakObjectPtr<UStaticMesh>> Meshes;
		TWeakPtr<SWindow> ParentWindow;
		TSharedPtr<IDetailsView> DetailsView;
		FText AnalysisText;

		FReply OnAnalyze()
		{
			const UMISSplitOptions& Options = UMISSplitOptions::Get();
			const int32 NumToAnalyze = FMath::Min(Meshes.Num(), MaxPreviewMeshes);
			const bool bCountActors = Options.bReplaceActorsInLevel && Scope == MISSplitDialog::EScope::Assets;
			UWorld* EditorWorld = (bCountActors && GEditor) ? GEditor->GetEditorWorldContext().World() : nullptr;

			FScopedSlowTask SlowTask(float(NumToAnalyze), LOCTEXT("Analyzing", "Analyzing static meshes..."));
			if (NumToAnalyze > 1)
			{
				SlowTask.MakeDialogDelayed(0.5f);
			}

			TArray<FString> Lines;
			int32 TotalParts = 0;
			int32 NumSplittable = 0;
			int32 NumActors = 0;
			for (int32 Index = 0; Index < NumToAnalyze; ++Index)
			{
				SlowTask.EnterProgressFrame(1.0f);
				UStaticMesh* Mesh = Meshes[Index].Get();
				if (!Mesh)
				{
					continue;
				}

				FMISSplitAnalysis Analysis;
				FText Error;
				if (!FMISMeshSplitter::Analyze(Mesh, Options.Split, Analysis, &Error))
				{
					Lines.Add(Error.ToString());
					continue;
				}

				if (Analysis.PartCount >= 2)
				{
					++NumSplittable;
					TotalParts += Analysis.PartCount;
				}
				FString Line = FString::Printf(TEXT("%s: %d island(s) -> %d part(s)%s"), *Mesh->GetName(), Analysis.IslandCount, Analysis.PartCount,
					Analysis.PartCount < 2 ? TEXT(" (skipped)") : TEXT(""));
				if (EditorWorld && Analysis.PartCount >= 2)
				{
					const int32 NumMeshActors = FMISActorReplacer::FindActorsUsingMesh(EditorWorld, Mesh).Num();
					NumActors += NumMeshActors;
					Line += FString::Printf(TEXT(", %d level actor(s)"), NumMeshActors);
				}
				Lines.Add(MoveTemp(Line));
			}

			FString Summary = FString::Printf(TEXT("%d of %d mesh(es) would be split into %d part(s)."), NumSplittable, NumToAnalyze, TotalParts);
			if (EditorWorld)
			{
				Summary += FString::Printf(TEXT(" %d level actor(s) would be replaced."), NumActors);
			}
			if (Meshes.Num() > NumToAnalyze)
			{
				Summary += FString::Printf(TEXT(" (Preview limited to the first %d of %d meshes.)"), NumToAnalyze, Meshes.Num());
			}
			Lines.Insert(Summary, 0);
			AnalysisText = FText::FromString(FString::Join(Lines, TEXT("\n")));
			return FReply::Handled();
		}

		FReply OnReset()
		{
			UMISSplitOptions& Options = UMISSplitOptions::Get();
			Options.Split = FMISSplitSettings();
			Options.bReplaceActorsInLevel = false;
			Options.Replace = FMISReplaceSettings();
			DetailsView->ForceRefresh();
			return FReply::Handled();
		}

		FReply OnSplit()
		{
			bConfirmed = true;
			return Close();
		}

		FReply OnCancel()
		{
			return Close();
		}

		FReply Close()
		{
			if (const TSharedPtr<SWindow> Window = ParentWindow.Pin())
			{
				Window->RequestDestroyWindow();
			}
			return FReply::Handled();
		}
	};
} // namespace MISSplitDialogImpl

bool MISSplitDialog::Show(const FText& Description, TConstArrayView<UStaticMesh*> Meshes, EScope Scope)
{
	if (Meshes.Num() == 0 || !GEditor)
	{
		return false;
	}

	TArray<TWeakObjectPtr<UStaticMesh>> WeakMeshes;
	for (UStaticMesh* Mesh : Meshes)
	{
		WeakMeshes.Add(Mesh);
	}

	const TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("WindowTitle", "Mesh Island Splitter"))
		.ClientSize(FVector2D(540.0f, 720.0f))
		.SupportsMinimize(false)
		.SupportsMaximize(false);

	const TSharedRef<MISSplitDialogImpl::SMISSplitDialog> Dialog = SNew(MISSplitDialogImpl::SMISSplitDialog)
		.Description(Description)
		.Scope(Scope)
		.Meshes(MoveTemp(WeakMeshes))
		.ParentWindow(Window);
	Window->SetContent(Dialog);

	GEditor->EditorAddModalWindow(Window);

	// 취소해도 편집한 옵션은 유지한다 (다음에 다이얼로그를 열 때 그대로 보이도록)
	UMISSplitOptions::Get().SaveConfig();
	return Dialog->bConfirmed;
}

#undef LOCTEXT_NAMESPACE
