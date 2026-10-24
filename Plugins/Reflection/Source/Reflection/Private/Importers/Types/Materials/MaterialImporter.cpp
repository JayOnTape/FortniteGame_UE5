/* Copyright Reflection Contributors 2024-2026 */

#include "Importers/Types/Materials/MaterialImporter.h"
#include "Importers/Types/Materials/MaterialStubs.h"

/* Include Material.h (depends on UE Version) */
#if (ENGINE_UE5 && ENGINE_MINOR_VERSION < 3) || ENGINE_UE4
#include "Materials/Material.h"
#else
#include "MaterialDomain.h"
#endif

/* Cached expression data, and the header holding it, only exist from 4.25 on */
#if !UE4_24_BELOW
#include "MaterialCachedData.h"
#endif
#include "Factories/MaterialFactoryNew.h"
#include "Settings/ReflectionSettings.h"
#include "Misc/CommandLine.h"
#include "Materials/MaterialExpressionTextureBase.h"

#if ENGINE_UE5
class UMaterialAccessor final : public UMaterial {
public:
	FMaterialCachedExpressionData* GetCachedExpressionDataRef() const {
		return CachedExpressionData.Get();
	}
};
#endif

UObject* IMaterialImporter::CreateAsset(UObject* CreatedAsset) {
	/* Create Material Factory (factory automatically creates the Material) */
	UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
	UMaterial* Material = Cast<UMaterial>(MaterialFactory->FactoryCreateNew(UMaterial::StaticClass(), GetPackage(), *GetAssetName(), RF_Standalone | RF_Public, nullptr, GWarn));

	return IImporter::CreateAsset(Material);
}

bool IMaterialImporter::Import() {
	/* Define material data from the JSON */
	FUObjectExportContainer* ExpressionContainer = new FUObjectExportContainer();
	TSharedPtr<FJsonObject> Props = FindMaterialData(GetAssetType(), ExpressionContainer);

	if (IsRunningCommandlet() && FParse::Param(FCommandLine::Get(), TEXT("ReflectMaterialSettingsOnly"))) {
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *GetPackage()->GetPathName());
		if (!Material) return false;
		UMaterialExpression* SourceOutput = nullptr;
		int32 SourceOutputIndex = 0;
		const TSharedPtr<FJsonObject>* Emissive;
		if (Props->TryGetObjectField(TEXT("EmissiveColor"), Emissive)) {
			FString Name;
			if ((*Emissive)->TryGetStringField(TEXT("ExpressionName"), Name) && Name != TEXT("None")) {
#if UE5_1_BEYOND
				const auto& Expressions = Material->GetExpressionCollection().Expressions;
#else
				const auto& Expressions = Material->Expressions;
#endif
				for (UMaterialExpression* Expression : Expressions)
					if (Expression && Expression->GetName() == Name) SourceOutput = Expression;
				if (!SourceOutput) return false;
				SourceOutputIndex = (*Emissive)->GetIntegerField(TEXT("OutputIndex"));
				if (!SourceOutput->GetOutputs().IsValidIndex(SourceOutputIndex)) return false;
			}
		}
		TSharedPtr<FJsonObject> Settings = MakeShared<FJsonObject>();
		for (const auto& Entry : Props->Values) {
			const FProperty* Property = FindFProperty<FProperty>(Material->GetClass(), *Entry.Key);
			if (Property && (Property->IsA<FNumericProperty>() || Property->IsA<FBoolProperty>()
				|| Property->IsA<FEnumProperty>() || Property->IsA<FNameProperty>()
				|| Property->IsA<FStrProperty>() || Property->IsA<FTextProperty>()))
				Settings->SetField(Entry.Key, Entry.Value);
		}
		Material->Modify();
		GetObjectSerializer()->DeserializeObjectProperties(Settings, Material);
		if (SourceOutput) Material->GetExpressionInputForProperty(MP_EmissiveColor)->Connect(SourceOutputIndex, SourceOutput);
		Material->PostEditChange();
		Material->MarkPackageDirty();
		AssetExport->Object = Material;
		UE_LOG(LogReflection, Display, TEXT("Updated authored material settings and output on existing graph: %s; graph reconstruction was not requested"), *Material->GetPathName());
		return OnAssetCreation(Material);
	}

	/* Whatever would be built out of a list with holes in it is not this material */
	if (HasNullExpressions(Props)) {
		ReportNullExpressions();

		return false;
	}

	UMaterial* Material = Create<UMaterial>();

	/* Clear any default expressions the engine adds */
#if UE5_1_BEYOND
	Material->GetExpressionCollection().Empty();
#else
	Material->Expressions.Empty();
#endif

    for (FUObjectExport* Export : ExpressionContainer->Exports) Export->Parent = Material;

	/* Map out each expression for easier access */
	ConstructExpressions(ExpressionContainer);

	const UReflectionSettings* Settings = GetSettings();
	
	/* If Missing Material Data */
	if (ExpressionContainer->Num() == 0) {
#if ENGINE_UE5
		if (GetModdingAssetSettings().Material.Stubs) {
			CreateStubs(this);
			ReportCreatedStubs();
		}
		else {
#endif
			ReportMaterialDataMissing();
#if ENGINE_UE5
			return false;
		}
#endif
	}

	/* Iterate through all the expressions, and set properties */
	PropagateExpressions(ExpressionContainer);

#if UE5_1_BEYOND
	UMaterialEditorOnlyData* EditorOnlyData = Material->GetEditorOnlyData();
#else
	UMaterial* EditorOnlyData = Material;
#endif
	
	TArray<FString> IgnoredProperties = TArray<FString> {
		"ParameterGroupData",
		"ExpressionCollection",
		"CustomizedUVs"
	};

	const TSharedPtr<FJsonObject> RawConnectionData = TSharedPtr<FJsonObject>(Props);
	for (FString Property : IgnoredProperties) {
		if (RawConnectionData->HasField(Property)) {
			RawConnectionData->RemoveField(Property);
		}
	}
	
	/* Connect all pins using deserializer */
	GetObjectSerializer()->DeserializeObjectProperties(RawConnectionData, EditorOnlyData);

	/* CustomizedUVs defined here */
	const TArray<TSharedPtr<FJsonValue>>* InputsPtr;
	
	if (Props->TryGetArrayField(TEXT("CustomizedUVs"), InputsPtr)) {
		int i = 0;
		for (const auto& InputValue : *InputsPtr) {
			FJsonObject* InputObject = InputValue->AsObject().Get();
			FName InputExpressionName = GetExpressionName(InputObject);

			if (ExpressionContainer->Contains(InputExpressionName)) {
				FExpressionInput Input = PopulateExpressionInput(InputObject, ExpressionContainer->Find<UMaterialExpression>(InputExpressionName));
				RemapConvertOutput(Input);

				EditorOnlyData->CustomizedUVs[i] = *reinterpret_cast<FVector2MaterialInput*>(&Input);
			}
			i++;
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* StringParameterGroupData;
	if (Props->TryGetArrayField(TEXT("ParameterGroupData"), StringParameterGroupData)) {
		TArray<FParameterGroupData> ParameterGroupData;

		for (const auto& ParameterGroupDataObject : *StringParameterGroupData) {
			if (ParameterGroupDataObject->IsNull()) continue;
			FParameterGroupData GroupData;

			FString GroupName;
			if (ParameterGroupDataObject->AsObject()->TryGetStringField(TEXT("GroupName"), GroupName)) GroupData.GroupName = GroupName;
			int GroupSortPriority;
			if (ParameterGroupDataObject->AsObject()->TryGetNumberField(TEXT("GroupSortPriority"), GroupSortPriority)) GroupData.GroupSortPriority = GroupSortPriority;

			ParameterGroupData.Add(GroupData);
		}

		EditorOnlyData->ParameterGroupData = ParameterGroupData;
	}

	/* Everything is wired by this point, which is what dropping an output needs */
	PruneMissingMaterialAttributes(Material, EditorOnlyData);

	/* Handle edit changes, and add it to the content browser */
	if (!OnAssetCreation(Material)) return false;

	const TSharedPtr<FJsonObject>* ShadingModelsPtr;
	
	if (GetAssetData()->TryGetObjectField(TEXT("ShadingModels"), ShadingModelsPtr)) {
		int ShadingModelField;
		
		if (ShadingModelsPtr->Get()->TryGetNumberField(TEXT("ShadingModelField"), ShadingModelField)) {
#if UE5_1_BEYOND
			Material->GetShadingModels().SetShadingModelField(ShadingModelField);
#else
			/* Not to sure what to do in UE4, no function exists to override it. */
#endif
		}
	}

	/* Deserialize any properties */
	GetObjectSerializer()->DeserializeObjectProperties(GetAssetData(), Material);

#if ENGINE_UE5
	/* Update Cached Expression Data */
	if (AssetExport->GetJsonObject().Has("CachedExpressionData")) {
		FUObjectJsonValueExport CachedExpressionData = AssetExport->GetJsonObject().GetObject("CachedExpressionData");

		UMaterialAccessor* Accessor = Cast<UMaterialAccessor>(Material);
		FMaterialCachedExpressionData* CachedData = Accessor->GetCachedExpressionDataRef();

		/*GetPropertySerializer()->DeserializeStruct(FMaterialCachedExpressionData::StaticStruct(), CachedExpressionData.JsonObject.ToSharedRef(), CachedData);*/
	}
#endif
	
	/* Move Material Result Node ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
	UMaterialExpression* PositionalExpression = EditorOnlyData->BaseColor.Expression;
	if (UMaterialExpression* MaterialAttributes = EditorOnlyData->MaterialAttributes.Expression) {
		if (Material->bUseMaterialAttributes) {
			PositionalExpression = MaterialAttributes;
		}
	}

	if (!PositionalExpression) {
		PositionalExpression = EditorOnlyData->EmissiveColor.Expression;
	}

	if (PositionalExpression) {
		Material->EditorX = PositionalExpression->MaterialExpressionEditorX + PositionalExpression->GetWidth() * 2.5;
		Material->EditorY = PositionalExpression->MaterialExpressionEditorY;
	}
	/* Move Material Result Node ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

	/* Cached expression data arrived in 4.25; before that ForceRecompileForRendering below is
	 * what walks the expressions the importer just built */
#if !UE4_24_BELOW
	Material->UpdateCachedExpressionData();
#endif

	FMaterialUpdateContext MaterialUpdateContext;
	MaterialUpdateContext.AddMaterial(Material);
	
#if UE5_1_BEYOND
	const auto& TextureExpressions = Material->GetExpressionCollection().Expressions;
#else
	const auto& TextureExpressions = Material->Expressions;
#endif
	for (UMaterialExpression* Expression : TextureExpressions) {
		UMaterialExpressionTextureBase* TextureExpression = Cast<UMaterialExpressionTextureBase>(Expression);
		if (TextureExpression && TextureExpression->Texture) TextureExpression->AutoSetSampleType();
	}

	Material->ForceRecompileForRendering();

	Material->PostEditChange();
	Material->MarkPackageDirty();
	Material->PreEditChange(nullptr);

	Save();

	return true;
}
