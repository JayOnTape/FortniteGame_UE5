#pragma once
#include "Commandlets/Commandlet.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameFramework/GameModeBase.h"
#include "BuildingEditSimplifyCommandlet.generated.h"
UCLASS()
class ABuildingEditTestGameMode : public AGameModeBase {
 GENERATED_BODY()
public:
 virtual void InitGame(const FString& MapName,const FString& Options,FString& ErrorMessage) override;
};
UCLASS()
class UBuildingEditSimplifyCommandlet : public UCommandlet {
 GENERATED_BODY()
public:
 UBuildingEditSimplifyCommandlet();
 virtual int32 Main(const FString& Params) override;
};
UCLASS()
class UBuildingEditTestLibrary : public UBlueprintFunctionLibrary {
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable) static void StartTest(bool Dedicated=false);
 UFUNCTION(BlueprintCallable) static FString Read(UObject* Object,const FString& Property);
 UFUNCTION(BlueprintCallable) static bool Write(UObject* Object,const FString& Property,const FString& Value);
 UFUNCTION(BlueprintCallable) static FString Invoke(UObject* Object,const FString& Function,const FString& Arguments=TEXT("{}"),bool Remote=false);
 UFUNCTION(BlueprintCallable) static AActor* SpawnBuilding(UWorld* World,int32 Kind,FVector Location,UClass* Resource,AActor* Owner);
 UFUNCTION(BlueprintCallable) static AActor* SpawnGrid(UWorld* World);
 UFUNCTION(BlueprintCallable) static bool Edit(UActorComponent* Manager,AActor* Building,const TArray<bool>& Tiles,int32 StartCell,int32 Revision);
};
