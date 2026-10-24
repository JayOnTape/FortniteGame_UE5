#include "InventoryCameraRepairCommandlet.h"
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CallArrayFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "KismetCompiler.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/SavePackage.h"
#include "UObject/StructOnScope.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"

using J=TSharedPtr<FJsonObject>;
static FString Str(J O,const TCHAR* K){FString V;O->TryGetStringField(K,V);return V;}
static UEdGraphPin* P(UEdGraphNode* N,const FString& Name){for(auto* Pin:N->Pins)if(Pin->PinName.ToString()==Name)return Pin;checkf(false,TEXT("Missing pin %s.%s"),*N->GetName(),*Name);return nullptr;}
static void L(UEdGraphPin* A,UEdGraphPin* B){checkf(GetDefault<UEdGraphSchema_K2>()->TryCreateConnection(A,B),TEXT("Cannot connect %s.%s -> %s.%s"),*A->GetOwningNode()->GetName(),*A->PinName.ToString(),*B->GetOwningNode()->GetName(),*B->PinName.ToString());}
static void Save(UObject* O){O->MarkPackageDirty();FSavePackageArgs A;A.TopLevelFlags=RF_Public|RF_Standalone;A.SaveFlags=SAVE_NoError;auto F=FPackageName::LongPackageNameToFilename(O->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension());check(UPackage::SavePackage(O->GetOutermost(),O,*F,A));}
static void Obj(UObject* O,const TCHAR* Name,UObject* V){auto* F=FindFProperty<FObjectPropertyBase>(O->GetClass(),Name);check(F);F->SetObjectPropertyValue_InContainer(O,V);}
static void Num(UObject* O,const TCHAR* Name,double V){auto* F=FindFProperty<FNumericProperty>(O->GetClass(),Name);check(F);F->SetFloatingPointPropertyValue(F->ContainerPtrToValuePtr<void>(O),V);}
static void Bool(UObject* O,const TCHAR* Name,bool V){auto* F=FindFProperty<FBoolProperty>(O->GetClass(),Name);check(F);F->SetPropertyValue_InContainer(O,V);}
static int32 Verify(const FString& Params){
    FString ReportFile;check(FParse::Value(*Params,TEXT("report="),ReportFile));
    auto* BP=LoadObject<UBlueprint>(nullptr,TEXT("/Game/Athena/PlayerPawn_Athena.PlayerPawn_Athena"));check(BP);
    UWorld* W=UWorld::CreateWorld(EWorldType::Game,false);
    FActorSpawnParameters S;S.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Pawn=W->SpawnActor<ACharacter>(BP->GeneratedClass,FVector(0,0,150),FRotator::ZeroRotator,S);check(Pawn);
    auto* PC=W->SpawnActor<APlayerController>();Obj(Pawn,TEXT("Controller"),PC);
    auto* Arm=Pawn->FindComponentByClass<USpringArmComponent>();auto* Camera=Pawn->FindComponentByClass<UCameraComponent>();check(Arm&&Camera&&Pawn->IsLocallyControlled());
    TGuardValue<bool> AllowScript(GAllowActorScriptExecutionInEditor,true); auto* CameraFunc=Pawn->FindFunctionChecked(TEXT("CameraSystem"));
    auto Step=[&](float DT){FStructOnScope Args(CameraFunc);auto* D=FindFProperty<FNumericProperty>(CameraFunc,TEXT("DeltaSeconds"));check(D);D->SetFloatingPointPropertyValue(D->ContainerPtrToValuePtr<void>(Args.GetStructMemory()),DT);Pawn->ProcessEvent(CameraFunc,Args.GetStructMemory());};
    TArray<TSharedPtr<FJsonValue>> Results;
    auto Record=[&](const FString& Name,bool Pass){J O=MakeShared<FJsonObject>();O->SetStringField(TEXT("case"),Name);O->SetBoolField(TEXT("passed"),Pass);O->SetNumberField(TEXT("arm"),Arm->TargetArmLength);O->SetNumberField(TEXT("fov"),Camera->FieldOfView);O->SetStringField(TEXT("pivot"),Arm->TargetOffset.ToString());O->SetStringField(TEXT("socket"),Arm->SocketOffset.ToString());Results.Add(MakeShared<FJsonValueObject>(O));UE_LOG(LogTemp,Warning,TEXT("CAMERA_TEST %s arm=%f fov=%f pass=%d"),*Name,Arm->TargetArmLength,Camera->FieldOfView,Pass);checkf(Pass,TEXT("Camera verification failed: %s"),*Name);};
    auto SetMode=[&](const TCHAR* Name){auto* O=LoadObject<UObject>(nullptr,*FString::Printf(TEXT("/Game/Blueprints/Camera/Athena/%s.%s"),Name,Name));check(O);Obj(Pawn,TEXT("Ref_CurrentCameraMode"),O);Obj(Pawn,TEXT("Ref_CurrentTargetingCamera"),O);Bool(Pawn,TEXT("bIsTargeted"),false);return O;};
    Obj(Pawn,TEXT("Ref_CurrentCameraMode"),nullptr);Obj(Pawn,TEXT("Ref_CurrentTargetingCamera"),nullptr);for(int i=0;i<120;i++)Step(1.f/60);Record(TEXT("Null camera modes recover"),FMath::IsNearlyEqual(Arm->TargetArmLength,280.f,.1f)&&FMath::IsNearlyEqual(Camera->FieldOfView,80.f,.1f));
    for(auto Name:{TEXT("Athena_PlayerCameraModeMelee"),TEXT("Athena_PlayerCameraModeRangedTargeting"),TEXT("Athena_PlayerCameraModeTargetingScope"),TEXT("Athena_PlayerCameraModeSkydiveGlide"),TEXT("Athena_PlayerCameraModeSkydiveParachute"),TEXT("Athena_PlayerCameraModeRanged")}){SetMode(Name);for(int i=0;i<120;i++)Step(1.f/60);Record(Name,Arm->TargetArmLength>=30&&Camera->FieldOfView>=5&&Camera->FieldOfView<=120);if(FString(Name).EndsWith(TEXT("Scope")))Record(TEXT("Scoped 15-degree FOV preserved"),FMath::IsNearlyEqual(Camera->FieldOfView,15.f,.1f));}
    SetMode(TEXT("Athena_PlayerCameraModeRanged"));Obj(Pawn,TEXT("Ref_CurrentTargetingCamera"),nullptr);Bool(Pawn,TEXT("bIsTargeted"),true);for(int i=0;i<120;i++)Step(1.f/60);Record(TEXT("Missing ADS uses valid base"),FMath::IsNearlyEqual(Camera->FieldOfView,80.f,.1f));
    auto* Broken=DuplicateObject(SetMode(TEXT("Athena_PlayerCameraModeRanged")),GetTransientPackage());Num(Broken,TEXT("FOV"),0);Num(Broken,TEXT("CameraSpaceForwardDistance"),0);Obj(Pawn,TEXT("Ref_CurrentCameraMode"),Broken);for(int i=0;i<120;i++)Step(1.f/60);Record(TEXT("Zero FOV and distance recover"),FMath::IsNearlyEqual(Arm->TargetArmLength,280.f,.1f)&&FMath::IsNearlyEqual(Camera->FieldOfView,80.f,.1f));
    auto* Array=FindFProperty<FArrayProperty>(Broken->GetClass(),TEXT("CameraSpaceViewOffset"));check(Array);FScriptArrayHelper(Array,Array->ContainerPtrToValuePtr<void>(Broken)).EmptyValues();for(int i=0;i<120;i++)Step(1.f/60);Record(TEXT("Malformed sample array recovers"),Arm->TargetArmLength>200&&Camera->FieldOfView>70);
    SetMode(TEXT("Athena_PlayerCameraModeRanged"));for(float DT:{1.f/30,1.f/60,1.f/144}){for(int i=0;i<200;i++){PC->SetControlRotation(FRotator(i%2?80:-80,i*9,0));Step(DT);}Record(FString::Printf(TEXT("Pitch/yaw sweep %.0f FPS"),1/DT),FMath::IsNearlyEqual(Arm->TargetOffset.Z,70.f,.1f)&&FMath::IsNearlyEqual(Arm->TargetArmLength,280.f,.1f));}
    auto* InvClass=LoadObject<UClass>(nullptr,TEXT("/Game/Script/Components/BpC_FortInventory.BpC_FortInventory_C"));auto* Inv=NewObject<UActorComponent>(Pawn,InvClass);Obj(Inv,TEXT("PlayerRef"),Pawn);
    auto* Equip=Inv->FindFunctionChecked(TEXT("PlayerEquipWeaponOnMulticast"));FStructOnScope A(Equip);Inv->ProcessEvent(Equip,A.GetStructMemory());Record(TEXT("Null equip actor ignored"),true);
    auto* WeaponClass=LoadObject<UClass>(nullptr,TEXT("/Game/Script/BlueprintActors/Weapon/BP_FortWeaponBase.BP_FortWeaponBase_C"));auto* Dead=W->SpawnActor<AActor>(WeaponClass,FVector::ZeroVector,FRotator::ZeroRotator,S);check(Dead);Dead->Destroy();FindFProperty<FObjectPropertyBase>(Equip,TEXT("PassedWeaponActor"))->SetObjectPropertyValue_InContainer(A.GetStructMemory(),Dead);Inv->ProcessEvent(Equip,A.GetStructMemory());Record(TEXT("Destroyed equip actor ignored"),true);
    auto* Server=Inv->FindFunctionChecked(TEXT("PlayerEquipWeaponOnServer"));for(int Index:{-1,999}){FStructOnScope Args(Server);FindFProperty<FIntProperty>(Server,TEXT("SlotNumber"))->SetPropertyValue_InContainer(Args.GetStructMemory(),Index);Inv->ProcessEvent(Server,Args.GetStructMemory());Record(FString::Printf(TEXT("Invalid inventory slot %d ignored"),Index),true);}
    J Report=MakeShared<FJsonObject>();Report->SetArrayField(TEXT("cases"),Results);Report->SetStringField(TEXT("environment"),TEXT("Headless standalone game world; compiled Blueprint execution, no BeginPlay or network session"));FString Json;FJsonSerializer::Serialize(Report.ToSharedRef(),TJsonWriterFactory<>::Create(&Json));check(FFileHelper::SaveStringToFile(Json,*ReportFile));W->DestroyWorld(false);return 0;
}
UInventoryCameraRepairCommandlet::UInventoryCameraRepairCommandlet(){IsEditor=true;IsClient=false;IsServer=false;LogToConsole=true;}
static int32 VerifyAimPitch(const FString& Params){
    FString ReportFile;check(FParse::Value(*Params,TEXT("report="),ReportFile));
    auto* BP=LoadObject<UBlueprint>(nullptr,TEXT("/Game/Athena/PlayerPawn_Athena.PlayerPawn_Athena"));check(BP);
    UWorld* W=UWorld::CreateWorld(EWorldType::Game,false);FActorSpawnParameters S;S.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Pawn=W->SpawnActor<ACharacter>(BP->GeneratedClass,FVector(0,0,150),FRotator(0,37,0),S);check(Pawn);auto* PC=W->SpawnActor<APlayerController>();Obj(Pawn,TEXT("Controller"),PC);
    auto* Arm=Pawn->FindComponentByClass<USpringArmComponent>();auto* Camera=Pawn->FindComponentByClass<UCameraComponent>();check(Arm&&Camera&&Pawn->IsLocallyControlled());TGuardValue<bool> AllowScript(GAllowActorScriptExecutionInEditor,true);
    auto* CameraFn=Pawn->FindFunctionChecked(TEXT("CameraSystem"));auto* OriginFn=Pawn->FindFunctionChecked(TEXT("GetLookAtPosition"));auto* RotationFn=Pawn->FindFunctionChecked(TEXT("GetWeaponAimRotation"));
    auto Step=[&](){FStructOnScope Args(CameraFn);auto* D=FindFProperty<FNumericProperty>(CameraFn,TEXT("DeltaSeconds"));check(D);D->SetFloatingPointPropertyValue(D->ContainerPtrToValuePtr<void>(Args.GetStructMemory()),1./60);Pawn->ProcessEvent(CameraFn,Args.GetStructMemory());Arm->TickComponent(1.f/60,LEVELTICK_All,nullptr);};
    auto Origin=[&](){FStructOnScope A(OriginFn);Pawn->ProcessEvent(OriginFn,A.GetStructMemory());auto* F=FindFProperty<FStructProperty>(OriginFn,TEXT("Output"));check(F);return *F->ContainerPtrToValuePtr<FVector>(A.GetStructMemory());};
    auto Rotation=[&](){FStructOnScope A(RotationFn);Pawn->ProcessEvent(RotationFn,A.GetStructMemory());auto* F=FindFProperty<FStructProperty>(RotationFn,TEXT("ViewRotation"));check(F);return *F->ContainerPtrToValuePtr<FRotator>(A.GetStructMemory());};
    auto* Target=W->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Target);Target->SetRootComponent(Box);Box->SetBoxExtent(FVector(20));Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Box->SetCollisionResponseToAllChannels(ECR_Block);Box->RegisterComponent();Target->SetActorLocation(FVector(10000,10000,10000));
    TArray<TSharedPtr<FJsonValue>> Cases;int32 Failures=0;
    for(const TCHAR* Mode:{TEXT("Athena_PlayerCameraModeRanged"),TEXT("Athena_PlayerCameraModeRangedTargeting"),TEXT("Athena_PlayerCameraModeTargetingScope")}){
        auto* O=LoadObject<UObject>(nullptr,*FString::Printf(TEXT("/Game/Blueprints/Camera/Athena/%s.%s"),Mode,Mode));check(O);Obj(Pawn,TEXT("Ref_CurrentCameraMode"),O);Obj(Pawn,TEXT("Ref_CurrentTargetingCamera"),O);Bool(Pawn,TEXT("bIsSkydiving"),false);Bool(Pawn,TEXT("bIsDeployingGlider"),false);Bool(Pawn,TEXT("bIsTargeted"),false);
        for(double Yaw:{0.,90.,180.,270.})for(double Pitch:{-80.,-45.,0.,45.,80.}){
            Target->SetActorLocation(FVector(10000,10000,10000));PC->SetControlRotation(FRotator(Pitch,Yaw,0));for(int i=0;i<120;i++)Step();
            FVector Start=Origin();FRotator Aim=Rotation();FVector CameraPos=Camera->GetComponentLocation();FRotator CameraRot=Camera->GetComponentRotation();FVector Orbit=FRotator(0,CameraRot.Yaw,0).UnrotateVector(CameraPos-Arm->GetComponentLocation()-Arm->TargetOffset);
            double OriginError=FVector::Distance(Start,CameraPos);double AngularError=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Aim.Vector(),CameraRot.Vector()),-1.,1.)));
            Target->SetActorLocation(CameraPos+CameraRot.Vector()*1000);FHitResult Hit;FCollisionQueryParams Query;Query.AddIgnoredActor(Pawn);bool HitTarget=W->LineTraceSingleByChannel(Hit,Start,Start+Aim.Vector()*2000,ECC_Visibility,Query)&&Hit.GetActor()==Target;
            bool Bounded=Orbit.Z>=-40.1&&Orbit.Z<=110.1&&Orbit.X<=-.55*Arm->TargetArmLength+.1&&Orbit.X>=-1.25*Arm->TargetArmLength-.1;
            bool Pass=Bounded&&OriginError<.1&&AngularError<.01&&HitTarget;Failures+=!Pass;J R=MakeShared<FJsonObject>();R->SetStringField(TEXT("mode"),Mode);R->SetNumberField(TEXT("pitch"),Pitch);R->SetNumberField(TEXT("yaw"),Yaw);R->SetBoolField(TEXT("passed"),Pass);R->SetBoolField(TEXT("hitCrosshairTarget"),HitTarget);R->SetNumberField(TEXT("originErrorCm"),OriginError);R->SetNumberField(TEXT("directionErrorDegrees"),AngularError);R->SetNumberField(TEXT("cameraHeightAbovePawnCm"),CameraPos.Z-Pawn->GetActorLocation().Z);R->SetStringField(TEXT("orbit"),Orbit.ToString());Cases.Add(MakeShared<FJsonValueObject>(R));
            if(!Pass)UE_LOG(LogTemp,Warning,TEXT("AIM_PITCH_FAIL %s p=%f y=%f origin=%f angle=%f orbit=%s hit=%d"),Mode,Pitch,Yaw,OriginError,AngularError,*Orbit.ToString(),HitTarget);
        }
    }
    // Move a camera-blocking wall behind the pivot and verify the trace follows
    // the retracted lens, then follows its restored position after leaving it.
    auto* Base=LoadObject<UObject>(nullptr,TEXT("/Game/Blueprints/Camera/Athena/Athena_PlayerCameraModeRanged.Athena_PlayerCameraModeRanged"));Obj(Pawn,TEXT("Ref_CurrentCameraMode"),Base);Obj(Pawn,TEXT("Ref_CurrentTargetingCamera"),Base);PC->SetControlRotation(FRotator::ZeroRotator);Target->SetActorLocation(FVector(10000,10000,10000));for(int i=0;i<120;i++)Step();FVector Free=Camera->GetComponentLocation();
    auto* Wall=W->SpawnActor<AActor>();auto* WallBox=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(WallBox);WallBox->SetBoxExtent(FVector(10,500,500));WallBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);WallBox->SetCollisionResponseToAllChannels(ECR_Block);WallBox->RegisterComponent();Wall->SetActorLocation(Arm->GetComponentLocation()+Arm->TargetOffset+FVector(-130,0,0));Step();
    bool Retracted=FVector::Distance(Camera->GetComponentLocation(),Free)>40&&FVector::Distance(Origin(),Camera->GetComponentLocation())<.1;J Collision=MakeShared<FJsonObject>();Collision->SetStringField(TEXT("case"),TEXT("Camera collision updates trace origin"));Collision->SetBoolField(TEXT("passed"),Retracted);Cases.Add(MakeShared<FJsonValueObject>(Collision));Failures+=!Retracted;
    Wall->SetActorLocation(FVector(10000,10000,10000));Step();bool Restored=FVector::Distance(Camera->GetComponentLocation(),Free)<.1&&FVector::Distance(Origin(),Free)<.1;J Recovery=MakeShared<FJsonObject>();Recovery->SetStringField(TEXT("case"),TEXT("Leaving wall restores lens and trace origin"));Recovery->SetBoolField(TEXT("passed"),Restored);Cases.Add(MakeShared<FJsonValueObject>(Recovery));Failures+=!Restored;
    J Report=MakeShared<FJsonObject>();Report->SetArrayField(TEXT("cases"),Cases);Report->SetNumberField(TEXT("failures"),Failures);Report->SetStringField(TEXT("environment"),TEXT("Standalone headless game world; actual spring-arm ticks, camera world transforms and collision raycasts; no full gameplay/network session"));FString Json;FJsonSerializer::Serialize(Report.ToSharedRef(),TJsonWriterFactory<>::Create(&Json));check(FFileHelper::SaveStringToFile(Json,*ReportFile));W->DestroyWorld(false);return Failures?1:0;
}
int32 UInventoryCameraRepairCommandlet::Main(const FString& Params){
    if(FParse::Param(*Params,TEXT("verifyaimpitch")))return VerifyAimPitch(Params);
    if(FParse::Param(*Params,TEXT("verify")))return Verify(Params);
    FString File;check(FParse::Value(*Params,TEXT("recipe="),File));FString Text;check(FFileHelper::LoadFileToString(Text,*File));J Root;check(FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root));
    TArray<UBlueprint*> Changed;
    for(auto BV:Root->GetArrayField(TEXT("blueprints"))){
        auto Spec=BV->AsObject();auto* BP=LoadObject<UBlueprint>(nullptr,*Str(Spec,TEXT("path")));check(BP);
        for(auto GV:Spec->GetArrayField(TEXT("graphs"))){auto GS=GV->AsObject();UEdGraph* G=nullptr;TArray<UEdGraph*> All;BP->GetAllGraphs(All);for(auto* Candidate:All)if(Candidate->GetName()==Str(GS,TEXT("name")))G=Candidate;
            bool NewGraph=false;
            if(!G && GS->HasField(TEXT("inputs"))){
                NewGraph=true;G=FBlueprintEditorUtils::CreateNewGraph(BP,*Str(GS,TEXT("name")),UEdGraph::StaticClass(),UEdGraphSchema_K2::StaticClass());FBlueprintEditorUtils::AddFunctionGraph(BP,G,false,static_cast<UClass*>(nullptr));
                UK2Node_FunctionEntry* Entry=nullptr;for(UEdGraphNode* N:G->Nodes)if(auto* E=Cast<UK2Node_FunctionEntry>(N))Entry=E;check(Entry);Entry->AddExtraFlags(FUNC_Public|FUNC_BlueprintCallable|FUNC_BlueprintPure);
                auto Type=[](J O){FEdGraphPinType T;T.PinCategory=*Str(O,TEXT("type"));if(T.PinCategory==TEXT("double")){T.PinCategory=UEdGraphSchema_K2::PC_Real;T.PinSubCategory=UEdGraphSchema_K2::PC_Double;}FString Object=Str(O,TEXT("object"));if(!Object.IsEmpty()){T.PinSubCategoryObject=LoadObject<UObject>(nullptr,*Object);check(T.PinSubCategoryObject.IsValid());}return T;};
                for(auto V:GS->GetArrayField(TEXT("inputs"))){auto I=V->AsObject();Entry->CreateUserDefinedPin(*Str(I,TEXT("name")),Type(I),EGPD_Output);}
                auto* Result=NewObject<UK2Node_FunctionResult>(G,TEXT("RepairResult"));Result->FunctionReference.SetSelfMember(*Str(GS,TEXT("name")));Result->CreateNewGuid();G->AddNode(Result,false,false);Result->PostPlacedNewNode();Result->AllocateDefaultPins();
                for(auto V:GS->GetArrayField(TEXT("outputs"))){auto I=V->AsObject();Result->CreateUserDefinedPin(*Str(I,TEXT("name")),Type(I),EGPD_Input);}
                Entry->Rename(TEXT("RepairEntry"),G);Entry->NodePosX=-600;Result->NodePosX=2200;
            }
            check(G);
            TMap<FString,UEdGraphNode*> Nodes;for(UEdGraphNode* N:G->Nodes)Nodes.Add(N->GetName(),N);
            for(auto NV:GS->GetArrayField(TEXT("nodes"))){auto NS=NV->AsObject();FString Kind=Str(NS,TEXT("kind")),Id=Str(NS,TEXT("id"));check(!Nodes.Contains(Id));UEdGraphNode* N=nullptr;
                if(Kind==TEXT("call")||Kind==TEXT("array")){auto* K=Kind==TEXT("array")?static_cast<UK2Node_CallFunction*>(NewObject<UK2Node_CallArrayFunction>(G,*Id)):NewObject<UK2Node_CallFunction>(G,*Id);FString Class=Str(NS,TEXT("class"));if(Class.IsEmpty())K->FunctionReference.SetSelfMember(*Str(NS,TEXT("function")));else{auto* C=LoadObject<UClass>(nullptr,*Class);check(C);auto* F=C->FindFunctionByName(*Str(NS,TEXT("function")));check(F);K->SetFromFunction(F);}N=K;}
                else if(Kind==TEXT("get")||Kind==TEXT("set")){UK2Node_Variable* K=Kind==TEXT("get")?static_cast<UK2Node_Variable*>(NewObject<UK2Node_VariableGet>(G,*Id)):NewObject<UK2Node_VariableSet>(G,*Id);FString Class=Str(NS,TEXT("class"));if(Class.IsEmpty())K->VariableReference.SetSelfMember(*Str(NS,TEXT("variable")));else{auto* C=LoadObject<UClass>(nullptr,*Class);check(C);K->VariableReference.SetExternalMember(*Str(NS,TEXT("variable")),C);}N=K;}
                else if(Kind==TEXT("branch"))N=NewObject<UK2Node_IfThenElse>(G,*Id);
                check(N);N->CreateNewGuid();G->AddNode(N,false,false);N->PostPlacedNewNode();N->AllocateDefaultPins();N->NodePosX=NS->GetIntegerField(TEXT("x"));N->NodePosY=NS->GetIntegerField(TEXT("y"));N->NodeComment=Str(NS,TEXT("comment"));N->bCommentBubbleVisible=!N->NodeComment.IsEmpty();Nodes.Add(Id,N);
            }
            for(auto DV:GS->GetArrayField(TEXT("disconnect"))){auto A=DV->AsArray();P(Nodes.FindChecked(A[0]->AsString()),A[1]->AsString())->BreakAllPinLinks();}
            for(auto DV:GS->GetArrayField(TEXT("defaults"))){auto D=DV->AsObject();auto* Pin=P(Nodes.FindChecked(Str(D,TEXT("node"))),Str(D,TEXT("pin")));FString Obj=Str(D,TEXT("object"));if(!Obj.IsEmpty()){Pin->DefaultObject=LoadObject<UObject>(nullptr,*Obj);check(Pin->DefaultObject);Pin->GetOwningNode()->PinDefaultValueChanged(Pin);}else GetDefault<UEdGraphSchema_K2>()->TrySetDefaultValue(*Pin,Str(D,TEXT("value")));}
            for(auto LV:GS->GetArrayField(TEXT("links"))){auto A=LV->AsArray();L(P(Nodes.FindChecked(A[0]->AsString()),A[1]->AsString()),P(Nodes.FindChecked(A[2]->AsString()),A[3]->AsString()));}
            if(NewGraph){FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);FCompilerResultsLog Log;FKismetEditorUtilities::CompileBlueprint(BP,EBlueprintCompileOptions::None,&Log);check(Log.NumErrors==0);}
        }
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);FCompilerResultsLog Log;FKismetEditorUtilities::CompileBlueprint(BP,EBlueprintCompileOptions::None,&Log);checkf(Log.NumErrors==0,TEXT("Compile failed %s (%d)"),*BP->GetName(),Log.NumErrors);Changed.Add(BP);
    }
    // Save only after every affected Blueprint compiled successfully.
    for(auto* BP:Changed){Save(BP);UE_LOG(LogTemp,Display,TEXT("CAMERA_REPAIR compiled/saved %s"),*BP->GetPathName());}
    return 0;
}





