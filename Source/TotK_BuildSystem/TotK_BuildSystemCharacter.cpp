// Copyright Epic Games, Inc. All Rights Reserved.

#include "TotK_BuildSystemCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Grabber.h"

#include "DebgugHelper.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ATotK_BuildSystemCharacter

ATotK_BuildSystemCharacter::ATotK_BuildSystemCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void ATotK_BuildSystemCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	// Initialize grabber component once blueprint components have been created
	GrabberComponent = FindComponentByClass<UGrabber>();
}

//////////////////////////////////////////////////////////////////////////
// Input

void ATotK_BuildSystemCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATotK_BuildSystemCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATotK_BuildSystemCharacter::Look);

		// Grab and release
		EnhancedInputComponent->BindAction(GrabAction, ETriggerEvent::Started, this, &ATotK_BuildSystemCharacter::Grab);
		EnhancedInputComponent->BindAction(ReleaseAction, ETriggerEvent::Completed, this, &ATotK_BuildSystemCharacter::Release);

		// Rotate held objects
		EnhancedInputComponent->BindAction(RotateLeftAction, ETriggerEvent::Started, this, &ATotK_BuildSystemCharacter::RotateLeft);
		EnhancedInputComponent->BindAction(RotateRightAction, ETriggerEvent::Started, this, &ATotK_BuildSystemCharacter::RotateRight);
		EnhancedInputComponent->BindAction(RotateUpAction, ETriggerEvent::Started, this, &ATotK_BuildSystemCharacter::RotateUp);
		EnhancedInputComponent->BindAction(RotateDownAction, ETriggerEvent::Started, this, &ATotK_BuildSystemCharacter::RotateDown);

		// Move held objects
		EnhancedInputComponent->BindAction(MoveTowardsAction, ETriggerEvent::Started, this, &ATotK_BuildSystemCharacter::MoveTowards);
		EnhancedInputComponent->BindAction(MoveAwayAction, ETriggerEvent::Started, this, &ATotK_BuildSystemCharacter::MoveAway);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ATotK_BuildSystemCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ATotK_BuildSystemCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

// Grab objects
void ATotK_BuildSystemCharacter::Grab(const FInputActionValue& value)
{
	if (GrabberComponent) {
		bIsGrabbing = true;
		GrabberComponent->Grab();
	}
}

// Release held objects
void ATotK_BuildSystemCharacter::Release(const FInputActionValue& value)
{
	if (GrabberComponent) {
		bIsGrabbing = false;
		GrabberComponent->Release();
	}
}

// Rotate held objects to the left
void ATotK_BuildSystemCharacter::RotateLeft(const FInputActionValue& value)
{
	if (GrabberComponent && GrabberComponent->IsHoldingObject()) {
		GrabberComponent->RotateLeft();
	}
}

// Rotate held objects to the right
void ATotK_BuildSystemCharacter::RotateRight(const FInputActionValue& value)
{
	if (GrabberComponent && GrabberComponent->IsHoldingObject()) {
		GrabberComponent->RotateRight();
	}
}

// Rotate held objects up
void ATotK_BuildSystemCharacter::RotateUp(const FInputActionValue& value)
{
	if (GrabberComponent && GrabberComponent->IsHoldingObject()) {
		GrabberComponent->RotateUp();
	}
}

// Rotate held objects down
void ATotK_BuildSystemCharacter::RotateDown(const FInputActionValue& value)
{
	if (GrabberComponent && GrabberComponent->IsHoldingObject()) {
		GrabberComponent->RotateDown();
	}
}

// Move held objects towards player
void ATotK_BuildSystemCharacter::MoveTowards(const FInputActionValue& value)
{
	if (GrabberComponent && GrabberComponent->IsHoldingObject()) {
		GrabberComponent->MoveTowards();
	}
}

// Move held objects away from player
void ATotK_BuildSystemCharacter::MoveAway(const FInputActionValue& value)
{
	if (GrabberComponent && GrabberComponent->IsHoldingObject()) {
		GrabberComponent->MoveAway();
	}
}