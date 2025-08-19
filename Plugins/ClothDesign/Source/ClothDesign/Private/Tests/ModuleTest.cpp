
#include "ClothDesignModule.h"
#include "ClothDesignCanvas.h"
#include "Misc/AutomationTest.h"



// since it's mostly a UI class, I didn't test a lot and just verified the output visually

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClothDesignModuleButtonTest,
	"ClothDesignModule.ButtonClicks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClothDesignModuleButtonTest::RunTest(const FString& Parameters)
{
	// Create the module
	FClothDesignModule Module;
	
	Module.CanvasWidget = SNew(SClothDesignCanvas);
	
	// Directly call private functions because it's a friend class
	Module.OnGenerateMeshClicked();
	Module.OnSewingClicked();
	Module.OnMergeMeshesClicked();

	// Check that CanvasWidget exists
	if (!Module.CanvasWidget.IsValid())
	{
		AddError(TEXT("CanvasWidget is null after initialization."));
	}

	return true;
}


