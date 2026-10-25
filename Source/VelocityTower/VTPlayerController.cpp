#include "VTPlayerController.h"
#include "VTMainHUDWidget.h"
#include "VTTowerGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"

AVTPlayerController::AVTPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableTouchEvents = true;
	bEnableMouseOverEvents = true;
	bAutoManageActiveCameraTarget = false;
}

void AVTPlayerController::BeginPlay()
{
	Super::BeginPlay();

	SetVirtualJoystickVisibility(false);

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	if (IsLocalController() && !MainHUDWidget)
	{
		MainHUDWidget = CreateWidget<UVTMainHUDWidget>(this, UVTMainHUDWidget::StaticClass());
		if (MainHUDWidget)
		{
			MainHUDWidget->AddToViewport(10);
		}
	}
}

void AVTPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputComponent) return;

	InputComponent->BindAxis(TEXT("MoveRight"), this, &AVTPlayerController::OnMoveRightAxis);
	InputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &AVTPlayerController::OnJumpPressed);
	InputComponent->BindAction(TEXT("Jump"), IE_Released, this, &AVTPlayerController::OnJumpReleased);

	InputComponent->BindTouch(IE_Pressed, this, &AVTPlayerController::OnTouchPressed);
	InputComponent->BindTouch(IE_Repeat, this, &AVTPlayerController::OnTouchMoved);
	InputComponent->BindTouch(IE_Released, this, &AVTPlayerController::OnTouchReleased);
}

void AVTPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// Direct polling for PC Gamepad & Keyboard so controller works even when Slate UI widgets were clicked!
	const float StickX = GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
	const float StickY = GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
	const bool bDpadL = IsInputKeyDown(EKeys::Gamepad_DPad_Left);
	const bool bDpadR = IsInputKeyDown(EKeys::Gamepad_DPad_Right);
	const bool bKbL = IsInputKeyDown(EKeys::A) || IsInputKeyDown(EKeys::Left);
	const bool bKbR = IsInputKeyDown(EKeys::D) || IsInputKeyDown(EKeys::Right);

	if (!bHasActiveTouch)
	{
		if (FMath::Abs(StickX) > 0.14f)
		{
			AnalogSteer = FMath::Clamp(StickX, -1.0f, 1.0f);
			bKeyLeft = (StickX < -0.14f);
			bKeyRight = (StickX > 0.14f);
			JoystickVisualX = AnalogSteer;
			JoystickVisualY = FMath::Clamp(-StickY, -1.0f, 1.0f);
		}
		else if (bDpadL || bDpadR || bKbL || bKbR)
		{
			bKeyLeft = bDpadL || bKbL;
			bKeyRight = bDpadR || bKbR;
			AnalogSteer = bKeyRight ? 1.0f : (bKeyLeft ? -1.0f : 0.0f);
			JoystickVisualX = AnalogSteer;
			JoystickVisualY = 0.0f;
		}
		else
		{
			JoystickVisualX = FMath::FInterpTo(JoystickVisualX, 0.0f, DeltaTime, 14.0f);
			JoystickVisualY = FMath::FInterpTo(JoystickVisualY, 0.0f, DeltaTime, 14.0f);
		}
	}

	const bool bGamepadJumpDown =
		IsInputKeyDown(EKeys::Gamepad_FaceButton_Bottom) ||
		IsInputKeyDown(EKeys::Gamepad_FaceButton_Right) ||
		IsInputKeyDown(EKeys::Gamepad_RightTrigger) ||
		IsInputKeyDown(EKeys::Gamepad_RightShoulder) ||
		IsInputKeyDown(EKeys::Gamepad_LeftShoulder) ||
		IsInputKeyDown(EKeys::SpaceBar) ||
		IsInputKeyDown(EKeys::W) ||
		IsInputKeyDown(EKeys::Up);

	if (bGamepadJumpDown && !bWasGamepadJumpDown)
	{
		TriggerJumpAction();
	}
	else if (!bGamepadJumpDown && bWasGamepadJumpDown)
	{
		ReleaseJumpAction();
	}
	bWasGamepadJumpDown = bGamepadJumpDown;

	// Developer Admin / Godmode Shortcut (F12 or G key in Editor)
	const bool bAdminKeyDown = IsInputKeyDown(EKeys::F12) || IsInputKeyDown(EKeys::G);
	if (bAdminKeyDown && !bWasAdminKeyDown)
	{
		if (MainHUDWidget)
		{
			MainHUDWidget->ToggleAdminGodModeModal();
		}
	}
	bWasAdminKeyDown = bAdminKeyDown;
}

void AVTPlayerController::TriggerJumpAction()
{
	AVTTowerGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AVTTowerGameMode>() : nullptr;
	if (!GM || GM->TowerRunState != EVTGameState::Play)
	{
		JumpBufferFrames = 0;
		bJumpHeld = false;
		return;
	}
	JumpBufferFrames = 12;
	bJumpHeld = true;
}

void AVTPlayerController::ReleaseJumpAction()
{
	bJumpHeld = false;
}

void AVTPlayerController::TriggerHapticFeedback(int32 DurationMs)
{
	const float Intensity = FMath::Clamp(float(DurationMs) / 60.0f, 0.15f, 1.0f);
	PlayDynamicForceFeedback(Intensity, float(DurationMs) / 1000.0f, true, true, true, true);
}

void AVTPlayerController::OnMoveRightAxis(float Value)
{
	if (!bHasActiveTouch && FMath::Abs(Value) > 0.14f)
	{
		AnalogSteer = FMath::Clamp(Value, -1.0f, 1.0f);
		bKeyLeft = (Value < -0.14f);
		bKeyRight = (Value > 0.14f);
	}
}

void AVTPlayerController::OnJumpPressed()
{
	TriggerJumpAction();
}

void AVTPlayerController::OnJumpReleased()
{
	ReleaseJumpAction();
}

void AVTPlayerController::OnTouchPressed(ETouchIndex::Type FingerIndex, FVector Location)
{
	AVTTowerGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AVTTowerGameMode>() : nullptr;
	if (!GM || GM->TowerRunState != EVTGameState::Play) return;

	int32 ViewX = 540, ViewY = 960;
	GetViewportSize(ViewX, ViewY);
	ViewX = FMath::Max(1, ViewX);
	ViewY = FMath::Max(1, ViewY);

	const float RelX = FMath::Clamp(Location.X / float(ViewX), 0.0f, 1.0f);
	const float RelY = FMath::Clamp(Location.Y / float(ViewY), 0.0f, 1.0f);

	TouchStartX = Location.X;
	TouchStartY = Location.Y;
	bHasActiveTouch = true;
	ActiveTouchRelX = RelX;
	ActiveFingerCount = FMath::Max(1, ActiveFingerCount + 1);

	const EVTControlMode Mode = GM->CurrentControlMode;
	if (Mode == EVTControlMode::VirtualJoystick)
	{
		if (RelX > 0.56f || ActiveFingerCount >= 2)
		{
			TriggerJumpAction();
		}
		else
		{
			JoystickVisualX = 0.0f;
			JoystickVisualY = 0.0f;
		}
	}
	else if (Mode == EVTControlMode::FullscreenSplit)
	{
		if (ActiveFingerCount >= 2 || RelY < 0.60f) TriggerJumpAction();
		if (RelX < 0.48f) { bKeyLeft = true; bKeyRight = false; AnalogSteer = -1.0f; JoystickVisualX = -1.0f; }
		else if (RelX > 0.52f) { bKeyRight = true; bKeyLeft = false; AnalogSteer = 1.0f; JoystickVisualX = 1.0f; }
	}
	else if (Mode == EVTControlMode::AutoBounce)
	{
		if (RelX < 0.5f) { bKeyLeft = true; bKeyRight = false; AnalogSteer = -1.0f; JoystickVisualX = -1.0f; }
		else { bKeyRight = true; bKeyLeft = false; AnalogSteer = 1.0f; JoystickVisualX = 1.0f; }
		if (ActiveFingerCount >= 2) TriggerJumpAction();
	}
	else if (Mode == EVTControlMode::OneThumbSlide)
	{
		const float RawCentered = (RelX - 0.5f) * 2.0f;
		AnalogSteer = FMath::Sign(RawCentered) * FMath::Pow(FMath::Abs(RawCentered), 1.30f);
		JoystickVisualX = AnalogSteer;
		bKeyLeft = (RawCentered < -0.06f);
		bKeyRight = (RawCentered > 0.06f);
		if (ActiveFingerCount >= 2) TriggerJumpAction();
	}
	else if (Mode == EVTControlMode::SwipeGestures)
	{
		if (ActiveFingerCount >= 2) TriggerJumpAction();
	}
	else
	{
		// Classic Buttons fallback zone touch
		if (ActiveFingerCount >= 2 || RelX > 0.65f || RelY < 0.62f)
		{
			TriggerJumpAction();
		}
		else if (RelX < 0.32f)
		{
			bKeyLeft = true; bKeyRight = false; AnalogSteer = -1.0f; JoystickVisualX = -1.0f;
		}
		else
		{
			bKeyRight = true; bKeyLeft = false; AnalogSteer = 1.0f; JoystickVisualX = 1.0f;
		}
	}
}

void AVTPlayerController::OnTouchMoved(ETouchIndex::Type FingerIndex, FVector Location)
{
	AVTTowerGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AVTTowerGameMode>() : nullptr;
	if (!GM || GM->TowerRunState != EVTGameState::Play) return;

	int32 ViewX = 540, ViewY = 960;
	GetViewportSize(ViewX, ViewY);
	ViewX = FMath::Max(1, ViewX);
	ViewY = FMath::Max(1, ViewY);

	const float RelX = FMath::Clamp(Location.X / float(ViewX), 0.0f, 1.0f);
	const float Dx = Location.X - TouchStartX;
	const float Dy = TouchStartY - Location.Y;
	ActiveTouchRelX = RelX;

	const EVTControlMode Mode = GM->CurrentControlMode;

	if (Mode == EVTControlMode::VirtualJoystick)
	{
		const float JoyRadiusPx = 68.0f;
		const float NormX = FMath::Clamp(Dx / JoyRadiusPx, -1.0f, 1.0f);
		const float NormY = FMath::Clamp((Location.Y - TouchStartY) / JoyRadiusPx, -1.0f, 1.0f);
		JoystickVisualX = NormX;
		JoystickVisualY = NormY;
		AnalogSteer = NormX;
		bKeyLeft = (NormX < -0.15f);
		bKeyRight = (NormX > 0.15f);

		if (Dy > 42.0f)
		{
			TriggerJumpAction();
			TouchStartY = Location.Y;
		}
	}
	else if (Mode == EVTControlMode::SwipeGestures)
	{
		if (Dy > 24.0f)
		{
			TriggerJumpAction();
			TouchStartY = Location.Y;
		}
		if (FMath::Abs(Dx) > 16.0f)
		{
			AnalogSteer = FMath::Clamp(Dx / 55.0f, -1.0f, 1.0f);
			JoystickVisualX = AnalogSteer;
			bKeyLeft = (Dx < -16.0f);
			bKeyRight = (Dx > 16.0f);
		}
	}
	else if (Mode == EVTControlMode::OneThumbSlide)
	{
		if (Dy > 28.0f)
		{
			TriggerJumpAction();
			TouchStartY = Location.Y;
		}
		const float RawCentered = (RelX - 0.5f) * 2.0f;
		AnalogSteer = FMath::Sign(RawCentered) * FMath::Pow(FMath::Abs(RawCentered), 1.30f);
		JoystickVisualX = AnalogSteer;
		bKeyLeft = (RawCentered < -0.06f);
		bKeyRight = (RawCentered > 0.06f);
	}
	else if (Mode == EVTControlMode::AutoBounce || Mode == EVTControlMode::FullscreenSplit)
	{
		if (Dy > 30.0f)
		{
			TriggerJumpAction();
			TouchStartY = Location.Y;
		}
		bKeyLeft = (RelX < 0.48f);
		bKeyRight = (RelX >= 0.52f);
		AnalogSteer = bKeyRight ? 1.0f : (bKeyLeft ? -1.0f : 0.0f);
		JoystickVisualX = AnalogSteer;
	}
}

void AVTPlayerController::OnTouchReleased(ETouchIndex::Type FingerIndex, FVector Location)
{
	AVTTowerGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AVTTowerGameMode>() : nullptr;
	ActiveFingerCount = FMath::Max(0, ActiveFingerCount - 1);

	if (GM && GM->TowerRunState == EVTGameState::Play &&
		(GM->CurrentControlMode == EVTControlMode::OneThumbSlide || GM->CurrentControlMode == EVTControlMode::SwipeGestures) &&
		ActiveFingerCount == 0)
	{
		TriggerJumpAction();
	}

	if (ActiveFingerCount == 0)
	{
		bKeyLeft = false;
		bKeyRight = false;
		bJumpHeld = false;
		AnalogSteer = 0.0f;
		JoystickVisualX = 0.0f;
		JoystickVisualY = 0.0f;
		bHasActiveTouch = false;
	}
}
