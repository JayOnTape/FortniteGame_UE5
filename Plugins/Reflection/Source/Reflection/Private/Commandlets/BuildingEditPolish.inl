namespace BuildSimplify {
// UE 5.3 has no FBlueprintEditorUtils::RenameFunctionReferences. Update the
// call and delegate nodes directly, retaining their scope and function GUID.
void RenameFunctionReferences(UBlueprint* Blueprint, UClass* FunctionClass, FName OldName, FName NewName) {
 for (UEdGraph* Graph : Graphs(Blueprint)) {
  for (UEdGraphNode* Node : Graph->Nodes) {
   if (auto* Call = Cast<UK2Node_CallFunction>(Node)) {
    FMemberReference& Reference = Call->FunctionReference;
    UClass* Scope = Reference.GetMemberParentClass(Blueprint->GeneratedClass);
    if (Reference.GetMemberName() == OldName && Scope && Scope->IsChildOf(FunctionClass)) {
     Call->Modify();
     if (Reference.IsSelfContext()) Reference.SetSelfMember(NewName, Reference.GetMemberGuid());
     else Reference.SetExternalMember(NewName, Reference.GetMemberParentClass(), Reference.GetMemberGuid());
    }
   } else if (auto* Delegate = Cast<UK2Node_CreateDelegate>(Node)) {
    UClass* Scope = Delegate->GetScopeClass();
    if (Delegate->GetFunctionName() == OldName && Scope && Scope->IsChildOf(FunctionClass)) {
     Delegate->Modify();
     Delegate->SetFunction(NewName);
    }
   }
  }
 }
}
int32 Polish(){BuildLibrary();PatternStruct=LoadObject<UUserDefinedStruct>(nullptr,TEXT("/Game/BuildingSystem/Chapter1Editing/S_EditPattern.S_EditPattern"));TArray<UBlueprint*> All;for(auto P:AssetPaths())All.AddUnique(LoadObject<UBlueprint>(nullptr,*P));for(auto* P:{TEXT("/Game/BuildingSystem/Chapter1Editing/BP_EditDoor.BP_EditDoor"),TEXT("/Game/Athena/PlayerPawn_Athena.PlayerPawn_Athena"),TEXT("/Game/BuildingSystem/Widgets/W_Building.W_Building")}){auto* B=LoadObject<UBlueprint>(nullptr,P);if(B)All.AddUnique(B);}auto* Base=LoadObject<UBlueprint>(nullptr,BasePath);auto* Manager=LoadObject<UBlueprint>(nullptr,ManagerPath);auto* Grid=LoadObject<UBlueprint>(nullptr,GridPath);
 for(auto* G:Graphs(Base))for(UEdGraphNode* N:G->Nodes)check(!N->IsA<UK2Node_Timeline>());Base->Timelines.Empty();CastChecked<UBlueprintGeneratedClass>(Base->GeneratedClass)->Timelines.Empty();for(USCS_Node* N:Base->SimpleConstructionScript->GetRootNodes())if(N->ComponentClass->IsChildOf(USceneComponent::StaticClass()))N->SetParent(CastChecked<AStaticMeshActor>(Base->ParentClass->GetDefaultObject())->GetStaticMeshComponent());
 struct Rename {UBlueprint* Owner;const TCHAR* Old;const TCHAR* New;};Rename Vars[]={{Base,TEXT("EditMask"),TEXT("EditSelection")},{Base,TEXT("ConfiguredDoorMask"),TEXT("ConfiguredDoorSelection")},{Manager,TEXT("PendingMask"),TEXT("PendingSelection")},{Manager,TEXT("VisitedCells"),TEXT("VisitedTiles")},{Manager,TEXT("StairStartCorner"),TEXT("StairStartCell")},{Grid,TEXT("SelectionMask"),TEXT("Selection")}};
 TMap<UBlueprint*,FString> EmptyDefaults;for(auto* B:All)if(B==Base||B->ParentClass==Base->GeneratedClass)EmptyDefaults.Add(B,Export(B->GeneratedClass->GetDefaultObject(),TEXT("EditMask")));
 for(auto R:Vars){for(auto& V:R.Owner->NewVariables)if(V.VarName==R.Old)V.RepNotifyFunc=NAME_None;FBlueprintEditorUtils::RenameMemberVariable(R.Owner,R.Old,R.New);for(auto* B:All)for(auto* Graph:Graphs(B))for(UEdGraphNode* N:Graph->Nodes)if(auto* V=Cast<UK2Node_Variable>(N))if(V->VariableReference.GetMemberName()==R.Old){bool Self=B==R.Owner;if(Self)V->VariableReference.SetSelfMember(R.New);else V->VariableReference.SetExternalMember(R.New,R.Owner->GeneratedClass);for(auto* P:N->Pins)if(P->PinName==R.Old)P->PinName=R.New;}}
 Rename Functions[]={{Base,TEXT("IsValidEditMask"),TEXT("IsValidEditSelection")},{Base,TEXT("PreviewEditMask"),TEXT("PreviewEditSelection")},{Base,TEXT("OnRep_EditMask"),TEXT("OnRep_EditSelection")},{Manager,TEXT("GetEditPatternMask"),TEXT("GetEditSelection")},{Manager,TEXT("ResolveStairPattern"),TEXT("MakeStairSelection")},{Manager,TEXT("DecodeStairStart"),TEXT("GetStairStartCell")}};
 for(auto R:Functions){auto* G=Graph(R.Owner,R.Old);FBlueprintEditorUtils::RenameGraph(G,R.New);for(auto* B:All)RenameFunctionReferences(B,R.Owner->GeneratedClass,R.Old,R.New);}
 for(auto& V:Base->NewVariables)if(V.VarName==TEXT("EditSelection"))V.RepNotifyFunc=TEXT("OnRep_EditSelection");
 for(auto* B:All)for(auto* Graph:Graphs(B)){bool Stair=Graph->GetFName()==TEXT("MakeStairSelection");for(UEdGraphNode* N:Graph->Nodes){auto* Call=Cast<UK2Node_CallFunction>(N);bool StairCall=Call&&Call->FunctionReference.GetMemberName()==TEXT("MakeStairSelection");for(auto* P:N->Pins)if(IsSelection(P)&&(P->PinName==TEXT("Mask")||P->PinName==TEXT("PatternKey")))P->PinName=(P->PinName==TEXT("PatternKey")&&(Stair||StairCall))?TEXT("Result"):TEXT("Selection");if(auto* U=Cast<UK2Node_EditablePinBase>(N))for(auto& D:U->UserDefinedPins)if(D->PinType.PinSubCategoryObject==SelectionStruct&&(D->PinName==TEXT("Mask")||D->PinName==TEXT("PatternKey")))D->PinName=(D->PinName==TEXT("PatternKey")&&Stair)?TEXT("Result"):TEXT("Selection");if(auto* E=Cast<UK2Node_FunctionEntry>(N)){E->MetaData.Category=FText::FromString(Graph->GetName().Contains(TEXT("Edit"))||Graph->GetName().Contains(TEXT("Selection"))||Graph->GetName().Contains(TEXT("Tile"))?TEXT("Building|Editing"):TEXT("Building"));}}PrunePure(Graph);}
 for(auto* B:{Grid,Manager}){bool Referenced=false;for(auto* G:Graphs(B))for(UEdGraphNode* N:G->Nodes)if(auto* V=Cast<UK2Node_Variable>(N))Referenced|=V->VariableReference.GetMemberName()==TEXT("TileBits");check(!Referenced);FBlueprintEditorUtils::RemoveMemberVariable(B,TEXT("TileBits"));}
 for(auto* B:All)Skeleton(B);for(auto* B:All){BP=B;ActorClass=B->GeneratedClass;FBlueprintEditorUtils::RefreshAllNodes(B);UE_LOG(LogTemp,Warning,TEXT("POLISH_COMPILE %s"),*B->GetName());FCompilerResultsLog Log;FKismetEditorUtilities::CompileBlueprint(B,EBlueprintCompileOptions::SkipSave|EBlueprintCompileOptions::SkipGarbageCollection|EBlueprintCompileOptions::SkipReinstancing,&Log);check(!Log.NumErrors);}
 for(auto& Pair:EmptyDefaults){auto* O=Pair.Key->GeneratedClass->GetDefaultObject();auto* P=FindFProperty<FProperty>(O->GetClass(),TEXT("EditSelection"));check(P);check(P->ImportText_Direct(*Pair.Value,P->ContainerPtrToValuePtr<void>(O),O,PPF_None));}
 PopulateAndCheck(true);for(auto* B:All)WSave(B);UE_LOG(LogTemp,Warning,TEXT("BUILDING_EDIT_POLISH_SAVED"));return 0;
}
}

