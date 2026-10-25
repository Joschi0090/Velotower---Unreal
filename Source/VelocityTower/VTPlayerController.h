#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VelocityTowerTypes.h"
#include "VTPlayerController.generated.h"

class UVTMainHUDWidget;

UCLASS()
class VELOCITYTOWER_API AVTPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AVTPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Input")
	void TriggerJumpAction();

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Input")
	void ReleaseJumpAction();

	UFUNCTION(BlueprintCallable, Category = "VelocityTower|Input")
	void TriggerHapticFeedback(int32 DurationMs = 20);

	// Input States
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Input")
	bool bKeyLeft = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Input")
	bool bKeyRight = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Input")
	bool bJumpHeld = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Input")
	int32 JumpBufferFrames = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Input")
	float AnalogSteer = 0.0f; // -1.0 (Left) .. +1.0 (Right)

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Input")
	float JoystickVisualX = 0.0f; // -1.0 .. +1.0 for subtle HUD knob rendering

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Input")
	float JoystickVisualY = 0.0f; // -1.0 .. +1.0 for subtle HUD knob rendering

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Input")
	bool bHasActiveTouch = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VelocityTower|Input")
	float ActiveTouchRelX = 0.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VelocityTower|UI")
	UVTMainHUDWidget* MainHUDWidget = nullptr;

protected:
	void OnMoveRightAxis(float Value);
	void OnJumpPressed();
	void OnJumpReleased();

	void OnTouchPressed(ETouchIndex::Type FingerIndex, FVector Location);
	void OnTouchMoved(ETouchIndex::Type FingerIndex, FVector Location);
	void OnTouchReleased(ETouchIndex::Type FingerIndex, FVector Location);

	float TouchStartX = 0.0f;
	float TouchStartY = 0.0f;
	int32 ActiveFingerCount = 0;
	bool bWasGamepadJumpDown = false;
	bool bWasAdminKeyDown = false;
};
