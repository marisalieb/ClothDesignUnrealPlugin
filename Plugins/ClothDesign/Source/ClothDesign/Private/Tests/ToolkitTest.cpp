#include "Misc/AutomationTest.h"
#include "ClothDesignToolkit.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"


class FClothDesignToolkitTestHelper
{
public:
    static void CallOnClothMeshSelected(FClothDesignToolkit& Toolkit, const FAssetData& Asset)
    {
        Toolkit.OnClothMeshSelected(Asset);
    }
    static FString CallOnGetSelectedClothMeshPath(FClothDesignToolkit& Toolkit)
    {
        return Toolkit.GetSelectedClothMeshPath();
    }

    
    static void CallOnCollisionMeshSelected(FClothDesignToolkit& Toolkit, const FAssetData& Asset)
    {
        Toolkit.OnCollisionMeshSelected(Asset);
    }
    
    static FString CallOnGetSelectedCollisionMeshPath(FClothDesignToolkit& Toolkit)
    {
        return Toolkit.GetSelectedCollisionMeshPath();
    }

    
    static void CallOnTextileMaterialSelected(FClothDesignToolkit& Toolkit, const FAssetData& Asset)
    {
        Toolkit.OnTextileMaterialSelected(Asset);
    }
    static FString CallOnGetSelectedTextileMaterialPath(FClothDesignToolkit& Toolkit)
    {
        return Toolkit.GetSelectedTextileMaterialPath();
    }
    
    static void CallOnPresetSelected(FClothDesignToolkit& Toolkit, TSharedPtr<FPresetItem> Preset)
    {
        Toolkit.OnPresetSelected(Preset, ESelectInfo::Direct);
    }

    static FReply CallOnOpen2DWindowClicked(FClothDesignToolkit& Toolkit)
    {
        return Toolkit.OnOpen2DWindowClicked();
    }
};



IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClothToolkit_ClothMeshSelectionTest, 
    "ClothDesignToolkit.ClothMeshSelection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClothToolkit_ClothMeshSelectionTest::RunTest(const FString& Parameters)
{
    FClothDesignToolkit Toolkit;

    // Simulate selecting a skeletal mesh
    USkeletalMesh* TestMesh = NewObject<USkeletalMesh>();
    FAssetData AssetData(TestMesh);
    
    FClothDesignToolkitTestHelper::CallOnClothMeshSelected(Toolkit, AssetData);
    FString Path = FClothDesignToolkitTestHelper::CallOnGetSelectedClothMeshPath(Toolkit);

    TestEqual(TEXT("SelectedClothMesh should match the asset"), Path, TestMesh->GetPathName());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClothToolkit_CollisionMeshSelectionTest, 
    "ClothDesignToolkit.CollisionMeshSelection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClothToolkit_CollisionMeshSelectionTest::RunTest(const FString& Parameters)
{
    FClothDesignToolkit Toolkit;

    // Simulate selecting a static mesh
    UStaticMesh* TestMesh = NewObject<UStaticMesh>();
    FAssetData AssetData(TestMesh);

    FClothDesignToolkitTestHelper::CallOnCollisionMeshSelected(Toolkit, AssetData);
    FString Path = FClothDesignToolkitTestHelper::CallOnGetSelectedCollisionMeshPath(Toolkit);
    
    TestEqual(TEXT("SelectedCollisionMesh should match the asset"), Path, TestMesh->GetPathName());

    return true;
}

// IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClothToolkit_TextileMaterialSelectionTest, 
//     "ClothDesignToolkit.TextileMaterialSelection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
//
// bool FClothToolkit_TextileMaterialSelectionTest::RunTest(const FString& Parameters)
// {
//     FClothDesignToolkit Toolkit;
//
//     // Simulate selecting a material
//     UMaterialInterface* TestMaterial = NewObject<UMaterialInterface>();
//     FAssetData AssetData(TestMaterial);
//     
//     FClothDesignToolkitTestHelper::CallOnTextileMaterialSelected(Toolkit, AssetData);
//     FString Path = FClothDesignToolkitTestHelper::CallOnGetSelectedTextileMaterialPath(Toolkit);
//     
//     TestEqual(TEXT("SelectedTextileMaterial should match the asset"), Path, TestMaterial->GetPathName());
//
//     return true;
// }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FClothToolkit_TextileMaterialSelectionTest,
    "ClothDesignToolkit.TextileMaterialSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClothToolkit_TextileMaterialSelectionTest::RunTest(const FString& Parameters)
{
    FClothDesignToolkit Toolkit;

    // Use a concrete subclass of UMaterialInterface
    UMaterial* TestMaterial = NewObject<UMaterial>(GetTransientPackage(), TEXT("UnitTest_Material"), RF_Transient);
    TestNotNull(TEXT("NewObject<UMaterial> should succeed"), TestMaterial);

    // Keep it alive for the duration of the test (avoid GC surprises)
    TStrongObjectPtr<UMaterial> Hold(TestMaterial);

    // Wrap in FAssetData; fine for transient objects for this usage
    FAssetData AssetData(TestMaterial, /*bAllowInvalidAssetClass=*/true);

    // Call the private method via your helper (friend/wrapper as you’ve set up)
    FClothDesignToolkitTestHelper::CallOnTextileMaterialSelected(Toolkit, AssetData);

    const FString Path = FClothDesignToolkitTestHelper::CallOnGetSelectedTextileMaterialPath(Toolkit);

    TestEqual(TEXT("SelectedTextileMaterial should match the asset"),
              Path, TestMaterial->GetPathName());

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClothToolkit_Open2DWindowTest, 
    "ClothDesignToolkit.UI_Open2DWindow", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClothToolkit_Open2DWindowTest::RunTest(const FString& Parameters)
{
    FClothDesignToolkit Toolkit;

    // Test that clicking the button returns handled
    FReply Reply = FClothDesignToolkitTestHelper::CallOnOpen2DWindowClicked(Toolkit);
    TestTrue(TEXT("OnOpen2DWindowClicked should return handled"), Reply.IsEventHandled());

    return true;
}
