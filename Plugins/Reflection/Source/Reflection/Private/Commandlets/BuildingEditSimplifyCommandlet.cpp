#include "BuildingEditSimplifyCommandlet.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/BlueprintSetLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CallArrayFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_Event.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_Self.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_MakeArray.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Timeline.h"
#include "Misc/PackageName.h"
#include "Misc/FileHelper.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "PlayInEditorDataTypes.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraActor.h"
#include "K2Node_SpawnActorFromClass.h"
#include "Components/BoxComponent.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "K2Node_AddDelegate.h"
#include "K2Node_CreateDelegate.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/SceneCapture2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HAL/FileManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "K2Node_CallParentFunction.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_GetArrayItem.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet2/StructureEditorUtils.h"
#include "UserDefinedStructure/UserDefinedStructEditorData.h"
#include "K2Node_Knot.h"
#include "K2Node_Variable.h"
#include "Internationalization/Regex.h"

namespace BuildSimplify {
const TCHAR* Path=TEXT("/Game/Script/BlueprintActors/Building/BaseActors/FortStaticMeshActor.FortStaticMeshActor");
UBlueprint* BP;UClass* ActorClass;
const UEdGraphSchema_K2* Schema(){return GetDefault<UEdGraphSchema_K2>();}
UEdGraphPin* WPin(UEdGraphNode* N,const TCHAR* Name){auto* Pin=N->FindPin(Name);if(!Pin){FString Names;for(auto* X:N->Pins)Names+=X->PinName.ToString()+TEXT(",");UE_LOG(LogTemp,Fatal,TEXT("Missing %s on %s: %s"),Name,*N->GetNodeTitle(ENodeTitleType::ListView).ToString(),*Names);}return Pin;}
void WLink(UEdGraphPin* A,UEdGraphPin* B){checkf(Schema()->TryCreateConnection(A,B),TEXT("Cannot link %s.%s -> %s.%s"),*A->GetOwningNode()->GetName(),*A->PinName.ToString(),*B->GetOwningNode()->GetName(),*B->PinName.ToString());}
void WDefault(UEdGraphPin* Pin,const FString& V){Schema()->TrySetDefaultValue(*Pin,V);}
void WObject(UEdGraphPin* Pin,UObject* V){Schema()->TrySetDefaultObject(*Pin,V);}
template<class T> T* WNode(UEdGraph* G){auto* V=NewObject<T>(G);V->CreateNewGuid();G->AddNode(V,false,false);return V;}
void WFinish(UEdGraphNode* V){V->PostPlacedNewNode();V->AllocateDefaultPins();}
FEdGraphPinType Type(FName Cat,UObject* Obj=nullptr,EPinContainerType Cont=EPinContainerType::None){FEdGraphPinType T;T.PinCategory=Cat;T.PinSubCategoryObject=Obj;T.ContainerType=Cont;if(Cat==UEdGraphSchema_K2::PC_Real)T.PinSubCategory=UEdGraphSchema_K2::PC_Double;return T;}
FEdGraphPinType Bool(){return Type(UEdGraphSchema_K2::PC_Boolean);}FEdGraphPinType Number(){return Type(UEdGraphSchema_K2::PC_Real);}FEdGraphPinType Int(){return Type(UEdGraphSchema_K2::PC_Int);}FEdGraphPinType Piece(EPinContainerType C=EPinContainerType::None){return Type(UEdGraphSchema_K2::PC_Object,ActorClass,C);}
void Variable(const TCHAR* Name,FEdGraphPinType T,const TCHAR* Default,const TCHAR* Tooltip,bool Edit=false,bool Rep=false){checkf(!BP->NewVariables.ContainsByPredicate([&](auto& V){return V.VarName==Name;}),TEXT("Variable exists: %s"),Name);check(FBlueprintEditorUtils::AddMemberVariable(BP,Name,T,Default));auto& V=BP->NewVariables.Last();V.Category=FText::FromString(TEXT("Destruction and Support"));V.SetMetaData(TEXT("Tooltip"),Tooltip);if(Edit)V.PropertyFlags&=~CPF_DisableEditOnInstance;if(!Edit){V.PropertyFlags|=CPF_Transient|CPF_DisableEditOnInstance;V.SetMetaData(TEXT("BlueprintPrivate"),TEXT("true"));}if(Rep){V.PropertyFlags|=CPF_Net|CPF_RepNotify;V.RepNotifyFunc=TEXT("OnRep_bDestroyed");}}
void WCompile(){UE_LOG(LogTemp,Warning,TEXT("BUILD_SIMPLIFY_COMPILE %s"),*BP->GetName());FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);FCompilerResultsLog Log;FKismetEditorUtilities::CompileBlueprint(BP,EBlueprintCompileOptions::SkipSave|EBlueprintCompileOptions::SkipGarbageCollection,&Log);checkf(!Log.NumErrors,TEXT("Blueprint has %d compile errors"),Log.NumErrors);ActorClass=BP->GeneratedClass;}
void WSave(UObject* Obj){Obj->MarkPackageDirty();FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;check(UPackage::SavePackage(Obj->GetOutermost(),Obj,*FPackageName::LongPackageNameToFilename(Obj->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension()),Args));}
struct F {
 UEdGraph* G;UK2Node_FunctionEntry* E;
 F(UEdGraph* Graph,bool Clear=false):G(Graph),E(nullptr){for(UEdGraphNode* N:G->Nodes)if(auto* V=Cast<UK2Node_FunctionEntry>(N))E=V;if(Clear){auto Nodes=G->Nodes;for(UEdGraphNode* N:Nodes)if(N!=E&&!N->IsA<UK2Node_FunctionResult>())N->DestroyNode();for(UEdGraphNode* N:G->Nodes)N->BreakAllNodeLinks();}}
 F(const TCHAR* Name){G=FBlueprintEditorUtils::CreateNewGraph(BP,Name,UEdGraph::StaticClass(),UEdGraphSchema_K2::StaticClass());FBlueprintEditorUtils::AddFunctionGraph(BP,G,false,static_cast<UClass*>(nullptr));E=nullptr;for(UEdGraphNode* V:G->Nodes)if(auto* T=Cast<UK2Node_FunctionEntry>(V))E=T;check(E);E->AddExtraFlags(FUNC_Public|FUNC_BlueprintCallable);E->MetaData.Category=FText::FromString(TEXT("Destruction and Support"));}
 UEdGraphPin* In(const TCHAR* Name,FEdGraphPinType T){return E->CreateUserDefinedPin(Name,T,EGPD_Output);}
 UK2Node_FunctionResult* Ret(){auto* R=WNode<UK2Node_FunctionResult>(G);R->FunctionReference=E->FunctionReference;WFinish(R);return R;}
 void Out(const TCHAR* Name,FEdGraphPinType T){UK2Node_FunctionResult* R=nullptr;for(UEdGraphNode* N:G->Nodes)if(auto* V=Cast<UK2Node_FunctionResult>(N))R=V;if(!R)R=Ret();R->CreateUserDefinedPin(Name,T,EGPD_Input);}
 void Local(const TCHAR* Name,FEdGraphPinType T,const TCHAR* Default=TEXT("")){FBPVariableDescription V;V.VarName=Name;V.VarGuid=FGuid::NewGuid();V.VarType=T;V.DefaultValue=Default;V.PropertyFlags=CPF_BlueprintVisible;E->LocalVariables.Add(V);}
 void Ref(FMemberReference& R,const TCHAR* Name,bool External=false){if(External){R.SetExternalMember(Name,ActorClass);return;}if(E)for(auto& V:E->LocalVariables)if(V.VarName==Name){R.SetLocalMember(Name,G->GetName(),V.VarGuid);return;}R.SetSelfMember(Name);}
 UEdGraphPin* Self(){auto* V=WNode<UK2Node_Self>(G);WFinish(V);return WPin(V,TEXT("self"));}
 UEdGraphPin* Get(const TCHAR* Name,UEdGraphPin* Target=nullptr){auto* V=WNode<UK2Node_VariableGet>(G);Ref(V->VariableReference,Name,Target!=nullptr);WFinish(V);if(Target)WLink(Target,WPin(V,TEXT("self")));return WPin(V,Name);}
 UK2Node_VariableSet* Set(const TCHAR* Name,UEdGraphPin* Value=nullptr,UEdGraphPin* Target=nullptr,const TCHAR* Def=nullptr){auto* V=WNode<UK2Node_VariableSet>(G);Ref(V->VariableReference,Name,Target!=nullptr);WFinish(V);if(Target)WLink(Target,WPin(V,TEXT("self")));if(Value)WLink(Value,WPin(V,Name));if(Def)WDefault(WPin(V,Name),Def);return V;}
 UK2Node_CallFunction* Call(UClass* C,const TCHAR* Name){auto* V=WNode<UK2Node_CallFunction>(G);auto* Fn=C->FindFunctionByName(Name);checkf(Fn,TEXT("No function: %s"),Name);V->SetFromFunction(Fn);WFinish(V);return V;}
 UK2Node_CallFunction* Call(const TCHAR* Name,UEdGraphPin* Target=nullptr){auto* V=WNode<UK2Node_CallFunction>(G);V->FunctionReference.SetSelfMember(Name);WFinish(V);if(Target)WLink(Target,WPin(V,TEXT("self")));return V;}
 UK2Node_CallFunction* Arr(const TCHAR* Name,UEdGraphPin* Array,UEdGraphPin* Item=nullptr){auto* V=WNode<UK2Node_CallArrayFunction>(G);V->SetFromFunction(UKismetArrayLibrary::StaticClass()->FindFunctionByName(Name));WFinish(V);WLink(Array,WPin(V,TEXT("TargetArray")));if(Item){auto* Input=V->FindPin(TEXT("NewItem"));if(!Input)Input=V->FindPin(TEXT("ItemToRemove"));if(!Input)Input=WPin(V,TEXT("Item"));WLink(Item,Input);}return V;}
 UK2Node_CallFunction* SetOp(const TCHAR* Name,UEdGraphPin* Set,UEdGraphPin* Item){auto* V=Call(UBlueprintSetLibrary::StaticClass(),Name);WLink(Set,WPin(V,TEXT("TargetSet")));WLink(Item,WPin(V,TEXT("NewItem")));return V;}
 UK2Node_IfThenElse* If(UEdGraphPin* Cond){auto* V=WNode<UK2Node_IfThenElse>(G);WFinish(V);WLink(Cond,WPin(V,TEXT("Condition")));return V;}
 UEdGraphPin* Math(const TCHAR* Name,UEdGraphPin* A,UEdGraphPin* B=nullptr,const TCHAR* Def=nullptr){auto* V=Call(UKismetMathLibrary::StaticClass(),Name);WLink(A,WPin(V,TEXT("A")));if(B)WLink(B,WPin(V,TEXT("B")));else if(Def)WDefault(WPin(V,TEXT("B")),Def);return WPin(V,TEXT("ReturnValue"));}
 UEdGraphPin* Not(UEdGraphPin* A){return Math(TEXT("Not_PreBool"),A);}UEdGraphPin* And(UEdGraphPin* A,UEdGraphPin* B){return Math(TEXT("BooleanAND"),A,B);}UEdGraphPin* Or(UEdGraphPin* A,UEdGraphPin* B){return Math(TEXT("BooleanOR"),A,B);}
 UEdGraphPin* Valid(UEdGraphPin* A){auto* V=Call(UKismetSystemLibrary::StaticClass(),TEXT("IsValid"));WLink(A,WPin(V,TEXT("Object")));return WPin(V,TEXT("ReturnValue"));}
 UEdGraphPin* Authority(){return WPin(Call(AActor::StaticClass(),TEXT("HasAuthority")),TEXT("ReturnValue"));}
 UK2Node_MacroInstance* Macro(const TCHAR* Name){auto* Macros=LoadObject<UBlueprint>(nullptr,TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros"));check(Macros);auto* MG=Macros->MacroGraphs.FindByPredicate([&](const TObjectPtr<UEdGraph>& Graph){return Graph->GetFName()==Name;});checkf(MG,TEXT("No macro %s"),Name);auto* V=WNode<UK2Node_MacroInstance>(G);V->SetMacroGraph(*MG);WFinish(V);return V;}
 UK2Node_MacroInstance* Each(UEdGraphPin* Array){auto* V=Macro(TEXT("ForEachLoop"));WLink(Array,WPin(V,TEXT("Array")));return V;}
 UK2Node_DynamicCast* CastPiece(UEdGraphPin* Obj){auto* V=WNode<UK2Node_DynamicCast>(G);V->TargetType=ActorClass;WFinish(V);WLink(Obj,WPin(V,TEXT("Object")));return V;}
 UEdGraphPin* CastResult(UK2Node_DynamicCast* V){return V->GetCastResultPin();}
 UK2Node_CallFunction* Timer(const TCHAR* Name,UEdGraphPin* Target=nullptr,const TCHAR* Time=TEXT("0.05"),UEdGraphPin* TimePin=nullptr){auto* V=Call(UKismetSystemLibrary::StaticClass(),TEXT("K2_SetTimer"));WLink(Target?Target:Self(),WPin(V,TEXT("Object")));WDefault(WPin(V,TEXT("FunctionName")),Name);if(TimePin)WLink(TimePin,WPin(V,TEXT("Time")));else WDefault(WPin(V,TEXT("Time")),Time);return V;}
 UEdGraphPin* MakeArray(FEdGraphPinType T,TArray<UEdGraphPin*> Items={},TArray<FString> Defaults={}){auto* V=WNode<UK2Node_MakeArray>(G);WFinish(V);auto* Out=WPin(V,TEXT("Array"));T.ContainerType=EPinContainerType::Array;Out->PinType=T;V->PinTypeChanged(Out);int Count=FMath::Max(Items.Num(),Defaults.Num());for(int I=1;I<Count;++I)V->AddInputPin();for(int I=0;I<Count;++I){auto* In=WPin(V,*FString::Printf(TEXT("[%d]"),I));In->PinType=T;In->PinType.ContainerType=EPinContainerType::None;Schema()->SetPinAutogeneratedDefaultValueBasedOnType(In);if(Items.IsValidIndex(I)&&Items[I])WLink(Items[I],In);else if(Defaults.IsValidIndex(I))WDefault(In,Defaults[I]);}return Out;}
 UEdGraphPin* ObjectTypes(bool StaticOnly=false){auto T=Type(UEdGraphSchema_K2::PC_Byte,StaticEnum<EObjectTypeQuery>());return MakeArray(T,{},StaticOnly?TArray<FString>{TEXT("ObjectTypeQuery1")}:TArray<FString>{TEXT("ObjectTypeQuery1"),TEXT("ObjectTypeQuery2")});}
 UK2Node_CallFunction* Bounds(){auto* V=Call(UKismetSystemLibrary::StaticClass(),TEXT("GetComponentBounds"));WLink(Get(TEXT("StaticMeshComponent")),WPin(V,TEXT("Component")));return V;}
 UK2Node_CallFunction* BreakVector(UEdGraphPin* Vec){auto* V=Call(UKismetMathLibrary::StaticClass(),TEXT("BreakVector"));WLink(Vec,WPin(V,TEXT("InVec")));return V;}
 UEdGraphPin* Vector(UEdGraphPin* X,UEdGraphPin* Y,UEdGraphPin* Z){auto* V=Call(UKismetMathLibrary::StaticClass(),TEXT("MakeVector"));if(X)WLink(X,WPin(V,TEXT("X")));if(Y)WLink(Y,WPin(V,TEXT("Y")));if(Z)WLink(Z,WPin(V,TEXT("Z")));return WPin(V,TEXT("ReturnValue"));}
 UK2Node_CallFunction* Trace(UEdGraphPin* Start,UEdGraphPin* End,UEdGraphPin* Extent){auto* V=Call(UKismetSystemLibrary::StaticClass(),TEXT("BoxTraceMultiForObjects"));WLink(Start,WPin(V,TEXT("Start")));WLink(End,WPin(V,TEXT("End")));WLink(Extent,WPin(V,TEXT("HalfSize")));WLink(ObjectTypes(true),WPin(V,TEXT("ObjectTypes")));WLink(MakeArray(Type(UEdGraphSchema_K2::PC_Object,AActor::StaticClass()),{Self()}),WPin(V,TEXT("ActorsToIgnore")));WDefault(WPin(V,TEXT("DrawDebugType")),TEXT("None"));WDefault(WPin(V,TEXT("bIgnoreSelf")),TEXT("true"));return V;}
 void Comment(UEdGraphNode* V,const TCHAR* Text){V->NodeComment=Text;V->bCommentBubbleVisible=true;}
};
UEdGraphPin* Exec(UEdGraphNode* V){auto* P=V->FindPin(TEXT("execute"));return P?P:WPin(V,TEXT("Exec"));}UEdGraphPin* Then(UEdGraphNode* V){return WPin(V,TEXT("then"));}
void Chain(UEdGraphPin*& Tail,UEdGraphNode* Node){WLink(Tail,Exec(Node));Tail=Then(Node);}
void Layout(UEdGraph* G){
 TMap<UEdGraphNode*,int32> Depth;for(UEdGraphNode* Node:G->Nodes)Depth.Add(Node,0);
 for(int32 Pass=0;Pass<G->Nodes.Num();++Pass){bool Changed=false;for(UEdGraphNode* Node:G->Nodes){int32 D=0;for(auto* Pin:Node->Pins)if(Pin->Direction==EGPD_Input)for(auto* Source:Pin->LinkedTo)if(Source->GetOwningNode()!=Node&&Depth.Contains(Source->GetOwningNode()))D=FMath::Max(D,Depth[Source->GetOwningNode()]+1);D=FMath::Min(D,64);if(D>Depth[Node]){Depth[Node]=D;Changed=true;}}if(!Changed)break;}
 TArray<UEdGraphNode*> Nodes;for(UEdGraphNode* Node:G->Nodes)Nodes.Add(Node);
 Nodes.StableSort([&](const UEdGraphNode& A,const UEdGraphNode& B){auto IsExec=[](const UEdGraphNode& Node){for(auto* P:Node.Pins)if(P->PinType.PinCategory==UEdGraphSchema_K2::PC_Exec)return true;return false;};return IsExec(A)&&!IsExec(B);});
 TMap<int32,int32> Y;for(auto* Node:Nodes){int32 Layer=Depth[Node];Node->NodePosX=Layer*500;Node->NodePosY=Y.FindOrAdd(Layer);int32 Inputs=0,Outputs=0;for(auto* Pin:Node->Pins)if(!Pin->bHidden){if(Pin->Direction==EGPD_Input)++Inputs;else ++Outputs;}Y[Layer]+=FMath::Max(180,80+32*FMath::Max(Inputs,Outputs))+100;}
}
}
namespace BuildSimplify {
FEdGraphPinType Vec(){return Type(UEdGraphSchema_K2::PC_Struct,TBaseStructure<FVector>::Get());}
UEdGraphPin* Field(F& G,const TCHAR* Name,UClass* Owner,UEdGraphPin* Target){auto* V=WNode<UK2Node_VariableGet>(G.G);V->VariableReference.SetExternalMember(Name,Owner);WFinish(V);WLink(Target,WPin(V,TEXT("self")));return WPin(V,Name);}
UEdGraphPin* Normal(F& G,UEdGraphPin* V){auto* C=G.Call(UKismetMathLibrary::StaticClass(),TEXT("Normal"));WLink(V,WPin(C,TEXT("A")));return WPin(C,TEXT("ReturnValue"));}
UEdGraphPin* Distance(F& G,UEdGraphPin* A,UEdGraphPin* B){auto* C=G.Call(UKismetMathLibrary::StaticClass(),TEXT("Vector_Distance"));WLink(A,WPin(C,TEXT("V1")));WLink(B,WPin(C,TEXT("V2")));return WPin(C,TEXT("ReturnValue"));}
UEdGraphPin* Location(F& G,UEdGraphPin* Actor=nullptr){auto* C=G.Call(AActor::StaticClass(),TEXT("K2_GetActorLocation"));if(Actor)WLink(Actor,WPin(C,TEXT("self")));return WPin(C,TEXT("ReturnValue"));}
UEdGraphPin* Extend(F& G,UEdGraphPin* From,UEdGraphPin* To,const TCHAR* Amount){return G.Math(TEXT("Add_VectorVector"),To,G.Math(TEXT("Multiply_VectorFloat"),Normal(G,G.Math(TEXT("Subtract_VectorVector"),To,From)),nullptr,Amount));}
UK2Node_CallFunction* Ray(F& G,UEdGraphPin* From,UEdGraphPin* To,bool Sweep=false){auto* C=G.Call(UKismetSystemLibrary::StaticClass(),Sweep?TEXT("SphereTraceSingle"):TEXT("LineTraceSingle"));WLink(From,WPin(C,TEXT("Start")));WLink(To,WPin(C,TEXT("End")));WDefault(WPin(C,TEXT("TraceChannel")),TEXT("TraceTypeQuery8"));WDefault(WPin(C,TEXT("bTraceComplex")),TEXT("true"));if(Sweep)WDefault(WPin(C,TEXT("Radius")),TEXT("4"));WLink(G.MakeArray(Type(UEdGraphSchema_K2::PC_Object,AActor::StaticClass()),{G.Self(),G.Get(TEXT("PlayerPawn"))}),WPin(C,TEXT("ActorsToIgnore")));WDefault(WPin(C,TEXT("bIgnoreSelf")),TEXT("true"));WDefault(WPin(C,TEXT("DrawDebugType")),TEXT("None"));return C;}
UK2Node_CallFunction* Hit(F& G,UK2Node_CallFunction* Trace){auto* H=G.Call(UGameplayStatics::StaticClass(),TEXT("BreakHitResult"));WLink(WPin(Trace,TEXT("OutHit")),WPin(H,TEXT("Hit")));return H;}
UEdGraphPin* OnOwner(F& G,UK2Node_CallFunction* Trace,UK2Node_CallFunction* H){return G.And(WPin(Trace,TEXT("ReturnValue")),G.And(G.Not(WPin(H,TEXT("bInitialOverlap"))),G.Math(TEXT("EqualEqual_ObjectObject"),WPin(H,TEXT("HitComponent")),G.Get(TEXT("TargetMesh")))));}
}

namespace BuildSimplify {
using J=TSharedPtr<FJsonObject>;
const FString RootDir=TEXT("C:/Users/Admin/Documents/ChatGPT/Reverse Fortnite/analysis/building-edit-simplify/");
const TCHAR* BasePath=TEXT("/Game/BuildingSystem/Blueprints/Classes/Bp_Master_Building.Bp_Master_Building");
const TCHAR* ManagerPath=TEXT("/Game/BuildingSystem/Blueprints/Classes/BpC_BuildingManager.BpC_BuildingManager");
FString Export(UObject* O,const TCHAR* Name){auto* P=FindFProperty<FProperty>(O->GetClass(),Name);if(!P)return TEXT("<missing>");FString V;P->ExportTextItem_Direct(V,P->ContainerPtrToValuePtr<void>(O),nullptr,O,PPF_None);return V;}
void WriteJSON(const TCHAR* File,J Data){FString Text;FJsonSerializer::Serialize(Data.ToSharedRef(),TJsonWriterFactory<>::Create(&Text));check(FFileHelper::SaveStringToFile(Text,*(RootDir+File)));}
TArray<FString> AssetPaths(){TArray<FString> R={BasePath,ManagerPath,TEXT("/Game/BuildingSystem/Chapter1Editing/BP_EditGrid.BP_EditGrid"),TEXT("/Game/BuildingSystem/Blueprints/Classes/BpC_BuildingStats.BpC_BuildingStats"),TEXT("/Game/Script/BlueprintActors/Building/BaseActors/FortStaticMeshActor.FortStaticMeshActor")};for(auto* Kind:{TEXT("Wall"),TEXT("Floor"),TEXT("Stairs"),TEXT("Roof")})R.Add(FString(TEXT("/Game/BuildingSystem/Blueprints/Classes/Building_"))+Kind+TEXT(".Building_")+Kind);return R;}
int32 Inspect(){
 J Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Assets,Cases;
 for(auto& P:AssetPaths()){
  auto* B=LoadObject<UBlueprint>(nullptr,*P);check(B);J A=MakeShared<FJsonObject>();A->SetStringField(TEXT("path"),P);A->SetStringField(TEXT("parent"),B->ParentClass->GetPathName());UObject* CDO=B->GeneratedClass->GetDefaultObject();J Props=MakeShared<FJsonObject>();
  for(auto* Name:{TEXT("BuildingData"),TEXT("ResourceVersions"),TEXT("Resource"),TEXT("BuildingKind"),TEXT("EditMask"),TEXT("EditPatterns"),TEXT("TileBits"),TEXT("PendingMask"),TEXT("SelectionMask"),TEXT("Built?"),TEXT("BuildingHP"),TEXT("MaxBuildingHP"),TEXT("ConstructionPartCount"),TEXT("replicates"),TEXT("bReplicates"),TEXT("bReplicateMovement")})Props->SetStringField(Name,Export(CDO,Name));A->SetObjectField(TEXT("defaults"),Props);
  TArray<TSharedPtr<FJsonValue>> Components;if(B->SimpleConstructionScript)for(auto* N:B->SimpleConstructionScript->GetAllNodes()){J C=MakeShared<FJsonObject>();C->SetStringField(TEXT("name"),N->GetVariableName().ToString());C->SetStringField(TEXT("class"),N->ComponentClass->GetPathName());for(auto* Name:{TEXT("StaticMesh"),TEXT("RelativeLocation"),TEXT("RelativeRotation"),TEXT("RelativeScale3D"),TEXT("Mobility"),TEXT("CollisionProfileName"),TEXT("bGenerateOverlapEvents")})C->SetStringField(Name,Export(N->ComponentTemplate,Name));Components.Add(MakeShared<FJsonValueObject>(C));}A->SetArrayField(TEXT("components"),Components);
  TArray<TSharedPtr<FJsonValue>> Functions;for(TFieldIterator<UFunction> It(B->GeneratedClass,EFieldIteratorFlags::ExcludeSuper);It;++It){J F=MakeShared<FJsonObject>();F->SetStringField(TEXT("name"),It->GetName());F->SetNumberField(TEXT("flags"),It->FunctionFlags);Functions.Add(MakeShared<FJsonValueObject>(F));}A->SetArrayField(TEXT("functions"),Functions);Assets.Add(MakeShared<FJsonValueObject>(A));
 }
 auto* MB=LoadObject<UBlueprint>(nullptr,ManagerPath);UObject* M=MB->GeneratedClass->GetDefaultObject();TGuardValue<bool> Guard(GAllowActorScriptExecutionInEditor,true);
 for(auto* Kind:{TEXT("Wall"),TEXT("Floor"),TEXT("Stairs"),TEXT("Roof")}){
  FString ClassPath=FString(TEXT("/Game/BuildingSystem/Blueprints/Classes/Building_"))+Kind+TEXT(".Building_")+Kind+TEXT("_C");UObject* O=LoadObject<UClass>(nullptr,*ClassPath)->GetDefaultObject();int32 Count=FCString::Stricmp(Kind,TEXT("Wall"))==0?512:16;bool Stairs=FCString::Stricmp(Kind,TEXT("Stairs"))==0;
  for(int32 Mask=0;Mask<Count;++Mask)for(int32 Corner=0;Corner<(Stairs?4:1);++Corner){int32 Key=Mask;
   if(Stairs){auto* F=M->FindFunctionChecked(TEXT("ResolveStairPattern"));TArray<uint8> D;D.SetNumZeroed(F->ParmsSize);FindFProperty<FIntProperty>(F,TEXT("Mask"))->SetPropertyValue_InContainer(D.GetData(),Mask);FindFProperty<FIntProperty>(F,TEXT("StartCorner"))->SetPropertyValue_InContainer(D.GetData(),Corner);M->ProcessEvent(F,D.GetData());Key=FindFProperty<FIntProperty>(F,TEXT("PatternKey"))->GetPropertyValue_InContainer(D.GetData());}
   auto* F=O->FindFunctionChecked(TEXT("FindEditPattern"));TArray<uint8> D;D.SetNumZeroed(F->ParmsSize);FindFProperty<FIntProperty>(F,TEXT("Mask"))->SetPropertyValue_InContainer(D.GetData(),Key);O->ProcessEvent(F,D.GetData());J C=MakeShared<FJsonObject>();C->SetStringField(TEXT("kind"),Kind);C->SetNumberField(TEXT("selection"),Mask);C->SetNumberField(TEXT("corner"),Corner);C->SetNumberField(TEXT("legacy_key"),Key);C->SetBoolField(TEXT("found"),FindFProperty<FBoolProperty>(F,TEXT("Found"))->GetPropertyValue_InContainer(D.GetData()));for(auto* Name:{TEXT("LookupMesh"),TEXT("LookupBoardCount"),TEXT("LookupRotation"),TEXT("LookupScale"),TEXT("LookupOffset"),TEXT("LookupSparkle"),TEXT("LookupHasDoor"),TEXT("LookupDoorOffset"),TEXT("LookupDoorRotation"),TEXT("LookupDoorScale")})C->SetStringField(Name,Export(O,Name));Cases.Add(MakeShared<FJsonValueObject>(C));
  }
 }
 Report->SetArrayField(TEXT("assets"),Assets);WriteJSON(TEXT("native-audit.json"),Report);J Baseline=MakeShared<FJsonObject>();Baseline->SetArrayField(TEXT("cases"),Cases);WriteJSON(TEXT("edit-baseline.json"),Baseline);UE_LOG(LogTemp,Display,TEXT("BUILD_EDIT_NATIVE_AUDIT_DONE %d cases"),Cases.Num());return 0;
}
}
UBuildingEditSimplifyCommandlet::UBuildingEditSimplifyCommandlet(){IsEditor=true;IsClient=false;IsServer=false;LogToConsole=true;}
namespace BuildSimplify {
UUserDefinedStruct* SelectionStruct;UBlueprint* EditLibrary;
const TCHAR* SelectionPath=TEXT("/Game/BuildingSystem/Chapter1Editing/S_BuildingEditSelection.S_BuildingEditSelection");
const TCHAR* LibraryPath=TEXT("/Game/BuildingSystem/Blueprints/Classes/BP_BuildingEditLibrary.BP_BuildingEditLibrary");
FEdGraphPinType BoolArray(){auto T=Bool();T.ContainerType=EPinContainerType::Array;return T;}
FEdGraphPinType SelectionType(){return Type(UEdGraphSchema_K2::PC_Struct,SelectionStruct);}
FProperty* SelectionField(const TCHAR* Name){auto* P=FStructureEditorUtils::GetPropertyByFriendlyName(SelectionStruct,Name);check(P);return P;}
UK2Node_FunctionResult* Result(F& G){for(UEdGraphNode* N:G.G->Nodes)if(auto* R=Cast<UK2Node_FunctionResult>(N))return R;return G.Ret();}
void PureStatic(F& G){G.E->AddExtraFlags(FUNC_Public|FUNC_BlueprintPure|FUNC_Static);G.E->MetaData.Category=FText::FromString(TEXT("Building|Edit Selection"));}
UEdGraphPin* Cells(F& G,UEdGraphPin* S){auto* N=WNode<UK2Node_BreakStruct>(G.G);N->StructType=SelectionStruct;WFinish(N);WLink(S,WPin(N,*SelectionStruct->GetName()));return WPin(N,*SelectionField(TEXT("Tiles"))->GetName());}
UEdGraphPin* Corner(F& G,UEdGraphPin* S){auto* N=WNode<UK2Node_BreakStruct>(G.G);N->StructType=SelectionStruct;WFinish(N);WLink(S,WPin(N,*SelectionStruct->GetName()));return WPin(N,*SelectionField(TEXT("StairStartCell"))->GetName());}
UEdGraphPin* Selection(F& G,UEdGraphPin* Tiles,UEdGraphPin* Start=nullptr){auto* N=WNode<UK2Node_MakeStruct>(G.G);N->StructType=SelectionStruct;WFinish(N);WLink(Tiles,WPin(N,*SelectionField(TEXT("Tiles"))->GetName()));if(Start)WLink(Start,WPin(N,*SelectionField(TEXT("StairStartCell"))->GetName()));else WDefault(WPin(N,*SelectionField(TEXT("StairStartCell"))->GetName()),TEXT("0"));return WPin(N,*SelectionStruct->GetName());}
UK2Node_CallFunction* Lib(F& G,const TCHAR* Name){check(EditLibrary&&EditLibrary->GeneratedClass);return G.Call(EditLibrary->GeneratedClass,Name);}
UEdGraphPin* ArrayGet(F& G,UEdGraphPin* A,UEdGraphPin* Index=nullptr,const TCHAR* DefaultIndex=TEXT("0")){auto* N=WNode<UK2Node_GetArrayItem>(G.G);WFinish(N);WLink(A,WPin(N,TEXT("Array")));if(Index)WLink(Index,WPin(N,TEXT("Dimension 1")));else WDefault(WPin(N,TEXT("Dimension 1")),DefaultIndex);return WPin(N,TEXT("Output"));}
UEdGraphPin* ReadTile(F& G,UEdGraphPin* S,UEdGraphPin* I=nullptr,const TCHAR* Default=TEXT("0")){auto* C=Lib(G,TEXT("IsTileSelected"));WLink(S,WPin(C,TEXT("Selection")));if(I)WLink(I,WPin(C,TEXT("Index")));else WDefault(WPin(C,TEXT("Index")),Default);return WPin(C,TEXT("Selected"));}
int32 BuildLibrary(){
 SelectionStruct=LoadObject<UUserDefinedStruct>(nullptr,SelectionPath);if(!SelectionStruct){auto* P=CreatePackage(TEXT("/Game/BuildingSystem/Chapter1Editing/S_BuildingEditSelection"));SelectionStruct=FStructureEditorUtils::CreateUserDefinedStruct(P,TEXT("S_BuildingEditSelection"),RF_Public|RF_Standalone);check(SelectionStruct);auto& Fields=FStructureEditorUtils::GetVarDesc(SelectionStruct);check(Fields.Num()==1);FGuid Tiles=Fields[0].VarGuid;check(FStructureEditorUtils::ChangeVariableType(SelectionStruct,Tiles,BoolArray()));check(FStructureEditorUtils::RenameVariable(SelectionStruct,Tiles,TEXT("Tiles")));check(FStructureEditorUtils::AddVariable(SelectionStruct,Int()));FGuid Start=FStructureEditorUtils::GetVarDesc(SelectionStruct).Last().VarGuid;check(FStructureEditorUtils::RenameVariable(SelectionStruct,Start,TEXT("StairStartCell")));FStructureEditorUtils::ChangeTooltip(SelectionStruct,TEXT("True selects an edit tile. StairStartCell identifies the first selected stair corner; revisions and cell indices are separate numeric identifiers."));FAssetRegistryModule::AssetCreated(SelectionStruct);WSave(SelectionStruct);}
 EditLibrary=LoadObject<UBlueprint>(nullptr,LibraryPath);if(EditLibrary)return 0;
 auto* Package=CreatePackage(TEXT("/Game/BuildingSystem/Blueprints/Classes/BP_BuildingEditLibrary"));BP=FKismetEditorUtilities::CreateBlueprint(UBlueprintFunctionLibrary::StaticClass(),Package,TEXT("BP_BuildingEditLibrary"),BPTYPE_FunctionLibrary,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass());check(BP);EditLibrary=BP;ActorClass=BP->GeneratedClass;
 {
  F G(TEXT("GetTileCount"));PureStatic(G);auto* Kind=G.In(TEXT("BuildingKind"),Int());G.Out(TEXT("Count"),Int());auto* Pick=G.Call(UKismetMathLibrary::StaticClass(),TEXT("SelectInt"));WDefault(WPin(Pick,TEXT("A")),TEXT("9"));WDefault(WPin(Pick,TEXT("B")),TEXT("4"));WLink(G.Math(TEXT("EqualEqual_IntInt"),Kind,nullptr,TEXT("0")),WPin(Pick,TEXT("bPickA")));auto* R=Result(G);WLink(WPin(Pick,TEXT("ReturnValue")),WPin(R,TEXT("Count")));WLink(Then(G.E),Exec(R));WCompile();Layout(G.G);
 }
 {
  F G(TEXT("MakeEmptySelection"));PureStatic(G);auto* Kind=G.In(TEXT("BuildingKind"),Int());G.Out(TEXT("Selection"),SelectionType());G.Local(TEXT("Tiles"),BoolArray());WCompile();auto* Count=Lib(G,TEXT("GetTileCount"));WLink(Kind,WPin(Count,TEXT("BuildingKind")));auto* Resize=G.Arr(TEXT("Array_Resize"),G.Get(TEXT("Tiles")));WLink(WPin(Count,TEXT("Count")),WPin(Resize,TEXT("Size")));auto* R=Result(G);WLink(Then(G.E),Exec(Resize));WLink(Then(Resize),Exec(R));WLink(Selection(G,G.Get(TEXT("Tiles"))),WPin(R,TEXT("Selection")));WCompile();Layout(G.G);
 }
 {
  F G(TEXT("IsTileSelected"));PureStatic(G);auto* S=G.In(TEXT("Selection"),SelectionType());auto* Index=G.In(TEXT("Index"),Int());G.Out(TEXT("Selected"),Bool());auto* Valid=G.Arr(TEXT("Array_IsValidIndex"),Cells(G,S));WLink(Index,WPin(Valid,TEXT("IndexToTest")));auto* Gate=G.If(WPin(Valid,TEXT("ReturnValue")));auto* R=Result(G);auto* Invalid=G.Ret();if(!Invalid->FindPin(TEXT("Selected")))Invalid->CreateUserDefinedPin(TEXT("Selected"),Bool(),EGPD_Input);WDefault(WPin(Invalid,TEXT("Selected")),TEXT("false"));WLink(Then(G.E),Exec(Gate));WLink(Then(Gate),Exec(R));WLink(WPin(Gate,TEXT("else")),Exec(Invalid));WLink(ArrayGet(G,Cells(G,S),Index),WPin(R,TEXT("Selected")));WCompile();Layout(G.G);
 }
 {
  F G(TEXT("IsSelectionEmpty"));PureStatic(G);auto* S=G.In(TEXT("Selection"),SelectionType());G.Out(TEXT("Empty"),Bool());auto* Contains=G.Arr(TEXT("Array_Contains"),Cells(G,S));WDefault(WPin(Contains,TEXT("ItemToFind")),TEXT("true"));auto* R=Result(G);WLink(G.And(G.Not(WPin(Contains,TEXT("ReturnValue"))),G.Math(TEXT("EqualEqual_IntInt"),Corner(G,S),nullptr,TEXT("0"))),WPin(R,TEXT("Empty")));WLink(Then(G.E),Exec(R));WCompile();Layout(G.G);
 }
 {
  F G(TEXT("SelectionsEqual"));PureStatic(G);auto* A=G.In(TEXT("A"),SelectionType());auto* B=G.In(TEXT("B"),SelectionType());G.Out(TEXT("Equal"),Bool());auto* Same=WNode<UK2Node_CallArrayFunction>(G.G);Same->SetFromFunction(UKismetArrayLibrary::StaticClass()->FindFunctionByName(TEXT("Array_Identical")));WFinish(Same);WLink(Cells(G,A),WPin(Same,TEXT("ArrayA")));WLink(Cells(G,B),WPin(Same,TEXT("ArrayB")));auto* R=Result(G);WLink(G.And(WPin(Same,TEXT("ReturnValue")),G.Math(TEXT("EqualEqual_IntInt"),Corner(G,A),Corner(G,B))),WPin(R,TEXT("Equal")));WLink(Then(G.E),Exec(R));WCompile();Layout(G.G);
 }
 {
  F G(TEXT("WithStairStartCell"));PureStatic(G);auto* S=G.In(TEXT("Selection"),SelectionType());auto* Index=G.In(TEXT("StartCell"),Int());G.Out(TEXT("Result"),SelectionType());auto* R=Result(G);WLink(Selection(G,Cells(G,S),Index),WPin(R,TEXT("Result")));WLink(Then(G.E),Exec(R));WCompile();Layout(G.G);
 }
 {
  F G(TEXT("SetTileSelected"));PureStatic(G);auto* S=G.In(TEXT("Selection"),SelectionType());auto* Index=G.In(TEXT("Index"),Int());auto* State=G.In(TEXT("Selected"),Bool());G.Out(TEXT("Result"),SelectionType());G.Local(TEXT("Tiles"),BoolArray());WCompile();auto* Copy=G.Set(TEXT("Tiles"),Cells(G,S));auto* Valid=G.If(G.And(G.Math(TEXT("GreaterEqual_IntInt"),Index,nullptr,TEXT("0")),G.Math(TEXT("Less_IntInt"),Index,nullptr,TEXT("9"))));auto* Set=G.Arr(TEXT("Array_Set"),G.Get(TEXT("Tiles")));WLink(Index,WPin(Set,TEXT("Index")));WLink(State,WPin(Set,TEXT("Item")));WDefault(WPin(Set,TEXT("bSizeToFit")),TEXT("true"));auto* R=Result(G);WLink(Then(G.E),Exec(Copy));WLink(Then(Copy),Exec(Valid));WLink(Then(Valid),Exec(Set));WLink(Then(Set),Exec(R));WLink(WPin(Valid,TEXT("else")),Exec(R));WLink(Selection(G,G.Get(TEXT("Tiles")),Corner(G,S)),WPin(R,TEXT("Result")));WCompile();Layout(G.G);
 }
 {
  F G(TEXT("HasValidTileCount"));PureStatic(G);auto* S=G.In(TEXT("Selection"),SelectionType());auto* Kind=G.In(TEXT("BuildingKind"),Int());G.Out(TEXT("Valid"),Bool());auto* Size=G.Arr(TEXT("Array_Length"),Cells(G,S));auto* Count=Lib(G,TEXT("GetTileCount"));WLink(Kind,WPin(Count,TEXT("BuildingKind")));auto* R=Result(G);WLink(G.And(G.Math(TEXT("EqualEqual_IntInt"),WPin(Size,TEXT("ReturnValue")),WPin(Count,TEXT("Count"))),G.And(G.Math(TEXT("GreaterEqual_IntInt"),Corner(G,S),nullptr,TEXT("0")),G.Math(TEXT("Less_IntInt"),Corner(G,S),nullptr,TEXT("4")))),WPin(R,TEXT("Valid")));WLink(Then(G.E),Exec(R));WCompile();Layout(G.G);
 }
 FAssetRegistryModule::AssetCreated(BP);WSave(BP);UE_LOG(LogTemp,Display,TEXT("BOOLEAN_EDIT_LIBRARY_SAVED"));return 0;
}
}
#include "BuildingEditMigration.inl"
#include "BuildingFortParent.inl"
#include "BuildingEditTests.inl"
#include "BuildingEditPolish.inl"
#include "BuildingEditLabels.inl"

#include "GhostCollapseFix.inl"
int32 UBuildingEditSimplifyCommandlet::Main(const FString& Params){if(FParse::Param(*Params,TEXT("GhostCollapse")))return BuildSimplify::GhostCollapseFix();if(FParse::Param(*Params,TEXT("Participation")))return BuildSimplify::FixSupportParticipation();if(FParse::Param(*Params,TEXT("Finalize")))return BuildSimplify::FinalizeSelection();if(FParse::Param(*Params,TEXT("Support")))return BuildSimplify::RefreshEditSupport();if(FParse::Param(*Params,TEXT("Defaults")))return BuildSimplify::RestoreFortDefaults();if(FParse::Param(*Params,TEXT("Labels")))return BuildSimplify::Labels();if(FParse::Param(*Params,TEXT("Polish")))return BuildSimplify::Polish();if(FParse::Param(*Params,TEXT("Parent")))return BuildSimplify::FortParent();if(FParse::Param(*Params,TEXT("Verify"))){BuildSimplify::BuildLibrary();BuildSimplify::PatternStruct=LoadObject<UUserDefinedStruct>(nullptr,TEXT("/Game/BuildingSystem/Chapter1Editing/S_EditPattern.S_EditPattern"));BuildSimplify::PopulateAndCheck(true);return 0;}if(FParse::Param(*Params,TEXT("Migrate")))return BuildSimplify::Migrate();if(FParse::Param(*Params,TEXT("Library")))return BuildSimplify::BuildLibrary();return BuildSimplify::Inspect();}







