// Copyright Epic Games, Inc. All Rights Reserved.

#include "Project5Character.h"

#include "DrawDebugHelpers.h"
#include "Project5Projectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "PickHint.h"
#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AProject5Character

AProject5Character::AProject5Character()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(false); // otherwise won't be visible in the multiplayer
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 20.0f);

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.

	// Create VR Controllers.
	R_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("R_MotionController"));
	R_MotionController->MotionSource = FXRMotionControllerBase::RightHandSourceId;
	R_MotionController->SetupAttachment(RootComponent);
	L_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("L_MotionController"));
	L_MotionController->SetupAttachment(RootComponent);

	// Create a gun and attach it to the right-hand VR controller.
	// Create a gun mesh component
	VR_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VR_Gun"));
	VR_Gun->SetOnlyOwnerSee(false); // otherwise won't be visible in the multiplayer
	VR_Gun->bCastDynamicShadow = false;
	VR_Gun->CastShadow = false;
	VR_Gun->SetupAttachment(R_MotionController);
	VR_Gun->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	VR_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("VR_MuzzleLocation"));
	VR_MuzzleLocation->SetupAttachment(VR_Gun);
	VR_MuzzleLocation->SetRelativeLocation(FVector(0.000004, 53.999992, 10.000000));
	VR_MuzzleLocation->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f)); // Counteract the rotation of the VR gun model.

	PickGunCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PickGunCollisionSphere"));
	PickGunCollision->SetSphereRadius(100);
	PickGunCollision->SetCollisionResponseToAllChannels(ECR_Overlap);
	PickGunCollision->SetupAttachment(RootComponent);
	PickGunCollision->SetVisibleFlag(false);

	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComp"));
	GunSwitchUICompo = CreateDefaultSubobject<UWidgetComponent>(TEXT("GunSwitchUICompo"));
	static ConstructorHelpers::FClassFinder<UUserWidget> SwitchUIBP(
		TEXT("WidgetBlueprint'/Game/UMG/PickHint.PickHint_C'"));
	if (SwitchUIBP.Succeeded())
	{
		GunSwitchUIWidget = SwitchUIBP.Class;
	}

	if (GunSwitchUICompo != nullptr)
	{
		GunSwitchUICompo->AttachToComponent(Scene, FAttachmentTransformRules::KeepRelativeTransform);
		GunSwitchUICompo->SetVisibility(true);
	}
	CurrentActor = "AR";
}

void AProject5Character::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true),
	                          TEXT("GripPoint"));

	// Show or hide the two versions of the gun based on whether or not we're using motion controllers.
	if (bUsingMotionControllers)
	{
		VR_Gun->SetHiddenInGame(false, true);
		Mesh1P->SetHiddenInGame(true, true);
	}
	else
	{
		VR_Gun->SetHiddenInGame(true, true);
		Mesh1P->SetHiddenInGame(false, true);
	}

	PickGunCollision->OnComponentBeginOverlap.AddDynamic(this, &AProject5Character::OnPickOverlapBegin);
	PickGunCollision->OnComponentEndOverlap.AddDynamic(this, &AProject5Character::OnPickOverlapEnd);

	if (GunSwitchUIWidget != nullptr)
	{
		UPickHint* Widget = CreateWidget<UPickHint>(GetWorld(), GunSwitchUIWidget);
		if (Widget != nullptr)
		{
			Widget->AddToViewport();
			if (GunSwitchUICompo != nullptr)
			{
				GunSwitchUICompo->SetWidget(Widget);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AProject5Character::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AProject5Character::OnFire);
	PlayerInputComponent->BindAction("Switch", IE_Pressed, this, &AProject5Character::SwitchGun);
	// Enable touchscreen input
	EnableTouchscreenMovement(PlayerInputComponent);

	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AProject5Character::OnResetVR);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AProject5Character::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AProject5Character::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AProject5Character::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AProject5Character::LookUpAtRate);
}

void AProject5Character::SwitchGun()
{
	UE_LOG(LogFPChar, Log, TEXT("Switch gun using F on keyboard"));
	if (WaitForPickSkeleton.Num() > 0)
	{
		USkeletalMesh* NewGun = WaitForPickSkeleton[WaitForPickSkeleton.Num() - 1];
		FP_Gun->SetSkeletalMesh(NewGun);
		CurrentActor = WaitForPickActor[WaitForPickActor.Num() - 1]->Tags[1].ToString();
	}
}

void AProject5Character::OnFire()
{
	// try and fire a projectile
	if (ProjectileClass != nullptr)
	{
		AProject5Projectile* pActor;
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			if (bUsingMotionControllers)
			{
				const FRotator SpawnRotation = VR_MuzzleLocation->GetComponentRotation();
				const FVector SpawnLocation = VR_MuzzleLocation->GetComponentLocation();
				World->SpawnActor<AProject5Projectile>(
					ProjectileClass, SpawnLocation, SpawnRotation);
			}
			else
			{
				const FRotator SpawnRotation = GetControlRotation();
				// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
				const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr)
					                               ? FP_MuzzleLocation->GetComponentLocation()
					                               : GetActorLocation()) + SpawnRotation.RotateVector(GunOffset);


				// spawn the projectile at the muzzle
				FTransform SpawnLocAndRotation;
				SpawnLocAndRotation.SetLocation(SpawnLocation);
				SpawnLocAndRotation.SetRotation(SpawnRotation.Quaternion());
				pActor = World->SpawnActorDeferred<AProject5Projectile>(ProjectileClass, SpawnLocAndRotation);
				if (pActor)
				{
					pActor->GunType = CurrentActor;
					pActor->SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					pActor->InitLocation = SpawnLocation;
					pActor->InitRotation = SpawnRotation;
					pActor->SetInstigator(this);
					UGameplayStatics::FinishSpawningActor(pActor, FTransform(SpawnRotation, SpawnLocation));
				}
			}
		}
	}

	// try and play the sound if specified
	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// try and play a firing animation if specified
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void AProject5Character::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AProject5Character::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == true)
	{
		return;
	}
	if ((FingerIndex == TouchItem.FingerIndex) && (TouchItem.bMoved == false))
	{
		OnFire();
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AProject5Character::OnPickOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor,
                                            class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                            const FHitResult& SweepResult)
{
	if (OtherActor->ActorHasTag("Weapon"))
	{
		TArray<USkeletalMeshComponent*> SkeletalMeshList;
		OtherActor->GetComponents(SkeletalMeshList);
		if (SkeletalMeshList.Num() != 0)
		{
			WaitForPickSkeleton.AddUnique(SkeletalMeshList[0]->SkeletalMesh);
			WaitForPickActor.AddUnique(OtherActor);
		}
	}
	if (WaitForPickActor.Num() > 0 && GunSwitchUICompo != nullptr)
	{
		UPickHint* Widget = Cast<UPickHint>(GunSwitchUICompo->GetUserWidgetObject());
		if (Widget != nullptr)
		{
			Widget->OnNextWeaponChange(WaitForPickActor[WaitForPickActor.Num() - 1]->GetName());
		}
	}
}

void AProject5Character::OnPickOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor,
                                          class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor->ActorHasTag("Weapon"))
	{
		TArray<USkeletalMeshComponent*> SkeletalMeshList;
		OtherActor->GetComponents(SkeletalMeshList);
		if (SkeletalMeshList.Num() != 0)
		{
			WaitForPickSkeleton.Remove(SkeletalMeshList[0]->SkeletalMesh);
			WaitForPickActor.Remove(OtherActor);
		}
	}
	if (WaitForPickActor.Num() == 0 && GunSwitchUICompo != nullptr)
	{
		UPickHint* Widget = Cast<UPickHint>(GunSwitchUICompo->GetUserWidgetObject());
		if (Widget != nullptr)
		{
			Widget->OnNextWeaponChange("");
		}
	}
}

void AProject5Character::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	TouchItem.bIsPressed = false;
}

void AProject5Character::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AProject5Character::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AProject5Character::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AProject5Character::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

bool AProject5Character::EnableTouchscreenMovement(class UInputComponent* PlayerInputComponent)
{
	if (FPlatformMisc::SupportsTouchInput() || GetDefault<UInputSettings>()->bUseMouseForTouch)
	{
		PlayerInputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AProject5Character::BeginTouch);
		PlayerInputComponent->BindTouch(EInputEvent::IE_Released, this, &AProject5Character::EndTouch);

		//Commenting this out to be more consistent with FPS BP template.
		//PlayerInputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AProject5Character::TouchUpdate);
		return true;
	}

	return false;
}
