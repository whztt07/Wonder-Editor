open AssetTreeNodeType;

open AssetNodeType;

let getWidget = () => EditorType.Asset;

let isAssetWDBFile = () => {
  let (widget, startNodeId) =
    StateEditorService.getState()
    |> CurrentDragSourceEditorService.getCurrentDragSource;

  switch (widget, startNodeId) {
  | (Some(widget), Some(nodeId)) =>
    widget === getWidget()
    && StateEditorService.getState()
    |> AssetWDBNodeMapEditorService.getWDBNodeMap
    |> WonderCommonlib.SparseMapService.get(nodeId)
    |> Js.Option.isSome
  | _ => false
  };
};

let isWidget = startWidget =>
  switch (startWidget) {
  | None => false
  | Some(startWidget) => startWidget === getWidget()
  };

let getTargetTreeNodeId = editorState =>
  switch (
    AssetCurrentNodeParentIdEditorService.getCurrentNodeParentId(editorState)
  ) {
  | None => editorState |> AssetTreeRootEditorService.getRootTreeNodeId
  | Some(nodeId) => nodeId
  };

let isIdEqual = (nodeId, targetNodeId) => nodeId === targetNodeId;

let rec getSpecificTreeNodeById = (nodeId, targetTreeNode) =>
  isIdEqual(nodeId, targetTreeNode.nodeId) ?
    Some(targetTreeNode) :
    {
      let (resultNode, _) =
        targetTreeNode.children
        |> WonderCommonlib.ArrayService.reduceOneParam(
             (. (resultNode, nodeId), child) =>
               switch (resultNode) {
               | Some(_) => (resultNode, nodeId)
               | None => (getSpecificTreeNodeById(nodeId, child), nodeId)
               },
             (None, nodeId),
           );
      resultNode;
    };

let _removeClonedGameObjectIfHasIt = (gameObjectUid, editorState, engineState) =>
  switch (
    editorState
    |> AssetClonedGameObjectMapEditorService.getClonedGameObjectMap
    |> WonderCommonlib.SparseMapService.get(gameObjectUid)
  ) {
  | None => (editorState, engineState)
  | Some(clonedGameObjectArr) => (
      editorState
      |> AssetClonedGameObjectMapEditorService.getClonedGameObjectMap
      |> SparseMapService.copy
      |> DomHelper.deleteKeyInMap(gameObjectUid)
      |. AssetClonedGameObjectMapEditorService.setClonedGameObjectMap(
           editorState,
         ),
      clonedGameObjectArr
      |. GameObjectEngineService.disposeGameObjectArrKeepOrder(engineState),
    )
  };

let _handleRemoveWDBNode = (nodeId, editorState) => {
  let {wdbGameObject} =
    editorState
    |> AssetWDBNodeMapEditorService.getWDBNodeMap
    |> WonderCommonlib.SparseMapService.unsafeGet(nodeId);
  let defaultCubeGeometryComponent =
    editorState
    |> AssetGeometryDataEditorService.getGeometryData
    |> (({defaultCubeGeometryComponent}) => defaultCubeGeometryComponent);

  let (editorState, engineState) =
    StateEngineService.unsafeGetState()
    |> GeometryEngineService.replaceAllGameObjectGeometryToDefaultGeometry(
         wdbGameObject,
         defaultCubeGeometryComponent,
       )
    |> _removeClonedGameObjectIfHasIt(wdbGameObject, editorState);

  StateLogicService.refreshEngineState(engineState);

  editorState
  |> AssetWDBNodeMapEditorService.getWDBNodeMap
  |> SparseMapService.copy
  |> DomHelper.deleteKeyInMap(nodeId)
  |. AssetWDBNodeMapEditorService.setWDBNodeMap(editorState);
};

let _removeTextureFromSceneBasicMaterials =
    (textureComponent, editorState, engineState) =>
  /* BasicMaterialEngineService.getAllBasicMaterials(engineState) */
  SceneEngineService.getSceneAllBasicMaterials(engineState)
  |> Js.Array.filter(basicMaterial =>
       BasicMaterialEngineService.isBasicMaterialMap(
         basicMaterial,
         textureComponent,
         engineState,
       )
     )
  |> WonderCommonlib.ArrayService.reduceOneParam(
       (. engineState, basicMaterial) =>
         OperateTextureLogicService.handleBasicMaterialComponentFromHasMapToNoMap(
           basicMaterial,
           engineState,
         ),
       engineState,
     );

let _removeTextureFromSceneLightMaterials =
    (textureComponent, editorState, engineState) =>
  SceneEngineService.getSceneAllLightMaterials(engineState)
  |> Js.Array.filter(lightMaterial =>
       LightMaterialEngineService.isLightMaterialMap(
         lightMaterial,
         textureComponent,
         engineState,
       )
     )
  |> WonderCommonlib.ArrayService.reduceOneParam(
       (. engineState, lightMaterial) =>
         OperateTextureLogicService.handleLightMaterialComponentFromHasDiffuseMapToNoMap(
           lightMaterial,
           engineState,
         ),
       engineState,
     );

/* TODO remove texture from material assets */
let _removeTextureEngineData = (textureComponent, editorState, engineState) =>
  engineState
  |> _removeTextureFromSceneBasicMaterials(textureComponent, editorState)
  |> _removeTextureFromSceneLightMaterials(textureComponent, editorState);

let _removeTextureEditorData =
    (nodeId, textureComponent, imageId, editorState) => {
  let {textureArray} as imageResult =
    editorState
    |> AssetImageBase64MapEditorService.getImageBase64Map
    |> WonderCommonlib.SparseMapService.unsafeGet(imageId);
  let newTextureArr =
    textureArray |> Js.Array.filter(texture => texture !== textureComponent);

  let editorState =
    switch (newTextureArr |> Js.Array.length) {
    | 0 =>
      editorState
      |> AssetImageBase64MapEditorService.getImageBase64Map
      |> Js.Array.copy
      |> DomHelper.deleteKeyInMap(imageId)
      |. AssetImageBase64MapEditorService.setImageBase64Map(editorState)
    | _ =>
      editorState
      |> AssetImageBase64MapEditorService.setResult(
           imageId,
           {...imageResult, textureArray: newTextureArr},
         )
    };

  editorState
  |> AssetTextureNodeMapEditorService.getTextureNodeMap
  |> SparseMapService.copy
  |> DomHelper.deleteKeyInMap(nodeId)
  |. AssetTextureNodeMapEditorService.setTextureNodeMap(editorState);
};

let _removeTextureTreeNode = (nodeId, editorState) => {
  let {textureComponent, imageId} =
    editorState
    |> AssetTextureNodeMapEditorService.getTextureNodeMap
    |> WonderCommonlib.SparseMapService.unsafeGet(nodeId);

  _removeTextureEngineData(textureComponent, editorState)
  |> StateLogicService.getAndSetEngineState;
  _removeTextureEditorData(nodeId, textureComponent, imageId, editorState);
};

let deepRemoveTreeNode = (removedTreeNode, editorState) => {
  let rec _iterateRemovedTreeNode = (nodeArr, removedAssetIdArr, editorState) =>
    nodeArr
    |> WonderCommonlib.ArrayService.reduceOneParam(
         (. (editorState, removedAssetIdArr), {nodeId, type_, children}) => {
           let editorState =
             switch (type_) {
             | Folder =>
               editorState
               |> AssetFolderNodeMapEditorService.getFolderNodeMap
               |> SparseMapService.copy
               |> DomHelper.deleteKeyInMap(nodeId)
               |. AssetFolderNodeMapEditorService.setFolderNodeMap(
                    editorState,
                  )
             | Texture => _removeTextureTreeNode(nodeId, editorState)
             | Json =>
               editorState
               |> AssetJsonNodeMapEditorService.getJsonNodeMap
               |> SparseMapService.copy
               |> DomHelper.deleteKeyInMap(nodeId)
               |. AssetJsonNodeMapEditorService.setJsonNodeMap(editorState)
             | WDB => _handleRemoveWDBNode(nodeId, editorState)
             | _ => editorState
             };

           _iterateRemovedTreeNode(
             children,
             removedAssetIdArr |> ArrayService.push(nodeId),
             editorState,
           );
         },
         (editorState, removedAssetIdArr),
       );

  _iterateRemovedTreeNode([|removedTreeNode|], [||], editorState);
};

let _checkRemovedTreeNodeAndGetVal = ((newAssetTreeArr, removedTreeNode)) => {
  WonderLog.Contract.requireCheck(
    () =>
      WonderLog.(
        Contract.(
          test(
            Log.buildAssertMessage(
              ~expect={j|removedTreeNode should exist|j},
              ~actual={j|not|j},
            ),
            () =>
            removedTreeNode |> Js.Option.isSome |> assertTrue
          )
        )
      ),
    StateEditorService.getStateIsDebug(),
  );

  (
    newAssetTreeArr |> ArrayService.unsafeGetFirst,
    removedTreeNode |> OptionService.unsafeGet,
  );
};

let removeSpecificTreeNode = (targetNodeId, assetTreeRoot) => {
  let rec _iterateAssetTree =
          (targetNodeId, assetTreeArr, newAssetTree, removedTreeNode) =>
    assetTreeArr
    |> WonderCommonlib.ArrayService.reduceOneParam(
         (. (newAssetTree, removedTreeNode), {nodeId, children} as treeNode) =>
           isIdEqual(nodeId, targetNodeId) ?
             (newAssetTree, Some(treeNode)) :
             {
               let (newAssetTreeChildrenArray, removedTreeNode) =
                 _iterateAssetTree(
                   targetNodeId,
                   children,
                   [||],
                   removedTreeNode,
                 );
               (
                 newAssetTree
                 |> ArrayService.push({
                      ...treeNode,
                      children: newAssetTreeChildrenArray,
                    }),
                 removedTreeNode,
               );
             },
         (newAssetTree, removedTreeNode),
       );

  _iterateAssetTree(targetNodeId, [|assetTreeRoot|], [||], None)
  |> _checkRemovedTreeNodeAndGetVal;
};

let insertSourceTreeNodeToTargetTreeNodeChildren =
    (targetNodeId, newTreeNode, assetTreeRoot) => {
  let rec _iterateInsertAssetTree = (targetNodeId, newTreeNode, assetTreeArr) =>
    assetTreeArr
    |> Js.Array.map(({nodeId, children} as treeNode) =>
         isIdEqual(nodeId, targetNodeId) ?
           {
             ...treeNode,
             children:
               children |> Js.Array.copy |> ArrayService.push(newTreeNode),
           } :
           {
             ...treeNode,
             children:
               _iterateInsertAssetTree(targetNodeId, newTreeNode, children),
           }
       );

  _iterateInsertAssetTree(targetNodeId, newTreeNode, [|assetTreeRoot|])
  /* TODO fix: first is root??? */
  |> (assetTreeArr => assetTreeArr |> ArrayService.unsafeGetFirst);
};

let rec _isRemovedTreeNodeBeTargetParent = (targetNodeId, removedTreeNode) =>
  isIdEqual(targetNodeId, removedTreeNode.nodeId) ?
    true :
    removedTreeNode.children
    |> WonderCommonlib.ArrayService.reduceOneParam(
         (. result, child) =>
           result ?
             true : _isRemovedTreeNodeBeTargetParent(targetNodeId, child),
         false,
       );

let _isTargetTreeNodeBeRemovedParent = (targetTreeNode, removedNodeId) =>
  targetTreeNode.children
  |> Js.Array.filter(child => isIdEqual(child.nodeId, removedNodeId))
  |> Js.Array.length >= 1 ?
    true : false;

let getChildrenNameAndIdArr =
    (nodeId, fileTargetType, (editorState, engineState)) => {
  WonderLog.Contract.requireCheck(
    () =>
      WonderLog.(
        Contract.(
          test(
            Log.buildAssertMessage(
              ~expect={j|the parent asset node type should be Folder|j},
              ~actual={j|not|j},
            ),
            () =>
            editorState
            |> AssetTreeRootEditorService.getAssetTreeRoot
            |> OptionService.unsafeGet
            |> getSpecificTreeNodeById(nodeId)
            |> OptionService.unsafeGet
            |> (({type_}) => type_ == Folder |> assertTrue)
          )
        )
      ),
    StateEditorService.getStateIsDebug(),
  );

  editorState
  |> AssetTreeRootEditorService.getAssetTreeRoot
  |> OptionService.unsafeGet
  |> getSpecificTreeNodeById(nodeId)
  |> OptionService.unsafeGet
  |> (
    ({children}: assetTreeNodeType) =>
      children
      |> Js.Array.filter(({type_ as childType}: assetTreeNodeType) =>
           childType === fileTargetType
         )
      |> Js.Array.map(({nodeId as currentNodeId, type_}: assetTreeNodeType) => {
           let name =
             editorState
             |> AssetNodeUtils.handleSpeficFuncByAssetNodeType(
                  type_,
                  (
                    AssetFolderNodeMapEditorService.getFolderName(
                      currentNodeId,
                    ),
                    AssetJsonNodeMapEditorService.getJsonBaseName(
                      currentNodeId,
                    ),
                    OperateTextureLogicService.getTextureBaseName(
                      currentNodeId,
                    ),
                    AssetMaterialNodeMapLogicService.getMaterialBaseName(
                      currentNodeId,
                      engineState,
                    ),
                    AssetWDBNodeMapEditorService.getWDBBaseName(
                      currentNodeId,
                    ),
                  ),
                );

           (name, nodeId);
         })
  );
};

let _isTargetTreeNodeHasSameNameChild =
    (targetNodeId, removedNodeId, (editorState, engineState)) => {
  let {type_}: assetTreeNodeType =
    editorState
    |> AssetTreeRootEditorService.getAssetTreeRoot
    |> OptionService.unsafeGet
    |> getSpecificTreeNodeById(removedNodeId)
    |> OptionService.unsafeGet;

  let removedNodeName =
    editorState
    |> AssetNodeUtils.handleSpeficFuncByAssetNodeType(
         type_,
         (
           AssetFolderNodeMapEditorService.getFolderName(removedNodeId),
           AssetJsonNodeMapEditorService.getJsonBaseName(removedNodeId),
           OperateTextureLogicService.getTextureBaseName(removedNodeId),
           AssetMaterialNodeMapLogicService.getMaterialBaseName(
             removedNodeId,
             engineState,
           ),
           AssetWDBNodeMapEditorService.getWDBBaseName(removedNodeId),
         ),
       );

  getChildrenNameAndIdArr(targetNodeId, type_, (editorState, engineState))
  |> Js.Array.map(((name, id)) => name)
  |> Js.Array.includes(removedNodeName) ?
    {
      ConsoleUtils.warn("the folder can't has the same name !");

      true;
    } :
    false;
};

let isTreeNodeRelationError =
    (targetNodeId, removedNodeId, (editorState, engineState)) =>
  isIdEqual(targetNodeId, removedNodeId) ?
    true :
    _isRemovedTreeNodeBeTargetParent(
      targetNodeId,
      editorState
      |> AssetTreeRootEditorService.unsafeGetAssetTreeRoot
      |> getSpecificTreeNodeById(removedNodeId)
      |> OptionService.unsafeGet,
    ) ?
      true :
      _isTargetTreeNodeBeRemovedParent(
        editorState
        |> AssetTreeRootEditorService.unsafeGetAssetTreeRoot
        |> getSpecificTreeNodeById(targetNodeId)
        |> OptionService.unsafeGet,
        removedNodeId,
      )
      || _isTargetTreeNodeHasSameNameChild(
           targetNodeId,
           removedNodeId,
           (editorState, engineState),
         );

let rec iterateNameArrBuildNewName = (name, childrenNameArr) =>
  childrenNameArr |> Js.Array.includes(name) ?
    childrenNameArr
    |> iterateNameArrBuildNewName(FileNameService.buildNameSucc(name)) :
    name;

let getUniqueTreeNodeName =
    (name, fileTargetType, nodeId, (editorState, engineState)) =>
  switch (nodeId) {
  | None => name
  | Some(nodeId) =>
    (editorState, engineState)
    |> getChildrenNameAndIdArr(nodeId, fileTargetType)
    |> Js.Array.map(((name, id)) => name)
    |> iterateNameArrBuildNewName(name)
  };