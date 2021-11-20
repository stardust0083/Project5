// Copyright Epic Games, Inc. All Rights Reserved.

#include "Project5Projectile.h"

#include <concrt.h>

#include "DrawDebugHelpers.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"

AProject5Projectile::AProject5Projectile()
{
	// Use a sphere as a simple collision representation
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->BodyInstance.SetCollisionProfileName("Projectile");
	CollisionComp->BodyInstance.SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComp->SetNotifyRigidBodyCollision(true);
	CollisionComp->BodyInstance.SetResponseToChannel(ECC_Pawn, ECR_Ignore);

	// set up a notification for when this component hits something blocking

	// Players can't walk on it
	CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComp->CanCharacterStepUpOn = ECB_No;
	// Set as root component
	CollisionComp->SetWorldLocation(InitLocation);
	CollisionComp->SetSimulatePhysics(true);
	RootComponent = CollisionComp;

	InitialLifeSpan = 60.0f;
	IsGrenade = false;
	GrenadeTrigger = false;
	this->SetActorTickEnabled(true);

	ExplosionRange = CreateDefaultSubobject<USphereComponent>(TEXT("SphereExplode"));
	ExplosionRange->InitSphereRadius(2.0f);
	ExplosionRange->SetupAttachment(RootComponent);
	ExplosionRange->BodyInstance.SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AProject5Projectile::BeginPlay()
{
	Super::BeginPlay();
	CollisionComp->OnComponentHit.AddDynamic(this, &AProject5Projectile::OnHit);
	if (GunType == "GL")
	{
		IsGrenade = true;
		//CollisionComp->SetSphereRadius(5.292534f);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,TEXT("GL"));
		float Mass = CollisionComp->GetMass();
		FVector FireDirection = InitRotation.Vector();
		float InitSpeed = 1000.f;
		CollisionComp->AddImpulse(Mass * FireDirection * InitSpeed);
		//ProjectileMovement->InitialSpeed=1000.f;
	}
	if (GunType == "AR" || GunType == "")
	{
		IsGrenade = false;
		this->SetActorScale3D(FVector(0.5f, 0.5f, 0.5f));
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,TEXT("AR"));
		float Mass = CollisionComp->GetMass();
		FVector FireDirection = InitRotation.Vector();
		float InitSpeed = 3000.f;
		CollisionComp->AddImpulse(Mass * FireDirection * InitSpeed);
	}
	if (GunType == "SG")
	{
		IsGrenade = false;
		//CollisionComp->SetSphereRadius(2.5f);
		this->SetActorScale3D(FVector(0.5f, 0.5f, 0.5f));
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,TEXT("AR"));
		float Mass = CollisionComp->GetMass();
		FVector FireDirection = InitRotation.Vector();
		float InitSpeed = 3000.f;
		CollisionComp->AddImpulse(Mass * FireDirection * InitSpeed);
	}
}

void AProject5Projectile::CheckExplosion()

{
	if (GrenadeTrigger)
	{
		TArray<AActor*> OutHits;
		ExplosionRange->SetSphereRadius(2000.f);
		ExplosionRange->GetOverlappingActors(OutHits, TSubclassOf<AActor>());
		DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRange->GetScaledSphereRadius(), 50, FColor::Cyan,
		                true);
		// loop through TArray
		for (auto& Hit : OutHits)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, (TEXT("Grenade %s"), Hit->GetName()));
			UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>((Hit)->GetRootComponent());

			if (MeshComp && Hit->IsRootComponentMovable())
			{
				// alternivly you can use  ERadialImpulseFalloff::RIF_Linear for the impulse to get linearly weaker as it gets further from origin.
				// set the float radius to 500 and the float strength to 2000.
				MeshComp->AddRadialImpulse(GetActorLocation(), 500.f, 2000.f, ERadialImpulseFalloff::RIF_Constant,
				                           true);
			}
		}
		ExplosionRange->SetSphereRadius(2.f);
	}
}

void AProject5Projectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                FVector NormalImpulse, const FHitResult& Hit)
{
	// Only add impulse and destroy projectile if we hit a physics
	if ((OtherActor != nullptr) && (OtherComp != nullptr))
	{
		if (!IsGrenade)
		{
			if (OtherComp->IsSimulatingPhysics())
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red,
				                                 (TEXT("Onhit%s %S"), OtherComp->GetName(), OtherActor->GetName()));
				OtherComp->AddImpulseAtLocation(GetVelocity() * 100.0f, GetActorLocation());
				Destroy();
			}
		}
		else
		{
			if (OtherActor->ActorHasTag("Enemy"))
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, (TEXT("Hit Enemy%s"), OtherActor->GetName()));
				GrenadeTrigger = true;
				CheckExplosion();
				Destroy();
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
				                                 FString::Printf(
					                                 TEXT("Velocity is %s"), *this->GetVelocity().ToString()));
				if (IsGrenade && this->GetVelocity().Size() < 200.0f)
				{
					GrenadeTrigger = true;
					CheckExplosion();
					Destroy();
				}
			}
		}
	}
}
