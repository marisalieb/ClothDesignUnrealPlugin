#include "Misc/AutomationTest.h"
#include "ClothDesignToolkit.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClothDesignToolkitTest,
    "ClothDesignToolkit.SelectionAndButton",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClothDesignToolkitTest::RunTest(const FString& Parameters)
{
    FClothDesignToolkit Toolkit;

    // 1) cloth mesh selection
    {
        USkeletalMesh* TestMesh = NewObject<USkeletalMesh>();
        FAssetData AssetData(TestMesh);

        Toolkit.OnClothMeshSelected(AssetData);
        FString Path = Toolkit.GetSelectedClothMeshPath();

        TestEqual(TEXT("SelectedClothMesh should match the asset"), Path, TestMesh->GetPathName());
    }

    // 2) collision mesh selection
    {
        UStaticMesh* TestMesh = NewObject<UStaticMesh>();
        FAssetData AssetData(TestMesh);

        Toolkit.OnCollisionMeshSelected(AssetData);
        FString Path = Toolkit.GetSelectedCollisionMeshPath();

        TestEqual(TEXT("SelectedCollisionMesh should match the asset"), Path, TestMesh->GetPathName());
    }
    
    // 3) texture selection
    {
        UMaterial* TestMaterial = NewObject<UMaterial>(GetTransientPackage(), TEXT("UnitTest_Material"), RF_Transient);
        TestNotNull(TEXT("NewObject<UMaterial> should succeed"), TestMaterial);

        TStrongObjectPtr<UMaterial> Hold(TestMaterial);

        FAssetData AssetData(TestMaterial, true);

        Toolkit.OnTextileMaterialSelected(AssetData);
        const FString Path = Toolkit.GetSelectedTextileMaterialPath();

        TestEqual(TEXT("SelectedTextileMaterial should match the asset"), Path, TestMaterial->GetPathName());
    }
    
    // 4) open 2d window button
    {
        FReply Reply = Toolkit.OnOpen2DWindowClicked();
        TestTrue(TEXT("OnOpen2DWindowClicked should return handled"), Reply.IsEventHandled());
    }


    return true;
}



