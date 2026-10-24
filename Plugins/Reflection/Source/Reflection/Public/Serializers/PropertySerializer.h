/* Copyright Reflection Contributors 2024-2026 */

#pragma once

#include "ObjectSerializer.h"
#include "Containers/ExportContainer.h"
#include "Dom/JsonObject.h"
#include "Structs/StructSerializer.h"
#include "UObject/Object.h"
#include "UObject/UnrealType.h"
#include "Engine/Compatibility.h"
#include "PropertySerializer.generated.h"

class IImporter;

UCLASS()
class REFLECTION_API UPropertySerializer : public UObject
{
	GENERATED_BODY()
public:
	UPropertySerializer();

	/** Disables property serialization entirely */
	void DisablePropertySerialization(const UStruct* Struct, FName PropertyName);
	void AddStructSerializer(UScriptStruct* Struct, const TSharedPtr<FStructSerializer>& Serializer);

	/** Checks whenever we should serialize property in question at all */
	bool ShouldDeserializeProperty(FProperty* Property) const;

	void DeserializePropertyValue(FProperty* Property, const TSharedRef<FJsonValue>& Value, void* OutValue, UObject* OptionalOuter = nullptr);
	void DeserializeStruct(UScriptStruct* Struct, const TSharedRef<FJsonObject>& Value, void* OutValue, UObject* OptionalOuter = nullptr) const;

	/* Moves an expression input that points at a rebuilt MaterialExpressionConvert onto the root
	 * standing in for the output it named. No-op unless ConvertOutputRoots has entries. */
	void RemapConvertOutput(const UStruct* Struct, void* StructValue) const;
private:
	FStructSerializer* GetStructSerializer(const UScriptStruct* Struct) const;
	
public:
	friend class UObjectSerializer;

	UPROPERTY()
	UObjectSerializer* ObjectSerializer;

	IImporter* Importer;
	
	TArray<FProperty*> BlacklistedProperties;
	TSharedPtr<FStructSerializer> FallbackStructSerializer;
	TMap<UScriptStruct*, TSharedPtr<FStructSerializer>> StructSerializers;

	bool FallbackToParentTrace = true;

	FUObjectExportContainer* ExportsContainer = nullptr;
	TArray<FString> BlacklistedPropertyNames;

	/* MaterialExpressionConvert has no class before 5.6, so IMaterialGraph rebuilds one as a
	 * network with a separate root expression per convert output. Keyed by the expression the
	 * convert export hands out to everything referencing it (the root of its first output), the
	 * value holds a root per output index. Empty for every asset that has no such node. */
	TMap<UObject*, TArray<UObject*>> ConvertOutputRoots;
};

/* Use to handle differentiating formats produced by UEParse */
inline bool PassthroughPropertyHandler(FProperty* Property, const FString& PropertyName, void* PropertyValue, const TSharedPtr<FJsonObject>& Properties, UPropertySerializer* PropertySerializer, UObject* OptionalOuter = nullptr) {
	/* Handles static arrays in the format of: PropertyName[Index] */
	if (Property->ArrayDim != 1) {
		/* Sized to the property and never grown past it. A static array lives inside the object
		 * rather than in an allocation of its own, so element ArrayDim would be written over
		 * whatever follows the object in memory. The index comes out of json this engine did not
		 * write: a newer one can declare the array longer than it is here, and the prefix match
		 * below answers to a longer property's key besides, so an index this array has no room for
		 * is something to expect rather than something to trust. */
		TArray<TSharedPtr<FJsonValue>> ArrayElements;
		ArrayElements.SetNum(Property->ArrayDim);

		// UEParse also emits fixed arrays as a single JSON array. Missing entries are
		// inherited defaults, not zero values.
		if (const TSharedPtr<FJsonValue> Whole = Properties->TryGetField(PropertyName)) {
			if (Whole->Type == EJson::Array) {
				const auto& Values = Whole->AsArray();
				for (int32 Index = 0; Index < FMath::Min(Values.Num(), Property->ArrayDim); ++Index) {
					ArrayElements[Index] = Values[Index];
				}
			} else {
				ArrayElements[0] = Whole;
			}
		}

		/* Finds array elements with the format: PropertyName[Index] and sets them properly into an array */
		for (const auto& Pair : Properties->Values) {
			const FString Key = JsonKeyToString(Pair.Key);
			const TSharedPtr<FJsonValue> Value = Pair.Value;

			/* If it doesn't start with the same property name */
			if (!Key.StartsWith(PropertyName + TEXT("[")) || !Key.EndsWith(TEXT("]"))) continue;

			/* By default, it should be 0 */
			int32 CurrentArrayIndex = INDEX_NONE;

			/* If it is formatted like PropertyName[Index] */
			if (Key.Contains("[") && Key.Contains("]")) {
				int32 OpenBracketPos, CloseBracketPos;

				/* Find the index in PropertyName[Index] (integer) */
				if (Key.FindChar('[', OpenBracketPos) && Key.FindChar(']', CloseBracketPos) && CloseBracketPos > OpenBracketPos) {
					FString IndexStr = Key.Mid(OpenBracketPos + 1, CloseBracketPos - OpenBracketPos - 1);
					
					if (!IndexStr.IsEmpty() && IndexStr.IsNumeric() && !IndexStr.StartsWith(TEXT("-"))) {
						CurrentArrayIndex = FCString::Atoi(*IndexStr);
					}
				}
			}

			/* Nothing to write it into, so it is dropped rather than written past the end */
			if (!ArrayElements.IsValidIndex(CurrentArrayIndex)) {
				continue;
			}

			ArrayElements[CurrentArrayIndex] = Value;
		}

		/* Array elements is filled up, now we set them in the property value */
		for (int32 ArrayIndex = 0; ArrayIndex < ArrayElements.Num(); ArrayIndex++) {
			uint8* ArrayPropertyValue = static_cast<uint8*>(PropertyValue) + GetElementSize(Property) * ArrayIndex;

			if (!ArrayElements.IsValidIndex(ArrayIndex)) continue;

			TSharedPtr<FJsonValue> ArrayJsonElement = ArrayElements[ArrayIndex];

			/* Check to see if it's null */
			if (!ArrayJsonElement.IsValid()) continue;
			if (ArrayJsonElement->IsNull() && !CastField<FObjectPropertyBase>(Property)) continue;
			
			const TSharedRef<FJsonValue> ArrayJsonValue = ArrayJsonElement.ToSharedRef();

			PropertySerializer->DeserializePropertyValue(Property, ArrayJsonValue, ArrayPropertyValue, OptionalOuter);
		}

		return true;
	}

	return false;
}
