namespace BuildSimplify {
int32 GhostCollapseFix(){
 auto* Fort=LoadObject<UBlueprint>(nullptr,Path);check(Fort);
 BP=Fort;ActorClass=Fort->GeneratedClass;
 // A virtual predicate keeps focus UI aware of child preview state without coupling props to the build system.
 {F G(TEXT("CanShowBuildingHealthBar"));G.E->AddExtraFlags(FUNC_BlueprintPure);G.E->MetaData.Category=FText::FromString(TEXT("Building|UI"));G.Out(TEXT("CanShow"),Bool());WLink(G.Not(G.Get(TEXT("bDestroyed"))),WPin(Result(G),TEXT("CanShow")));WLink(Then(G.E),Exec(Result(G)));Layout(G.G);}
 WCompile();
 {F G(Graph(Fort,TEXT("EventGraph"))); // Locate the existing focus branch by its event output.
  UK2Node_Event* Focus=nullptr;for(UEdGraphNode* N:G.G->Nodes)if(auto* E=Cast<UK2Node_Event>(N))if(E->EventReference.GetMemberName()==TEXT("SetFocused"))Focus=E;check(Focus);
  auto* Existing=Then(Focus)->LinkedTo[0];Then(Focus)->BreakAllPinLinks();auto* Allowed=G.Call(TEXT("CanShowBuildingHealthBar"));auto* Gate=G.If(WPin(Allowed,TEXT("CanShow")));WLink(Then(Focus),Exec(Gate));WLink(Then(Gate),Existing);
  auto* Hide=G.Call(USceneComponent::StaticClass(),TEXT("SetVisibility"));WLink(G.Get(TEXT("BuildingHealthBar")),WPin(Hide,TEXT("self")));WDefault(WPin(Hide,TEXT("bNewVisibility")),TEXT("false"));WLink(WPin(Gate,TEXT("else")),Exec(Hide));Gate->NodePosX=Focus->NodePosX+300;Gate->NodePosY=Focus->NodePosY;Allowed->NodePosX=Gate->NodePosX;Allowed->NodePosY=Gate->NodePosY+200;Hide->NodePosX=Gate->NodePosX+400;Hide->NodePosY=Gate->NodePosY+300;
 }
 {auto* G=Graph(Fort,TEXT("CheckStructuralIntegrity"));for(UEdGraphNode* N:G->Nodes)if(auto* C=Cast<UK2Node_CallFunction>(N))if(C->FunctionReference.GetMemberName()==TEXT("FMax"))WDefault(WPin(C,TEXT("B")),TEXT("0.01"));}
 for(auto& V:Fort->NewVariables)if(V.VarName==TEXT("CollapseDelay"))V.SetMetaData(TEXT("Tooltip"),TEXT("Wait before the first unsupported member breaks. Minimum 0.01 seconds."));else if(V.VarName==TEXT("CollapsePropagationDelay"))V.SetMetaData(TEXT("Tooltip"),TEXT("Additional wait between successive unsupported members, in breadth-first order. Minimum 0.01 seconds."));
 WCompile();
 auto* Base=LoadObject<UBlueprint>(nullptr,BasePath);check(Base);BP=Base;ActorClass=Base->GeneratedClass;
 {F G(NewOverride(Base,TEXT("CanShowBuildingHealthBar")));WLink(G.And(G.Get(TEXT("Built?")),G.And(G.Not(G.Get(TEXT("bDestroyed"))),G.Not(G.Get(TEXT("LocalEditing"))))),WPin(Result(G),TEXT("CanShow")));WLink(Then(G.E),Exec(Result(G)));Layout(G.G);}
 // Both legacy construction UI and inherited harvesting UI must exclude preview actors.
 {F G(Graph(Base,TEXT("EventGraph")));auto Nodes=G.G->Nodes;for(UEdGraphNode* N:Nodes){
  if(auto* C=Cast<UK2Node_CallFunction>(N))if(C->FunctionReference.GetMemberName()==TEXT("SetVisibility")){auto* Target=Source(WPin(C,TEXT("self")));auto* V=Target?Cast<UK2Node_VariableGet>(Target->GetOwningNode()):nullptr;auto* Show=WPin(C,TEXT("bNewVisibility"));if(V&&V->VariableReference.GetMemberName()==TEXT("HealthBar")&&!Show->LinkedTo.Num()&&Show->DefaultValue==TEXT("true")){auto* Allowed=G.Call(TEXT("CanShowBuildingHealthBar"));WLink(WPin(Allowed,TEXT("CanShow")),Show);Allowed->NodePosX=C->NodePosX-300;Allowed->NodePosY=C->NodePosY+250;}}
  if(auto* E=Cast<UK2Node_Event>(N))if(E->EventReference.GetMemberName()==TEXT("ReceiveBeginPlay")){auto* Hide=G.Call(USceneComponent::StaticClass(),TEXT("SetVisibility"));WLink(G.Get(TEXT("BuildingHealthBar")),WPin(Hide,TEXT("self")));WDefault(WPin(Hide,TEXT("bNewVisibility")),TEXT("false"));InsertAfter(G,E,Hide);Hide->NodePosX=E->NodePosX+300;Hide->NodePosY=E->NodePosY;}
 }}
 TArray<UBlueprint*> Family={Fort,Base};for(auto* Kind:{TEXT("Wall"),TEXT("Floor"),TEXT("Stairs"),TEXT("Roof")})Family.Add(LoadObject<UBlueprint>(nullptr,*(FString(TEXT("/Game/BuildingSystem/Blueprints/Classes/Building_"))+Kind+TEXT(".Building_")+Kind)));
 for(auto* B:Family){BP=B;ActorClass=B->GeneratedClass;if(B!=Fort)WCompile();auto* O=B->GeneratedClass->GetDefaultObject();FindFProperty<FDoubleProperty>(O->GetClass(),TEXT("CollapseDelay"))->SetPropertyValue_InContainer(O,0.20);FindFProperty<FDoubleProperty>(O->GetClass(),TEXT("CollapsePropagationDelay"))->SetPropertyValue_InContainer(O,0.25);WSave(B);}
 UE_LOG(LogTemp,Display,TEXT("GHOST_HEALTH_COLLAPSE_FIX_SAVED"));return 0;
}
}
