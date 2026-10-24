#pragma once
#include "Commandlets/Commandlet.h"
#include "InventoryCameraRepairCommandlet.generated.h"

// Editor authoring/verification only; gameplay assets use stock Blueprint nodes.
UCLASS()
class UInventoryCameraRepairCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UInventoryCameraRepairCommandlet();
    virtual int32 Main(const FString& Params) override;
};
