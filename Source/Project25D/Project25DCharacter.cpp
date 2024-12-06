// Copyright Epic Games, Inc. All Rights Reserved.

#include "Project25DCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AProject25DCharacter

AProject25DCharacter::AProject25DCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0, 1, 0)); // Constrain movement to X-Z plane
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
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	
	CameraBoom->SetWorldRotation(FRotator(0.0f, -90.0f, 0.0f)); // Side-scroller perspective
	CameraBoom->TargetArmLength = 500.0f; // Adjust distance as needed

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AProject25DCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AProject25DCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AProject25DCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AProject25DCharacter::Look);

		//ragdoll
		EnhancedInputComponent->BindAction(RagdollAction, ETriggerEvent::Completed, this, &AProject25DCharacter::TriggerRagdoll);

	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AProject25DCharacter::TriggerRagdoll()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("TriggerRagdoll called!"));

	USkeletalMeshComponent* SkeletalMesh = GetMesh();
	if (SkeletalMesh)
	{
		// Enable physics only for the legs (bones below the pelvis)
		SkeletalMesh->SetAllBodiesBelowSimulatePhysics(TEXT("pelvis"), true);

		// Keep the upper body kinematic
		SkeletalMesh->SetAllBodiesBelowPhysicsBlendWeight(TEXT("spine_01"), 0.0f);

		// Set collision profile for the lower body
		SkeletalMesh->SetCollisionProfileName(TEXT("Ragdoll"));

		// Disable character movement
		GetCharacterMovement()->DisableMovement();
	}
}




void AProject25DCharacter::Move(const FInputActionValue& Value)
{

	// Input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Define a fixed forward direction for 2.5D movement (e.g., X-axis)
		const FVector ForwardDirection = FVector(1, 0, 0); // Move along X-axis only

		// Add movement input for constrained directions
		AddMovementInput(ForwardDirection, MovementVector.X); // Move left or right

		// Smoothly rotate character based on movement direction
		if (MovementVector.X != 0)	
		{
			// Target rotation: face right (0 degrees) or left (180 degrees)
			FRotator TargetRotation = FRotator(0.0f, (MovementVector.X > 0) ? 0.0f : 180.0f, 0.0f);

			// Smoothly interpolate current rotation to target rotation
			FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, GetWorld()->GetDeltaSeconds(), 5.0f); // Interpolation speed is 10.0f

			// Apply the new rotation
			SetActorRotation(NewRotation);
		}
	}
}


void AProject25DCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AProject25DCharacter::PlayAnimationDirectly()
{
	if (USkeletalMeshComponent* SkeletalMesh = GetMesh())
	{
		UAnimInstance* AnimInstance = SkeletalMesh->GetAnimInstance();

		if (AnimInstance && Retarget_Standing_React_Death_Left_1_)
		{
			// Play animation with no automatic blend-out and hold the final pose
			AnimInstance->PlaySlotAnimationAsDynamicMontage(
				Retarget_Standing_React_Death_Left_1_, // Animation to play
				"DefaultSlot",                        // Slot to play it in
				0.5f,                                 // Blend in time
				0.0f,                                 // Blend out time (0 ensures it holds the last frame)
				1.0f,                                 // Play rate
				1                                     // Loop count (1 = play once)
			);

			// Stop further updates from the animation blueprint
			SkeletalMesh->bPauseAnims = true;

			UE_LOG(LogTemp, Log, TEXT("Playing death animation and holding final pose."));
		}
	}
}

