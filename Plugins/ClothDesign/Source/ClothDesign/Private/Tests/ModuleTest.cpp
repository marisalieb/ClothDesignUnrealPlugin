#include "ClothDesignModule.h"
#include "ClothDesignCanvas.h"
#include "Misc/AutomationTest.h"

// since it's mainly a UI class, I didn't test a lot and just verified the output visually

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClothDesignModuleButtonTest,
	"ClothDesignModule.ButtonClicks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClothDesignModuleButtonTest::RunTest(const FString& Parameters)
{
	FClothDesignModule Module;
	
	Module.CanvasWidget = SNew(SClothDesignCanvas);
	
	Module.OnGenerateMeshClicked();
	Module.OnSewingClicked();
	Module.OnMergeMeshesClicked();

	if (!Module.CanvasWidget.IsValid())
	{
		AddError(TEXT("CanvasWidget is null after initialization."));
	}

	return true;
}


