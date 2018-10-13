open Wonder_jest;

open Expect;

open Expect.Operators;

open Sinon;

let _ =
  describe("controller header add gameObject", () => {
    let sandbox = getSandboxDefaultVal();

    beforeEach(() => {
      sandbox := createSandbox();
      MainEditorSceneTool.initState(~sandbox, ());
      MainEditorSceneTool.createDefaultScene(
        sandbox,
        MainEditorSceneTool.setFirstBoxToBeCurrentSceneTreeNode,
      );

      ControllerTool.stubRequestAnimationFrame(
        createEmptyStubWithJsObjSandbox(sandbox),
      );
      ControllerTool.run();
    });
    afterEach(() => restoreSandbox(refJsObjToSandbox(sandbox^)));

    describe("test add box", () => {
      test("test add one box", () => {
        MainEditorSceneTreeHeaderTool.addBox();

        StateEngineService.unsafeGetState()
        |> GameObjectUtils.getChildren(MainEditorSceneTool.unsafeGetScene())
        |> Js.Array.length
        |> expect == 5;
      });
      test("test add two boxes", () => {
        MainEditorSceneTreeHeaderTool.addBox();
        MainEditorSceneTreeHeaderTool.addBox();

        StateEngineService.unsafeGetState()
        |> GameObjectUtils.getChildren(MainEditorSceneTool.unsafeGetScene())
        |> Js.Array.length
        |> expect == 6;
      });
      describe("test scene tree snapshot", () =>
        test("test add one box", () => {
          MainEditorSceneTreeHeaderTool.addBox();

          BuildComponentTool.buildSceneTree(
            TestTool.buildAppStateSceneGraphFromEngine(),
          )
          |> ReactTestTool.createSnapshotAndMatch;
        })
      );
    });

    describe("test add emptyGameObject", () => {
      test("test add one emptyGameObject", () => {
        MainEditorSceneTreeHeaderTool.addEmptyGameObject();

        StateEngineService.unsafeGetState()
        |> GameObjectUtils.getChildren(MainEditorSceneTool.unsafeGetScene())
        |> Js.Array.length
        |> expect == 5;
      });
      describe("test scene tree snanpshot", () =>
        test("test add one emptyGameObject", () => {
          MainEditorSceneTreeHeaderTool.addEmptyGameObject();

          BuildComponentTool.buildSceneTree(
            TestTool.buildAppStateSceneGraphFromEngine(),
          )
          |> ReactTestTool.createSnapshotAndMatch;
        })
      );
    });
  });