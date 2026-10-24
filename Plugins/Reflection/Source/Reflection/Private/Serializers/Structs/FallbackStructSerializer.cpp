/* Copyright Reflection Contributors 2024-2026 */

#include "Serializers/Structs/FallbackStructSerializer.h"
#include "Serializers/PropertySerializer.h"
#include "Importers/Constructor/ImportIssues.h"
#include "Engine/Scene.h"
#include "Settings/Runtime.h"
#include "Serialization/MemoryReader.h"
#include "UObject/RenderingObjectVersion.h"
#include "UObject/ReleaseObjectVersion.h"
#if ENGINE_UE5
#include "UObject/UE5ReleaseStreamObjectVersion.h"
#include "UObject/FortniteMainBranchObjectVersion.h"
#endif

void FFallbackStructSerializer::Deserialize(UScriptStruct* Struct, void* StructValue, const TSharedPtr<FJsonObject> JsonValue, UObject* OptionalOuter) {
	if (!JsonValue.IsValid()) return;
	const bool bPostProcess = Struct->IsChildOf(FPostProcessSettings::StaticStruct());
	const bool bOldExposure = bPostProcess && GReflectionRuntime.IsUE4() && GReflectionRuntime.MinorVersion >= 0 && GReflectionRuntime.MinorVersion < 25;
	if (bOldExposure) {
		FPostProcessSettings& Settings = *static_cast<FPostProcessSettings*>(StructValue);
		// PostSerialize stores the raw bias. Restore it before another sparse import
		// so conversion is idempotent and inherited settings are not converted twice.
		Settings.AutoExposureBias = Settings.AutoExposureBiasBackup;
		Settings.bOverride_AutoExposureBias = Settings.bOverride_AutoExposureBiasBackup;
	}
	TSharedPtr<FJsonObject> Values = JsonValue;
#if ENGINE_UE5
	if (bPostProcess) {
		// Work on a copy: mapping one struct must not change the source used by
		// another phase or by a child Blueprint's inherited defaults.
		Values = MakeShared<FJsonObject>();
		Values->Values = JsonValue->Values;
		const auto CopyField = [&Values](const FString& To, const TSharedPtr<FJsonValue>& Value) {
			if (Value.IsValid() && !Values->HasField(To)) Values->SetField(To, Value);
		};
		// UE4's prefilter vector represents minimum, maximum and multiplier.
		if (const TSharedPtr<FJsonValue> Filter = JsonValue->TryGetField(TEXT("BloomConvolutionPreFilter")); Filter.IsValid() && Filter->Type == EJson::Object) {
			const auto Object = Filter->AsObject();
			CopyField(TEXT("BloomConvolutionPreFilterMin"), Object->TryGetField(TEXT("X")));
			CopyField(TEXT("BloomConvolutionPreFilterMax"), Object->TryGetField(TEXT("Y")));
			CopyField(TEXT("BloomConvolutionPreFilterMult"), Object->TryGetField(TEXT("Z")));
		}
		const TSharedPtr<FJsonValue> FilterOverride = JsonValue->TryGetField(TEXT("bOverride_BloomConvolutionPreFilter"));
		CopyField(TEXT("bOverride_BloomConvolutionPreFilterMin"), FilterOverride);
		CopyField(TEXT("bOverride_BloomConvolutionPreFilterMax"), FilterOverride);
		CopyField(TEXT("bOverride_BloomConvolutionPreFilterMult"), FilterOverride);

		// Legacy Blendables had no per-entry weight; the engine used a weight of one.
		if (!Values->HasField(TEXT("WeightedBlendables"))) {
			const TSharedPtr<FJsonValue> Blendables = JsonValue->TryGetField(TEXT("Blendables"));
			if (Blendables.IsValid() && Blendables->Type == EJson::Array) {
				TArray<TSharedPtr<FJsonValue>> Array;
				for (const auto& Blendable : Blendables->AsArray()) {
					TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
					Entry->SetNumberField(TEXT("Weight"), 1.0);
					Entry->SetField(TEXT("Object"), Blendable);
					Array.Add(MakeShared<FJsonValueObject>(Entry));
				}
				TSharedPtr<FJsonObject> Weighted = MakeShared<FJsonObject>();
				Weighted->SetArrayField(TEXT("Array"), Array);
				Values->SetObjectField(TEXT("WeightedBlendables"), Weighted);
			}
		}
	}
#endif

	for (FProperty* Property = Struct->PropertyLink; Property; Property = Property->PropertyLinkNext) {
		const FString PropertyName = Property->GetName();
		FString ValueName = PropertyName;
		// Keep legacy post process data in the deprecated storage when available.
		// It is not a rendering substitute for a feature removed from this engine.
		const bool bLegacyPostProcess = bPostProcess && Property->HasAnyPropertyFlags(CPF_Deprecated);
		if (bLegacyPostProcess && !Values->HasField(ValueName)) ValueName.RemoveFromEnd(TEXT("_DEPRECATED"));
		if (PropertySerializer->ShouldDeserializeProperty(Property) || (bLegacyPostProcess && !PropertySerializer->BlacklistedProperties.Contains(Property))) {
			void* PropertyValue = Property->ContainerPtrToValuePtr<void>(StructValue);
			const bool bHandled = PassthroughPropertyHandler(Property, ValueName, PropertyValue, Values, PropertySerializer, OptionalOuter);
			if (!bHandled && Values->HasField(ValueName)) {
				const TSharedPtr<FJsonValue> Value = Values->TryGetField(ValueName);
				if (Value.IsValid()) PropertySerializer->DeserializePropertyValue(Property, Value.ToSharedRef(), PropertyValue, OptionalOuter);
			}
		}
	}

	if (bOldExposure) {
		// JSON does not pass through the engine's versioned asset loader. Run its
		// real compatibility conversions rather than feeding removed Bokeh/Gaussian
		// settings to Diaphragm DOF or ignoring legacy exposure calibration.
		TArray<uint8> Empty;
		FMemoryReader VersionArchive(Empty);
		VersionArchive.SetCustomVersion(FRenderingObjectVersion::GUID,
			GReflectionRuntime.MinorVersion < 23 ? FRenderingObjectVersion::DiaphragmDOFOnlyForDeferredShadingRenderer - 1 : FRenderingObjectVersion::AutoExposureChanges - 1, NAME_None);
		VersionArchive.SetCustomVersion(FReleaseObjectVersion::GUID,
			GReflectionRuntime.MinorVersion < 23 ? FReleaseObjectVersion::FocalDistanceDisablesDOF - 1 : FReleaseObjectVersion::LatestVersion, NAME_None);
#if ENGINE_UE5
		VersionArchive.SetCustomVersion(FUE5ReleaseStreamObjectVersion::GUID, FUE5ReleaseStreamObjectVersion::LatestVersion, NAME_None);
		VersionArchive.SetCustomVersion(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::LatestVersion, NAME_None);
#endif
		// PostSerialize itself is not DLL-exported; invoke the registered struct
		// operations, exactly as the engine's reflected serialization does.
		if (UScriptStruct::ICppStructOps* Ops = Struct->GetCppStructOps(); Ops && Ops->HasPostSerialize())
			Ops->PostSerialize(VersionArchive, StructValue);
	}

	if (bPostProcess) {
		TArray<FString> Unsupported;
		for (const auto& Pair : JsonValue->Values) {
			const FString Name = JsonKeyToString(Pair.Key);
			if (!Name.StartsWith(TEXT("bOverride_")) || Pair.Value->Type != EJson::Boolean || !Pair.Value->AsBool()) continue;
			if (Name == TEXT("bOverride_BloomConvolutionPreFilter")) continue;
			FProperty* Property = Struct->FindPropertyByName(FName(*Name));
			if (!Property || Property->HasAnyPropertyFlags(CPF_Deprecated)) Unsupported.Add(Name.RightChop(10));
		}
		if (!Unsupported.IsEmpty()) {
			Unsupported.Sort();
			const FString Fields = FString::Join(Unsupported, TEXT(", "));
			const FString ReportKey = GetPathNameSafe(OptionalOuter) + TEXT(":") + Fields;
			if (!ReportedPostProcessOverrides.Contains(ReportKey)) {
				ReportedPostProcessOverrides.Add(ReportKey);
			FImportIssues::Report(EImportIssue::Data, TEXT("Some post process overrides have no active field in this engine"),
				FString::Printf(TEXT("%s: %s. Legacy storage is retained where available, but these features cannot render 1:1 in this engine version."), *GetPathNameSafe(OptionalOuter), *Fields));
			}
		}
	}
	if (Struct->IsChildOf(FRichCurveKey::StaticStruct())) {
		FRichCurveKey* Key = static_cast<FRichCurveKey*>(StructValue);
		if (Key->TangentWeightMode == 183) Key->TangentWeightMode = RCTWM_WeightedNone;
	}
}
