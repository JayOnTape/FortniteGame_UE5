/* Copyright Reflection Contributors 2024-2026 */

#pragma once

#include "StructSerializer.h"

class UPropertySerializer;

/** Fallback struct serializer using default UE reflection */
class FFallbackStructSerializer : public FStructSerializer {
	UPropertySerializer* PropertySerializer;
	// Defaults are read before and after Blueprint compilation, and through
	// component references. Report each unsupported override set once per owner.
	TSet<FString> ReportedPostProcessOverrides;
	
public:
	explicit FFallbackStructSerializer(UPropertySerializer* PropertySerializer) : PropertySerializer(PropertySerializer) { }
	virtual void Deserialize(UScriptStruct* Struct, void* StructValue, const TSharedPtr<FJsonObject> JsonValue, UObject* OptionalOuter) override;
};
